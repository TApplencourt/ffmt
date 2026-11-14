# Setup for id 59: clang CUDA on GH200 (sm_90), llvm release-23.1.3 + cuda 12.9.1 + g++ 14.3.0, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load llvm/release-23.1.3
module load cuda/12.9.1
export CUDA_PATH=${CUDA_PATH:-/soft/compilers/cuda/cuda-12.9.1}
if [ ! -x "$CUDA_PATH/bin/nvcc" ]; then echo "setup_59.sh: missing $CUDA_PATH/bin/nvcc" >&2; return 1; fi
export LD_LIBRARY_PATH=$CUDA_PATH/lib64:${LD_LIBRARY_PATH:-}
export CUDA_VISIBLE_DEVICES=0
export CXX=/soft/compilers/llvm/release-23.1.3/bin/clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x cuda --cuda-path=$CUDA_PATH --cuda-gpu-arch=sm_90 --gcc-install-dir=/usr/lib64/gcc/aarch64-suse-linux/14 -L$CUDA_PATH/lib64 -lcudart -O2 -fno-fast-math"
export FFMT_EXTRA_FILES="examples/readme1_cuda.cu examples/readme2_cuda.cu"
