"""Declare verified original tube infrastructure producers in the existing catalogue.

@file register-tube-infrastructure-sources.py Disposable declaration helper.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DDB.
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
    """Register nine actual original tube infrastructure leaves and exact input signatures.

    @return Small declaration counts; no public media, source geometry or native code changes.
    """
    path = ROOT / "mlm/review-producers.json"
    catalogue = json.loads(path.read_text(encoding="utf-8"))
    template = next(row for row in catalogue["recipes"] if row["id"] == "ground-source-mine-coal")
    facts = source.load_facts()
    fixed = {source.handoff.SCOPE, ROOT / "src/tests/fixtures/graphics/ground-source-facts.json",
             ROOT / "src/tests/fixtures/cleanup/tool-requirements.json"}
    fixed.update(ROOT / row["path"] for row in [*facts["protected_prefix_files"], *facts["original_native_descriptors"]])
    rows = []
    # Each source leaf calls its actual geometry/facility producer. Primitives
    # compose in memory from retained originals, without old atlas prerequisites.
    for asset in source.TUBE_INFRA:
        module, record = source.converter(asset)
        records = {record}
        records.update(owner.RECORD for owner in source.related_converters(module) if hasattr(owner, "RECORD"))
        inputs = fixed | set(source.shared.source_snapshot("ground", records=records))
        implementation = set(template["implementation_sources"])
        retained = []
        # Native prefix headers retain implementation ownership; original media
        # and independently frozen JSON facts participate in retained signatures.
        for index, original in enumerate(sorted(inputs)):
            relative = original.relative_to(ROOT).as_posix()
            if relative.startswith("src/") and original.suffix != ".json":
                implementation.add(relative)
            else:
                retained.append(dict(role="original-input-" + str(index + 1), **identity(original)))
        slug = asset.replace(".", "-")
        row = copy.deepcopy(template)
        row.update(id="ground-source-" + slug, scenario="source-" + slug,
                   producer=Path(module.__file__).relative_to(ROOT).as_posix(), requires=[],
                   retained_inputs=retained, implementation_sources=sorted(implementation),
                   original_parameters={"asset": asset, "collection": "ground", "publication": False,
                                        "source_symbol": "build_straight" if asset == "tube.straight" else "build"})
        rows.append(row)
    ids = {row["id"] for row in rows}
    catalogue["recipes"] = [row for row in catalogue["recipes"] if row["id"] not in ids]
    index = next(index for index, row in enumerate(catalogue["recipes"]) if row["id"] == "ground-derived-inputs")
    catalogue["recipes"][index:index] = rows
    path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
    return {"tube_infrastructure_leaves": len(rows), "retained_per_leaf": [len(row["retained_inputs"]) for row in rows]}


if __name__ == "__main__":
    print(json.dumps(register(), indent=2))
