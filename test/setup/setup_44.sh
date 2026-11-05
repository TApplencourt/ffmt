# Setup for id 44: icpx SYCL (DPC++) on Aurora PVC, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS, FFMT_MAIN, FFMT_SYNTAX_FLAGS
# and FFMT_FMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module load oneapi/release/2026.1.0
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export ZE_AFFINITY_MASK=0.0   # one tile: never share a GPU between runs
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fsycl -O2 -fno-fast-math"
export FFMT_MAIN=test/test_main_sycl.cpp
# Only for the negative tests' first pass: NEG_SYNTAX_CHECK turns the
# harness's format call into a no-op, so a negative file that still fails to
# compile has a bug of its own and its rejection would mean nothing.
export FFMT_SYNTAX_FLAGS="-DNEG_SYNTAX_CHECK"
# Only for the negative tests' {fmt} cross-check: NEG_CHECK_FMT makes the
# harness call fmt::format instead of ffmt, and {fmt} must reject the case
# too, or the test is a restriction we invented that {fmt} would accept.
export FFMT_FMT_FLAGS="-std=c++20 -Wall -Werror -DNEG_CHECK_FMT"
