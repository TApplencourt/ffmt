# Setup for id 15: icpx SYCL (DPC++) on PVC, -O2, fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

set --   # setvars.sh reads the caller's "$@" as its own arguments
source /vast/users/bertoni/intel/oneapi/setvars.sh
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fsycl -O2 -ffast-math -Wno-nan-infinity-disabled"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
