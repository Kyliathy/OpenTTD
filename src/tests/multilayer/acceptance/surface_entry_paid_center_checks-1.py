"""Execute current original pure admission or paid center construction without moving cargo.

@file surface_entry_paid_center_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQK.
Native command accounting, allocation and generations remain authoritative.
Every stock carrier, useful service and original wagon retains its real owner.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_gateway_facility_ledger import TABLE, observe, facility
from surface_regional_ledger import lives

DIRECTORY = Path(__file__).resolve().parent
FIELDS = runpy.run_path(str(DIRECTORY / "surface_entry_chemical_checks-2.py"))["fields"]
REVISION = runpy.run_path(str(DIRECTORY / "surface_entry_checks-7.py"))["known_revision"]
PAUSE = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))["restore_pause"]


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual focused build. @param phase Pure center-query or paid center boundary.
    @param receipt Exact selected current predecessor. @param recover_run Successful original run to inspect without repetition.
    @return Native payment, allocation, geometry, complete material/service and process proof.
    """
    assert phase in ("center-query", "center")
    value = json.loads(receipt.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
    original = value["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-paid-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_gateway\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / f"build/{configuration}/openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_GATEWAY_PHASE=phase)
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-gateway]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=environment)
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "SURFACE_GATEWAY_COMPLETE " in terminal, log
    before, output = [context.saves / f"surface-gateway-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    initial, final = observe(before), observe(output)
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_GATEWAY_" + prefix + " ")]
            for name, prefix in dict(plan="PLAN", facility_plan="FACILITY_PLAN", paid="FACILITY_PAID", invoices="INVOICE").items()}
    plan, = rows["facility_plan"]; whole, = rows["plan"]
    assert whole["complete_capital"] and plan["anchor"] == 2890 and plan["rotation"] == 1 and plan["member"] == 0
    assert plan["center_cost"] == whole["center_quote"] and plan["target"] == whole["target"] == 1870000
    assert plan["credit"] == max(0, (plan["target"] - plan["cash"] + 9999) // 10000 * 10000)
    assert plan["ceiling"] == 5000000 and plan["debt"] + plan["credit"] <= plan["ceiling"]
    assert lives(before) == lives(output) and first[b"VEHS"] == last[b"VEHS"] and first[b"CAPA"] == last[b"CAPA"]
    old = dict(initial); new = dict(final)
    for key in ("company", "launch_facility"):
        old.pop(key); new.pop(key)
    assert old == new
    differences = [key.decode() for key in first if first[key] != last[key]]
    if phase == "center-query":
        assert first == last and initial == final and not rows["paid"] and not rows["invoices"]
    else:
        paid, = rows["paid"]; owned = final["launch_facility"]
        assert initial["launch_facility"]["center"] is None and owned["center"] == paid["station"]
        assert owned["generation"] == paid["generation"] == initial["launch_facility"]["next_center"]
        assert owned["member_generation"] == paid["member_generation"] == initial["launch_facility"]["next_member"]
        assert owned["next_center"] == owned["generation"] + 1 and owned["next_member"] == owned["member_generation"] + 1
        assert paid["cost"] == plan["center_cost"] == sum(row["cost"] for row in rows["invoices"])
        assert len(rows["invoices"]) == 3 and paid["cargo_moved"] == paid["work"] == 0
        assert final["company"]["cash"] == initial["company"]["cash"] + plan["credit"] - paid["cost"] == paid["cash"]
        assert final["company"]["debt"] == initial["company"]["debt"] + plan["credit"] == paid["debt"]
        old_stations, new_stations = TABLE(before, b"STNN"), TABLE(output, b"STNN")
        assert new_stations.keys() - old_stations.keys() == {owned["center"]}
        assert all(new_stations[key] == row for key, row in old_stations.items())
        # Native construction may rewrite only its twelve admitted footprint
        # cells. Preserve the entire other layers, canal and commercial routes.
        for key in (b"MAPT", b"MAPO", b"MAP2", b"M3LO", b"M3HI", b"MAP5", b"MAPE", b"MAP7", b"MAP8"):
            a, b = first[key][3:], last[key][3:]; stride = 2 if key in (b"MAP2", b"MAP8") else 1
            assert len(a) == len(b)
            assert {index // stride for index, (x, y) in enumerate(zip(a, b, strict=True)) if x != y} <= set(owned["cells"])
        assert first[b"MAPH"] == last[b"MAPH"]
        old_date, new_date = TABLE(before, b"DATE")[0], TABLE(output, b"DATE")[0]; rng = {"random_state[0]", "random_state[1]"}
        assert {k:v for k,v in old_date.items() if k not in rng} == {k:v for k,v in new_date.items() if k not in rng}
        state, other = [old_date[f"random_state[{index}]"] for index in (0, 1)]; mixed = other ^ 0x1234567F
        expected = [(state + ((mixed >> 7) | (mixed << 25)) + 1) & 0xFFFFFFFF, (((state >> 3) | (state << 29)) - 1) & 0xFFFFFFFF]
        assert [new_date[f"random_state[{index}]"] for index in (0, 1)] == expected
        assert set(differences) <= {"MAPT", "MAPO", "MAP2", "M3LO", "M3HI", "MAP5", "MAPE", "MAP7", "MAP8", "DATE", "CITY", "PLYR", "STNN", "LCTR", "LCHB", "LCOP", "MLMD"}
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert len(processes) == 1
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=final, source_pause=source_pause, changed_families=differences,
                retained_harness=identity(retained), native_processes=processes,
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_gateway.cpp"), identity(DIRECTORY / "surface_gateway_facility_ledger.py")])


def main():
    """Retain one actual selected pure or paid original center boundary without repeating successful work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("center-query", "center"), required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
