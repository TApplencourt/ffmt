// Negative test: {fmt} (since v10) and std::format reject an enum argument;
// a scoped enum is not formatted as its underlying integer.
#include "negative.hpp"
enum class color : int { red };
int main() {
  NEG_EXPECT_REJECTED("{}\n", color::red);
}
