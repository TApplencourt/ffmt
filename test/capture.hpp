#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

#include <ffmt/base.hpp>
// Pull in <sycl/sycl.hpp> only when the TU is built for a SYCL backend
// (so capture_stdout can take a sycl::queue&). Host test rig and OpenMP
// backends don't define SYCL_LANGUAGE_VERSION and don't have ACPP, so
// they skip this include.
#if defined(SYCL_LANGUAGE_VERSION) || FFMT_COMPILER_ACPP
#include <sycl/sycl.hpp>
#endif

constexpr int N = 2;

// Exit status of a test binary that ran to the end and saw a failing test.
// Unlike 1 (ASan, UBSan), 23 (LSan) or 128+n (a signal), it cannot come from
// a crash, so test/check_id.py can tell the two apart.
constexpr int TEST_FAILED = 88;

static std::string capture_stdout(auto&& fn) {
  std::cout.flush();
  fflush(stdout);

  // tmpfile() rather than Linux-only memfd_create: an anonymous, already-
  // unlinked regular file on any POSIX libc (Linux, macOS, BSD).
  FILE* tmp = std::tmpfile();
  assert(tmp);
  int mem_fd = fileno(tmp);
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
  ssize_t n = ::read(mem_fd, result.data(), size);
  assert(n == size);  // regular file: full read or EOF, no shorts
  fclose(tmp);
  return result;
}

// Split a captured stream into blocks keyed by `Test N` markers. Each block
// is the text emitted *after* that marker and before the next one. Anything
// before the first marker (e.g. setup noise) is ignored.
[[maybe_unused]] static std::map<int, std::string>
parse_blocks(const std::string& s) {
  std::map<int, std::string> out;
  std::istringstream in(s);
  std::string line;
  int id = -1;
  while (std::getline(in, line)) {
    if (line.starts_with("Test ")) {
      id = std::atoi(line.c_str() + 5);
      out.try_emplace(id);
    } else if (id >= 0) {
      out[id] += "    " + line + "\n";
    }
  }
  return out;
}

// TAP-ish per-test reporter. Parses each stream independently so a desync
// in one block doesn't bleed into later tests (previously: one shorter
// `Actual:` swallowed every passing test after it). Emits one line per
// test id and only prints the expected/actual diff on failure. Returns
// true when every test matched.
[[maybe_unused]] static bool diff_output(const char* name,
                        const std::string& expected,
                        const std::string& actual) {
  auto exp_blocks = parse_blocks(expected);
  auto act_blocks = parse_blocks(actual);
  // Union of ids, sorted ascending — std::map iterates in order; merge keys.
  std::set<int> ids;
  for (auto& [k, _] : exp_blocks) ids.insert(k);
  for (auto& [k, _] : act_blocks) ids.insert(k);
  int passed = 0, failed = 0;
  for (int id : ids) {
    const auto& e = exp_blocks[id];
    const auto& a = act_blocks[id];
    if (e == a) {
      fprintf(stderr, "ok %d - %s\n", id, name);
      ++passed;
    } else {
      fprintf(stderr, "not ok %d - %s\n", id, name);
      fprintf(stderr, "  Expected:\n%s", e.c_str());
      fprintf(stderr, "  Actual:\n%s", a.c_str());
      ++failed;
    }
  }
  fprintf(stderr, "# %s: %d/%zu passed\n", name, passed, ids.size());
  return failed == 0;
}
