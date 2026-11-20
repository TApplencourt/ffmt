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
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

// The programming model of this binary, which decides how a case reaches the
// device. Which device that is, is not decided here: the id's setup script
// selects it through the runtime's own environment variables
// (ONEAPI_DEVICE_SELECTOR, OMP_TARGET_OFFLOAD, CUDA_VISIBLE_DEVICES, ...).
#if defined(FFMT_TEST_KOKKOS)
#define TEST_MODEL_KOKKOS
#elif defined(SYCL_LANGUAGE_VERSION) \
    || defined(__ADAPTIVECPP__) || defined(__HIPSYCL__) || defined(__ACPP__)
#define TEST_MODEL_SYCL
#elif defined(_OPENMP)
#define TEST_MODEL_OPENMP
#elif defined(__HIPCC__) || defined(__CUDACC__)
#define TEST_MODEL_CUDA_HIP
#endif

#ifdef TEST_MODEL_SYCL
#include <sycl/sycl.hpp>
#endif
#include <ffmt/base.hpp>
#ifdef TEST_MODEL_KOKKOS
#include <Kokkos_Core.hpp>
#ifndef FFMT_TEST_KOKKOS_SPACE
#error "FFMT_TEST_KOKKOS requires FFMT_TEST_KOKKOS_SPACE"
#endif
#endif

constexpr int N = 1;

// Exit status of a test binary that ran to the end and saw a failing test.
// Unlike 1 (ASan, UBSan, an exception), 23 (LSan) or 128+n (a signal), it
// cannot come from a crash, so test/check_id.py can tell the two apart.
constexpr int TEST_FAILED = 88;

// device_sync(): waits for the kernel just launched and says whether it
// launched and ran. CUDA and HIP report neither by themselves; this covers
// Kokkos on top of them too.
#if (defined(TEST_MODEL_KOKKOS) || defined(TEST_MODEL_CUDA_HIP)) \
    && (defined(__HIPCC__) || defined(__CUDACC__))
#if defined(__HIPCC__)
#include <hip/hip_runtime.h>
#define DEVICE_API(name) hip##name
#else
#include <cuda_runtime.h>
#define DEVICE_API(name) cuda##name
#endif
#if defined(__HIPCC__)
using device_properties = hipDeviceProp_t;
#else
using device_properties = cudaDeviceProp;
#endif
[[maybe_unused]] static std::string cuda_hip_device_name() {
  int device = 0;
  device_properties properties{};
  (void)DEVICE_API(GetDevice)(&device);
  (void)DEVICE_API(GetDeviceProperties)(&properties, device);
  return properties.name;
}
[[maybe_unused]] static bool device_sync() {
  auto launch_error = DEVICE_API(GetLastError)();
  auto execution_error = DEVICE_API(DeviceSynchronize)();
  bool ok = true;
  for (auto error : {launch_error, execution_error}) {
    if (error == 0) continue;
    fprintf(stderr, "# device error: %s\n", DEVICE_API(GetErrorString)(error));
    ok = false;
  }
  return ok;
}
#undef DEVICE_API
#else
[[maybe_unused]] static bool device_sync() { return true; }
#endif

// What depends on the programming model, for test_body.inc: what a test
// file needs before its first case (TEST_SETUP), the name of the device its
// cases run on (TEST_DEVICE, a std::string: test/check_id.py compares it
// with the one test/configs.json expects) and how a RUN(...) case reaches
// the device (DEVICE_RUN). _t (test_body.inc) captures and judges one case
// at a time: _t.begin() before it, _t.end() after it.
#if defined(TEST_MODEL_KOKKOS)
#define TEST_SETUP Kokkos::ScopeGuard _kokkos;
#define TEST_DEVICE std::string("Kokkos ") + FFMT_TEST_KOKKOS_SPACE::name()
#define DEVICE_RUN(...) do { _t.begin(); \
  Kokkos::parallel_for("ffmt_test", Kokkos::RangePolicy<FFMT_TEST_KOKKOS_SPACE>(0, 1), \
    KOKKOS_LAMBDA(int) { for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } }); \
  Kokkos::fence(); _t.end(device_sync()); } while (0)
#elif defined(TEST_MODEL_SYCL)
// The handler turns asynchronous errors into exceptions at wait_and_throw().
#define TEST_SETUP ::sycl::queue q{[](::sycl::exception_list errors) { \
  for (const auto& error : errors) std::rethrow_exception(error); }};
#define TEST_DEVICE q.get_device().get_info<::sycl::info::device::name>()
#define DEVICE_RUN(...) do { _t.begin(); \
  q.parallel_for(N, [=](::sycl::id<1>) { __VA_ARGS__; }).wait_and_throw(); _t.end(); } while (0)
