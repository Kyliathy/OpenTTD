"""Freeze the original paid Dock service and actual Surface/Core return cycle in both configurations.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9DB
The joined437 partial Dock is a dated report/save boundary. Actual packet quantities, service bills/history and descent receipts qualify each state independently of printed totals.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_cycle_checks-1.py")))
FIELDS = runpy.run_path(str(Path(__file__).with_name("heavy_mining_runtime_checks-2.py")))
PACKETS = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-2.py")))
PARTS = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-3.py")))
TABLES = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-2.py")))
JOINED_AUDIT = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-3.py")))
AUDIT = SHARED["JOINED"]["AUDIT"]; PHYSICAL = SHARED["PHYSICAL"]
SCOPES = ("focused", "native-service", "native-surface", "native-orbital", "reload-service", "reload-surface", "reload-orbital")
SOURCES = ("src/orbital_service.cpp", "src/orbital_service.h", "src/utility_power.cpp", "src/utility_power.h","src/tests/CMakeLists.txt", "src/tests/heavy_cycle_native.cpp", "src/tests/assembly_native_fixture.h",
           "src/tests/multilayer/acceptance/heavy_cycle_checks-1.py", "src/tests/multilayer/acceptance/heavy_cycle_checks-2.py",
           "src/tests/multilayer/acceptance/heavy_joined_checks-1.py", "src/tests/multilayer/acceptance/heavy_joined_checks-2.py", "src/tests/multilayer/acceptance/heavy_joined_checks-3.py",
           "src/tests/multilayer/acceptance/heavy_mining_runtime_checks-2.py", "src/tests/multilayer/acceptance/heavy_descent_checks-2.py",
           "src/tests/multilayer/acceptance/heavy_contained_service_checks-3.py", "src/tests/multilayer/acceptance/heavy_contained_service_checks-4.py",
           "src/tests/multilayer/support/runner.py", "src/tests/multilayer/support/artifact_paths.py", "src/tests/test_artifact_paths.cpp",
           "src/space_vehicle.cpp", "src/space_vehicle.h", "src/space_mining_tuning.cpp", "src/space_mining_tuning.h",
           "src/space_orders.cpp", "src/space_approach.cpp", "src/space_manifest.cpp", "src/space_descent.cpp", "src/space_pod.cpp",
           "src/vehicle.cpp", "src/saveload/vehicle_sl.cpp", "src/saveload/space_descent_sl.cpp", "src/saveload/space_cargo_preview.cpp", "src/saveload/multilayer_sl.cpp")


def hull(raw, vehicle):
    """Read one actual hull's saved extraction and physical cargo rather than trusting a printed total.

    @param raw Original decompressed current native stream.
    @param vehicle Exact original paid VehicleID.
    @return Actual cargo capacity/quantity, partial work, lifetime extraction and native target.
    """
    fields = FIELDS["native_fields"](raw, vehicle)
    def scalar(name):
        """Read one actual native field.

        @param name Fully qualified original native scalar path.
        @return Its serialized unsigned value, after exact extent lookup.
        """
        position, width = fields[name]; return int.from_bytes(raw[position:position + width], "big")
    position, width = fields["space.common.cargo.packets"]; assert width % 4 == 0; quantity = 0; references = []
    # Native references encode ID+1. Every physically held packet must contribute its own positive CAPA quantity, independently of a cached/displayed hold count.
    for offset in range(position, position + width, 4):
        encoded = int.from_bytes(raw[offset:offset + 4], "big"); assert encoded > 0; packet = encoded - 1; count = PACKETS["packet_count_offset"](raw, packet)
        amount = int.from_bytes(raw[count:count + 2], "big"); assert amount > 0; quantity += amount; references.append(packet)
    assert len(set(references)) == len(references)
    return dict(held=quantity, packets=references, capacity=scalar("space.common.cargo_cap"), work=scalar("space.mission.work_ticks"),
                extracted=scalar("space.extracted_units"), target=scalar("space.mission.target"))


def surface_quantity(raw):
    """Count actual original launch-center ore through all saved native next-hop buckets.

    @param raw Exact decompressed current stream.
    @return Positive CAPA quantities held by original station4's NORE compartment.
    """
    rows = [row for row in TABLES["table_rows"](raw, b"STNN") if row["index"] == 4]; assert len(rows) == 1; quantity = 0; packets = []
    # All bucket occurrences belong to the original goods compartment; an invalid-destination bucket still owns real freight and cannot be omitted.
    for field in rows[0]["fields"]:
        if ".goods[16].cargo[" not in field["path"] or not field["path"].endswith(".second"):
            continue
        assert field["width"] == 4
        for offset in range(field["position"], field["end"], 4):
            encoded = int.from_bytes(raw[offset:offset + 4], "big"); assert encoded > 0; packet = encoded - 1; packets.append(packet)
            count = PACKETS["packet_count_offset"](raw, packet); amount = int.from_bytes(raw[count:count + 2], "big"); assert amount > 0; quantity += amount
    assert len(packets) == len(set(packets)); return quantity


def qualify(saved, phase):
    """Qualify actual original service bills, finite heavy output, shell custody and once-only quota.

    @param saved Exact current native/ordinary/own-resave identity.
    @param phase Required original gameplay checkpoint represented by this save.
    @return Independently parsed original hull, manifest, stock and contained job receipts.
    """
    path = ROOT / saved["path"]; assert identity(path) == saved; version, raw = read_stream(path); assert version == 437
    # Save437 adds only separate CDIM metadata; original SPST bytes retain the accepted436 framing and immutable role append.
    layout = PARTS["LAYOUT"]["structure_rows"](raw, 436); rows = layout["rows"]; assert set(rows) == {0, 1, 2, 3, 4}
    core, dock = rows[0], rows[4]; assert (dock["generation"], dock["kind"], dock["x"], dock["y"]) == (5, 7, 2, 4)
    service = phase in ("paused", "resumed"); descent = phase == "descent"; surface = phase in ("surface-recovered", "orbital-blocked", "orbital-returned")
    heavy, small = hull(raw, 22), hull(raw, 16)
    assert heavy["capacity"] == 40 and heavy["work"] == 0
    assert heavy["held"] == (40 if phase in ("surface-mined", "orbital-blocked") else 0)
    assert heavy["extracted"] == (80 if phase in ("paused", "resumed", "serviced") else 160 if phase.startswith("orbital-") else 120)
    assert small["held"] == small["extracted"] == small["work"] == 0 and small["capacity"] == 20
    allowance = 0 if service or phase == "orbital-returned" else 1 if surface else 2
    heavy_manifest = PARTS["manifest"](raw, 22, 8)
    assert heavy_manifest == dict(work=0, allowance=allowance, paused=0, pending=int(descent), history=int(not service), pods=int(service or phase in ("serviced", "surface-mined")), quantities=[0, 0, 0])
    assert PARTS["manifest"](raw, 16, 2) == dict(work=0, allowance=2, paused=0, pending=0, history=0, pods=2, quantities=[0, 1, 1])
    assert PARTS["manifest"](raw, 18, 4) == dict(work=0, allowance=2, paused=0, pending=0, history=int(not service), pods=0, quantities=[0, 0, 0])
    assert dock["history"] == (0 if service else 2) and core["history"] == 1; jobs = []
    # Each original reserved bill survives interrupted work intact; both canonical empty receipts prevent another service settlement from the same consumed input.
    for slot, job in enumerate(dock["jobs"]):
        ticks = (259 if phase == "paused" else 242) if slot == 0 else 135
        assert job["ticks"] == (ticks if service else 0) and job["paused"] == int(slot == 0 and phase == "paused")
        assert job["model"] == ((21 if slot == 0 else 20) if service else 65535)
        assert (job["craft"], job["generation"]) == (((22, 8) if slot == 0 else (18, 4)) if service else (2**20 - 1, 0))
        for part in job["parts"]:
            quantity, _ = PARTS["quantity"](raw, part["packet_offsets"]); assert quantity == part["quantity"] == ((2 if slot == 0 else 1) if service else 0)
        jobs.append((job["ticks"], job["model"], job["paused"]))
    stock = {}
    # Independently sum the original CAPA references in each module stock; native display totals alone cannot establish physical output custody.
    for site, row in rows.items():
        stock[site] = {}
        for cargo, part in row.get("stock", {}).items():
            quantity, _ = PARTS["quantity"](raw, part["packet_offsets"]); assert quantity == part["quantity"]; stock[site][cargo] = quantity
    assert stock[0].get(16, 0) == (80 if phase == "orbital-returned" else 40)
    surface_stock = surface_quantity(raw); assert surface_stock == (80 if surface else 40)
    if descent:
        _, _, _, row = PACKETS["descent"](raw); assert int.from_bytes(raw[row + 51:row + 53], "big") == 111
    else:
        assert chunks(path)[1][b"SPDJ"][3:][8:10] == b"\0\0"
    return dict(heavy=heavy, small=small, manifest=heavy_manifest, jobs=jobs, stock=stock, surface_stock=surface_stock)


def process_evidence(native, configuration, references):
    """Bind one actual current paid-cycle process to its executable and terminal log.

    @param native Original invocation process report.
    @param configuration Exact matching current test/product configuration.
    @param references Shared direct artifact identity audit.
    @return Actual terminal identity, including the original ordinary console routing proof.
    """
    if Path(native["executable"]["path"]).name == "openttd_console.exe":
        return JOINED_AUDIT["process_evidence"](native, configuration, references)
    assert ROOT / native["executable"]["path"] == ROOT / "build" / configuration / "openttd_test.exe"
    filename = "focused.log" if native["scenario"] == "heavy-cycle-focused" else "heavy-cycle.log"
    result = identity(ROOT / "logs/space" / native["scenario"] / native["run_id"] / filename); AUDIT["inspect_references"](result, references); return result


def configuration(name, selectors, references):
    """Audit exact native action membership and each actual own-resave next result.

    @param name Matching current configuration.
    @param selectors Seven completed scopes in explicit acceptance order.
    @param references Shared direct retained-identity audit; historical graphs stop at their dated boundary.
    @return Qualified configuration and actual checkpoints for cross-build comparison.
    """
    selectors = [p.resolve() for p in selectors]; states = {}; paths = {}; native_saves = {}; snapshots = {}; processes = {}; reports = []
    totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0)
    index, producer, source = SHARED["entry"](name)
    # Exact explicit current membership prevents duplicated cases, another configuration or an unrelated terminal artifact from supplying a missing original action.
    for scope, selector in zip(SCOPES, selectors, strict=True):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["complete"] and selected["scope"] == scope
        assert len(selected["reports"]) == (1 if scope == "focused" else len(SHARED["GROUPS"][scope.split("-", 1)[1]]))
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path)); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                assert child["path"] not in processes; native = SHARED["read"](ROOT / child["path"]); AUDIT["inspect_references"](native, references)
                assert native["configuration"] == name and native["exit_status"] == 0
                processes[child["path"]] = dict(report=child, terminal=process_evidence(native, name, references))
            phase = proof["phase"]
            if scope == "focused":
                totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] > 0
            elif scope.startswith("native-"):
                assert phase not in paths and proof["save_version"] == 437 and proof["procurement_acceptance"] is False; parent = SHARED["PARENTS"][phase]
                assert proof["predecessor"] == (producer if parent is None else paths[parent]) and proof["source"] == (source if parent is None else native_saves[parent])
                assert proof["entry_index"] == (index if parent is None else None)
                paths[phase] = identity(path); native_saves[phase] = proof["after"]; states["native", phase] = proof["after"]; snapshots[phase] = qualify(proof["after"], phase); totals["native_assertions"] += proof["assertions"]
            else:
                assert proof["proof"] == paths[phase] and proof["source"] == native_saves[phase]; states["ordinary", phase] = proof["ordinary"]
                assert qualify(proof["ordinary"], phase) == snapshots[phase]
                child = next((p for p, parent in SHARED["PARENTS"].items() if parent == phase), None)
                assert proof["completion"] == (paths[child] if child else None) and proof["expected"] == native_saves[child or phase]
                assert qualify(proof["resumed"], child or phase) == snapshots[child or phase]; states["resumed", phase] = proof["resumed"]; totals["resumed_assertions"] += proof["assertions"]
    assert set(paths) == set(SHARED["PARENTS"]) and len(states) == 24 and len(processes) == 25
    return dict(configuration=name, save_version=437, selectors=[identity(p) for p in selectors], reports=reports, processes=list(processes.values()), ordinary_checkpoints=8, own_resave_actions=7, own_resave_terminal_admissions=1, **totals), states


def main():
    """Publish paid-cycle acceptance only after both actual builds and every physical owner/configured metadata comparison agree."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=7, type=Path, required=True); parser.add_argument("--debug", nargs=7, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", args.release, references); debug, right = configuration("Debug", args.debug, references); assert left.keys() == right.keys(); comparisons = 0
    # The same original paid service and two-return cycle must converge across actual native, ordinary and own-resave actions; CDIM stays separate metadata rather than another physical inventory owner.
    for key in left:
        a, b = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in PHYSICAL:
            assert a[family] == b[family], (key, family); comparisons += 1
        assert a[b"CDIM"] == b[b"CDIM"], key
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9DB; actual original two-role paid Dock service, shared750J work, finite40t Surface descent and podless Core unload. Original quotas, reserved parts and last shell settle once; no additional starting fixture. Network/final heavy aggregate remains9DC; processors remain10-12.",
                  configurations=[release, debug], sources=[identity(ROOT / name) for name in SOURCES], direct_references=list(references.values()),
                  exact_owner_families=[f.decode("ascii") for f in PHYSICAL], cross_configuration_comparisons=comparisons, configured_metadata_comparisons=len(left), procurement_acceptance=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], comparisons, flush=True)


if __name__ == "__main__":
    main()
