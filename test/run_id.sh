#!/bin/bash
# Build and run the ffmt tests for one id of test/configs.json.
#
#   test/run_id.sh 4          build and run id 4
#   test/run_id.sh 4 build    build only
#   test/run_id.sh 4 extra-test  build and run only its extra examples
#
# Sources test/setup/setup_<id>.sh, so its modules and exports are in
# place, then runs Makefile.id, which reads CXX, FFMT_FLAGS and the rest
# from the environment.
cd "$(dirname "$0")/.." || exit 1

# Read the arguments before sourcing: the setup scripts run `set --` for
# setvars.sh, which clears them.
ID=${1:?usage: test/run_id.sh <id> [test|build|extra-test]}
GOAL=${2:-test}

SETUP=test/setup/setup_$ID.sh
if [ ! -f "$SETUP" ]; then
  echo "test/run_id.sh: no $SETUP" >&2
  exit 1
fi
source "$SETUP"

# Do not let a failed rebuild or an interrupted step reuse results from an
# earlier invocation. Example-only runs retain the prior complete main run.
case "$GOAL" in
  test|build)
    mkdir -p "results/id-$ID"
    rm -f "results/id-$ID/run.status" "results/id-$ID/run.log" \
      "results/id-$ID/negative.log" "results/id-$ID/isolated.log" \
      "results/id-$ID/extra.log" "results/id-$ID/build.status"
    ;;
  extra-test)
    mkdir -p "results/id-$ID"
    rm -f "results/id-$ID/extra.log"
    ;;
esac

# -j: the compiles are independent targets. Programs that use the GPU
# still run one at a time; Makefile.id orders them. Keep going after a main
# build failure so the independent negative, isolated, and extra logs are
# still complete for check_id.py.
# getconf, not nproc: macOS has no nproc.
make --keep-going -j"$(getconf _NPROCESSORS_ONLN)" -f Makefile.id id="$ID" "$GOAL"
status=$?
if [ "$GOAL" = build ]; then
  echo "$status" > "results/id-$ID/build.status"
fi
exit "$status"
