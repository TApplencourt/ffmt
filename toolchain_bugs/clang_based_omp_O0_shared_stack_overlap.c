/* Clang-based OpenMP offload, -O0: two live locals get the same address.
 *
 * In a target parallel region at -O0, locals whose address escapes are
 * allocated by the OpenMP device runtime (__kmpc_alloc_shared). A local that
 * starts right after 256 bytes of live locals is placed back at the start of
 * the thread's area, over locals still in use: b gets a's address. With
 * a[200] instead of a[256], or at -O2, the two are apart.
 *
 * Reported upstream: https://github.com/llvm/llvm-project/issues/245328
 *
 * Reproduces with the compiler and flags of ids 5, 19 (NVPTX) and 7, 21 (ROCm) (test/run_id.sh).
 * ffmt hits it in println on the precision float path ({:e}, {:f}, {:g}):
 * fmt_sci's digit_writer and exact_decimal land inside println's fmt_buf,
 * the digits overwrite the writer's pointer, and the next access faults
 * (HSA_STATUS_ERROR_MEMORY_APERTURE_VIOLATION on ids 7 and 21).
 *
 *   clang -fopenmp --offload-arch=sm_80 -O0 clang_based_omp_O0_shared_stack_overlap.c && ./a.out
 *
 * Exits 1 when a and b overlap. */
#include <stdint.h>
#include <stdio.h>
__attribute__((noinline)) void inner(char *a, uintptr_t *addr) {
  char b[16];
  addr[0] = (uintptr_t)a;
  addr[1] = (uintptr_t)b;
}
__attribute__((noinline)) void outer(uintptr_t *addr) {
  char a[256];
  inner(a, addr);
}
int main(void) {
  uintptr_t addr[2];
#pragma omp target parallel num_threads(1) map(from : addr)
  outer(addr);
  printf("a[256] at 0x%llx, b[16] at 0x%llx\n", (unsigned long long)addr[0],
         (unsigned long long)addr[1]);
  if (addr[1] < addr[0] + 256 && addr[0] < addr[1] + 16) {
    printf("BUG: a and b overlap\n");
    return 1;
  }
  return 0;
}
