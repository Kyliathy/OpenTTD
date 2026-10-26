"""Compare complete original CAPA rows across genuine legacy conversion and native resave.

@file asteroid_cargo_roundtrip_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group21.
This independent reader changes no input or native owner. The native loader
separately joins each mineral to its same-file body and actual cargo container.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from savegame_tables import read_stream, saved_owner_table


def compare(source, target):
    """Require every original identity, amount and accounting field to survive native transport.

    @param source Immutable genuine older or modern save.
    @param target Actual native441-or-later resave, never a fixture-rewritten version.
    @return Bounded source hashes, versions and conserved packet/unit/domain counts.
    """
    before_version, _ = read_stream(source)
    after_version, _ = read_stream(target)
    assert after_version >= 441
    before = saved_owner_table(source, b"CAPA")
    after = saved_owner_table(target, b"CAPA")
    assert before.keys() == after.keys(), "native packet identities changed"
    natural = 0
    # Compare complete independently parsed original rows, including ordinary
    # source IDs, invalid sentinels, age, travelled distance and virtual credit.
    # Only explicit old asteroid qualification may change the source scalar.
    for identity, row in before.items():
        expected = dict(row)
        if row["source_type"] == 3:
            natural += 1
            if before_version < 441:
                expected["source_id"] = 65535
                expected["asteroid_body"] = row["source_id"]
                expected["asteroid_mineral"] = after[identity]["asteroid_mineral"]
            assert expected["source_id"] == 65535
            assert expected["asteroid_mineral"] in {int.from_bytes(label, "big") for label in (b"NORE", b"WICE", b"RARE")}
        else:
            expected["asteroid_body"] = 2**32 - 1
            expected["asteroid_mineral"] = 0
        assert after[identity] == expected, (identity, row, after[identity])
    return {"_documentation": "r2sff1-space-fixes-features-1.md Group21",
            "before_version": before_version, "after_version": after_version,
            "packets": len(before), "natural": natural, "ordinary": len(before) - natural,
            "units": sum(row["count"] for row in before.values()),
            "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "target_sha256": hashlib.sha256(target.read_bytes()).hexdigest()}


def main():
    """Parse one read-only native source/resave comparison.

    @return Zero after exact semantic conservation, or an assertion/error otherwise.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--target", required=True)
    args = parser.parse_args()
    print(json.dumps(compare((ROOT / args.source).resolve(), (ROOT / args.target).resolve())))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
