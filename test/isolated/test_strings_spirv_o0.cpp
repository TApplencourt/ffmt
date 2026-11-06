// Strings and bools: what the specifiers path prints through %s. Intel's
// SPIR-V printf at -O0 crashes on a %s argument loaded from memory, which is
// how ffmt passes them (implementation_limitation/dpcpp_O0.cpp), and the
// device code is compiled as a whole: one such case takes every test of its
// binary down. Built as its own binary (test/isolated/), so the crash stays
// here: the -O0 SPIR-V ids list strings_spirv_o0:crash.
#ifndef TEST_INC
#define TEST_NAME strings_spirv_o0
#define TEST_INC "isolated/test_strings_spirv_o0.cpp"
#include "../test_body.inc"
#else

// Bool
RUN(PRINT("{}\n", false));
RUN(PRINT("{}\n", true));
RUN(PRINT("{:>10}\n", true));
RUN(PRINT("{:<10}\n", false));
RUN(PRINT("{:s}\n", true));
RUN(PRINT("{:s}\n", false));

// String
RUN(PRINT("{}\n", "hello world"));
RUN(PRINT("{:<20s}\n", "hello"));
RUN(PRINT("{:>20s}\n", "hello"));
RUN(PRINT("{:.5s}\n", "hello world"));
RUN(PRINT("{:.0s}\n", "hello"));
RUN(PRINT("{:.100s}\n", "hello"));
RUN(PRINT("{:>10.5s}\n", "hello world"));
RUN(PRINT("{:<10.3s}\n", "hello"));
RUN(PRINT("{:20s}\n", "hello"));
RUN(PRINT("[{:.3}]\n", "静夜思"));
RUN(PRINT("[{:.4}]\n", "静夜思"));
RUN(PRINT("[{:>5}]\n", "\xff\xe2\x94"));
// Overlong '/', a UTF-16 surrogate, and > U+10FFFF: each byte is invalid.
RUN(PRINT("[{:>6}]\n", "\xc0\xaf"));
RUN(PRINT("[{:>6}]\n", "\xed\xa0\x80"));
RUN(PRINT("[{:>6}]\n", "\xf4\x90\x80\x80"));
// A spec with no width or precision: copied without measuring.
RUN(PRINT("[{:s}] [{:<}]\n", "静夜思", "hôpital"));

// ── Writes near the buffer cap ────────────────────────────────────────────────
#define FILL120 "........................................................................................................................"
RUN(PRINT("{}{:o}\n", FILL120, ~0ull));                       // write_uint_raw, base 8
RUN(PRINT("{}{}\n", FILL120, ~0ull));                         // write_arg_default
RUN(PRINT("{}{}\n", FILL120, reinterpret_cast<void*>(0x123456789abcdef0ULL)));
RUN(PRINT("{}{:p}\n", FILL120, reinterpret_cast<void*>(0x123456789abcdef0ULL)));
RUN(PRINT("{}{}{}\n", FILL120, FILL120, 12345));              // already full
// println keeps its '\n' even when the line is truncated; before the fix the
// next "Test N" header was glued onto the truncated line.
RUN(PRINTLN("{}{}", FILL120, FILL120));
#undef FILL120

#endif
