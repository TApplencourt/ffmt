# Setup for id 45: icpx OpenMP offload to SPIR-V on Aurora PVC, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module load oneapi/release/2026.1.0
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export ZE_AFFINITY_MASK=0.0   # one tile: never share a GPU between runs
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host
export CXX=icpx
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fiopenmp -fopenmp-targets=spir64 -O2 -fno-fast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_omp.cpp examples/readme2_omp.cpp"
