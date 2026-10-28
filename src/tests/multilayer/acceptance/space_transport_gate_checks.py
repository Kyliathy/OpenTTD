"""Check joined current source/field/mission refusal with a distinct world open.

@file space_transport_gate_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups24/28.
Every mutation is a named family in a new repository test-cache copy. Retained
inputs, current-world owners and genuine version headers remain unchanged.
"""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from savegame_tables import Reader, chunks, read_stream, table_header
from asteroid_cargo_wire_checks import gamma, malformed
from asteroid_field_wire_checks import riff
from space_mining_wire_checks import fixture as mining_fixture, header_bytes


def make_bad(source, target, case):
    """Create one explicit same-file dependency fault, preserving every unrelated owner.

    @param source Valid current candidate. @param target New invocation-owned file.
    @param case Explicit source/field/mission/company/schema or size-credit history fault.
    """
    if case == "source":
        malformed(source, target, "missing-mineral")
        return
    if case == "mission":
        mining_fixture(source, target, "wrong-effective")
        return
    version, stream = read_stream(source)
    assert version >= 443 and not target.exists()
    owners = chunks(stream)
    if case in ("known-cost", "bootstrap", "weight-inflation"):
        asteroid = bytearray(next(owner["payload"] for owner in owners if owner["name"] == b"ASTR"))
        count = int.from_bytes(asteroid[27:31], "big"); footer = len(asteroid) - 62
        records = [asteroid[index:index + 40] for index in range(51, 51 + count * 40, 40)]
        if case == "known-cost":
            # A genuine original supplies the immutable initial population and
            # canonical Large record; no generated field or guessed reserve is
            # substituted. Coarse total cost can hide this known underpayment.
            _, original_stream = read_stream(ROOT / "src/tests/fixtures/demos/space/space-mining-holds-405.sav")
            original = next(owner["payload"] for owner in chunks(original_stream) if owner["name"] == b"ASTR")
            assert asteroid[:18] == original[:18] and asteroid[26] == original[26]
            initial = int.from_bytes(original[27:31], "big")
            large = next(original[index:index + 40] for index in range(51, 51 + initial * 40, 40) if original[index + 5] == 2)
            index = next(index for index, row in enumerate(records) if int.from_bytes(row[:4], "big") == 70000)
            replacement = bytearray(large); replacement[:4] = (70000).to_bytes(4, "big")
            replacement[32:36] = (2**32 - 1).to_bytes(4, "big"); replacement[36:40] = (2**20 - 1).to_bytes(4, "big")
            asteroid[51 + index * 40:91 + index * 40] = replacement
            earned = sum(int.from_bytes(asteroid[footer + 7 + size * 8:footer + 15 + size * 8], "big") * weight for size, weight in enumerate((1, 3, 5)))
            spawned = int.from_bytes(asteroid[footer + 2:footer + 6], "big") - initial
            asteroid[footer + 31:footer + 39] = (earned - spawned).to_bytes(8, "big")
        elif case == "weight-inflation":
            # Two alleged cheap historical births refund as Large exits. Their
            # coarse count/cost bounds close, but retired replacements cannot
            # add eight net units to the original retired-size weight budget.
            issued = int.from_bytes(asteroid[footer + 2:footer + 6], "big")
            large = int.from_bytes(asteroid[footer + 23:footer + 31], "big")
            credit = int.from_bytes(asteroid[footer + 31:footer + 39], "big")
            asteroid[footer + 2:footer + 6] = (issued + 2).to_bytes(4, "big")
            asteroid[footer + 23:footer + 31] = (large + 2).to_bytes(8, "big")
            asteroid[footer + 31:footer + 39] = (credit + 8).to_bytes(8, "big")
            asteroid[footer + 39:footer + 42] = bytes([1, 2, 0])
        else:
            # All originals remain, so no real initial terminal event could
            # fund any of this algebraically balanced renewal-only history.
            assert int.from_bytes(asteroid[footer + 2:footer + 6], "big") == count
            body = bytearray(next(row for row in records if row[5] == 0)); body[:4] = (70000).to_bytes(4, "big")
            body[32:36] = (2**32 - 1).to_bytes(4, "big"); body[36:40] = (2**20 - 1).to_bytes(4, "big")
            asteroid[51 + count * 40:51 + count * 40] = body; footer += 40
            asteroid[27:31] = (count + 1).to_bytes(4, "big"); asteroid[18:26] = (1).to_bytes(8, "big")
            asteroid[footer + 2:footer + 6] = (70001).to_bytes(4, "big")
            exits = 70001 - count - 1
            asteroid[footer + 23:footer + 31] = exits.to_bytes(8, "big")
            asteroid[footer + 31:footer + 39] = (5 * exits - (70001 - count)).to_bytes(8, "big")
            asteroid[footer + 39:footer + 42] = bytes([1, 2, 0]); asteroid[footer + 42:footer + 46] = (741).to_bytes(4, "big")
        name = b"ASTR"; replacement = riff(name, asteroid)
    elif case == "field":
        asteroid = next(owner["payload"] for owner in owners if owner["name"] == b"ASTR")
        count = int.from_bytes(asteroid[27:31], "big")
        assert all(int.from_bytes(asteroid[index:index + 4], "big") != 0 for index in range(51, 51 + 40 * count, 40))
        # Natural packet0 history remains valid, but live knowledge cannot
        # borrow body0 from the different open factory world where it survives.
        name = b"SPKN"; replacement = riff(name, b"\0\1" + bytes(4) + (1).to_bytes(8, "big"))
    elif case == "company":
        owner = next(owner for owner in owners if owner["name"] == b"PLYR")
        assert owner["kind"] in (3, 4)
        # Remove candidate companies only. Existing native owners still require
        # their own candidate company; the open world's company0 grants none.
        name = b"PLYR"; header = owner["rows"][0]
        replacement = name + bytes([owner["kind"]]) + gamma(len(header) + 1) + header + b"\0"
    elif case == "zero-hull-schema":
        owner = next(owner for owner in owners if owner["name"] == b"VEHS")
        fields = table_header(Reader(owner["rows"][0]))
        space = next(nested for key, _, nested in fields if key == "space")
        field = next(field for field in space if field[0] == "mission.continuation.mineral")
        field[0] = "x" + field[0][1:]
        rows = [header_bytes(fields)] + owner["rows"][1:]
        name = b"VEHS"
        replacement = name + bytes([owner["kind"]]) + b"".join(gamma(len(row) + 1) + row for row in rows) + b"\0"
    else:
        raise ValueError(case)
    target.write_bytes(b"OTTN" + (version << 16).to_bytes(4, "big") +
                       b"".join(replacement if owner["name"] == name else owner["raw"] for owner in owners) + bytes(4))


