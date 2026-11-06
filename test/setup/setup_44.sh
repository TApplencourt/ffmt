# Setup for id 44: icpx SYCL (DPC++) on Aurora PVC, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module load oneapi/release/2026.1.0
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export ZE_AFFINITY_MASK=0.0   # one tile: never share a GPU between runs
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fsycl -O2 -fno-fast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
