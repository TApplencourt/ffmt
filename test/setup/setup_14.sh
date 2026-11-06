# Setup for id 14: icpx SYCL (DPC++) on PVC, -O0, fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# 1032.21, not the 821.36 module: under 821 the Level Zero devices are
# not visible to this oneAPI, so level_zero:gpu finds nothing and SYCL
# falls back to OpenCL. There is no module for 1032.21 yet.
export LD_LIBRARY_PATH=/soft/libraries/intel-gpu-umd/1032.21/lib64:/soft/libraries/intel-gpu-umd/1032.21/lib64/intel-opencl:${LD_LIBRARY_PATH:-}
set --   # setvars.sh reads the caller's "$@" as its own arguments
source /vast/users/bertoni/intel/oneapi/setvars.sh
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fsycl -O0 -ffast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
