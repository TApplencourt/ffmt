# Setup for id 34: host clang++ 18 (ubuntu-24.04), specifiers path, coverage instrumentation, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS and FFMT_MAIN.

# For the coverage gate: .github/scripts/coverage_union.py runs the
# binaries of both coverage ids and takes the union of their regions.
# ubuntu-24.04's default clang (18), as on the GitHub runners; another
# clang version is another id.
export CXX=clang++-18
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=0 -fprofile-instr-generate -fcoverage-mapping"
export FFMT_MAIN=test/main_host.cpp
