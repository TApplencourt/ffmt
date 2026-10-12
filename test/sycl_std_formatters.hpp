#pragma once
// Host-side fmt::formatter specializations for SYCL composite types.
// Mirror exactly the device-side output produced by ffmt::formatter
// so test reference output (fmt::format) can be diffed against device output.
//
// Test-only — kept here, not in the main header, to avoid pulling SYCL into
// the std namespace for users who only want device printing.

#include <fmt/format.h>
#include <sycl/sycl.hpp>

template <int N>
struct fmt::formatter<::sycl::range<N>> : fmt::formatter<std::string> {
  auto format(const ::sycl::range<N>& r, fmt::format_context& ctx) const {
    std::string s;
    for (int i = 0; i < N; i++) {
      if (i) s += 'x';
      s += std::to_string(r[i]);
    }
    return fmt::formatter<std::string>::format(s, ctx);
  }
};

template <int N>
struct fmt::formatter<::sycl::id<N>> : fmt::formatter<std::string> {
  auto format(const ::sycl::id<N>& id, fmt::format_context& ctx) const {
    std::string s = "(";
    for (int i = 0; i < N; i++) {
      if (i) s += ", ";
      s += std::to_string(id[i]);
    }
    s += ")";
    return fmt::formatter<std::string>::format(s, ctx);
  }
};