#elif defined(TEST_MODEL_OPENMP)
// OpenMP cannot name a device: its number, or the host if that is where a
// target region runs.
#include <omp.h>
[[maybe_unused]] static std::string openmp_device_name() {
  int on_host = 1;
#pragma omp target map(from : on_host)
  { on_host = omp_is_initial_device(); }
  return on_host ? "host" : "OpenMP device " + std::to_string(omp_get_default_device());
}
#define TEST_DEVICE openmp_device_name()
// No do/while around the target region: icpx registers an OpenMP kernel
// emitted inside one under one __LINE__ and looks it up under another, and
// every offload fails with "Failed to load kernel __omp_offloading_*_l<N>".
// RUN(...) sits at the top level of a lambda, so bare statements are fine.
#define DEVICE_RUN(...) _t.begin();                                             \
  _Pragma("omp target teams distribute parallel for num_teams(1) thread_limit(N)") \
  for (int _i = 0; _i < N; _i++) { __VA_ARGS__; }                               \
  _t.end();
#elif defined(TEST_MODEL_CUDA_HIP)
// One single-thread launch per RUN, with the N repetitions inside, as on the
// host. Needs extended device lambdas.
template <typename F> __global__ void _ffmt_run_kernel(F f) { f(); }
#define TEST_DEVICE cuda_hip_device_name()
#define DEVICE_RUN(...) do { _t.begin();                                   \
  _ffmt_run_kernel<<<1, 1>>>([=] __device__ () {                           \
    for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } });                    \
  _t.end(device_sync()); } while (0)
#else
#define DEVICE_RUN(...) do { _t.begin(); \
  for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } _t.end(); } while (0)
#endif
#ifndef TEST_SETUP
#define TEST_SETUP
#endif
#ifndef TEST_DEVICE
#define TEST_DEVICE std::string("host")
#endif

// Sends stdout to an anonymous file from start() to stop(), which returns
// what was written meanwhile.
struct stdout_capture {
  FILE* tmp = nullptr;
  int saved_fd = -1;

  void start() {
    std::cout.flush();
    fflush(stdout);
    // tmpfile() rather than Linux-only memfd_create: an anonymous, already-
    // unlinked regular file on any POSIX libc (Linux, macOS, BSD).
    tmp = std::tmpfile();
    assert(tmp);
    saved_fd = dup(STDOUT_FILENO);
    dup2(fileno(tmp), STDOUT_FILENO);
  }

  std::string stop() {
    std::cout.flush();
    fflush(stdout);
    dup2(saved_fd, STDOUT_FILENO);
    close(saved_fd);
    int fd = fileno(tmp);
    auto size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    std::string result(size, '\0');
    [[maybe_unused]] ssize_t n = ::read(fd, result.data(), size);
    assert(n == size);  // regular file: full read or EOF, no shorts
    fclose(tmp);
    return result;
  }
};

// TAP reporter of the device pass, on stderr (unbuffered, and not captured).
// The plan `1..<count>` first, so that a run that dies still says how many
// tests the file holds. Then, per test, `# start N` before it runs, so that
// a crash names the test it happened in, and `ok N - <file>` or
// `not ok N - <file>` with both outputs once it has run. test_body.inc
// prints `# device: <name>` before all that.
struct tap_reporter {
  const char* name;
  std::vector<std::string> expected{};
  stdout_capture cap{};
  int id = 0, failed = 0;

  // One case of the reference pass: what {fmt} prints for it.
  void reference_begin() { cap.start(); }
  void reference_end() { expected.push_back(cap.stop()); }
  void plan() const { fprintf(stderr, "1..%zu\n", expected.size()); }

  void begin() {
    ++id;
    fprintf(stderr, "# start %d\n", id);
    cap.start();
  }

  // device_ok: false if the launch itself reported an error (device_sync).
  void end(bool device_ok = true) {
    std::string actual = cap.stop();
    bool known = static_cast<size_t>(id) <= expected.size();
    if (device_ok && known && actual == expected[id - 1]) {
      fprintf(stderr, "ok %d - %s\n", id, name);
      return;
    }
    ++failed;
    fprintf(stderr, "not ok %d - %s\n", id, name);
    auto show = [](const char* title, const std::string& text) {
      fprintf(stderr, "  %s:\n", title);
      std::istringstream in(text);
      for (std::string line; std::getline(in, line);) fprintf(stderr, "    %s\n", line.c_str());
    };
    show("Expected", known ? expected[id - 1] : "(no such test in the reference pass)");
    show("Actual", actual);
  }

  // The exit status. A device pass that runs another number of tests than
  // the plan says is told by test/check_id.py, from the TAP.
  int status() const { return failed ? TEST_FAILED : 0; }
};
