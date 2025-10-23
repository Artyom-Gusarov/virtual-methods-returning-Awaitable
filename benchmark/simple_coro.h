#pragma once

#include <coroutine>
#include <memory>
#include <mutex>
#include <thread>

namespace simple {

class SharedData {
    class Awaiter {
      public:
        Awaiter(SharedData& data) : data_(data) {
        }

        bool await_ready() {
            data_.mutex_.lock();
            if (data_.ready_) {
                data_.mutex_.unlock();
            }
            return data_.ready_;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            data_.callerHandle_ = handle;
            data_.handleSetted_ = true;
            data_.mutex_.unlock();
        }

        int await_resume() {
            return data_.data_;
        }

      private:
        SharedData& data_;
    };

  public:
    Awaiter operator co_await() {
        return Awaiter{*this};
    }

    void setData(int data) {
        std::unique_lock lk(mutex_);
        data_ = data;
        ready_ = true;
        if (handleSetted_) {
            lk.unlock();
            callerHandle_.resume();
        }
    }

    void blockingWait() {
        std::unique_lock lk(mutex_);
        while (!ready_) {
            lk.unlock();
            std::this_thread::yield();
            lk.lock();
        }
    }

  private:
    std::mutex mutex_;
    std::coroutine_handle<> callerHandle_;
    int data_;
    bool ready_ = false;
    bool handleSetted_ = false;
};

class Future {
  public:
    Future(std::shared_ptr<SharedData> data) : data_(data) {
    }

    auto operator co_await() {
        return data_->operator co_await();
    }

    void blockingWait() {
        data_->blockingWait();
    }

  private:
    std::shared_ptr<SharedData> data_;
};

class Promise {
  public:
    Future get_return_object() {
        data_ = std::make_shared<SharedData>();
        return Future{data_};
    }

    std::suspend_never initial_suspend() {
        return {};
    }

    std::suspend_never final_suspend() noexcept {
        return {};
    }

    void unhandled_exception() {
        auto exception = std::current_exception();
        throw exception;
    }

    void return_value(int value) {
        data_->setData(value);
    }

  private:
    std::shared_ptr<SharedData> data_;
};

}  // namespace simple

template <typename... Args>
struct std::coroutine_traits<simple::Future, Args...> {
    using promise_type = simple::Promise;
};
