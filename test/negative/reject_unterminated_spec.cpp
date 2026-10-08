// Negative test: {fmt} rejects a '}' inside the spec at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:}<5}", 1);
}
