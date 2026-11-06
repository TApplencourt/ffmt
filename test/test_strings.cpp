#ifndef TEST_INC
#define TEST_NAME strings
#define TEST_INC "test_strings.cpp"
#include "test_body.inc"
#else

// Char
RUN(PRINT("{}\n", 'A'));
RUN(PRINT("{:d}\n", true));
RUN(PRINT("{}\n", 'Z'));
RUN(PRINT("{:d}\n", 'Z'));
RUN(PRINT("{:c}\n", 65));

// println
RUN(PRINTLN("hello println"));
RUN(PRINTLN("{} + {} = {}", 1, 2, 3));
RUN(PRINTLN("{:08x}", 255u));

// Dynamic char
RUN({
  volatile char c = 'Q';
  PRINT("{}\n", static_cast<char>(c));
});

// Width on char
RUN(PRINT("{:<10c}\n", 'B'));
RUN(PRINT("{:>10c}\n", 'C'));
RUN(PRINT("{:10c}\n", 'A')); // left-aligned by default

#endif
