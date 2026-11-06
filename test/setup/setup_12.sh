# Setup for id 12: host g++ 13 (ubuntu-24.04), specifiers path, -O2, fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# ubuntu-24.04's GCC 13 (13.3.0), as on the GitHub runners; another GCC
# version is another id.
export CXX=g++-13
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -ffast-math -DFFMT_BUFFER_PATH=0"
