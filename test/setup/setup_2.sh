# Setup for id 2: icpx OpenMP offload to SPIR-V on PVC, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS, FFMT_MAIN, FFMT_SYNTAX_FLAGS
# and FFMT_FMT_FLAGS.

# No UMD on LD_LIBRARY_PATH, unlike the SYCL setups: any of 803.45, 821.36
# or 1032.21 segfaults this at -O0, while the system runtime passes.
set --   # setvars.sh reads the caller's "$@" as its own arguments
source /vast/users/bertoni/intel/oneapi/setvars.sh
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fiopenmp -fopenmp-targets=spir64 -O0 -fno-fast-math"
export FFMT_MAIN=test/main_omp.cpp
# Only for the negative tests' first pass: NEG_SYNTAX_CHECK turns the
# harness's format call into a no-op, so a negative file that still fails to
# compile has a bug of its own and its rejection would mean nothing.
export FFMT_SYNTAX_FLAGS="-DNEG_SYNTAX_CHECK"
# Only for the negative tests' {fmt} cross-check: NEG_CHECK_FMT makes the
# harness call fmt::format instead of ffmt, and {fmt} must reject the case
# too, or the test is a restriction we invented that {fmt} would accept.
export FFMT_FMT_FLAGS="-std=c++20 -Wall -Werror -DNEG_CHECK_FMT"
