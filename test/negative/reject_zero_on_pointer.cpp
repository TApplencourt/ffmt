// Negative test: {fmt} rejects '0' on a pointer at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:05}", static_cast<void*>(nullptr));
}
