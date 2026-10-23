"""Declare verified original material producers in the existing recipe catalogue.

@file register-ground-material-sources.py Disposable Group17DB declaration tooling.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md.
Rebuild by running this script after the focused material-source checks pass.
"""

import copy
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_ground_sources as source
from evidence import identity


def media_ancestry(record):
    """Collect actual retained media references without dereferencing old products.

    @param record Explicit original generation record owned by its source producer.
    @return Exact file paths contributing source bytes to resumable recipe signatures.
    """
    paths, pending = {record}, [json.loads(record.read_text(encoding="utf-8"))]
    # Original component, model, palette and prompt inputs all affect source
    # regeneration. Historical log strings never become retained prerequisites.
    while pending:
        value = pending.pop()
        if isinstance(value, dict):
            pending.extend(value.values())
        elif isinstance(value, list):
            pending.extend(value)
        elif isinstance(value, str) and value.startswith("media/"):
            path = source.contained(ROOT / value, ROOT / "media")
            if path.is_file():
                paths.add(path)
    return paths


def register():
    """Register the eleven actual material leaves and their real M1 prerequisite.

    @return Small declaration counts; the catalogue remains the sole recipe authority.
    """
    path = ROOT / "mlm/review-producers.json"
    catalogue = json.loads(path.read_text(encoding="utf-8"))
    template = next(row for row in catalogue["recipes"] if row["id"] == "launch-source-vehicle-model-14")
    facts = source.load_facts()
    fixed = {source.handoff.PUBLIC, source.handoff.SCOPE,
             ROOT / "src/tests/fixtures/graphics/ground-source-facts.json",
             ROOT / "src/tests/fixtures/cleanup/tool-requirements.json"}
    fixed.update(ROOT / row["path"] for row in facts["protected_prefix_files"])
    fixed.update(ROOT / row["path"] for row in facts["original_native_descriptors"])
    implementation = set(template["implementation_sources"])
    implementation.update(("media/multilayer/regenerate_ground_sources.py",
                           "src/tests/multilayer/support/ground_source_facts.py",
                           "src/tests/multilayer/support/ground_source_conversion_check.py"))
    rows = []
    assets = ["mine." + mineral for mineral in source.ground_mine_contract.SELECTED] + list(source.PROCESSORS)
    # Every leaf names its actual original producer and explicit dependency;
    # selecting a mine must not reconstruct the remaining Ground families.
    for asset in assets:
        module, record = source.converter(asset)
        row = copy.deepcopy(template)
        slug = asset.replace(".", "-").lower()
        row.update(id="ground-source-" + slug, producer=Path(module.__file__).relative_to(ROOT).as_posix(),
                   feature="multilayer-art", scenario="source-" + slug,
                   requires=[{"recipe": "m1-source-facility", "role": "manifest", "as": "facility-source"}],
                   original_parameters={"asset": asset, "collection": "ground", "publication": False, "source_symbol": "build"})
        retained = []
        # C++ generated prefix declarations remain source identities, while
        # actual original media and independent JSON facts keep retained roles.
        for index, original in enumerate(sorted(fixed | media_ancestry(record))):
            relative = original.relative_to(ROOT).as_posix()
            if relative.startswith("src/") and original.suffix != ".json":
                implementation.add(relative)
                continue
            retained.append(dict(role="original-input-" + str(index + 1), **identity(original)))
        row["retained_inputs"] = retained
        row["implementation_sources"] = sorted(implementation)
        rows.append(row)
    ids = {row["id"] for row in rows}
    catalogue["recipes"] = [row for row in catalogue["recipes"] if row["id"] not in ids]
    index = next(index for index, row in enumerate(catalogue["recipes"]) if row["id"] == "ground-derived-inputs")
    catalogue["recipes"][index:index] = rows
    # The real reused facility prerequisite also needs exact original-media
    # signatures; a changed raw component cannot reuse a completed old leaf.
    donor = next(row for row in catalogue["recipes"] if row["id"] == "m1-source-facility")
    existing = {row["path"] for row in donor["retained_inputs"]}
    for original in sorted(media_ancestry(ROOT / "media/multilayer/source/m1/facility-generation-record.json")):
        reference = identity(original)
        if reference["path"] not in existing:
            donor["retained_inputs"].append(dict(role="facility-original-" + str(len(donor["retained_inputs"])), **reference))
            existing.add(reference["path"])
    path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
    return {"material_leaves": len(rows), "retained_per_leaf": [len(row["retained_inputs"]) for row in rows],
            "facility_retained_inputs": len(donor["retained_inputs"])}


if __name__ == "__main__":
    print(json.dumps(register(), indent=2))
