#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <type_traits>


class ThreadPool {
public:

    class Task {
    public:
        virtual ~Task() = default;
        virtual void run() noexcept = 0;
    };

    using PTask = std::unique_ptr<Task>;

    template<typename Fn>
    requires (std::is_nothrow_invocable_v<Fn>)
    void push(Fn &&fn) {
        class FnTask : public Task{
        public:
            FnTask(Fn &&fn):_fn(fn) {}
            virtual void run() noexcept {_fn();}
        protected:
            Fn _fn;
        };

        PTask t = std::make_unique<FnTask>(std::forward<Fn>(fn));
        {
            std::lock_guard _(_mx);
            _q.push(std::move(t));        
        }
        _cv.notify_one();
    }

    void start(std::size_t threads) {
        std::lock_guard _(_mx);
        while (_threads.size() < threads) {
            std::size_t idx = _threads.size();
            _exitFlags.push_back(nullptr);
            _threads.push_back(std::thread([this, idx]{
                worker(idx);
            }));
        }
    }

    void stop() {
        std::queue<PTask> done;
        std::vector<std::thread> threads;
        std::vector<bool *> exitFlags;
        {
            std::lock_guard _(_mx);
            std::swap(done, _q);
            std::swap(threads, _threads);
            std::swap(exitFlags, _exitFlags);
            _q.push(nullptr);
            
        }
        _cv.notify_all();
        auto cur = std::this_thread::get_id();
        for (std::size_t i = 0, cnt = threads.size(); i < cnt; ++i) {
            if (threads[i].get_id() == cur) {
                *exitFlags[i] = true;
                threads[i].detach();                
            } else {
                threads[i].join();
            }
        }
    }

    ~ThreadPool() {
        stop();
    }

protected:

    

    std::vector<std::thread> _threads;
    std::vector<bool *> _exitFlags;
    std::condition_variable _cv;
    std::mutex _mx;
    std::queue<PTask> _q;


    void worker(std::size_t idx) {
        bool exitFlg = false;
        _exitFlags[idx] = &exitFlg;
        std::unique_lock lk(_mx);
        while (true) {
            if (_q.empty()) {
                _cv.wait(lk);
            } else {                
                PTask &t = _q.front();
                if (t == nullptr) break;
                PTask u = std::move(t);
                _q.pop();
                lk.unlock();
                u->run();
                if (exitFlg) break;
                lk.lock();
            }
        }
    }

};
