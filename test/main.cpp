#include "capture.hpp"

// TEST_LIST comes from Makefile.id: X(<name>) for each test/test_<name>.cpp.
#define X(name) bool test_##name(TEST_PARAM);
TEST_LIST
#undef X

int main() {
  try {
    FFMT_TEST_MAIN_GUARD;
    bool ok = true;
#define X(name) ok &= test_##name(TEST_ARG);
    TEST_LIST
#undef X
    return ok ? 0 : TEST_FAILED;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "device test exception: %s\n", error.what());
    return 1;
  }
}
