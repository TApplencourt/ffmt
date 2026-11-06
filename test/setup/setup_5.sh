# Setup for id 5: clang OpenMP offload to NVPTX on A100 (sm_80), cuda 12.3.0 + llvm release-22.1.8, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module load cuda/12.3.0
module load llvm/release-22.1.8
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fopenmp --offload-arch=sm_80 -O0 -fno-fast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_omp.cpp examples/readme2_omp.cpp"
