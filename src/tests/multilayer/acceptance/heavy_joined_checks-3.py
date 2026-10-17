"""Freeze exact joined-validation evidence after both current native configurations pass.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CD
Historical input indexes terminate at their original report/save identities. This audit binds current binaries/logs and every actual ordinary, own-resave and independently refused candidate.
"""
import argparse
import json
from pathlib import Path
import runpy
from PIL import Image

from space_preview_checks import ROOT, chunks, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-1.py")))
MALFORMED = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-2.py")))
AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
SCOPES = ("focused", "native-services", "native-flight", "native-legacy", "reload-services", "reload-flight", "reload-legacy", "reject-owners", "reject-dimensions", "reject-handling")
SOURCES = ("src/fios.h", "src/fios_gui.cpp", "src/space_structure_gui.cpp", "src/asteroid_gui.cpp", "src/lang/english.txt",
           "src/saveload/CMakeLists.txt", "src/tests/CMakeLists.txt", "src/saveload/saveload.h", "src/saveload/saveload.cpp", "src/saveload/afterload.cpp",
           "src/saveload/multilayer_sl.cpp", "src/saveload/cargopacket_sl.cpp", "src/saveload/vehicle_sl.cpp", "src/saveload/station_sl.cpp", "src/saveload/launch_center_sl.cpp",
           "src/saveload/space_cargo_preview.h", "src/saveload/space_cargo_preview.cpp", "src/saveload/space_descent_sl.cpp", "src/saveload/space_structure_sl.cpp",
           "src/space_descent.cpp", "src/space_structure.cpp", "src/space_vehicle.cpp", "src/orbital_service.cpp", "src/space_manifest.cpp", "src/space_mining_tuning.cpp",
           "src/saveload/engine_sl.cpp", "src/engine.cpp", "src/saveload/asteroid_sl.cpp",
           "src/tests/heavy_joined_validation.cpp", "src/tests/heavy_joined_native.cpp", "src/tests/assembly_native_fixture.h", "src/tests/native_art_render_fixture.h",
           "src/tests/multilayer/support/runner.py", "src/tests/multilayer/support/artifact_paths.py", "src/tests/test_artifact_paths.h", "src/tests/test_artifact_paths.cpp") + tuple(
               "src/tests/multilayer/acceptance/heavy_joined_checks-" + str(number) + ".py" for number in (1, 2, 3))


def process_evidence(native, configuration, references):
    """Verify actual executable bytes and terminal evidence for one current process.

    @param native Invocation-owned original process report.
    @param configuration Matching current test/product configuration.
    @param references Shared exact artifact identity audit.
    @return Original terminal log identity, including ordinary console routing.
    """
    executable = ROOT / native["executable"]["path"]; ordinary = executable.name == "openttd_console.exe"
    product = ROOT / "build" / configuration / ("openttd.exe" if ordinary else "openttd_test.exe")
    if ordinary:
        encoded = bytearray(product.read_bytes()); header = int.from_bytes(encoded[60:64], "little"); assert encoded[header:header + 4] == b"PE\0\0"
        encoded[header + 92:header + 94] = (3).to_bytes(2, "little"); assert executable.read_bytes() == encoded
    else:
        assert executable == product
    filename = "ordinary.log" if ordinary else "focused.log" if native["scenario"] == "heavy-joined-focused" else "heavy-joined.log"
    log = ROOT / "logs/space" / native["scenario"] / native["run_id"] / filename; selected = identity(log); AUDIT["inspect_references"](selected, references)
    return selected


def capture_evidence(path, proof, references):
    """Bind every reviewed PNG to unchanged pixels and its actual native PPM producer.

    @param path Exact current native checkpoint report.
    @param proof Parsed original checkpoint, including explicit PNG and native-process membership.
    @param references Shared retained identity audit.
    @return Exact conversion-record identity, or absence when this scenario intentionally has no capture.
    """
    if not proof["images"]:
        return None
    records = path.parent / "capture-conversions.jsonl"; selected = identity(records); AUDIT["inspect_references"](selected, references)
    conversions = [json.loads(line) for line in records.read_text(encoding="utf-8").splitlines()]; assert len(conversions) == len(proof["images"])
    # Conversion records must point to the same invocation's exact process and raster; a coincidentally named PNG cannot stand in for actual source-linked UI rendering.
    for picture, conversion in zip(proof["images"], conversions, strict=True):
        assert {k: conversion[k] for k in ("path", "sha256")} == picture and conversion["pixels_unchanged"] is True
        source = conversion["source_container"]; AUDIT["inspect_references"](source, references)
        assert conversion["native_reports"] and all(parent in proof["native_reports"] for parent in conversion["native_reports"])
        for parent in conversion["native_reports"]:
            assert source in SHARED["read"](ROOT / parent["path"])["artifacts"]
        with Image.open(ROOT / source["path"]) as original, Image.open(ROOT / picture["path"]) as encoded:
            assert original.size == encoded.size and original.tobytes() == encoded.tobytes()
    return selected


