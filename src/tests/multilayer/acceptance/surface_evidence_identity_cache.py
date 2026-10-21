"""Reuse exact SHA identities only while their original files remain unchanged.

@file surface_evidence_identity_cache.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEC.
This invocation-local read cache changes no gameplay or proof requirements.
Every distinct path is SHA-checked and all file metadata is rechecked at close.
"""
from pathlib import Path

from space_preview_checks import identity


def token(path):
    """Read the actual file incarnation and native change metadata.

    @param path Exact original or retained authority, never a generated identity.
    @return Device, inode, size, modification and change timestamps for consistency checks.
    """
    stat = path.stat()
    return stat.st_dev, stat.st_ino, stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns


class IdentityCache:
    """Invocation-local exact content identities with fail-closed file-change detection."""

    def __init__(self):
        """Create an empty read cache; no file identity is trusted before its first SHA read."""
        self.entries = {}
        self.hits = 0

    def __call__(self, path):
        """Read the actual identity once per unchanged path during this verification.

        @param path Exact authority path supplied by the original binder.
        @return Original identity format, byte-for-byte identical to an uncached SHA read.
        @throws AssertionError If a previously observed authority changes during the bind.
        """
        path = Path(path)
        key, current = str(path), token(path)
        if key in self.entries:
            original, value = self.entries[key]
            assert original == current, ("authority changed during evidence bind", path)
            self.hits += 1
            return dict(value)
        value = identity(path)
        assert token(path) == current, ("authority changed during SHA read", path)
        self.entries[key] = current, value
        return dict(value)

    def assert_unchanged(self):
        """Recheck every actually SHA-read path before closing the proof.

        @throws AssertionError If any observed original or retained authority changed after reading.
        """
        # Every distinct file remains part of the same read-only snapshot;
        # repeated references cannot hide a later write or inode replacement.
        for path, (original, value) in self.entries.items():
            assert token(Path(path)) == original, ("authority changed before evidence close", path)
