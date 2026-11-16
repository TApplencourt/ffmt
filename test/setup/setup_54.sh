# Setup for id 54: clang CUDA on H100 (sm_90), cuda 12.9.1 + llvm release-22.1.8, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load cuda/12.9.1
module load llvm/release-22.1.8
export CUDA_VISIBLE_DEVICES=0
export CXX=/soft/compilers/llvm/release-22.1.8/bin/clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -Wno-unknown-cuda-version -x cuda --cuda-path=$CUDA_PATH --cuda-gpu-arch=sm_90 -L$CUDA_PATH/lib64 -lcudart -O0 -fno-fast-math"
export FFMT_EXTRA_FILES="examples/readme1_cuda.cu examples/readme2_cuda.cu"
