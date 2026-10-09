// Negative test: {fmt} rejects an unknown presentation type at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:q}", 1);
}
