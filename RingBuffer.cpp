#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>


// 检查写线程是否完成
std::atomic<bool> writerDone{false};

template <typename T>
class RingBuffer
{
public: 
    RingBuffer(int capacity)
    {
        // 将容量调整为2的幂次方，避免运算提高性能
        capacity_ = 1;
        while(capacity_ < capacity)
        {
            capacity_ <<= 1;
        }
        mask = capacity_ - 1;
        
        // 采用版本号来实现无锁数据结构
        this->elements = std::make_unique<std::atomic<VersionedElement>[]>(capacity_);
        for(size_t i = 0; i < capacity_; ++i)
        {
            VersionedElement element{T{}, 0};

            // store，向原子变量写入新k
            // memory_order_relaxed，，仅保证原子操作本身的原子性
            elements[i].store(element, std::memory_order_relaxed);
        }
    }
    
    // 非原子操作，会导致程序崩溃问题
    #if 0
    bool isFull()
    {
        // 使用掩码提高性能
        return ((writeIndex.load(std::memory_order_relaxed) + 1) & mask) == readIndex.load(std::memory_order_acquire);
    }
    
    bool isEmpty() 
    {
        return writeIndex.load(std::memory_order_acquire) == readIndex.load(std::memory_order_acquire);
    }
    #endif
    
    
    bool write(const T& value)
    {
        VersionedElement currentElement, newElement;
        size_t currentWriteIndex, nextWriteIndex;
        do {
            //load，读取原子变量的当前值
            currentWriteIndex = writeIndex.load(std::memory_order_relaxed);
           
            // 使用掩码提高性能
            nextWriteIndex = (currentWriteIndex + 1) & mask; 

            // Buffer is full
            if (nextWriteIndex == readIndex.load(std::memory_order_acquire))
            {
                return false; // Buffer is full
            }

            currentElement = elements[currentWriteIndex].load(std::memory_order_acquire);

            newElement.value = value;
            newElement.version = currentElement.version + 1; 

        // compare_exchange_weak，CAS操作
        // 仅当变量当前值与预期值相等时，才会写入新值
        } while(!elements[currentWriteIndex].compare_exchange_weak(
            currentElement, 
            newElement
        ));
        
        // store，向原子变量写入新值
        // memory_order_release，确保在写入writeIndex之前，所有对elements的修改都已经完成
        writeIndex.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    bool read(T& value)
    {
        VersionedElement currentElement, newElement;
        size_t currentReadIndex;

        do {
            currentReadIndex = readIndex.load(std::memory_order_relaxed);
            if(currentReadIndex == writeIndex.load(std::memory_order_acquire)) {
                return false; // Buffer is empty
            }

            currentElement = elements[currentReadIndex].load(std::memory_order_acquire);
            value = currentElement.value;

            newElement.value = T{};
            newElement.version = currentElement.version + 1; 
        } while(!elements[currentReadIndex].compare_exchange_weak(
            currentElement, 
            newElement
        ));

        // 使用掩码提高性能
        readIndex.store((currentReadIndex + 1) & mask, std::memory_order_release);
        return true;
    }   

private:
    struct VersionedElement
    {
        T value;
        int version; // 版本号
    };

    std::unique_ptr<std::atomic<VersionedElement>[]> elements;
    int capacity_;
    size_t mask;
    std::atomic<size_t> readIndex{0};
    std::atomic<size_t> writeIndex{0};
};

int main()
{
    RingBuffer<int> ringbuffer(1023);

    std::thread writer([&](){
        for(int i = 0; i < 1000000; ++i)
        {
            ringbuffer.write(i);
            std::cout << "Wrote: " << i << std::endl;
            // 减缓写入速度
            if(i % 100 == 0)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }

        // 写入操作
        writerDone.store(true, std::memory_order_release);
    });

    std::thread reader([&](){
        int value = -1;
        int count = 0;
        while(count < 1000000)
        {
            // 只有当成功读取时，count才++
            if(ringbuffer.read(value))
            {
                std::cout << "Read: " << value << "\n";
                count++;
            }
            // 读取操作
            else if(writerDone.load(std::memory_order_acquire))
            {
                break; // 写完且无数据可读，退出
            }
        }
    });

    writer.join();
    reader.join();

    return 0;
}