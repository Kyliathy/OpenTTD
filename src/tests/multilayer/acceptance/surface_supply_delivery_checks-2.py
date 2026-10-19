"""Ordinary game restoration and focused refusals for physically bought cargo.

@file surface_supply_delivery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.2.
The input is an exact accepted driven receipt. No money, packet, receiver stock,
vehicle pose or construction work is supplied by this runner.
"""

import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import console_executable, run_native

sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import saved_table

FILES = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-5.py")))
MALFORMED = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-6.py")))
GUARDS = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-8.py")))
FAMILIES = (b"CAPA", b"STNN", b"VEHS", b"INDY", b"SSOR", b"GCSH", b"GCSD", b"GCBH", b"GCBD",
            b"GRSH", b"GRSD", b"GEOS", b"GVNT", b"LPWH", b"LPWK", b"UPLH", b"UPLD", b"TUPH",
            b"TUPD", b"TUVH", b"TUVD", b"MAPS", b"MAPT", b"MAPH", b"MAPO", b"MAP2", b"M3LO",
            b"M3HI", b"MAP5", b"MAPE", b"MAP7", b"MAP8", b"PLYR", b"DATE")


def checked(reference):
    """Require unchanged exact original evidence.

    @param reference Explicit path/digest, optionally including its byte count.
    @return Existing original repository path.
    """
    path = ROOT / reference["path"]
    assert identity(path) == {key: reference[key] for key in ("path", "sha256")}, path
    return path


def compare(first, second, initial=False):
    """Compare original native owners at a paused or deterministic boundary.

    @param first Exact original native source.
    @param second Actual ordinary resave/continuation.
    @param initial Permit only documented initial company-face/RNG normalization.
    @return Number of compared owner families.
    """
    before, after = chunks(first)[1], chunks(second)[1]
    if initial:
        compare_initial_metadata(first, second)
    # A first load may initialize the reduced scenario's missing manager face.
    # Its shared verifier checks every other company/clock field individually.
    for family in FAMILIES:
        if initial and family in (b"PLYR", b"DATE"):
            continue
        assert family in before and before[family] == after.get(family), (family, first, second)
    return len(FAMILIES)


def compare_initial_metadata(first, second):
    """Validate only the ordinary loader's declared initial presentation/catalogue work.

    @param first Reduced scenario's immutable original save.
    @param second Its actual first ordinary game resave.
    All money, clock and existing engine identities remain exact. The ordinary
    content loader may append its absent Space engine definition without a hull.
    """
    before, after = saved_table(first, b"PLYR"), saved_table(second, b"PLYR")
    assert before.keys() == after.keys()
    missing_face = False
    # A missing fixture face can receive normal presentation defaults. Every
    # financial/control field remains individually equal to the original save.
    for company, fields in before.items():
        unset = fields["face_style"] == "" and fields["face"] == 0
        missing_face |= unset
        for name, value in fields.items():
            if unset and name in ("face", "face_style"):
                continue
            assert value == after[company][name], (company, name, "changed initial company")
    clocks, restored = saved_table(first, b"DATE"), saved_table(second, b"DATE")
    assert clocks.keys() == restored.keys()
    # Only native face initialization consumes these RNG draws; it cannot
    # advance game/economy/calendar clocks or authorize another purchased unit.
    for clock, fields in clocks.items():
        for name, value in fields.items():
            if missing_face and name in ("random_state[0]", "random_state[1]"):
                continue
            assert value == restored[clock][name], (name, "changed initial clock")
    engines, rebuilt = saved_table(first, b"EIDS"), saved_table(second, b"EIDS")
    assert engines.keys() <= rebuilt.keys()
    for engine, fields in engines.items():
        assert rebuilt[engine] == fields, (engine, "changed existing engine identity")
    # The old ground fixture omits the cost-free native Space prototype. Its
    # normal required-content append is permitted; exact VEHS/CAPA comparisons
    # below still prohibit a new paid hull, physical carrier or cargo packet.
    for engine in rebuilt.keys() - engines.keys():
        assert rebuilt[engine]["type"] == 6, (engine, "unexpected non-Space engine append")


