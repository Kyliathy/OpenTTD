"""Freeze actual heavy onboard evidence after bounded native, ordinary and independent-reader proofs pass in both builds.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CC
Dated paid supply/descent reports and saves remain immutable inputs. Current native parts/work/history, rather than a report label or hypothetical budget, supply acceptance.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_onboard_service_checks-1.py")))
MALFORMED = runpy.run_path(str(Path(__file__).with_name("heavy_onboard_service_checks-2.py")))
AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
SOURCES = (
    "src/space_manifest.h", "src/space_manifest.cpp", "src/saveload/space_manifest_sl.cpp", "src/saveload/saveload.h",
    "src/space_mining_tuning.h", "src/space_mining_tuning.cpp", "src/space_vehicle.cpp", "src/space_order_actions.cpp",
    "src/fios.h", "src/saveload/cargopacket_sl.cpp", "src/saveload/vehicle_sl.cpp",
    "src/tests/heavy_onboard_service.cpp", "src/tests/heavy_onboard_service_native.cpp", "src/tests/CMakeLists.txt",
    "src/tests/manufacturing_fixture.h", "src/tests/assembly_native_fixture.h",
    "src/tests/multilayer/acceptance/heavy_supply_custody_checks-2.py", "src/tests/multilayer/acceptance/heavy_descent_checks-2.py",
    "src/tests/multilayer/acceptance/probe_role_checks-3.py",
) + tuple(f"src/tests/multilayer/acceptance/heavy_onboard_service_checks-{number}.py" for number in (1, 2, 3))


def qualify(saved, phase):
    """Read original saved service quantities/work independently of report labels and current active pools.

    @param saved Exact accepted current save identity.
    @param phase Required native operation/checkpoint.
    @return Actual original manifest scalar and physical part snapshot.
    """
    source = ROOT / saved["path"]; assert identity(source) == saved; version, raw = read_stream(source); assert version == 435
    row, compartments = MALFORMED["slots"](raw); quantities = []
    # Each exact SPSU compartment gets its quantity solely from original same-file native CAPA rows; no open-world inventory or cached declared bill can supply missing material.
    for slot in compartments:
        quantity = 0
        for position in slot:
            packet = int.from_bytes(raw[position:position + 4], "big"); offset = MALFORMED["PACKET_OFFSET"](raw, packet); quantity += int.from_bytes(raw[offset:offset + 2], "big")
        quantities.append(quantity)
    expected = {"prepared": 296, "partial": 259, "paused": 259, "resumed": 242, "last-tick": 1, "complete": 0,
                "ordered": 259, "ordered-complete": 0, "target-lost": 259, "lost-complete": 0, "premature": 0}
    complete = phase in ("complete", "ordered-complete", "lost-complete"); ticks = int.from_bytes(raw[row + 38:row + 40], "big")
    history = int.from_bytes(raw[row + 52:row + 60], "big"); allowance = raw[row + 40]; paused = raw[row + 41]; location = raw[row + 43]; pods = raw[row + 60]
    assert ticks == expected[phase] and raw[row + 42] == 0 and history == (1 if complete else 0) and paused == (1 if phase == "paused" else 0)
    assert allowance == (2 if complete else 1 if phase == "premature" else 0) and location == (1 if ticks else 0) and pods == (2 if phase == "premature" else 1)
    assert quantities == ([0, 0, 0] if complete else [0, 2, 2])
    return dict(craft=22, generation=8, remaining_ticks=ticks, services_completed=history, allowance=allowance, pending=0, paused=bool(paused), location=location, held_pods=pods, quantities=quantities)


def configuration(name, selectors, references):
    """Audit exact native, split ordinary/continuation and both independently paired refusal memberships.

    @param name Matching current build configuration.
    @param selectors Exact five current scope selectors in acceptance order.
    @param references Shared direct retained artifact identity audit.
    @return Qualified configuration index row and exact saved physical states.
    """
    reports = []; processes = {}; states = {}; phases = set(); edges = set(); faults = set(); snapshots = {}
    totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0, preview_assertions=0)
    # Every exact scope owns complete explicit membership; partial smoke runs or another successful branch cannot substitute a required paid-state or malformed-reader boundary.
    for scope, selector, expected in zip(("native", "reloads-main", "reloads-orders", "rejects-receipts", "rejects-joins"), selectors, (12, 7, 5, 6, 6)):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["scope"] == scope and selected["complete"] and len(selected["reports"]) == expected
        AUDIT["inspect_references"](identity(selector), references)
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path))
            AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = SHARED["read"](ROOT / child["path"]); assert native["configuration"] == name and child["path"] not in processes
                intended = scope.startswith("rejects-") and native["scenario"].startswith("heavy-onboard-full-"); assert native["exit_status"] == (1 if intended else 0)
                AUDIT["inspect_references"](native, references)
                if intended:
                    log = ROOT / "logs/space" / native["scenario"] / native["run_id"] / "heavy-onboard.log"; evidence = log.read_text(encoding="utf-8", errors="replace")
                    assert "Broken savegame" in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence; AUDIT["inspect_references"](identity(log), references)
                processes[child["path"]] = dict(report=child, intended_refusal=intended)
            if scope == "native":
                if proof["phase"] == "focused":
                    totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] == 14
                else:
                    assert proof["save_version"] == 435 and proof["procurement_acceptance"] is False and proof["retired_center_fixture"] == (proof["phase"] == "target-lost")
                    assert proof["phase"] not in phases; phases.add(proof["phase"]); states["native", proof["phase"], ""] = proof["after"]
                    snapshots[proof["phase"]] = qualify(proof["after"], proof["phase"]); totals["native_assertions"] += proof["assertions"]
            elif scope.startswith("reloads-"):
                assert proof["exact_owner_families"] == len(EXACT); next_phase = SHARED["read"](ROOT / proof["completion"]["path"])["phase"] if "completion" in proof else ""
                key = (proof["phase"], next_phase); assert key not in edges; edges.add(key); states[("ordinary",) + key] = proof["ordinary"]
                assert qualify(proof["ordinary"], proof["phase"]) == snapshots[proof["phase"]]
                if "resumed" in proof:
                    assert proof["resumed_input"] == proof["ordinary"]; states[("resumed",) + key] = proof["resumed"]; totals["resumed_assertions"] += proof["assertions"]
                    assert qualify(proof["resumed"], next_phase) == snapshots[next_phase]
            else:
                assert proof["kind"] not in faults; faults.add(proof["kind"]); totals["preview_assertions"] += proof["assertions"]
    assert phases == set(SHARED["PARENTS"]) and faults == set(MALFORMED["KINDS"])
    expected_edges = {(parent, child) for child, parent in SHARED["PARENTS"].items() if parent is not None} | {(phase, "") for phase in phases if phase not in SHARED["PARENTS"].values()}
    assert edges == expected_edges and len(states) == 31 and len(processes) == 56 and sum(row["intended_refusal"] for row in processes.values()) == 12
    return dict(configuration=name, save_version=435, reports=reports, selectors=[identity(path) for path in selectors], processes=list(processes.values()),
                ordinary_checkpoints=12, resumed_continuations=8, refusal_pairs=12, service_snapshots=snapshots, **totals), states


def main():
    """Freeze both current builds after original physical owners, service quantities and all independent clean reader boundaries agree."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=5, type=Path, required=True); parser.add_argument("--debug", nargs=5, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references)
    debug, right = configuration("Debug", [path.resolve() for path in args.debug], references); assert left.keys() == right.keys(); comparisons = 0
    # The same actual paid hull, parts/ages, consumed bills, native money/engine, finite resources and original pod/quota owners must converge across compilers and every actual own-resave branch.
    for key in left:
        before, after = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; actual original onboard heavy2RFMT/2ELEC/296work, interruptions, typed action and marked target-loss continuation. Retirement/reinstallation is a declared hook fixture on the same paid footprint, not new infrastructure acceptance. Inherited starting procurement remains unverified.",
                  configurations=[release, debug], exact_owner_families=len(EXACT), cross_configuration_comparisons=comparisons,
                  sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()), entry_indexes=[identity(ROOT / row[0]) for row in SHARED["ENTRIES"].values()],
                  procurement_acceptance=False, contained_service_acceptance=False,
                  copied_scope="Source/per-model service budget, original same-file CAPA compartment quantities and original reached/marked-lost action owner; no active-world parts or private timer. Contained service/legacy format and joined broader disclosures remain later9CCB/9CCC/9CD.")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(json.dumps(dict(index=identity(target), owners=len(EXACT), comparisons=comparisons)), flush=True)


if __name__ == "__main__":
    main()
