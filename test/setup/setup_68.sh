# Setup for id 68: GH200 native g++ 14.3.0 (aarch64), specifiers path, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

export CXX=/usr/bin/g++-14
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O0 -fno-fast-math -DFFMT_BUFFER_PATH=0"
