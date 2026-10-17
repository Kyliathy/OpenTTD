"""Pair independent full/copy refusals for heavy supply source admission and original shell custody.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB
Only invocation-owned malformed copies are changed; native framing and exact paid identities select each field.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_supply_custody_checks-1.py")))
KINDS = ("future-flight", "future-supply", "future-history", "premature-service", "pod-position", "pod-owner", "pod-surface", "pod-generation", "duplicate-shell", "unpaid-shell")


def manifest_row(raw, vehicle=22, generation=8):
    """Locate one complete original native compartment owner through exact bounded SPSU framing.

    @param raw Decompressed original native stream.
    @param vehicle Original paid native VehicleID.
    @param generation Original paid native incarnation.
    @return Absolute original row offset, with every preceding variable compartment admitted.
    """
    _, body, _, end = riff_location(raw, b"SPSU"); count = raw[body]; position = body + 1; selected = []
    key = vehicle.to_bytes(4, "big") + generation.to_bytes(8, "big"); assert count <= 64
    # Each shell and compartment has its actual saved extent; payload bytes or a reused numeric vehicle cannot impersonate the selected manifest.
    for _ in range(count):
        row = position; assert row + 61 <= end; pods = raw[row + 60]; assert pods <= 8; position += 61 + 10 * pods
        for _ in range(3):
            assert position < end; packets = raw[position]; assert packets <= 128; position += 1 + 6 * packets
        assert position <= end
        if raw[row:row + 12] == key:
            selected.append(row)
    assert position == end and len(selected) == 1; return selected[0]


def carried_pod_row(raw):
    """Select a real original shell currently carried by the exact paid heavy receiver.

    @param raw Decompressed accepted delivered native stream.
    @return Absolute fixed65-byte shell row offset.
    """
    _, body, length, end = riff_location(raw, b"SPOD"); count = int.from_bytes(raw[body + 8:body + 10], "big"); assert length == 10 + 65 * count
    selected = []
    # Only the canonical custody byte and exact generation-qualified carrier select a shell; all rows retain their original paid Surface provenance.
    for index in range(count):
        row = body + 10 + 65 * index; assert row + 65 <= end
        if raw[row + 11] == 2 and raw[row + 32:row + 44] == (22).to_bytes(4, "big") + (8).to_bytes(8, "big"):
            selected.append(row)
    assert len(selected) == 2; return selected[0]


def damage(context, source, kind):
    """Break one paid supply or custody invariant in a fresh explicitly owned native copy.

    @param context Current invocation's malformed-artifact owner.
    @param source Exact accepted ascent/delivered source or genuine432 empty-heavy input.
    @param kind Independent capability, physical custody or paid receipt fault.
    @return New malformed save preserving actual original native transport framing.
    """
    version, raw = read_stream(source); changed = bytearray(raw)
    if kind in ("future-flight", "future-supply"):
        assert version == 433; version = 432
    elif kind in ("future-history", "premature-service"):
        assert version == (432 if kind == "future-history" else 433); row = manifest_row(raw); assert int.from_bytes(raw[row + 52:row + 60], "big") == 0
        changed[row + 52:row + 60] = (1).to_bytes(8, "big")
    elif kind == "duplicate-shell":
        assert version == 433; row = manifest_row(raw); assert raw[row + 60] == 2
        changed[row + 71:row + 81] = raw[row + 61:row + 71]
    else:
        assert version == 433; row = carried_pod_row(raw)
        if kind == "pod-position":
            position = row + 48; changed[position:position + 4] = (int.from_bytes(raw[position:position + 4], "big") + 1).to_bytes(4, "big")
        elif kind == "pod-owner":
            assert raw[row + 10] == 0; changed[row + 10] = 1
        elif kind == "pod-surface":
            changed[row + 44:row + 48] = (2**32 - 1).to_bytes(4, "big")
        elif kind == "pod-generation":
            assert int.from_bytes(raw[row + 36:row + 44], "big") == 8; changed[row + 36:row + 44] = (9).to_bytes(8, "big")
        else:
            assert kind == "unpaid-shell" and int.from_bytes(raw[row + 60:row + 64], "big") == 2000
            changed[row + 60:row + 64] = (1999).to_bytes(4, "big")
    assert changed != raw or version == 432
    target = context.saves / ("malformed-heavy-supply-" + kind + ".sav"); target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def verify(configuration, proofs, kind):
    """Require independent real full startup and copied preview to reject the same physical fault.

    @param configuration Matching current actual native configuration.
    @param proofs Explicit accepted milestone-to-report mapping.
    @param kind Independent source or custody violation.
    @return Immutable paired-reader refusal report.
    """
    selected = proofs["ascent" if kind == "future-flight" else "delivered"]; proof = SHARED["read"](selected)
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]; predecessor = identity(selected); malformed_source = source
    if kind == "future-history":
        predecessor, original = SHARED["entry"](configuration); malformed_source = ROOT / original["path"]; assert identity(malformed_source) == original
    context = current_run_context("space", "heavy-supply-custody-refusal-" + kind); malformed = damage(context, malformed_source, kind)
    full = context.for_feature("space", "heavy-supply-custody-full-" + kind); preview = context.for_feature("space", "heavy-supply-custody-preview-" + kind)
    SHARED["native"](full, configuration, "corrupt", malformed, reject=True)
    assertions = SHARED["native"](preview, configuration, "corrupt", source, corrupt=malformed); assert identity(source) == proof["after"]
    target = context.reports / "heavy-supply-custody-refusal.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; independent full/copied admission with original paid active-world owners retained.",
                                     configuration=configuration, kind=kind, proof=identity(selected), source=proof["after"], predecessor=predecessor, malformed_source=identity(malformed_source),
                                     malformed=identity(malformed), assertions=assertions,
                                     native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Run only the bounded active supply/custody refusal burst using explicit accepted native membership."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); selected = SHARED["read"](args.selector)
    assert selected["configuration"] == args.configuration and selected["complete"]; proofs = {SHARED["read"](ROOT / path)["phase"]: ROOT / path for path in selected["reports"]}; reports = []
    # Every fault gets its own independent full reader and copied reader; an earlier rejected candidate supplies no evidence for a different physical invariant.
    for kind in KINDS:
        reports.append(verify(args.configuration, proofs, kind)); print(args.configuration, kind, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; explicit paired supply/custody refusal membership.",
                                          configuration=args.configuration, scope="rejects", complete=True, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
