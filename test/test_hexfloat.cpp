// Cases using {:a} / {:A}, hexadecimal floating point. The specifiers path cannot express these with printf.
// Ids that cannot compile it list its tests under failed_to_compile in
// test/configs.json.
#ifndef TEST_INC
#define TEST_NAME hexfloat
#define TEST_INC "test_hexfloat.cpp"
#include "test_body.inc"
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

[[maybe_unused]] auto opaque = [](auto v) { volatile decltype(v) x = v; return x; };
#ifdef FFMT_STD_PATH
#define EXPECT(want, fmt_str, v) printf("%s\n", want)
#else
#define EXPECT(want, fmt_str, v) PRINT(fmt_str "\n", opaque(v))
#endif

RUN(EXPECT("0x0.0000000000001p-1022", "{:a}", 4.9406564584124654e-324));
RUN(EXPECT("0x0.fffffffffffffp-1022", "{:a}", 2.2250738585072009e-308));
RUN(EXPECT("0x1p-149", "{:a}", 1e-45f));
#undef EXPECT

{
  uint64_t st = 0x9E3779B97F4A7C15ULL;
  auto rnd = [&st]() { st ^= st << 13; st ^= st >> 7; st ^= st << 17; return st; };
  auto finite_bits = [&]<typename F, typename Bits>(size_t n) { // random finite F
    std::vector<F> v;
    while (v.size() < n)
      if (F x = __builtin_bit_cast(F, static_cast<Bits>(rnd())); x - x == 0) v.push_back(x);
    return v;
  };
  auto dv = finite_bits.operator()<double, uint64_t>(400);
  auto fv = finite_bits.operator()<float, uint32_t>(300);
  for (int i = 0; i < 200; i++) {                       // k / 10^j and k / 2^j
    double k = static_cast<double>(rnd() % 100000);
    dv.push_back(k / std::pow(10.0, static_cast<double>(rnd() % 8)));
    dv.push_back(std::ldexp(k + 0.5, -static_cast<int>(rnd() % 12)));
  }
#define SWEEP(vec, fmt) RUN(for (auto v : vec) PRINT(fmt "\n", v))
  SWEEP(dv, "{:a}");    SWEEP(dv, "{:.0a}");
  SWEEP(dv, "{:.5a}");  SWEEP(dv, "{:#.3a}");
  SWEEP(fv, "{:a}");    SWEEP(fv, "{:.2a}");
#undef SWEEP
}

#endif
