// Differential sweep against {fmt}: deterministic pseudo-random floats, all
// but hexadecimal (test_hexfloat_sweep.cpp). A file of its own, so that an id
// that cannot compile it loses only these tests (test/configs.json lists them
// under failed_to_compile).
#ifndef TEST_INC
#define TEST_NAME float_sweep
#define TEST_INC "test_float_sweep.cpp"
#include "test_body.inc"
#else
#include "sweep_values.inc"
SWEEP_DOUBLES("{}");      SWEEP_DOUBLES("{:f}");    SWEEP_DOUBLES("{:.0f}");
SWEEP_DOUBLES("{:.3f}");  SWEEP_DOUBLES("{:.17f}"); SWEEP_DOUBLES("{:e}");
SWEEP_DOUBLES("{:.0e}");  SWEEP_DOUBLES("{:.16e}"); SWEEP_DOUBLES("{:.30e}");
SWEEP_DOUBLES("{:g}");    SWEEP_DOUBLES("{:.1g}");  SWEEP_DOUBLES("{:.17g}");
SWEEP_DOUBLES("{:#g}");   SWEEP_DOUBLES("{:+15}");  SWEEP_DOUBLES("{:#}");
SWEEP_DOUBLES("{:.5}");
SWEEP_FLOATS("{}");       SWEEP_FLOATS("{:12}");    SWEEP_FLOATS("{:e}");
SWEEP_FLOATS("{:.10f}");  SWEEP_FLOATS("{:g}");     SWEEP_FLOATS("{:#}");
#undef SWEEP_FLOATS
#undef SWEEP_DOUBLES
#undef XORSHIFT
#endif
