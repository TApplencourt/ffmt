# Setup for id 18: clang CUDA on A100 (sm_80), cuda 12.3.0 + llvm release-22.1.8, -O2, fast-math.
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS, FFMT_MAIN, FFMT_SYNTAX_FLAGS
# and FFMT_FMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module load cuda/12.3.0
module load llvm/release-22.1.8
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x cuda --cuda-gpu-arch=sm_80 -L${CUDA_PATH:-/usr/local/cuda}/lib64 -lcudart -O2 -ffast-math"
export FFMT_MAIN=test/main_omp.cpp
# Only for the negative tests' first pass: NEG_SYNTAX_CHECK turns the
# harness's format call into a no-op, so a negative file that still fails to
# compile has a bug of its own and its rejection would mean nothing.
export FFMT_SYNTAX_FLAGS="-DNEG_SYNTAX_CHECK"
# Only for the negative tests' {fmt} cross-check: NEG_CHECK_FMT makes the
# harness call fmt::format instead of ffmt, and {fmt} must reject the case
# too, or the test is a restriction we invented that {fmt} would accept.
export FFMT_FMT_FLAGS="-std=c++20 -Wall -Werror -DNEG_CHECK_FMT"
