"""Freeze genuine pre-heavy contained Core service before its later job transport changes.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCB
Actual original packet/work owners and unchanged435 framing supply legacy acceptance, without relabelling a future-format job or inferring a lost sender role.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

SHARED = runpy.run_path(str(Path(__file__).with_name("legacy_contained_service_checks-1.py")))
PACKETS = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-2.py")))
MANIFEST = runpy.run_path(str(Path(__file__).with_name("heavy_supply_custody_checks-2.py")))
AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
SOURCES = ("src/orbital_service.h", "src/orbital_service.cpp", "src/orbital_station.h", "src/orbital_station.cpp", "src/saveload/space_structure_sl.cpp",
           "src/utility_power.h", "src/utility_power.cpp", "src/saveload/utility_power_sl.cpp", "src/space_manifest.h", "src/space_manifest.cpp",
           "src/saveload/saveload.h", "src/tests/legacy_contained_service_native.cpp", "src/tests/assembly_native_fixture.h", "src/tests/CMakeLists.txt",
           "src/tests/multilayer/acceptance/heavy_onboard_service_checks-1.py", "src/tests/multilayer/acceptance/heavy_supply_custody_checks-2.py",
           "src/tests/multilayer/acceptance/heavy_descent_checks-2.py") + tuple(f"src/tests/multilayer/acceptance/legacy_contained_service_checks-{number}.py" for number in (1, 2))


def qualify(saved, phase):
    """Inspect the actual original Core's unchanged first contained-job framing and real same-file CAPA quantities.

    @param saved Exact accepted current native/ordinary/resumed save identity.
    @param phase Required genuine old service state.
    @return Actual old51-byte active row and original paid-hull receipt snapshot.
    """
    source = ROOT / saved["path"]; assert identity(source) == saved; version, raw = read_stream(source); assert version == 435
    _, body, _, end = riff_location(raw, b"SPST"); position = body + 42; assert raw[position:position + 10] == bytes(2) + (1).to_bytes(8, "big"); position += 29
    # The original first Core's construction input extents precede its contained module. Follow their actual bounded counts instead of searching for matching job bytes in stock or text.
    for _ in range(2):
        count = int.from_bytes(raw[position + 4:position + 6], "big"); assert count <= 128; position += 6 + 6 * count
    assert raw[position] == 1; position += 1; assert int.from_bytes(raw[position:position + 2], "big") == 0; position += 4
    count = raw[position]; position += 1
    for _ in range(count):
        packets = int.from_bytes(raw[position + 3:position + 5], "big"); assert packets <= 64; position += 5 + 6 * packets
    history = int.from_bytes(raw[position + 9:position + 17], "big"); position += 17; row = position; assert row + 39 <= end
    craft = int.from_bytes(raw[row:row + 4], "big"); generation = int.from_bytes(raw[row + 4:row + 12], "big"); work = int.from_bytes(raw[row + 12:row + 14], "big")
    paused = raw[row + 14]; interval = int.from_bytes(raw[row + 15:row + 23], "big"); serial = int.from_bytes(raw[row + 23:row + 31], "big"); position += 31; quantities = []; refs = []
    # Every exact original reserved unit must also exist in this save's native packet table. Report aggregates and parts in the active world cannot finance this old job.
    for _ in range(2):
        quantity = int.from_bytes(raw[position:position + 2], "big"); count = int.from_bytes(raw[position + 2:position + 4], "big"); assert count <= 1; position += 4; actual = 0
        for _ in range(count):
            packet = int.from_bytes(raw[position:position + 4], "big"); refs.append(packet); offset = PACKETS["packet_count_offset"](raw, packet)
            actual += int.from_bytes(raw[offset:offset + 2], "big"); position += 6
        assert actual == quantity; quantities.append(quantity)
    complete = phase == "complete"; assert work == dict(prepared=148, partial=111, paused=111, complete=0)[phase] and paused == (1 if phase == "paused" else 0)
    assert serial == 0 and history == (2 if complete else 1) and quantities == ([0, 0] if complete else [1, 1])
    assert position - row == (39 if complete else 51) and len(set(refs)) == len(refs)
    assert (craft, generation) == ((2**20 - 1, 0) if complete else (18, 4)) and (interval == 0 if phase in ("prepared", "complete") else interval > 0)
    original = MANIFEST["manifest_row"](raw, 18, 4); assert int.from_bytes(raw[original + 38:original + 40], "big") == 0 and raw[original + 40] == 2 and int.from_bytes(raw[original + 52:original + 60], "big") == int(complete)

    def loose_parts(vehicle, generation):
        """Read a specific original hull's physical compartment quantities from this same saved world.

        @param vehicle Exact original native VehicleID.
        @param generation Exact paid hull incarnation.
        @return Three actual CAPA-joined compartment quantities.
        """
        manifest = MANIFEST["manifest_row"](raw, vehicle, generation); position = manifest + 61 + 10 * raw[manifest + 60]; result = []
        # The separate original small/heavy bills cannot be substituted for the Core's real reserved input; every retained reference still resolves to this same file's native packet quantity.
        for _ in range(3):
            count = raw[position]; assert count <= 128; position += 1; actual = 0
            for _ in range(count):
                packet = int.from_bytes(raw[position:position + 4], "big"); offset = PACKETS["packet_count_offset"](raw, packet)
                actual += int.from_bytes(raw[offset:offset + 2], "big"); position += 6
            result.append(actual)
        return result

    assert loose_parts(16, 2) == [0, 1, 1] and loose_parts(22, 8) == [0, 2, 2] and loose_parts(18, 4) == [0, 0, 0]
    return dict(source_version=435, site=0, site_generation=1, craft=craft, generation=generation, remaining_ticks=work, paused=bool(paused), last_work_interval=interval,
                services_at_start=serial, core_services=history, quantities=quantities, packet_references=refs, row_bytes=position - row)


def configuration(name, selectors, references):
    """Audit complete genuine old-format native/ordinary/own-resave membership for one actual build.

    @param name Matching actual configuration.
    @param selectors Exact native and reload selectors in that order.
    @param references Shared current retained-identity audit.
    @return Qualified configuration index and exact saved physical states.
    """
    reports = []; processes = {}; states = {}; snapshots = {}; totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0)
    # Explicit complete membership excludes failed diagnostic runs and unrelated accepted branches; each resumed operation consumes its own ordinary resave.
    for scope, selector, expected in zip(("native", "reloads"), selectors, (5, 4)):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["scope"] == scope and selected["complete"] and len(selected["reports"]) == expected
        AUDIT["inspect_references"](identity(selector), references)
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path))
            AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = SHARED["read"](ROOT / child["path"]); assert native["configuration"] == name and native["exit_status"] == 0 and child["path"] not in processes
                AUDIT["inspect_references"](native, references); processes[child["path"]] = child
            phase = proof["phase"]
            if scope == "native":
                if phase == "focused":
                    totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] == 13
                else:
                    assert proof["save_version"] == 435 and proof["procurement_acceptance"] is False and proof["production_or_wire_changed"] is False
                    assert phase not in snapshots; snapshots[phase] = qualify(proof["after"], phase); states["native", phase] = proof["after"]; totals["native_assertions"] += proof["assertions"]
            else:
                assert proof["exact_owner_families"] == len(EXACT); assert qualify(proof["ordinary"], phase) == snapshots[phase]; states["ordinary", phase] = proof["ordinary"]
                if "resumed" in proof:
                    next_phase = SHARED["read"](ROOT / proof["completion"]["path"])["phase"]; assert proof["resumed_input"] == proof["ordinary"]
                    assert qualify(proof["resumed"], next_phase) == snapshots[next_phase]; states["resumed", phase] = proof["resumed"]; totals["resumed_assertions"] += proof["assertions"]
    assert set(snapshots) == set(SHARED["PHASES"]) and len(states) == 11 and len(processes) == 12
    return dict(configuration=name, save_version=435, reports=reports, selectors=[identity(path) for path in selectors], processes=list(processes.values()),
                ordinary_checkpoints=4, resumed_continuations=3, legacy_service=states["native", "partial"], legacy_receipt=snapshots["partial"], **totals), states


def main():
    """Freeze genuine current435 old contained jobs only after all original physical owners converge in both actual configurations."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=2, type=Path, required=True); parser.add_argument("--debug", nargs=2, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references)
    debug, right = configuration("Debug", [path.resolve() for path in args.debug], references); assert left.keys() == right.keys(); comparisons = 0
    # Original packets/ages, exact common work/history, native money and every paid hull/infrastructure owner must match across both compilers and actual own-resave continuations.
    for key in left:
        before, after = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCB; genuine435 old one-part/148tick Core job before heavy contained-job changes. Original paid Core stock funds deployed20 Vehicle18/generation4; both miners' loose parts remain original. Inherited procurement remains unverified.",
                  configurations=[release, debug], exact_owner_families=len(EXACT), cross_configuration_comparisons=comparisons,
                  sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()), entry_index=identity(ROOT / "logs/heavy-descent-verification.json"),
                  procurement_acceptance=False, production_or_wire_changed=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(json.dumps(dict(index=identity(target), owners=len(EXACT), comparisons=comparisons)), flush=True)


if __name__ == "__main__":
    main()
