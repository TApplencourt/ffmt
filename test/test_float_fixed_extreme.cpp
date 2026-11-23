#ifndef TEST_INC
#define TEST_NAME float_fixed_extreme
#define TEST_INC "test_float_fixed_extreme.cpp"
#include "test_body.inc"
#else

// Fixed formatting whose exact output has a very large integer part.
RUN(PRINT("{:f}\n", 1e22));
RUN(PRINT("{:f}\n", 1.7976931348623157e308));          // 309 integer digits
RUN(PRINT("{:f}\n", 3.4028234663852886e38f));          // FLT_MAX

#endif
