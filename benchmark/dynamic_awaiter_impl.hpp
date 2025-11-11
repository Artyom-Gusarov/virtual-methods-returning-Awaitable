#pragma once

#include "../dynamic_awaiter.h"
#include "simple_coro.h"

class SharedDataBase {
  protected:
    using dAwaiter = dynamic::Awaiter<int, 16>;

  public:
    virtual dAwaiter asyncGet() = 0;
    virtual ~SharedDataBase() = default;
};

class SharedDataStorage : public SharedDataBase {
  public:
    dAwaiter asyncGet() final {
        return data_.operator co_await();
    }

    void setData(int data) {
        data_.setData(data);
    }

  private:
    simple::SharedData data_;
};

template <typename TransformFunc>
class SharedDataTransform : public SharedDataBase {
    class Awaiter {
      public:
        Awaiter(dAwaiter& baseAwaiter, TransformFunc& func) : baseAwaiter_(baseAwaiter), transformFunc_(func) {
        }

        bool await_ready() {
            return baseAwaiter_.await_ready();
        }

        void await_suspend(std::coroutine_handle<> handle) {
            baseAwaiter_.await_suspend(handle);
        }

        int await_resume() {
            return transformFunc_(baseAwaiter_.await_resume());
        }

      private:
        dAwaiter& baseAwaiter_;
        TransformFunc& transformFunc_;
    };

  public:
    template <typename F>
    SharedDataTransform(F&& func, std::shared_ptr<SharedDataBase> baseData)
        : baseData_(std::move(baseData)), baseAwaiter_(baseData_->asyncGet()), transformFunc_(std::forward<F>(func)) {
    }

    dAwaiter asyncGet() final {
        return Awaiter(baseAwaiter_, transformFunc_);
    }

  private:
    std::shared_ptr<SharedDataBase> baseData_;
    dAwaiter baseAwaiter_;
    TransformFunc transformFunc_;
};
