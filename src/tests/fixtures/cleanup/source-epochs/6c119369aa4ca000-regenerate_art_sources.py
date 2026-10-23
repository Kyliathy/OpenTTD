"""Rebuild one bounded source-conversion cache stage without publishing art status.

@file regenerate_art_sources.py Deterministic products belong under ignored logs/proofs/.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 6 and 10.
"""

import argparse
import hashlib
import importlib
import json

import convert_m1_art as base
from evidence import CACHE, contained

STAGES = ("base", "terrain", "portal", "vehicle", "purchase", "facility", "control", "hardness", "packing")


def main():
    """Regenerate and verify one explicit stage, preserving all source declarations.

    @return None; failure retains diagnostics without turning them into acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("stage", choices=STAGES)
    args = parser.parse_args()
    originals = [base.PUBLIC, *base.ASSETS.glob("*.json")]
    before = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in originals}
    base.OUTPUT.mkdir(parents=True, exist_ok=True)
    try:
        if args.stage == "base":
            base.build(cache_only=True)
            base.check()
        elif args.stage == "packing":
            import pack_m1_art as pack
            report, files = pack.derive()
            # Distribution inputs remain checked in for generator-free builds.
            # A cache refresh verifies them byte-for-byte rather than silently
            # publishing changed source pixels, offsets or C++ sprite bounds.
            for path, encoded in files.items():
                assert path.read_bytes() == encoded, f"distributed source differs from deterministic conversion: {path}"
            (base.OUTPUT / "packed-manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        else:
            module = importlib.import_module("convert_" + args.stage + "_art")
            report, files = module.derive()[-2:]
            # Source converters retain one authority for cropping/geometry.
            # Their pure derivations can be cached without invoking authoring
            # publication, which would reset source/runtime completion flags.
            for path, encoded in files.items():
                target = contained(path, CACHE)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(encoded)
            module.REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
            if hasattr(module, "check"):
                module.check(report, files)
    finally:
        # A failed cache refresh must also leave immutable generation records
        # and the single public sprite authority exactly as they were supplied.
        for path, expected in before.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, f"cache refresh changed source: {path}"
    print(f"Art cache stage {args.stage}: deterministic outputs regenerated; source contracts unchanged", flush=True)


if __name__ == "__main__":
    main()
