// Negative test: {fmt} rejects switching from manual to automatic indexing at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{0} {}", 1);
}
