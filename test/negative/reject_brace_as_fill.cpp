// Negative test: {fmt} rejects '{' as a fill character at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:{<5}", 1);
}
