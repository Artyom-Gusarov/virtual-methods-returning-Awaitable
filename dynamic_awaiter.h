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

template <typename T>
concept AwaiterType =
    requires(T t) {
        { t.await_ready() } -> std::same_as<bool>;
        {
            t.await_suspend(std::declval<std::coroutine_handle<>>())
            } -> std::same_as<void>;
        { t.await_resume() } -> std::same_as<int>;
    };

template <AwaiterType Base, size_t MaxSize>
class Awaiter {
  public:
    template <AwaiterType T>
        requires(sizeof(T) <= MaxSize && std::is_base_of_v<Base, T>)
    Awaiter(T&& awaiter) {
        new (storage_) T(std::forward<T>(awaiter));
    }

    template <AwaiterType T>
        requires(sizeof(T) <= MaxSize && std::is_base_of_v<Base, T>)
    Awaiter& operator=(T&& awaiter) noexcept {
        awaiter_.~Base();
        new (storage_) T(std::forward<T>(awaiter));
        return *this;
    }

    bool await_ready() {
        return awaiter_.await_ready();
    }

    void await_suspend(std::coroutine_handle<> handle) {
        awaiter_.await_suspend(handle);
    }

    int await_resume() {
        return awaiter_.await_resume();
    }

  private:
    std::byte storage_[MaxSize];
    Base& awaiter_ = *reinterpret_cast<Base*>(storage_);
};

}  // namespace dynamic
