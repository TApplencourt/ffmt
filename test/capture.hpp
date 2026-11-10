#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

#if defined(SYCL_LANGUAGE_VERSION) \
    || defined(__ADAPTIVECPP__) || defined(__HIPSYCL__) || defined(__ACPP__)
#include <sycl/sycl.hpp>
#endif
#include <ffmt/base.hpp>
#ifdef FFMT_TEST_KOKKOS
#include <Kokkos_Core.hpp>
#include <string_view>
#include <type_traits>
#ifndef FFMT_TEST_KOKKOS_SPACE
#error "FFMT_TEST_KOKKOS requires FFMT_TEST_KOKKOS_SPACE"
#endif
#endif
// SYCL backends only: TEST_PARAM below is a sycl::queue.
#if defined(SYCL_LANGUAGE_VERSION) \
    || defined(__ADAPTIVECPP__) || defined(__HIPSYCL__) || defined(__ACPP__)
[[maybe_unused]] static ::sycl::queue make_gpu_test_queue() {
  return ::sycl::queue{::sycl::gpu_selector_v, [](::sycl::exception_list errors) {
    for (const auto& error : errors) std::rethrow_exception(error);
  }};
}
#endif

constexpr int N = 2;

// Exit status of a test binary that ran to the end and saw a failing test.
// Unlike 1 (ASan, UBSan), 23 (LSan) or 128+n (a signal), it cannot come from
// a crash, so test/check_id.py can tell the two apart.
constexpr int TEST_FAILED = 88;

#ifdef FFMT_TEST_KOKKOS
#define FFMT_TEST_STRINGIFY2(x) #x
#define FFMT_TEST_STRINGIFY(x) FFMT_TEST_STRINGIFY2(x)
[[maybe_unused]] static const char* kokkos_test_space_name() {
  constexpr std::string_view configured = FFMT_TEST_STRINGIFY(FFMT_TEST_KOKKOS_SPACE);
  if (configured == "Kokkos::Serial") return "Serial";
  if (configured == "Kokkos::Cuda") return "Cuda";
  if (configured == "Kokkos::HIP") return "HIP";
  if (configured == "Kokkos::SYCL") return "SYCL";
  return nullptr;
}
template <typename Space>
[[maybe_unused]] static bool check_kokkos_test_device(const Space& space) {
#ifdef KOKKOS_ENABLE_SYCL
  if constexpr (std::is_same_v<Space, Kokkos::SYCL>) {
    if (!space.sycl_queue().get_device().is_gpu()) {
      std::fprintf(stderr, "Kokkos test: refusing SYCL CPU fallback\n");
      return false;
    }
  }
#endif
  (void)space;
  return true;
}
[[maybe_unused]] static bool check_kokkos_test_space() {
  const char* expected = kokkos_test_space_name();
  const char* actual = Kokkos::DefaultExecutionSpace::name();
  if (!expected || std::strcmp(actual, expected) != 0) {
    std::fprintf(stderr, "Kokkos test space mismatch: configured %s, expected %s, got %s\n",
                 FFMT_TEST_STRINGIFY(FFMT_TEST_KOKKOS_SPACE),
                 expected ? expected : "unsupported", actual);
    return false;
  }
  return check_kokkos_test_device(FFMT_TEST_KOKKOS_SPACE{});
}
#define FFMT_TEST_MAIN_GUARD \
  Kokkos::ScopeGuard kokkos_guard; \
  if (!check_kokkos_test_space()) return 1
#else
#define FFMT_TEST_MAIN_GUARD
#endif

// What depends on the programming model, for test_body.inc and main.cpp:
// how a test function receives what it needs (TEST_PARAM, TEST_ARG) and
// how a RUN(...) case reaches the device (DEVICE_RUN).
#if defined(FFMT_TEST_KOKKOS) \
    || (!defined(SYCL_LANGUAGE_VERSION) && !defined(__ADAPTIVECPP__) \
        && !defined(__HIPSYCL__) && !defined(__ACPP__) && !defined(_OPENMP))
#if defined(__HIPCC__) || defined(__CUDACC__)
#if defined(__HIPCC__)
#include <hip/hip_runtime.h>
#define FFMT_TEST_LAST_ERROR hipGetLastError
#define FFMT_TEST_SYNCHRONIZE hipDeviceSynchronize
#define FFMT_TEST_ERROR_STRING hipGetErrorString
#else
#include <cuda_runtime.h>
#define FFMT_TEST_LAST_ERROR cudaGetLastError
#define FFMT_TEST_SYNCHRONIZE cudaDeviceSynchronize
#define FFMT_TEST_ERROR_STRING cudaGetErrorString
#endif
#define FFMT_TEST_CUDA_HIP_ERRORS
[[maybe_unused]] static bool synchronize_test_kernel() {
  auto launch_error = FFMT_TEST_LAST_ERROR();
  auto execution_error = FFMT_TEST_SYNCHRONIZE();
  for (auto error : {launch_error, execution_error}) {
    if (error != 0) {
      std::fprintf(stderr, "device test runtime error: %s\n",
                   FFMT_TEST_ERROR_STRING(error));
      return false;
    }
  }
  return true;
}
#undef FFMT_TEST_LAST_ERROR
#undef FFMT_TEST_SYNCHRONIZE
#undef FFMT_TEST_ERROR_STRING
#endif
#endif

