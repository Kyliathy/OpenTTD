"""Register verified paired fleet producers in the existing source recipe catalogue.

@file register-ground-fleet-sources.py Disposable declaration tooling, never an asset catalogue.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DC.
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


def register():
    """Declare nine real paired source producers with original byte resume signatures.

    @return Registration counts; no art or public source contract is modified.
    """
    path = ROOT / "mlm/review-producers.json"
    catalogue = json.loads(path.read_text(encoding="utf-8"))
    template = next(row for row in catalogue["recipes"] if row["id"] == "ground-source-mine-coal")
    facts = source.load_facts()
    fixed = {source.handoff.SCOPE, ROOT / "src/tests/fixtures/graphics/ground-source-facts.json",
             ROOT / "media/multilayer/cargo_catalogue.json", ROOT / "src/tests/fixtures/cleanup/tool-requirements.json"}
    fixed.update(ROOT / row["path"] for row in [*facts["protected_prefix_files"], *facts["original_native_descriptors"]])
    rows = []
    # A model leaf produces its own body and purchase pair. Selecting one must
    # prepare only its actual reusable M1 base source, never every Ground model.
    for asset in (*source.CONVENTIONAL, *source.CAPSULES):
        module, record = source.converter(asset)
        related = source.related_converters(module)
        records = {record}
        records.update(owner.RECORD for owner in related if hasattr(owner, "RECORD"))
        inputs = fixed | set(source.shared.source_snapshot("ground", records=records))
        row = copy.deepcopy(template)
        slug = asset.replace(".", "-")
        row.update(id="ground-source-" + slug, scenario="source-" + slug,
                   producer=Path(module.__file__).relative_to(ROOT).as_posix(),
                   requires=[{"recipe": "m1-source-base", "role": "manifest", "as": "base-source"}],
                   original_parameters={"asset": asset, "collection": "ground", "publication": False, "source_symbol": "build"})
        implementation = set(row["implementation_sources"])
        implementation.update(("src/multilayer_vehicle_catalogue.cpp", "src/multilayer_content.cpp"))
        retained = []
        # Prefix C++ declarations remain implementation identities; all actual
        # media originals and independent JSON expectations keep retained roles.
        for index, original in enumerate(sorted(inputs)):
            relative = original.relative_to(ROOT).as_posix()
            if relative.startswith("src/") and original.suffix != ".json":
                implementation.add(relative)
            else:
                retained.append(dict(role="original-input-" + str(index + 1), **identity(original)))
        row["retained_inputs"] = retained
        row["implementation_sources"] = sorted(implementation)
        rows.append(row)
    ids = {row["id"] for row in rows}
    catalogue["recipes"] = [row for row in catalogue["recipes"] if row["id"] not in ids]
    index = next(index for index, row in enumerate(catalogue["recipes"]) if row["id"] == "ground-derived-inputs")
    catalogue["recipes"][index:index] = rows
    donor = next(row for row in catalogue["recipes"] if row["id"] == "m1-source-base")
    existing = {row["path"] for row in donor["retained_inputs"]}
    # The base prerequisite's actual original media must participate in its
    # own signature; a modified raw source cannot resume a completed base leaf.
    for original in sorted(source.shared.source_snapshot("base")):
        relative = original.relative_to(ROOT).as_posix()
        if relative.startswith("media/") and relative not in existing:
            donor["retained_inputs"].append(dict(role="base-original-" + str(len(donor["retained_inputs"])), **identity(original)))
            existing.add(relative)
    path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
    return {"paired_model_leaves": len(rows), "retained_per_leaf": [len(row["retained_inputs"]) for row in rows],
            "base_retained_inputs": len(donor["retained_inputs"])}


if __name__ == "__main__":
    print(json.dumps(register(), indent=2))
