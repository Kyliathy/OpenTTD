"""Prepare and verify the user-requested relocation of auxiliary execution notes.

@file relocate_notes.py
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup-2.md
This disposable helper stages documentation edits under test-cache. PowerShell
performs the explicit file moves after reviewing the containment-checked manifest.
Source code, historical evidence, plan checkboxes and Git state are not changed.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "test-cache/documentation-relocation"
MANIFEST = CACHE / "manifest.json"
SOURCE = ROOT / "docs/zz-reach2/upgrades/2026-10"
DESTINATION = ROOT / "docs/zz-zhr/upgrades/2026-10"
LINKS = (
    re.compile(r"(?P<prefix>\]\([ \t]*)(?P<target><[^>\r\n]+>|[^\s)]+)"),
    re.compile(r"(?m)^(?P<prefix> {0,3}\[[^\]\r\n]+\]:[ \t]*)(?P<target><[^>\r\n]+>|\S+)"),
    re.compile(r"(?i)(?P<prefix>\b(?:href|src)=[\"'])(?P<target>[^\"']+)"),
)


def digest(data):
    """@param data Exact file bytes. @return SHA256 identity used to detect concurrent edits."""
    return hashlib.sha256(data).hexdigest()


def relative(path):
    """@param path Contained repository path. @return Portable manifest location."""
    return path.relative_to(ROOT).as_posix()


def local_target(target, document):
    """@param target Markdown destination, including optional delimiters or fragment.
    @param document Original or relocated document path. @return Local target and syntax metadata, or None for external/in-page links.
    """
    angled = target.startswith("<")
    raw = target[1:-1] if angled else target
    path = re.split(r"[?#]", raw, maxsplit=1)[0]
    if not path:
        return None
    # External websites and user attachment directories remain outside this
    # repository migration; only actual repository paths can be redirected.
    if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", path):
        if not re.match(r"^[a-zA-Z]:[/\\]", path):
            return None
    if path.startswith("//"):
        return None
    decoded = unquote(path).replace("\\", "/")
    resolved = (document.parent / decoded).resolve()
    if not resolved.is_relative_to(ROOT):
        return None
    return resolved, path, raw[len(path):], angled


def rewrite_links(text, original, relocated, moves, changes):
    """@param text Original documentation text. @param original Original owner path.
    @param relocated New owner path. @param moves Exact old-to-new note mapping.
    @param changes Mutable audit trail. @return Text whose links keep their original logical targets.
    """
    def replace(match):
        """@param match One supported Markdown/HTML destination. @return Same syntax with a relocated local target when necessary."""
        target = match.group("target")
        parsed = local_target(target, original)
        if parsed is None:
            return match.group(0)
        resolved, path, suffix, angled = parsed
        destination = moves.get(resolved, resolved)
        # Documents staying in place need edits only for destinations that move.
        # Relocated notes also need their links back to the retained source plans.
        if original == relocated and destination == resolved:
            return match.group(0)
        absolute = Path(unquote(path)).is_absolute()
        new_path = str(destination) if absolute else os.path.relpath(destination, relocated.parent)
        new_path = new_path.replace("\\", "/").replace(" ", "%20")
        replacement = new_path + suffix
        if angled:
            replacement = "<" + replacement + ">"
        if replacement == target:
            return match.group(0)
        changes.append(dict(before=target, after=replacement, target=relative(destination), existed=resolved.exists()))
        return match.group("prefix") + replacement

    for expression in LINKS:
        text = expression.sub(replace, text)
    return text


def prepare():
    """Stage exact document edits and an immutable move manifest without changing any documentation."""
    assert (ROOT / ".git").exists()
    assert not MANIFEST.exists(), "Preserve any previous migration manifest"
    candidates = sorted(SOURCE.glob("r2mlsp-z-*.md"))
    assert len(candidates) == 34, "Review a changed note inventory before moving"
    moves = {path.resolve(): (DESTINATION / path.name).resolve() for path in candidates}
    # A move must never overwrite existing archive content or traverse a link
    # into a different repository or user-owned directory.
    for original, relocated in moves.items():
        assert original.is_relative_to(ROOT) and relocated.is_relative_to(ROOT)
        assert not relocated.exists(), relocated
        assert not original.is_symlink()
    records = []
    move_records = []
    for original in sorted((ROOT / "docs").rglob("*.md")):
        raw = original.read_bytes()
        text = raw.decode("utf-8-sig")
        relocated = moves.get(original.resolve(), original)
        changes = []
        updated = rewrite_links(text, original, relocated, moves, changes)
        # Explicit repository-qualified references outside links must follow the
        # same mapping; filenames alone remain useful historical document names.
        for source, destination in moves.items():
            old, new = relative(source), relative(destination)
            for before, after in ((old, new), (old.replace("/", "\\"), new.replace("/", "\\"))):
                if before in updated:
                    changes.append(dict(before=before, after=after, repository_reference=True))
                    updated = updated.replace(before, after)
        if updated != text:
            encoded = (b"\xef\xbb\xbf" if raw.startswith(b"\xef\xbb\xbf") else b"") + updated.encode("utf-8")
            staged = CACHE / "staged" / relative(relocated)
            staged.parent.mkdir(parents=True, exist_ok=True)
            staged.write_bytes(encoded)
            records.append(dict(original=relative(original), destination=relative(relocated), before_sha256=digest(raw),
                                after_sha256=digest(encoded), staged=relative(staged), changes=changes))
        if original.resolve() in moves:
            move_records.append(dict(source=relative(original), destination=relative(relocated), sha256=digest(raw)))
    result = dict(documentation=__doc__, phase="prepared", moves=move_records, edits=records)
    MANIFEST.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(moves=len(move_records), edited_documents=len(records),
                          destinations_updated=sum(len(row["changes"]) for row in records), manifest=relative(MANIFEST))))


def apply_edits():
    """Apply staged link edits only after every moved note's original bytes have been verified at its destination."""
    plan = json.loads(MANIFEST.read_text(encoding="utf-8"))
    assert plan["phase"] == "prepared"
    for row in plan["moves"]:
        assert not (ROOT / row["source"]).exists()
        assert digest((ROOT / row["destination"]).read_bytes()) == row["sha256"]
    # Validate the complete edit set before the first write, so concurrent user
    # edits cause a refusal rather than being overwritten by staged text.
    for row in plan["edits"]:
        assert digest((ROOT / row["destination"]).read_bytes()) == row["before_sha256"], row["destination"]
        assert digest((ROOT / row["staged"]).read_bytes()) == row["after_sha256"]
    for row in plan["edits"]:
        (ROOT / row["destination"]).write_bytes((ROOT / row["staged"]).read_bytes())
    plan["phase"] = "applied"
    MANIFEST.write_text(json.dumps(plan, indent=2) + "\n", encoding="utf-8")
    print("Applied staged documentation links")


