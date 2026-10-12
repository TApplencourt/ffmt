// Negative test: {fmt} rejects a precision on an integer at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:.3}", 1);
}
