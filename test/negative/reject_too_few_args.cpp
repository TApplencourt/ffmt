// Negative test: more placeholders than arguments must be rejected at
// consteval. std::format makes this ill-formed.
#include <ffmt/base.hpp>
int main() {
  FFMT_PRINT("{} {}\n", 1);
}
