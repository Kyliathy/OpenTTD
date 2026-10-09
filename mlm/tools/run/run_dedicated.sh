#!/usr/bin/env bash
# @file Shared shell entry for isolated repository multi-layer sessions.
# @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 5.
# @param $1 Build directory containing the requested native configuration.
# @param $2 Debug or RelWithDebInfo.
# @param $3 Log basename; the shared run context owns its directory.
# @param $@ Remaining structured native arguments, including optional -c/-g.
# @return Native exit status; Python retains current-run logs and runtime data.
set -eu
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
exec python -B "$script_dir/native_launch.py" --dedicated "$@"
