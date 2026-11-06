#!/bin/bash
# Build and run the ffmt tests for one id of test/configs.json.
#
#   test/run_id.sh 4          build and run id 4
#   test/run_id.sh 4 build    build only
#
# Sources test/setup/setup_<id>.sh, so its modules and exports are in
# place, then runs Makefile.id, which reads CXX, FFMT_FLAGS and the rest
# from the environment.
cd "$(dirname "$0")/.." || exit 1

# Read the arguments before sourcing: the setup scripts run `set --` for
# setvars.sh, which clears them.
ID=${1:?usage: test/run_id.sh <id> [test|build]}
GOAL=${2:-test}

SETUP=test/setup/setup_$ID.sh
if [ ! -f "$SETUP" ]; then
  echo "test/run_id.sh: no $SETUP" >&2
  exit 1
fi
source "$SETUP"

# -j: the compiles are independent targets. Programs that use the GPU
# still run one at a time; Makefile.id orders them.
# getconf, not nproc: macOS has no nproc.
exec make -j"$(getconf _NPROCESSORS_ONLN)" -f Makefile.id id="$ID" "$GOAL"
