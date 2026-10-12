// Negative test: {fmt} rejects a sign ('+' or ' ') on unsigned arguments
// (std::format accepts it). The specifiers path used to drop it
// silently ("%+u") and the buffer path printed it.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:+}\n", 5u);
}
