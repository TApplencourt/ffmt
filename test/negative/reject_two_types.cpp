// Negative test: {fmt} rejects two presentation types at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:dx}", 1);
}
