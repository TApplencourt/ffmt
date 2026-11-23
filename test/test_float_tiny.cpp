#ifndef TEST_INC
#define TEST_NAME float_tiny
#define TEST_INC "test_float_tiny.cpp"
#include "test_body.inc"
#else

// Tiny, minimum-normal, and subnormal values with explicit formatting.
RUN(PRINT("{:g}\n", 1.0e-300));
RUN(PRINT("{:e}\n", 2.2250738585072014e-308));
RUN(PRINT("{:.16e}\n", 2.2250738585072014e-308));       // DBL_MIN
RUN(PRINT("{:e}\n", 4.9406564584124654e-324));         // smallest subnormal
RUN(PRINT("{:e}\n", 5e-324));
RUN(PRINT("{:g}\n", 5e-324));

#endif
