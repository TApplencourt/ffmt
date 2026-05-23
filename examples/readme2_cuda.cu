#include <ffmt/base.hpp>

__global__ void kernel() {
  int id = threadIdx.x;
  float v = 3.14159f * (id + 1);
  FFMT_PRINTLN("format used: 'id: {{0}}, v2dp={{1:6.2f}}, v={{1:8.5f}}' -> id: {0}, v2dp={1:6.2f}, v={1:8.5f}", id, v);
}

int main() {
  kernel<<<1, 4>>>();
  cudaDeviceSynchronize();
}
