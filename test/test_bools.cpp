#ifndef TEST_INC
#define TEST_NAME bools
#define TEST_INC "test_bools.cpp"
#include "test_body.inc"
#else

// Bool as a string: %s of "true"/"false" on the specifiers path.
RUN(PRINT("{}\n", false));
RUN(PRINT("{}\n", true));
RUN(PRINT("{:>10}\n", true));
RUN(PRINT("{:<10}\n", false));
RUN(PRINT("{:s}\n", true));
RUN(PRINT("{:s}\n", false));

#endif
