"""Observe genuine ordinary newgame generation before paid ground readiness.

@file normal_ground_newgame.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No development fixture,manual world initialization or starting inventory is loaded.
"""

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from runner import current_run_context, start_native
from construction_provider_return import identity
from save_tables import chunks, read_stream, saved_table

TAG = "[.normal-ground-newgame]"
SOURCES = ("src/tests/normal_ground_newgame_fixture.h", "src/tests/tbm_reuse.cpp", "src/openttd.cpp",
           "src/genworld.cpp", "src/company_cmd.cpp", "src/multilayer_content.cpp", "src/geology_generation.cpp", "src/landscape.cpp")


def run_one(configuration):
    """Generate one real native GUI player world under declared legal options.

    @param configuration Actual Debug or RelWithDebInfo native test application.
    @return Strict initial-owner/pure-write evidence;full bootstrap remains unclaimed.
    """
    context = current_run_context("acceptance", "normal-ground")
    context.prepare_runtime()
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    # Replace the tiny graphics-loader config with this exact declared normal
    # game's legal settings. Seed/date go through the normal application CLI.
    context.config_path.write_text(
        "; Owner: r2mlvs16-integrated-acceptance-and-balance.md6B; archi-map-layers.md.\n"
        "[game_creation]\nmap_x = 7\nmap_y = 7\nlandscape = temperate\ngameplay_profile = multilayer_industry\n"
        "[gui]\npause_on_newgame = true\nautosave_on_exit = false\n", encoding="utf-8")
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6B; archi-map-layers.md.\nnormal_ground_start\n", encoding="utf-8")
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"newgame-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, TAG], cwd=ROOT / "build", env=dict(os.environ),
                                 stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                                 startupinfo=launch)
            assert child.wait(timeout=110) == 0, log
    finally:
        if child is not None and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait(timeout=5)
        assert identity(executable) == binary
    output = log.read_text(encoding="utf-8", errors="replace")
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
    assert counts and int(counts[1]) > 0 and int(counts[2]) == 1, log
    rows = re.findall(r"NORMAL_GROUND_START ([^\r\n]+)", output)
    assert len(rows) == 1, log
    owners = {key: int(value) for key, value in (field.split("=", 1) for field in rows[0].split())}
    assert (owners["seed"], owners["width"], owners["height"], owners["year"]) == (420, 128, 128, 1980)
    assert owners["cash"] == owners["loan"] > 0
    assert owners["opened"] == 0 and owners["solid"] > 0
    assert owners["fleet"] == 0
    assert owners["quote"] > 0 and owners["deposits"] >= 10 and owners["vents"] > 0
    saves = {name: context.saves / name for name in ("normal-start.sav", "normal-start-repeat.sav")}
    transports = {}
    for name, path in saves.items():
        version, stream = read_stream(path)
        assert version == 434
        transports[name] = {row["name"]: row["raw"] for row in chunks(stream)}
        assert len(transports[name]) == 126
    assert transports["normal-start.sav"] == transports["normal-start-repeat.sav"], "Native starting-world observation changed saved owners"
    assert saved_table(saves["normal-start.sav"], b"DATE")[0]["tick_counter"] == owners["tick"]
    registry = saved_table(saves["normal-start.sav"], b"NGRF")
    assert len(registry) == 1
    activated = []
    for record in registry.values():
        # Read the ordinary saved registry,not a test-only activation table.
        # Resolve only this invocation's actual repository distribution.
        filename = bytes.fromhex(record["filename"]).decode("utf-8")
        package = (context.runtime_dir / "newgrf" / filename).resolve()
        assert package.is_relative_to((context.runtime_dir / "newgrf").resolve())
        assert list(hashlib.md5(package.read_bytes()).digest()) == record["ident.md5sum"]
        activated.append({"registry": record, "file": identity(package)})
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6B;actual ordinary -g/-G/-t application generation,real normal company/loan/profile/package,solid untouched rock and pure quoted entrance;not full bootstrap",
              "classification": "normal_start_foundation", "bootstrap_claim": False, "shortcuts": [],
              "configuration": configuration, "tag": TAG, "controller": identity(__file__), "binary": binary,
              "sources": [identity(ROOT / name) for name in SOURCES], "config": identity(context.config_path),
              "startup": identity(startup), "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1},
              "children_reaped": child is not None and child.poll() is not None,
              "starting_owners": owners, "saves": {name: identity(path) for name, path in saves.items()},
              "activated_content": activated,
              "content": identity(context.runtime_dir / "baseset/openttd.grf"), "native_version": 434,
              "remaining": "Paid normal financing/construction/real hauler/readiness/access and complete original S1/S2 loops6C/6D;old injected fixtures remain isolated boundaries."}
    target = context.output("reports", f"normal-start-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "starting_owners": owners}), flush=True)
    return target


def main():
    """Run one isolated native starting-world foundation below the two-minute case bound."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    run_one(args.config)


if __name__ == "__main__":
    main()
