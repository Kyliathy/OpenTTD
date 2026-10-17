"""Freeze actual original two-miner work/lease/upkeep and own-resave evidence in both configurations.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9D
The joined437 input is a dated report/save boundary. This audit independently reads actual cargo/mission/lease receipts and binds current binaries/terminal logs without reopening older implementation graphs.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_role_checks-1.py")))
FIELDS = runpy.run_path(str(Path(__file__).with_name("heavy_mining_runtime_checks-2.py")))
PACKETS = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-2.py")))
PARTS = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-3.py")))
JOINED_AUDIT = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-3.py")))
AUDIT = SHARED["JOINED"]["AUDIT"]; PHYSICAL = SHARED["PHYSICAL"]
SCOPES = ("focused", "native-start", "native-end", "reload-start", "reload-end")
SOURCES = ("src/tests/CMakeLists.txt", "src/tests/heavy_role_native.cpp", "src/tests/assembly_native_fixture.h",
           "src/tests/multilayer/acceptance/heavy_role_checks-1.py", "src/tests/multilayer/acceptance/heavy_role_checks-2.py",
           "src/tests/multilayer/acceptance/heavy_joined_checks-1.py", "src/tests/multilayer/acceptance/heavy_joined_checks-3.py",
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


def qualify(saved, phase):
    """Qualify original two-role output/lease and unchanged actual supplies from one accepted save.

    @param saved Exact current native/ordinary/own-resave identity.
    @param phase Required original gameplay checkpoint represented by this save.
    @return Original parsed hull/manifest state for continuation comparison.
    """
    path = ROOT / saved["path"]; assert identity(path) == saved; version, raw = read_stream(path); assert version == 437
    heavy, small = hull(raw, 22), hull(raw, 16); after_heavy = phase in ("contended", "legacy-attached", "legacy-work"); after_small = phase == "legacy-work"
    assert heavy["held"] == (4 if after_heavy else 0) and heavy["extracted"] == (44 if after_heavy else 40) and heavy["capacity"] == 40
    assert small["held"] == (2 if after_small else 0) and small["extracted"] == (22 if after_small else 20) and small["capacity"] == 20
    assert heavy["work"] == (37 if phase == "partial" else 0) and small["work"] == 0
    # Each original bill, shell and earned quota stays with that same paid incarnation. Existing descent recovery spends only its original job shell, never either remaining onboard shell.
    for vehicle, generation, pods, parts in ((22, 8, 2, 2), (16, 2, 1, 1)):
        manifest = PARTS["manifest"](raw, vehicle, generation)
        assert manifest == dict(work=0, allowance=1, paused=0, pending=0, history=0, pods=pods, quantities=[0, parts, parts])
    if phase in ("attached", "partial", "contended", "legacy-attached", "legacy-work"):
        owner = 16 if phase.startswith("legacy-") else 22; target = small["target"] if owner == 16 else heavy["target"]
        field = chunks(path)[1][b"ASTR"][3:]; count = int.from_bytes(field[27:31], "big"); rows = [field[51 + 40 * i:51 + 40 * (i + 1)] for i in range(count)]
        original = [row for row in rows if int.from_bytes(row[:4], "big") == target]; assert len(original) == 1
        assert int.from_bytes(original[0][36:40], "big") == owner
    assert chunks(path)[1][b"SPDJ"][3:][8:10] == b"\0\0"
    return dict(heavy=heavy, small=small)


def process_evidence(native, configuration, references):
    """Bind one actual current role process to its executable and terminal log.

    @param native Original invocation process report.
    @param configuration Exact matching current test/product configuration.
    @param references Shared direct artifact identity audit.
    @return Actual terminal identity, including the original ordinary console routing proof.
    """
    if Path(native["executable"]["path"]).name == "openttd_console.exe":
        return JOINED_AUDIT["process_evidence"](native, configuration, references)
    assert ROOT / native["executable"]["path"] == ROOT / "build" / configuration / "openttd_test.exe"
    filename = "focused.log" if native["scenario"] == "heavy-role-focused" else "heavy-role.log"
    result = identity(ROOT / "logs/space" / native["scenario"] / native["run_id"] / filename); AUDIT["inspect_references"](result, references); return result


def configuration(name, selectors, references):
    """Audit exact native action membership and each actual own-resave next result.

    @param name Matching current configuration.
    @param selectors Five completed scopes in explicit acceptance order.
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
                totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] == 12
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
    assert set(paths) == set(SHARED["PARENTS"]) and len(states) == 21 and len(processes) == 22
    return dict(configuration=name, save_version=437, selectors=[identity(p) for p in selectors], reports=reports, processes=list(processes.values()), ordinary_checkpoints=7, own_resave_actions=6, own_resave_terminal_admissions=1, **totals), states


def main():
    """Publish role acceptance only after both actual builds and every physical owner/configured metadata comparison agree."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=5, type=Path, required=True); parser.add_argument("--debug", nargs=5, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", args.release, references); debug, right = configuration("Debug", args.debug, references); assert left.keys() == right.keys(); comparisons = 0
    # The same original two paid hulls must converge across actual native, ordinary and own-resave actions; CDIM stays separate metadata rather than another physical inventory owner.
    for key in left:
        a, b = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in PHYSICAL:
            assert a[family] == b[family], (key, family); comparisons += 1
        assert a[b"CDIM"] == b[b"CDIM"], key
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9D; actual original15/21 finite work, moving exclusive lease, exact native active/idle expense and original quota/parts/pod conservation. Paid service cycle/network/final heavy aggregate remain9DB/9DC; processors remain10-12.",
                  configurations=[release, debug], sources=[identity(ROOT / name) for name in SOURCES], direct_references=list(references.values()),
                  exact_owner_families=[f.decode("ascii") for f in PHYSICAL], cross_configuration_comparisons=comparisons, configured_metadata_comparisons=len(left), procurement_acceptance=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], comparisons, flush=True)


if __name__ == "__main__":
    main()
