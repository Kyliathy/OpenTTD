#!/usr/bin/env bash
#Run the OpenTTD game with its window for a player, without a console window, and keep its debug output in a log file.
#
#Written for the scenario runs of docs/zz-reach2/upgrades/2026-10/r2rvm2-review-map-2.md (task 17.1).
#Debug facilities and levels: docs/zz-reach2/architecture/logging/archi-logging.md.
#
#Usage: run_game_logged.sh <build dir> <config> <log file> <openttd arguments...>
#  build dir  the CMake build folder, also the working and personal folder of the game (-c makes it so)
#  config     Debug or RelWithDebInfo; scenario runs use Debug, because the asserts are part of the evidence
#  log file   where stdout and stderr of the game go; a relative path is relative to the build dir
#  arguments  passed on unchanged, for example: -c scenario.cfg -g -G 1234
#Environment: OTTD_DEBUG overrides the -d levels, EDITBIN the path of editbin.exe.
#
#With -d, openttd.exe opens a console window of its own (CreateConsole in os/windows/win32.cpp) and writes
#there. A copy marked as a console program inherits this shell's output instead, so the redirect works.
#Desync messages never reach that output: DebugPrint writes them to commands-out.log in the autosave folder,
#which is <build dir>/save/autosave/commands-out.log with a -c config in the build folder.

set -u

if [ "$#" -lt 3 ]; then
	echo "usage: run_game_logged.sh <build dir> <config> <log file> <openttd arguments...>" >&2
	exit 2
fi
build_dir=$1
config=$2
log=$3
shift 3

exe="$build_dir/$config/openttd.exe"
console="$build_dir/$config/openttd_console.exe"
if [ ! -f "$exe" ]; then
	echo "run_game_logged.sh: $exe does not exist; build $config first" >&2
	exit 2
fi

#Print the PE subsystem of an executable: 2 is a GUI program, 3 a console program.
#  $1 path of the executable
pe_subsystem() {
	local header
	header=$(od -An -t u4 -j 60 -N 4 "$1" | tr -d ' ')
	od -An -t u2 -j $((header + 92)) -N 2 "$1" | tr -d ' '
}

#A build replaces openttd.exe, so the console copy is redone when it is missing, older, or an edit of it failed.
if [ ! -f "$console" ] || [ "$exe" -nt "$console" ] || [ "$(pe_subsystem "$console")" != "3" ]; then
	editbin=${EDITBIN:-}
	if [ -z "$editbin" ]; then
		#The Build Tools keep one folder per MSVC version; the newest one is taken.
		for candidate in "/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/"*/bin/Hostx64/x64/editbin.exe; do
			[ -f "$candidate" ] && editbin=$candidate
		done
	fi
	if [ -z "$editbin" ]; then
		echo "run_game_logged.sh: editbin.exe not found; set EDITBIN" >&2
		exit 2
	fi
	cp "$exe" "$console" || exit 2
	#Git Bash would turn the /options into Windows paths, so path conversion is off for this call.
	MSYS_NO_PATHCONV=1 "$editbin" /nologo /subsystem:console "$(cygpath -w "$console")" || exit 2
	if [ "$(pe_subsystem "$console")" != "3" ]; then
		echo "run_game_logged.sh: $console is still not a console program" >&2
		exit 2
	fi
fi

cd "$build_dir" || exit 2
mkdir -p "$(dirname "$log")"
#desync=2 turns on CheckCaches every tick; misc shows paths and config, sl loading and saving, net a local server.
levels=${OTTD_DEBUG:-desync=2,misc=3,sl=2,net=2}
started=$(date +%T)
#No time limit: the player decides when the game ends.
"./$config/openttd_console.exe" -d "$levels" "$@" > "$log" 2>&1
status=$?
echo "run_game_logged.sh: $started-$(date +%T) exit $status, $(wc -l < "$log") log lines in $log"
exit $status
