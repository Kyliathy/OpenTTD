"""Verify independent full/copied-preview commissioning provenance and framing refusals.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9B
Malformed copies use decoded native owner offsets; immutable accepted inputs stay unchanged.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, chunks, identity, read_stream
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_commissioning_checks-1.py")))
LAYOUT = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-3.py")))["layout"]
CASES = {"sdef-old-current": "upgrade", "sdef-signature": "upgrade", "future-job-old": "before", "future-hull-old": "complete",
         "future-frame-old": "upgrade", "heavy-old-receipt": "complete", "wrong-native-role": "complete", "unpaid-capital": "queued",
         "short-input": "before", "future-product-generation": "complete", "unsupported-frame-content": "upgrade"}


def damage(context, source, kind):
    """Change one explicit owner invariant in an invocation-owned malformed native copy.

    @param context Current malformed-copy artifact owner.
    @param source Exact accepted valid current431 checkpoint.
    @param kind Independent content, source-version, receipt, input or product identity fault.
    @return New malformed copy retaining original native framing.
    """
    version, raw = read_stream(source); assert version == 431; owners, jobs, bodies = LAYOUT(raw); changed = bytearray(raw)
    sdef = owners[b"SDEF"]["start"]; assert owners[b"SDEF"]["end"] - sdef == 10
    if kind == "sdef-signature":
        changed[sdef + 9] ^= 1
    elif kind == "sdef-old-current":
        changed[sdef:sdef + 10] = bytes.fromhex("0002a1608eff08bf795b")
    elif kind in ("future-job-old", "future-hull-old", "future-frame-old"):
        version = 430; changed[sdef:sdef + 10] = bytes.fromhex("0002a1608eff08bf795b")
        if kind == "future-hull-old":
            assert len(jobs) == 1 and jobs[0]["model"] == 21
            # The old construction16 uses exactly this full heavy bill. Removing the future job role isolates future native hull-receipt admission instead of relying on the earlier SPST job rejection.
            position = jobs[0]["phase_offset"] + 1; changed[position:position + 2] = (16).to_bytes(2, "big")
        elif kind == "future-frame-old":
            position = owners[b"SPST"]["start"] + 42 + 17; assert changed[position:position + 2] == bytes.fromhex("0002")
            changed[position:position + 2] = (3).to_bytes(2, "big")
    elif kind in ("heavy-old-receipt", "wrong-native-role"):
        candidates = [row for row in bodies if row["model"] == 21]; assert len(candidates) == 1; row = candidates[0]
        position = row["work"] + (2 if kind == "heavy-old-receipt" else 4)
        changed[position:position + 2] = (2 if kind == "heavy-old-receipt" else 16).to_bytes(2, "big")
    elif kind == "unsupported-frame-content":
        position = owners[b"SPST"]["start"] + 42 + 17; changed[position:position + 2] = (4).to_bytes(2, "big")
    else:
        assert len(jobs) == 1 and jobs[0]["model"] == 21; job = jobs[0]
        if kind == "unpaid-capital":
            position = job["phase_offset"] + 21; assert int.from_bytes(changed[position:position + 4], "big") == 200000
            changed[position:position + 4] = bytes(4)
        elif kind == "short-input":
            position = job["phase_offset"] + 34; assert int.from_bytes(changed[position:position + 2], "big") == 6
            changed[position:position + 2] = (5).to_bytes(2, "big")
        elif kind == "future-product-generation":
            position = job["product"] + 4; assert int.from_bytes(changed[position:position + 8], "big") > 0
            changed[position:position + 8] = (2**64 - 1).to_bytes(8, "big")
        else:
            raise AssertionError(kind)
    assert bytes(changed) != raw or version != 431
    target = context.saves / ("malformed-heavy-" + kind + ".sav"); target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def verify(configuration, proof_path, kind):
    """Independently reject the same malformed copy through full startup and active-world copied preview.

    @param configuration Matching completed current native test build.
    @param proof_path Exact accepted original checkpoint suitable for this fault.
    @param kind One explicit independent invariant violation.
    @return Current immutable paired-refusal proof.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration and proof["phase"] == CASES[kind]
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "heavy-commissioning-refusal-" + kind); malformed = damage(context, source, kind)
    full = context.for_feature("space", "heavy-commissioning-full-" + kind); preview = context.for_feature("space", "heavy-commissioning-preview-" + kind)
    SHARED["native"](full, configuration, "corrupt", malformed, reject=True)
    assertions, _ = SHARED["native"](preview, configuration, "corrupt", source, corrupt=malformed)
    assert identity(source) == proof["after"]
    target = context.reports / "heavy-commissioning-refusal.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; independent full/copy admission refusal with active native physical owners unchanged.",
                                     configuration=configuration, kind=kind, proof=identity(proof_path), source=proof["after"], malformed=identity(malformed), assertions=assertions,
                                     native_reports=[identity(path) for item in (full, preview) for path in sorted(item.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run the active commissioning malformed-admission burst with explicit native predecessor membership."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); content = json.loads(args.selector.read_text(encoding="utf-8"))
    assert content["configuration"] == args.configuration and content["complete"] and len(content["reports"]) == 9
    states = {json.loads((ROOT / member).read_text(encoding="utf-8"))["phase"]: ROOT / member for member in content["reports"]}; reports = []
    # Every fault independently loads the real valid original for preview and the malformed copy for full refusal; no failed check can stand in for the other reader.
    for kind, phase in CASES.items():
        reports.append(verify(args.configuration, states[phase], kind)); print(args.configuration, kind, flush=True)
    value = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; explicit paired full/copied-preview refusal membership.",
                 configuration=args.configuration, scope="rejects", complete=True, reports=[path.relative_to(ROOT).as_posix() for path in reports])
    args.output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
