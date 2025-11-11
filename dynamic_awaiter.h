#pragma once

#include <coroutine>

namespace dynamic {

namespace internal {

template <class R>
concept await_suspend_result = std::same_as<R, void> || std::same_as<R, bool>;

template <typename A, size_t MaxSize>
concept FitsStorage = (sizeof(std::remove_cvref_t<A>) <= MaxSize) &&
                      (alignof(std::remove_cvref_t<A>) <= alignof(std::max_align_t));

}  // namespace internal

template <class A, class T>
concept AwaiterType = requires(std::remove_cvref_t<A> a, std::coroutine_handle<> h) {
                          { a.await_ready() } -> std::same_as<bool>;
                          { a.await_resume() } -> std::same_as<T>;
                          { a.await_suspend(h) } -> internal::await_suspend_result;
                      };

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
        destroy(storage_);
        new (storage_) std::remove_cvref_t<A>(std::forward<A>(awaiter));
        assign_functions<std::remove_cvref_t<A>>();
        return *this;
    }

    Awaiter(const Awaiter& other) {
        other.copy(storage_, other.storage_);
        ready = other.ready;
        suspend = other.suspend;
        resume = other.resume;
        destroy = other.destroy;
        copy = other.copy;
    }

    Awaiter& operator=(const Awaiter& other) {
        if (this != &other) {
            destroy(storage_);
            copy(storage_, other.storage_);
        }
        return *this;
    }

    Awaiter(Awaiter&& other) {
        other.move(storage_, other.storage_);
        ready = other.ready;
        suspend = other.suspend;
        resume = other.resume;
        destroy = other.destroy;
        copy = other.copy;
    }

    Awaiter& operator=(Awaiter&& other) {
        if (this != &other) {
            destroy(storage_);
            move(storage_, other.storage_);
        }
        return *this;
    }

    ~Awaiter() {
        destroy(storage_);
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
        return ready(storage_);
    }

    auto await_suspend(std::coroutine_handle<> handle) {
        return suspend(storage_, handle);
    }

    T await_resume() {
        return resume(storage_);
    }

  private:
    template <typename A>
    void assign_functions() {
        ready = [](void* storage) -> bool { return static_cast<A*>(storage)->A::await_ready(); };
        suspend = [](void* storage, std::coroutine_handle<> handle) -> bool {
            if constexpr (std::is_void_v<decltype(std::declval<A>().A::await_suspend(handle))>) {
                static_cast<A*>(storage)->A::await_suspend(handle);
                return true;
            } else {
                return static_cast<A*>(storage)->A::await_suspend(handle);
            }
        };
        resume = [](void* storage) -> T { return static_cast<A*>(storage)->A::await_resume(); };
        destroy = [](void* storage) { static_cast<A*>(storage)->A::~A(); };
        copy = [](void* dest, const void* src) { new (dest) A(*static_cast<const A*>(src)); };
        move = [](void* dest, void* src) { new (dest) A(std::move(*static_cast<A*>(src))); };
    }

  private:
    alignas(alignof(std::max_align_t)) std::byte storage_[MaxSize];
    bool (*ready)(void*);
    bool (*suspend)(void*, std::coroutine_handle<>);
    T (*resume)(void*);
    void (*destroy)(void*);
    void (*copy)(void*, const void*);
    void (*move)(void*, void*);
};

}  // namespace dynamic
