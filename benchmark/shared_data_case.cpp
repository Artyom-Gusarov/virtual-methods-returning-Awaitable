#include <benchmark/benchmark.h>

#include "simple_coro.h"

#ifdef TEST_VIRTUAL_AWAITER
#include "dynamic_awaiter_impl.hpp"
#elif TEST_NEW_FUTURE
#include "new_future_impl.hpp"
#endif

class SharedDataUser {
  public:
    SharedDataUser(std::shared_ptr<SharedDataBase> data) : data_(data) {
    }

    auto getDataValue() {
        return data_->asyncGet();
    }

    template <typename TransformFunc>
    void transformData(TransformFunc&& func) {
        data_ = std::make_shared<SharedDataTransform<TransformFunc>>(
            std::forward<TransformFunc>(func), std::move(data_));
    }

  private:
    std::shared_ptr<SharedDataBase> data_;
};

template <size_t TransformsCount>
void BM_SyncDataUser(benchmark::State& state) {
    for (auto _ : state) {
        auto future = []() -> simple::Future {
            auto storage = std::make_shared<SharedDataStorage>();
            storage->setData(42);
            SharedDataUser user(std::move(storage));
            for (size_t i = 0; i < TransformsCount; ++i) {
                user.transformData([](int value) { return value + 1; });
            }
            int result = co_await user.getDataValue();
            assert(result == 42 + TransformsCount);
            benchmark::DoNotOptimize(result);
            co_return 0;
        }();
        future.blockingWait();
    }
}

template <size_t TransformsCount>
void BM_AsyncDataUser(benchmark::State& state) {
    std::atomic<SharedDataStorage*> storagePtr = nullptr;
    bool finishFlag = false;

    std::thread setterThread([&storagePtr, &finishFlag]() {
        while (true) {
            storagePtr.wait(nullptr);
            if (finishFlag) {
                break;
            }
            SharedDataStorage* storage = storagePtr.exchange(nullptr);
            storage->setData(42);
        }
    });

    for (auto _ : state) {
        auto future = [&storagePtr]() -> simple::Future {
            auto storage = std::make_shared<SharedDataStorage>();
            storagePtr.store(storage.get());
            storagePtr.notify_one();
            SharedDataUser user(std::move(storage));
            for (size_t i = 0; i < TransformsCount; ++i) {
                user.transformData([](int value) { return value + 1; });
            }
            int result = co_await user.getDataValue();
            assert(result == 42 + TransformsCount);
            benchmark::DoNotOptimize(result);
            co_return 0;
        }();
        future.blockingWait();
    }

    finishFlag = true;
    storagePtr.store((SharedDataStorage*)1);
    storagePtr.notify_one();
    setterThread.join();
}

BENCHMARK(BM_SyncDataUser<0>);
BENCHMARK(BM_SyncDataUser<1>);
BENCHMARK(BM_SyncDataUser<10>);
BENCHMARK(BM_SyncDataUser<100>);
BENCHMARK(BM_SyncDataUser<1000>);

BENCHMARK(BM_AsyncDataUser<0>);
BENCHMARK(BM_AsyncDataUser<1>);
BENCHMARK(BM_AsyncDataUser<10>);
BENCHMARK(BM_AsyncDataUser<100>);

BENCHMARK_MAIN();
