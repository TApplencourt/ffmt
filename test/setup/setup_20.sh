# Setup for id 20: clang OpenMP offload to NVPTX on A100 (sm_80), cuda 12.3.0 + llvm release-23.1.6, -O2, fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module use "${FFMT_RUNTIME_ROOT:?FFMT_RUNTIME_ROOT must be set}/modulefiles"
module load cuda/12.3.0
module load llvm/release-23.1.6
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fopenmp --offload-arch=sm_80 -O2 -ffast-math -Wno-nan-infinity-disabled"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_omp.cpp examples/readme2_omp.cpp"
