# Setup for id 46: Aurora g++ 14.3 (module gcc/14.3.0), specifiers path, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS and FFMT_MAIN.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module load gcc/14.3.0
export CXX=g++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=0"
export FFMT_MAIN=test/main_host.cpp
