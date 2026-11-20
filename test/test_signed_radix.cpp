// Cases using {:x} / {:o} on signed integers, and signs on them. The specifiers path cannot express these with printf.
// A file of its own, so that an id that cannot compile it loses only these
// tests (test/configs.json lists them under failed_to_compile).
#ifndef TEST_INC
#define TEST_NAME signed_radix
#define TEST_INC "test_signed_radix.cpp"
#include "test_body.inc"
#else
// ── Integers ──────────────────────────────────────────────────────────────────
RUN(PRINT("{:x}\n", -2147483647 - 1));
RUN(PRINT("{:x}\n", static_cast<int>(-1)));
// INT64_MIN with non-decimal bases
RUN(PRINT("{:x}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:X}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:o}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
// INT_MIN (32-bit) with non-decimal bases
RUN(PRINT("{:o}\n", -2147483647 - 1));
RUN(PRINT("{:X}\n", -2147483647 - 1));

// ── Strings ───────────────────────────────────────────────────────────────────
RUN(PRINTLN("{:08x}", 255));

// ── Layout ────────────────────────────────────────────────────────────────────
RUN(PRINT("{:+010x}\n", -1));
RUN(PRINT("{:1x}\n", 0xDEAD));

// ── Misc ──────────────────────────────────────────────────────────────────────
RUN(PRINT("{:+x}\n", 255));
RUN(PRINT("{:+x}\n", -1));
RUN(PRINT("{: x}\n", 255));
RUN(PRINT("{: x}\n", -1));
RUN(PRINT("{:+o}\n", 255));
RUN(PRINT("{: o}\n", 255));

// ── Type-instantiation coverage ───────────────────────────────────────────────
RUN(PRINT("{:x}\n", static_cast<int8_t>(-5)));
RUN(PRINT("{:x}\n", static_cast<short>(-300)));
RUN(PRINT("{:x}\n", static_cast<int64_t>(-1)));

#endif
