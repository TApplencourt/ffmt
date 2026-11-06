// Differential sweep against {fmt}: deterministic pseudo-random floats.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME float_sweep
#define TEST_INC "isolated/test_float_sweep.cpp"
#include "../test_body.inc"
#else
// ── Differential sweep against {fmt} (host only) ───────────────────────────
// Deterministic pseudo-random doubles/floats (raw bit patterns, so every
// exponent and subnormals are hit) plus short decimals that land on rounding
// ties. Far denser than the hand-picked cases above; this is what would have
// caught the >2^53 precision bugs.
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
  SWEEP(dv, "{}");      SWEEP(dv, "{:f}");    SWEEP(dv, "{:.0f}");
  SWEEP(dv, "{:.3f}");  SWEEP(dv, "{:.17f}"); SWEEP(dv, "{:e}");
  SWEEP(dv, "{:.0e}");  SWEEP(dv, "{:.16e}"); SWEEP(dv, "{:.30e}");
  SWEEP(dv, "{:g}");    SWEEP(dv, "{:.1g}");  SWEEP(dv, "{:.17g}");
  SWEEP(dv, "{:#g}");   SWEEP(dv, "{:a}");    SWEEP(dv, "{:.0a}");
  SWEEP(dv, "{:.5a}");  SWEEP(dv, "{:#.3a}"); SWEEP(dv, "{:+15}");
  SWEEP(dv, "{:#}");    SWEEP(dv, "{:.5}");
  SWEEP(fv, "{}");      SWEEP(fv, "{:12}");   SWEEP(fv, "{:e}");
  SWEEP(fv, "{:.10f}"); SWEEP(fv, "{:g}");    SWEEP(fv, "{:a}");
  SWEEP(fv, "{:.2a}");  SWEEP(fv, "{:#}");
#undef SWEEP
}

#endif
