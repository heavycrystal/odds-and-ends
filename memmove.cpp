#include <iostream>
#include <cstring>
#include <chrono>
#include <random>
#include <thread>
#include <mutex>

static std::random_device hrng;
static std::mutex lock_stdout;
static bool canbegin = false;

static int processor(int id, uint64_t size, uint64_t count) {
    /*  workaround to prevent loop from being optimized out */
    while(canbegin == false) {
        std::cout << "";        
    }
    bool isgood = true;
    uint8_t* source =  new uint8_t[size];
    uint8_t* destination = new uint8_t[size];
    std::uniform_int_distribution<uint8_t> rand_uint8(0, UINT8_MAX);
    std::mt19937 engine;
    std::unique_lock<std::mutex> guard(lock_stdout, std::defer_lock);
    uint64_t seed = hrng();
    engine.seed(seed);
    for (uint64_t i = 0; i < size; i++) {
        source[i] = rand_uint8(engine);
    }
    auto start = std::chrono::system_clock::now();
    for(uint64_t i = 0; i < count/2; i++) {
        memset(destination, 0, sizeof(uint8_t) * size);
        memcpy(destination, source, sizeof(uint8_t) * size);
        memset(source, 0, sizeof(uint8_t) * size);
        memcpy(source, destination, sizeof(uint8_t) * size);
    }
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);

    guard.lock();
    std::cout << "[Thread #" << id + 1 << "] Finished " << count << " transfers of " << sizeof(uint8_t)*size << " byte memory block in " << duration.count() << " milliseconds.\n";
    if(duration.count() != 0) {
        std::cout << "[Thread #" << id + 1 << "] Transfers per second: " << count*1000/duration.count() << ".\n"; 
        std::cout << "[Thread #" << id + 1 << "] Memory bandwidth: " << count*1000*sizeof(uint8_t)*size/duration.count() << " bytes/second.\n";
    }
    std::cout << "[Thread #" << id + 1 << "] Verifying output...\n";
    guard.unlock();
    engine.seed(seed);
    rand_uint8.reset();
    for(uint64_t i = 0; i < size; i++) {
        if(rand_uint8(engine) != destination[i]) {
            guard.lock();
            std::cout << "[Thread #" << id + 1 << "] Verify failed! Wrong value found at index " << i << ".\n";
            guard.unlock();
            isgood = false;
        }
    }
    delete[] source;
    delete[] destination;
    if(isgood == true) {
        guard.lock();
        std::cout << "[Thread #" << id + 1 << "] Output OK.\n";
        guard.unlock();
        return 0;
    }
    return 1;
}

int main(void) {
    uint64_t size = 0;
    uint64_t count = 0;
    uint64_t threads_count = 0;

    std::cout << "Memory area size (in bytes)? ";
    std::cin >> size;
    std::cout << "Number of transfers? ";
    std::cin >> count;
    std::cout << "Threads to be spawned? ";
    std::cin >> threads_count;

    std::vector<std::thread> thread_array(threads_count);
    for(uint64_t i = 0; i < threads_count; i++) {
        std::cout << "Spawned thread #" << i + 1 << "\n";
        thread_array.at(i) = std::thread(processor, i, size, count);
    }
    canbegin = true;
    for(uint64_t i = 0; i < threads_count; i++) {
        thread_array.at(i).join();
    }
    std::cout << "Complete!\n";
    return 0;   
}

