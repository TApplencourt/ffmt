#ifndef TEST_INC
#define TEST_NAME misc
#define TEST_INC "test_misc.cpp"
#include "test_select_body.inc"
#else

// Combined specs
RUN(PRINT("{:+010d}\n", 42));
RUN(PRINT("{:+15.6f}\n", 3.14));

// Multiple arguments
RUN({
  int a = 1; int b = 2;
  PRINT("{} + {} = {}\n", a, b, a + b);
});
RUN(PRINT("{} {} {} {} {} {} {}\n", 1, 2, 3, 4, 5, 6, 7));

// Plain text and escaped braces
RUN(PRINT("Hello from GPU!\n"));
RUN(PRINT("{{}} is literal braces\n"));
RUN(PRINT("value={{{}}} done\n", 42));


// Edge cases
RUN(PRINT("{}\n", 0));
RUN(PRINT("{:g}\n", 0.0));
RUN(PRINT("{}\n", 2147483647));
RUN(PRINT("{}\n", -2147483647 - 1));

// Positional arguments
RUN(PRINT("{0} {1} {0}\n", 42, 99));
RUN(PRINT("{0:>10} {1:<10}\n", 42, 99));
RUN(PRINT("{2} {0} {1}\n", 'a', 'b', 'c'));
RUN(PRINT("{0} {0} {0}\n", 7));
RUN(PRINT("{0:.2f} {1:+d}\n", 3.14, -42));

// Zero with every type specifier
RUN(PRINT("{:d}\n", 0u));
RUN(PRINT("{:x}\n", 0u));
RUN(PRINT("{:X}\n", 0u));
RUN(PRINT("{:o}\n", 0u));
RUN(PRINT("{:f}\n", 0.0));
RUN(PRINT("{:e}\n", 0.0));
RUN(PRINT("{:g}\n", 0.0));

// Pointer formatting — fixed-value casts so host and device produce
// identical addresses (no real &x: device address would diverge from host
// std::format reference). The null-pointer cases further down fail on the
// specifiers path, whose libc printf emits "(nil)" instead of "0x0".
RUN(PRINT("{}\n",         reinterpret_cast<void*>(0xdeadbeef)));
RUN(PRINT("{:p}\n",       reinterpret_cast<void*>(0xcafef00d)));
RUN(PRINT("[{:>20p}]\n",  reinterpret_cast<void*>(0xdeadbeef)));
RUN(PRINT("[{:<20p}]\n",  reinterpret_cast<void*>(0xdeadbeef)));
// {:p} on a char pointer prints the address, as {fmt} does (std::format
// rejects it; this used to be a negative test).
RUN(PRINT("{:p}\n",       reinterpret_cast<const char*>(0xdeadbeef)));
RUN(PRINT("[{:>20p}]\n",  reinterpret_cast<const char*>(0xcafef00d)));
// Pointers are right-aligned by default (the buffer path left-aligned them).
RUN(PRINT("[{:20}]\n",    reinterpret_cast<void*>(0xdeadbeef)));
RUN(PRINT("[{:20p}]\n",   reinterpret_cast<void*>(0xdeadbeef)));
RUN(PRINT("[{:20p}]\n",   reinterpret_cast<const char*>(0xdeadbeef)));

// Wide widths — on ACPP these exercise the truncation branches in pad_in_place
// (the host harness truncates the std reference to FFMT_BUFFER_SIZE
// to match). On DPC++ the output is full-length and matches the std reference.
RUN(PRINT("{:200d}\n", 7));         // plain lpad overflow
RUN(PRINT("{:+200d}\n", 7));        // sign+overflow: hits sign-bound check
RUN(PRINT("{:0200d}\n", 7));        // zero-pad+overflow: hits zfill-bound check
RUN(PRINT("{:150.2f}\n", 3.14));

// No arguments: the literal goes through the printf format string on the
// specifiers path the literal is printed through "%s" ('%' kept as is: it
// used to come out doubled, "100%%"). One print per RUN: on a device the N
// work-items interleave separate prints.
RUN(PRINT("no args\n"));
RUN(PRINT("100% {{braces}} %d %s %%\n"));
RUN(PRINT(""));
RUN(PRINTLN(""));
RUN(PRINTLN("}}{{"));

// ── Misc ──────────────────────────────────────────────────────────────────────
// Null-pointer formatting — buffer-path-only because the specifiers path
// routes through libc printf which emits "(nil)" for null pointers, while
// std::format (and our buffer-path implementation) emit "0x0".
RUN(PRINT("{}\n",         static_cast<void*>(nullptr)));
RUN(PRINT("{:p}\n",       static_cast<void*>(nullptr)));

// ── Type-instantiation coverage ───────────────────────────────────────────────
RUN(PRINT("{:o}\n", static_cast<uint64_t>(0xDEADBEEFCAFEULL)));

// ── Writes near the buffer cap ────────────────────────────────────────────────
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
