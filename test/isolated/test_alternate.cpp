// Cases using the '#' alternate form. The specifiers path cannot express these with printf.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME alternate
#define TEST_INC "isolated/test_alternate.cpp"
#include "../test_body.inc"
#else
// ── Integers ──────────────────────────────────────────────────────────────────
RUN(PRINT("{:#x}\n", 255));
RUN(PRINT("{:#x}\n", 0));
RUN(PRINT("{:#x}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:#o}\n", -2147483647 - 1));
// Sign + alternate + zero-pad combinations
RUN(PRINT("{:+#010x}\n", 255));
RUN(PRINT("{:+#010x}\n", -1));
RUN(PRINT("{: #010x}\n", 255));
RUN(PRINT("{: #010x}\n", -1));
RUN(PRINT("{:+#010o}\n", 255));
RUN(PRINT("{: #010o}\n", 255));
RUN(PRINT("{:+#010X}\n", 255));

// ── Misc ──────────────────────────────────────────────────────────────────────
RUN(PRINT("{:#010x}\n", 255));
RUN(PRINT("hex={:#x} pi={:.4f}\n", 255, 3.14159));
RUN(PRINT("{:#010x}\n", 0));
RUN(PRINT("{:#o}\n", 0));
RUN(PRINT("{:#X}\n", 255));
RUN(PRINT("{:#o}\n", 8));

// ── Type-instantiation coverage ───────────────────────────────────────────────
RUN(PRINT("{:#x}\n", -1L));

// ── Writes near the buffer cap ────────────────────────────────────────────────
#define FILL120 "........................................................................................................................"
RUN(PRINT("{}{:#X}\n", FILL120, ~0ull));                      // prefix + base 16
#undef FILL120

// ── Width overflow ──────────────────────────────────────────────────────────
RUN(PRINT("{:#200x}\n", 0xff));     // prefix+overflow: hits prefix-bound check

#endif
