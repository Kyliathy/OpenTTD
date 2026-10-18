"""Bind focused actual paid Surface execution without replaying old evidence graphs.

@file surface_supply_checks-10.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCD
Original source/product/process/save identities define acceptance. This binder
does not run native children or claim driven construction/final factory production.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, identity


def read(path):
    """@param path Explicit repository-owned original JSON. @return Its parsed original value."""
    return json.loads(path.read_text(encoding="utf-8"))


def verify(reference):
    """@param reference Original retained file identity. @return Its path after exact current-byte validation."""
    path = ROOT / reference["path"]
    assert identity(path) == reference, path
    return path


def member(path):
    """Validate an actual final membership, its process and all current tested source/product bytes.

    @param path Explicit final focused member, never an invalidated diagnostic run.
    @return Complete original proof with independently verified immediate evidence.
    """
    reference = read(path)["report"]
    proof = read(verify(reference))
    assert proof["complete"]
    application = proof.get("application", proof.get("executable"))
    # Every current tested original must match, so a stale executable, changed
    # helper or another process's generated save cannot satisfy this group.
    for original in [application, proof["terminal"], *proof["sources"], *proof["native_processes"]]:
        verify(original)
    if "source" in proof:
        verify(proof["source"])
    for output in proof.get("outputs", {}).values():
        verify(output["file"])
    return proof


def bind(specification):
    """Bind exact final both-build component/native/refusal boundaries and retained predecessor applications.

    @param specification Explicit repository-owned member manifest.
    @return Complete paid-execution proof and focused build/retention receipt.
    """
    spec = read(specification)
    prior = read(ROOT / "logs/surface-supply-order-persistence-build-verification.json")
    retention = ROOT / "logs/surface-supply-order-execution-retained-tests.json"
    retained = read(retention)
    assert prior["complete"] and retained["complete"]
    configurations, builds = [], []
    process_count = 0
    # Each build must independently run the same current paid command owners;
    # historical component products and declared persistence records cannot substitute.
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        components = member(ROOT / entry["components"])
        files = [member(ROOT / entry[key]) for key in ("prepare", "restore", "resave")]
        assert components["configuration"] == configuration and components["native_financial_commands"]
        assert {proof["configuration"] for proof in files} == {configuration}
        assert [proof["phase"] for proof in files] == ["paid-prepare", "paid-restore", "paid-resave"]
        application = components["executable"]
        assert all(proof["application"] == application for proof in files)
        prepare, restore, resave = files
        assert restore["source"] == prepare["outputs"]["surface-supply-paid-pending-440.sav"]["file"]
        assert resave["source"] == restore["outputs"]["surface-supply-paid-fulfilled-440.sav"]["file"]
        predecessor = read(verify(read(ROOT / entry["empty_predecessor"])["report"]))
        assert predecessor["complete"] and predecessor["configuration"] == configuration
        assert prepare["source"] == predecessor["outputs"]["surface-supply-empty-upgrade-440.sav"]["file"]
        verify(predecessor["source"])
        refusals = []
        # These focused malformed paid copies must refuse through both original
        # full and copied loaders while preserving the exact accepted predecessor.
        for path in entry["refusals"]:
            refusal = read(ROOT / path)
            assert refusal["complete"] and refusal["configuration"] == configuration
            assert refusal["source"] == prepare["outputs"]["surface-supply-paid-pending-440.sav"]["file"]
            for original in (refusal["source"], refusal["malformed"], *refusal["sources"]):
                verify(original)
            phases = set()
            for reference in refusal["reports"]:
                proof = read(verify(reference))
                assert proof["complete"] and proof["application"] == application
                assert proof["configuration"] == configuration and proof["source"] == refusal["source"] and proof["malformed"] == refusal["malformed"]
                phases.add(proof["phase"])
                for original in (proof["source"], proof["malformed"], proof["terminal"], *proof["native_processes"], *proof["sources"]):
                    verify(original)
                process_count += len(proof["native_processes"])
            assert phases == {"preview-refusal", "full-refusal"} and len(refusal["reports"]) == 2
            refusals.append(dict(membership=identity(ROOT / path), **refusal))
        assert {proof["case"] for proof in refusals} == {"price", "pending-packet", "missing-station-field"}
        old = next(row for row in prior["configurations"] if row["configuration"] == configuration)
        kept = next(row for row in retained["configurations"] if row["configuration"] == configuration)
        assert kept["original"] == old["focused_test_application"]
        assert kept["retained"]["sha256"] == old["focused_test_application"]["sha256"]
        verify(kept["retained"])
        verify(old["complete_test_application_unchanged"])
        verify(old["game_application_unchanged"])
        build_log = ROOT / entry["build_log"]
        raw = build_log.read_bytes()
        # Preserve the original build-log bytes/identity while decoding the
        # PowerShell transcript's actual BOM; UTF-16 is transport, not a failed compiler result.
        terminal = raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8", errors="replace")
        assert "openttd_surface_supply_test.vcxproj ->" in terminal and " error " not in terminal
        builds.append(dict(configuration=configuration, build_log=identity(build_log), focused_test_application=application,
                           retained_persistence_application=kept["retained"], complete_test_application_unchanged=old["complete_test_application_unchanged"],
                           game_application_unchanged=old["game_application_unchanged"]))
        process_count += len(components["native_processes"]) + sum(len(proof["native_processes"]) for proof in files)
        configurations.append(dict(configuration=configuration, components=components, paid_files=files, malformed_refusals=refusals))
    assert {row["configuration"] for row in configurations} == {"Debug", "RelWithDebInfo"} and len(configurations) == 2
    proof = dict(_documentation=__doc__, complete=True, configurations=configurations, native_processes=process_count,
                 actual_paid_commands=True, actual_native_station_dispatch=True, native_lifecycle_retention=True,
                 actual_paid_native_continuation=True, no_full_regression=True, driven_construction_acceptance=False,
                 final_surface_factory_acceptance=False, manifest=identity(specification), binder=identity(Path(__file__)))
    build = dict(_documentation=__doc__, complete=True, target="openttd_surface_supply_test", configurations=builds,
                 retained_tests=identity(retention), no_full_regression=True, game_application_relinked=False)
    return proof, build


def main():
    """Write new explicit final proof/build receipts only after all focused original boundaries validate."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build-output", type=Path, required=True)
    args = parser.parse_args()
    outputs = [(ROOT / path).resolve() for path in (args.output, args.build_output)]
    assert all(path.is_relative_to(ROOT / "logs") and not path.exists() for path in outputs)
    proof, build = bind(ROOT / args.manifest)
    outputs[0].write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    build["focused_acceptance"] = identity(outputs[0])
    outputs[1].write_text(json.dumps(build, indent=2) + "\n", encoding="utf-8")
    print(identity(outputs[0]), identity(outputs[1]), flush=True)


if __name__ == "__main__":
    main()
