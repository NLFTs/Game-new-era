#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <map>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <random>
#include <iomanip>

// =================================================================
// 1. UTILITY & TRAITS (Metaprogramming)
// =================================================================

template <typename T>
struct is_storable {
    static const bool value = std::is_copy_constructible<T>::value && !std::is_abstract<T>::value;
};

// Custom Logger dengan Singleton Pattern
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string& message, const std::string& level = "INFO") {
        std::lock_guard<std::mutex> lock(logMutex);
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "[" << std::put_time(std::localtime(&now), "%H:%M:%S") << "] "
                  << "[" << level << "] " << message << std::endl;
    }

private:
    Logger() {}
    std::mutex logMutex;
};

// =================================================================
// 2. CORE ENGINE: MEMORY BLOCK & ALLOCATOR
// =================================================================

enum class BlockState { FREE, ALLOCATED, FRAGMENTED, RESERVED };

struct MemoryBlock {
    size_t id;
    size_t size;
    BlockState state;
    std::string owner;

    MemoryBlock(size_t i, size_t s) : id(i), size(s), state(BlockState::FREE), owner("NONE") {}
};

template <typename T>
class SmartResource {
private:
    T* data;
    std::function<void(T*)> deleter;

public:
    explicit SmartResource(T* p, std::function<void(T*)> d) : data(p), deleter(d) {}
    ~SmartResource() { if (data) deleter(data); }
    T* operator->() { return data; }
};

// =================================================================
// 3. CONCURRENCY: THREAD-SAFE TASK QUEUE
// =================================================================

template <typename T>
class SafeQueue {
private:
    std::vector<T> queue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push_back(std::move(value));
        cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty(); });
        T value = std::move(queue.front());
        queue.erase(queue.begin());
        return value;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }
};

// =================================================================
// 4. MEMORY MANAGER SYSTEM (The Heavyweight Part)
// =================================================================

class VirtualMemorySystem {
private:
    std::vector<std::unique_ptr<MemoryBlock>> heap;
    std::mutex systemMutex;
    size_t totalCapacity;
    size_t usedMemory;

public:
    VirtualMemorySystem(size_t capacity) : totalCapacity(capacity), usedMemory(0) {
        // Inisialisasi heap dengan blok besar
        heap.push_back(std::make_unique<MemoryBlock>(0, capacity));
    }

    bool allocate(size_t size, const std::string& requester) {
        std::lock_guard<std::mutex> lock(systemMutex);
        
        for (auto& block : heap) {
            if (block->state == BlockState::FREE && block->size >= size) {
                // Fragmentasi blok jika ukuran lebih besar
                if (block->size > size) {
                    size_t remainingSize = block->size - size;
                    size_t newId = heap.size();
                    block->size = size;
                    block->state = BlockState::ALLOCATED;
                    block->owner = requester;
                    
                    heap.push_back(std::make_unique<MemoryBlock>(newId, remainingSize));
                } else {
                    block->state = BlockState::ALLOCATED;
                    block->owner = requester;
                }
                
                usedMemory += size;
                Logger::getInstance().log("Allocated " + std::to_string(size) + " units for " + requester);
                return true;
            }
        }
        return false;
    }

    void deallocate(const std::string& requester) {
        std::lock_guard<std::mutex> lock(systemMutex);
        for (auto& block : heap) {
            if (block->owner == requester) {
                usedMemory -= block->size;
                block->state = BlockState::FREE;
                block->owner = "NONE";
            }
        }
        Logger::getInstance().log("Deallocated memory for " + requester);
    }

    void defragment() {
        std::lock_guard<std::mutex> lock(systemMutex);
        Logger::getInstance().log("Starting Defragmentation...", "CRITICAL");
        // Logika penggabungan blok (simulasi)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void displayStatus() {
        std::lock_guard<std::mutex> lock(systemMutex);
        std::cout << "\n--- SYSTEM STATUS ---" << std::endl;
        std::cout << "Usage: " << usedMemory << " / " << totalCapacity << std::endl;
        for (const auto& b : heap) {
            std::cout << "[Block " << b->id << " | " << b->size << " | " 
                      << (b->state == BlockState::FREE ? "FREE" : b->owner) << "] ";
        }
        std::cout << "\n---------------------\n" << std::endl;
    }
};

// =================================================================
// 5. WORKER PROCESSES
// =================================================================

class WorkerNode {
private:
    std::string id;
    VirtualMemorySystem& vms;
    std::thread workerThread;
    bool running;

public:
    WorkerNode(std::string name, VirtualMemorySystem& system) 
        : id(name), vms(system), running(true) {}

    void start() {
        workerThread = std::thread(&WorkerNode::process, this);
    }

    void stop() {
        running = false;
        if (workerThread.joinable()) workerThread.join();
    }

    void process() {
        std::default_random_engine generator;
        std::uniform_int_distribution<int> dist(50, 200);
        
        while (running) {
            int taskSize = dist(generator);
            if (vms.allocate(taskSize, id)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(dist(generator) * 10));
                vms.deallocate(id);
            } else {
                Logger::getInstance().log(id + " failed to allocate memory!", "WARNING");
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
};

// =================================================================
// 6. MAIN EXECUTION
// =================================================================

int main() {
    Logger::getInstance().log("Initializing Advanced Memory Manager...");

    VirtualMemorySystem globalVMS(1000);
    
    std::vector<std::unique_ptr<WorkerNode>> nodes;
    nodes.push_back(std::make_unique<WorkerNode>("Alpha", globalVMS));
    nodes.push_back(std::make_unique<WorkerNode>("Beta", globalVMS));
    nodes.push_back(std::make_unique<WorkerNode>("Gamma", globalVMS));

    for (auto& node : nodes) node->start();

    // Monitor Loop
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        globalVMS.displayStatus();
        if (i == 2) globalVMS.defragment();
    }

    Logger::getInstance().log("Shutting down nodes...");
    for (auto& node : nodes) node->stop();

    std::cout << "\nSimulasi selesai dengan sukses." << std::endl;
    return 0;
}