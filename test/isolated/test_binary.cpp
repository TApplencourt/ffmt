// Cases using {:b} / {:B}, binary integers. The specifiers path cannot express these with printf.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME binary
#define TEST_INC "isolated/test_binary.cpp"
#include "../test_body.inc"
#else
// ── Integers ──────────────────────────────────────────────────────────────────
RUN(PRINT("{:b}\n", 255));
RUN(PRINT("{:B}\n", 255));
RUN(PRINT("{:#b}\n", 255));
RUN(PRINT("{:b}\n", -2147483647 - 1));
RUN(PRINT("{:b}\n", static_cast<uint64_t>(18446744073709551615ULL)));
RUN(PRINT("{:#b}\n", 0));
RUN(PRINT("{:b}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:#b}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:#b}\n", -2147483647 - 1));
RUN(PRINT("{:+#010b}\n", 42));
RUN(PRINT("{:+#010b}\n", -1));
RUN(PRINT("{: #010b}\n", 42));
RUN(PRINT("{:+#020b}\n", -42));

// ── Layout ────────────────────────────────────────────────────────────────────
RUN(PRINT("{:#^20b}\n", 255));
RUN(PRINT("{:>20b}\n", static_cast<uint8_t>(255)));
RUN(PRINT("{:1b}\n", 255));

// ── Misc ──────────────────────────────────────────────────────────────────────
RUN(PRINT("{0:x} {0:d} {0:b}\n", 255));
RUN(PRINT("{:+b}\n", 42));
RUN(PRINT("{: b}\n", 42));
RUN(PRINT("{:+b}\n", -1));
RUN(PRINT("{:b}\n", 0u));
RUN(PRINT("{:B}\n", 0u));
RUN(PRINT("{:#b}\n", 1));
RUN(PRINT("{:#010b}\n", 1));

// ── Type-instantiation coverage ───────────────────────────────────────────────
// write_int_rt<int8_t/short/int64_t/uint64_t/long>: integer specs through
// less common types so the per-type buffer_path::write_int_rt instantiations
// actually get exercised by coverage.
RUN(PRINT("{:b}\n", static_cast<int8_t>(-5)));
RUN(PRINT("{:b}\n", static_cast<short>(-300)));
RUN(PRINT("{:b}\n", static_cast<int64_t>(-1)));
RUN(PRINT("{:b}\n", static_cast<uint64_t>(0xDEADBEEFCAFEULL)));
RUN(PRINT("{:b}\n", 1L));

// ── Writes near the buffer cap ────────────────────────────────────────────────
#define FILL120 "........................................................................................................................"
// Each case starts with 120 filler chars so the next write straddles the
// 128-byte cap. Before the fix these wrote past fmt_buf::data (up to 23 bytes
// for 64 binary digits) — invisible in the truncated output, so they only
// fail under -fsanitize=address. Keep running the host suite with ASan.
RUN(PRINT("{}{:b}\n", FILL120, ~0ull));                       // write_uint_raw, base 2
#undef FILL120

#endif
