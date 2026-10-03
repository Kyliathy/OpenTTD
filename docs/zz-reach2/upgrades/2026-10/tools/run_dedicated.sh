#!/usr/bin/env bash
#Run one OpenTTD dedicated-server game without opening a console window, and keep all of its output in a log file.
#
#Written for docs/zz-reach2/upgrades/2026-10/r2rvm-review-map.md (### Commands used by every group).
#Debug facilities and levels: docs/zz-reach2/architecture/logging/archi-logging.md.
#
#Usage: run_dedicated.sh <build dir> <config> <log file> <openttd arguments...>
#  build dir  the CMake build folder, also the working and personal folder of the game (-c makes it so)
#  config     Debug or RelWithDebInfo
#  log file   where stdout and stderr of the game go; a relative path is relative to the build dir
#  arguments  passed on unchanged, for example: -c baseline.cfg -G 1234
#Environment: OTTD_DEBUG overrides the -d levels, OTTD_TIMEOUT the time limit in seconds (default 600),
#EDITBIN the path of editbin.exe.
#
#openttd.exe is a GUI program. With -D it opens a console window of its own and writes most output there,
#so a redirect catches almost nothing. Regression.cmake solves this with a copy marked as a console
#program; this script does the same, so the output reaches the log and no window appears.

set -u

if [ "$#" -lt 3 ]; then
	echo "usage: run_dedicated.sh <build dir> <config> <log file> <openttd arguments...>" >&2
	exit 2
fi
build_dir=$1
config=$2
log=$3
shift 3

exe="$build_dir/$config/openttd.exe"
console="$build_dir/$config/openttd_console.exe"
if [ ! -f "$exe" ]; then
	echo "run_dedicated.sh: $exe does not exist; build $config first" >&2
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
		echo "run_dedicated.sh: editbin.exe not found; set EDITBIN" >&2
		exit 2
	fi
	cp "$exe" "$console" || exit 2
	#Git Bash would turn the /options into Windows paths, so path conversion is off for this call.
	MSYS_NO_PATHCONV=1 "$editbin" /nologo /subsystem:console "$(cygpath -w "$console")" || exit 2
	if [ "$(pe_subsystem "$console")" != "3" ]; then
		echo "run_dedicated.sh: $console is still not a console program" >&2
		exit 2
	fi
fi

cd "$build_dir" || exit 2
mkdir -p "$(dirname "$log")"
#misc shows paths and config, net the server start, sl loading and saving, sprite the graphics set, script the console scripts.
levels=${OTTD_DEBUG:-misc=3,net=3,sl=2,sprite=1,script=2}
started=$(date +%T)
timeout "${OTTD_TIMEOUT:-600}" "./$config/openttd_console.exe" -D -x -d "$levels" "$@" > "$log" 2>&1
status=$?
echo "run_dedicated.sh: $started-$(date +%T) exit $status, $(wc -l < "$log") log lines in $log"
exit $status
