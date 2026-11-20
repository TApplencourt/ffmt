// Differential sweep against {fmt}: deterministic pseudo-random floats.
// Ids that cannot compile it list its tests under failed_to_compile in
// test/configs.json.
#ifndef TEST_INC
#define TEST_NAME float_sweep
#define TEST_INC "test_float_sweep.cpp"
#include "test_body.inc"
#else
// ── Differential sweep against {fmt} ───────────────────────────────────────
// Deterministic pseudo-random doubles/floats (raw bit patterns, so every
// exponent and subnormals are hit) plus short decimals that land on rounding
// ties. Far denser than the hand-picked cases above; this is what would have
// caught the >2^53 precision bugs.
//
// Each case builds the values at compile time (constexpr), so they reach the
// device in the kernel's own code. A host std::vector captured by the kernel
// would carry a host heap pointer that the device cannot read.
{
  struct sweep {
    double d[800];
    float f[300];
    static constexpr sweep make() {
      sweep s{};
      uint64_t st = 0x9E3779B97F4A7C15ULL;
      auto rnd = [&st]() { st ^= st << 13; st ^= st >> 7; st ^= st << 17; return st; };
      // Finite only, tested on the bits: a NaN is not a constant expression.
      for (int n = 0; n < 400;)
        if (uint64_t b = rnd(); (b >> 52 & 0x7ff) != 0x7ff) s.d[n++] = __builtin_bit_cast(double, b);
      for (int n = 0; n < 300;)
        if (auto b = static_cast<uint32_t>(rnd()); (b >> 23 & 0xff) != 0xff) s.f[n++] = __builtin_bit_cast(float, b);
      // k / 10^j and k / 2^j, exact powers: std::pow and std::ldexp are not constexpr.
      constexpr double p10[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7};
      for (int i = 0; i < 200; i++) {
        double k = static_cast<double>(rnd() % 100000);
        s.d[400 + 2 * i] = k / p10[rnd() % 8];
        s.d[401 + 2 * i] = (k + 0.5) / static_cast<double>(1u << (rnd() % 12));
      }
      return s;
    }
  };
#define SWEEP(vec, fmt) RUN(constexpr sweep s = sweep::make(); for (auto v : s.vec) PRINT(fmt "\n", v))
  SWEEP(d, "{}");      SWEEP(d, "{:f}");    SWEEP(d, "{:.0f}");
  SWEEP(d, "{:.3f}");  SWEEP(d, "{:.17f}"); SWEEP(d, "{:e}");
  SWEEP(d, "{:.0e}");  SWEEP(d, "{:.16e}"); SWEEP(d, "{:.30e}");
  SWEEP(d, "{:g}");    SWEEP(d, "{:.1g}");  SWEEP(d, "{:.17g}");
  SWEEP(d, "{:#g}");   SWEEP(d, "{:a}");    SWEEP(d, "{:.0a}");
  SWEEP(d, "{:.5a}");  SWEEP(d, "{:#.3a}"); SWEEP(d, "{:+15}");
  SWEEP(d, "{:#}");    SWEEP(d, "{:.5}");
  SWEEP(f, "{}");      SWEEP(f, "{:12}");   SWEEP(f, "{:e}");
  SWEEP(f, "{:.10f}"); SWEEP(f, "{:g}");    SWEEP(f, "{:a}");
  SWEEP(f, "{:.2a}");  SWEEP(f, "{:#}");
#undef SWEEP
}

#endif
