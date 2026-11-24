# Setup for id 90: upstream clang OpenMP offload on MI300X (gfx942), llvm release-23.1.6 + rocm 10.1.0 device libraries, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module use "${FFMT_RUNTIME_ROOT:?FFMT_RUNTIME_ROOT must be set}/modulefiles"
module load llvm/release-23.1.6
# Upstream clang needs ROCm's device libraries for gfx942 (--rocm-path).
export ROCM_PATH=/vast/projects/intel_anl_shared/ffmt-ci/work/tapplencourt/rocm-10.1.0
export LD_LIBRARY_PATH=$ROCM_PATH/lib:${LD_LIBRARY_PATH:-}
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fopenmp --offload-arch=gfx942 --rocm-path=$ROCM_PATH -O0 -fno-fast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_omp.cpp examples/readme2_omp.cpp"
