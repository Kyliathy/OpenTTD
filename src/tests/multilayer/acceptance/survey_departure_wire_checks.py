"""Exercise source-era and active-lease refusal for original ownerless survey rows.

@file survey_departure_wire_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group26.
Copied ASTR faults retain every original family and genuine source version.
"""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from savegame_tables import chunks, read_stream
from asteroid_field_wire_checks import riff


def fixture(source, departure, target, case):
    """Make one explicitly malformed source without relabelling its version.

    @param source Genuine444 ownerless checkpoint before its actual test release.
    @param departure Current445-or-later native no-target Return from that source.
    @param target New repository test-cache file. @param case Older source, forged active target or orphaned lease.
    """
    old_version, old_stream = read_stream(source)
    new_version, new_stream = read_stream(departure)
    assert old_version == 444 and new_version >= 445 and not target.exists()
    old_owners, new_owners = chunks(old_stream), chunks(new_stream)
    old = next(owner["payload"] for owner in old_owners if owner["name"] == b"ASTR")
    payload = bytearray(next(owner["payload"] for owner in new_owners if owner["name"] == b"ASTR"))
    bodies = int.from_bytes(payload[27:31], "big")
    surveys = int.from_bytes(payload[31:35], "big")
    start = 51 + bodies * 40
    changed = None
    # Locate the actual released row by its explicit no-target discriminator,
    # retaining its original id/home/count/pose and all other survey records.
    for index in range(surveys):
        offset = start + 34 * index
        if payload[offset + 22:offset + 26] == bytes([255]) * 4:
            changed = offset
            break
    assert changed is not None and payload[changed + 26] == 2
    if case == "active":
        payload[changed + 26] = 0
    if case == "lease":
        original_target = int.from_bytes(old[changed + 22:changed + 26], "big")
        original_id = payload[changed:changed + 4]
        # Restore only the old original body's lease while its controller is
        # released. The reverse join must refuse this orphan in both readers.
        for offset in range(51, start, 40):
            if int.from_bytes(payload[offset:offset + 4], "big") == original_target:
                payload[offset + 32:offset + 36] = original_id
                break
        else:
            raise AssertionError("original released body must still exist in this pre-erasure component")
    owners, version = (old_owners, old_version) if case == "older" else (new_owners, new_version)
    target.write_bytes(b"OTTN" + (version << 16).to_bytes(4, "big") +
                       b"".join(riff(b"ASTR", payload) if owner["name"] == b"ASTR" else owner["raw"] for owner in owners) + bytes(4))


def main():
    """Run one paired full/preview refusal with no native simulation opportunities.

    @return First native failure or zero after both bounded separate processes.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--departure", required=True)
    parser.add_argument("--case", choices=("older", "active", "lease"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--run", required=True)
    args = parser.parse_args()
    source, departure = (ROOT / args.source).resolve(), (ROOT / args.departure).resolve()
    directory = ROOT / "test-cache/space-fixes/survey-wire-cases"
    directory.mkdir(parents=True, exist_ok=True)
    target = (directory / (args.run + ".sav")).resolve()
    assert source.is_relative_to(ROOT) and departure.is_relative_to(ROOT) and target.parent == directory.resolve()
    fixture(source, departure, target, args.case)
    accepted = source if args.case == "older" else departure
    # Full refusal tears down its own disposable world. Preview performs
    # valid-corrupt-valid against the unchanged actual accepted input.
    for mode in ("preview", "full"):
        command = [sys.executable, str(Path(__file__).with_name("space_fixes_checks.py")),
                   "--source", accepted.relative_to(ROOT).as_posix(), "--cargo-origin",
                   "--cargo-bad", target.relative_to(ROOT).as_posix(), "--configuration", args.configuration,
                   "--run", args.run + "-" + mode]
        if mode == "full":
            command.append("--cargo-full")
        result = subprocess.run(command, cwd=ROOT, check=False)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
