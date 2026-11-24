#ifndef TEST_INC
#define TEST_NAME buffer_cap
#define TEST_INC "test_buffer_cap.cpp"
#include "test_body.inc"
#else

// ── Writes near the buffer cap ────────────────────────────────────────────────
// Its own file: on the icpx and Intel LLVM ids (OpenMP and SYCL, PVC) every
// one of these crashes (exit 139), and in misc the first one stopped the rest.
#define FILL120 "........................................................................................................................"
RUN(PRINT("{}{:o}\n", FILL120, ~0ull));                       // write_uint_raw, base 8
RUN(PRINT("{}{}\n", FILL120, ~0ull));                         // write_arg_default
RUN(PRINT("{}{}\n", FILL120, reinterpret_cast<void*>(0x123456789abcdef0ULL)));
RUN(PRINT("{}{:p}\n", FILL120, reinterpret_cast<void*>(0x123456789abcdef0ULL)));
RUN(PRINT("{}{}{}\n", FILL120, FILL120, 12345));              // already full
// println keeps its '\n' even when the line is truncated; before the fix the
// next "Test N" header was glued onto the truncated line.
RUN(PRINTLN("{}{}", FILL120, FILL120));
#undef FILL120

#endif
