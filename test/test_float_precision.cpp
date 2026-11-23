#ifndef TEST_INC
#define TEST_NAME float_precision
#define TEST_INC "test_float_precision.cpp"
#include "test_body.inc"
#else

// Extended precision beyond the ordinary fixed/scientific/general cases.
RUN(PRINT("{:.30f}\n", 1.0 / 3.0));
RUN(PRINT("{:.20e}\n", 1e23));                          // 1e23 is not a double
RUN(PRINT("{:.25g}\n", 1e24));
RUN(PRINT("{:.40g}\n", 1.0 / 3.0));
RUN(PRINT("{:.150e}\n", 1.0 / 3.0));

#endif
