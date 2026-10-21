"""Check that evidence reuse never accepts changed on-disk authorities.

@file surface_evidence_identity_cache_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEC.
Disposable actual files stay under test-cache; no native game or receipt changes.
"""
import tempfile
from pathlib import Path

from space_preview_checks import ROOT, identity
from surface_evidence_identity_cache import IdentityCache


def main():
    """Check unchanged reuse, a real intervening write, and a write detected only at proof close."""
    parent = ROOT / "test-cache/surface-entry/identity-cache-checks"
    parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=parent) as directory:
        path = Path(directory) / "authority.bin"
        path.write_bytes(b"original")
        cache = IdentityCache()
        assert cache(path) == cache(path) == identity(path) and cache.hits == 1
        path.write_bytes(b"changed-content")
        rejected = False
        try:
            cache(path)
        except AssertionError:
            rejected = True
        assert rejected
        current = IdentityCache()
        current(path)
        current.assert_unchanged()
        path.write_bytes(b"changed-again")
        rejected = False
        try:
            current.assert_unchanged()
        except AssertionError:
            rejected = True
        assert rejected
    print("unchanged reuse and both actual file-change refusals passed")


if __name__ == "__main__":
    main()
