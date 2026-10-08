// Negative test: {fmt} rejects an automatic dynamic width inside a manual placeholder at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{0:{}}", 1, 2);
}
