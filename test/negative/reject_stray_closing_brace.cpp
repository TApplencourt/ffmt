// Negative test: {fmt} rejects a lone '}' (only '}}' is valid) at compile time;
// ffmt used to accept it (and print something else).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("}");
}
