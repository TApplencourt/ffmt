// Negative test: {fmt} rejects a floating-point dynamic precision at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:.{}}", 1.0, 2.0);
}
