// Values for the differential sweeps against {fmt} (float_sweep and
// hexfloat_sweep): deterministic pseudo-random doubles/floats (raw bit
// patterns, so every exponent and subnormals are hit) plus short decimals that
// land on rounding ties. Far denser than the hand-picked cases of
// test_floats.cpp, which aim at specific code paths, specials and spec options;
// this is what would have caught the >2^53 precision bugs.
//
// Each RUN makes its values where it prints them, with the same integer code
// on the host and on the device: a vector built on the host is not visible
// to a device kernel (#44). Finite is tested on the bits, not with x - x == 0,
// which fast-math may fold to true. The including test #undefs the macros.
#define XORSHIFT(st) (st ^= st << 13, st ^= st >> 7, st ^= st << 17, st)
#define SWEEP_DOUBLES(fmt) RUN(                                                  \
  uint64_t st = 0x9E3779B97F4A7C15ULL;                                           \
  for (int n = 0; n < 400;) {                    /* random finite bit patterns */ \
    uint64_t b = XORSHIFT(st);                                                   \
    if ((b >> 52 & 0x7FF) != 0x7FF) { PRINTLN(fmt, __builtin_bit_cast(double, b)); n++; } \
  }                                                                              \
  const double p10[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7};                \
  for (int i = 0; i < 200; i++) {                /* k / 10^j and (k + 0.5) / 2^j */ \
    double k = static_cast<double>(XORSHIFT(st) % 100000);                       \
    PRINTLN(fmt, k / p10[XORSHIFT(st) % 8]);                                     \
    PRINTLN(fmt, (k + 0.5) / static_cast<double>(1u << (XORSHIFT(st) % 12)));    \
  })
#define SWEEP_FLOATS(fmt) RUN(                                                   \
  uint64_t st = 0x2545F4914F6CDD1DULL;                                           \
  for (int n = 0; n < 300;) {                                                    \
    uint32_t b = static_cast<uint32_t>(XORSHIFT(st));                            \
    if ((b >> 23 & 0xFF) != 0xFF) { PRINTLN(fmt, __builtin_bit_cast(float, b)); n++; } \
  })
