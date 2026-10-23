"""Reject one malformed424 initial-input payload through full native load and read-only preview.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
Each bounded invocation handles one corruption so full-load and preview failures
stay independently attributable and do not become a broad regression batch.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context, run_native


def damaged_copy(context, source, kind):
    """Change only an invocation-owned ASTR framing/input copy, preserving the genuine source.

    @param context Explicit corruption artifact owner.
    @param source Genuine424 field save whose bytes remain immutable.
    @param kind One named framing or initial-input error.
    @return Uncompressed native corruption copy path.
    """
    version, raw = read_stream(source)
    assert version == 424
    start, body, length, end = riff_location(raw, b"ASTR")
    payload = bytearray(raw[body:end])
    trailer = len(payload) - 26
    if kind == "kind":
        payload[trailer] = 4
    elif kind == "coordinates":
        payload[trailer + 1:trailer + 5] = bytes.fromhex("ffffffff") if payload[trailer] == 3 else bytes(4)
    elif kind == "bound":
        payload[-8:] = bytes.fromhex("ffffffffffffffff")
    elif kind == "legacy":
        payload[trailer] = 0
    elif kind == "short":
        payload = payload[:-1]
    elif kind == "extra":
        payload += b"\x00"
    else:
        assert kind == "duplicate"
    # Reframe only this bounded RIFF owner; all unrelated native families retain their exact source bytes.
    header = b"ASTR" + bytes([(raw[start + 4] & 15) | ((len(payload) >> 24) << 4)]) + (len(payload) & 0xffffff).to_bytes(3, "big")
    changed = raw[:start] + header + payload + raw[end:]
    if kind == "duplicate":
        changed = raw[:end] + raw[start:end] + raw[end:]
    target = context.saves / ("asteroid-input-corrupt-" + kind + ".sav")
    target.write_bytes(b"OTTN" + (version << 16).to_bytes(4, "big") + changed)
    return target


def verify(configuration, source, kind):
    """Reject one corruption in both native loading paths without mutating the accepted current field.

    @param configuration Built configuration whose current loader is tested.
    @param source Exact genuine424 input save.
    @param kind Named malformed input/framing condition.
    @return Accepted rejection report path with exact native/source identities.
    """
    context = current_run_context("space", "asteroid-input-reject-" + kind)
    original = identity(source)
    corrupt = damaged_copy(context, source, kind)
    reports = []
    # Full load must fail before publication; preview must fail while a separately loaded genuine world stays unchanged.
    for preview in (False, True):
        owner = context.for_feature("space", "asteroid-input-preview-" + kind if preview else "asteroid-input-full-" + kind)
        owner.prepare_runtime()
        (owner.runtime_dir / "scripts/game_start.scr").write_text("asteroid_starter_test\n", encoding="utf-8")
        environment = dict(os.environ, OTTD_ASTEROID_STARTER_SOURCE=str(source if preview else corrupt))
        if preview:
            environment["OTTD_ASTEROID_STARTER_CORRUPT"] = str(corrupt)
        else:
            environment.pop("OTTD_ASTEROID_STARTER_CORRUPT", None)
        log = owner.logs / "asteroid-input-rejection.log"
        with log.open("wb") as sink:
            result = run_native(owner, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.asteroid-generation-native]"],
                                cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=45)
        evidence = log.read_text(encoding="utf-8", errors="replace")
        if preview:
            assert result.returncode == 0 and "preview_reject=true unchanged=true" in evidence, log
        else:
            assert result.returncode != 0 and "All tests passed" not in evidence and "asteroid" in evidence.lower(), log
        reports.append({"preview": preview, "log": identity(log), "native_reports": [identity(path) for path in sorted(owner.reports.glob("native-*.json"))]})
    assert identity(source) == original
    report = context.reports / "asteroid-input-rejection.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs10 Group2; schema424 input/framing corruption refusal and read-only preview nonmutation.",
        "configuration": configuration, "kind": kind, "source": original, "corrupt": identity(corrupt), "results": reports}, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one explicit genuine source and corruption case, then print its accepted report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--kind", choices=("kind", "coordinates", "bound", "legacy", "short", "extra", "duplicate"), required=True)
    args = parser.parse_args()
    print(verify(args.configuration, args.source.resolve(), args.kind).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
