#include <ffmt/base.hpp>

int main() {
  FFMT_PRINTLN("{2} {1}{0}!\n", 23, "C++", "Hello"); // overload (1)

  FFMT_PRINTLN("┌{0:─^{2}}┐\n"
               "│{1: ^{2}}│\n" 
               "└{0:─^{2}}┘\n", "", "Hello, world!", 23);

}