def ordinary(configuration, source, name, advance=False):
    """Load an original world through the complete game executable.

    @param configuration Independently built ordinary game.
    @param source Immutable actual paid/physical checkpoint.
    @param name Explicit isolated process scenario.
    @param advance Earn3,000 real game-loop calls instead of a paused resave.
    @return Actual output and original process evidence.
    """
    context = create_run_context("space", "surface-supply-delivery-" + name)
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd.exe"
    executable = console_executable(context, application)
    original = identity(source)
    output = context.saves / "ordinary-440.sav"
    startup = "pause\nset savegame_format lzma\n"
    if advance:
        startup += "unpause\n"
        config = context.config_path.read_bytes()
        assert b"[gui]" not in config
        context.config_path.write_bytes(config + b"\n[gui]\nautosave_on_exit = true\n")
    else:
        startup += f'save "{output.with_suffix("").as_posix()}"\nquit\n'
    (context.runtime_dir / "scripts/game_start.scr").write_text(startup, encoding="utf-8")
    log = context.logs / "ordinary.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), "-v", "null:ticks=3000" if advance else "null:ticks=2",
                            "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-d", "sl=3",
                            "-c", str(context.config_path), "-g", str(source)], cwd=context.runtime_dir,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    if advance:
        generated = context.runtime_dir / "save/autosave/exit.sav"
        assert generated.is_file(), log
        output.write_bytes(generated.read_bytes())
        context.write_report("exit-save.json", Path(__file__).relative_to(ROOT).as_posix(), [output], [source],
                             configuration, native_writer="VideoDriver_Null::MainLoop -> DoExitSave")
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and output.is_file(), log
    assert not re.search(r"(?:save failed|load failed|error loading|assertion failed)", terminal, re.I), log
    assert identity(source) == original
    proof = dict(source=original, output=identity(output), application=identity(application), terminal=identity(log),
                 advance=advance, native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))])
    return output, proof


def observe(path):
    """Read original financial, packet and consumed-construction owners.

    @param path Actual ordinary native output.
    @return Exact relevant scalar values without a second runtime stock owner.
    """
    orders = FILES["orders"](path)
    companies = saved_table(path, b"PLYR")
    company, = companies.values()
    packets = saved_table(path, b"CAPA")
    collectors = saved_table(path, b"GCBD")
    risers = saved_table(path, b"GRSD")
    industries = saved_table(path, b"INDY")
    accepted = {}
    # Each machinery binding selects its original actual native Industry.
    # Histories remain authoritative after complete material consumption.
    for binding in (*collectors.values(), *risers.values()):
        industry = industries[binding["industry"]]
        for index in range(industry["accepted.count"]):
            prefix = f"accepted[{index}]"
            cargo = industry[prefix + ".cargo"]
            # An ordinary3,000-frame continuation can cross month end. Sum
            # retained native history slots so rollover cannot appear as lost
            # delivery or permit a duplicate bill in this independent check.
            received = sum(industry.get(f"{prefix}.history[{month}].accepted", 0)
                           for month in range(industry.get(prefix + ".history.count", 0)))
            accepted[cargo] = accepted.get(cargo, 0) + received
    # Every surviving physical packet must retain purchased purpose and zero
    # feeder credit; fulfilled history grants no permission to recreate stock.
    for packet in packets.values():
        assert packet["source_type"] == 4 and packet["feeder_share"] == 0, packet
    return dict(orders=orders, money=company["money"], expenses=company["yearly_expenses"],
                income=company["cur_economy[0].income"],
                packet_units=sorted(packet["count"] for packet in packets.values()),
                accepted=accepted, collectors=list(collectors.values()), risers=list(risers.values()))


