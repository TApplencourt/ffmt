// White-box test of buffer_path::escape_percent_inplace.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME escape_percent_inplace
#define TEST_INC "isolated/test_escape_percent_inplace.cpp"
#include "../test_body.inc"
#else
// ── escape_percent_inplace (host-only unit test) ───────────────────────────
// The ACPP device emit escapes % → %% in place. When the escaped text no
// longer fits, it must keep the longest fitting prefix (never half of a %%
// pair) and keep println's trailing '\n'. Before the fix the right-to-left
// copy overran unread source bytes and the output came out as "%%%%…".
{
  auto make_input = [](int n, bool nl) {
    std::string in;
    for (int i = 0; i < n; i++) in += (i % 10 == 9) ? '%' : char('a' + i % 10);
    if (nl) in += '\n';
    return in;
  };
  for (int n : {5, 100, 116, 117, 118, 128}) {
    for (bool nl : {false, true}) {
      std::string in = make_input(n, nl);
#ifdef FFMT_STD_PATH
      // Reference: escape char by char while the result fits in the cap.
      std::string body = nl ? in.substr(0, in.size() - 1) : in, want;
      for (char c : body) {
        std::string e = (c == '%') ? "%%" : std::string(1, c);
        if (want.size() + e.size() > FFMT_BUFFER_SIZE) break;
        want += e;
      }
      if (nl) want += '\n';
      RUN(printf("[%s]\n", want.c_str()));
#else
      ffmt::detail::fmt_buf b;
      for (char c : in) b.data[b.len++] = c;
      ffmt::detail::buffer_path::escape_percent_inplace(b);
      b.data[b.len] = '\0';
      RUN(printf("[%s]\n", b.data));
#endif
    }
  }
}

#endif
