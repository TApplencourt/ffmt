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

module load intel_compute_runtime/release/821.36
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
