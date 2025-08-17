#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>

static const int MAX = 10e8;
static double sum = 0;

void worker(int min, int max)
{
    for(int i = min; i < max; ++i)
    {
        sum += sqrt(i);
    }
}

/**
 * @brief 串行任务
 * 运行结果：
 * Serial work took 3088 ms
 * Sum: 2.10819e+13
 * 
 */
void serial_work(int min, int max)
{
    auto start_time = std::chrono::steady_clock::now();
    worker(0, MAX);
    auto end_time = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Serial work took " << ms << " ms" << std::endl;
    std::cout << "Sum: " << sum << std::endl;
}

/**
 * @brief 并行改造
 * 运行结果:
 * concurreny conut: 4
 * Concurrent work took 4533 ms
 * Sum: 7.30805e+12
 * 
 * 运行时间更长，并且结果错误
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
        threads.push_back(std::thread(worker, min, range));
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