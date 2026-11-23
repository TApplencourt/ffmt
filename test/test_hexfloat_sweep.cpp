// Differential hexadecimal sweep against {fmt}: deterministic pseudo-random floats.
// A file of its own, so that an id that cannot compile it loses only these
// tests (test/configs.json lists them under failed_to_compile).
#ifndef TEST_INC
#define TEST_NAME hexfloat_sweep
#define TEST_INC "test_hexfloat_sweep.cpp"
#include "test_body.inc"
#else
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
