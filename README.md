# virtual-methods-returning-Awaitable

## Problem

If we want frequently used function returning awaitable type we can make it coroutine or make it return hard-coded awaiter type, so it can be faster (no coroutine heap allocation and less calls).

But if we want to have virtual methods returning awaitable type we can't use different awaiter types in derived classes, because return type must be the same in all overrides. So we need type-erased awaiter that can store different awaiter types on stack.

## Solution

- **dynamic::Awaiter<T, MaxSize>** class can store awaiter type(has `await_ready`, `await_suspend`, `await_resume`) with `await_resume()` returning `T` and size not exceeding `MaxSize` bytes
- is copyable and movable if stored awaiter type is copyable/movable
- correctly calls stored awaiter's destructor
- supports awaiters with `void` or `bool` return type of `await_suspend()`
- has compile time checks for corectness of assigned awaiter type

## Benchmark and example of use

### Example problem

```cpp
class SharedDataBase {
  public:
    virtual SomeAwaitableType asyncGet() = 0;
    virtual ~SharedDataBase() = default;
};

class SharedDataStorage : public SharedDataBase {
  public:
    SomeAwaitableType asyncGet() final;

    void setData(int data);
};

template <typename TransformFunc>
class SharedDataTransform : public SharedDataBase {
  public:
    template <typename F>
    SharedDataTransform(F&& func, std::shared_ptr<SharedDataBase> baseData);

    SomeAwaitableType asyncGet() final;
};
```

We want this classes with virtual method `asyncGet()` returning awaitable type. And we can implement it using `dynamic::Awaiter` and hard-coded Awaiters or spawn new coroutine to put logic inside it.

### Benchmark

Both implementations are provided in [benchmark/dynamic_awaiter_impl.hpp](benchmark/dynamic_awaiter_impl.hpp) and [benchmark/new_future_impl.hpp](benchmark/new_future_impl.hpp) and benchmarked in [benchmark/shared_data_case.cpp](benchmark/shared_data_case.cpp) using Google Benchmark. 

### Results

Results on AMD 5500U / 16GB RAM in [benchmark/results](benchmark/results)

## Unit tests

Unit tests in [tests/dynamic_awaiter.cpp](tests/dynamic_awaiter.cpp) using Google Test framework.