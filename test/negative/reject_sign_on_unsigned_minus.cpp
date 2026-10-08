// Negative test: {fmt} rejects any sign, '-' included, on unsigned arguments
// (the standard and std::format accept it).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:-}\n", 5u);
}
