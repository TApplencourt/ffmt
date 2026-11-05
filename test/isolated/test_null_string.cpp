// A null const char*. g++ with -Werror rejects it on the specifiers path:
// printf's %s with a null argument is diagnosed at compile time.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME null_string
#define TEST_INC "isolated/test_null_string.cpp"
#include "../test_select_body.inc"
#else

// A null const char* is undefined in std::format/{fmt}; on a GPU it is an illegal
// access. The buffer path prints glibc's "(null)" instead of faulting.
#ifdef FFMT_STD_PATH
RUN(printf("(null)\n"));
RUN(printf("[    (null)]\n"));
RUN(printf("[(nu]\n"));
#else
RUN(PRINT("{}\n", static_cast<const char*>(nullptr)));
RUN(PRINT("[{:>10}]\n", static_cast<const char*>(nullptr)));
RUN(PRINT("[{:.3}]\n", static_cast<const char*>(nullptr)));
#endif

#endif
