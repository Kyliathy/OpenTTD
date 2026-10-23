"""Seal useful build configuration for r2fc-file-cleanup3.md Group2.4.

@file preserve_configs.py Disposable execution aid; exact provenance is migration.json.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups2/9.
"""

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_inventory import checked_path, file_identity
from artifact_paths import contained, owned_root


def main():
    """Preserve unique build configs without rewriting their original immutable bytes.

    @return None; emit an exact source/destination/hash receipt under test-cache.
    """
    manifest_path = ROOT / "src/tests/fixtures/cleanup/migration.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    owner = owned_root(ROOT, "src/tests/fixtures/config/multilayer/retained")
    exported = []
    # These are useful source-workstation settings, not generated runtime copies.
    # Equal-content configs share the payload only through complete manifest aliases.
    for payload in manifest["payloads"]:
        if payload["role"] != "configuration":
            continue
        selected = [name for name in payload["original_paths"] if name.startswith("build/")]
        if not selected:
            continue
        source = checked_path(selected[0], ROOT)
        target = contained(checked_path(payload["destination"], ROOT), owner)
        if file_identity(source)[0] != payload["sha256"]:
            raise ValueError(f"configuration changed before preservation: {source}")
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists():
            with target.open("xb") as stream:
                stream.write(source.read_bytes())
        if file_identity(target)[0] != payload["sha256"]:
            raise ValueError(f"configuration destination conflicts: {target}")
        payload["verification"] = "exported_verified"
        exported.append(dict(source=selected[0], destination=payload["destination"],
            sha256=payload["sha256"], aliases=selected))
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    (ROOT / "test-cache/cleanup/preserved-configs.json").write_text(json.dumps(
        dict(_documentation=__doc__, exports=exported), indent=2) + "\n", encoding="utf-8")
    print("Preserved unique build configuration payloads:", len(exported))


if __name__ == "__main__":
    main()
