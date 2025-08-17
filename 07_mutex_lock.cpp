#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

static const int MAX = 10e8;
static double sum = 0;
std::mutex sum_mutex;

void concurrent_worker(int min, int max)
{
    for(int i = min; i < max; ++i)
    {
        // 对访问共享资源加锁
        sum_mutex.lock();
        sum += sqrt(i);
        sum_mutex.unlock();
    }
}

 /**
  * @brief 加入互斥锁后的并行改造
  * 运行结果：
  * concurreny conut: 4
  * Concurrent work took 93531 ms
  * Sum: 2.10819e+13
  * 答案正确，但是耗时加剧，多线程切换任务，频繁加锁解锁致性能损耗
  * 
  * @param min 
  * @param max 
  */
void concurrent_work(int min, int max)
{
    auto start_time = std::chrono::steady_clock::now();
    unsigned int concurreny_count = std::thread::hardware_concurrency();
    std::cout << "concurreny conut: " << concurreny_count << std::endl;

    std::vector<std::thread> threads;
    for(int i = 0; i < concurreny_count; i++)
    {
        int range = max / concurreny_count * (i + 1);
        threads.push_back(std::thread(concurrent_worker, min, range));
        min = range + 1;
    }

    for(auto& thread : threads)
    {
        thread.join();
    }

    auto end_time = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Concurrent work took " << ms << " ms" << std::endl;
    std::cout << "Sum: " << sum << std::endl;
}

int main()
{
    concurrent_work(0, MAX);

    return 0;
}