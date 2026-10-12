// Negative test: {fmt} rejects '#' on a pointer at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:#}", static_cast<void*>(nullptr));
}
