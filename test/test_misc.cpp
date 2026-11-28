#ifndef TEST_INC
#define TEST_NAME misc
#define TEST_INC "test_misc.cpp"
#include "test_body.inc"
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
RUN(PRINT("{}\n", 2147483647));
RUN(PRINT("{}\n", -2147483647 - 1));

// Positional arguments
RUN(PRINT("{0} {1} {0}\n", 42, 99));
RUN(PRINT("{0:>10} {1:<10}\n", 42, 99));
RUN(PRINT("{2} {0} {1}\n", 'a', 'b', 'c'));
RUN(PRINT("{0} {0} {0}\n", 7));
RUN(PRINT("{0:.2f} {1:+d}\n", 3.14, -42));

// Zero with integer type specifiers
RUN(PRINT("{:d}\n", 0u));
RUN(PRINT("{:x}\n", 0u));
RUN(PRINT("{:X}\n", 0u));
RUN(PRINT("{:o}\n", 0u));

// Pointer formatting — fixed-value casts so host and device produce
// identical addresses (no real &x: device address would diverge from the
// host {fmt} reference).
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

// Wide widths — on the buffer path these exercise the truncation branches in
// pad_in_place (test_body.inc truncates the {fmt} reference to
// FFMT_BUFFER_SIZE to match). On the specifiers path the output is
// full-length, like the reference.
RUN(PRINT("{:200d}\n", 7));         // plain lpad overflow
RUN(PRINT("{:+200d}\n", 7));        // sign+overflow: hits sign-bound check
RUN(PRINT("{:0200d}\n", 7));        // zero-pad+overflow: hits zfill-bound check
RUN(PRINT("{:150.2f}\n", 3.14));

// No arguments: on the specifiers path the literal is printed through "%s"
// ('%' kept as is: it used to come out doubled, "100%%").
RUN(PRINT("no args\n"));
RUN(PRINT("100% {{braces}} %d %s %%\n"));
RUN(PRINTLN(""));
RUN(PRINTLN("}}{{"));

// ── Misc ──────────────────────────────────────────────────────────────────────
// Null-pointer formatting — {fmt} prints "0x0" on both paths.
RUN(PRINT("{}\n",         static_cast<void*>(nullptr)));
RUN(PRINT("{:p}\n",       static_cast<void*>(nullptr)));

// ── Type-instantiation coverage ───────────────────────────────────────────────
RUN(PRINT("{:o}\n", static_cast<uint64_t>(0xDEADBEEFCAFEULL)));

#endif
