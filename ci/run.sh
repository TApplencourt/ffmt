#!/bin/bash
# Build and run one ffmt backend, and summarise its TAP output as JSON.
#
#   ci/run.sh sycl O2
#   backends: sycl, omp-spirv (chiatta); cuda, omp-nvptx (a100);
#             omp-amdgpu (mi300x)
#
# Writes results/<backend>-<opt>.{log,json}.
set -o pipefail
cd "$(dirname "$0")/.." || exit 1

BACKEND=${1:?usage: run.sh <backend> <opt>}
OPT=${2:?usage: run.sh <backend> <opt>}

case "$BACKEND" in
  sycl)
    # 1032.21, not the 821.36 module: under 821 the Level Zero devices are
    # not visible to this oneAPI, so level_zero:gpu finds nothing and SYCL
    # falls back to OpenCL. There is no module for 1032.21 yet.
    export LD_LIBRARY_PATH=/soft/libraries/intel-gpu-umd/1032.21/lib64:/soft/libraries/intel-gpu-umd/1032.21/lib64/intel-opencl:${LD_LIBRARY_PATH:-}
    set --   # setvars.sh reads the caller's "$@" as its own arguments
    source /vast/users/bertoni/intel/oneapi/setvars.sh
    export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
    DEVICE=pvc
    CXX=icpx
    GOAL=test
    ARGS="OPT_LEVELS=$OPT"
    ;;
  omp-spirv)
    export LD_LIBRARY_PATH=/soft/libraries/intel-gpu-umd/1032.21/lib64:/soft/libraries/intel-gpu-umd/1032.21/lib64/intel-opencl:${LD_LIBRARY_PATH:-}
    set --
    source /vast/users/bertoni/intel/oneapi/setvars.sh
    export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
    DEVICE=pvc
    CXX=icpx
    GOAL=test-omp
    ARGS="USE_OMP_ICPX=1 OMP_OPT=-$OPT"
    ;;
  cuda)
    module load cuda/12.9.1
    DEVICE=${OFFLOAD_ARCH:=sm_80}
    CXX=clang++
    GOAL=test-cuda
    ARGS="USE_CUDA_CLANG=1 OFFLOAD_ARCH=$DEVICE"
    ;;
  omp-nvptx)
    module load cuda/12.9.1
    DEVICE=${OFFLOAD_ARCH:=sm_80}
    CXX=clang++
    GOAL=test-omp
    ARGS="USE_OMP_CLANG=1 OFFLOAD_ARCH=$DEVICE OMP_OPT=-$OPT"
    ;;
  omp-amdgpu)
    module load rocm/7.0.2
    DEVICE=${OFFLOAD_ARCH:=gfx942}
    CXX=$ROCM_PATH/llvm/bin/clang++
    GOAL=test-omp
    ARGS="USE_OMP_CLANG=1 OFFLOAD_ARCH=$DEVICE OMP_OPT=-$OPT"
    ;;
  *)
    echo "unknown backend: $BACKEND" >&2
    exit 2
    ;;
esac

export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host

mkdir -p results
make clean

# TAP goes to stderr, hence 2>&1.
make -j CXX="$CXX" $ARGS "$GOAL" 2>&1 \
  | tee "results/$BACKEND-$OPT.log" \
  | ci/tapsum.py --backend "$BACKEND" --opt "$OPT" --device "$DEVICE" --compiler "$CXX" \
      --commit "${CI_COMMIT_SHA:-$(git rev-parse HEAD 2>/dev/null)}" \
  > "results/$BACKEND-$OPT.json"
RC=$?

cat "results/$BACKEND-$OPT.json"
exit $RC
