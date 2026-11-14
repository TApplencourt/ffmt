# Setup for id 50: nvcc CUDA on H100 (sm_90), cuda 12.9.1, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load cuda/12.9.1
module load gcc/13.3.0
export CUDA_VISIBLE_DEVICES=0
export CXX=$CUDA_PATH/bin/nvcc
export FFMT_ORACLE_CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FFMT_ORACLE_LINK='-Xlinker=$(OUT)/fmt.o'
export FFMT_LANGUAGE_RESET=
export FFMT_UNUSED_CLI_WARNING=
export FFMT_FLAGS="-std=c++20 -x cu --extended-lambda --expt-relaxed-constexpr -arch=sm_90 -Xcompiler=-Wall,-Werror,-fno-fast-math -O0"
export FFMT_EXTRA_FILES="examples/readme1_cuda.cu examples/readme2_cuda.cu"
