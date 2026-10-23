"""Prepare and validate the existing selected runtime owner before native gallery/public activation.

@file selected_binding_review.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84B-84D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This controller selects existing proof artifacts through EvidenceSet; it creates no sprite, range, quantity or gameplay registry.
"""
import argparse
import copy
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import activate_m1_art as activation
from evidence import EvidenceSet, DEFAULT_MANIFEST, CACHE
from selected_runtime_audit import CASES
from geothermal_machinery_raster import verify_art_references
from mole_useful_raster_report import count_native

RECEIPTS = {
    "tube-guideway-acceptance": "logs/tubes/guideway-art/20261017-native-01/verification.json",
    "tube-capsule-acceptance": "logs/tubes/capsule-art/20261018-native-01/verification.json",
    "tube-facility-acceptance": "logs/tubes/facility-art/20261018-native-01/verification.json",
    "tube-conduit-acceptance": "logs/tubes/conduit-art/20261018-native-01/verification.json",
    "machinery-acceptance": "logs/graphics/geothermal-machinery/20261018-native-01/machinery-verification.json",
    "natural-vent-acceptance": "logs/geothermal/first-plant/20261017-native-01/window-raster-verification.json",
    "geothermal-conduit-acceptance": "logs/graphics/geothermal-conduits/20261018-native-01/verification.json",
    "selected-controls-acceptance": "logs/graphics/selected-control-icons/20261018-native-01/verification.json",
}


