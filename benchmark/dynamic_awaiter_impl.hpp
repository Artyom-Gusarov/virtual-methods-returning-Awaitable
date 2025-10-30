#pragma once

#include "simple_coro.h"

#ifdef TEST_DYNAMIC_AWAITER_LAMBDAS
#include "../dynamic_awaiter_lambdas.hpp"
#elif TEST_DYNAMIC_AWAITER_VIRTUAL_INTERFACE
#include "../dynamic_awaiter_virtual_interface.hpp"
#elif TEST_DYNAMIC_AWAITER_VIRTUAL_BASE
#include "../dynamic_awaiter_virtual_base.hpp"
#else
#error                                                                                                                 \
    "Define one of TEST_DYNAMIC_AWAITER_LAMBDAS, TEST_DYNAMIC_AWAITER_VIRTUAL_INTERFACE, TEST_DYNAMIC_AWAITER_VIRTUAL_BASE"
#endif

class SharedDataBase {
  protected:
#ifdef TEST_DYNAMIC_AWAITER_VIRTUAL_BASE
    using dAwaiter = dynamic::Awaiter<int, 24>;
#else
    using dAwaiter = dynamic::Awaiter<int, 16>;
#endif

  public:
    virtual dAwaiter asyncGet() = 0;
    virtual ~SharedDataBase() = default;
};

class SharedDataStorage : public SharedDataBase {
#ifdef TEST_DYNAMIC_AWAITER_VIRTUAL_BASE
    class Awaiter : public dynamic::AwaiterBase {
      public:
        Awaiter(simple::SharedData& data) : awaiterImpl_{data.operator co_await()} {
        }

        bool await_ready() final {
            return awaiterImpl_.await_ready();
        }

        void await_suspend(std::coroutine_handle<> handle) final {
            awaiterImpl_.await_suspend(handle);
        }

        int await_resume() final {
            return awaiterImpl_.await_resume();
        }

      private:
        decltype(std::declval<simple::SharedData>().operator co_await()) awaiterImpl_;
    };
#else
    class Awaiter {
      public:
        Awaiter(simple::SharedData& data) : awaiterImpl_{data.operator co_await()} {
        }

        bool await_ready() {
            return awaiterImpl_.await_ready();
        }

        void await_suspend(std::coroutine_handle<> handle) {
            awaiterImpl_.await_suspend(handle);
        }

        int await_resume() {
            return awaiterImpl_.await_resume();
        }

      private:
        decltype(std::declval<simple::SharedData>().operator co_await()) awaiterImpl_;
    };
#endif

  public:
    dAwaiter asyncGet() final {
        return Awaiter(data_);
    }

    void setData(int data) {
        data_.setData(data);
    }

  private:
    simple::SharedData data_;
};

template <typename TransformFunc>
class SharedDataTransform : public SharedDataBase {
#ifdef TEST_DYNAMIC_AWAITER_VIRTUAL_BASE
    class Awaiter : public dynamic::AwaiterBase {
      public:
        Awaiter(TransformFunc&& func, dAwaiter& data)
            : baseAwaiter_(data), transformFunc_(std::forward<TransformFunc>(func)) {
        }

        bool await_ready() final {
            return baseAwaiter_.await_ready();
        }

        void await_suspend(std::coroutine_handle<> handle) final {
            baseAwaiter_.await_suspend(handle);
        }

        int await_resume() final {
            return transformFunc_(baseAwaiter_.await_resume());
        }

      private:
        dAwaiter& baseAwaiter_;
        TransformFunc transformFunc_;
    };
#else
    class Awaiter {
      public:
        Awaiter(TransformFunc&& func, dAwaiter& data)
            : baseAwaiter_(data), transformFunc_(std::forward<TransformFunc>(func)) {
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
        TransformFunc transformFunc_;
    };
#endif

  public:
    SharedDataTransform(TransformFunc&& func, std::shared_ptr<SharedDataBase> baseData)
        : baseData_(baseData), baseAwaiter_(baseData_->asyncGet()), transformFunc_(std::forward<TransformFunc>(func)) {
    }

    dAwaiter asyncGet() final {
        return Awaiter(std::move(transformFunc_), baseAwaiter_);
    }

  private:
    std::shared_ptr<SharedDataBase> baseData_;
    dAwaiter baseAwaiter_;
    TransformFunc transformFunc_;
};
