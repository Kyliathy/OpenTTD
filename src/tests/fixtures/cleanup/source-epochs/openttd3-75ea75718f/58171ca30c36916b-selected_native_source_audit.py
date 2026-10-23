"""Audit exact current source drift in the existing selected native gallery receipts.

@file selected_native_source_audit.py
@see r2mlvs14-remaining-assets-and-runtime-integration.md Group84FB.
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DEC.
This is a read-only source audit,never a source-hash alias or native acceptance substitute.
"""

import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import activate_m1_art as activation
from tools import selected_gallery as gallery


def audit(selection_path, output):
    """Bind actual changed sources and exact unchanged public/native/pixel authorities before refresh."""
    selection = activation.select_evidence(ROOT / selection_path)
    public = gallery.read(activation.PUBLIC)
    required = {"asset_id", "consumer", "directions", "frames", "anchor", "source"}
    assert all(required.issubset(row) for row in public["assets"])
    rows, sources = {}, {}
    for role in gallery.ROLES:
        path = selection.selected("reports", role)
        proof = gallery.read(path)
        changed = {}
        for ref in gallery.references(proof):
            name = ref["path"]
            if name.startswith(("src/", "media/")) and name not in (gallery.PUBLIC, activation.PACKED.relative_to(ROOT).as_posix()):
                actual = activation.evidence_identity(ROOT / name)
                sources[name] = actual
                if actual["sha256"] != ref["sha256"]:
                    changed[name] = {"sealed": ref, "current": actual}
        assert set(changed) == {"src/tests/CMakeLists.txt"}, (role, sorted(changed))
        rows[role] = {"receipt": activation.evidence_identity(path), "changed_sources": list(changed.values())}
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84FB;required normal-test translation units stale all four strict selected native source bindings;actual refresh required",
              "controller": activation.evidence_identity(Path(__file__)), "selection": activation.evidence_identity(selection.path),
              "public": activation.evidence_identity(activation.PUBLIC), "packed": activation.evidence_identity(activation.PACKED),
              "required_public_fields": sorted(required), "roles": rows, "unique_current_sources": list(sources.values()),
              "current_binaries": {cfg: activation.evidence_identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in ("Debug", "RelWithDebInfo")},
              "remaining": "Refresh actual purchase/structure/movement/geothermal native observations;strict source guards remain unchanged"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"audit": activation.evidence_identity(target), "affected_roles": 4, "changed_source": "src/tests/CMakeLists.txt"}), flush=True)


def main():
    """Select an exact retained named gallery and an absent source-audit artifact."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    audit(args.selection, args.output)


if __name__ == "__main__":
    main()
