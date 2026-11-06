# Setup for id 35: host clang++ 18 (ubuntu-24.04), buffer path, coverage instrumentation, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS, FFMT_MAIN, FFMT_SYNTAX_FLAGS
# and FFMT_FMT_FLAGS.

# For the coverage gate: .github/scripts/coverage_union.py runs the
# binaries of both coverage ids and takes the union of their regions.
# ubuntu-24.04's default clang (18), as on the GitHub runners; another
# clang version is another id.
export CXX=clang++-18
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=1 -fprofile-instr-generate -fcoverage-mapping"
export FFMT_MAIN=test/main_host.cpp
# Only for the negative tests' first pass: NEG_SYNTAX_CHECK turns the
# harness's format call into a no-op, so a negative file that still fails to
# compile has a bug of its own and its rejection would mean nothing.
export FFMT_SYNTAX_FLAGS="-DNEG_SYNTAX_CHECK"
# Only for the negative tests' {fmt} cross-check: NEG_CHECK_FMT makes the
# harness call fmt::format instead of ffmt, and {fmt} must reject the case
# too, or the test is a restriction we invented that {fmt} would accept.
export FFMT_FMT_FLAGS="-std=c++20 -Wall -Werror -DNEG_CHECK_FMT"
