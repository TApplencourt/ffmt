// Negative test: more placeholders than arguments must be rejected at
// consteval. std::format makes this ill-formed.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{} {}\n", 1);
}
