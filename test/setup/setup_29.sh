# Setup for id 29: host clang++ 18 (ubuntu-24.04), buffer path, ASan+UBSan, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS and FFMT_MAIN.

# ubuntu-24.04's default clang (18), as on the GitHub runners; another
# clang version is another id.
export CXX=clang++-18
# -fno-sanitize=bool: libstdc++'s std::basic_format_arg keeps a `bool
# _M_bool` in a union initialised through `_M_none()`, so `_S_get<bool>`
# loads a 0 byte that UBSan reports as an invalid bool. The load is in
# <format>, not in ffmt, so only that one check is turned off.
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
export ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1:detect_stack_use_after_return=1
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O0 -fno-fast-math -DFFMT_BUFFER_PATH=1 -fsanitize=address,undefined -fno-sanitize=bool"
export FFMT_MAIN=test/main_host.cpp
# A host example the buffer path can print (the specifiers path rejects its
# specs): built and run by the test goal, but not a test (Makefile.id).
export FFMT_EXTRA_FILES="examples/ccpreference_print.cpp"
