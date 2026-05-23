#include "sycl_khx_print.hpp"

int main() {
  #pragma omp target teams distribute parallel for num_teams(1) thread_limit(4)
  for (int i = 0; i < 4; i++) {
    KHX_PRINTLN("work-item {} says {}", i, "hello");
  }
}
