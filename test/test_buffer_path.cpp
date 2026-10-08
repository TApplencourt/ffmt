#ifndef TEST_INC
#define TEST_NAME buffer_path
#define TEST_INC "test_buffer_path.cpp"
#include "test_select_body.inc"
#else

// ── Integers (from test_integers.cpp) ──
RUN(PRINT("{:b}\n", 255));
RUN(PRINT("{:B}\n", 255));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#x}\n", 255));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#b}\n", 255));
RUN(PRINT("{:x}\n", -2147483647 - 1));
RUN(PRINT("{:b}\n", -2147483647 - 1));
RUN(PRINT("{:b}\n", static_cast<uint64_t>(18446744073709551615ULL)));
RUN(PRINT("{:#b}\n", 0));
RUN(PRINT("{:x}\n", static_cast<int>(-1)));
RUN(PRINT("{:#x}\n", 0));

// INT64_MIN with non-decimal bases
RUN(PRINT("{:x}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:X}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:o}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:b}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#x}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#b}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));

// INT_MIN (32-bit) with non-decimal bases
RUN(PRINT("{:o}\n", -2147483647 - 1));
RUN(PRINT("{:X}\n", -2147483647 - 1));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#o}\n", -2147483647 - 1));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#b}\n", -2147483647 - 1));

// Sign + alternate + zero-pad combinations
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+#010x}\n", 255));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+#010x}\n", -1));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{: #010x}\n", 255));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{: #010x}\n", -1));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+#010b}\n", 42));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+#010b}\n", -1));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{: #010b}\n", 42));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+#010o}\n", 255));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{: #010o}\n", 255));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+#010X}\n", 255));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+#020b}\n", -42));

// ── Floats (from test_floats.cpp) ──
RUN(PRINT("{}\n", 1.0e10f));
RUN(PRINT("{}\n", 1.0e-10f));
RUN(PRINT("{}\n", 1.0e100));
RUN(PRINT("{}\n", 1.0e-100));

// Float extremes — default format
RUN(PRINT("{}\n", std::numeric_limits<float>::max()));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{}\n", -std::numeric_limits<float>::max()));
RUN(PRINT("{}\n", std::numeric_limits<float>::min()));
RUN(PRINT("{}\n", std::numeric_limits<float>::denorm_min()));
RUN(PRINT("{}\n", -0.0f));
RUN(PRINT("{}\n", std::numeric_limits<double>::max()));
RUN(PRINT("{}\n", std::numeric_limits<double>::min()));
RUN(PRINT("{}\n", -0.0));
RUN(PRINT("{}\n", std::numeric_limits<float>::infinity()));
RUN(PRINT("{}\n", -std::numeric_limits<float>::infinity()));
RUN(PRINT("{}\n", std::numeric_limits<float>::quiet_NaN()));

SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:a}\n", 3.14159));
RUN(PRINT("{:A}\n", 3.14159));
RUN(PRINT("{:a}\n", 0.0));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:a}\n", -0.0));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:a}\n", 1.0));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#a}\n", 3.14159));
RUN(PRINT("{:e}\n", 5e-324));
RUN(PRINT("{:g}\n", 5e-324));

// ── Strings (from test_strings.cpp) ──
RUN(PRINT("{:*^10}\n", true));
RUN(PRINTLN("{:08x}", 255));
RUN(PRINT("{:10c}\n", 'A'));
RUN(PRINT("{:^10c}\n", 'D'));
RUN(PRINT("{:*>5c}\n", 'X'));
RUN(PRINT("{:*<5c}\n", 'X'));
RUN(PRINT("{:*^5c}\n", 'X'));
#define _S (FFMT_SPIRV_O0 || FFMT_PTX_CLANG_O0)
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:20s}\n", "hello"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:*^10.5s}\n", "hello world"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:*<20s}\n", "hello"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:*>20s}\n", "hello"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:*^20s}\n", "hello"));
#undef _S

// ── Layout (from test_layout.cpp) ──
RUN(PRINT("{:^10d}\n", 42));
RUN(PRINT("{:*<10d}\n", 42));
RUN(PRINT("{:*>10d}\n", 42));
RUN(PRINT("{:*^10d}\n", 42));
RUN(PRINT("{:#^20b}\n", 255));
RUN(PRINT("{:+010x}\n", -1));
RUN(PRINT("{:>20b}\n", static_cast<uint8_t>(255)));
RUN(PRINT("{:*>5d}\n", -42));
RUN(PRINT("{:<10x}\n", 255u));
RUN(PRINT("{:>10x}\n", 255u));
RUN(PRINT("{:^10x}\n", 255u));
RUN(PRINT("{:<10o}\n", 255u));
RUN(PRINT("{:>10o}\n", 255u));
RUN(PRINT("{:*<10x}\n", 255u));
RUN(PRINT("{:*>10x}\n", 255u));
RUN(PRINT("{:*^10x}\n", 255u));
RUN(PRINT("{:*<10o}\n", 255u));
RUN(PRINT("{:1x}\n", 0xDEAD));
RUN(PRINT("{:1b}\n", 255));
RUN(PRINT("{:*<10}\n", true));
RUN(PRINT("{:*>10}\n", false));
RUN(PRINT("{:*^10}\n", true));

