#include "capture.hpp"

// TEST_LIST comes from Makefile.id: X(<name>) for each test/test_<name>.cpp.
#define X(name) bool test_##name();
TEST_LIST
#undef X

int main() {
  bool ok = true;
#define X(name) ok &= test_##name();
  TEST_LIST
#undef X
  return ok ? 0 : TEST_FAILED;
}
