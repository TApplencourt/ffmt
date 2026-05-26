// Negative test: {:p} on a char* argument must be rejected at consteval.
// std::format makes this ill-formed; ffmt mirrors that behavior.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:p}\n", "hello");
}