def main():
    """Run one joined preview/full refusal pair under the original bounded native harness.

    @return First native failure, or zero after both original loading paths refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", required=True)
    parser.add_argument("--open-world", required=True)
    parser.add_argument("--case", choices=("source", "field", "mission", "company", "zero-hull-schema", "known-cost", "bootstrap", "weight-inflation"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--run", required=True)
    args = parser.parse_args()
    candidate = (ROOT / args.candidate).resolve(); open_world = (ROOT / args.open_world).resolve()
    directory = ROOT / "test-cache/space-fixes/transport-gate-cases"
    directory.mkdir(parents=True, exist_ok=True)
    target = (directory / (args.run + ".sav")).resolve()
    assert target.parent == directory.resolve() and candidate.is_relative_to(ROOT) and open_world.is_relative_to(ROOT)
    make_bad(candidate, target, args.case)
    # Preview checks valid-corrupt-valid and restores a semantically different
    # original preview. Full refusal tears down a separate disposable process.
    for mode in ("preview", "full"):
        command = [sys.executable, str(Path(__file__).with_name("space_fixes_checks.py")),
                   "--configuration", args.configuration, "--source", (open_world if mode == "preview" else candidate).relative_to(ROOT).as_posix(),
                   "--cargo-origin", "--cargo-bad", target.relative_to(ROOT).as_posix(), "--run", args.run + "-" + mode]
        if mode == "preview":
            command.extend(["--preview-source", candidate.relative_to(ROOT).as_posix()])
        else:
            command.append("--cargo-full")
        result = subprocess.run(command, cwd=ROOT, check=False)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
