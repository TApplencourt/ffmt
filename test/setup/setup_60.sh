# Setup for id 60: Rome g++ 13.3.0, specifiers path, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load gcc/13.3.0
export CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=0"
