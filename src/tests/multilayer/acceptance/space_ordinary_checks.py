#!/usr/bin/env python3
"""Load/resave or continue an immutable demo through the ordinary game executable.

Owner: docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31.
@see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 8: the native executable resolves on Linux single-configuration builds too.
Uses space_demo_checks.ordinary's native null-driver/exit-save pattern while
keeping all transient files under test-cache/space-fixes and logs/space-fixes.
There is no test observer, assigned world state, private simulation clock or
fixture callback in this application. Each process has a110-second deadline.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import time

from space_fixes_checks import ROOT, TEMPLATES, identity, native_executable


def main():
    """Run one bounded ordinary application using only disposable configuration.

    @return Zero after actual native save/continuation and immutable-input checks.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--mode", choices=("load", "advance"), default="load")
    parser.add_argument("--run", required=True)
    args = parser.parse_args()
    source = (ROOT / args.source).resolve()
    # Existing repository-owned worlds alone supply assets and saved progress;
    # a user save/configuration cannot be overwritten by this acceptance path.
    if not source.is_file() or not source.is_relative_to(ROOT) or not re.fullmatch(r"[a-z0-9-]+", args.run):
        raise ValueError("select an immutable repository source and a unique lowercase run identity")
    original = identity(source)
    output = ROOT / "test-cache/space-fixes" / args.run
    output.mkdir(parents=True, exist_ok=False)
    runtime = output / "runtime"
    runtime.mkdir()
    # The ordinary application gets original installed assets in its own data
    # home. No configuration or save is written beside the retained source.
    for folder in ("baseset", "newgrf"):
        shutil.copytree(ROOT / "build" / folder, runtime / folder)
    (runtime / "lang").mkdir()
    for language in ("english.lng", "english_US.lng"):
        shutil.copy2(ROOT / "build/lang" / language, runtime / "lang" / language)
    config = (TEMPLATES / "play.cfg").read_text(encoding="utf-8-sig")
    enabled = "true" if args.mode == "advance" else "false"
    # Native configuration deliberately repeats AI slot names. Preserve that
    # exact structure rather than round-tripping it through a generic INI map.
    # Only the disposable GUI setting controls the ordinary driver's exit save.
    setting = "autosave_on_exit = " + enabled + "\n"
    section = re.search(r"(?m)^\[gui\]\s*\n", config)
    if section:
        config = re.sub(r"(?m)^autosave_on_exit\s*=.*\n?", "", config)
        section = re.search(r"(?m)^\[gui\]\s*\n", config)
        config = config[:section.end()] + setting + config[section.end():]
    else:
        config += "\n[gui]\n" + setting
    path = runtime / "ordinary.cfg"
    path.write_text(config, encoding="utf-8")
    (runtime / "scripts").mkdir()
    saved = output / "ordinary-current.sav"
    commands = "pause\nset savegame_format lzma\n"
    if args.mode == "advance":
        commands += "unpause\n"
    else:
        commands += f'save "{saved.with_suffix("").as_posix()}"\nquit\n'
    (runtime / "scripts/game_start.scr").write_text(commands, encoding="utf-8")
    executable = native_executable(args.configuration, "openttd")
    log = ROOT / "logs/space-fixes" / (args.run + ".log")
    ticks = 3000 if args.mode == "advance" else 2
    started = time.monotonic()
    with log.open("w", encoding="utf-8") as sink:
        result = subprocess.run([str(executable), "-v", f"null:ticks={ticks}", "-b", "null", "-s", "null", "-m", "null",
                                 "-x", "-X", "-I", "original_dos", "-d", "misc=3,sl=3", "-c", str(path), "-g", str(source)],
                                cwd=runtime, stdout=sink, stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ))
    if args.mode == "advance":
        exit_save = runtime / "save/autosave/exit.sav"
        assert exit_save.is_file(), log
        shutil.copy2(exit_save, saved)
    assert result.returncode == 0 and saved.is_file(), log
    assert not re.search(r"(?:save failed|load failed|error loading|assertion failed)", log.read_text(errors="replace"), re.I), log
    assert identity(source) == original, "immutable source changed"
    report = {"_documentation": "r2sff1-space-fixes-features-1.md Group31 ordinary application", "source": original,
              "executable": identity(executable), "config": identity(path), "saved": identity(saved), "mode": args.mode,
              "exit_code": result.returncode, "elapsed_seconds": round(time.monotonic() - started, 3), "driver_calls": ticks,
              "log": log.relative_to(ROOT).as_posix(), "native_observer": False}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
