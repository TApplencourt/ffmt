#ifndef TEST_INC
#define TEST_NAME float_zero
#define TEST_INC "test_float_zero.cpp"
#include "test_body.inc"
#else

// Positive and negative zero through the explicit-type formatting paths.
RUN(PRINT("{:f}\n", 0.0));
RUN(PRINT("{:f}\n", -0.0));
RUN(PRINT("{:f}\n", -0.0f));
RUN(PRINT("{:e}\n", -0.0f));
RUN(PRINT("{:g}\n", -0.0f));
RUN(PRINT("{:g}\n", -0.0));
RUN(PRINT("{:e}\n", -0.0));
RUN(PRINT("{:g}\n", 0.0));
RUN(PRINT("{:g}\n", -0.0));
RUN(PRINT("{:e}\n", 0.0f));
RUN(PRINT("{:f}\n", 0.0f));
RUN(PRINT("{:g}\n", 0.0f));

#endif
