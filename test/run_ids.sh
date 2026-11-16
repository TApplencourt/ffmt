#!/bin/bash
# Run several ids one after another, in one allocation, then check them all.
#
#   test/run_ids.sh 2 3 10 11
#
# Each id gets its own test/run_id.sh process, so one that fails does not
# stop the next; test/check_id.py decides, and its status is this script's.
cd "$(dirname "$0")/.." || exit 1
[ "$#" -gt 0 ] || { echo "usage: test/run_ids.sh <id> [<id> ...]" >&2; exit 64; }

for id in "$@"; do
  test/run_id.sh "$id"
done
exec "${PYTHON:-python3}" test/check_id.py "$@"
