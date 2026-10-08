// Negative test: {fmt} rejects a manual dynamic width inside an automatic placeholder at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:{1}}", 1, 2);
}
