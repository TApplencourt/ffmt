// Negative test: {:s} on a float is ill-formed in std::format.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:s}\n", 1.0);
}
