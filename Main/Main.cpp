#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int g_sum = 0;
std::mutex m;

void Add()
{
    for (int i = 0; i < 100000; i++) {
        std::lock_guard<std::mutex> lock(m);
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
