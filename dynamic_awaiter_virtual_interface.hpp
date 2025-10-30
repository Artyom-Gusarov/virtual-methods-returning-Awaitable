#pragma once

#include <coroutine>
#include <cstddef>

namespace dynamic {

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

namespace internal {

template <typename T>
struct AwaiterInterface {
    virtual bool await_ready(void* storage) = 0;

    virtual bool await_suspend(void* storage, std::coroutine_handle<> handle) = 0;

    virtual T await_resume(void* storage) = 0;

    virtual void destroy(void* storage) = 0;
};

template <typename T, AwaiterType<T> A>
struct AwaiterInterfaceWrapper : AwaiterInterface<T> {
    bool await_ready(void* storage) final {
        return static_cast<A*>(storage)->await_ready();
    }

    bool await_suspend(void* storage, std::coroutine_handle<> handle) final {
        if constexpr (std::is_void_v<decltype(std::declval<A>().await_suspend(handle))>) {
            static_cast<A*>(storage)->await_suspend(handle);
            return true;
        } else {
            return static_cast<A*>(storage)->await_suspend(handle);
        }
    }

    T await_resume(void* storage) final {
        return static_cast<A*>(storage)->await_resume();
    }

    void destroy(void* storage) final {
        static_cast<A*>(storage)->~A();
    }
};

}  // namespace internal

template <typename T, size_t MaxSize>
class Awaiter {
  public:
    template <AwaiterType<T> A>
        requires(sizeof(A) <= MaxSize && alignof(A) <= alignof(std::max_align_t))
    Awaiter(A&& awaiter) {
        new (storage_) A(std::forward<A>(awaiter));

        new (interface_storage_) internal::AwaiterInterfaceWrapper<T, A>();
    }

    template <AwaiterType<T> A>
        requires(sizeof(A) <= MaxSize && alignof(A) <= alignof(std::max_align_t))
    Awaiter& operator=(A&& awaiter) noexcept {
        reinterpret_cast<internal::AwaiterInterface<T>*>(interface_storage_)->destroy(storage_);
        new (storage_) A(std::forward<A>(awaiter));

        new (interface_storage_) internal::AwaiterInterfaceWrapper<T, A>();

        return *this;
    }

    bool await_ready() {
        return reinterpret_cast<internal::AwaiterInterface<T>*>(interface_storage_)->await_ready(storage_);
    }

    auto await_suspend(std::coroutine_handle<> handle) {
        return reinterpret_cast<internal::AwaiterInterface<T>*>(interface_storage_)->await_suspend(storage_, handle);
    }

    T await_resume() {
        return reinterpret_cast<internal::AwaiterInterface<T>*>(interface_storage_)->await_resume(storage_);
    }

  private:
    alignas(alignof(std::max_align_t)) std::byte storage_[MaxSize];
    std::byte interface_storage_[8];
};

}  // namespace dynamic
