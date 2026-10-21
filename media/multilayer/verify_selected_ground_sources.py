"""Audit the exact selected underground source set before native packing.

@file verify_selected_ground_sources.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.4C/D.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group42C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This read-only source audit preserves the prior native, packing and runtime
claims. It does not rerun historical producer fixtures or grant missing display.
"""

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
SUPPLY = tuple("mine." + resource for resource in ("COAL", "IORE", "CORE", "SALT", "LITH", "COBT", "RARE")) + (
    "facility.metal-refinery", "facility.chemical-works", "facility.battery-works", "facility.electronics-works")
FLEET = tuple(f"{kind}.model-{model:02d}" for model in range(4, 14) for kind in ("vehicle", "purchase"))
INFRASTRUCTURE = (
    "facility.rail-station", "facility.road-terminal", "facility.tube-terminal", "facility.tube-service",
    "facility.tube-interchange", "facility.vent-collector", "facility.vent-powerhouse", "facility.magma-powerhouse",
    "facility.utility-riser", "geothermal.vent", "geothermal.conduit", "tube.straight", "tube.bend45", "tube.junction",
    "tube.end", "tube.crossing", "tube.conduit", "control.tube", "control.geothermal")
SELECTED = SUPPLY + FLEET + INFRASTRUCTURE


def identity(relative_path, expected=None):
    """Read a real repository source artifact and verify its declared immutable identity.

    @param relative_path Published repository-relative original, native atlas or source manifest.
    @param expected Optional exact published SHA256; absent only for the manifest receipt itself.
    @return Actual repository-relative path and SHA256 identity.
    @throws ValueError On missing, escaped or changed artifacts.
    """
    path = (ROOT / relative_path).resolve()
    # Artifacts must stay within this source checkout and really exist; a
    # foreign cache or missing cleaned file cannot establish source readiness.
    if not path.is_relative_to(ROOT) or not path.is_file():
        raise ValueError("selected source artifact is absent or outside the repository: " + str(relative_path))
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if expected is not None and digest != expected:
        raise ValueError("selected source artifact changed: " + relative_path)
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": digest}


def audit(document):
    """Verify the fifty selected complete source families through their existing shared owners.

    @param document Current public asset manifest, including untouched Space and unselected rows.
    @return Source-only receipt with exact original/atlas/manifest identities and prior runtime declarations.
    @throws ValueError If any selected family is missing, ambiguous, incomplete or unbound.
    """
    rows = []
    # The master selects eleven material sources, conventional/tube fleet and
    # the named infrastructure. Unused profile families cannot silently join
    # this closure or disappear behind a whole-profile completion claim.
    for asset_id in SELECTED:
        matches = [row for row in document["assets"] if row["asset_id"] == asset_id]
        if len(matches) != 1:
            raise ValueError("selected source family is missing or ambiguous: " + asset_id)
        row = matches[0]
        source, authoring = row["source"], row.get("authoring", {})
        complete = authoring.get("coverage_complete") is True or source.get("source_coverage_complete") is True
        if source.get("status") != "authored" or not complete:
            raise ValueError("selected family lacks complete authored source: " + asset_id)
        original = identity(source["path"], source["sha256"])
        atlas_path = authoring.get("native_atlas", source["path"])
        atlas = identity(atlas_path, authoring.get("native_sha256", source["sha256"]))
        manifest = identity(authoring.get("atlas_manifest", source.get("manifest")))
        # The actual retained native source must keep its palette/transparency
        # contract. This is source proof, independent of packed sprite loading.
        with Image.open(ROOT / atlas["path"]) as image:
            if image.mode != "P" or image.info.get("transparency") != 0:
                raise ValueError("selected source loses its indexed native transparency: " + asset_id)
            canvas = list(image.size)
        poses = (len(row["directions"]) * row["frames"] * len(row["states"])
                 * len(row["parts"]) * row["tile_variants"])
        declared = authoring.get("staged_pose_count", poses)
        if declared != poses:
            raise ValueError("selected source declared pose matrix changed: " + asset_id)
        rows.append({"asset_id": asset_id, "source_complete": True, "source_poses": poses,
                     "original": original, "native_atlas": atlas, "source_manifest": manifest, "canvas": canvas,
                     "prior_runtime": row.get("runtime"), "prior_packed_declaration": row.get("packed"),
                     "runtime_activated": authoring.get("runtime_activated", source.get("runtime_activated", False))})
    return {"_documentation": {"owner": "r2mlvs3mp-master-plan.md checkpoint10.4C; r2mlvs14 Group42C",
                               "architecture": "archi-map-layers.md", "scope": "Selected source closure only; native packing/decoding/game delivery remain10.4D"},
            "selected_families": len(rows), "selected_source_poses": sum(row["source_poses"] for row in rows),
            "source_coverage_complete": True, "new_packing_or_runtime_claim": False, "families": rows,
            "existing_base_reuse": "Original Steel/Oil art and delivered M1 rail/road service depots retain their original owners",
            "unselected_scope": "Bauxite/Nickel/Uranium and component/robotics/mirror-kit/whole-profile controls remain pending",
            "rebuild": "python media/multilayer/verify_selected_ground_sources.py --report <repository logs path>"}


def main():
    """Write a reproducible source-only receipt under the repository diagnostics owner.

    @return None; no public manifest, original, packed payload or native owner is modified.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    target = (ROOT / args.report).resolve()
    if not target.is_relative_to(ROOT / "logs"):
        raise ValueError("selected source audit report must stay under repository logs")
    report = audit(json.loads(PUBLIC.read_text(encoding="utf-8")))
    report["public_manifest"] = identity(PUBLIC.relative_to(ROOT).as_posix())
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("selected_families", "selected_source_poses", "source_coverage_complete")}))


if __name__ == "__main__":
    main()
