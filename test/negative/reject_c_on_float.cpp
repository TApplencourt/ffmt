// Negative test: {:c} on float is ill-formed in std::format.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:c}\n", 1.0);
}
