// Negative test: {fmt} rejects an argument id that is not a number at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{x}", 1);
}
