#ifndef TEST_INC
#define TEST_NAME println
#define TEST_INC "test_println.cpp"
#include "test_body.inc"
#else

// println
RUN(PRINTLN("hello println"));
RUN(PRINTLN("{} + {} = {}", 1, 2, 3));
RUN(PRINTLN("{:08x}", 255u));

#endif
