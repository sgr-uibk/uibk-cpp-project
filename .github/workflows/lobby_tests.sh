#!/usr/bin/env bash
set -euo pipefail

# pick a random port >= 20000
PORT=$(( (RANDOM % 45535) + 20000 ))

# start server and clients in background, remember their PIDs
./LobbyServer "$PORT" &
PID_SERVER=$!

./LobbyClient A "$PORT" &
PID_A=$!
./LobbyClient B "$PORT" &
PID_B=$!
./LobbyClient C "$PORT" &
PID_C=$!
./LobbyClient D "$PORT" &
PID_D=$!

echo "Started LobbyServer ($PID_SERVER:$PORT) and clients ($PID_A,$PID_B,$PID_C,$PID_D)"
sleep 15

# attempt graceful termination, then force after a timeout
kill $PID_A $PID_B $PID_C $PID_D $PID_SERVER 2>/dev/null || true
sleep 1
kill -9 $PID_A $PID_B $PID_C $PID_D $PID_SERVER 2>/dev/null || true
