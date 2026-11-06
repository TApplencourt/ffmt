// A string in SYCL USM shared memory (sycl::malloc_shared), printed from
// the device.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME sycl_usm_string
#define TEST_INC "isolated/test_sycl_usm_string.cpp"
#include <cstring>
#include "../test_body.inc"
#else
{
  const char *env = "cpu-char-* copied";
#ifdef FFMT_STD_PATH
  RUN(PRINT("{}\n", env));
#else
  size_t len = std::strlen(env) + 1;
  char *shared = ::sycl::malloc_shared<char>(len, q);
  std::memcpy(shared, env, len);
  RUN(PRINT("{}\n", shared));
  ::sycl::free(shared, q);
#endif
}

#endif
