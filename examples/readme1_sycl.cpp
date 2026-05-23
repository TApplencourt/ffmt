#include <ffmt/base.hpp>
#include <sycl/sycl.hpp>

int main() {
  sycl::queue q;
  q.parallel_for(4, [=](sycl::id<1> i) {
    FFMT_PRINTLN("work-item {} says {}", i, "hello");
  }).wait();
}
