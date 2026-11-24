#ifndef TEST_INC
#define TEST_NAME explicit_float_zero
#define TEST_INC "test_explicit_float_zero.cpp"
#include "test_body.inc"
#else

// Positive zero with explicit float type specifiers.
RUN(PRINT("{:g}\n", 0.0));
RUN(PRINT("{:f}\n", 0.0));
RUN(PRINT("{:e}\n", 0.0));
RUN(PRINT("{:g}\n", 0.0));

#endif
