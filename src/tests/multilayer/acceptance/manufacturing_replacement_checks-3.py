"""Verify strict actual replacement framing and independent full/copied-preview refusal.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
Offsets come from bounded native chunk/row decoding; matching payload bytes never select an owner.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, Reader, CHUNK_RIFF, CHUNK_RECORDS, read_stream, chunks, identity, ordinary
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-1.py")))
KINDS = ("old-version", "presence", "resume", "settled-flag", "source-id", "source-generation", "source-model", "credit", "retirement-phase", "self-origin", "truncated", "trailing")


def families(raw):
    """Locate the exact native RIFF family extents while admitting every intervening table record.

    @param raw Original decompressed native stream, excluding its eight-byte container header.
    @return Mapping from actual unique RIFF IDs to frame/payload/end offsets.
    """
    reader = Reader(raw); result = {}
    # Chunk framing alone identifies authoritative data; names inside strings, packet content or other owners cannot select a replacement field.
    while True:
        name = reader.take(4)
        if name == bytes(4):
            break
        frame = reader.pos; mode = reader.byte(); kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            assert name not in result; result[name] = dict(frame=frame, start=reader.pos, end=reader.pos + length); reader.take(length)
        elif kind in CHUNK_RECORDS:
            while True:
                length = reader.gamma()
                if not length:
                    break
                reader.take(length - 1)
        else:
            raise AssertionError("unsupported native framing")
    assert reader.pos == len(raw)
    return result


def layout(raw):
    """Decode bounded429 actual SPST jobs and variable SVST rows, retaining exact optional receipt offsets.

    @param raw Exact uncompressed native stream for a current429 save.
    @return Native family extents and original job/body receipt metadata.
    """
    owners = families(raw); jobs = []; bodies = []

    def reader_for(family):
        """Create an independent family-bounded integer reader.

        @param family Actual RIFF payload/end offsets.
        @return Integer reader and a callback returning its exact current offset.
        """
        position = family["start"]

        def read(width):
            """Admit a complete integer/skip extent before moving within this one owner.

            @param width Exact nonnegative byte count.
            @return Unsigned big-endian value.
            """
            nonlocal position
            assert 0 <= width <= family["end"] - position
            value = int.from_bytes(raw[position:position + width], "big"); position += width
            return value

        def offset():
            """Return the current exact bounded native offset without consuming bytes."""
            return position

        return read, offset

    read, offset = reader_for(owners[b"SPST"])

    def packets(maximum=64):
        """Skip an original quantity and bounded native packet-reference list.

        @param maximum Original physical containing slot's packet limit.
        """
        read(2); count = read(2); assert count <= maximum; read(6 * count)

    def service():
        """Skip one complete original native service state and both one-packet input owners."""
        read(31); packets(1); packets(1)

    read(8); count = read(2); read(32); assert count <= 256
    # Preserve every original construction/stock/service/work owner; replacement offsets follow the actual sparse packets and bounded clone text/instructions.
    for _ in range(count):
        read(29)
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
                job = dict(phase_offset=offset()); job["phase"] = read(1); job["model"] = read(2); read(31)
                for _ in range(4):
                    packets()
                read(20); job["product"] = offset(); read(20)
                clone = read(1); assert clone <= 1
                if clone:
                    read(12); size = read(1); assert size <= 124; read(size); orders = read(1); assert orders <= 32; read(14 * orders)
                job["presence"] = offset(); job["present"] = read(1); assert job["present"] <= 1
                if job["present"]:
                    job["intent"] = offset(); read(20)
                jobs.append(job)
        read(20)
    assert offset() == owners[b"SPST"]["end"]
    read, offset = reader_for(owners[b"SVST"]); count = read(1); assert count <= 64
    # Each actual429 receipt retains the unchanged67-byte paid prefix and its own strict optional append; older fixed-row arithmetic cannot select a body.
    for _ in range(count):
        body = dict(product=offset()); read(12); body["placement"] = read(1); read(24); body["work"] = offset(); read(4); body["model"] = read(2); read(24)
        body["presence"] = offset(); body["present"] = read(1); assert body["present"] <= 1
        if body["present"]:
            body["intent"] = offset(); read(20)
        bodies.append(body)
    assert offset() == owners[b"SVST"]["end"]
    return owners, jobs, bodies


def damage(context, source, owner, kind):
    """Change one independent actual owner boundary in an invocation-owned malformed copy.

    @param context Explicit malformed-copy artifact owner.
    @param source Exact immutable valid current save.
    @param owner Orbital SPST job or Earth SVST body.
    @param kind Independent strict optional intent/accounting/framing fault.
    @return Malformed copy path, preserving original native chunk ownership and adjusted framing for extent faults.
    """
    version, raw = read_stream(source); assert version == 429
    owners, jobs, bodies = layout(raw); family = owners[b"SPST" if owner == "orbital" else b"SVST"]
    candidates = [row for row in (jobs if owner == "orbital" else bodies) if row["present"]]; assert len(candidates) == 1
    row = candidates[0]; intent = row["intent"]; changed = bytearray(raw)
    if kind == "old-version":
        version = 428
    elif kind in ("presence", "resume", "settled-flag"):
        changed[row["presence"] if kind == "presence" else intent + (14 if kind == "resume" else 15)] = 2
    elif kind == "source-id":
        changed[intent:intent + 4] = bytes([255]) * 4
    elif kind == "source-generation":
        changed[intent + 4:intent + 12] = bytes([255]) * 8
    elif kind == "source-model":
        changed[intent + 12:intent + 14] = (18).to_bytes(2, "big")
    elif kind == "credit":
        changed[intent + 16:intent + 20] = bytes([255]) * 4
    elif kind == "retirement-phase":
        changed[intent + 15] = 1 - changed[intent + 15]; changed[intent + 16:intent + 20] = bytes(4)
    elif kind == "self-origin":
        changed[intent:intent + 12] = raw[row["product"]:row["product"] + 12]
    else:
        assert kind in ("truncated", "trailing")
        start, end = family["start"], family["end"]
        payload = raw[start:end - 1] if kind == "truncated" else raw[start:end] + bytes(1)
        changed[start:end] = payload; length = len(payload); assert length < 1 << 28
        changed[family["frame"]:start] = bytes([(length >> 24) << 4]) + (length & 0xffffff).to_bytes(3, "big")
    target = context.saves / f"replacement-malformed-{owner}-{kind}.sav"; assert not target.exists()
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def verify(configuration, proof_path, owner, kind):
    """Require independent native full-load and copied-preview refusal with active world owners unchanged.

    @param configuration Matching current completed game/test build.
    @param proof_path Exact accepted native job/body checkpoint with one present replacement receipt.
    @param owner Actual orbital job or Earth body transport.
    @param kind Independent malformed intent/accounting/framing fault.
    @return Accepted hash-bound two-process rejection report path.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration and proof["save_version"] == 429
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", f"manufacturing-replacement-reject-{owner}-{kind}")
    report = context.reports / "manufacturing-replacement-rejection.json"; assert not report.exists()
    malformed = damage(context, source, owner, kind)
    full = context.for_feature("space", f"manufacturing-replacement-full-{owner}-{kind}")
    preview = context.for_feature("space", f"manufacturing-replacement-preview-{owner}-{kind}")
    SHARED["native"](full, configuration, "repeat", proof["model"], malformed, proof["original"], reject=True)
    SHARED["native"](preview, configuration, "repeat", proof["model"], source, proof["original"], corrupt=malformed)
    assert identity(source) == proof["after"]
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; independent native full/copied-preview refusal of actual contained replacement authority, with active world owners unchanged.",
                  configuration=configuration, save_version=429, owner=owner, kind=kind, proof=identity(proof_path), source=proof["after"], malformed=identity(malformed),
                  native_reports=[identity(path) for item in (full, preview) for path in sorted(item.reports.glob("native-*.json"))])
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def legacy(configuration, proof_path):
    """Restore exactly428's original paid layouts by removing only429's absent replacement bytes in both physical producer families.

    @param configuration Matching current game/test build.
    @param proof_path Exact current prepared checkpoint without any replacement request or retirement receipt.
    @return Accepted independent legacy-upgrade report, comparing all thirty current native owner families.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration and proof["phase"] in ("prepared", "earth-prepared", "upgrade-clone")
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    version, raw = read_stream(source); assert version == 429; owners, jobs, bodies = layout(raw)
    assert all(not row["present"] for row in jobs + bodies)
    scope = f'{proof["phase"]}-{proof["model"]}'
    context = current_run_context("space", "manufacturing-replacement-legacy-" + scope)
    report = context.reports / "manufacturing-replacement-legacy.json"; assert not report.exists(); changed = bytearray(raw)
    # Remove only explicit absent-intent bytes, processing later original extents first so another native family's offsets never drift. All original paid receipts, packets, clone intent, native rules and work remain byte-identical.
    for family, rows in sorted(((owners[b"SPST"], jobs), (owners[b"SVST"], bodies)), key=lambda entry: entry[0]["frame"], reverse=True):
        payload = bytearray(raw[family["start"]:family["end"]])
        for row in sorted(rows, key=lambda row: row["presence"], reverse=True):
            assert payload[row["presence"] - family["start"]] == 0; del payload[row["presence"] - family["start"]]
        length = len(payload); assert length < 1 << 28
        changed[family["frame"]:family["end"]] = bytes([(length >> 24) << 4]) + (length & 0xffffff).to_bytes(3, "big") + payload
    old = context.saves / "replacement-legacy-428.sav"; old.write_bytes(b"OTTN" + struct.pack(">I", 428 << 16) + changed)
    owner = context.for_feature("space", "manufacturing-replacement-legacy-ordinary-" + scope)
    # The ordinary application performs the real versioned load/upgrade/save. No observer adds missing materials, work, identity, intent or sale credit.
    previous_configuration = os.environ.get("OTTD_TEST_CONFIGURATION"); os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(owner, {"name": "replacement-legacy-" + scope, "snapshot": identity(old)}, configuration)
    finally:
        if previous_configuration is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous_configuration
    upgraded = owner.saves / "ordinary-resave.sav"; assert chunks(upgraded)[0] == 429
    expected, actual = chunks(source)[1], chunks(upgraded)[1]
    for family in SHARED["EXACT"]:
        assert expected[family] == actual[family], (scope, family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; exact428 absence normalization in both SPST and SVST, followed by ordinary current native reconstruction without an inferred paid replacement.",
                  configuration=configuration, proof=identity(proof_path), source=proof["after"], legacy=identity(old), upgraded=identity(upgraded),
                  removed_spst_bytes=len(jobs), removed_svst_bytes=len(bodies), exact_owner_families=len(SHARED["EXACT"]),
                  native_reports=[identity(path) for path in sorted(owner.reports.glob("native-*.json"))])
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one exact accepted current job/body and one independent malformed ownership boundary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--owner", choices=("orbital", "earth"), required=True)
    parser.add_argument("--kind", choices=KINDS, required=True)
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.owner, args.kind).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
