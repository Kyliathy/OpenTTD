"""Freeze actual heavy Core service and genuine old-job upgrades after both builds agree.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCC
Native packet quantities, original job receipts and exact process membership supply acceptance.
Actual paid Dock construction and integrated heavy/network disclosures remain later groups.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-1.py")))
MALFORMED = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-2.py")))
LAYOUT = SHARED["LAYOUT"]; AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
PACKETS = MALFORMED["PACKETS"]; MANIFEST = MALFORMED["MANIFEST"]
SCOPES = ("native-main", "native-failures", "native-legacy", "reloads-main", "reloads-failures", "reloads-legacy", "rejects-receipts", "rejects-joins")
SOURCES = (
    "src/orbital_service.h", "src/orbital_service.cpp", "src/orbital_station.h", "src/orbital_station.cpp", "src/space_structure.cpp",
    "src/saveload/space_structure_sl.cpp", "src/saveload/saveload.h", "src/space_approach.h", "src/space_approach.cpp",
    "src/space_mining_tuning.h", "src/space_mining_tuning.cpp", "src/space_manifest.h", "src/space_manifest.cpp",
    "src/utility_power.h", "src/utility_power.cpp", "src/saveload/utility_power_sl.cpp",
    "src/saveload/cargopacket_sl.cpp", "src/saveload/space_manifest_sl.cpp", "src/saveload/space_order_sl.cpp",
    "src/tests/heavy_contained_service.cpp", "src/tests/heavy_contained_service_native.cpp", "src/tests/CMakeLists.txt",
    "src/tests/manufacturing_fixture.h", "src/tests/assembly_native_fixture.h",
    "src/tests/multilayer/acceptance/heavy_onboard_service_checks-1.py",
    "src/tests/multilayer/acceptance/heavy_supply_custody_checks-2.py", "src/tests/multilayer/acceptance/heavy_descent_checks-2.py",
) + tuple(f"src/tests/multilayer/acceptance/heavy_contained_service_checks-{number}.py" for number in (1, 2, 3, 4))


def quantity(raw, offsets):
    """Join exact retained references to this file's actual native packet counts.

    @param raw Original decompressed native stream.
    @param offsets Absolute original packet-ID field positions.
    @return Physical units and original packet IDs, without consulting any active pool.
    """
    result = 0; references = []
    # Every actual reference contributes its own same-file quantity; a declared reservation cannot replace a missing or short packet.
    for position in offsets:
        packet = int.from_bytes(raw[position:position + 4], "big"); offset = PACKETS["packet_count_offset"](raw, packet)
        result += int.from_bytes(raw[offset:offset + 2], "big"); references.append(packet)
    assert len(set(references)) == len(references); return result, references


def manifest(raw, vehicle, generation):
    """Read an exact original hull's unchanged manifest and physical compartments.

    @param raw Original decompressed native stream.
    @param vehicle Original paid native VehicleID.
    @param generation Original paid hull incarnation.
    @return Native allowance/history/work and actual same-file loose input quantities.
    """
    row = MANIFEST["manifest_row"](raw, vehicle, generation); position = row + 61 + 10 * raw[row + 60]; quantities = []
    # Each compartment follows its own original shell/count framing; other hulls' parts cannot finance this hull's retained bill.
    for _ in range(3):
        count = raw[position]; assert count <= 128; position += 1
        quantities.append(quantity(raw, [position + 6 * index for index in range(count)])[0]); position += 6 * count
    return dict(work=int.from_bytes(raw[row + 38:row + 40], "big"), allowance=raw[row + 40], paused=raw[row + 41], pending=raw[row + 42],
                history=int.from_bytes(raw[row + 52:row + 60], "big"), pods=raw[row + 60], quantities=quantities)


def qualify(saved, phase):
    """Independently qualify original Core bill/work, real parts and genuine unknown legacy role.

    @param saved Exact accepted current native, ordinary or resumed save identity.
    @param phase Required actual checkpoint or next operation.
    @return Original physical and immutable service snapshot for exact continuation comparison.
    """
    source = ROOT / saved["path"]; assert identity(source) == saved; version, raw = read_stream(source); assert version == 436
    layout = LAYOUT["structure_rows"](raw, version); assert set(layout["rows"]) == {0, 1, 2, 3}; core = layout["rows"][0]
    assert (core["generation"], core["owner"], core["kind"], core["x"], core["y"]) == (1, 0, 6, 4, 4)
    job = core["jobs"][0]; expected = dict(prepared=296, partial=259, paused=259, resumed=242, **{"last-tick": 1, "restarted": 296,
                    "outage": 259, "restored": 242, "sender-lost": 259, "legacy-upgrade": 111})
    ticks = expected.get(phase, 0); legacy = phase.startswith("legacy-"); active_legacy = phase == "legacy-upgrade"; active_heavy = ticks != 0 and not legacy
    complete = phase in ("complete", "legacy-complete"); orphan = phase in ("sender-lost", "orphan-cancelled"); premature = phase in ("pretopup", "premature")
    assert job["ticks"] == ticks and job["model"] == (21 if active_heavy else 65535) and job["serial"] == 0
    assert (job["craft"], job["generation"]) == ((22, 8) if active_heavy else (18, 4) if active_legacy else (2**20 - 1, 0))
    assert job["paused"] == int(phase == "paused") and core["history"] == (2 if complete else 1)
    # Fresh/restarted work has earned no interval; empty receipts erase the former interval rather than authorizing another final settlement.
    if phase in ("prepared", "restarted") or ticks == 0:
        assert job["interval"] == 0
    elif active_legacy:
        assert job["interval"] == 22047
    else:
        assert job["interval"] > 0
    parts = []; references = []
    # Both complete immutable input bills stay in the contained owner until its last opportunity; cancellation and completion leave no residual service-owned packet.
    for part in job["parts"]:
        actual, refs = quantity(raw, part["packet_offsets"]); assert actual == part["quantity"] == (2 if active_heavy else 1 if active_legacy else 0)
        parts.append(actual); references.extend(refs)
    assert len(set(references)) == len(references)
    # The original world has only one Core service owner; every other contained slot must remain canonical empty, including the Assembly's non-service slots.
    for row in layout["rows"].values():
        for other in row["jobs"]:
            if other is job:
                continue
            assert (other["craft"], other["generation"], other["ticks"], other["paused"], other["interval"], other["serial"], other["model"]) == (2**20 - 1, 0, 0, 0, 0, 0, 65535)
            assert all(part["quantity"] == part["count"] == 0 for part in other["parts"])
    stock = {}
    # Original content revision 3 slots 9/16/22/25 are GOOD/NORE/RFMT/ELEC in the frozen paid entry. Every stock aggregate must independently match actual original CAPA references.
    for cargo, part in core["stock"].items():
        stock[cargo] = quantity(raw, part["packet_offsets"])[0]; assert stock[cargo] == part["quantity"]
    remaining = 0 if legacy else 3 if phase in ("cancelled", "orphan-cancelled", "pretopup", "premature") else 1
    assert {cargo: amount for cargo, amount in stock.items() if amount} == {9: 2, 16: 40, **({22: remaining, 25: remaining} if remaining else {})}
    small = manifest(raw, 16, 2); original = manifest(raw, 18, 4)
    assert small["quantities"] == [0, 1, 1] and original["quantities"] == [0, 0, 0]
    assert original["work"] == 0 and original["allowance"] == 2 and original["history"] == int(phase == "legacy-complete")
    heavy = None
    if orphan:
        _, body, _, end = riff_location(raw, b"SPSU"); count = raw[body]; position = body + 1
        # A lost sender must actually be absent from the complete saved manifest family; a report flag cannot hide a replacement original-ID hull.
        for _ in range(count):
            row = position; assert raw[row:row + 12] != (22).to_bytes(4, "big") + (8).to_bytes(8, "big"); position += 61 + 10 * raw[row + 60]
            for _ in range(3):
                position += 1 + 6 * raw[position]
        assert position == end
    else:
        heavy = manifest(raw, 22, 8)
        assert heavy["work"] == heavy["pending"] == heavy["paused"] == 0 and heavy["history"] == int(phase == "complete")
        assert heavy["allowance"] == (2 if phase == "complete" else 1 if premature else 0) and heavy["pods"] == (2 if premature else 1)
        assert heavy["quantities"] == ([0, 2, 2] if legacy else [0, 0, 0])
        if active_heavy:
            assert raw[LAYOUT["approach_slot_offset"](raw)] == 0
    return dict(craft=job["craft"], generation=job["generation"], model=job["model"], remaining_ticks=ticks, paused=bool(job["paused"]),
                last_work_interval=job["interval"], services_at_start=job["serial"], core_services=core["history"], quantities=parts,
                packet_references=references, core_stock=stock, heavy=heavy, original_builder=original, small=small)


def process_evidence(native, name, references):
    """Bind each actual process to its matching build bytes and retained terminal log.

    @param native Original invocation-owned process report.
    @param name Matching current build configuration.
    @param references Shared retained artifact identity audit.
    @return Exact retained terminal-log identity.
    """
    executable = ROOT / native["executable"]["path"]; ordinary = executable.name == "openttd_console.exe"
    product = ROOT / "build" / name / ("openttd.exe" if ordinary else "openttd_test.exe")
    if ordinary:
        encoded = bytearray(product.read_bytes()); header = int.from_bytes(encoded[60:64], "little")
        assert encoded[header:header + 4] == b"PE\0\0"; encoded[header + 92:header + 94] = (3).to_bytes(2, "little")
        assert executable.read_bytes() == encoded
    else:
        assert executable == product
    filename = "ordinary.log" if ordinary else "focused.log" if native["scenario"] == "heavy-contained-focused" else "heavy-contained.log"
    log = ROOT / "logs/space" / native["scenario"] / native["run_id"] / filename
    result = identity(log); AUDIT["inspect_references"](result, references); return result


def configuration(name, selectors, references):
    """Audit every explicit native, ordinary, own-resave and independently paired refusal scope.

    @param name Matching actual current build configuration.
    @param selectors Exact eight completed scope selectors in acceptance order.
    @param references Shared retained identity audit, stopping at dated producer boundaries.
    @return Qualified configuration row and all original saved physical states.
    """
    reports = []; processes = {}; states = {}; phases = set(); edges = set(); faults = set(); snapshots = {}
    totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0, preview_assertions=0)
    # Exact complete scope membership excludes diagnostic partial runs, duplicate branches and proofs from another executable configuration.
    for scope, selector, expected in zip(SCOPES, selectors, (7, 6, 4, 9, 6, 4, 6, 6)):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["scope"] == scope and selected["complete"] and len(selected["reports"]) == expected
        AUDIT["inspect_references"](identity(selector), references)
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path))
            AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = SHARED["read"](ROOT / child["path"]); assert native["configuration"] == name and child["path"] not in processes
                intended = scope.startswith("rejects-") and native["scenario"].startswith("heavy-contained-full-"); assert native["exit_status"] == (1 if intended else 0)
                AUDIT["inspect_references"](native, references); terminal_log = process_evidence(native, name, references)
                if intended:
                    evidence = (ROOT / terminal_log["path"]).read_text(encoding="utf-8", errors="replace")
                    assert "Broken savegame" in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence
                processes[child["path"]] = dict(report=child, log=terminal_log, intended_refusal=intended)
            if scope.startswith("native-"):
                if proof["phase"] == "focused":
                    totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] == 20
                else:
                    phase = proof["phase"]; assert proof["save_version"] == 436 and proof["procurement_acceptance"] is False and proof["sender_destruction_fixture"] == (phase == "sender-lost")
                    assert phase not in phases; phases.add(phase); states["native", phase, ""] = proof["after"]
                    snapshots[phase] = qualify(proof["after"], phase); totals["native_assertions"] += proof["assertions"]
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
    assert edges == expected_edges and len(states) == 48 and len(processes) == 73 and sum(row["intended_refusal"] for row in processes.values()) == 12
    return dict(configuration=name, save_version=436, reports=reports, selectors=[identity(path) for path in selectors], processes=list(processes.values()),
                ordinary_checkpoints=19, resumed_continuations=13, refusal_pairs=12, service_snapshots=snapshots, **totals), states


def main():
    """Freeze exact current Core and genuine upgrade acceptance after all physical owners agree across both builds."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=8, type=Path, required=True); parser.add_argument("--debug", nargs=8, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references)
    debug, right = configuration("Debug", [path.resolve() for path in args.debug], references); assert left.keys() == right.keys(); comparisons = 0
    # Original parts/ages, native utility/money, paid hulls and saved work must converge at every actual native, ordinary and own-resave action boundary across compilers.
    for key in left:
        before, after = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCC; the original paid Core consumes the heavy miner's two RFMT and two ELEC over 296 shared work opportunities of 750 J. Genuine 435 unknown-role upgrades preserve their original bill. Explicit sender destruction leaves its remaining original shell drifting; inherited starting procurement is unverified. Actual paid Dock acceptance remains with 9CCCC, joined disclosures/network with 9CD/9D.",
                  configurations=[release, debug], exact_owner_families=len(EXACT), cross_configuration_comparisons=comparisons,
                  sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()),
                  entry_indexes=[identity(ROOT / row[0]) for row in SHARED["SHARED"]["ENTRIES"].values()] + [identity(ROOT / SHARED["LEGACY_INDEX"][0])],
                  procurement_acceptance=False, paid_core_service_acceptance=True, genuine_legacy_upgrade_acceptance=True, paid_dock_service_acceptance=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(json.dumps(dict(index=identity(target), owners=len(EXACT), comparisons=comparisons)), flush=True)


if __name__ == "__main__":
    main()
