"""Independently pair clean full/copied refusals for integrated actual heavy role, content, progress and packet ownership.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9DC
Only invocation-owned malformed copies change. Source437 receipt framing, native scalar fields and actual CAPA references select each fault.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import struct
import subprocess

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context, run_native

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_network_checks-1.py")))
TABLES = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-2.py")))
FIELDS = runpy.run_path(str(Path(__file__).with_name("heavy_mining_runtime_checks-2.py")))
CASES = {"role": ("service", "after"), "content": ("mining", "after"), "progress": ("mining", "complete"), "owner": ("descent", "after")}


def damage(context, source, kind):
    """Break exactly one actual heavy admission invariant in a retained malformed copy.

    @param context Invocation-owned save/log directories.
    @param source Exact accepted current heavy native checkpoint.
    @param kind Original role bill, paid content provenance, partial work or exclusive packet ownership fault.
    @return New malformed save with all unrelated owner framing retained.
    """
    version, raw = read_stream(source); assert version == 437; changed = bytearray(raw)
    if kind == "role":
        job = SHARED["CYCLE"]["JOINED"]["LAYOUT"]["structure_rows"](raw, 436)["rows"][4]["jobs"][0]; assert job["model"] == 21 and job["ticks"] == 242
        changed[job["model_offset"]:job["model_offset"] + 2] = (15).to_bytes(2, "big")
    elif kind == "content":
        _, body, _, end = riff_location(raw, b"SVST"); count = raw[body]; assert count <= 64; position = body + 1; matched = 0
        # Every original67-byte paid prefix has its own strict optional retirement append. Walk exact row extents so another hull's receipt cannot impersonate heavy22/generation8.
        for _ in range(count):
            assert position + 68 <= end
            if raw[position:position + 12] == (22).to_bytes(4, "big") + (8).to_bytes(8, "big"):
                assert raw[position + 39:position + 43] == b"\0\3\0\x15"; changed[position + 39:position + 41] = (2).to_bytes(2, "big"); matched += 1
            present = raw[position + 67]; assert present <= 1; position += 68 + 20 * present
        assert position == end and matched == 1
    elif kind == "progress":
        offset, width = FIELDS["native_fields"](raw, 22)["space.mission.work_ticks"]; assert width == 2 and int.from_bytes(raw[offset:offset + width], "big") == 37
        changed[offset:offset + width] = (74).to_bytes(2, "big")
    else:
        assert kind == "owner"; _, body, _, _ = riff_location(raw, b"SPDJ"); row = body + 10; assert int.from_bytes(raw[body + 8:body + 10], "big") == 1 and raw[row + 61] > 0
        packet = int.from_bytes(raw[row + 62:row + 66], "big"); changed = TABLES["alias_native"](raw, b"STNN", packet)
    assert changed != raw; target = context.saves / ("malformed-heavy-network-" + kind + ".sav"); target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def admission(context, configuration, source, corrupt=None):
    """Invoke actual independent full startup or detached copied-preview refusal.

    @param context Invocation-owned process/log directories.
    @param configuration Matching current native test build.
    @param source Malformed startup candidate, or unchanged valid world for copied inspection.
    @param corrupt Optional same malformed candidate checked inside the valid original world.
    @return Passing copied assertions and actual reported rejection reason; full refusal returns zero assertions.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_joined_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_JOINED_SOURCE=str(source)); environment.pop("OTTD_HEAVY_JOINED_CAPTURE", None); environment.pop("OTTD_HEAVY_JOINED_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_JOINED_CORRUPT"] = str(corrupt)
    log = context.logs / "heavy-joined.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-joined-native]"], cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=30)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if corrupt is None:
        assert result.returncode == 1 and not match and "Broken savegame" in evidence and "HEAVY_JOINED native=true" not in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence, log
        return 0, "clean full-load refusal"
    assert result.returncode == 0 and match and int(match[2]) == 1 and "HEAVY_JOINED preview_reject=true authority_unchanged=true" in evidence, log
    reason = re.search(r"HEAVY_JOINED preview_error=(.+)", evidence); assert reason, log; return int(match[1]), reason[1]


def main():
    """Pair each exact malformed candidate in independent clean full and valid-world copied processes."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True); parser.add_argument("--selector", nargs=2, type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); states = {}; reports = []
    # Only explicitly accepted local native membership supplies each actual fault source; network/ordinary copies cannot hide a missing original candidate.
    for selector, scope in zip(args.selector, ("native-a", "native-b"), strict=True):
        selected = SHARED["read"](selector); assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"] == scope
        for member in selected["reports"]:
            proof = SHARED["read"](ROOT / member)
            if proof["mode"] == "local":
                assert proof["case"] not in states; states[proof["case"]] = ROOT / member
    # Each fault gets two fresh processes against the same owned malformed bytes. A successful copied refusal never substitutes for a clean full-startup refusal.
    for kind, (case, field) in CASES.items():
        proof_path = states[case]; proof = SHARED["read"](proof_path); source = ROOT / proof[field]["path"]; assert identity(source) == proof[field]
        context = current_run_context("space", "heavy-network-refusal-" + kind); malformed = damage(context, source, kind); full = context.for_feature("space", "heavy-network-full-" + kind); preview = context.for_feature("space", "heavy-network-preview-" + kind)
        admission(full, args.configuration, malformed); assertions, reason = admission(preview, args.configuration, source, corrupt=malformed); assert identity(source) == proof[field]; target = context.reports / "heavy-network-refusal.json"
        target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9DC; same actual malformed role/content/progress/owner input independently refused by clean full and copied readers, retaining valid-world authority/money/RNG.",
                                         configuration=args.configuration, kind=kind, proof=identity(proof_path), source=proof[field], malformed=identity(malformed), preview_reason=reason, assertions=assertions,
                                         native_reports=[identity(p) for owner in (full, preview) for p in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
        reports.append(target); print(args.configuration, "rejected", kind, reason, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9DC; four independent clean full/copied refusal pairs.", configuration=args.configuration, scope="rejects", complete=True, reports=[p.relative_to(ROOT).as_posix() for p in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
