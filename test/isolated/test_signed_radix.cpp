// Cases using {:x} / {:o} on signed integers, and signs on them. The specifiers path cannot express these with printf.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME signed_radix
#define TEST_INC "isolated/test_signed_radix.cpp"
#include "../test_body.inc"
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
