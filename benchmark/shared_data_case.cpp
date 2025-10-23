#include <benchmark/benchmark.h>

#include "new_future_impl.hpp"
#include "simple_coro.h"

#ifdef TEST_VIRTUAL_AWAITER
#include "virtual_awaiter_impl.hpp"
#elif TEST_NEW_FUTURE
#include "new_future_impl.hpp"
#endif

class SharedDataUser {
  public:
    SharedDataUser(std::shared_ptr<SharedDataBase> data) : data_(data) {
    }

    simple::Future getData() {
        int value = co_await data_->asyncGet();
        co_return value;
    }

    template <typename TransformFunc>
    void transformData(TransformFunc&& func) {
        data_ = std::make_shared<SharedDataTransform<TransformFunc>>(
            std::forward<TransformFunc>(func), std::move(data_));
    }

  private:
    std::shared_ptr<SharedDataBase> data_;
};

void BM_SyncDataUser(benchmark::State& state) {
    for (auto _ : state) {
        auto future = []() -> simple::Future {
            auto storage = std::make_shared<SharedDataStorage>();
            storage->setData(42);
            SharedDataUser user(storage);
            user.transformData([](int value) { return value * 2; });
            int result = co_await user.getData();
            assert(result == 84);
            benchmark::DoNotOptimize(result);
            co_return 0;
        }();
        future.blockingWait();
    }
}

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
            SharedDataUser user(storage);
            user.transformData([](int value) { return value * 2; });
            int result = co_await user.getData();
            assert(result == 84);
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

BENCHMARK(BM_SyncDataUser);
BENCHMARK(BM_AsyncDataUser);
BENCHMARK_MAIN();