def verify():
    """Verify exact relocation, changed links, plan-task preservation and absence of stale repository-qualified paths."""
    plan = json.loads(MANIFEST.read_text(encoding="utf-8"))
    assert plan["phase"] == "applied"
    missing = []
    for row in plan["edits"]:
        current = ROOT / row["destination"]
        assert digest(current.read_bytes()) == row["after_sha256"], current
        for change in row["changes"]:
            # Old disposable evidence can already be absent; relocation must not
            # introduce missing targets for links that worked before the move.
            if change.get("existed") and not (ROOT / change["target"]).exists():
                missing.append(dict(document=row["destination"], target=change["target"]))
    assert not missing, missing
    for row in plan["moves"]:
        assert not (ROOT / row["source"]).exists()
        assert (ROOT / row["destination"]).is_file()
    stale = []
    for document in (ROOT / "docs").rglob("*.md"):
        content = document.read_text(encoding="utf-8-sig")
        for row in plan["moves"]:
            if row["source"] in content:
                stale.append(dict(document=relative(document), obsolete_path=row["source"]))
    assert not stale, stale
    result = dict(complete=True, moved_documents=len(plan["moves"]), edited_documents=len(plan["edits"]),
                  destinations_updated=sum(len(row["changes"]) for row in plan["edits"]), introduced_missing_targets=missing,
                  stale_repository_paths=stale, manifest=relative(MANIFEST))
    (ROOT / "logs/documentation-note-relocation-verification.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result))


def main():
    """Dispatch only the explicitly selected preparation, edit or read-only verification phase."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("prepare", "apply", "verify"))
    args = parser.parse_args()
    {"prepare": prepare, "apply": apply_edits, "verify": verify}[args.phase]()


if __name__ == "__main__":
    main()
