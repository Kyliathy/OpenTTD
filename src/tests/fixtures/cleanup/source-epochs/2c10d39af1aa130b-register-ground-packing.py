"""Declare the implemented complete current Ground source-to-packing closure.

@file register-ground-packing.py Disposable catalogue update tooling under test-cache.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DE.
"""

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_ground_sources as source
from evidence import identity


def register():
    """Enable only the actual complete source closure and its existing independent canonical gate.

    @return Exact source-leaf/physical-family/prerequisite counts for inspection.
    """
    path = ROOT / "mlm/review-producers.json"
    catalogue = json.loads(path.read_text(encoding="utf-8"))
    row = next(row for row in catalogue["recipes"] if row["id"] == "ground-derived-inputs")
    leaves = [row for row in catalogue["recipes"] if row["id"].startswith("ground-source-")]
    assets = [asset for row in leaves for asset in row["original_parameters"]["source_assets"]]
    scope = source.read_scope()
    # Every actual output has one supplier, including paired purchase/controls.
    # A gap or duplicate cannot be hidden by a hand-written fallback manifest.
    if len(assets) != len(set(assets)) or set(assets) != set(scope["asset_ids"]):
        raise ValueError("Ground source declarations do not cover exact physical packing inputs")
    requires = [{"recipe": leaf["id"], "role": "manifest", "as": "source-" + str(index + 1)}
                for index, leaf in enumerate(leaves)]
    implementation = set()
    # The selected closure already declares actual producer/policy sources;
    # retain those signatures and the original packer rather than a new build owner.
    for leaf in leaves:
        implementation.update(leaf["implementation_sources"])
    implementation.update(("media/multilayer/pack_ground_art.py", "media/multilayer/tools/ground_art_handoff.py",
                           "src/tests/multilayer/support/artifact_recipe_selection_check.py",
                           "src/tests/multilayer/support/ground_packing_contract_check.py"))
    existing = {item["path"] for item in row["retained_inputs"]}
    # Current shipping bytes are independent expectations, never candidate
    # destinations. C++ prefix declarations retain implementation ownership.
    for role in ("png", "nfo", "header"):
        original = ROOT / scope[role]
        if role == "header":
            implementation.add(scope[role])
        elif scope[role] not in existing:
            row["retained_inputs"].append(dict(role="canonical-" + role, **identity(original)))
            existing.add(scope[role])
    row.update(enabled=True, pending_groups=[], requires=requires, implementation_sources=sorted(implementation),
               original_parameters={"stage": "packing", "collection": "ground", "publication": False,
                                    "scope": "media/multilayer/ground-pack-scope.json"})
    path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
    return {"source_leaves": len(leaves), "physical_families": len(assets), "packing_enabled": True}


if __name__ == "__main__":
    print(json.dumps(register(), indent=2))
