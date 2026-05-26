// Negative test: integer-only spec ({:d}) on a float arg is ill-formed
// in std::format; ffmt mirrors that.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:d}\n", 1.0);
}
