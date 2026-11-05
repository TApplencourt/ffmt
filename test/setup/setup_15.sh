# Setup for id 15: icpx SYCL (DPC++) on PVC, -O2, fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS, FFMT_MAIN, FFMT_SYNTAX_FLAGS
# and FFMT_FMT_FLAGS.

# 1032.21, not the 821.36 module: under 821 the Level Zero devices are
# not visible to this oneAPI, so level_zero:gpu finds nothing and SYCL
# falls back to OpenCL. There is no module for 1032.21 yet.
export LD_LIBRARY_PATH=/soft/libraries/intel-gpu-umd/1032.21/lib64:/soft/libraries/intel-gpu-umd/1032.21/lib64/intel-opencl:${LD_LIBRARY_PATH:-}
set --   # setvars.sh reads the caller's "$@" as its own arguments
source /vast/users/bertoni/intel/oneapi/setvars.sh
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fsycl -O2 -ffast-math"
export FFMT_MAIN=test/test_main_sycl.cpp
# Only for the negative tests' first pass: NEG_SYNTAX_CHECK turns the
# harness's format call into a no-op, so a negative file that still fails to
# compile has a bug of its own and its rejection would mean nothing.
export FFMT_SYNTAX_FLAGS="-DNEG_SYNTAX_CHECK"
# Only for the negative tests' {fmt} cross-check: NEG_CHECK_FMT makes the
# harness call fmt::format instead of ffmt, and {fmt} must reject the case
# too, or the test is a restriction we invented that {fmt} would accept.
export FFMT_FMT_FLAGS="-std=c++20 -Wall -Werror -DNEG_CHECK_FMT"
