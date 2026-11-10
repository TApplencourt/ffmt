# Setup for id 52: nvcc CUDA on GH200 (sm_90), cuda 12.9.1 + g++ 14.3.0, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load cuda/12.9.1
export CUDA_PATH=${CUDA_PATH:-/soft/compilers/cuda/cuda-12.9.1}
if [ ! -x "$CUDA_PATH/bin/nvcc" ]; then echo "setup_52.sh: missing $CUDA_PATH/bin/nvcc" >&2; return 1; fi
export PATH=$CUDA_PATH/bin:$PATH
export LD_LIBRARY_PATH=$CUDA_PATH/lib64:${LD_LIBRARY_PATH:-}
export CUDA_VISIBLE_DEVICES=0
export CXX=$CUDA_PATH/bin/nvcc
export FFMT_ORACLE_CXX=/usr/bin/g++-14
export FFMT_ORACLE_LINK='-Xlinker=$(OUT)/fmt.o'
export FFMT_LANGUAGE_RESET=
export FFMT_UNUSED_CLI_WARNING=
export FFMT_FLAGS="-std=c++20 -x cu --extended-lambda --expt-relaxed-constexpr -arch=sm_90 -ccbin=/usr/bin/g++-14 -Xcompiler=-Wall,-Werror,-fno-fast-math -O0"
export FFMT_EXTRA_FILES="examples/readme1_cuda.cu examples/readme2_cuda.cu"