def configuration(name, selectors, references):
    """Audit complete explicit current membership and physical reconstruction equivalence.

    @param name Exact current configuration.
    @param selectors Ten completed focused/native/reload/refusal scope selectors in acceptance order.
    @param references Shared direct artifact identity audit, without reopening old producer graphs.
    @return Qualified configuration evidence and actual native/ordinary/own-resave states.
    """
    selectors = [path.resolve() for path in selectors]
    reports = []; states = {}; proofs = {}; faults = set(); processes = {}; captures = 0; conversions = []
    totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0, preview_assertions=0)
    # Every required scope has exact explicit membership; a diagnostic, partial selector or duplicated successful child cannot supply a missing boundary.
    for scope, selector in zip(SCOPES, selectors, strict=True):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["complete"] and selected["scope"] == scope
        expected = 1 if scope == "focused" else len(MALFORMED["GROUPS"][scope[7:]]) if scope.startswith("reject-") else len(SHARED["GROUPS"][scope.split("-", 1)[1]])
        assert len(selected["reports"]) == expected
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path)); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                assert child["path"] not in processes; native = SHARED["read"](ROOT / child["path"]); AUDIT["inspect_references"](native, references)
                intended = scope.startswith("reject-") and native["scenario"].startswith("heavy-joined-full-")
                assert native["configuration"] == name and native["exit_status"] == (1 if intended else 0)
                terminal = process_evidence(native, name, references)
                processes[child["path"]] = dict(report=child, terminal=terminal, intended_refusal=intended)
            if scope == "focused":
                totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] == 23
            elif scope.startswith("native-"):
                case = proof["case"]; assert case not in proofs and proof["save_version"] == 437 and proof["procurement_acceptance"] is False
                index, producer, source = SHARED["entry"](name, case); assert (proof["entry_index"], proof["producer"], proof["source"]) == (index, producer, source)
                proofs[case] = identity(path); states["native", case] = proof["after"]; totals["native_assertions"] += proof["assertions"]; captures += len(proof["images"])
                if conversion := capture_evidence(path, proof, references):
                    conversions.append(conversion)
            elif scope.startswith("reload-"):
                case = proof["case"]; assert proof["proof"] == proofs[case] and proof["source"] == states["native", case]
                states["ordinary", case] = proof["ordinary"]; states["own", case] = proof["resumed"]; totals["resumed_assertions"] += proof["assertions"]
                # Every original physical family and its new configured metadata remain exact through each process's own reconstruction.
                original_families = chunks(ROOT / proof["source"]["path"])[1]
                for saved in (proof["ordinary"], proof["resumed"]):
                    saved_families = chunks(ROOT / saved["path"])[1]
                    for family in (*EXACT, b"CDIM"):
                        assert saved_families[family] == original_families[family], (case, family)
            else:
                kind = proof["kind"]; assert kind not in faults and proof["proof"] == proofs[MALFORMED["CASES"][kind]] and proof["source"] == states["native", MALFORMED["CASES"][kind]]
                faults.add(kind); totals["preview_assertions"] += proof["assertions"]
    assert set(proofs) == set(SHARED["CASES"]) and faults == set(MALFORMED["CASES"])
    assert len(states) == 36 and len(processes) == 57 and sum(row["intended_refusal"] for row in processes.values()) == 10 and captures == 16
    return dict(configuration=name, save_version=437, selectors=[identity(p) for p in selectors], reports=reports, processes=list(processes.values()), capture_conversions=conversions,
                ordinary_checkpoints=12, own_resave_admissions=12, refusal_pairs=10, inspector_captures=captures, **totals), states


def main():
    """Publish the joined heavy handoff only after both actual configurations and every physical owner comparison agree."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=10, type=Path, required=True); parser.add_argument("--debug", nargs=10, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", args.release, references); debug, right = configuration("Debug", args.debug, references); assert left.keys() == right.keys(); comparisons = metadata = 0
    # Actual original states converge across builds at every native, ordinary and own-resave boundary; metadata equality is reported separately from physical inventory ownership.
    for key in left:
        before, after = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
        assert before[b"CDIM"] == after[b"CDIM"], key; metadata += 1
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; joined ordinary/Space cargo exclusivity, configured same-file mass, actual original service disclosures and immutable older-format compatibility. Integrated paid mission/network acceptance remains9D; arbitrary content-store authenticity is not claimed by copied metadata.",
                  configurations=[release, debug], sources=[identity(ROOT / name) for name in SOURCES], direct_references=list(references.values()), exact_owner_families=[f.decode("ascii") for f in EXACT],
                  cross_configuration_comparisons=comparisons, configured_metadata_comparisons=metadata, procurement_acceptance=False, legacy_dimensions="Genuine pre437 inputs retain full native configured-mass validation; independent copied configured dimensions begin at437.",
                  old_handling_scope="Actual423 handling upgrades through current native admission. Only explicitly absent factory/replacement provenance, unknown starter input, content2-to3 identity and proven native startup fields on four unstarted canonical engines are normalized; all other original bytes are exact. Current ordinary and own-resave physical families are all exact.")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], comparisons, flush=True)


if __name__ == "__main__":
    main()
