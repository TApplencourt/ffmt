// Differential hexadecimal sweep against {fmt}: the values of float_sweep
// through {:a}. A file of its own, so that an id that cannot compile it (the
// specifiers path has no {:a}) loses only these tests (test/configs.json
// lists them under failed_to_compile).
#ifndef TEST_INC
#define TEST_NAME hexfloat_sweep
#define TEST_INC "test_hexfloat_sweep.cpp"
#include "test_body.inc"
#else
#include "sweep_values.inc"
SWEEP_DOUBLES("{:a}");    SWEEP_DOUBLES("{:.0a}");
SWEEP_DOUBLES("{:.5a}");  SWEEP_DOUBLES("{:#.3a}");
SWEEP_FLOATS("{:a}");     SWEEP_FLOATS("{:.2a}");
#undef SWEEP_FLOATS
#undef SWEEP_DOUBLES
#undef XORSHIFT
#endif
