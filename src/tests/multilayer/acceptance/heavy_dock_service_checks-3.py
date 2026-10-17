"""Freeze actual paid Dock construction and both original heavy/legacy berth orderings.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC
Native packets, original paid receipts and exact process membership supply acceptance. Declared Surface fuel remains a starting-content fixture, with procurement unverified.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_dock_service_checks-1.py")))
MALFORMED = runpy.run_path(str(Path(__file__).with_name("heavy_dock_service_checks-2.py")))
CORE_AUDIT = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-3.py")))
LAYOUT = SHARED["CORE"]["LAYOUT"]; AUDIT = SHARED["CORE"]["AUDIT"]; EXACT = SHARED["EXACT"]
SCOPES = ("native-producer", "native-heavy", "native-legacy", "reloads-material", "reloads-frame", "reloads-heavy-start", "reloads-heavy-end",
          "reloads-legacy-start", "reloads-legacy-end", "rejects-berths", "rejects-inputs")
SOURCES = (
    "src/orbital_service.h", "src/orbital_service.cpp", "src/orbital_station.h", "src/orbital_station.cpp", "src/space_structure.cpp",
    "src/space_structure_cmd.h", "src/space_approach.h", "src/space_approach.cpp", "src/space_manifest.h", "src/space_manifest.cpp",
    "src/space_mining_tuning.h", "src/space_mining_tuning.cpp", "src/multilayer_space_content.cpp", "src/utility_power.h", "src/utility_power.cpp",
    "src/saveload/space_structure_sl.cpp", "src/saveload/space_manifest_sl.cpp", "src/saveload/space_order_sl.cpp", "src/saveload/cargopacket_sl.cpp",
    "src/saveload/utility_power_sl.cpp", "src/saveload/saveload.h", "src/tests/assembly_native_fixture.h", "src/tests/CMakeLists.txt",
    "src/tests/heavy_dock_service_native.cpp", "src/tests/heavy_dock_work_native.cpp", "src/tests/heavy_contained_service.cpp",
    "src/tests/multilayer/acceptance/heavy_descent_checks-2.py", "src/tests/multilayer/acceptance/heavy_supply_custody_checks-2.py",
    "src/tests/multilayer/acceptance/heavy_onboard_service_checks-1.py",
) + tuple(f"src/tests/multilayer/acceptance/heavy_contained_service_checks-{number}.py" for number in (1, 3, 4)) + tuple(
    f"src/tests/multilayer/acceptance/heavy_dock_service_checks-{number}.py" for number in (1, 2, 3))


def construction(raw, row):
    """Read one actual paid site's construction receipt and same-file physical inputs.

    @param raw Exact decompressed native stream.
    @param row Bounded original structure layout from the versioned SPST walker.
    @return Paid capital, earned work, consumed and physically held quantities, plus optional actual module activation receipt.
    """
    start = row["start"]; position = start + 29; consumed = []; held = []
    # Each original material keeps its own consumed receipt and native packet references; a report label cannot replace the actual complete construction bill.
    for _ in range(2):
        spent = int.from_bytes(raw[position:position + 2], "big"); amount = int.from_bytes(raw[position + 2:position + 4], "big"); count = int.from_bytes(raw[position + 4:position + 6], "big")
        assert count <= 128; position += 6; actual, _ = CORE_AUDIT["quantity"](raw, [position + 6 * index for index in range(count)]); assert actual == amount
        consumed.append(spent); held.append(actual); position += 6 * count
    present = raw[position]; assert present <= 1
    return dict(capital=int.from_bytes(raw[start + 19:start + 23], "big"), work=int.from_bytes(raw[start + 23:start + 27], "big"),
                consumed=consumed, held=held, module_activation=int.from_bytes(raw[position + 1:position + 3], "big") if present else None)


def stock(raw, row):
    """Read actual sparse module stock without accepting cached aggregate quantities alone.

    @param raw Exact original native stream.
    @param row Original module row and bounded sparse stock fields.
    @return Nonempty native cargo-slot quantities joined to this file's actual CAPA rows.
    """
    result = {}
    # Every actual cargo bucket funds only its own stock; zero or unrelated active-world inventory cannot supply a missing native unit.
    for cargo, part in row.get("stock", {}).items():
        amount, _ = CORE_AUDIT["quantity"](raw, part["packet_offsets"]); assert amount == part["quantity"]
        if amount:
            result[cargo] = amount
    return result


def qualify(saved, phase):
    """Independently qualify the original paid Dock, exact immutable bills and both physical berth roles.

    @param saved Exact accepted current native, ordinary or own-resave identity.
    @param phase Required actual construction or service checkpoint.
    @return Complete qualified original construction/service snapshot for exact continuation comparison.
    """
    source = ROOT / saved["path"]; assert identity(source) == saved; version, raw = read_stream(source); assert version == 436
    rows = LAYOUT["structure_rows"](raw, version)["rows"]; producer = phase in SHARED["PRODUCER"]; before_dock = phase in ("inspected", "starting-fuel")
    assert set(rows) == ({0, 1, 2, 3} if before_dock else {0, 1, 2, 3, 4}); core, unused = rows[0], rows[2]
    assert core["generation"] == 1 and core["history"] == 1 and unused["generation"] == 3 and unused["phase"] == (0 if before_dock else 2)
    unused_bill = construction(raw, unused); assert unused_bill["capital"] == 120000 and unused_bill["work"] == 0 and unused_bill["consumed"] == unused_bill["held"] == [0, 0]
    assert stock(raw, core) == {9: 2, 16: 40, **({22: 1, 25: 1} if producer else {})}
    small = CORE_AUDIT["manifest"](raw, 16, 2); heavy = CORE_AUDIT["manifest"](raw, 22, 8); builder = CORE_AUDIT["manifest"](raw, 18, 4)
    other_builder = CORE_AUDIT["manifest"](raw, 19, 5); rocket = CORE_AUDIT["manifest"](raw, 15, 1)
    rocket_row = CORE_AUDIT["MANIFEST"]["manifest_row"](raw, 15, 1); rocket["fuel_burned"] = int.from_bytes(raw[rocket_row + 44:rocket_row + 52], "big")
    assert small["quantities"] == [0, 1, 1] and other_builder["quantities"] == [0, 0, 0] and other_builder["history"] == 1
    assert rocket["quantities"] == [0, 0, 0] and rocket["pods"] == 0
    assert heavy["work"] == heavy["pending"] == heavy["paused"] == builder["work"] == builder["pending"] == 0 and heavy["pods"] == 1
    assert builder["allowance"] == 2 and builder["quantities"] == [0, 0, 0]
    result = dict(unused_blueprint=unused_bill, core_stock=stock(raw, core), heavy=heavy, builder=builder, other_builder=other_builder, small=small, rocket=rocket)
    dock = None
    if before_dock:
        assert heavy["history"] == 0 and heavy["allowance"] == 0 and heavy["quantities"] == [0, 2, 2] and builder["history"] == 0
    else:
        dock = rows[4]; assert (dock["generation"], dock["kind"], dock["owner"], dock["x"], dock["y"]) == (5, 7, 0, 2, 4)
        bill = construction(raw, dock); assert bill["capital"] == 160000
        expected_work = 0 if phase in ("blueprint", "steel") else 222 if phase == "goods" else 444
        assert bill["work"] == expected_work and dock["phase"] == (0 if expected_work < 444 else 1)
        assert bill["consumed"] == ([8, 12] if expected_work == 444 else [0, 0])
        assert bill["held"] == ([8, 0] if phase == "steel" else [8, 12] if phase == "goods" else [0, 0])
        result["dock_construction"] = bill
        if producer:
            assert heavy["history"] == 0 and heavy["allowance"] == 0 and heavy["quantities"] == [0, 2, 2] and builder["history"] == 0
        # Activation creates the contained owner; the paid completed frame remains inert. Every later service checkpoint must retain that real activated module and its original source connection.
        if phase in ("activating", "ready") or not producer:
            assert bill["module_activation"] == (37 if phase == "activating" else 0)
            facility = dock["facility"]; assert int.from_bytes(raw[facility:facility + 4], "big") == 2**32 - 1
            source_offset = dock["jobs"][0]["end"] + 4; source_ref = (int.from_bytes(raw[source_offset:source_offset + 2], "big"), int.from_bytes(raw[source_offset + 2:source_offset + 10], "big"))
            assert source_ref == ((65535, 0) if phase == "activating" else (0, 1)); result["dock_source"] = source_ref
        else:
            assert bill["module_activation"] is None
    active_jobs = set(); job_snapshots = []
    if not producer:
        role, step = phase.split("-", 1); assert role in ("heavy", "legacy"); models = (21, 20) if role == "heavy" else (20, 21); totals = tuple(296 if model == 21 else 148 for model in models)
        expected = {"prepared": totals, "partial": (totals[0] - 37, totals[1]), "paused": (totals[0] - 37, totals[1] - 37),
                    "outage": (totals[0] - 37, totals[1] - 37), "restored": (totals[0] - 54, totals[1] - 37), "first-last": (1, totals[1] - 37),
                    "first-complete": (0, totals[1] - 46), "second-last": (0, 1), "complete": (0, 0), "cancelled": (0, 0)}[step]
        completed = 2 if step == "complete" else 1 if step in ("first-complete", "second-last") else 0; references = []
        assert dock["history"] == completed and stock(raw, dock) == ({22: 3, 25: 3} if step == "cancelled" else {})
        assert heavy["quantities"] == [0, 0, 0]
        # Each physical slot keeps its original role, complete CAPA-funded bill and service serial until its own last opportunity; the other slot cannot borrow them.
        for slot, job in enumerate(dock["jobs"]):
            model, ticks = models[slot], expected[slot]; vehicle, generation = (22, 8) if model == 21 else (18, 4); units = (2 if model == 21 else 1) if ticks else 0
            assert job["ticks"] == ticks and job["model"] == (model if ticks else 65535) and job["serial"] == 0 and job["paused"] == int(slot == 0 and step == "paused")
            assert (job["craft"], job["generation"]) == ((vehicle, generation) if ticks else (2**20 - 1, 0))
            quantities = []; packet_references = []
            for part in job["parts"]:
                amount, refs = CORE_AUDIT["quantity"](raw, part["packet_offsets"]); assert amount == part["quantity"] == units; references.extend(refs); quantities.append(amount); packet_references.append(refs)
            if ticks:
                active_jobs.add(job["start"])
                assert (job["interval"] == 0) == (step == "prepared" or (slot == 1 and step == "partial"))
            else:
                assert job["interval"] == 0
            job_snapshots.append(dict(craft=job["craft"], generation=job["generation"], model=job["model"], work=ticks, paused=job["paused"], interval=job["interval"], serial=job["serial"], quantities=quantities, packet_references=packet_references))
            manifest = heavy if model == 21 else builder; finished = slot < completed
            assert manifest["history"] == int(finished) and manifest["allowance"] == (2 if model == 20 or finished else 0)
            offset = LAYOUT["approach_slot_offset"](raw, vehicle, generation); assert raw[offset] == slot
            fields = MALFORMED["PACKETS"]["NATIVE_FIELDS"](raw, vehicle)
            # Exact saved native positions must match their own shared half-tile Dock slot; a queued target or coincident other hull cannot stand in for physical arrival.
            for field, value in (("space.position.x", 2 * 65536 + slot * 32768), ("space.position.y", 6 * 65536), ("space.mission.phase", 4)):
                position, width = fields[field]; assert int.from_bytes(raw[position:position + width], "big") == value
        assert len(set(references)) == len(references); result["dock_jobs"] = job_snapshots; result["dock_history"] = completed; result["dock_stock"] = stock(raw, dock)
    # All other contained records remain canonical empty, including the original Core's separate service slot and the Assembly's non-service owners.
    for row in rows.values():
        for job in row["jobs"]:
            if job["start"] in active_jobs:
                continue
            assert (job["craft"], job["generation"], job["ticks"], job["paused"], job["interval"], job["serial"], job["model"]) == (2**20 - 1, 0, 0, 0, 0, 0, 65535)
            assert all(part["quantity"] == part["count"] == 0 for part in job["parts"])
    return result


def process_evidence(native, name, references):
    """Bind every actual Dock process to its current build bytes and retained terminal log.

    @param native Exact invocation-owned process report.
    @param name Matching current actual build configuration.
    @param references Shared retained artifact identity audit.
    @return Actual terminal-log identity, including original shared focused and ordinary cases.
    """
    # Ordinary console copies and shared focused contracts retain their established log/PE proof; actual Dock observers must use their own current executable and terminal log.
    if native["scenario"] == "heavy-contained-focused" or Path(native["executable"]["path"]).name == "openttd_console.exe":
        return CORE_AUDIT["process_evidence"](native, name, references)
    assert ROOT / native["executable"]["path"] == ROOT / "build" / name / "openttd_test.exe"
    log = ROOT / "logs/space" / native["scenario"] / native["run_id"] / "heavy-dock.log"; result = identity(log)
    AUDIT["inspect_references"](result, references); return result


def configuration(name, selectors, references):
    """Audit complete original paid construction, both berth branches and independent reader membership.

    @param name Matching actual build configuration.
    @param selectors Exact eleven completed scope selectors in acceptance order.
    @param references Shared retained artifact identity audit, stopping at immutable dated handoffs.
    @return Qualified configuration row and every original saved physical state.
    """
    reports = []; processes = {}; states = {}; phases = set(); edges = set(); faults = set(); snapshots = {}; paths = {}; native_saves = {}
    totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0, preview_assertions=0)
    original_report, original_save = SHARED["CORE"]["entry"](name, "prepared")
    # Exact scope and predecessor membership excludes incomplete smoke runs, duplicated phases and borrowed parents from another original role or configuration.
    for scope, selector, expected in zip(SCOPES, selectors, (9, 10, 10, 4, 5, 5, 6, 5, 6, 4, 4)):
        selected = SHARED["CORE"]["read"](selector); assert selected["configuration"] == name and selected["scope"] == scope and selected["complete"] and len(selected["reports"]) == expected
        AUDIT["inspect_references"](identity(selector), references)
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["CORE"]["read"](path); assert proof["configuration"] == name; reports.append(identity(path))
            AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = SHARED["CORE"]["read"](ROOT / child["path"]); assert native["configuration"] == name and child["path"] not in processes
                intended = scope.startswith("rejects-") and native["scenario"].startswith("heavy-dock-full-"); assert native["exit_status"] == (1 if intended else 0)
                AUDIT["inspect_references"](native, references); terminal_log = process_evidence(native, name, references)
                if intended:
                    evidence = (ROOT / terminal_log["path"]).read_text(encoding="utf-8", errors="replace")
                    assert "Broken savegame" in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence
                processes[child["path"]] = dict(report=child, log=terminal_log, intended_refusal=intended)
            if scope.startswith("native-"):
                if proof["phase"] == "focused":
                    totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] == 20
                else:
                    phase = proof["phase"]; assert phase not in phases and proof["save_version"] == 436 and proof["procurement_acceptance"] is False
                    parent = SHARED["PARENTS"][phase]
                    assert proof["predecessor"] == (original_report if parent is None else paths[parent]) and proof["source"] == (original_save if parent is None else native_saves[parent])
                    paths[phase] = identity(path); native_saves[phase] = proof["after"]; phases.add(phase); states["native", phase, ""] = proof["after"]
                    snapshots[phase] = qualify(proof["after"], phase); totals["native_assertions"] += proof["assertions"]
                    if phase == "inspected":
                        assert proof["inspected_stock"] == dict(steel=12, goods=52, surface_fuel=0, rocket_fuel=0)
                    elif phase == "starting-fuel":
                        assert proof["declared_surface_fuel_fixture"] is True and proof["fuel_added"] == 30 and proof["fuel_retained"] == 0
            elif scope.startswith("reloads-"):
                phase = proof["phase"]; assert proof["exact_owner_families"] == len(EXACT) and proof["proof"] == paths[phase] and proof["source"] == native_saves[phase]
                next_phase = SHARED["CORE"]["read"](ROOT / proof["completion"]["path"])["phase"] if "completion" in proof else ""
                key = (phase, next_phase); assert key not in edges; edges.add(key); states[("ordinary",) + key] = proof["ordinary"]
                assert qualify(proof["ordinary"], phase) == snapshots[phase]
                if "resumed" in proof:
                    assert proof["resumed_input"] == proof["ordinary"] and proof["completion"] == paths[next_phase]
                    states[("resumed",) + key] = proof["resumed"]; totals["resumed_assertions"] += proof["assertions"]; assert qualify(proof["resumed"], next_phase) == snapshots[next_phase]
            else:
                kind = proof["kind"]; assert kind not in faults and proof["proof"] == paths[MALFORMED["CASES"][kind]] and proof["source"] == native_saves[MALFORMED["CASES"][kind]]
                faults.add(kind); totals["preview_assertions"] += proof["assertions"]
    assert phases == set(SHARED["PARENTS"]) and faults == set(MALFORMED["CASES"])
    expected_edges = {(parent, child) for child, parent in SHARED["PARENTS"].items() if parent is not None} | {(phase, "") for phase in phases if phase not in SHARED["PARENTS"].values()}
    assert edges == expected_edges and len(states) == 86 and len(processes) == 103 and sum(row["intended_refusal"] for row in processes.values()) == 8
    initial_burned = snapshots["inspected"]["rocket"]["fuel_burned"]
    # Both actual material flights burn their original fifteen-unit round-trip bill once. Later construction/service cannot mint or reburn a rocket allowance.
    for phase, snapshot in snapshots.items():
        flights = 0 if phase in ("inspected", "starting-fuel", "blueprint") else 1 if phase == "steel" else 2
        assert snapshot["rocket"]["fuel_burned"] == initial_burned + 15 * flights
        if phase not in SHARED["PRODUCER"]:
            role = phase.split("-", 1)[0]; prepared = snapshots[role + "-prepared"]
            # Each active job keeps the exact original reserved packet IDs through pause/outage and partial work, rather than replacing spent material with a counter or another hull's parts.
            for slot, job in enumerate(snapshot["dock_jobs"]):
                if job["work"]:
                    assert job["packet_references"] == prepared["dock_jobs"][slot]["packet_references"]
    return dict(configuration=name, save_version=436, reports=reports, selectors=[identity(path) for path in selectors], processes=list(processes.values()),
                ordinary_checkpoints=31, resumed_continuations=27, refusal_pairs=8, dock_snapshots=snapshots, **totals), states


def main():
    """Freeze exact original Dock acceptance only after physical owners agree across both completed current builds."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=11, type=Path, required=True); parser.add_argument("--debug", nargs=11, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references)
    debug, right = configuration("Debug", [path.resolve() for path in args.debug], references); assert left.keys() == right.keys(); comparisons = 0
    # Original material/age/share, consumed bills, exact shared work, paid hulls, native money and utility owners must agree at every real native/ordinary/own-resave boundary across compilers.
    for key in left:
        before, after = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC; actual original rocket/builder construction of a separately paid Dock and original heavy/construction service in both precise berth orderings. Original empty Storage blueprint cancellation retains its paid capital without refund. Thirty separately declared Surface fuel units fund two actual material flights; no orbital material, completed module, work or utility source is granted. Procurement remains unverified; joined disclosures/network remain 9CD/9D.",
                  configurations=[release, debug], exact_owner_families=len(EXACT), cross_configuration_comparisons=comparisons,
                  sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()),
                  entry_indexes=[identity(ROOT / path) for path in ("logs/heavy-descent-verification.json", "logs/heavy-contained-service-verification.json", "logs/heavy-onboard-service-verification.json")],
                  procurement_acceptance=False, declared_surface_fuel_added=30, paid_dock_construction_acceptance=True, paid_dock_service_acceptance=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(json.dumps(dict(index=identity(target), owners=len(EXACT), comparisons=comparisons)), flush=True)


if __name__ == "__main__":
    main()
