// Negative test: {fmt} rejects '0' on a string at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:05}", "s");
}
