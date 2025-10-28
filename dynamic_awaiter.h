#pragma once

#include <coroutine>

namespace dynamic {

class AwaiterBase {
  public:
    virtual bool await_ready() = 0;

    virtual void await_suspend(std::coroutine_handle<> handle) = 0;

    virtual int await_resume() = 0;

    virtual ~AwaiterBase() = default;
};

namespace internal {

template <class R>
concept await_suspend_result = std::same_as<R, void> || std::same_as<R, bool>;

}  // namespace internal

template <class A, class T>
concept AwaiterType = requires(A a, std::coroutine_handle<> h) {
                          { a.await_ready() } -> std::same_as<bool>;
                          { a.await_resume() } -> std::same_as<T>;
                          {
                              a.await_suspend(h)
                              } -> internal::await_suspend_result;
                      };

template <typename T, size_t MaxSize, AwaiterType<T> Base = AwaiterBase>
class Awaiter {
  public:
    template <typename A>
        requires(sizeof(A) <= MaxSize && std::is_base_of_v<Base, A>)
    Awaiter(A&& awaiter) {
        new (storage_) A(std::forward<A>(awaiter));
    }

    template <typename A>
        requires(sizeof(A) <= MaxSize && std::is_base_of_v<Base, A>)
    Awaiter& operator=(A&& awaiter) noexcept {
        awaiter_.~Base();
        new (storage_) A(std::forward<A>(awaiter));
        return *this;
    }

    bool await_ready() {
        return awaiter_.await_ready();
    }

    auto await_suspend(std::coroutine_handle<> handle) {
        return awaiter_.await_suspend(handle);
    }

    T await_resume() {
        return awaiter_.await_resume();
    }

  private:
    std::byte storage_[MaxSize];
    Base& awaiter_ = *reinterpret_cast<Base*>(storage_);
};

}  // namespace dynamic
