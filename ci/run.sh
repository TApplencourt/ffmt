#!/bin/bash
# Build and run one ffmt backend, and write its TAP summary as JSON.
#
#   ci/run.sh <backend> <opt>
#
# backend is one of the names below; opt is O0 or O2. Each name maps to the
# Makefile variables for that backend (USE_ACPP, USE_OMP_CLANG, ...) and to
# the test goal that runs it.
#
# Writes results/<backend>-<opt>.{log,json}. Exits nonzero if any test
# failed; a skipped test is not a failure -- it names a known compiler bug
# from the catalog in test/capture.hpp.
set -o pipefail

BACKEND=${1:?usage: run.sh <backend> <opt>}
OPT=${2:?usage: run.sh <backend> <opt>}
# JLSE hardware: Intel PVC (chiatta), NVIDIA GB300 (gpu_gb300), AMD MI300X
# (gpu_amd_mi300x). sm_100 is Blackwell; gfx942 is MI300X.
: "${NVIDIA_ARCH:=sm_100}"
: "${AMD_ARCH:=gfx942}"

# Per-machine environment. FFMT_MACHINE selects it; by default guess from
# the hostname, so run.sh works unchanged whether a runner lands on the node
# or a batch script does.
: "${FFMT_MACHINE:=$(case "$(hostname -s)" in
      chiatta*)        echo chiatta ;;
      amdgpu*)         echo mi300x ;;
      graceblackwell*) echo gb300 ;;
      *)               echo unknown ;;
    esac)}"

if [ -f /etc/profile.d/modules.sh ]; then
  source /etc/profile.d/modules.sh
fi
if type module >/dev/null 2>&1; then
  module use /soft/modulefiles
fi

# Toolchains JLSE does not ship: AdaptiveCpp, a current chipStar, Kokkos.
# Nathan built these for the paper; point FFMT_SOFTWARE elsewhere to use a
# different set. Layout is <prefix>/<profile>/install/<tool>.
: "${FFMT_SOFTWARE:=/vast/users/nnichols/ffmt_project/ffmt/software}"
PREBUILT="$FFMT_SOFTWARE/$FFMT_MACHINE/install"

case "$FFMT_MACHINE" in
  chiatta)
    # Level Zero runtime for the Max 1550. Our own oneAPI, not the shared one.
    module load intel_compute_runtime/release/821.36 || true
    export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
    export CHIP_BE="${CHIP_BE:-level0}"
    export CHIP_DEVICE_TYPE="${CHIP_DEVICE_TYPE:-gpu}"
    # chipStar 7.2.0-ed3bc0c (v1.3.0). No AdaptiveCpp is built for chiatta.
    : "${FFMT_CHIPSTAR:=$PREBUILT/chipstar/bin/hipcc}"
    : "${FFMT_KOKKOS_PREFIX:=$PREBUILT/kokkos-sycl}"
    ;;
  mi300x)
    module load rocm/7.0.2 || true
    # ROCm's own clang, not the llvm/release-22 module: LLVM22 with ROCm7
    # faults during printf on MI300X, which is exactly what ffmt tests.
    : "${ROCM_PATH:=/soft/compilers/rocm/rocm-7.0.2}"
    export AMD_CLANGXX="${AMD_CLANGXX:-$ROCM_PATH/llvm/bin/clang++}"
    export LD_LIBRARY_PATH="$ROCM_PATH/llvm/lib:${LD_LIBRARY_PATH:-}"
    : "${FFMT_ACPP:=$PREBUILT/adaptivecpp/bin/acpp}"
    : "${FFMT_KOKKOS_PREFIX:=$PREBUILT/kokkos-hip}"
    ;;
  gb300)
    # aarch64: no /soft x86 toolchains. Take the newest clang and cuda present.
    for v in release-23.1.3 release-22.1.8 release-21.1.0 release-20.1.8; do
      if [ -x "/soft/compilers/llvm/$v/bin/clang++" ]; then
        export PATH="/soft/compilers/llvm/$v/bin:$PATH"
        break
      fi
    done
    for v in cuda-12.9.1 cuda-12.3.0 cuda-12.2.2; do
      if [ -x "/soft/compilers/cuda/$v/bin/nvcc" ]; then
        export CUDA_PATH="/soft/compilers/cuda/$v"
        export PATH="$CUDA_PATH/bin:$PATH"
        export LD_LIBRARY_PATH="$CUDA_PATH/lib64:${LD_LIBRARY_PATH:-}"
        break
      fi
    done
    ;;
esac

# Fail rather than silently run on the host when offload is unavailable.
export OMP_TARGET_OFFLOAD=MANDATORY
export LC_ALL=C

