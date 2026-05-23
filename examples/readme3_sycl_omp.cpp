// SYCL + OpenMP-target in the same TU. Compile with both -fsycl and
// -fiopenmp -fopenmp-targets=spir64. The header's SYCL detection
// (SYCL_LANGUAGE_VERSION) wins over the OpenMP auto-install — so both the
// SYCL kernel and the OpenMP target region funnel through the SYCL printf
// path (sycl::ext::oneapi::experimental::printf).

#include <ffmt/base.hpp>
#include <sycl/sycl.hpp>

int main() {
  // SYCL kernel
  sycl::queue q;
  q.parallel_for(2, [=](sycl::id<1> i) {
    FFMT_PRINTLN("sycl  work-item {} says hello", static_cast<int>(i));
  }).wait();

  // OpenMP target region in the same binary
  #pragma omp target teams distribute parallel for num_teams(1) thread_limit(2)
  for (int i = 0; i < 2; i++) {
    FFMT_PRINTLN("omp   work-item {} says hello", i);
  }
}
