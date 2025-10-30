#pragma once

#include <coroutine>

namespace dynamic {

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

template <typename T, size_t MaxSize>
class Awaiter {
  public:
    template <AwaiterType<T> A>
        requires(sizeof(A) <= MaxSize &&
                 alignof(A) <= alignof(std::max_align_t))
    Awaiter(A&& awaiter) {
        new (storage_) A(std::forward<A>(awaiter));

        ready = [](void* storage) -> bool {
            return static_cast<A*>(storage)->await_ready();
        };
        suspend = [](void* storage, std::coroutine_handle<> handle) {
            static_cast<A*>(storage)->await_suspend(handle);
        };
        resume = [](void* storage) -> T {
            return static_cast<A*>(storage)->await_resume();
        };
    }

    template <AwaiterType<T> A>
        requires(sizeof(A) <= MaxSize &&
                 alignof(A) <= alignof(std::max_align_t))
    Awaiter& operator=(A&& awaiter) noexcept {
        // TODO destructor call
        new (storage_) A(std::forward<A>(awaiter));

        ready = [](void* storage) -> bool {
            return static_cast<A*>(storage)->await_ready();
        };
        suspend = [](void* storage, std::coroutine_handle<> handle) {
            static_cast<A*>(storage)->await_suspend(handle);
        };
        resume = [](void* storage) -> T {
            return static_cast<A*>(storage)->await_resume();
        };

        return *this;
    }

    bool await_ready() {
        return ready(storage_);
    }

    auto await_suspend(std::coroutine_handle<> handle) {
        return suspend(storage_, handle);
    }

    T await_resume() {
        return resume(storage_);
    }

  private:
    alignas(alignof(std::max_align_t)) std::byte storage_[MaxSize];
    bool (*ready)(void*);
    void (*suspend)(void*, std::coroutine_handle<>);  // TODO bool suspend
    T (*resume)(void*);
};

}  // namespace dynamic
