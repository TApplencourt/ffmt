#ifndef TEST_INC
#define TEST_NAME characters
#define TEST_INC "test_characters.cpp"
#include "test_body.inc"
#else

// Char
RUN(PRINT("{}\n", 'A'));
RUN(PRINT("{:d}\n", true));
RUN(PRINT("{}\n", 'Z'));
RUN(PRINT("{:d}\n", 'Z'));
RUN(PRINT("{:c}\n", 65));

// Dynamic char
RUN({
  volatile char c = 'Q';
  PRINT("{}\n", static_cast<char>(c));
});

// Width on char
RUN(PRINT("{:<10c}\n", 'B'));
RUN(PRINT("{:>10c}\n", 'C'));
RUN(PRINT("{:10c}\n", 'A'));

#endif
