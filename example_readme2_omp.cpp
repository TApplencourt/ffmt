#include "sycl_khx_print.hpp"

int main() {
  #pragma omp target teams distribute parallel for num_teams(1) thread_limit(4)
  for (int i = 0; i < 4; i++) {
    int id = i;
    float v = 3.14159f * (id + 1);
    KHX_PRINTLN("format used: 'id: {{0}}, v2dp={{1:6.2f}}, v={{1:8.5f}}' -> id: {0}, v2dp={1:6.2f}, v={1:8.5f}", id, v);
  }
}
