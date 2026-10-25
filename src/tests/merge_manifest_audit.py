"""Observe current donor path dispositions without changing the original routing audit.

@file merge_manifest_audit.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/m2o23-4-come-space.md Group59.
This report is a review of the retained manifest, not a recipe catalogue or a
native acceptance receipt. Unreviewed semantic differences remain explicit.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = "src/tests/fixtures/cleanup/merge-path-dispositions.json"
TEXT_SUFFIXES = {".cpp", ".c", ".h", ".hpp", ".py", ".json", ".txt", ".md", ".cfg", ".ini", ".nfo", ".svg", ".html", ".cmake", ".ps1", ".sh"}
REPLACEMENTS = {
    "src/saveload/surface_supply_sl.cpp": ("src/saveload/surface_supply_order_sl.cpp", "src/surface_supply_merged_wire.cpp"),
    "src/surface_supply_cmd.cpp": ("src/surface_supply.cpp", "src/surface_supply_order_runtime.cpp"),
    "src/transport_tube_stock.cpp": ("src/transport_tube_cargo.cpp",),
    "src/transport_tube_stock.h": ("src/transport_tube_cargo.h",),
    "src/tests/multilayer/support/runner.py": ("src/tests/multilayer/support/runner.py",),
    "src/multilayer_state.cpp": ("src/multilayer_state.cpp", "src/tests/fixtures/saveload/merged-family-set.json"),
}


def identify(path, root):
    """Hash actual reviewed bytes while retaining their checkout-relative name.

    @param path Existing original or live owner inside root.
    @param root Explicit selected checkout.
    @return Exact path, length and SHA256 identity.
    """
    data = path.read_bytes()
    return dict(path=path.relative_to(root).as_posix(), bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def disposition(row, root):
    """Keep semantic integration separate from equality, preservation and unresolved work.

    @param row One unchanged original donor routing row.
    @param root Explicit selected checkout; the donor directory is never read.
    @return Current classification with actual supporting identities and limits.
    """
    name = row["path"]
    live = root / name
    retained = row.get("retained")
    original = None
    if retained:
        original = identify(root / retained["destination"], root)
        # Preservation is a prerequisite to every classification; a changed
        # retained original cannot be excused by a successful current build.
        if original["sha256"] != retained["sha256"] or original["bytes"] != retained["bytes"]:
            raise ValueError("retained manifest original changed: " + name)
    current = identify(live, root) if live.is_file() else None
    result = dict(path=name, original_decision=row["decision"], owner_groups=row["owner_groups"],
                  retained=original, current=current)
    if name in REPLACEMENTS:
        owners = [identify(root / relative, root) for relative in REPLACEMENTS[name]]
        return dict(result, status="integrated", owners=owners,
                    reason="Reviewed original-owner reconciliation in completed merge groups; current native qualification remains separately required.")
    # Exact/LF-only equality is a byte comparison with the original, never an
    # inference from a coincident filename or a checked planning task.
    if original and current:
        before = (root / original["path"]).read_bytes()
        after = live.read_bytes()
        comparison = "exact bytes"
        if live.suffix.lower() in TEXT_SUFFIXES:
            before = before.replace(b"\r\n", b"\n")
            after = after.replace(b"\r\n", b"\n")
            comparison = "text bytes after newline normalization"
        if before == after:
            return dict(result, status="equivalent", reason="Actual original/current " + comparison + " match.")
    if name.startswith("docs/"):
        imported = root / "docs/zz-rhr/imports/openttd3-75ea75718f" / name
        if imported.is_file():
            return dict(result, status="historical", imported=identify(imported, root),
                        reason="Namespaced donor documentation is preserved; active architecture is finalized separately in Group71.")
    if row["decision"] == "reuse_retained_original":
        return dict(result, status="historical", reason="Exact original save/fixture retained as an input; no current gameplay claim.")
    if row["decision"] == "preserve_destination_no_deletion":
        return dict(result, status="historical", reason="Original audit requires destination preservation; no donor deletion or replacement.")
    if row["decision"] == "equivalent_keep_destination":
        # The pinned Git blob independently checks an entry without a retained
        # physical copy; line endings are normalized exactly as Git stores text.
        if current:
            data = live.read_bytes().replace(b"\r\n", b"\n")
            blob = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
            if blob == row["donor_blob"]["blob"]:
                return dict(result, status="equivalent", reason="Current normalized bytes match the pinned donor Git blob.")
    return dict(result, status="explicitly_unresolved",
                reason="Original semantic/asset/entrypoint difference is not certified by presence alone; retain exact owners and originals for the named review groups.")


def audit(root=ROOT):
    """Review every original manifest entry and reject duplicate or absent declared owners.

    @param root Explicit selected checkout, including focused persistent fixtures.
    @return Complete conservative path review; unresolved entries are not enabled dependencies.
    """
    root = Path(root).resolve()
    document = json.loads((root / MANIFEST).read_text(encoding="utf-8"))
    names = [row["path"] for row in document["paths"]]
    if len(set(names)) != len(names):
        raise ValueError("duplicate donor manifest path")
    # Review every row in original order so no inconvenient missing helper,
    # duplicate asset or historical entry disappears from the closure report.
    rows = [disposition(row, root) for row in document["paths"]]
    return dict(_documentation="Owner: archi-historical-debug.md; m2o23-4 Group59 conservative current review, not native acceptance.",
                schema_version=1, kind="merge_manifest_current_review", manifest=identify(root / MANIFEST, root),
                source_heads=document["source_heads"], entries=len(rows), counts=dict(Counter(row["status"] for row in rows)),
                native_acceptance=False, paths=rows)


def main():
    """Write this rebuildable observation only beneath the disposable test-cache owner.

    @return Zero after observing every retained donor manifest entry.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "test-cache/merge-build/group59-manifest.json")
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "test-cache"):
        raise ValueError("manifest observation must stay under test-cache")
    document = audit()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=str(output), entries=document["entries"], counts=document["counts"])))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
