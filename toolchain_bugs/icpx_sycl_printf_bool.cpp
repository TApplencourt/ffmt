// Reported: https://github.com/argonne-lcf/AuroraBugTracking/issues/189
#include <sycl/sycl.hpp>

int main() {
  sycl::queue q;

  // A bool passed to printf — IGC segfaults when it builds the kernel, at
  // -O0 and -O2 (oneAPI 2026.2.0, Max 1550). printf is a variadic template,
  // so the bool is not promoted to int; (int)true prints 1.
  q.submit([&](sycl::handler &cgh) {
    cgh.single_task([=]() {
      ::sycl::ext::oneapi::experimental::printf("%d\n", true);
    });
  }).wait();

  return 0;
}
