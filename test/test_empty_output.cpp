#ifndef TEST_INC
#define TEST_NAME empty_output
#define TEST_INC "test_empty_output.cpp"
#include "test_body.inc"
#else

// A print with no arguments and no output bytes.
RUN(PRINT(""));

#endif
