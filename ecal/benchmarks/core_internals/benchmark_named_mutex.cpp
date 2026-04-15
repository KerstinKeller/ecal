/* ========================= eCAL LICENSE =================================
 *
 * Copyright 2025 AUMOVIO and subsidiaries. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
 */

#include <benchmark/benchmark.h>

#include <ecal_utils/barrier.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "io/mtx/ecal_named_mutex_base.h"

#ifdef ECAL_OS_WINDOWS
#include "io/mtx/win32/ecal_named_mutex_impl.h"
#endif

#ifdef ECAL_OS_LINUX
#include "io/mtx/linux/ecal_named_mutex_impl.h"
#if defined(ECAL_HAS_ROBUST_MUTEX) || defined(ECAL_HAS_CLOCKLOCK_MUTEX)
#include "io/mtx/linux/ecal_named_mutex_robust_clocklock_impl.h"
#endif
#endif

namespace
{
  struct MutexImplementation
  {
    using Factory = std::unique_ptr<eCAL::CNamedMutexImplBase>(*)(const std::string&, bool);

    std::string label;
    Factory factory;
  };

  std::vector<MutexImplementation> BuildMutexRegistry()
  {
    std::vector<MutexImplementation> mutex_registry;

#ifdef ECAL_OS_WINDOWS
    mutex_registry.push_back(
      {
        "Win32",
        [](const std::string& name, bool recoverable)
        {
          return std::unique_ptr<eCAL::CNamedMutexImplBase>(new eCAL::CNamedMutexImpl(name, recoverable));
        }
      });
#endif

#ifdef ECAL_OS_LINUX
    mutex_registry.push_back(
      {
        "LinuxBasic",
        [](const std::string& name, bool recoverable)
        {
          return std::unique_ptr<eCAL::CNamedMutexImplBase>(new eCAL::CNamedMutexImpl(name, recoverable));
        }
      });

#if defined(ECAL_HAS_ROBUST_MUTEX) || defined(ECAL_HAS_CLOCKLOCK_MUTEX)
    mutex_registry.push_back(
      {
        "LinuxRobustClockLock",
        [](const std::string& name, bool recoverable)
        {
          return std::unique_ptr<eCAL::CNamedMutexImplBase>(new eCAL::CNamedMutexRobustClockLockImpl(name, recoverable));
        }
      });
#endif
#endif

    return mutex_registry;
  }

  void BusyWaitNs(int64_t busy_wait_ns)
  {
    if (busy_wait_ns <= 0)
      return;

    const auto busy_wait_duration = std::chrono::nanoseconds(busy_wait_ns);
    const auto start_time = std::chrono::steady_clock::now();
    while ((std::chrono::steady_clock::now() - start_time) < busy_wait_duration)
    {
    }
  }

  void BM_NamedMutex_BatchTime(benchmark::State& state, MutexImplementation::Factory factory)
  {
    constexpr int64_t rounds_per_thread = 1000;
    constexpr int64_t lock_timeout_ms = 100;

    const auto thread_count = static_cast<std::size_t>(state.range(0));
    const auto busy_wait_ns = state.range(1);

    for (auto _ : state)
    {
      static std::atomic<uint64_t> benchmark_iteration_counter{ 0 };
      const auto unique_iteration_id = benchmark_iteration_counter.fetch_add(1, std::memory_order_relaxed);
      const std::string mutex_name = "ecal_benchmark_named_mutex_"
        + std::to_string(reinterpret_cast<uintptr_t>(&state))
        + "_" + std::to_string(unique_iteration_id);

      Barrier start_barrier(thread_count + 1);
      Barrier round_barrier(thread_count);

      std::atomic<int64_t> failed_locks{ 0 };
      std::vector<std::thread> workers;
      workers.reserve(thread_count);

      for (std::size_t thread_index = 0; thread_index < thread_count; ++thread_index)
      {
        workers.emplace_back(
          [&, mutex_name]()
          {
            auto mutex = factory(mutex_name, true);
            start_barrier.wait();

            for (int64_t round = 0; round < rounds_per_thread; ++round)
            {
              round_barrier.wait();
              if (mutex->IsCreated() && mutex->Lock(lock_timeout_ms))
              {
                BusyWaitNs(busy_wait_ns);
                mutex->Unlock();
              }
              else
              {
                failed_locks.fetch_add(1, std::memory_order_relaxed);
              }
              round_barrier.wait();
            }
          });
      }

      start_barrier.wait();

      const auto t0 = std::chrono::steady_clock::now();
      for (auto& worker : workers)
      {
        worker.join();
      }
      const auto t1 = std::chrono::steady_clock::now();

      const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
      const auto total_lock_ops = static_cast<double>(thread_count * static_cast<std::size_t>(rounds_per_thread));

      state.SetIterationTime(std::chrono::duration<double>(t1 - t0).count());
      state.counters["total_lock_ops"] = benchmark::Counter(total_lock_ops, benchmark::Counter::kIsIterationInvariantRate);
      state.counters["failed_locks"] = static_cast<double>(failed_locks.load(std::memory_order_relaxed));
      state.counters["ns_per_lock"] = (total_lock_ops > 0.0)
        ? static_cast<double>(elapsed_ns) / total_lock_ops
        : 0.0;
    }
  }

  void RegisterBenchmarks()
  {
    constexpr int64_t busy_wait_values_ns[] = { 250, 500, 1000, 2000, 4000 };
    const auto mutex_registry = BuildMutexRegistry();

    for (const auto& mutex_implementation : mutex_registry)
    {
      for (int64_t thread_count = 1; thread_count <= 128; thread_count *= 2)
      {
        for (const auto busy_wait_ns : busy_wait_values_ns)
        {
          benchmark::RegisterBenchmark(
            ("NamedMutex/BatchTime/" + mutex_implementation.label
              + "/threads:" + std::to_string(thread_count)
              + "/busywait_ns:" + std::to_string(busy_wait_ns)).c_str(),
            BM_NamedMutex_BatchTime,
            mutex_implementation.factory)
            ->Args({ thread_count, busy_wait_ns })
            ->UseManualTime()
            ->Iterations(1);
        }
      }
    }
  }
}

int main(int argc, char** argv)
{
  ::benchmark::Initialize(&argc, argv);
  RegisterBenchmarks();
  ::benchmark::RunSpecifiedBenchmarks();
}