#if defined(FFMT_TEST_KOKKOS)
#ifdef FFMT_TEST_CUDA_HIP_ERRORS
#define FFMT_TEST_KOKKOS_POST_LAUNCH runtime_ok = synchronize_test_kernel() && runtime_ok
#else
#define FFMT_TEST_KOKKOS_POST_LAUNCH ((void)0)
#endif
#define DEVICE_RUN(...) do { printf("Test %d\n", ++_test_id); \
  Kokkos::parallel_for("ffmt_test", Kokkos::RangePolicy<FFMT_TEST_KOKKOS_SPACE>(0, 1), \
    KOKKOS_LAMBDA(int) { for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } }); \
  Kokkos::fence(); FFMT_TEST_KOKKOS_POST_LAUNCH; } while (0)
#elif defined(SYCL_LANGUAGE_VERSION) \
    || defined(__ADAPTIVECPP__) || defined(__HIPSYCL__) || defined(__ACPP__)
#define FFMT_TEST_SYCL_QUEUE
#define TEST_PARAM ::sycl::queue q  // a handle: copies share the queue
#define TEST_ARG make_gpu_test_queue()
#define DEVICE_RUN(...) do { printf("Test %d\n", ++_test_id); \
  q.parallel_for(N, [=](::sycl::id<1>) { __VA_ARGS__; }).wait_and_throw(); } while (0)
#elif defined(_OPENMP)
#include <omp.h>
#define FFMT_TEST_OPENMP
// No do/while around the target region: icpx registers an OpenMP kernel
// emitted inside one under one __LINE__ and looks it up under another, and
// every offload fails with "Failed to load kernel __omp_offloading_*_l<N>".
// RUN(...) sits at the top level of a lambda, so bare statements are fine.
#define DEVICE_RUN(...) printf("Test %d\n", ++_test_id);                        \
  _Pragma("omp target teams distribute parallel for num_teams(1) thread_limit(N)") \
  for (int _i = 0; _i < N; _i++) { __VA_ARGS__; }
#elif defined(__HIPCC__) || defined(__CUDACC__)
// One single-thread launch per RUN, with the N repetitions inside, as on the
// host. Needs extended device lambdas.
template <typename F> __global__ void _ffmt_run_kernel(F f) { f(); }
#define DEVICE_RUN(...) do { printf("Test %d\n", ++_test_id);              \
  _ffmt_run_kernel<<<1, 1>>>([=] __device__ () {                           \
    for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } });                    \
  runtime_ok = synchronize_test_kernel() && runtime_ok; } while (0)
#else
#define DEVICE_RUN(...) do { printf("Test %d\n", ++_test_id); \
  for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } } while (0)
#endif
#ifndef TEST_PARAM
#define TEST_PARAM
#define TEST_ARG
#endif

static std::string capture_stdout(auto&& fn) {
  std::cout.flush();
  fflush(stdout);

  // tmpfile() rather than Linux-only memfd_create: an anonymous, already-
  // unlinked regular file on any POSIX libc (Linux, macOS, BSD).
  FILE* tmp = std::tmpfile();
  assert(tmp);
  int mem_fd = fileno(tmp);
  struct CaptureGuard {
    FILE* tmp;
    int saved_fd;
    void restore() {
      if (saved_fd < 0) return;
      std::cout.flush();
      fflush(stdout);
      dup2(saved_fd, STDOUT_FILENO);
      close(saved_fd);
      saved_fd = -1;
    }
    ~CaptureGuard() {
      restore();
      fclose(tmp);
    }
  } guard{tmp, dup(STDOUT_FILENO)};
  dup2(mem_fd, STDOUT_FILENO);

  fn();

  guard.restore();

  auto size = lseek(mem_fd, 0, SEEK_END);
  lseek(mem_fd, 0, SEEK_SET);
  std::string result(size, '\0');
  ssize_t n = ::read(mem_fd, result.data(), size);
  assert(n == size);  // regular file: full read or EOF, no shorts
  return result;
}

// Split a captured stream into blocks keyed by `Test N` markers. Each block
// is the text emitted *after* that marker and before the next one. Anything
// before the first marker (e.g. setup noise) is ignored.
[[maybe_unused]] static std::map<int, std::string>
parse_blocks(const std::string& s) {
  std::map<int, std::string> out;
  std::istringstream in(s);
  std::string line;
  int id = -1;
  while (std::getline(in, line)) {
    if (line.starts_with("Test ")) {
      id = std::atoi(line.c_str() + 5);
      out.try_emplace(id);
    } else if (id >= 0) {
      out[id] += "    " + line + "\n";
    }
  }
  return out;
}

// TAP-ish per-test reporter. Parses each stream independently so a desync
// in one block doesn't bleed into later tests (previously: one shorter
// `Actual:` swallowed every passing test after it). Emits one line per
// test id and only prints the expected/actual diff on failure. Returns
// true when every test matched.
[[maybe_unused]] static bool diff_output(const char* name,
                        const std::string& expected,
                        const std::string& actual) {
  auto exp_blocks = parse_blocks(expected);
  auto act_blocks = parse_blocks(actual);
  // Union of ids, sorted ascending — std::map iterates in order; merge keys.
  std::set<int> ids;
  for (auto& [k, _] : exp_blocks) ids.insert(k);
  for (auto& [k, _] : act_blocks) ids.insert(k);
  int passed = 0, failed = 0;
  for (int id : ids) {
    const auto& e = exp_blocks[id];
    const auto& a = act_blocks[id];
    if (e == a) {
      fprintf(stderr, "ok %d - %s\n", id, name);
      ++passed;
    } else {
      fprintf(stderr, "not ok %d - %s\n", id, name);
      fprintf(stderr, "  Expected:\n%s", e.c_str());
      fprintf(stderr, "  Actual:\n%s", a.c_str());
      ++failed;
    }
  }
  fprintf(stderr, "# %s: %d/%zu passed\n", name, passed, ids.size());
  return failed == 0;
}
