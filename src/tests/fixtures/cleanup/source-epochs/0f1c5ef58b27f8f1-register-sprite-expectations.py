"""Enable actual verified source expectation producers in the existing recipe catalogue.

@file register-sprite-expectations.py Disposable registration for r2fc-file-cleanup3.md18.
"""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
path = ROOT / "mlm/review-producers.json"
catalogue = json.loads(path.read_text(encoding="utf-8"))
recipes = {row["id"]: row for row in catalogue["recipes"]}
facts = ROOT / "src/tests/fixtures/graphics/expected-sprite-facts.json"
originals = {"m1": ("map_layers",), "space": ("space_preview",), "orbital": (),
             "launch": ("map_layers", "map_layers_m1", "space_preview", "space_final", "orbital_final"), "ground": ()}
# Each existing family leaf owns its actual source protocol. Preserve all
# original producer/feature/output identities while supplying concrete selected
# input roles and source implementation instead of stale pending-group labels.
for family, stems in originals.items():
    recipe = recipes[family + "-expected-payload"]
    packing = recipes[family + "-derived-inputs"]
    scope_path = packing["original_parameters"]["scope"]
    scope = json.loads((ROOT / scope_path).read_text(encoding="utf-8"))
    selected = {row["path"]: dict(row) for row in recipe["retained_inputs"]}
    inputs = [scope_path, facts.relative_to(ROOT).as_posix(), scope["png"], scope["nfo"]]
    inputs.extend("media/baseset/openttd/" + stem + extension for stem in stems for extension in (".png", ".nfo"))
    for index, relative in enumerate(inputs):
        selected[relative] = dict(role="source-input-" + str(index), path=relative,
            sha256=hashlib.sha256((ROOT / relative).read_bytes()).hexdigest())
    recipe.update(enabled=True, pending_groups=[], retained_inputs=list(selected.values()))
    recipe["implementation_sources"] = sorted({"src/tests/multilayer/support/sprite_expected_payload.py",
        "src/tests/multilayer_packed_art_fixture.py", scope["header"], *recipe.get("implementation_sources", [])})
    recipe["original_parameters"] = dict(symbol="produce_selected", family="launch-fleet" if family == "launch" else family,
        publication=False, source_expectations_only=True)
    if family == "launch":
        for requirement in recipe["requires"]:
            if requirement["as"].endswith("-prefix"):
                requirement["role"] = "manifest"
    if family == "ground":
        recipe["requires"] = [dict(recipe="ground-derived-inputs", role="manifest", **{"as": "ground-derived-inputs"}),
            dict(recipe="launch-expected-payload", role="manifest", **{"as": "launch-fleet-prefix"})]
path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
print("enabled", [family + "-expected-payload" for family in originals])
