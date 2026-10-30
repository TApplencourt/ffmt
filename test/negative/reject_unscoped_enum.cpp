// Negative test: {fmt} (since v10) and std::format reject an enum argument;
// an unscoped enum must not be formatted through its implicit conversion to
// int. Callers convert explicitly (static_cast / std::to_underlying).
#include "negative.hpp"
enum color { red };
int main() {
  NEG_EXPECT_REJECTED("{}\n", red);
}
