// Negative test: {fmt} rejects '#' on a string at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:#}", "s");
}