def ordinary_group(configuration, driven):
    """Verify paid-pending, travelling and full receivers through ordinary game loads.

    @param configuration Actual selected ordinary executable.
    @param driven Accepted original native receipt with exact six outputs.
    @return Original process boundaries and independently decoded native accounting.
    """
    inputs = {Path(item["path"]).name: checked(item) for item in driven["outputs"]}
    results = []
    # Each state has a distinct custody owner: pending station commitments,
    # actual in-flight wagons, then consumed bills with retained overflow.
    for state in ("paid-pending", "loaded-deep", "full-refusal-return"):
        source = inputs[f"delivery-{state}-440.sav"]
        first, first_proof = ordinary(configuration, source, state + "-load")
        compare(source, first, initial=True)
        second, second_proof = ordinary(configuration, first, state + "-resave")
        compare(first, second)
        continued, continued_proof = ordinary(configuration, first, state + "-continue", True)
        repeated, repeated_proof = ordinary(configuration, second, state + "-resaved-continue", True)
        compare(continued, repeated)
        before, after = observe(first), observe(continued)
        assert after["income"] == before["income"] == 0
        # Property is native ExpensesType::Property (ordinal6). Running trains
        # may incur costs; restoring or dispatching cannot repeat procurement.
        assert after["expenses"][6] == before["expenses"][6] == 6470
        assert [row["state"] for row in after["orders"]["rows"]] == [1, 1, 1, 1]
        for old, new in zip(before["orders"]["rows"], after["orders"]["rows"]):
            assert {k: v for k, v in old.items() if k not in ("state", "issued_packet")} == {
                k: v for k, v in new.items() if k not in ("state", "issued_packet")}
        if state == "paid-pending":
            assert after["packet_units"] == [3, 5, 9, 13]
            assert all(row["materials_committed"] == 0 for row in after["collectors"] + after["risers"])
        else:
            assert after["packet_units"] == [1, 1, 1, 1]
            assert sorted(after["accepted"].values()) == [2, 4, 8, 12]
            assert all(row["materials_committed"] == 1 and row["work_ticks"] == 296
                       for row in after["collectors"] + after["risers"])
        results.append(dict(state=state, owners=len(FAMILIES), before=before, after=after,
                            processes=[first_proof, second_proof, continued_proof, repeated_proof]))
        print(configuration, state, "ordinary load/resave/continue passed", flush=True)
    return results


def refusal_group(configuration, driven):
    """Refuse narrowly malformed copies of the actual paid pending world.

    @param configuration Independently linked full/copied native loader.
    @param driven Accepted native source receipt.
    @return Exact malformed inputs and both original native refusal processes.
    """
    source, = [checked(item) for item in driven["outputs"] if item["path"].endswith("delivery-paid-pending-440.sav")]
    context = create_run_context("space", "surface-supply-delivery-refusals")
    results = []
    # These faults challenge price, pending physical custody and original
    # station authority in this actual collector/riser world, without replaying
    # the completed broad persistence-corruption matrix.
    for fault in ("price", "pending-packet", "missing-station-field"):
        malformed = MALFORMED["malformed"](context, source, fault)
        reports = [FILES["execute"](configuration, phase, source, malformed)
                   for phase in ("preview-refusal", "full-refusal")]
        results.append(dict(fault=fault, source=identity(source), malformed=identity(malformed), reports=reports))
        print(configuration, fault, "full/copied refusal passed", flush=True)
    return results


def main():
    """Run one explicit bounded acceptance family and publish its exact membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("ordinary", "refusals", "guards"), required=True)
    parser.add_argument("--driven", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    driven_path = (ROOT / args.driven).resolve()
    driven = json.loads(driven_path.read_text(encoding="utf-8"))
    assert driven["configuration"] == args.configuration and driven["phase"] == "drive"
    checked(driven["executable"])
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    if args.phase == "ordinary":
        evidence = ordinary_group(args.configuration, driven)
    elif args.phase == "refusals":
        evidence = refusal_group(args.configuration, driven)
    else:
        evidence = GUARDS["execute"](args.configuration)
    proof = dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                 driven=identity(driven_path), evidence=evidence, harness=identity(Path(__file__)),
                 final_surface_factory_acceptance=False, unassisted=False)
    target.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
