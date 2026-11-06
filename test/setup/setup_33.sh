# Setup for id 33: macOS Apple clang (libc++), buffer path, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS, FFMT_MAIN, FFMT_SYNTAX_FLAGS
# and FFMT_FMT_FLAGS.

# Apple clang with libc++: catches what only shows up on macOS.
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=1"
export FFMT_MAIN=test/main_host.cpp
# A host example the buffer path can print (the specifiers path rejects its
# specs): built and run by the test goal, but not a test (Makefile.id).
export FFMT_EXTRA_FILES="examples/ccpreference_print.cpp"
# Only for the negative tests' first pass: NEG_SYNTAX_CHECK turns the
# harness's format call into a no-op, so a negative file that still fails to
# compile has a bug of its own and its rejection would mean nothing.
export FFMT_SYNTAX_FLAGS="-DNEG_SYNTAX_CHECK"
# Only for the negative tests' {fmt} cross-check: NEG_CHECK_FMT makes the
# harness call fmt::format instead of ffmt, and {fmt} must reject the case
# too, or the test is a restriction we invented that {fmt} would accept.
export FFMT_FMT_FLAGS="-std=c++20 -Wall -Werror -DNEG_CHECK_FMT"
