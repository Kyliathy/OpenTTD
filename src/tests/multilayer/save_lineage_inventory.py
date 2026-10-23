"""Observe original save framing for the parallel-lineage compatibility matrix.

@file save_lineage_inventory.py Read-only characterization; not a gameplay loader.
@see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group 4.
@see docs/zz-reach2/architecture/savegame/archi-savegame.md.
The supplied fixture provenance labels observations; it never decides the future
runtime classifier's result. Output belongs under test-cache/merge-inventory.
"""

import argparse
from collections import Counter
import hashlib
import json
import lzma
from pathlib import Path
import sys
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent / "support"))
from artifact_paths import contained, repository_root
from savegame_tables import Reader, chunks, read_stream, table_header, table_record

INTERESTING = {"MLMD", "CAPA", "SDEF", "SPST", "CDIM", "SPRC", "SSOH", "SSOR", "GPGH", "GPGD", "UCVH", "UCVD", "TGSH", "TGSD", "TUVD"}


def observe(path):
    """Decode concrete chunk kinds, table columns and small metadata identities.

    @param path Exact retained original; it is never rewritten or loaded natively.
    @return Version, input hash and detached framing facts for selected families.
    @throws ValueError For unsupported compression or malformed original framing.
    """
    path = Path(path)
    version, stream = read_stream(path)
    families = {}
    seen = set()
    # Frame every owner so a truncated ordinary chunk cannot be hidden behind a
    # convenient later marker. No native handler, game pool or live map is used.
    for chunk in chunks(stream):
        name = chunk["name"].decode("ascii")
        if name in seen:
            raise ValueError("repeated original chunk: " + name)
        seen.add(name)
        if name not in INTERESTING:
            continue
        fact = dict(kind=chunk["kind"])
        if chunk["payload"] is not None:
            payload = chunk["payload"]
            fact["length"] = len(payload)
            fact["prefix_hex"] = payload[:42].hex()
            if name == "SDEF":
                if len(payload) != 10:
                    raise ValueError("invalid original SDEF length")
                fact["content_version"] = int.from_bytes(payload[:2], "big")
                fact["content_signature"] = int.from_bytes(payload[2:], "big")
        else:
            if not chunk["rows"]:
                raise ValueError("original table lacks a header: " + name)
            reader = Reader(chunk["rows"][0])
            fields = table_header(reader)
            if reader.pos != len(reader.data):
                raise ValueError("trailing original table header: " + name)
            fact["columns"] = [[name, kind] for name, kind, _ in fields]
            fact["rows"] = len(chunk["rows"]) - 1
            if name == "MLMD":
                if len(chunk["rows"]) != 2:
                    raise ValueError("original metadata needs exactly one row")
                reader = Reader(chunk["rows"][1])
                fact["metadata"] = table_record(reader, fields)
                if reader.pos != len(reader.data):
                    raise ValueError("trailing original metadata")
        families[name] = fact
    return dict(version=version, sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                bytes=path.stat().st_size, chunk_count=len(seen), families=families,
                authority="framing observation only; full semantic/native qualification remains later")


def candidates(root):
    """Select exact retained originals from the existing import and input owners.

    @param root Current destination checkout; the donor directory is not accessed.
    @return Origin-labelled unique save identities, sorted by size for bounded reads.
    """
    catalogue = json.loads((root / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))["inputs"]
    result = {}
    for original, reference in catalogue.items():
        origin = "B" if original.startswith("donor/openttd3-75ea75718f/") else "A"
        path = contained(root / reference["path"], root)
        if path.suffix.lower() not in {".sav", ".scn"}:
            continue
        key = (origin, reference["sha256"])
        if key in result:
            continue
        with path.open("rb") as stream:
            header = stream.read(8)
        # Only native modern containers can describe this collision interval.
        # Unsupported/older originals remain retained; this observer does not
        # invent a modern version for them or delete them as irrelevant.
        if len(header) != 8 or header[:4] not in {b"OTTX", b"OTTZ", b"OTTN"}:
            continue
        version = int.from_bytes(header[4:6], "big")
        if not 424 <= version <= 440:
            continue
        result[key] = dict(origin=origin, version=version, alias=original,
                           path=reference["path"], sha256=reference["sha256"], bytes=path.stat().st_size)
    return sorted(result.values(), key=lambda row: (row["origin"], row["version"], row["bytes"], row["path"]))


def inventory(root=None, limit=3):
    """Observe a bounded sample per original lineage/version, retaining failures.

    @param root Explicit destination repository or the current checkout.
    @param limit Maximum successful framing observations per origin/version.
    @return Candidate counts and exact sampled original facts, including refusals.
    """
    root = repository_root(root)
    rows = candidates(root)
    counts = Counter((row["origin"], row["version"]) for row in rows)
    selected = Counter()
    observations = []
    refusals = []
    # Sampling only avoids repeated large-world decompression. Candidate labels
    # are provenance, not classifier evidence, and no failed input disappears.
    for row in rows:
        key = (row["origin"], row["version"])
        if selected[key] >= limit:
            continue
        try:
            fact = observe(root / row["path"])
            if fact["sha256"] != row["sha256"]:
                raise ValueError("retained original hash changed")
        except (ValueError, UnicodeError, IndexError, KeyError, lzma.LZMAError, zlib.error) as error:
            refusals.append(dict(**row, error=str(error)))
            continue
        observations.append(dict(**row, facts=fact))
        selected[key] += 1
    return dict(_documentation="m2o23-1-come-space.md Group 4; original-byte framing facts, never runtime lineage authority.",
                counts=[dict(origin=k[0], version=k[1], count=v) for k, v in sorted(counts.items())],
                observations=observations, refusals=refusals)


def main():
    """Generate one reproducible characterization report in the disposable owner.

    @return None; prints only coverage totals, never decompresses through native code.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--limit", type=int, default=3)
    args = parser.parse_args()
    if not 1 <= args.limit <= 10:
        raise ValueError("sampling limit must be 1 through 10")
    root = repository_root()
    result = inventory(root, args.limit)
    output = root / "test-cache/merge-inventory/save-wire-facts.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(counts=result["counts"], observations=len(result["observations"]), refusals=len(result["refusals"])), indent=2))


if __name__ == "__main__":
    main()
