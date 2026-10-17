"""Pair independent clean full/copied refusals for contained heavy role, work, actual packets and shared berth.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC
Only invocation-owned malformed copies change; actual native SPST/SPSU/SPOR/CAPA framing selects every field.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-1.py")))
LAYOUT = SHARED["LAYOUT"]
MANIFEST = runpy.run_path(str(Path(__file__).with_name("heavy_supply_custody_checks-2.py")))
PACKETS = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-2.py")))
KINDS = ("old-version", "overwork", "wrong-role", "unknown-role", "allowance", "legacy-overwork",
         "metal-shortage", "electronics-shortage", "missing-packet", "duplicate-packet", "missing-berth", "orphan-role")


def damage(context, source, kind):
    """Change one original service invariant in an owned malformed native copy.

    @param context Exact current malformed-artifact owner.
    @param source Accepted heavy partial/lost-sender or genuinely upgraded old job.
    @param kind Independent work, immutable-role, packet or original berth violation.
    @return New malformed save preserving all unrelated native fields/owners.
    """
    version, raw = read_stream(source); assert version == 436; layout = LAYOUT["structure_rows"](raw, version); job = layout["rows"][0]["jobs"][0]; row = job["start"]; changed = bytearray(raw)
    if kind == "old-version":
        # This is explicitly a malformed future-capability candidate, not a genuine legacy producer: remove only the actual append, then older framing must still reject unavailable heavy work/bills.
        changed = LAYOUT["replace_family"](raw, b"SPST", LAYOUT["legacy_payload"](raw, require_unknown=False)); version = 435
    elif kind in ("overwork", "legacy-overwork"):
        changed[row + 12:row + 14] = (149 if kind == "legacy-overwork" else 297).to_bytes(2, "big")
    elif kind in ("wrong-role", "unknown-role", "orphan-role"):
        model = 65535 if kind == "unknown-role" else 20 if kind == "orphan-role" else 16; offset = job["model_offset"]; changed[offset:offset + 2] = model.to_bytes(2, "big")
    elif kind == "allowance":
        manifest = MANIFEST["manifest_row"](raw); assert raw[manifest + 40] == 0; changed[manifest + 40] = 1
    elif kind in ("metal-shortage", "electronics-shortage"):
        index = 0 if kind == "metal-shortage" else 1; part = job["parts"][index]; assert part["packet_offsets"]
        # A split original bill may own two one-unit packets or a single two-unit packet. Retain only one physical unit without creating a zero-count native packet that could assert before clean load admission.
        if part["count"] == 1:
            position = part["packet_offsets"][0]; packet = int.from_bytes(raw[position:position + 4], "big"); offset = PACKETS["packet_count_offset"](raw, packet)
            assert int.from_bytes(raw[offset:offset + 2], "big") == 2; changed[offset:offset + 2] = (1).to_bytes(2, "big")
        else:
            assert part["count"] == 2; changed[part["count_offset"]:part["count_offset"] + 2] = (1).to_bytes(2, "big")
            payload = bytearray(changed[layout["body"]:layout["end"]]); offset = part["packet_offsets"][-1] - layout["body"]; del payload[offset:offset + 6]
            changed = LAYOUT["replace_family"](raw, b"SPST", payload)
    elif kind == "missing-packet":
        position = job["parts"][0]["packet_offsets"][0]; changed[position:position + 4] = (2**20 - 2).to_bytes(4, "big")
    elif kind == "duplicate-packet":
        left, right = job["parts"][0]["packet_offsets"][0], job["parts"][1]["packet_offsets"][0]; changed[right:right + 4] = raw[left:left + 4]
    else:
        assert kind == "missing-berth"; position = LAYOUT["approach_slot_offset"](raw); assert raw[position] == 0; changed[position] = 1
    assert changed != raw or version != 436; target = context.saves / ("malformed-heavy-contained-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def verify(configuration, proofs, kind):
    """Require independent clean full startup and detached valid-world preview to refuse one same-file candidate.

    @param configuration Matching actual build.
    @param proofs Explicit accepted current native phase-to-report membership.
    @param kind Exact independent malformed boundary.
    @return Immutable paired-refusal proof.
    """
    phase = "legacy-upgrade" if kind == "legacy-overwork" else "sender-lost" if kind == "orphan-role" else "partial"
    selected = proofs[phase]; proof = SHARED["read"](selected); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "heavy-contained-refusal-" + kind); malformed = damage(context, source, kind)
    full = context.for_feature("space", "heavy-contained-full-" + kind); preview = context.for_feature("space", "heavy-contained-preview-" + kind)
    SHARED["native"](full, configuration, "corrupt", malformed, reject=True); assertions = SHARED["native"](preview, configuration, "corrupt", source, corrupt=malformed)
    assert identity(source) == proof["after"]; target = context.reports / "heavy-contained-refusal.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; independent clean full/copied role/bill/work/packet/berth refusal with original active-world money/RNG/physical authority unchanged.",
                                     configuration=configuration, kind=kind, proof=identity(selected), source=proof["after"], malformed=identity(malformed), assertions=assertions,
                                     native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Collect six independently paired receipt or packet/berth faults per focused burst."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", nargs=3, type=Path, required=True); parser.add_argument("--part", choices=("receipts", "joins"), required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); proofs = {}; reports = []
    # Exact three native scopes supply the actual heavy, orphan and genuine-old boundaries; no unrelated successful save or current relabelled producer supplies a candidate.
    for selector in args.selector:
        selected = SHARED["read"](selector); assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"].startswith("native-")
        for member in selected["reports"]:
            path = ROOT / member; phase = SHARED["read"](path)["phase"]; assert phase not in proofs; proofs[phase] = path
    assert set(proofs) == {"focused"} | set(SHARED["PARENTS"])
    for kind in KINDS[:6] if args.part == "receipts" else KINDS[6:]:
        reports.append(verify(args.configuration, proofs, kind)); print(args.configuration, kind, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; explicit independent clean malformed-reader membership.",
                                          configuration=args.configuration, scope="rejects-" + args.part, complete=True, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