def prepare(reports, output):
    """Extend the existing named evidence selection with actual current producer logs and sealed native closures.

    @param reports Exact current Debug/optimized eight-tag native producer bursts, never a latest-run search.
    @param output Absent EvidenceSet selection JSON beneath the existing generated native-art cache owner.
    @return None; original public/runtime/gallery/payload files remain unchanged.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(CACHE) and not output.exists()
    base = EvidenceSet(DEFAULT_MANIFEST); document = copy.deepcopy(base.document); cache, records = {}, {}
    for name in reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg = report["configuration"]; assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        assert tuple(row["case"] for row in report["native"]) == CASES
        assert report["binary"] == activation.evidence_identity(ROOT / f"build/{cfg}/openttd_test.exe")
        count = {"cases": 0, "assertions": 0}
        for proof in report["native"]:
            count_native(proof, count)
        records[cfg] = {"report": activation.evidence_identity(path), "native": report, "counts": count}
        suffix = "debug" if cfg == "Debug" else "optimized"
        for index, role in enumerate(("tube-guideway", "tube-body", "tube-purchase", "tube-facility", "tube-conduit")):
            document["logs"][role + "-" + suffix] = report["native"][index]["log"]
    assert records.keys() == {"Debug", "RelWithDebInfo"} and records["Debug"]["counts"] == records["RelWithDebInfo"]["counts"]
    for role, name in RECEIPTS.items():  # Dated original renderer/semantic closures remain opaque after cleanup; a missing artifact refuses rather than rebuilding a paid bootstrap or inventing a log.
        path = ROOT / name; document["reports"][role] = activation.evidence_identity(path)
    for role, receipt_role, focused_index in (("machinery", "machinery-acceptance", 0), ("geothermal-conduit", "geothermal-conduit-acceptance", 0), ("selected-controls", "selected-controls-acceptance", 1)):
        receipt = json.loads((ROOT / RECEIPTS[receipt_role]).read_text(encoding="utf-8"))
        for record in receipt["focused"]:
            suffix = "debug" if record["configuration"] == "Debug" else "optimized"
            document["logs"][role + "-" + suffix] = record["native"][focused_index]["log"]
    controls = json.loads((ROOT / RECEIPTS["selected-controls-acceptance"]).read_text(encoding="utf-8"))
    for record in controls["rasters"]:  # The same actual paid native observations include active original vent pixels and complete owner/RNG purity, not merely window text.
        if record["case"] != "ready":
            continue
        suffix = "debug" if record["configuration"] == "Debug" else "optimized"
        document["logs"]["natural-vent-" + suffix] = record["native"]["log"]
    document.update(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md84B.2; existing EvidenceSet selection,current native producer/callback review and exact dated original closures;gallery/public activation pending84C/84D",
                    configuration="both", base_evidence=activation.evidence_identity(DEFAULT_MANIFEST), selected_native_producers=records,
                    retained_published_runtime=activation.evidence_identity(base.output("runtime")), retained_published_gallery=activation.evidence_identity(base.output("gallery")))
    document["outputs"]["runtime"] = (output.parent / "runtime-manifest.json").relative_to(ROOT).as_posix()
    document["outputs"]["gallery"] = (output.parent / "gallery/m1-runtime-gallery.json").relative_to(ROOT).as_posix()
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"evidence": activation.evidence_identity(output), "producer_counts": {cfg: row["counts"] for cfg, row in records.items()}}), flush=True)


def review(selection, output, check_logs=None):
    """Validate actual selected callback authority and immutable range/payload ownership without public publication.

    @param selection Explicit current selected EvidenceSet manifest prepared from retained artifacts and current native producer bursts.
    @param output Absent immutable binding-review receipt beneath repository logs.
    @param check_logs Optional exact focused binding/packing unittest logs; a final closure requires both successful logs.
    @return None; missing/stale/false implementation or proof refuses before any runtime/gallery/public flag changes.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists(); chosen = activation.select_evidence(selection)
    public = json.loads(activation.PUBLIC.read_text(encoding="utf-8")); packed = json.loads(activation.PACKED.read_text(encoding="utf-8"))
    before = activation.evidence_identity(activation.PUBLIC); bindings = activation.validate_binding_review(public, packed)
    assert len(bindings) == 69 and sum(row["sprite_count"] for row in bindings) == 10557
    assert sum(row["native_source_proof"] is not None for row in bindings) == 26 and activation.evidence_identity(activation.PUBLIC) == before
    checks = []
    for name in check_logs or ():  # Final task closure binds actual successful focused refusals, rather than assuming the positive callback review tested corrupt inputs.
        path = (ROOT / name).resolve(); assert path.is_relative_to(ROOT / "logs")
        text = activation.read_evidence(path); count = re.search(r"Ran (\d+) tests? in", text)
        assert count is not None and re.search(r"(?m)^OK\s*$", text), "focused selected binding checks did not pass"
        checks.append({"log": activation.evidence_identity(path), "tests": int(count.group(1))})
    if check_logs is not None:
        assert sorted(row["tests"] for row in checks) == [1, 9], "both focused binding and packing checks are required"
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84B.2; exact selected implementation review,not native gallery/public runtime acceptance",
              "controller": activation.evidence_identity(Path(__file__)), "activation_owner": activation.evidence_identity(ROOT / "media/multilayer/activate_m1_art.py"),
              "selection": activation.evidence_identity(chosen.path), "public": before, "scope": activation.evidence_identity(activation.pack.SCOPE), "packed": activation.evidence_identity(activation.PACKED),
              "packed_payload": packed["packed_grf"], "bindings": bindings, "selected_families": len(bindings), "selected_sprites": sum(row["sprite_count"] for row in bindings),
              "current_addition_callbacks": 26, "retained_published_prefix": 43, "public_runtime_promoted": False,
              "focused_checks": checks, "check_source": activation.evidence_identity(ROOT / "src/tests/multilayer/selected_binding_checks.py"),
              "packing_check_source": activation.evidence_identity(ROOT / "src/tests/multilayer/selected_packing_checks.py"),
              "current_native_producers": chosen.document["selected_native_producers"],
              "retained_published_runtime": chosen.document["retained_published_runtime"], "retained_published_gallery": chosen.document["retained_published_gallery"],
              "remaining": ("" if checks else "Focused missing/future/stale/false-consumer refusal proof;") + "84C current selected native gallery,84D publication;whole84.3/master10.7/human/network/operator gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(json.dumps({"receipt": activation.evidence_identity(output), "families": 69, "sprites": 10557}), flush=True)


def main():
    """Select exact current evidence preparation or read-only callback review.

    @return None; explicit existing artifacts and an absent generated destination are mandatory.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("prepare", "review"), required=True)
    parser.add_argument("--reports", nargs=2); parser.add_argument("--evidence-manifest", type=Path); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--check-logs", nargs=2); args = parser.parse_args()
    if args.phase == "prepare":
        assert args.reports; prepare(args.reports, args.output)
    else:
        assert args.evidence_manifest; review(args.evidence_manifest, args.output, args.check_logs)


if __name__ == "__main__":
    main()
