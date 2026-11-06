# Setup for id 0: host g++ 13 (ubuntu-24.04), specifiers path, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS and FFMT_MAIN.

# ubuntu-24.04's GCC 13 (13.3.0), as on the GitHub runners; another GCC
# version is another id.
export CXX=g++-13
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=0"
export FFMT_MAIN=test/main_host.cpp
