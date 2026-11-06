// icpx -fsycl -O0 spirv_printf_O0_sycl.cpp && ./a.out
// Expected "ok". At -O0 it segfaults (at -O2 it prints), on PVC with
// icpx 2026.1.0 and IGC 2.11.43 or 2.34.4. Passing F directly works.
#include <sycl/sycl.hpp>

static const __attribute__((opencl_constant)) char F[] = "ok\n";

int main() {
  sycl::queue q;
  q.single_task([=] {
    const __attribute__((opencl_constant)) char *f = F; // format through a local
    sycl::ext::oneapi::experimental::printf(f);
  }).wait();
}
