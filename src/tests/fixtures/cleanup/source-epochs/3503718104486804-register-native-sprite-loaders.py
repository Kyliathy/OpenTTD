"""Register actual bounded native sprite adapters and their explicit five-family join.

@file register-native-sprite-loaders.py Disposable registration for r2fc-file-cleanup3.md18C.
"""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
path = ROOT / "mlm/review-producers.json"
catalogue = json.loads(path.read_text(encoding="utf-8"))
names = ("m1", "space", "orbital", "launch", "ground")
recipes = {row["id"]: row for row in catalogue["recipes"]}
# Each native family retains its existing producer, actual case, target and
# source roles. Enabling the implemented adapter grants no completed-task flag.
for family in names:
    row = recipes[family + "-native-loader"]
    row.update(enabled=True, pending_groups=[])
    row["implementation_sources"] = sorted(set(row["implementation_sources"]) |
        {"src/tests/multilayer/support/sprite_native_loader.py"})
if "sprite-native-loaders" in recipes:
    raise ValueError("native source closure already registered")
catalogue["recipes"].append(dict(id="sprite-native-loaders",
    producer="src/tests/multilayer/support/sprite_native_loader.py", feature="multilayer-art",
    scenario="selected-native-loaders", target=None, test_tags=[], tag_sources=[], execution="read_only",
    enabled=True, pending_groups=[], publishes_source=False, stages=["inspect", "build", "prepare", "verify"],
    configuration_required=True, configuration_reuse="forbid", comparison="current_native_observation",
    tools=["python"], retained_inputs=[],
    requires=[dict(recipe=family + "-native-loader", role="manifest", **{"as": family + "-native"}) for family in names],
    outputs=[dict(role="manifest", path="reports/current-{configuration}.json")],
    original_parameters=dict(operation="join_actual_native_sprite_loaders", publication=False),
    implementation_sources=["src/tests/multilayer/support/sprite_native_loader.py"], native_timeout_seconds=None))
path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
print("registered five actual native families and their existing-recipe aggregate")
