// Negative test: {:c} on bool is ill-formed in std::format (bool's allowed
// spec set is d/b/B/o/x/X/s — no c).
#include "negative.hpp"
int main() {
  NEG_EXPECT_REJECTED("{:c}\n", true);
}
