# Setup for id 17: icpx OpenMP offload to SPIR-V on PVC, -O2, fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# No UMD on LD_LIBRARY_PATH, unlike the SYCL setups: any of 803.45, 821.36
# or 1032.21 segfaults this at -O0, while the system runtime passes.
set --   # setvars.sh reads the caller's "$@" as its own arguments
source /vast/users/bertoni/intel/oneapi/setvars.sh
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fiopenmp -fopenmp-targets=spir64 -O2 -ffast-math -Wno-nan-infinity-disabled"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_omp.cpp examples/readme2_omp.cpp"
