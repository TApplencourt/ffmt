#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

#include <ffmt/base.hpp>
// SYCL backends only: TEST_PARAM below is a sycl::queue.
#if defined(SYCL_LANGUAGE_VERSION) || FFMT_COMPILER_ACPP
#include <sycl/sycl.hpp>
#endif

constexpr int N = 2;

// Exit status of a test binary that ran to the end and saw a failing test.
// Unlike 1 (ASan, UBSan), 23 (LSan) or 128+n (a signal), it cannot come from
// a crash, so test/check_id.py can tell the two apart.
constexpr int TEST_FAILED = 88;

// What depends on the programming model, for test_body.inc and main.cpp:
// how a test function receives what it needs (TEST_PARAM, TEST_ARG) and
// how a RUN(...) case reaches the device (DEVICE_RUN).
#if defined(SYCL_LANGUAGE_VERSION) \
    || defined(__ADAPTIVECPP__) || defined(__HIPSYCL__) || defined(__ACPP__)
#define TEST_PARAM ::sycl::queue q  // a handle: copies share the queue
#define TEST_ARG ::sycl::queue()
#define DEVICE_RUN(...) do { printf("Test %d\n", ++_test_id); \
  q.parallel_for(N, [=](::sycl::id<1>) { __VA_ARGS__; }).wait(); } while (0)
#elif defined(_OPENMP)
// No do/while around the target region: icpx registers an OpenMP kernel
// emitted inside one under one __LINE__ and looks it up under another, and
// every offload fails with "Failed to load kernel __omp_offloading_*_l<N>".
// RUN(...) sits at the top level of a lambda, so bare statements are fine.
#define DEVICE_RUN(...) printf("Test %d\n", ++_test_id);                        \
  _Pragma("omp target teams distribute parallel for num_teams(1) thread_limit(N)") \
  for (int _i = 0; _i < N; _i++) { __VA_ARGS__; }
#elif defined(__CUDACC__)
// One single-thread launch per RUN, with the N repetitions inside, as on the
// host. Needs extended device lambdas.
template <typename F> __global__ void _ffmt_run_kernel(F f) { f(); }
#define DEVICE_RUN(...) do { printf("Test %d\n", ++_test_id);              \
  _ffmt_run_kernel<<<1, 1>>>([=] __device__ () {                           \
    for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } });                    \
  cudaDeviceSynchronize(); } while (0)
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
  int saved_fd = dup(STDOUT_FILENO);
  dup2(mem_fd, STDOUT_FILENO);

  fn();

  std::cout.flush();
  fflush(stdout);
  dup2(saved_fd, STDOUT_FILENO);
  close(saved_fd);

  auto size = lseek(mem_fd, 0, SEEK_END);
  lseek(mem_fd, 0, SEEK_SET);
  std::string result(size, '\0');
  ssize_t n = ::read(mem_fd, result.data(), size);
  assert(n == size);  // regular file: full read or EOF, no shorts
  fclose(tmp);
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
