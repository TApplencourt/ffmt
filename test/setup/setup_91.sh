# Setup for id 91: upstream clang HIP on MI300X (gfx942), llvm release-23.1.6 + rocm 10.1.0, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module use "${FFMT_RUNTIME_ROOT:?FFMT_RUNTIME_ROOT must be set}/modulefiles"
module load llvm/release-23.1.6
# Upstream clang needs ROCm's device libraries and HIP runtime (--rocm-path).
export ROCM_PATH=/vast/projects/intel_anl_shared/ffmt-ci/work/tapplencourt/rocm-10.1.0
export LD_LIBRARY_PATH=$ROCM_PATH/lib:${LD_LIBRARY_PATH:-}
export HIP_VISIBLE_DEVICES=0
export CXX=clang++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x hip --offload-arch=gfx942 --rocm-path=$ROCM_PATH -O0 -fno-fast-math"
export FFMT_LDFLAGS="-L$ROCM_PATH/lib -lamdhip64"
