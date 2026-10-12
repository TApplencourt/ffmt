// Negative test: a fill must be one valid UTF-8 code point; a truncated
// sequence (lead byte of a 3-byte '─' without its tail) is rejected.
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:\xe2<5}", 1);
}
