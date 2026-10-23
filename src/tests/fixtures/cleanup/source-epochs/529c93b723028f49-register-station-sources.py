"""Register the two verified original conventional facility source producers.

@file register-station-sources.py Disposable declaration tooling under test-cache.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DD.
"""

import copy
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_ground_sources as source
import station_source_facts as station
from evidence import identity


def register():
    """Declare actual station/terminal sources with exact durable original signatures.

    @return Compact registration counts; canonical media and native code remain unchanged.
    """
    path = ROOT / "mlm/review-producers.json"
    catalogue = json.loads(path.read_text(encoding="utf-8"))
    template = next(row for row in catalogue["recipes"] if row["id"] == "ground-source-mine-coal")
    facts = source.load_facts()
    rows = []
    # Station and terminal each have one original source producer. Their frozen
    # physical geometry does not require reconstructing other Ground families.
    for asset in source.STATIONS:
        module, record = source.converter(asset)
        records = {record}
        records.update(owner.RECORD for owner in source.related_converters(module) if hasattr(owner, "RECORD"))
        original = station.load_originals()["families"][asset]
        inputs = set(source.shared.source_snapshot("ground", records=records))
        inputs.update((station.FIXTURE, ROOT / original["descriptor"]["path"], source.handoff.SCOPE,
                       ROOT / "src/tests/fixtures/graphics/ground-source-facts.json",
                       ROOT / "src/tests/fixtures/cleanup/tool-requirements.json"))
        inputs.update(ROOT / row["path"] for row in [*facts["protected_prefix_files"], *facts["original_native_descriptors"]])
        implementation = set(template["implementation_sources"])
        implementation.update(("src/tests/multilayer/support/station_source_facts.py",
                               "src/tests/multilayer/support/station_source_facts_check.py"))
        retained = []
        # Native prefix declarations keep source ownership while exact media
        # originals and independent physical facts become retained signatures.
        for index, original_path in enumerate(sorted(inputs)):
            relative = original_path.relative_to(ROOT).as_posix()
            if relative.startswith("src/") and original_path.suffix != ".json":
                implementation.add(relative)
            else:
                retained.append(dict(role="original-input-" + str(index + 1), **identity(original_path)))
        slug = asset.replace(".", "-")
        row = copy.deepcopy(template)
        row.update(id="ground-source-" + slug, scenario="source-" + slug,
                   producer=Path(module.__file__).relative_to(ROOT).as_posix(), requires=[],
                   retained_inputs=retained, implementation_sources=sorted(implementation),
                   original_parameters={"asset": asset, "collection": "ground", "publication": False, "source_symbol": "build"})
        rows.append(row)
    ids = {row["id"] for row in rows}
    catalogue["recipes"] = [row for row in catalogue["recipes"] if row["id"] not in ids]
    index = next(index for index, row in enumerate(catalogue["recipes"]) if row["id"] == "ground-derived-inputs")
    catalogue["recipes"][index:index] = rows
    path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
    return {"station_source_leaves": len(rows), "retained_per_leaf": [len(row["retained_inputs"]) for row in rows]}


if __name__ == "__main__":
    print(json.dumps(register(), indent=2))
