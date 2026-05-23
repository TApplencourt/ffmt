#include <ffmt/base.hpp>

__global__ void hello() {
  FFMT_PRINTLN("work-item {} says {}", threadIdx.x, "hello");
}

int main() {
  hello<<<1, 4>>>();
  cudaDeviceSynchronize();
}
