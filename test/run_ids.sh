#!/bin/bash
set +e
cd "$(dirname "$0")/.." || exit 1

if [ "$#" -eq 0 ]; then
  echo "usage: test/run_ids.sh <id> [<id> ...]" >&2
  exit 64
fi

python=${PYTHON:-python3}
mkdir -p results || exit 1
printf 'id\trun_id_status\tcheck_id_status\n' > results/batch-status.tsv || exit 1
printf '%s\n' "${COBALT_JOBID:-}" > results/cobalt-job-id || exit 1
failed=0

for id in "$@"; do
  mkdir -p "results/id-$id"
  test/run_id.sh "$id"
  run_status=$?
  "$python" test/check_id.py "$id"
  check_status=$?

  printf 'run_id_status=%s\ncheck_id_status=%s\n' \
    "$run_status" "$check_status" > "results/id-$id/batch.status"
  printf '%s\t%s\t%s\n' "$id" "$run_status" "$check_status" >> results/batch-status.tsv
  printf 'batch id=%s run_id_status=%s check_id_status=%s\n' \
    "$id" "$run_status" "$check_status"
  if [ "$check_status" -ne 0 ]; then
    failed=1
  fi
done

exit "$failed"