// ── Misc (from test_misc.cpp) ──
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#010x}\n", 255));
RUN(PRINT("{:*>15.6f}\n", 3.14));
RUN(PRINT("hex={:#x} pi={:.4f}\n", 255, 3.14159));
RUN(PRINT("{:a}\n", 1.0 / 0.0));
RUN(PRINT("{:A}\n", 0.0 / 0.0));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:a}\n", 0.125));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{0:x} {0:d} {0:b}\n", 255));
RUN(PRINT("{:{}d}\n", 42, 10));
RUN(PRINT("{:{}d}\n", 42, 1));
RUN(PRINT("{:.{}f}\n", 3.14159, 2));
RUN(PRINT("{:.{}f}\n", 4.14159, 0));
RUN(PRINT("{:{}.{}f}\n", 5.14, 15, 6));
RUN(PRINT("{0:{1}x}\n", 255, 10));
RUN(PRINT("{0:{2}.{1}f}\n", 6.14, 4, 20));
RUN(PRINT("{:+x}\n", 255));
RUN(PRINT("{:+x}\n", -1));
RUN(PRINT("{: x}\n", 255));
RUN(PRINT("{: x}\n", -1));
RUN(PRINT("{:+o}\n", 255));
RUN(PRINT("{: o}\n", 255));
RUN(PRINT("{:+b}\n", 42));
RUN(PRINT("{: b}\n", 42));
RUN(PRINT("{:+b}\n", -1));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+a}\n", 3.14));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:+a}\n", -3.14));
RUN(PRINT("{: a}\n", 3.14));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{: a}\n", -3.14));
RUN(PRINT("{:+A}\n", 1.0));
RUN(PRINT("{:b}\n", 0u));
RUN(PRINT("{:B}\n", 0u));
RUN(PRINT("{:a}\n", 0.0));
RUN(PRINT("{:{}d}\n", 42, 0));
RUN(PRINT("{:.{}f}\n", 3.14159, 10));
RUN(PRINT("{:{}c}\n", 'A', 5));
RUN(PRINT("{:{}}\n", true, 10));
RUN(PRINT("{:{}.{}f}\n", 3.14, 20, 0));
RUN(PRINT("{:#010x}\n", 0));
RUN(PRINT("{:#o}\n", 0));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#b}\n", 1));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#010b}\n", 1));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#X}\n", 255));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#o}\n", 8));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:<20a}\n", 3.14));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:>20a}\n", 3.14));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:^20a}\n", 3.14));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:*>20a}\n", 3.14));

// Null-pointer formatting — buffer-path-only because the specifiers path
// routes through libc printf which emits "(nil)" for null pointers, while
// std::format (and our buffer-path implementation) emit "0x0".
RUN(PRINT("{}\n",         static_cast<void*>(nullptr)));
RUN(PRINT("{:p}\n",       static_cast<void*>(nullptr)));
// Centre alignment on pointers — '^' is buffer-path-only generally.
RUN(PRINT("[{:^20p}]\n",  reinterpret_cast<void*>(0xdeadbeef)));

// ── Type-instantiation coverage ──
// write_int_rt<int8_t/short/int64_t/uint64_t/long>: integer specs through
// less common types so the per-type buffer_path::write_int_rt instantiations
// actually get exercised by coverage.
RUN(PRINT("{:b}\n", static_cast<int8_t>(-5)));
RUN(PRINT("{:x}\n", static_cast<int8_t>(-5)));
RUN(PRINT("{:b}\n", static_cast<short>(-300)));
RUN(PRINT("{:x}\n", static_cast<short>(-300)));
RUN(PRINT("{:b}\n", static_cast<int64_t>(-1)));
RUN(PRINT("{:x}\n", static_cast<int64_t>(-1)));
RUN(PRINT("{:b}\n", static_cast<uint64_t>(0xDEADBEEFCAFEULL)));
RUN(PRINT("{:o}\n", static_cast<uint64_t>(0xDEADBEEFCAFEULL)));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:b}\n", 1L));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#x}\n", -1L));

