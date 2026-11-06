# Setup for id 43: host g++ 13 (ubuntu-24.04), buffer path, C++23, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS and FFMT_MAIN.

# ubuntu-24.04's GCC 13 (13.3.0), as on the GitHub runners; another GCC
# version is another id.
export CXX=g++-13
export FFMT_FLAGS="-std=c++23 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=1"
export FFMT_MAIN=test/main_host.cpp
# A host example the buffer path can print (the specifiers path rejects its
# specs): built and run by the test goal, but not a test (Makefile.id).
export FFMT_EXTRA_FILES="examples/ccpreference_print.cpp"
