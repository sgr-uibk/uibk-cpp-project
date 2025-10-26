#!/usr/bin/env bash
set -euo pipefail

SERVER="./LobbyServer"
CLIENT="./LobbyClient"
NAMES=(Server A B C D)
PIDS=()

"$SERVER" & PIDS+=($!)
for name in A B C D; do
  "$CLIENT" "$name" & PIDS+=($!)
done

timeout=15
end=$((SECONDS + timeout))

all_exited() {
  for pid in "${PIDS[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then return 1; fi
  done
  return 0
}

while (( SECONDS < end )); do
  all_exited && break
  sleep 0.1
done

# If any still running -> fail
running=()
for pid in "${PIDS[@]}"; do
  if kill -0 "$pid" 2>/dev/null; then running+=("$pid"); fi
done
if (( ${#running[@]} )); then
  echo "Timeout: still running: ${running[*]}" >&2
  kill "${running[@]}" 2>/dev/null || true
  exit 1
fi

# Collect exit codes
failures=0
for i in "${!PIDS[@]}"; do
  pid=${PIDS[i]}
  wait "$pid"
  code=$?
  echo "${NAMES[i]} (pid $pid) exit $code"
  (( code != 0 )) && failures=$((failures+1))
done

(( failures )) && { echo "Failed: $failures non-zero exits" >&2; exit 1; }
echo "OK: all exited 0 within ${timeout}s"
