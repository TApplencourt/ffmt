/* Clang-based OpenMP offload to NVPTX, -O2: a local constant array read at
 * a run-time index faults ("an illegal memory access was encountered").
 *
 * openmp-opt marks the loads from the local array as local (addrspace(5)),
 * then InstCombine replaces the array with the constant global it is
 * initialized from and keeps those casts: the kernel reads local memory at
 * the global's address. A plain "omp target" region and -O0 are fine.
 * Not affected: nvc++ -mp=gpu, the same kernel in CUDA (nvcc 12.3, clang
 * 23.1.6), ROCm 10.1.0 OpenMP on gfx942.
 *
 * Reported upstream: https://github.com/llvm/llvm-project/issues/245290
 *
 * Reproduces with the compiler and flags of ids 6 and 20, as set by
 * test/setup/;
 * ffmt's unqualified local p10[] hit it in float_sweep and hexfloat_sweep;
 * test/sweep_values.inc now uses static const to avoid the bad rewrite.
 *
 *   clang -fopenmp --offload-arch=sm_80 -O2 clang_based_omp_O2_const_array.c && ./a.out
 *
 * Expected: 10 100 1000 10000 100000 1e+06 1e+07 1 */
#include <stdio.h>
int main(int argc, char **argv) {
  (void)argv;
  double out[8];
#pragma omp target teams distribute parallel for map(from : out) firstprivate(argc)
  for (int i = 0; i < 8; i++) {
    const double p10[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7};
    out[i] = p10[(i + argc) % 8];
  }
  for (int i = 0; i < 8; i++) printf("%g ", out[i]);
  printf("\n");
}
