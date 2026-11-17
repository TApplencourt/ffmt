# Setup for id 21: ROCm clang OpenMP offload on MI300X (gfx942), rocm 10.1.0, -O0, fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
# ROCm 10.1.0 (TheRock), newer than any /soft module; the tarball
# therock-dist-linux-gfx94X-dcgpu-10.1.0.tar.gz from
# https://stable.repo.amd.com/rocm/core/tarball/, unpacked as is.
export ROCM_PATH=/vast/projects/intel_anl_shared/ffmt-ci/work/tapplencourt/rocm-10.1.0
export PATH=$ROCM_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ROCM_PATH/lib:${LD_LIBRARY_PATH:-}
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host
export CXX=$ROCM_PATH/llvm/bin/clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fopenmp --offload-arch=gfx942 -O0 -ffast-math -Wno-nan-infinity-disabled"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_omp.cpp examples/readme2_omp.cpp"
