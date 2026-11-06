// icpx -fiopenmp -fopenmp-targets=spir64 -O0 spirv_printf_O0_omp.cpp && ./a.out
// Expected "ok". At -O0 it segfaults (at -O2 it prints), on PVC with
// icpx 2026.1.0 and IGC 2.11.43 or 2.34.4. Passing F directly works.
#include <cstdio>

#if defined(__SPIR__)
#define AS2 __attribute__((opencl_constant))
template <typename... Args> extern int __spirv_ocl_printf(const AS2 char *, Args...);
#else // host pass
#define AS2
#define __spirv_ocl_printf printf
#endif

int main() {
#pragma omp target
  {
    static const AS2 char F[] = "ok\n";
    const AS2 char *f = F; // format through a local
    __spirv_ocl_printf(f);
  }
}
