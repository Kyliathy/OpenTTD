"""Refuse newly supported empty-queue intent in an unchanged genuine443 source era.

@file legacy_departure_wire_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group25.
The source version is preserved. Only the existing VEHS/ASTR/SPOR owners receive
the corresponding native444 test departure; no fixture version is relabelled.
"""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from savegame_tables import chunks, read_stream


def forge_old(source, departure, target):
    """Transplant the same hull's native departure into the original443 framing.

    @param source Genuine443 saved world with an inert unordered legacy miner.
    @param departure Native444-or-later resave produced from that world's actual hull.
    @param target New repository test-cache copy used only for refusal checks.
    """
    old_version, old_stream = read_stream(source)
    new_version, new_stream = read_stream(departure)
    assert old_version == 443 and new_version >= 444 and not target.exists()
    old_owners, new_owners = chunks(old_stream), chunks(new_stream)
    current = {owner["name"]: owner for owner in new_owners}
    selected = {b"VEHS", b"ASTR", b"SPOR"}
    # These versions share every explicit continuation scalar and queue field.
    # Preserve the original table schema and all packet custody bytes; the only
    # capability being forged is the new unqueued local departure semantics.
    for owner in old_owners:
        if owner["name"] == b"VEHS":
            assert owner["rows"][0] == current[b"VEHS"]["rows"][0]
        if owner["name"] == b"CAPA":
            assert owner["raw"] == current[b"CAPA"]["raw"]
    target.write_bytes(b"OTTN" + (old_version << 16).to_bytes(4, "big") +
                       b"".join(current[owner["name"]]["raw"] if owner["name"] in selected
                                else owner["raw"] for owner in old_owners) + bytes(4))


def main():
    """Run paired full/preview refusal in separate bounded native processes.

    @return First failing native result, or zero after both source-era refusals.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--departure", required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--run", required=True)
    args = parser.parse_args()
    source, departure = (ROOT / args.source).resolve(), (ROOT / args.departure).resolve()
    directory = ROOT / "test-cache/space-fixes/legacy-wire-cases"
    directory.mkdir(parents=True, exist_ok=True)
    target = (directory / (args.run + ".sav")).resolve()
    assert source.is_relative_to(ROOT) and departure.is_relative_to(ROOT) and target.parent == directory.resolve()
    forge_old(source, departure, target)
    # Full refusal destroys only its disposable world. Preview checks the
    # valid443 source, corrupt copy, then the valid source again without ticks.
    for mode in ("preview", "full"):
        command = [sys.executable, str(Path(__file__).with_name("space_fixes_checks.py")),
                   "--source", source.relative_to(ROOT).as_posix(), "--cargo-origin",
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
