"""Pair independent actual full/copied refusals for new heavy descent receipts and packet joins.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB
Every malformed candidate is an invocation-owned copy selected through exact SPDJ/native framing. It never replaces a paid producer or genuine legacy input.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, Reader, CHUNK_RIFF, CHUNK_RECORDS, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-1.py")))
NATIVE_FIELDS = runpy.run_path(str(Path(__file__).with_name("heavy_mining_runtime_checks-2.py")))["native_fields"]
KINDS = ("future-source", "sender-role", "small-mass", "unit-weight", "freight-mass", "quantity", "sender-owner", "missing-packet", "duplicate-packet", "packet-count", "coherent-quantity", "future-sender")


def descent(raw):
    """Locate exactly one actual current partial heavy job.

    @param raw Original decompressed434 stream.
    @return Complete SPDJ RIFF extent and original row start.
    """
    start, body, length, end = riff_location(raw, b"SPDJ"); row = body + 10
    assert int.from_bytes(raw[body + 8:body + 10], "big") == 1 and raw[row + 36:row + 48] == (22).to_bytes(4, "big") + (8).to_bytes(8, "big")
    assert raw[row + 56:row + 61] == b"\0\x15\x10\x02\x80" and int.from_bytes(raw[row + 49:row + 51], "big") == 40
    assert row + 62 + 6 * raw[row + 61] == end and length == 10 + 62 + 6 * raw[row + 61]
    return start, body, end, row


def packet_count_offset(raw, packet):
    """Read the original CAPA self-description and native dense/sparse row identity.

    @param raw Exact decompressed current stream.
    @param packet Original raw CargoPacketID retained by the descent.
    @return Absolute original count-field offset, without searching arbitrary packet bytes.
    """
    reader = Reader(raw)
    # Original chunk framing supplies every extent; no cargo value or string payload may impersonate the packet table.
    while True:
        family = reader.take(4); assert family != bytes(4); mode = reader.byte(); kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte(); reader.take(length); continue
        assert kind in CHUNK_RECORDS
        if family != b"CAPA":
            while True:
                extent = reader.gamma()
                if not extent:
                    break
                reader.take(extent - 1)
            continue
        assert kind in (3, 4); extent = reader.gamma(); schema = Reader(reader.take(extent - 1)); fields = []
        while True:
            tag = schema.byte()
            if not tag:
                break
            name = schema.take(schema.gamma()).decode("utf-8"); assert tag & 16 == 0
            width = {1: 1, 2: 1, 3: 2, 4: 2, 5: 4, 6: 4, 7: 8, 8: 8, 9: 2}[tag & 15]; fields.append((name, width))
        assert schema.pos == len(schema.data); dense = 0
        # Native Table rows include empty index gaps; sparse tables instead carry their own gamma index. Only the actual original identity can supply this receipt's physical count.
        while True:
            extent = reader.gamma(); assert extent != 0; absolute = reader.pos; row = Reader(reader.take(extent - 1)); index = row.gamma() if kind == 4 else dense; dense += 1
            if index != packet:
                continue
            for name, width in fields:
                if name == "count":
                    assert width == 2; return absolute + row.pos
                row.take(width)


def damage(context, source, kind):
    """Break one actual saved invariant in an owned malformed input.

    @param context Explicit malformed-artifact owner.
    @param source Exact accepted434 partial heavy descent.
    @param kind Independent source, receipt, packet or sender violation.
    @return Newly retained malformed save with real surrounding native framing.
    """
    version, raw = read_stream(source); assert version == 434; start, body, end, row = descent(raw); changed = bytearray(raw)
    if kind == "future-source":
        # Reframe only SPDJ to the real old layout, isolating unavailable heavy pending descent rather than depending on misread new bytes. This is deliberately malformed future capability, never a genuine old producer.
        payload = raw[body:row + 56] + raw[row + 61:end]; changed = bytearray(raw[:start] + b"SPDJ" + len(payload).to_bytes(4, "big") + payload + raw[end:]); version = 433
    elif kind == "sender-role":
        changed[row + 56:row + 58] = (16).to_bytes(2, "big")
    elif kind == "small-mass":
        changed[row + 56:row + 58] = (15).to_bytes(2, "big")
    elif kind == "unit-weight":
        changed[row + 58] = 8
    elif kind == "freight-mass":
        changed[row + 59:row + 61] = (639).to_bytes(2, "big")
    elif kind in ("quantity", "coherent-quantity"):
        changed[row + 49:row + 51] = (39).to_bytes(2, "big")
        if kind == "coherent-quantity":
            changed[row + 59:row + 61] = (624).to_bytes(2, "big")
    elif kind == "missing-packet":
        changed[row + 62:row + 66] = (2**20 - 2).to_bytes(4, "big")
    elif kind == "duplicate-packet":
        assert raw[row + 61] > 1; changed[row + 68:row + 72] = raw[row + 62:row + 66]
    elif kind == "packet-count":
        packet = int.from_bytes(raw[row + 62:row + 66], "big"); offset = packet_count_offset(raw, packet); count = int.from_bytes(raw[offset:offset + 2], "big"); assert count > 0
        changed[offset:offset + 2] = (count + 1).to_bytes(2, "big")
    elif kind == "future-sender":
        changed[row + 40:row + 48] = (2**64 - 1).to_bytes(8, "big")
    else:
        assert kind == "sender-owner"; offset, width = NATIVE_FIELDS(raw, 22)["space.common.owner"]; assert width == 1 and raw[offset] == 0; changed[offset] = 1
    assert changed != raw or version != 434
    target = context.saves / ("malformed-heavy-descent-" + kind + ".sav"); target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def verify(configuration, proof_path, kind):
    """Require independent real full load and valid-world copied preview to reject the same fault.

    @param configuration Matching current build.
    @param proof_path Exact accepted partial heavy descent report.
    @param kind Independent new physical admission fault.
    @return Immutable paired-refusal proof path.
    """
    proof = SHARED["read"](proof_path); assert proof["configuration"] == configuration and proof["phase"] == "partial"
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "heavy-descent-refusal-" + kind); malformed = damage(context, source, kind)
    full = context.for_feature("space", "heavy-descent-full-" + kind); preview = context.for_feature("space", "heavy-descent-preview-" + kind)
    SHARED["native"](full, configuration, "corrupt", malformed, reject=True); assertions = SHARED["native"](preview, configuration, "corrupt", source, corrupt=malformed)
    assert identity(source) == proof["after"]; target = context.reports / "heavy-descent-refusal.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; actual independently paired readers with original active-world owners, money and RNG preserved.",
                                     configuration=configuration, kind=kind, proof=identity(proof_path), source=proof["after"], malformed=identity(malformed), assertions=assertions,
                                     native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run one bounded subset of the active receipt/packet malformed-admission burst."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--part", choices=("receipts", "joins"), required=True)
    args = parser.parse_args(); assert not args.output.exists(); selected = SHARED["read"](args.selector); assert selected["configuration"] == args.configuration and selected["complete"]
    partial = [ROOT / path for path in selected["reports"] if SHARED["read"](ROOT / path)["phase"] == "partial"]; assert len(partial) == 1; reports = []
    # Six pairs per subprocess keep each native burst bounded while preserving explicit complete membership across the two selectors.
    for kind in KINDS[:6] if args.part == "receipts" else KINDS[6:]:
        reports.append(verify(args.configuration, partial[0], kind)); print(args.configuration, kind, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; explicit independent paired reader membership.",
                                          configuration=args.configuration, scope="rejects-" + args.part, complete=True, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
