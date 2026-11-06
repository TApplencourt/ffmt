// A null const char*. g++ with -Werror rejects it on the specifiers path:
// printf's %s with a null argument is diagnosed at compile time.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME null_string
#define TEST_INC "isolated/test_null_string.cpp"
#include "../test_body.inc"
#else

// fmt::format reports a null const char* as an error, which a device can
// neither throw nor abort on. ffmt prints it as the string "(null)", spec
// included, as fmt::printf does (README), so {fmt} formatting "(null)" is
// the reference.
#ifdef FFMT_STD_PATH
#define NULL_STRING "(null)"
#else
#define NULL_STRING static_cast<const char*>(nullptr)
#endif
RUN(PRINT("{}\n", NULL_STRING));
RUN(PRINT("[{:>10}]\n", NULL_STRING));
RUN(PRINT("[{:.3}]\n", NULL_STRING));
#undef NULL_STRING

#endif
