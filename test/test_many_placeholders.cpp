#ifndef TEST_INC
#define TEST_NAME many_placeholders
#define TEST_INC "test_many_placeholders.cpp"
#include "test_body.inc"
#else

// More than 16 placeholders (the buffer path used to cap them at 16).
// Its own file: forwarding 20 arguments crashes upstream clang 23.1.6's
// AMDGPU backend at -O0 ("Cannot scavenge register in FI elimination!"),
// https://github.com/llvm/llvm-project/pull/215182, fixed on main, not in
// 23.x; in misc it kept all of misc from compiling.
RUN(PRINTLN("{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9));

#endif
