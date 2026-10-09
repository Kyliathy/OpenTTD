"""Launch repository multi-layer sessions with isolated native data and logs.

@file native_launch.py Shared implementation for the shell launch entry points.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 5.
@see docs/zz-reach2/architecture/logging/archi-logging.md.
"""

import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import contained
from runner import current_run_context, console_executable, run_native


def main():
    """Materialize one declared native profile and run the requested session.

    @return Native exit status; evidence and runtime copies remain repository-local.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--dedicated", action="store_true")
    parser.add_argument("build", type=Path)
    parser.add_argument("configuration", choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("log", type=Path)
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    context = current_run_context(os.environ.get("OTTD_TEST_FEATURE", "acceptance"),
                                  os.environ.get("OTTD_TEST_SCENARIO", "dedicated" if args.dedicated else "manual-game"))
    executable = contained(args.build.resolve() / args.configuration / "openttd.exe", ROOT / "build")
    console = console_executable(context, executable)
    log = context.output("logs", args.log.name)
    arguments = args.arguments
    # Explicit caller configuration is materialized by run_native; sessions
    # without one use the shared fixture instead of the caller's directory.
    if "-c" not in arguments:
        arguments = ["-c", str(context.config_path), *arguments]
    if "-X" not in arguments:
        arguments = ["-X", *arguments]
    flags = ["-D", "-x"] if args.dedicated else []
    timeout = int(os.environ.get("OTTD_TIMEOUT", "600")) if args.dedicated else None
    with log.open("wb") as sink:
        result = run_native(context, [console, *flags, "-d", os.environ.get("OTTD_DEBUG", "misc=3,sl=2,net=2"), *arguments],
                            cwd=context.runtime_dir, stdout=sink, stderr=subprocess.STDOUT, timeout=timeout,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=args.configuration))
    print(f"Native session exit={result.returncode}, log={log.relative_to(ROOT)}", flush=True)
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
