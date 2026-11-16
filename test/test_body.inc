// The body of every test file: test_<TEST_NAME>() prints each RUN(...) case
// of TEST_INC through {fmt} on the host, then through ffmt on the device, and
// diffs the two. Only how a case reaches the device depends on the
// programming model: DEVICE_RUN, in capture.hpp.
#include "capture.hpp"

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)

bool CONCAT(test_, TEST_NAME)(TEST_PARAM) {
#ifdef FFMT_TEST_SYCL_QUEUE
  if (!q.get_device().is_gpu()) {
    std::fprintf(stderr, "%s: refusing SYCL CPU fallback\n", TEST_INC);
    return false;
  }
#endif
#ifdef FFMT_TEST_OPENMP
  int on_initial_device = 1;
#pragma omp target map(from : on_initial_device)
  { on_initial_device = omp_is_initial_device(); }
  if (on_initial_device) {
    std::fprintf(stderr, "%s: refusing OpenMP host fallback\n", TEST_INC);
    return false;
  }
#endif
  int _test_id = 0;
  [[maybe_unused]] bool runtime_ok = true;

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

  return diff_output(TEST_INC, std_out, device_out) && runtime_ok;
}

#ifndef TEST_NO_MAIN
int main() {
  try {
    FFMT_TEST_MAIN_GUARD;
    return CONCAT(test_, TEST_NAME)(TEST_ARG) ? 0 : TEST_FAILED;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s: device test exception: %s\n", TEST_INC, error.what());
    return 1;
  }
}
#endif