// hex_float_to_buf_rt<float>: {:a} on float (tests above only use double).
// PTX-clang at -O0 emits the exponent letter as uppercase 'P' even for
// the lowercase '{:a}' spec — distinct from the prefix/sign cluster above.
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:a}\n", 3.14f));
RUN(PRINT("{:A}\n", 3.14f));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:a}\n", 0.0f));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:a}\n", -0.0f));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:a}\n", std::numeric_limits<float>::infinity()));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("{:#a}\n", 1.5f));

// resolve_int_arg dispatch for type combinations not hit elsewhere:
// dynamic width/precision with mixed integer types and floats.
RUN(PRINT("{:{}d}\n", 42, static_cast<short>(8)));      // resolve_int_arg<short>
RUN(PRINT("{:{}d}\n", 42, static_cast<int64_t>(8)));    // resolve_int_arg<int64_t>
RUN(PRINT("{:{}d}\n", 42, static_cast<uint64_t>(8)));   // resolve_int_arg<uint64_t>
RUN(PRINT("{:{}d}\n", 42, 8L));                         // resolve_int_arg<long>
RUN(PRINT("{:{}d}\n", 42, static_cast<unsigned>(8)));   // resolve_int_arg<unsigned>
RUN(PRINT("{:{}d}\n", 42, static_cast<int8_t>(8)));     // resolve_int_arg<int8_t>
RUN(PRINT("{:{}d}\n", 42, static_cast<uint8_t>(8)));    // resolve_int_arg<uint8_t>

// ── Writes near the buffer cap ──────────────────────────────────────────────
// Each case starts with 120 filler chars so the next write straddles the
// 128-byte cap. Before the fix these wrote past fmt_buf::data (up to 23 bytes
// for 64 binary digits) — invisible in the truncated output, so they only
// fail under -fsanitize=address. Keep running the host suite with ASan.
#define FILL120 "........................................................................................................................"
RUN(PRINT("{}{:b}\n", FILL120, ~0ull));                       // write_uint_raw, base 2
RUN(PRINT("{}{:o}\n", FILL120, ~0ull));                       // write_uint_raw, base 8
RUN(PRINT("{}{:#X}\n", FILL120, ~0ull));                      // prefix + base 16
RUN(PRINT("{}{}\n", FILL120, ~0ull));                         // write_arg_default
RUN(PRINT("{}{}\n", FILL120, reinterpret_cast<void*>(0x123456789abcdef0ULL)));
RUN(PRINT("{}{:p}\n", FILL120, reinterpret_cast<void*>(0x123456789abcdef0ULL)));
RUN(PRINT("{}{:a}\n", FILL120, 1e300));                       // hex-float exponent digits
RUN(PRINT("{}{}{}\n", FILL120, FILL120, 12345));              // already full
// println keeps its '\n' even when the line is truncated; before the fix the
// next line was glued onto the truncated one.
RUN(PRINTLN("{}{}", FILL120, FILL120); PRINT("next line\n"));
#undef FILL120

// ── Null char* ──────────────────────────────────────────────────────────────
// std::format leaves a null const char* undefined; on a GPU it is an illegal
// access. The buffer path prints glibc's "(null)" instead of faulting.
#ifdef FFMT_STD_PATH
RUN(printf("(null)\n"));
RUN(printf("[    (null)]\n"));
RUN(printf("[(nu]\n"));
#else
RUN(PRINT("{}\n", static_cast<const char*>(nullptr)));
RUN(PRINT("[{:>10}]\n", static_cast<const char*>(nullptr)));
RUN(PRINT("[{:.3}]\n", static_cast<const char*>(nullptr)));
#endif

// ── escape_percent_inplace (host-only unit test) ───────────────────────────
// The ACPP device emit escapes % → %% in place. When the escaped text no
// longer fits, it must keep the longest fitting prefix (never half of a %%
// pair) and keep println's trailing '\n'. Before the fix the right-to-left
// copy overran unread source bytes and the output came out as "%%%%…".
#if !defined(_OPENMP) && !defined(__CUDACC__) && \
    !(defined(SYCL_LANGUAGE_VERSION) || FFMT_COMPILER_ACPP)
{
  auto make_input = [](int n, bool nl) {
    std::string in;
    for (int i = 0; i < n; i++) in += (i % 10 == 9) ? '%' : char('a' + i % 10);
    if (nl) in += '\n';
    return in;
  };
  for (int n : {5, 100, 116, 117, 118, 128}) {
    for (bool nl : {false, true}) {
      std::string in = make_input(n, nl);
#ifdef FFMT_STD_PATH
      // Reference: escape char by char while the result fits in the cap.
      std::string body = nl ? in.substr(0, in.size() - 1) : in, want;
      for (char c : body) {
        std::string e = (c == '%') ? "%%" : std::string(1, c);
        if (want.size() + e.size() > FFMT_BUFFER_SIZE) break;
        want += e;
      }
      if (nl) want += '\n';
      RUN(printf("[%s]\n", want.c_str()));
#else
      ffmt::detail::fmt_buf b;
      for (char c : in) b.data[b.len++] = c;
      ffmt::detail::buffer_path::escape_percent_inplace(b);
      b.data[b.len] = '\0';
      RUN(printf("[%s]\n", b.data));
#endif
    }
  }
}
#endif

#endif
