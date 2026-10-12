// Positive control for test-negative: a valid call through the same harness
// must compile in every variant. Without it, a negative test "passes" when
// compilation fails for an unrelated reason (missing header, typo, ...).
#include "../negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{} {:>8.3f} {:x} {:s}\n", 1, 2.5, 255u, "ok");
}
