# Setup for id 35: host clang++ 18 (ubuntu-24.04), buffer path, coverage instrumentation, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# For the coverage gate: .github/scripts/coverage_union.py runs the
# binaries of both coverage ids and takes the union of their regions.
# ubuntu-24.04's default clang (18), as on the GitHub runners; another
# clang version is another id.
export CXX=clang++-18
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O2 -fno-fast-math -DFFMT_BUFFER_PATH=1 -fprofile-instr-generate -fcoverage-mapping"
# A host example the buffer path can print (the specifiers path rejects its
# specs): built and run by the test goal, but not a test (Makefile.id).
export FFMT_EXTRA_FILES="examples/ccpreference_print.cpp"
