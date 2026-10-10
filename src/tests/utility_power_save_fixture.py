"""Bounded native utility file round trips, exact chunk comparison and corrupt preview refusal.

@file utility_power_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 7B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
"""

import hashlib
import lzma
import os
from pathlib import Path
import subprocess

from geology_save_fixture import chunks, encode_chunk
from geology_generation_fixture import read_stream
from multilayer_state_fixture import log_path

ROOT = Path(__file__).resolve().parents[2]
DIRECTORY = ROOT / "src/tests/fixtures/utility"


from functools import partial
from pathlib import Path
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import log_path
CONTEXT = current_run_context("utilities", "utility-power-save-fixture")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()
INPUTS = ROOT / "src/tests/fixtures/utility"
DIRECTORY = CONTEXT.saves
log_path = partial(log_path, context=CONTEXT)


def utility_chunks(path):
    """Read actual versioned utility transport without interpreting private runtime state.

    @param path Native save produced by this feature's ordinary writer.
    @return Exact utility header/table bytes; malformed or older transport refuses.
    """
    version, stream = read_stream(path)
    assert version in (384, 385), "utility effect/lifecycle transport needs an explicit supported save version"
    selected = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in (b"UPLH", b"UPLD")}
    assert set(selected) == {b"UPLH", b"UPLD"}
    return selected


def run_case(mode, source, suffix, ticket=None, prefix="r2mlvs3mp-u", context=CONTEXT):
    """Load one actual world in an isolated native process and retain bounded owner evidence.

    @param mode Native observer operation for this publication/settlement/save boundary.
    @param source Actual retained/generated input, never rewritten by the observer.
    @param suffix Unique owning-plan log suffix, including repeated load pass identity.
    @param ticket Original issuer sequence/seal for actual saved retry proof; absent otherwise.
    @param prefix Owning focused group's log prefix; only the basename reaches the shared log router.
    @param context Explicit caller context for output/runtime ownership.
    @return Exact retained native evidence; failures preserve logs and raise.
    """
    log = log_path(f"{prefix}-power-file-{suffix}.log", context=context)
    environment = dict(os.environ, OTTD_UTILITY_FILE_CASE=mode, OTTD_UTILITY_FILE_INPUT=str(source))
    if ticket is not None:
        environment.update(OTTD_UTILITY_TICKET_SEQUENCE=ticket[0], OTTD_UTILITY_TICKET_SEAL=ticket[1])
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build/Debug/openttd_test.exe"), "[.utility-power-file]"],
                       cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence, suffix
    assert f"UTILITY_FILE case={mode}" in evidence, suffix
    print(f"{suffix}: actual utility file boundary passed", flush=True)
    return evidence


def corrupt_variants(source):
    """Alter only this feature's concrete bytes for five bounded parser/owner failures.

    @param source Actual native allocated-grant output; its original bytes remain unchanged.
    @return None; test-owned corrupt candidates live beside utility save fixtures.
    """
    original = source.read_bytes()
    version, stream = read_stream(source)
    # Current lifecycle and retained effect saves share the fixed row geometry;
    # corruption must still start from a genuine supported native writer.
    assert version in (384, 385) and original[:4] == b"OTTX"
    # Every variant starts from the real save independently. Unrelated world,
    # cargo, companies and work-controller chunks remain byte-exact.
    for fault in ("missing-table", "bad-count", "bad-carry", "bad-reserve", "bad-owner"):
        records = chunks(stream)
        header = next(row for row in records if row["name"] == b"UPLH")
        table = next(row for row in records if row["name"] == b"UPLD")
        if fault == "missing-table":
            records.remove(table)
        elif fault == "bad-count":
            payload = bytearray(header["payload"])
            assert len(payload) == 96
            payload[40:48] = (5).to_bytes(8, "big")
            header.update(payload=bytes(payload), edited=True)
        else:
            # The fixed scalar source row has explicit offsets in the owning
            # descriptor; verify the record size/type before narrow corruption.
            index = 2 if fault == "bad-reserve" else 1
            row = bytearray(table["rows"][index])
            assert len(row) == 104 and row[0] == 0
            if fault == "bad-carry":
                row[57:59] = (1000).to_bytes(2, "big")
            elif fault == "bad-reserve":
                row[37:45] = (55).to_bytes(8, "big")
            else:
                row[9] = 255
            table["rows"][index] = bytes(row)
            table["edited"] = True
        encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in records) + bytes(4)
        (DIRECTORY / f"utility-corrupt-{fault}.sav").write_bytes(original[:8] + lzma.compress(encoded))


def main():
    """Check three saved phases twice plus five corrupt previews, retaining supplied startup/input bytes.

    @return None; assertions require exact utility bytes and successful real owner acceptance.
    """
    source = ROOT / "src/tests/fixtures/haulage/spoil-loop-underground/m1-loop-underground-5-complete-loop.sav"
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    startup = RUNTIME / "scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    try:
        startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md7B native utility file proof.\nutility_file_check\n", encoding="utf-8")
        run_case("create", source, "create")
        # Stable phase inputs retain exact publication fractions, original grant
        # slices and retired-source release through two independent native loads.
        for phase in ("published", "granted", "lost"):
            original = DIRECTORY / f"utility-{phase}.sav"
            expected = utility_chunks(original)
            run_case(phase, original, f"{phase}-1")
            restored = DIRECTORY / f"utility-{phase}-reloaded.sav"
            assert utility_chunks(restored) == expected, (phase, "first reload changed authoritative utility bytes")
            run_case(phase, restored, f"{phase}-2")
            assert utility_chunks(restored) == expected, (phase, "second reload changed authoritative utility bytes")
        corrupt_variants(DIRECTORY / "utility-granted.sav")
        run_case("preview", DIRECTORY / "utility-granted.sav", "corrupt-preview")
        run_case("resume", DIRECTORY / "utility-lost.sav", "resume")
        resumed = utility_chunks(DIRECTORY / "utility-resumed.sav")
        run_case("resumed", DIRECTORY / "utility-resumed.sav", "resumed-load")
        assert utility_chunks(DIRECTORY / "utility-resumed-reloaded.sav") == resumed
        print("Utility file proof: three phases/two loads, exact header/table bytes, five corrupt refusals, valid preview retry and saved carry/reservation resume", flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "retained paid-world input changed"


if __name__ == "__main__":
    main()
