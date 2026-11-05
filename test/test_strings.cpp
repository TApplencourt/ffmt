#ifndef TEST_INC
#define TEST_NAME strings
#define TEST_INC "test_strings.cpp"
#include "test_select_body.inc"
#else

// Char
RUN(PRINT("{}\n", 'A'));
RUN(PRINT("{:d}\n", true));
RUN(PRINT("{}\n", 'Z'));
RUN(PRINT("{:d}\n", 'Z'));
RUN(PRINT("{:c}\n", 65));

// Bool/string as %s — see "spirv-o0" / "ptx-clang-o0" in capture.hpp.
RUN(PRINT("{}\n", false));
RUN(PRINT("{}\n", true));
RUN(PRINT("{:>10}\n", true));
RUN(PRINT("{:<10}\n", false));
RUN(PRINT("{:s}\n", true));
RUN(PRINT("{:s}\n", false));

// println
RUN(PRINTLN("hello println"));
RUN(PRINTLN("{} + {} = {}", 1, 2, 3));
RUN(PRINTLN("{:08x}", 255u));

// Dynamic char
RUN({
  volatile char c = 'Q';
  PRINT("{}\n", static_cast<char>(c));
});

// Width on char
RUN(PRINT("{:<10c}\n", 'B'));
RUN(PRINT("{:>10c}\n", 'C'));

// String %s tests — same gate.
RUN(PRINT("{}\n", "hello world"));
RUN(PRINT("{:<20s}\n", "hello"));
RUN(PRINT("{:>20s}\n", "hello"));
RUN(PRINT("{:.5s}\n", "hello world"));
RUN(PRINT("{:.0s}\n", "hello"));
RUN(PRINT("{:.100s}\n", "hello"));
RUN(PRINT("{:>10.5s}\n", "hello world"));
RUN(PRINT("{:<10.3s}\n", "hello"));

// ── Strings ───────────────────────────────────────────────────────────────────
RUN(PRINT("{:10c}\n", 'A'));
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

#endif
