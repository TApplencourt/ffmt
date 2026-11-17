/* ROCm OpenMP offload, -O0, AMDGPU: live locals overlap.
 *
 * At -O0, locals whose address escapes inside a parallel region are put on
 * the device runtime's shared-memory stack (__kmpc_alloc_shared). Once about
 * 256 bytes are live in a thread, further blocks wrap back to the start of
 * the thread's region, over locals still in use: no free in between. In a
 * plain target region they come from global memory, and at -O2 they are not
 * moved at all; both are fine.
 *
 * ffmt hits it in println on the precision float path ({:e}, {:f}, {:g}):
 * fmt_sci's digit_writer and exact_decimal land inside println's fmt_buf,
 * the digits overwrite the writer's pointer, and the next access faults
 * (HSA_STATUS_ERROR_MEMORY_APERTURE_VIOLATION) on ids 7 and 21.
 *
 * Seen with ROCm 10.1.0 (AMD clang 24.0.0git d6f6cb69), MI300X (gfx942).
 *
 *   clang -fopenmp --offload-arch=gfx942 -O0 rocm_omp_O0_shared_stack_overlap.c && ./a.out
 *
 * Five nested calls with escaping locals of the sizes the compiler
 * allocates for ffmt::println("{:e}", 0.0); the innermost one records every
 * address and the host reports any two live locals that overlap. Prints
 * "BUG: live locals overlap" for the parallel region, "ok" for the plain
 * target region. */
#include <stdint.h>
#include <stdio.h>
#pragma omp declare target
struct rec { uintptr_t addr[9]; unsigned size[9]; int n; };
__attribute__((noinline)) void note(struct rec *r, void *p, unsigned size) {
  r->addr[r->n] = (uintptr_t)p; r->size[r->n] = size; r->n++;
}
__attribute__((noinline)) void f5(struct rec *r) {
  char a[24], b[176];
  note(r, a, sizeof a); note(r, b, sizeof b);
}
__attribute__((noinline)) void f4(struct rec *r) {
  char a[1], b[1];
  note(r, a, sizeof a); note(r, b, sizeof b); f5(r);
}
__attribute__((noinline)) void f3(struct rec *r) {
  char a[1];
  note(r, a, sizeof a); f4(r);
}
__attribute__((noinline)) void f2(struct rec *r) {
  int a; char b[8];
  note(r, &a, sizeof a); note(r, b, sizeof b); f3(r);
}
__attribute__((noinline)) void f1(struct rec *r) {
  char a[164]; char b[8];
  note(r, a, sizeof a); note(r, b, sizeof b); f2(r);
}
#pragma omp end declare target
int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  for (int shape = 0; shape < 2; shape++) {
    struct rec r = {{0}, {0}, 0};
    if (shape == 0) {
#pragma omp target parallel num_threads(1) map(tofrom: r)
      { struct rec l = {{0}, {0}, 0}; f1(&l); r = l; }
    } else {
#pragma omp target map(tofrom: r)
      { struct rec l = {{0}, {0}, 0}; f1(&l); r = l; }
    }
    int bad = 0;
    printf("%s:\n", shape == 0 ? "target parallel num_threads(1)" : "target");
    for (int i = 0; i < r.n; i++) {
      int ov = -1;
      for (int j = 0; j < i; j++)
        if (r.addr[i] < r.addr[j] + r.size[j] && r.addr[j] < r.addr[i] + r.size[i]) ov = j;
      if (ov >= 0) bad = 1;
      printf("  local of %3u bytes at 0x%llx", r.size[i], (unsigned long long)r.addr[i]);
      if (ov >= 0) printf("   OVERLAPS the %u-byte local at 0x%llx", r.size[ov], (unsigned long long)r.addr[ov]);
      printf("\n");
    }
    printf("  %s\n", bad ? "BUG: live locals overlap" : "ok");
  }
  return 0;
}
