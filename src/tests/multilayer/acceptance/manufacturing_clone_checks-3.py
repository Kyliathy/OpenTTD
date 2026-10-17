"""Verify bounded clone decoding, independent full/preview refusal and exact legacy normalization.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB
The structural walker reads native framing and contained owners; it introduces no alternate gameplay authority.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from runner import current_run_context
from space_preview_checks import ROOT, Reader, CHUNK_RIFF, CHUNK_RECORDS, read_stream, chunks, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("manufacturing_clone_checks-1.py")))
NATIVE = runpy.run_path(str(Path(__file__).with_name("manufacturing_materialization_checks-1.py")))["native"]
KINDS = ("old-version", "presence", "name-length", "name-utf8", "name-nul", "order-count", "action", "domain", "source-id", "source-generation", "target-generation", "self-origin", "truncated", "trailing")


def structure_layout(raw, version):
    """Walk complete native framing and bounded SPST rows to locate append-only intent fields.

    @param raw Original uncompressed native stream excluding the eight-byte container header.
    @param version Admitted427 or428 owner layout.
    @return Original SPST framing, contained jobs and exact presence/payload offsets.
    """
    assert version in (427, 428)
    reader = Reader(raw); family = None
    # Only the declared native chunk length locates SPST; arbitrary matching bytes inside names, cargo or other owners cannot select a field.
    while True:
        name = reader.take(4)
        if name == bytes(4):
            break
        frame = reader.pos; mode = reader.byte(); kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            if name == b"SPST":
                assert family is None; family = (frame, reader.pos, reader.pos + length)
            reader.take(length)
        elif kind in CHUNK_RECORDS:
            while True:
                length = reader.gamma()
                if not length:
                    break
                reader.take(length - 1)
        else:
            raise AssertionError("unsupported native framing")
    assert family is not None
    frame, start, end = family; position = start

    def read(width):
        """Admit each read entirely inside the original native SPST family.

        @param width Exact nonnegative native field width.
        @return Unsigned big-endian field value.
        """
        nonlocal position
        assert 0 <= width <= end - position
        value = int.from_bytes(raw[position:position + width], "big"); position += width
        return value

    def packets(maximum=64):
        """Skip one quantity and bounded native packet-reference list.

        @param maximum Original containing slot's packet-reference limit.
        """
        read(2); count = read(2); assert count <= maximum; read(6 * count)

    def service():
        """Skip the original service state and both independently bounded one-packet inputs."""
        read(31); packets(1); packets(1)

    next_structure = read(8); count = read(2); read(32); jobs = []
    assert count <= (end - position) // 62
    # Retain every original construction/stock/service/work owner while walking each contained job; optional clone offsets derive from their real preceding lengths.
    for _ in range(count):
        read(11); kind = read(1); read(17)
        for _ in range(2):
            read(2); packets()
        present = read(1); assert present <= 1
        if present:
            read(4); slots = read(1); assert slots <= 64
            for _ in range(slots):
                read(1); packets()
            read(17); service(); read(4); read(10); service(); read(8)
            present_job = read(1); assert present_job <= 1
            if present_job:
                job = {"kind": kind}; read(1); job["model"] = read(2); read(31)
                for _ in range(4):
                    packets()
                read(20); job["product"] = position; read(20)
                job["presence"] = position; job["present"] = 0
                if version == 428:
                    job["present"] = read(1); assert job["present"] <= 1
                    if job["present"]:
                        job["source"] = position; read(12); job["name_length"] = position; size = read(1); assert size <= 124
                        job["name"] = position; job["name_size"] = size; read(size)
                        job["order_count"] = position; orders = read(1); assert orders <= 32
                        job["orders"] = position; job["orders_size"] = orders; read(14 * orders)
                job["intent_end"] = position; jobs.append(job)
        read(20)
    assert position == end
    return dict(frame=frame, start=start, end=end, next_structure=next_structure, jobs=jobs)


def damage(context, source, kind):
    """Change one original captured-intent boundary in an isolated copy.

    @param context Explicit malformed-copy artifact owner.
    @param source Genuine accepted complete clone checkpoint.
    @param kind One independent native decoding or same-file semantic fault.
    @return New owned uncompressed malformed save; original source bytes stay unchanged.
    """
    version, raw = read_stream(source); assert version == 428
    layout = structure_layout(raw, version); candidates = [job for job in layout["jobs"] if job["present"]]
    assert len(candidates) == 1; job = candidates[0]; changed = bytearray(raw)
    assert job["name_size"] > 0 and job["orders_size"] > 0
    if kind == "old-version":
        version = 427
    elif kind == "presence":
        changed[job["presence"]] = 2
    elif kind == "name-length":
        changed[job["name_length"]] = 125
    elif kind in ("name-utf8", "name-nul"):
        changed[job["name"]] = 255 if kind == "name-utf8" else 0
    elif kind == "order-count":
        changed[job["order_count"]] = 33
    elif kind in ("action", "domain"):
        changed[job["orders"] + (kind == "domain")] = 255
    elif kind == "source-id":
        changed[job["source"]:job["source"] + 4] = bytes([255]) * 4
    elif kind == "source-generation":
        changed[job["source"] + 4:job["source"] + 12] = bytes([255]) * 8
    elif kind == "target-generation":
        assert raw[job["orders"] + 1] in (1, 2)
        changed[job["orders"] + 6:job["orders"] + 14] = struct.pack(">Q", layout["next_structure"])
    elif kind == "self-origin":
        changed[job["source"]:job["source"] + 12] = raw[job["product"]:job["product"] + 12]
    else:
        assert kind in ("truncated", "trailing")
        # Alter SPST framing together with its payload so the failure belongs to bounded owner decoding, rather than accidental corruption of the following chunk ID.
        if kind == "truncated":
            del changed[job["intent_end"] - 1]
        else:
            changed[layout["end"]:layout["end"]] = bytes(1)
        length = layout["end"] - layout["start"] + (1 if kind == "trailing" else -1)
        assert length < 1 << 28
        changed[layout["frame"]:layout["start"]] = bytes([(length >> 24) << 4]) + (length & 0xffffff).to_bytes(3, "big")
    target = context.saves / ("clone-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def verify(configuration, proof_path, kind):
    """Require independent native full/preview rejection without changing valid open-world owners.

    @param configuration Current matching game/test configuration.
    @param proof_path Exact accepted current complete clone report.
    @param kind One independent captured-intent fault.
    @return Hash-bound rejection report path.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    assert proof["configuration"] == configuration and proof["phase"] == "complete" and proof["save_version"] == 428
    source = ROOT / proof["after"]["path"]; original = identity(source); assert original == proof["after"]
    context = current_run_context("space", "manufacturing-clone-reject-" + kind); report = context.reports / "manufacturing-clone-rejection.json"; assert not report.exists()
    malformed = damage(context, source, kind)
    full = context.for_feature("space", "manufacturing-clone-full-" + kind)
    preview = context.for_feature("space", "manufacturing-clone-preview-" + kind)
    # The existing native observer owns full startup validation and independent copied preview; it does not require a dated outer wrapper or a427 output basename for either refusal path.
    NATIVE(full, configuration, "repeat", proof["model"], malformed, reject=True)
    NATIVE(preview, configuration, "repeat", proof["model"], source, corrupt=malformed)
    assert identity(source) == original
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BB; independent native full/copied-preview captured-intent rejection with open-world owners unchanged.",
                  configuration=configuration, save_version=428, kind=kind, proof=identity(proof_path), source=original, malformed=identity(malformed),
                  native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))])
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def legacy(configuration, proof_path):
    """Compare legacy upgrade/final completion with the immutable original427 native producer.

    @param configuration Current matching game/test configuration.
    @param proof_path Exact accepted upgrade or legacy-complete report.
    @return Hash-bound26-owner comparison report with only absent clone presence normalized.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    assert proof["configuration"] == configuration and proof["phase"] in ("upgrade", "legacy-complete") and proof["save_version"] == 428
    entry = json.loads(SHARED["ENTRY"].read_text(encoding="utf-8")); assert identity(SHARED["ENTRY"])["sha256"] == SHARED["ENTRY_SHA256"]
    accepted = next(row for row in entry["configurations"] if row["configuration"] == configuration)
    originals = []
    # Only exact frozen427 native completions can authorize backward compatibility; similarly named current files and ordinary report phase labels cannot substitute for the producer.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; old = json.loads(path.read_text(encoding="utf-8"))
        if old.get("phase") == "complete" and old.get("model") == proof["model"] and old.get("save_version") == 427 and "after" in old:
            originals.append((path, old))
    assert len(originals) == 1; original_path, original = originals[0]
    old_source = ROOT / original["after"]["path"]; new_source = ROOT / proof["after"]["path"]
    assert identity(old_source) == original["after"] and identity(new_source) == proof["after"]
    version, raw = read_stream(new_source); layout = structure_layout(raw, version)
    assert all(not job["present"] for job in layout["jobs"])
    normalized = bytearray(raw[layout["start"]:layout["end"]])
    # Removing exactly one explicit absent-intent byte per existing job restores427's SPST payload; no work, receipt, source, stock or other owner is normalized away.
    for job in reversed(layout["jobs"]):
        del normalized[job["presence"] - layout["start"]]
    old_version, old_raw = read_stream(old_source); old_layout = structure_layout(old_raw, old_version)
    assert bytes(normalized) == old_raw[old_layout["start"]:old_layout["end"]]
    expected, actual = chunks(old_source)[1], chunks(new_source)[1]
    for family in SHARED["EXACT"]:
        if family != b"SPST":
            assert expected[family] == actual[family], (proof["phase"], family)
    context = current_run_context("space", "manufacturing-clone-legacy-" + proof["phase"]); report = context.reports / "manufacturing-clone-legacy.json"; assert not report.exists()
    report.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BB; exact427 upgrade/last-tick compatibility, normalized only one absent-clone presence byte per actual job.",
                                     configuration=configuration, proof=identity(proof_path), producer=identity(original_path), original=identity(old_source), current=identity(new_source),
                                     exact_owner_families=len(SHARED["EXACT"]), normalized_absent_clone_bytes=len(layout["jobs"])), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one exact current completion fault or one explicit legacy compatibility comparison."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--kind", choices=KINDS)
    args = parser.parse_args()
    result = verify(args.configuration, args.proof.resolve(), args.kind) if args.kind else legacy(args.configuration, args.proof.resolve())
    print(result.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
