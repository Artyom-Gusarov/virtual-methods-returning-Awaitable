#pragma once

#include <coroutine>
#include <cstddef>

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
                          { a.await_suspend(h) } -> internal::await_suspend_result;
                      };

template <typename T, size_t MaxSize>
class Awaiter {
    using Base = AwaiterBase;

  public:
    template <AwaiterType<T> A>
        requires(sizeof(A) <= MaxSize && std::is_base_of_v<Base, A>)
    Awaiter(A&& awaiter) {
        static_assert(alignof(A) <= alignof(std::max_align_t));
        new (storage_) A(std::forward<A>(awaiter));
    }

    template <AwaiterType<T> A>
        requires(sizeof(A) <= MaxSize && std::is_base_of_v<Base, A>)
    Awaiter& operator=(A&& awaiter) noexcept {
        static_assert(alignof(A) <= alignof(std::max_align_t));
        reinterpret_cast<Base*>(storage_)->~Base();
        new (storage_) A(std::forward<A>(awaiter));
        return *this;
    }

    bool await_ready() {
        return reinterpret_cast<Base*>(storage_)->await_ready();
    }

    auto await_suspend(std::coroutine_handle<> handle) {
        return reinterpret_cast<Base*>(storage_)->await_suspend(handle);
    }

    T await_resume() {
        return reinterpret_cast<Base*>(storage_)->await_resume();
    }

  private:
    alignas(alignof(std::max_align_t)) std::byte storage_[MaxSize];
};

}  // namespace dynamic
