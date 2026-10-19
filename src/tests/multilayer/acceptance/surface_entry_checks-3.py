"""Verify the authentic starting company's original ordinary load/resave boundary.

@file surface_entry_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BB.3.
Both complete applications consume one exact real New Game source. No financial,
terrain, cargo, industry, vehicle or construction mutation is requested.
"""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import console_executable, run_native

sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import saved_table


def verify_revision_log(source, output):
    """Admit only the ordinary same-commit clean/modified-build revision record.

    @param source Original real-game save with its unchanged startup history.
    @param output Actual resave from the other complete application.
    Settings, cheat, GRF, money and gameplay changes cannot use this exception.
    """
    before, after = saved_table(source, b"GLOG"), saved_table(output, b"GLOG")
    assert set(after) == set(before) | {len(before)}
    # Preserve every original action and admit exactly one Load/Revision. A
    # general log exclusion could otherwise conceal settings or cheat grants.
    for action, fields in before.items():
        assert after[action] == fields
    appended = after[len(before)]
    assert appended["at"] == 1 and appended["action.count"] == 1 and appended["action[0].ct"] == 1
    assert appended["tick"] == saved_table(source, b"DATE")[0]["tick_counter"]
    assert appended["action[0].revision.count"] == 1
    for name, value in appended.items():
        if name.endswith(".count"):
            assert value == (1 if name in ("action.count", "action[0].revision.count") else 0), name
    original_revision = before[0]
    prefix = "action[0].revision[0].revision."
    old_text, new_text = [bytes.fromhex(row[prefix + "text"]).decode("ascii") for row in (original_revision, appended)]
    assert old_text[1:] == new_text[1:]
    assert old_text[0] in "gm" and new_text[0] in "gm"
    for name in ("newgrf", "slver"):
        assert appended[prefix + name] == original_revision[prefix + name]
    assert appended[prefix + "modified"] in (0, 1, 2)


def ordinary(configuration, source, name):
    """Pause and resave one immutable original world in the actual application.

    @param configuration Current complete ordinary game executable.
    @param source Exact original input, never replaced.
    @param name Isolated native operation name.
    @return Actual native output and exact child evidence.
    """
    context = create_run_context("space", "surface-entry-" + name)
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd.exe"
    executable = console_executable(context, application)
    original = identity(source)
    output = context.saves / "surface-entry-ordinary-440.sav"
    (context.runtime_dir / "scripts/game_start.scr").write_text(
        f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    log = context.logs / "ordinary.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), "-v", "null:ticks=2", "-s", "null", "-m", "null",
                            "-x", "-X", "-I", "original_dos", "-d", "sl=3", "-c", str(context.config_path), "-g", str(source)],
                            cwd=context.runtime_dir, stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and output.is_file(), log
    assert not re.search(r"(?:save failed|load failed|error loading|assertion failed)", terminal, re.I), log
    assert identity(source) == original
    before, after = chunks(source)[1], chunks(output)[1]
    changed = {key for key in before if before[key] != after.get(key)}
    # The two already-built ordinary applications have the same source commit
    # but different clean/modified revision metadata. Validate that exact native
    # append; every actual game/financial/clock owner remains byte-identical.
    if b"GLOG" in changed:
        verify_revision_log(source, output)
    assert before.keys() == after.keys() and changed <= {b"VIEW", b"GLOG"}, changed
    company, = saved_table(output, b"PLYR").values()
    assert company["money"] == company["current_loan"] == 100000
    assert company["cur_economy[0].income"] == 0 and not any(company["yearly_expenses"])
    assert not saved_table(output, b"CAPA")
    assert all(row["type"] in (4, 5) for row in saved_table(output, b"VEHS").values())
    proof = dict(source=original, output=identity(output), application=identity(application), terminal=identity(log),
                 unchanged_owner_families=len(before) - len(changed), presentation_changes=sorted(key.decode() for key in changed),
                 qualified_revision_append=b"GLOG" in changed,
                 money=company["money"], loan=company["current_loan"], packet_count=0, company_vehicle_count=0,
                 native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))])
    return output, proof


def main():
    """Verify one build's baseline without replaying any manufacturing/Space graph."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    first, first_proof = ordinary(args.configuration, source, "baseline-load")
    second, second_proof = ordinary(args.configuration, first, "baseline-resave")
    assert chunks(first) == chunks(second)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     source=identity(source), processes=[first_proof, second_proof],
                                     exact_second_resave=True, harness=identity(Path(__file__)),
                                     factory_or_launch_entry_complete=False), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
