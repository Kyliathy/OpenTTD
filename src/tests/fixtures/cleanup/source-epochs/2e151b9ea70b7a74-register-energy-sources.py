"""Register verified original Ground energy/utility/control source producers.

@file register-energy-sources.py Disposable declaration tooling, no second asset catalogue.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DDC.
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
    """Declare six real source leaves covering seven independently checked physical families.

    @return Registration counts; original media/public/packing/native code stay untouched.
    """
    path = ROOT / "mlm/review-producers.json"
    catalogue = json.loads(path.read_text(encoding="utf-8"))
    template = next(row for row in catalogue["recipes"] if row["id"] == "ground-source-tube-straight")
    facts = source.load_facts()
    fixed = {source.handoff.SCOPE, ROOT / "src/tests/fixtures/graphics/ground-source-facts.json",
             ROOT / "src/tests/fixtures/cleanup/tool-requirements.json"}
    fixed.update(ROOT / row["path"] for row in [*facts["protected_prefix_files"], *facts["original_native_descriptors"]])
    rows = []
    # The actual control owner creates both glyph families together. One leaf
    # retains both source reports; the other actual producers remain independent.
    for asset in source.ENERGY:
        if asset == "control.geothermal":
            continue
        module, record = source.converter(asset)
        records = {record}
        records.update(owner.RECORD for owner in source.related_converters(module) if hasattr(owner, "RECORD"))
        inputs = fixed | set(source.shared.source_snapshot("ground", records=records))
        implementation = set(template["implementation_sources"])
        retained = []
        # Code declarations keep implementation ownership; original media and
        # immutable independent JSON expectations become retained signatures.
        for index, original in enumerate(sorted(inputs)):
            relative = original.relative_to(ROOT).as_posix()
            if relative.startswith("src/") and original.suffix != ".json":
                implementation.add(relative)
            else:
                retained.append(dict(role="original-input-" + str(index + 1), **identity(original)))
        requires = []
        if asset == "geothermal.vent":
            requires = [{"recipe": "m1-source-hardness", "role": "manifest", "as": "hardness-source"}]
        if asset == "facility.utility-riser":
            requires = [{"recipe": "ground-source-geothermal-conduit", "role": "manifest", "as": "conduit-source"}]
        slug = asset.replace(".", "-")
        row = copy.deepcopy(template)
        row.update(id="ground-source-" + slug, scenario="source-" + slug,
                   producer=Path(module.__file__).relative_to(ROOT).as_posix(), requires=requires,
                   retained_inputs=retained, implementation_sources=sorted(implementation),
                   original_parameters={"asset": asset, "collection": "ground", "publication": False,
                                        "source_symbol": "build_generator" if source.ENERGY[asset] == "generator" else "build"})
        rows.append(row)
    ids = {row["id"] for row in rows}
    catalogue["recipes"] = [row for row in catalogue["recipes"] if row["id"] not in ids]
    index = next(index for index, row in enumerate(catalogue["recipes"]) if row["id"] == "ground-derived-inputs")
    catalogue["recipes"][index:index] = rows
    # State explicit actual output coverage in the existing recipe authority.
    # Paired body/purchase or control source files never gain a separate registry.
    for row in catalogue["recipes"]:
        if row["id"].startswith("ground-source-"):
            asset = row["original_parameters"]["asset"]
            assets = [asset, asset.replace("vehicle.", "purchase.", 1)] if asset.startswith("vehicle.") else [asset]
            row["original_parameters"]["source_assets"] = list(source.CONTROLS) if asset in source.CONTROLS else assets
    donor = next(row for row in catalogue["recipes"] if row["id"] == "m1-source-hardness")
    existing = {row["path"] for row in donor["retained_inputs"]}
    # The real geological review background must also resume only against its
    # exact current original media, independently of the vent overlay producer.
    for original in sorted(source.shared.source_snapshot("hardness")):
        relative = original.relative_to(ROOT).as_posix()
        if relative.startswith("media/") and relative not in existing:
            donor["retained_inputs"].append(dict(role="hardness-original-" + str(len(donor["retained_inputs"])), **identity(original)))
            existing.add(relative)
    path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
    return {"energy_source_leaves": len(rows), "physical_families": 7,
            "retained_per_leaf": [len(row["retained_inputs"]) for row in rows], "hardness_retained_inputs": len(donor["retained_inputs"])}


if __name__ == "__main__":
    print(json.dumps(register(), indent=2))
