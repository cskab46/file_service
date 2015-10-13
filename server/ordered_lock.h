#include <mutex>
#include <condition_variable>
#include <queue>

class ordered_lock {
    std::queue<std::condition_variable>  cvar;
    std::mutex                           cvar_lock;
    bool                                 locked;
public:
    ordered_lock() : locked(false) {}
    void lock() {
        std::unique_lock<std::mutex> acquire(cvar_lock);
        if (locked) {
            cvar.emplace();
            cvar.back().wait(acquire);
        } else {
            locked = true;
        }
    }
    void unlock() {
        std::unique_lock<std::mutex> acquire(cvar_lock);
        if (cvar.empty()) {
            locked = false;
        } else {
            cvar.front().notify_one();
            cvar.pop();
        }
    }
};