case "$BACKEND" in
  icpx-sycl)
    set --   # setvars.sh reads the caller's "$@" as its own arguments
    source "${ONEAPI_SETVARS:-/vast/users/bertoni/intel/oneapi/setvars.sh}"
    COMPILER=icpx
    DEVICE=pvc
    GOAL=test
    ARGS="CXX=icpx AOT_DEVICE=pvc OPT_LEVELS=$OPT"
    ;;
  icpx-omp)
    set --
    source "${ONEAPI_SETVARS:-/vast/users/bertoni/intel/oneapi/setvars.sh}"
    COMPILER=icpx
    DEVICE=pvc
    GOAL=test-omp
    ARGS="CXX=icpx AOT_DEVICE=pvc USE_OMP_ICPX=1 OMP_OPT=-$OPT"
    ;;
  # acpp takes the buffer path where icpx takes the specifiers path, so
  # running it on an Intel GPU compares both emit paths on one device.
  # AdaptiveCpp. Built for mi300x and h100, not for chiatta -- so this
  # route has no compiler on the Intel side until one is built.
  acpp)
    COMPILER=${FFMT_ACPP:?no AdaptiveCpp for $FFMT_MACHINE; set FFMT_ACPP}
    DEVICE=${ACPP_DEVICE:-generic}
    GOAL=test
    ARGS="USE_ACPP=1 CXX=$COMPILER"
    ;;
  clang-cuda)
    COMPILER=clang++
    DEVICE=$NVIDIA_ARCH
    GOAL=test-cuda
    ARGS="CXX=clang++ USE_CUDA_CLANG=1 OFFLOAD_ARCH=$NVIDIA_ARCH"
    ;;
  # Both levels matter: the README records ~50 NVPTX-printf codegen bugs at
  # -O0, auto-gated via FFMT_PTX_CLANG_O0, so the two levels differ in how
  # much of the suite can run at all.
  clang-omp-amdgpu)
    COMPILER=${AMD_CLANGXX:?set AMD_CLANGXX or run on an mi300x node}
    DEVICE=$AMD_ARCH
    GOAL=test-omp
    ARGS="CXX=$COMPILER USE_OMP_CLANG=1 OFFLOAD_ARCH=$AMD_ARCH OMP_OPT=-$OPT"
    ;;
  clang-omp-nvptx)
    COMPILER=clang++
    DEVICE=$NVIDIA_ARCH
    GOAL=test-omp
    ARGS="CXX=clang++ USE_OMP_CLANG=1 OFFLOAD_ARCH=$NVIDIA_ARCH OMP_OPT=-$OPT"
    ;;
  # ─── routes that need work ffmt has not merged ───
  #
  # chipStar: HIP source on an Intel GPU. The toolchain is built
  # (7.2.0-ed3bc0c on chiatta) but the Makefile has no HIP backend -- issue
  # #13 lists it as a TODO. When it lands this should be close to:
  #
  # chipstar)
  #   COMPILER=${FFMT_CHIPSTAR:?no chipStar for $FFMT_MACHINE}
  #   DEVICE=pvc
  #   GOAL=test-hip
  #   ARGS="USE_HIP=1 CXX=$COMPILER"
  #   ;;
  #
  # Native HIP on AMD, same missing backend:
  #
  # hip)
  #   COMPILER=${ROCM_PATH:?}/bin/hipcc
  #   DEVICE=$AMD_ARCH
  #   GOAL=test-hip
  #   ARGS="USE_HIP=1 CXX=$COMPILER"
  #   ;;
  #
  # Kokkos: ffmt's print called from inside a Kokkos kernel, to check it
  # works under another programming model. kokkos-sycl is built for
  # chiatta and kokkos-hip for mi300x; ffmt has no Kokkos target yet
  # (also issue #13, "custom-print overloads for CUDA, SYCL, HIP, Kokkos").
  #
  # kokkos)
  #   COMPILER=... # whichever host compiler that Kokkos was built against
  #   GOAL=test-kokkos
  #   ARGS="USE_KOKKOS=1 KOKKOS_PREFIX=${FFMT_KOKKOS_PREFIX:?}"
  #   ;;

  *)
    echo "unknown backend: $BACKEND" >&2
    exit 2
    ;;
esac

OUT=results/$BACKEND-$OPT
mkdir -p results
make clean

# TAP goes to stderr, hence 2>&1. tapsum.py's exit status is the job's:
# nonzero when a test failed, zero when everything ran or was skipped.
make -j"$(nproc)" $ARGS "$GOAL" 2>&1 \
  | tee "$OUT.log" \
  | ci/tapsum.py \
      --backend "$BACKEND" \
      --opt "$OPT" \
      --device "$DEVICE" \
      --compiler "$COMPILER" \
      --commit "${CI_COMMIT_SHA:-$(git rev-parse HEAD 2>/dev/null)}" \
  > "$OUT.json"
RC=$?

cat "$OUT.json"
exit $RC
