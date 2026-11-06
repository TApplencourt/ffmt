// The body of every test file: test_<TEST_NAME>() prints each RUN(...) case
// of TEST_INC through {fmt} on the host, then through ffmt on the device, and
// diffs the two (capture.hpp). Only how a case reaches the device depends on
// the programming model, told from the compiler's predefines: test files
// include this before ffmt/base.hpp.
#include "capture.hpp"

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)

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

bool CONCAT(test_, TEST_NAME)(TEST_PARAM) {
  int _test_id = 0;

  auto std_out = capture_stdout([&]() {
#define FFMT_STD_PATH
// The buffer path cuts each print at FFMT_BUFFER_SIZE: so does the reference.
#define PRINT(fmt_str, ...) std::cout << ::fmt::format(fmt_str __VA_OPT__(,) __VA_ARGS__) \
  .substr(0, FFMT_BUFFER_PATH ? static_cast<size_t>(FFMT_BUFFER_SIZE) : std::string::npos)
#define PRINTLN(fmt_str, ...) PRINT(fmt_str __VA_OPT__(,) __VA_ARGS__) << '\n'
#define RUN(...) do { printf("Test %d\n", ++_test_id); \
  for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } } while (0)
#include TEST_INC
#undef RUN
#undef PRINTLN
#undef PRINT
#undef FFMT_STD_PATH
  });

  _test_id = 0;

  auto device_out = capture_stdout([&]() {
#define PRINT(fmt_str, ...) FFMT_PRINT(fmt_str __VA_OPT__(,) __VA_ARGS__)
#define PRINTLN(fmt_str, ...) FFMT_PRINTLN(fmt_str __VA_OPT__(,) __VA_ARGS__)
#define RUN DEVICE_RUN
#include TEST_INC
#undef RUN
#undef PRINTLN
#undef PRINT
  });

  return diff_output(TEST_INC, std_out, device_out);
}

#ifndef TEST_NO_MAIN
int main() { return CONCAT(test_, TEST_NAME)(TEST_ARG) ? 0 : TEST_FAILED; }
#endif
