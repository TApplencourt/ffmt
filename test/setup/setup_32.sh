# Setup for id 32: macOS Apple clang (libc++), specifiers path, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# Apple clang with libc++: catches what only shows up on macOS.
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=0"
