// Cases using dynamic width and precision ({:{}} and {:.{}}). The specifiers path cannot express these with printf.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME dynamic
#define TEST_INC "isolated/test_dynamic.cpp"
#include "../test_select_body.inc"
#else
// ── Strings ───────────────────────────────────────────────────────────────────
RUN(PRINT("┌{0:─^{2}}┐\n│{1: ^{2}}│\n", "", "Hello", 9));
RUN(PRINT("[{:>{}.{}}]\n", "静夜思", 8, 5));

// ── Misc ──────────────────────────────────────────────────────────────────────
RUN(PRINT("{:{}d}\n", 42, 10));
RUN(PRINT("{:{}d}\n", 42, 1));
RUN(PRINT("{:.{}f}\n", 3.14159, 2));
RUN(PRINT("{:.{}f}\n", 4.14159, 0));
RUN(PRINT("{:{}.{}f}\n", 5.14, 15, 6));
RUN(PRINT("{0:{1}x}\n", 255, 10));
RUN(PRINT("{0:{2}.{1}f}\n", 6.14, 4, 20));
RUN(PRINT("{:{}d}\n", 42, 0));
RUN(PRINT("{:.{}f}\n", 3.14159, 10));
RUN(PRINT("{:{}c}\n", 'A', 5));
RUN(PRINT("{:{}}\n", true, 10));
RUN(PRINT("{:{}.{}f}\n", 3.14, 20, 0));

// ── Type-instantiation coverage ───────────────────────────────────────────────
// resolve_int_arg dispatch for type combinations not hit elsewhere:
// dynamic width/precision with mixed integer types and floats.
RUN(PRINT("{:{}d}\n", 42, static_cast<short>(8)));      // resolve_int_arg<short>
RUN(PRINT("{:{}d}\n", 42, static_cast<int64_t>(8)));    // resolve_int_arg<int64_t>
RUN(PRINT("{:{}d}\n", 42, static_cast<uint64_t>(8)));   // resolve_int_arg<uint64_t>
RUN(PRINT("{:{}d}\n", 42, 8L));                         // resolve_int_arg<long>
RUN(PRINT("{:{}d}\n", 42, static_cast<unsigned>(8)));   // resolve_int_arg<unsigned>
RUN(PRINT("{:{}d}\n", 42, static_cast<int8_t>(8)));     // resolve_int_arg<int8_t>
RUN(PRINT("{:{}d}\n", 42, static_cast<uint8_t>(8)));    // resolve_int_arg<uint8_t>

// ── Float formatting fixes ────────────────────────────────────────────────────
// Huge dynamic precision: used to write ~1000 bytes past the buffer.
RUN(PRINT("{:.{}f}\n", 1.5, 1000));
RUN(PRINT("{:.{}e}\n", 1.0 / 3.0, 5000));
RUN(PRINT("{:.{}g}\n", 0.1, 400));

#endif
