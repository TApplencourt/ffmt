#ifndef TEST_INC
#define TEST_NAME fast_math
#define TEST_INC "test_fast_math.cpp"
#include "test_body.inc"
#else

// Meaningful on the fast-math ids (-ffast-math). On x86 that enables DAZ
// (denormals-are-zero), and icpx does so by default: a subnormal then
// compares equal to 0.0, so the old `val == 0.0` checks printed 5e-324 as
// "0". Zero tests are now done on the bits.
//
// No reference compiled with these flags can be trusted: {fmt} itself prints
// {:e} of 5e-324 as "0.000000e+00" under DAZ (as does libstdc++'s
// std::format), and -0.0 as "0" (-fno-signed-zeros). Subnormal expectations are therefore literals, taken from
// {fmt} built without -ffast-math.
//
// Values go through a volatile so the widening and zero tests happen at
// run time; with plain literals the compiler folds them and DAZ never gets
// a chance to flush anything.
#ifdef FFMT_STD_PATH
#define EXPECT(want, fmt_str, v) printf("%s\n", want)
#else
#define EXPECT(want, fmt_str, v) PRINT(fmt_str "\n", opaque(v))
#endif

RUN(EXPECT("4.940656e-324", "{:e}", 4.9406564584124654e-324));
RUN(EXPECT("4.941e-324", "{:.3e}", 4.9406564584124654e-324));
RUN(EXPECT("4.94066e-324", "{:g}", 4.9406564584124654e-324));
RUN(EXPECT("2.225074e-308", "{:e}", 2.2250738585072009e-308));
RUN(EXPECT("1.401298e-45", "{:e}", 1e-45f));
RUN(EXPECT("-0", "{}", -0.0)); // -fno-signed-zeros: {fmt} built with it prints "0"
RUN(EXPECT("9.9999999999999694493e-311", "{:.20g}", 1e-310));
RUN(EXPECT("5e-324", "{}", 4.9406564584124654e-324));
RUN(EXPECT("-5e-324", "{}", -4.9406564584124654e-324));
RUN(EXPECT("2.225073858507201e-308", "{}", 2.2250738585072009e-308));
RUN(EXPECT("1e-45", "{}", 1e-45f));
#undef EXPECT

// Normal values: {fmt} is still a valid reference here.
RUN(PRINT("{:.17g}\n", 0.1));
RUN(PRINT("{:f}\n", 1e15));
RUN(PRINT("{:.3e}\n", 9.9995));
RUN(PRINT("{:.0f}\n", 2.5));
RUN(PRINT("{}\n", 0.0));

// NaN and infinity that appear at run time. -ffinite-math-only (part of
// -ffast-math) lets the compiler assume there are none: a literal such as
// quiet_NaN() may already be garbage before it reaches the call (UB, which
// clang reports with -Wnan-infinity-disabled), so the floats cases that use
// one are expected failures on the fast-math ids. A NaN computed or read at
// run time must still print as one, and ffmt tests the bits, not x != x.
// Only the integer bits go through opaque: a float parameter may be assumed
// not NaN. Expectations are literals, from {fmt} built without fast-math,
// as for the subnormals above: the reference's inlined {fmt} code is built
// with this file's flags, and this check must not depend on what they do.
#ifdef FFMT_STD_PATH
#define EXPECT_BITS(want, fmt_str, T, U, bits) printf("%s\n", want)
#else
#define EXPECT_BITS(want, fmt_str, T, U, bits) \
  PRINT(fmt_str "\n", __builtin_bit_cast(T, opaque(U(bits))))
#endif
RUN(EXPECT_BITS("nan", "{}", float, uint32_t, 0x7fc00000u)); // quiet NaN
RUN(EXPECT_BITS("-nan", "{}", float, uint32_t, 0xffc00000u)); // negative quiet NaN
RUN(EXPECT_BITS("nan", "{}", float, uint32_t, 0x7f800001u)); // signaling NaN
RUN(EXPECT_BITS("nan", "{}", float, uint32_t, 0x7fc12345u)); // NaN with a payload
RUN(EXPECT_BITS("inf", "{}", float, uint32_t, 0x7f800000u)); // inf
RUN(EXPECT_BITS("-inf", "{}", float, uint32_t, 0xff800000u)); // -inf
RUN(EXPECT_BITS("nan", "{:f}", float, uint32_t, 0x7fc00000u));
RUN(EXPECT_BITS("nan", "{:e}", float, uint32_t, 0x7fc00000u));
RUN(EXPECT_BITS("inf", "{:g}", float, uint32_t, 0x7f800000u));
RUN(EXPECT_BITS("NAN", "{:F}", float, uint32_t, 0x7fc00000u));
RUN(EXPECT_BITS("   inf", "{:>6}", float, uint32_t, 0x7f800000u));
RUN(EXPECT_BITS("      -inf", "{:010}", float, uint32_t, 0xff800000u)); // '0' is ignored for inf/nan
RUN(EXPECT_BITS("nan", "{}", double, uint64_t, 0x7ff8000000000000ull)); // quiet NaN
RUN(EXPECT_BITS("-nan", "{}", double, uint64_t, 0xfff8000000000000ull)); // negative quiet NaN
RUN(EXPECT_BITS("inf", "{}", double, uint64_t, 0x7ff0000000000000ull)); // inf
RUN(EXPECT_BITS("-inf", "{}", double, uint64_t, 0xfff0000000000000ull)); // -inf
RUN(EXPECT_BITS("nan", "{:e}", double, uint64_t, 0x7ff8000000000000ull));
RUN(EXPECT_BITS("-inf", "{:.3f}", double, uint64_t, 0xfff0000000000000ull));
#undef EXPECT_BITS

#endif
