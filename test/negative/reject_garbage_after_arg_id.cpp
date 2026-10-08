// Negative test: {fmt} rejects characters after the argument id at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{0x}", 1);
}
