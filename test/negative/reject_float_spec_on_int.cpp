// Negative test: float-only spec ({:f}) on an int arg is ill-formed
// in std::format; ffmt mirrors that.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:f}\n", 1);
}
