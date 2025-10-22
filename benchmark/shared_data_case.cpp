#include <benchmark/benchmark.h>

static void BM_Dummy(benchmark::State &state) {
  for (auto _ : state) {
    int x = 0;
    ++x;
  }
}

BENCHMARK(BM_Dummy);
BENCHMARK_MAIN();
