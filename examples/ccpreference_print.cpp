// The box is 184 bytes ("─" is 3): over the default 128-byte buffer, which
// would cut it, and the cut can split a UTF-8 character.
#define FFMT_BUFFER_SIZE 256
#include <ffmt/base.hpp>

int main() {
  FFMT_PRINTLN("{2} {1}{0}!\n", 23, "C++", "Hello"); // overload (1)

  FFMT_PRINTLN("┌{0:─^{2}}┐\n"
               "│{1: ^{2}}│\n" 
               "└{0:─^{2}}┘\n", "", "Hello, world!", 23);

}
