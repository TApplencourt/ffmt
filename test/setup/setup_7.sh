# Setup for id 7: ROCm clang OpenMP offload on MI300X (gfx942), rocm 7.0.2, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load rocm/7.0.2
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host
export CXX=$ROCM_PATH/llvm/bin/clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fopenmp --offload-arch=gfx942 -O0 -fno-fast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_omp.cpp examples/readme2_omp.cpp"
