# Setup for id 56: clang OpenMP offload to NVPTX on H100 (sm_90), cuda 12.9.1 + llvm release-23.1.6, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module use "${FFMT_RUNTIME_ROOT:?FFMT_RUNTIME_ROOT must be set}/modulefiles"
module load cuda/12.9.1
module load llvm/release-23.1.6
export CUDA_VISIBLE_DEVICES=0
export OMP_TARGET_OFFLOAD=MANDATORY
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -Wno-unknown-cuda-version -fopenmp --offload-arch=sm_90 --cuda-path=$CUDA_PATH -O0 -fno-fast-math"
export FFMT_EXTRA_FILES="examples/readme1_omp.cpp examples/readme2_omp.cpp"
