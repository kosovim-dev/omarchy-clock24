#!/usr/bin/env bash
# Reload the Omarchy shell without leaving duplicate bars behind.
#
# omarchy-restart-shell's `quickshell kill` can miss an old instance whose
# runtime socket is already gone (e.g. after a prior engine reload), leaving
# the old bar mounted next to the new one. Stop every supervisor tree first —
# each supervisor's trap terminates its own shell instance — then run the
# ordinary restart and verify that exactly one shell comes back.

set -u

echo "Stopping existing Omarchy shell supervisor(s)..."
pkill -TERM -f 'omarchy-launch-shell' 2>/dev/null || true

# Wait for the old processes (and any defunct children) to be reaped.
for _ in $(seq 1 50); do
    pgrep -f 'quickshell .*-p /usr/share/omarchy/shell' >/dev/null 2>&1 || break
    sleep 0.1
done

omarchy-restart-shell
status=$?
if (( status != 0 )); then
    echo "omarchy-restart-shell failed (exit $status)." >&2
    exit $status
fi

count=$(pgrep -f 'quickshell .*-p /usr/share/omarchy/shell' | wc -l)
if (( count != 1 )); then
    echo "ERROR: $count shell instances running; expected exactly one." >&2
    pgrep -af 'quickshell .*-p /usr/share/omarchy/shell'
    exit 1
fi

echo "Shell reloaded with a single instance (supervisor $(pgrep -f omarchy-launch-shell | tr '\n' ' '))"
omarchy-shell shell ping