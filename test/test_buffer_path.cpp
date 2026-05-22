#ifndef TEST_INC
#define TEST_NAME buffer_path
#define TEST_INC "test_buffer_path.cpp"
#include "test_select_body.inc"
#else

// ── Integers (from test_integers.cpp) ──
RUN(PRINT("{:b}\n", 255));
RUN(PRINT("{:B}\n", 255));
RUN(PRINT("{:#x}\n", 255));
RUN(PRINT("{:#b}\n", 255));
RUN(PRINT("{:x}\n", -2147483647 - 1));
RUN(PRINT("{:b}\n", -2147483647 - 1));
RUN(PRINT("{:b}\n", static_cast<uint64_t>(18446744073709551615ULL)));
RUN(PRINT("{:#b}\n", 0));
RUN(PRINT("{:x}\n", static_cast<int>(-1)));
RUN(PRINT("{:#x}\n", 0));

// INT64_MIN with non-decimal bases
RUN(PRINT("{:x}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:X}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:o}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:b}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:#x}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));
RUN(PRINT("{:#b}\n", static_cast<int64_t>(-9223372036854775807LL - 1)));

// INT_MIN (32-bit) with non-decimal bases
RUN(PRINT("{:o}\n", -2147483647 - 1));
RUN(PRINT("{:X}\n", -2147483647 - 1));
RUN(PRINT("{:#o}\n", -2147483647 - 1));
RUN(PRINT("{:#b}\n", -2147483647 - 1));

// Sign + alternate + zero-pad combinations
RUN(PRINT("{:+#010x}\n", 255));
RUN(PRINT("{:+#010x}\n", -1));
RUN(PRINT("{: #010x}\n", 255));
RUN(PRINT("{: #010x}\n", -1));
RUN(PRINT("{:+#010b}\n", 42));
RUN(PRINT("{:+#010b}\n", -1));
RUN(PRINT("{: #010b}\n", 42));
RUN(PRINT("{:+#010o}\n", 255));
RUN(PRINT("{: #010o}\n", 255));
RUN(PRINT("{:+#010X}\n", 255));
RUN(PRINT("{:+#020b}\n", -42));

// ── Floats (from test_floats.cpp) ──
RUN(PRINT("{}\n", 1.0e10f));
RUN(PRINT("{}\n", 1.0e-10f));
RUN(PRINT("{}\n", 1.0e100));
RUN(PRINT("{}\n", 1.0e-100));

// Float extremes — default format
RUN(PRINT("{}\n", std::numeric_limits<float>::max()));
RUN(PRINT("{}\n", -std::numeric_limits<float>::max()));
RUN(PRINT("{}\n", std::numeric_limits<float>::min()));
RUN(PRINT("{}\n", std::numeric_limits<float>::denorm_min()));
RUN(PRINT("{}\n", -0.0f));
RUN(PRINT("{}\n", std::numeric_limits<double>::max()));
RUN(PRINT("{}\n", std::numeric_limits<double>::min()));
RUN(PRINT("{}\n", -0.0));
RUN(PRINT("{}\n", std::numeric_limits<float>::infinity()));
RUN(PRINT("{}\n", -std::numeric_limits<float>::infinity()));
RUN(PRINT("{}\n", std::numeric_limits<float>::quiet_NaN()));

RUN(PRINT("{:a}\n", 3.14159));
RUN(PRINT("{:A}\n", 3.14159));
RUN(PRINT("{:a}\n", 0.0));
RUN(PRINT("{:a}\n", -0.0));
RUN(PRINT("{:a}\n", 1.0));
RUN(PRINT("{:#a}\n", 3.14159));
RUN(PRINT("{:e}\n", 5e-324));
RUN(PRINT("{:g}\n", 5e-324));

// ── Strings (from test_strings.cpp) ──
RUN(PRINT("{:*^10}\n", true));
RUN(PRINTLN("{:08x}", 255));
RUN(PRINT("{:10c}\n", 'A'));
RUN(PRINT("{:^10c}\n", 'D'));
RUN(PRINT("{:*>5c}\n", 'X'));
RUN(PRINT("{:*<5c}\n", 'X'));
RUN(PRINT("{:*^5c}\n", 'X'));
#ifndef FMT_SYCL_WA_STR
RUN(PRINT("{:20s}\n", "hello"));
RUN(PRINT("{:*^10.5s}\n", "hello world"));
RUN(PRINT("{:*<20s}\n", "hello"));
RUN(PRINT("{:*>20s}\n", "hello"));
RUN(PRINT("{:*^20s}\n", "hello"));
#endif

// ── Layout (from test_layout.cpp) ──
RUN(PRINT("{:^10d}\n", 42));
RUN(PRINT("{:*<10d}\n", 42));
RUN(PRINT("{:*>10d}\n", 42));
RUN(PRINT("{:*^10d}\n", 42));
RUN(PRINT("{:#^20b}\n", 255));
RUN(PRINT("{:+010x}\n", -1));
RUN(PRINT("{:>20b}\n", static_cast<uint8_t>(255)));
RUN(PRINT("{:*>5d}\n", -42));
RUN(PRINT("{:<10x}\n", 255u));
RUN(PRINT("{:>10x}\n", 255u));
RUN(PRINT("{:^10x}\n", 255u));
RUN(PRINT("{:<10o}\n", 255u));
RUN(PRINT("{:>10o}\n", 255u));
RUN(PRINT("{:*<10x}\n", 255u));
RUN(PRINT("{:*>10x}\n", 255u));
RUN(PRINT("{:*^10x}\n", 255u));
RUN(PRINT("{:*<10o}\n", 255u));
RUN(PRINT("{:1x}\n", 0xDEAD));
RUN(PRINT("{:1b}\n", 255));
RUN(PRINT("{:*<10}\n", true));
RUN(PRINT("{:*>10}\n", false));
RUN(PRINT("{:*^10}\n", true));

// ── Misc (from test_misc.cpp) ──
RUN(PRINT("{:#010x}\n", 255));
RUN(PRINT("{:*>15.6f}\n", 3.14));
RUN(PRINT("hex={:#x} pi={:.4f}\n", 255, 3.14159));
RUN(PRINT("{:a}\n", 1.0 / 0.0));
RUN(PRINT("{:A}\n", 0.0 / 0.0));
RUN(PRINT("{:a}\n", 0.125));
RUN(PRINT("{0:x} {0:d} {0:b}\n", 255));
RUN(PRINT("{:{}d}\n", 42, 10));
RUN(PRINT("{:{}d}\n", 42, 1));
RUN(PRINT("{:.{}f}\n", 3.14159, 2));
RUN(PRINT("{:.{}f}\n", 4.14159, 0));
RUN(PRINT("{:{}.{}f}\n", 5.14, 15, 6));
RUN(PRINT("{0:{1}x}\n", 255, 10));
RUN(PRINT("{0:{2}.{1}f}\n", 6.14, 4, 20));
RUN(PRINT("{:+x}\n", 255));
RUN(PRINT("{:+x}\n", -1));
RUN(PRINT("{: x}\n", 255));
RUN(PRINT("{: x}\n", -1));
RUN(PRINT("{:+o}\n", 255));
RUN(PRINT("{: o}\n", 255));
RUN(PRINT("{:+b}\n", 42));
RUN(PRINT("{: b}\n", 42));
RUN(PRINT("{:+b}\n", -1));
RUN(PRINT("{:+a}\n", 3.14));
RUN(PRINT("{:+a}\n", -3.14));
RUN(PRINT("{: a}\n", 3.14));
RUN(PRINT("{: a}\n", -3.14));
RUN(PRINT("{:+A}\n", 1.0));
RUN(PRINT("{:b}\n", 0u));
RUN(PRINT("{:B}\n", 0u));
RUN(PRINT("{:a}\n", 0.0));
RUN(PRINT("{:{}d}\n", 42, 0));
RUN(PRINT("{:.{}f}\n", 3.14159, 10));
RUN(PRINT("{:{}c}\n", 'A', 5));
RUN(PRINT("{:{}}\n", true, 10));
RUN(PRINT("{:{}.{}f}\n", 3.14, 20, 0));
RUN(PRINT("{:#010x}\n", 0));
RUN(PRINT("{:#o}\n", 0));
RUN(PRINT("{:#b}\n", 1));
RUN(PRINT("{:#010b}\n", 1));
RUN(PRINT("{:#X}\n", 255));
RUN(PRINT("{:#o}\n", 8));
RUN(PRINT("{:<20a}\n", 3.14));
RUN(PRINT("{:>20a}\n", 3.14));
RUN(PRINT("{:^20a}\n", 3.14));
RUN(PRINT("{:*>20a}\n", 3.14));

// ── Type-instantiation coverage ──
// write_int_rt<int8_t/short/int64_t/uint64_t/long>: integer specs through
// less common types so the per-type buffer_path::write_int_rt instantiations
// actually get exercised by coverage.
RUN(PRINT("{:b}\n", static_cast<int8_t>(-5)));
RUN(PRINT("{:x}\n", static_cast<int8_t>(-5)));
RUN(PRINT("{:b}\n", static_cast<short>(-300)));
RUN(PRINT("{:x}\n", static_cast<short>(-300)));
RUN(PRINT("{:b}\n", static_cast<int64_t>(-1)));
RUN(PRINT("{:x}\n", static_cast<int64_t>(-1)));
RUN(PRINT("{:b}\n", static_cast<uint64_t>(0xDEADBEEFCAFEULL)));
RUN(PRINT("{:o}\n", static_cast<uint64_t>(0xDEADBEEFCAFEULL)));
RUN(PRINT("{:b}\n", 1L));
RUN(PRINT("{:#x}\n", -1L));

// hex_float_to_buf_rt<float>: {:a} on float (tests above only use double).
RUN(PRINT("{:a}\n", 3.14f));
RUN(PRINT("{:A}\n", 3.14f));
RUN(PRINT("{:a}\n", 0.0f));
RUN(PRINT("{:a}\n", -0.0f));
RUN(PRINT("{:a}\n", std::numeric_limits<float>::infinity()));
RUN(PRINT("{:#a}\n", 1.5f));

// resolve_int_arg dispatch for type combinations not hit elsewhere:
// dynamic width/precision with mixed integer types and floats.
RUN(PRINT("{:{}d}\n", 42, static_cast<short>(8)));      // resolve_int_arg<short>
RUN(PRINT("{:{}d}\n", 42, static_cast<int64_t>(8)));    // resolve_int_arg<int64_t>
RUN(PRINT("{:{}d}\n", 42, static_cast<uint64_t>(8)));   // resolve_int_arg<uint64_t>
RUN(PRINT("{:{}d}\n", 42, 8L));                         // resolve_int_arg<long>
RUN(PRINT("{:{}d}\n", 42, static_cast<unsigned>(8)));   // resolve_int_arg<unsigned>
RUN(PRINT("{:{}d}\n", 42, static_cast<int8_t>(8)));     // resolve_int_arg<int8_t>
RUN(PRINT("{:{}d}\n", 42, static_cast<uint8_t>(8)));    // resolve_int_arg<uint8_t>

#endif
