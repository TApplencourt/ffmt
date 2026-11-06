// Cases using {:a} / {:A}, hexadecimal floating point. The specifiers path cannot express these with printf.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME hexfloat
#define TEST_INC "isolated/test_hexfloat.cpp"
#include "../test_body.inc"
#else
// ── Floats ────────────────────────────────────────────────────────────────────
RUN(PRINT("{:a}\n", 3.14159));
RUN(PRINT("{:A}\n", 3.14159));
RUN(PRINT("{:a}\n", 0.0));
RUN(PRINT("{:a}\n", -0.0));
RUN(PRINT("{:a}\n", 1.0));
RUN(PRINT("{:#a}\n", 3.14159));

// ── Misc ──────────────────────────────────────────────────────────────────────
RUN(PRINT("{:a}\n", 1.0 / 0.0));
RUN(PRINT("{:A}\n", 0.0 / 0.0));
RUN(PRINT("{:a}\n", 0.125));
RUN(PRINT("{:+a}\n", 3.14));
RUN(PRINT("{:+a}\n", -3.14));
RUN(PRINT("{: a}\n", 3.14));
RUN(PRINT("{: a}\n", -3.14));
RUN(PRINT("{:+A}\n", 1.0));
RUN(PRINT("{:a}\n", 0.0));
RUN(PRINT("{:<20a}\n", 3.14));
RUN(PRINT("{:>20a}\n", 3.14));
RUN(PRINT("{:^20a}\n", 3.14));
RUN(PRINT("{:*>20a}\n", 3.14));

// ── Type-instantiation coverage ───────────────────────────────────────────────
// hex_float_to_buf_rt<float>: {:a} on float (tests above only use double).
// PTX-clang at -O0 emits the exponent letter as uppercase 'P' even for
// the lowercase '{:a}' spec — distinct from the prefix/sign cluster above.
RUN(PRINT("{:a}\n", 3.14f));
RUN(PRINT("{:A}\n", 3.14f));
RUN(PRINT("{:a}\n", 0.0f));
RUN(PRINT("{:a}\n", -0.0f));
RUN(PRINT("{:a}\n", std::numeric_limits<float>::infinity()));
RUN(PRINT("{:#a}\n", 1.5f));

// ── Writes near the buffer cap ────────────────────────────────────────────────
#define FILL120 "........................................................................................................................"
RUN(PRINT("{}{:a}\n", FILL120, 1e300));                       // hex-float exponent digits
#undef FILL120

// ── Float formatting fixes ────────────────────────────────────────────────────
// Hex float: precision rounds half up like {fmt} (and may carry into the
// leading digit); '0' pads after the "0x".
RUN(PRINT("{:.0a}\n", 1.5));
RUN(PRINT("{:.0a}\n", 1.96875));
RUN(PRINT("{:.1a}\n", 1.03125));
RUN(PRINT("{:.1a}\n", 1.09375));
RUN(PRINT("{:.3a}\n", 1.0 / 3.0));
RUN(PRINT("{:.20a}\n", 1.0 / 3.0));
RUN(PRINT("{:.2a}\n", 0.0));
RUN(PRINT("{:#.0a}\n", 1.0));
RUN(PRINT("{:.3A}\n", -0.1f));
// Subnormal hex, where the standard libraries disagree with each other and
// with {fmt}: a float is printed as the equivalent double.
RUN(PRINT("{:a}\n", 1e-45f));                           // FLT_TRUE_MIN
RUN(PRINT("{:a}\n", 4.9406564584124654e-324));         // DBL_TRUE_MIN
RUN(PRINT("{:a}\n", 1.1754942e-38f));                   // largest float subnormal
RUN(PRINT("{:a}\n", -2.2250738585072009e-308));        // largest double subnormal
RUN(PRINT("{:.3a}\n", 4.9406564584124654e-324));
RUN(PRINT("{:.1a}\n", 1.3e-44f));
RUN(PRINT("[{:012a}]\n", -1.0));
RUN(PRINT("[{:+012a}]\n", 1.0));

#endif
