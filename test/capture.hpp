#pragma once

#include <cstdint>
#include <cstdio>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

#include "../sycl_khx_print.hpp"
// Pull in <sycl/sycl.hpp> only when the TU is built for a SYCL backend
// (so capture_stdout can take a sycl::queue&). Host test rig and OpenMP
// backends don't define SYCL_LANGUAGE_VERSION and don't have ACPP, so
// they skip this include.
#if defined(SYCL_LANGUAGE_VERSION) || FMT_SYCL_COMPILER_ACPP
#include <sycl/sycl.hpp>
#endif

constexpr int N = 2;

static std::string capture_stdout(auto&& fn) {
  std::cout.flush();
  fflush(stdout);

  int mem_fd = memfd_create("capture", 0);
  int saved_fd = dup(STDOUT_FILENO);
  dup2(mem_fd, STDOUT_FILENO);

  fn();

  std::cout.flush();
  fflush(stdout);
  dup2(saved_fd, STDOUT_FILENO);
  close(saved_fd);

  auto size = lseek(mem_fd, 0, SEEK_END);
  lseek(mem_fd, 0, SEEK_SET);
  std::string result(size, '\0');
  for (ssize_t off = 0; off < size; ) {
    ssize_t n = ::read(mem_fd, result.data() + off, size - off);
    if (n <= 0) break;
    off += n;
  }
  close(mem_fd);
  return result;
}

// unused in test_main_{sycl,host}.cpp which include this header but only
// call test_*()
[[maybe_unused]] static bool diff_output(const char* name,
                        const std::string& expected,
                        const std::string& actual) {
  if (expected == actual)
    return true;
  std::istringstream a(expected), b(actual);
  std::string la, lb;
  int test_id = 0;
  bool ga, gb, mismatch = false;
  std::string exp_block, act_block;
  auto flush = [&]() {
    if (mismatch) {
      fprintf(stderr, "[  FAILED  ] %s / test %d\n", name, test_id);
      fprintf(stderr, "  Expected:\n%s", exp_block.c_str());
      fprintf(stderr, "  Actual:\n%s", act_block.c_str());
    }
    exp_block.clear();
    act_block.clear();
    mismatch = false;
  };
  while (ga = bool(std::getline(a, la)),
         gb = bool(std::getline(b, lb)),
         ga || gb) {
    if (la.starts_with("Test ") && la == lb) {
      flush();
      test_id = std::atoi(la.c_str() + 5);
    } else {
      if (la != lb) mismatch = true;
      exp_block += "    " + la + "\n";
      act_block += "    " + lb + "\n";
    }
    la.clear();
    lb.clear();
  }
  flush();
  return false;
}
