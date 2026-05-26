// Negative test: {:s} requires a string-like (or bool) arg in std::format;
// applying it to an int is ill-formed.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:s}\n", 1);
}
