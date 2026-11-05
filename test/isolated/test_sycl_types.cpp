// SYCL types (sycl::range, sycl::id) through their formatters.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME sycl_types
#define TEST_INC "isolated/test_sycl_types.cpp"
#include "../capture.hpp"
#include "../sycl_std_formatters.hpp"
#include "../test_select_body.inc"
#else
RUN(PRINTLN("range = {}", sycl::range<1>{4}));
RUN(PRINTLN("range = {}", sycl::range<2>{4, 8}));
RUN(PRINTLN("range = {}", sycl::range<3>{4, 8, 16}));

RUN(PRINTLN("id = {}", sycl::id<1>{2}));
RUN(PRINTLN("id = {}", sycl::id<2>{2, 5}));
RUN(PRINTLN("id = {}", sycl::id<3>{1, 2, 3}));

RUN(PRINTLN("step {} of {}: range={}", 3, 100, sycl::range<3>{4, 8, 16}));

RUN(PRINT("[{}]\n", sycl::id<2>{0, 0}));

#endif
