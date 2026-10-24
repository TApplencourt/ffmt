#!/bin/bash
# Build and run one ffmt backend, and summarise its TAP output as JSON.
#
#   ci/run.sh icpx-sycl O2
#
# Writes results/<backend>-<opt>.{log,json}.
set -o pipefail
cd "$(dirname "$0")/.." || exit 1

BACKEND=${1:?usage: run.sh <backend> <opt>}
OPT=${2:?usage: run.sh <backend> <opt>}

# 1032.21, not the 821.36 module: under 821 the Level Zero devices are not
# visible to this oneAPI, so level_zero:gpu finds nothing and SYCL silently
# falls back to OpenCL. There is no module for 1032.21 yet.
export LD_LIBRARY_PATH=/soft/libraries/intel-gpu-umd/1032.21/lib64:/soft/libraries/intel-gpu-umd/1032.21/lib64/intel-opencl:${LD_LIBRARY_PATH:-}
set --   # setvars.sh reads the caller's "$@" as its own arguments
source /vast/users/bertoni/intel/oneapi/setvars.sh

export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export OMP_TARGET_OFFLOAD=MANDATORY   # fail rather than fall back to the host

mkdir -p results
make clean

# TAP goes to stderr, hence 2>&1.
make -j"$(nproc)" CXX=icpx OPT_LEVELS="$OPT" test 2>&1 \
  | tee "results/$BACKEND-$OPT.log" \
  | ci/tapsum.py --backend "$BACKEND" --opt "$OPT" --device pvc --compiler icpx \
  > "results/$BACKEND-$OPT.json"
RC=$?

cat "results/$BACKEND-$OPT.json"
exit $RC
