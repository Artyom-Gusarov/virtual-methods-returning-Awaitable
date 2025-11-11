#pragma once

#include <coroutine>
#include <cstring>

namespace dynamic {

namespace internal {

template <class R>
concept AwaitSuspendResult = std::same_as<R, void> || std::same_as<R, bool>;

template <typename A, size_t MaxSize>
concept FitsStorage = (sizeof(std::remove_cvref_t<A>) <= MaxSize) &&
                      (alignof(std::remove_cvref_t<A>) <= alignof(std::max_align_t));

}  // namespace internal

template <class A, class T>
concept AwaiterType = requires(std::remove_cvref_t<A> a, std::coroutine_handle<> h) {
                          { a.await_ready() } -> std::same_as<bool>;
                          { a.await_resume() } -> std::same_as<T>;
                          { a.await_suspend(h) } -> internal::AwaitSuspendResult;
                      };

namespace internal {

template <typename T>
struct AwaiterInterface {
    virtual bool await_ready(void* storage) = 0;

    virtual bool await_suspend(void* storage, std::coroutine_handle<> handle) = 0;

    virtual T await_resume(void* storage) = 0;

    virtual void destroy(void* storage) = 0;

    virtual void copy(void* dest, const void* src) = 0;

    virtual void move(void* dest, void* src) = 0;
};

template <typename T, AwaiterType<T> A>
struct AwaiterInterfaceWrapper : AwaiterInterface<T> {
    bool await_ready(void* storage) final {
        return static_cast<A*>(storage)->A::await_ready();
    }

    bool await_suspend(void* storage, std::coroutine_handle<> handle) final {
        if constexpr (std::is_void_v<decltype(std::declval<A>().A::await_suspend(handle))>) {
            static_cast<A*>(storage)->A::await_suspend(handle);
            return true;
        } else {
            return static_cast<A*>(storage)->A::await_suspend(handle);
        }
    }

    T await_resume(void* storage) final {
        return static_cast<A*>(storage)->A::await_resume();
    }

    void destroy(void* storage) final {
        static_cast<A*>(storage)->A::~A();
    }

    void copy(void* dest, const void* src) final {
        if constexpr (std::is_copy_constructible_v<A>) {
            new (dest) A(*static_cast<const A*>(src));
        } else {
            throw std::logic_error("Attempted to copy a non-copyable awaiter type");
        }
    }

    void move(void* dest, void* src) final {
        new (dest) A(std::move(*static_cast<A*>(src)));
    }
};

}  // namespace internal

template <typename T, size_t MaxSize>
class Awaiter {
  public:
    template <AwaiterType<T> A>
        requires(internal::FitsStorage<A, MaxSize>)
    Awaiter(A&& awaiter) {
        new (storage_) std::remove_cvref_t<A>(std::forward<A>(awaiter));
        assign_functions<std::remove_cvref_t<A>>();
    }

    template <AwaiterType<T> A>
        requires(internal::FitsStorage<A, MaxSize>)
    Awaiter& operator=(A&& awaiter) {
        reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->destroy(storage_);
        new (storage_) std::remove_cvref_t<A>(std::forward<A>(awaiter));
        assign_functions<std::remove_cvref_t<A>>();
        return *this;
    }

    Awaiter(const Awaiter& other) {
        std::memcpy(interfaceStorage_, other.interfaceStorage_, sizeof(interfaceStorage_));
        reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->copy(storage_, other.storage_);
    }

    Awaiter& operator=(const Awaiter& other) {
        if (this != &other) {
            reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->destroy(storage_);
            std::memcpy(interfaceStorage_, other.interfaceStorage_, sizeof(interfaceStorage_));
            reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->copy(storage_, other.storage_);
        }
        return *this;
    }

    Awaiter(Awaiter&& other) {
        std::memcpy(interfaceStorage_, other.interfaceStorage_, sizeof(interfaceStorage_));
        reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->move(storage_, other.storage_);
    }

    Awaiter& operator=(Awaiter&& other) {
        if (this != &other) {
            reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->destroy(storage_);
            std::memcpy(interfaceStorage_, other.interfaceStorage_, sizeof(interfaceStorage_));
            reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->move(storage_, other.storage_);
        }
        return *this;
    }

    ~Awaiter() {
        reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->destroy(storage_);
    }

    // Fallbacks for compile errors readability
    template <typename A>
        requires(!(AwaiterType<A, T> && internal::FitsStorage<A, MaxSize>) &&
                 !std::is_same_v<std::remove_cvref_t<A>, Awaiter>)
    Awaiter(A&&) {
        static_assert(AwaiterType<A, T>);
        static_assert(internal::FitsStorage<A, MaxSize>);
    }

    template <typename A>
        requires(!(AwaiterType<A, T> && internal::FitsStorage<A, MaxSize>) &&
                 !std::is_same_v<std::remove_cvref_t<A>, Awaiter>)
    Awaiter& operator=(A&&) {
        static_assert(AwaiterType<A, T>);
        static_assert(internal::FitsStorage<A, MaxSize>);
    }

    bool await_ready() {
        return reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->await_ready(storage_);
    }

    auto await_suspend(std::coroutine_handle<> handle) {
        return reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->await_suspend(storage_, handle);
    }

    T await_resume() {
        return reinterpret_cast<internal::AwaiterInterface<T>*>(interfaceStorage_)->await_resume(storage_);
    }

  private:
    template <typename A>
    void assign_functions() {
        new (interfaceStorage_) internal::AwaiterInterfaceWrapper<T, A>();
    }

  private:
    alignas(alignof(std::max_align_t)) std::byte storage_[MaxSize];
    alignas(alignof(internal::AwaiterInterface<T>)) std::byte interfaceStorage_[sizeof(internal::AwaiterInterface<T>)];
};

}  // namespace dynamic
