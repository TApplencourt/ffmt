# Setup for id 49: native HIP on MI300X (gfx942), rocm 7.0.2 hipcc, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load gcc/13.3.0
module load rocm/7.0.2
export HIP_VISIBLE_DEVICES=0
export CXX=$ROCM_PATH/bin/hipcc
export FFMT_ORACLE_CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x hip --offload-arch=gfx942 -O2 -fno-fast-math"
