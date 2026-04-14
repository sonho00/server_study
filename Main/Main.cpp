#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<int> g_sum = 0;

void Add()
{
    for (int i = 0; i < 100000; i++) {
        g_sum++;
    }
}

int main()
{
    std::vector<std::thread> workers;

    for (int i = 0; i < 10; i++) {
        workers.push_back(std::thread(Add));
    }

    for (auto& t : workers) {
        t.join();
    }

    std::cout << "Result: " << g_sum << std::endl;

    return 0;
}
