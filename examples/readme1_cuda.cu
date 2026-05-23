#include "sycl_khx_print.hpp"

__global__ void hello() {
  KHX_PRINTLN("work-item {} says {}", threadIdx.x, "hello");
}

int main() {
  hello<<<1, 4>>>();
  cudaDeviceSynchronize();
}
