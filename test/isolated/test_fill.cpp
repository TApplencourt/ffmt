// Cases using custom fill characters and centre alignment. The specifiers path cannot express these with printf.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME fill
#define TEST_INC "isolated/test_fill.cpp"
#include "../test_select_body.inc"
#else
// ── Strings ───────────────────────────────────────────────────────────────────
RUN(PRINT("{:*^10}\n", true));
RUN(PRINT("{:^10c}\n", 'D'));
RUN(PRINT("{:*>5c}\n", 'X'));
RUN(PRINT("{:*<5c}\n", 'X'));
RUN(PRINT("{:*^5c}\n", 'X'));
RUN(PRINT("{:*^10.5s}\n", "hello world"));
RUN(PRINT("{:*<20s}\n", "hello"));
RUN(PRINT("{:*>20s}\n", "hello"));
RUN(PRINT("{:*^20s}\n", "hello"));
// Multi-byte UTF-8 fill (2, 3 and 4 bytes)
RUN(PRINT("[{:─<6}]\n", 42));
RUN(PRINT("[{:─>+8}]\n", 42));
RUN(PRINT("[{:─^9.2f}]\n", 3.14159));
RUN(PRINT("[{:😀>4c}]\n", 'x'));
RUN(PRINT("[{:é^9}]\n", "abc"));
// Width and string precision count display columns, like {fmt}: 1 per code
// point, 2 for East Asian wide and emoji, 1 per invalid byte.
RUN(PRINT("[{:^10}]\n", "hôpital"));
RUN(PRINT("[{:*>8}]\n", "静夜思"));
RUN(PRINT("[{:*<8.3}]\n", "hôpital"));
RUN(PRINT("[{:─^7}]\n", "😀"));

// ── Layout ────────────────────────────────────────────────────────────────────
RUN(PRINT("{:^10d}\n", 42));
RUN(PRINT("{:*<10d}\n", 42));
RUN(PRINT("{:*>10d}\n", 42));
RUN(PRINT("{:*^10d}\n", 42));
RUN(PRINT("{:*>5d}\n", -42));
RUN(PRINT("{:^10x}\n", 255u));
RUN(PRINT("{:*<10x}\n", 255u));
RUN(PRINT("{:*>10x}\n", 255u));
RUN(PRINT("{:*^10x}\n", 255u));
RUN(PRINT("{:*<10o}\n", 255u));
RUN(PRINT("{:*<10}\n", true));
RUN(PRINT("{:*>10}\n", false));
RUN(PRINT("{:*^10}\n", true));

// ── Misc ──────────────────────────────────────────────────────────────────────
RUN(PRINT("{:*>15.6f}\n", 3.14));
// Centre alignment on pointers — '^' is buffer-path-only generally.
RUN(PRINT("[{:^20p}]\n",  reinterpret_cast<void*>(0xdeadbeef)));

// ── Width overflow ──────────────────────────────────────────────────────────
RUN(PRINT("{:^200s}\n", "x"));      // center align with truncation

#endif
