#pragma once

// stdlib
#include <condition_variable>
#include <mutex>
#include <queue>

namespace Auris::CAN {
    template <class T> class SharedQueue {
      public:
        SharedQueue() = default;
        ~SharedQueue() = default;

        void clear() {
            std::unique_lock<std::mutex> lock(_mutex);
            _queue = {};
        }

        bool empty() const {
            std::unique_lock<std::mutex> lock(_mutex);
            return _queue.empty();
        }

        void push(T t) {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push(t);
            _condvar.notify_one();
        }

        T pop() {
            std::unique_lock<std::mutex> lock(_mutex);
            while (_queue.empty()) {
                _condvar.wait(lock);
            }

            T val = _queue.front();
            _queue.pop();
            return val;
        }

      private:
        std::condition_variable _condvar{};
        mutable std::mutex _mutex{};
        std::queue<T> _queue{};
    };
} // namespace Auris::CAN
