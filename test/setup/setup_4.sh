# Setup for id 4: clang CUDA on A100 (sm_80), cuda 12.3.0 + llvm release-22.1.8, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load cuda/12.3.0
module load llvm/release-22.1.8
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x cuda --cuda-gpu-arch=sm_80 -L${CUDA_PATH:-/usr/local/cuda}/lib64 -lcudart -O2 -fno-fast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_cuda.cu examples/readme2_cuda.cu"
