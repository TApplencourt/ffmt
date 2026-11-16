// The body of every test file, each its own binary: main() prints each
// RUN(...) case of TEST_INC through {fmt} on the host (the reference pass),
// then through ffmt on the device, one case at a time, and compares the two
// as it goes (tap_reporter, capture.hpp). Only how a case reaches the device
// depends on the programming model: DEVICE_RUN, in capture.hpp. An exception
// (a SYCL error, say) ends the binary like a crash: in the test it names.
#include "capture.hpp"

#define STR2(a) #a
#define STR(a) STR2(a)

int main() try {
  TEST_SETUP
  tap_reporter _t{STR(TEST_NAME)};

  // Each pass in its own lambda: a file's own declarations stay local to
  // the pass, and RUN(...) sits at the top level of a function body.
  [&]() {
#define FFMT_STD_PATH
// The buffer path cuts each print at FFMT_BUFFER_SIZE: so does the reference.
#define PRINT(fmt_str, ...) std::cout << ::fmt::format(fmt_str __VA_OPT__(,) __VA_ARGS__) \
  .substr(0, FFMT_BUFFER_PATH ? static_cast<size_t>(FFMT_BUFFER_SIZE) : std::string::npos)
#define PRINTLN(fmt_str, ...) PRINT(fmt_str __VA_OPT__(,) __VA_ARGS__) << '\n'
#define RUN(...) do { _t.reference_begin(); \
  for (int _i = 0; _i < N; _i++) { __VA_ARGS__; } _t.reference_end(); } while (0)
#include TEST_INC
#undef RUN
#undef PRINTLN
#undef PRINT
#undef FFMT_STD_PATH
  }();
  _t.plan();

  [&]() {
#define PRINT(fmt_str, ...) FFMT_PRINT(fmt_str __VA_OPT__(,) __VA_ARGS__)
#define PRINTLN(fmt_str, ...) FFMT_PRINTLN(fmt_str __VA_OPT__(,) __VA_ARGS__)
#define RUN DEVICE_RUN
#include TEST_INC
#undef RUN
#undef PRINTLN
#undef PRINT
  }();

  return _t.status();
} catch (const std::exception& error) {
  fprintf(stderr, "# exception: %s\n", error.what());
  return 1;
}
