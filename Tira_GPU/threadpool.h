#pragma once

#include <vector>
#include <queue>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace tira {

    /** Thread Safe Priority Queue */
    template<class _T, class _Container = std::vector<_T>, class _Compare = std::less<_T>>
    class ThreadSafePriorityQueue : protected std::priority_queue<_T, _Container, _Compare> {
    private:
        std::condition_variable _cv;
        mutable std::mutex _mtx;

    public:
        using Lock = std::unique_lock<std::mutex>;
        using Base = std::priority_queue<_T, _Container, _Compare>;

        ThreadSafePriorityQueue() = default;
        ~ThreadSafePriorityQueue() { clear(); }
        ThreadSafePriorityQueue(ThreadSafePriorityQueue const&) = delete;
        ThreadSafePriorityQueue(ThreadSafePriorityQueue&&) = delete;
        ThreadSafePriorityQueue& operator=(ThreadSafePriorityQueue const&) = delete;
        ThreadSafePriorityQueue& operator=(ThreadSafePriorityQueue&&) = delete;

        bool empty() const {
            Lock lock(_mtx);
            return Base::empty();
        }

        size_t size() const {
            Lock lock(_mtx);
            return Base::size();
        }

        void clear() {
            Lock lock(_mtx);
            while (!Base::empty()) {
                Base::pop();
            }
        }

        /** enqueue, as push() in STL queue */
        void enqueue(_T const& obj) {
            Lock lock(_mtx);
            Base::push(obj);
            _cv.notify_one();
        }

        template <typename ...Args>
        void emplace(Args&& ...args) {
            Lock lock(_mtx);
            Base::emplace(std::forward<Args>(args)...);
            _cv.notify_one();
        }

        /** dequeue, combination of top() and pop() in STL queue */
        bool dequeue(_T& holder) {
            Lock lock(_mtx);
            if (Base::empty()) {
                return false;
            }
            else {
                holder = std::move(Base::top());
                Base::pop();
                return true;
            }
        }

        /** dequeue with timeout */
        template <class _Rep, class _Period>
        bool dequeue(_T& holder, const std::chrono::duration<_Rep, _Period>& timeout) {
            Lock lock(_mtx);
            if (_cv.wait_for(lock, timeout, [&] { return !Base::empty(); })) {
                holder = std::move(Base::top());
                Base::pop();
                return true;
            }
            else {
                return false;
            }
        }
    };

    /** Thread Safe Queue */
    template<class _T, class _Container = std::deque<_T>>
    class ThreadSafeQueue : protected std::queue<_T, _Container> {
    private:
        std::condition_variable _cv;
        mutable std::mutex _mtx;

    public:
        using Lock = std::unique_lock<std::mutex>;
        using Base = std::queue<_T, _Container>;

        ThreadSafeQueue() = default;
        ~ThreadSafeQueue() { clear(); }
        ThreadSafeQueue(ThreadSafeQueue const&) = delete;
        ThreadSafeQueue(ThreadSafeQueue&&) = delete;
        ThreadSafeQueue& operator=(ThreadSafeQueue const&) = delete;
        ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

        bool empty() const {
            Lock lock(_mtx);
            return Base::empty();
        }

        size_t size() const {
            Lock lock(_mtx);
            return Base::size();
        }

        void clear() {
            Lock lock(_mtx);
            while (!Base::empty()) {
                Base::pop();
            }
        }

        /** enqueue, as push() in STL queue */
        void enqueue(_T const& obj) {
            Lock lock(_mtx);
            Base::push(obj);
            _cv.notify_one();
        }

        template <typename ...Args>
        void emplace(Args&& ...args) {
            Lock lock(_mtx);
            Base::emplace(std::forward<Args>(args)...);
            _cv.notify_one();
        }

        /** dequeue, combination of top() and pop() in STL queue */
        bool dequeue(_T& holder) {
            Lock lock(_mtx);
            if (Base::empty()) {
                return false;
            }
            else {
                holder = std::move(Base::front());
                Base::pop();
                return true;
            }
        }

        /** dequeue with timeout */
        template <class _Rep, class _Period>
        bool dequeue(_T& holder, const std::chrono::duration<_Rep, _Period>& timeout) {
            Lock lock(_mtx);
            if (_cv.wait_for(lock, timeout, [&] { return !Base::empty(); })) {
                holder = std::move(Base::top());
                Base::pop();
                return true;
            }
            else {
                return false;
            }
        }
    };

    /** Thread Pool */
    class ThreadPool {
    public:
        ThreadPool(size_t);
        template<class F, class... Args>
        std::future<typename std::invoke_result<F, Args...>::type> enqueue(F&& f, Args&&... args);
        ~ThreadPool();
    private:
        std::vector< std::thread > workers;
        std::queue< std::function<void()> > tasks;

        std::mutex _mtx;
        std::condition_variable _cond;
        bool stop;
    };

    inline ThreadPool::ThreadPool(size_t size)
        : stop(false) {
        for (size_t i = 0; i < size; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->_mtx);
                        this->_cond.wait(lock, [this] {
                            return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    inline ThreadPool::~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(_mtx);
            stop = true;
        }
        _cond.notify_all();
        for (std::thread& worker : workers)
            worker.join();
    }

    template<class F, class... Args>
    std::future<typename std::invoke_result<F, Args...>::type> ThreadPool::enqueue(F&& f, Args&&... args)
    {
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(_mtx);
            tasks.emplace([task]() { (*task)(); });
        }
        _cond.notify_one();
        return res;
    }
} // namespace tira
