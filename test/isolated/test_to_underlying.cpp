// Enums converted with C++23's std::to_underlying, so C++20 ids cannot
// compile it.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME to_underlying
#define TEST_INC "isolated/test_to_underlying.cpp"
#include <utility>
enum class status : int { warning = -3 };
enum color { red, green };
#include "../test_select_body.inc"
#else

RUN(PRINT("{}\n", std::to_underlying(status::warning)));
RUN(PRINT("{:x}\n", std::to_underlying(green)));

#endif
