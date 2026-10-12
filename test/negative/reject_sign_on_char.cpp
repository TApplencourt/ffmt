// Negative test: {fmt} rejects a sign on a char at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:+}", 'c');
}
