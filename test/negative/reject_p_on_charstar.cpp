// Negative test: {:p} on a char* argument must be rejected at consteval.
// std::format makes this ill-formed; ffmt mirrors that behavior.
#include <ffmt/base.hpp>
int main() {
  FFMT_PRINT("{:p}\n", "hello");
}
