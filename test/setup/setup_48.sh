# Setup for id 48: native HIP on MI300X (gfx942), rocm 10.1.0 hipcc, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load gcc/13.3.0
# ROCm 10.1.0 (TheRock), newer than any /soft module; the tarball
# therock-dist-linux-gfx94X-dcgpu-10.1.0.tar.gz from
# https://stable.repo.amd.com/rocm/core/tarball/, unpacked as is.
export ROCM_PATH=/vast/projects/intel_anl_shared/ffmt-ci/work/tapplencourt/rocm-10.1.0
export PATH=$ROCM_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ROCM_PATH/lib:${LD_LIBRARY_PATH:-}
export HIP_VISIBLE_DEVICES=0
export CXX=$ROCM_PATH/bin/hipcc
export FFMT_ORACLE_CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x hip --offload-arch=gfx942 -O0 -fno-fast-math"
