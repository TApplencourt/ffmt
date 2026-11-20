// Cases using the '#' alternate form. The specifiers path cannot express these with printf.
// A file of its own, so that an id that cannot compile it loses only these
// tests (test/configs.json lists them under failed_to_compile).
#ifndef TEST_INC
#define TEST_NAME alternate
#define TEST_INC "test_alternate.cpp"
#include "test_body.inc"
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
