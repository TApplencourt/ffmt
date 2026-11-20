# Setup for id 11: icpx SYCL (DPC++) on PVC, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load intel/oneapi/release/2026.2.0 compiler
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fsycl -O2 -fno-fast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
