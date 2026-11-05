# Setup for id 31: host clang++, buffer path, ASan+UBSan, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS, FFMT_MAIN, FFMT_SYNTAX_FLAGS
# and FFMT_FMT_FLAGS.

export CXX=clang++
# -fno-sanitize=bool: libstdc++'s std::basic_format_arg keeps a `bool
# _M_bool` in a union initialised through `_M_none()`, so `_S_get<bool>`
# loads a 0 byte that UBSan reports as an invalid bool. The load is in
# <format>, not in ffmt, so only that one check is turned off.
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
export ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1:detect_stack_use_after_return=1
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=1 -fsanitize=address,undefined -fno-sanitize=bool"
export FFMT_MAIN=test/test_main_host.cpp
# Only for the negative tests' first pass: NEG_SYNTAX_CHECK turns the
# harness's format call into a no-op, so a negative file that still fails to
# compile has a bug of its own and its rejection would mean nothing.
export FFMT_SYNTAX_FLAGS="-DNEG_SYNTAX_CHECK"
# Only for the negative tests' {fmt} cross-check: NEG_CHECK_FMT makes the
# harness call fmt::format instead of ffmt, and {fmt} must reject the case
# too, or the test is a restriction we invented that {fmt} would accept.
export FFMT_FMT_FLAGS="-std=c++20 -Wall -Werror -DNEG_CHECK_FMT"
