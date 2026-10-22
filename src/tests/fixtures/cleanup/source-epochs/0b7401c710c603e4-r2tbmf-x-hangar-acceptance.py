"""Run the combined hangar/order acceptance batch against one completed build.

@see r2tbmf-tbm-fixes.md Group8
@see ../../architecture/savegame/archi-savegame.md
"""

import argparse
import hashlib
import lzma
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import Reader, read_stream
from geology_save_fixture import chunks, encode_chunk
from tbm_logistics_save_fixture import WIDTHS, saved_table, table_header, table_record

FIXTURES = ROOT / "src/tests/fixtures/haulage"
BUILD = ROOT / "build"


def digest(path):
    """@param path Retained input. @return SHA256 of the exact bytes."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def malformed(source, chunk_name, identity, field, value, serial):
    """Copy a real saved table and change precisely one named scalar.

    @param source Immutable native 397/398 input.
    @param chunk_name Owning chunk, without a parallel schema.
    @param identity Actual dense or sparse row identity.
    @param field Existing scalar name.
    @param value Deliberately invalid replacement.
    @param serial Unique evidence run; earlier malformed fixtures are retained.
    @return New repository-owned malformed file.
    """
    original = source.read_bytes()
    assert original[:4] == b"OTTX"
    version, stream = read_stream(source)
    records = chunks(stream)
    record = next(item for item in records if item["name"] == chunk_name)
    fields = table_header(Reader(record["rows"][0]))
    changed = False
    for row_index, raw in enumerate(record["rows"][1:], 1):
        reader = Reader(raw)
        row_id = reader.gamma() if record["kind"] == 4 else row_index - 1
        if row_id != identity:
            continue
        for name, kind, nested in fields:
            start = reader.pos
            table_record(reader, [(name, kind, nested)])
            if name != field:
                continue
            assert nested is None and not kind & 16 and kind & 15 in WIDTHS
            row = bytearray(raw)
            row[start:reader.pos] = value.to_bytes(WIDTHS[kind & 15], "big")
            record["rows"][row_index] = bytes(row)
            changed = True
            break
        break
    assert changed
    encoded = b"".join(encode_chunk(item) if item is record else item["raw"] for item in records) + bytes(4)
    target = FIXTURES / f"tbm-hangar-bad-{version}-{chunk_name.decode()}-{field}-{serial}.sav"
    assert not target.exists()
    target.write_bytes(original[:8] + lzma.compress(encoded))
    expected = saved_table(source, chunk_name)
    expected[identity][field] = value
    assert saved_table(target, chunk_name) == expected
    return target


def run_native(tag, environment, log):
    """Run one bounded native process, retaining failures for the batch summary.

    @param tag Catch selection against the same already-built executable.
    @param environment Explicit probe options.
    @param log Fresh evidence destination.
    @return Native exit status and decoded log.
    """
    env = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_TBM_")}
    env.update(environment)
    with log.open("w", encoding="utf-8") as output:
        result = subprocess.run([str(BUILD / "RelWithDebInfo/openttd_test.exe"), tag], cwd=BUILD, env=env,
                                stdout=output, stderr=subprocess.STDOUT, timeout=120)
    return result.returncode, log.read_text(encoding="utf-8", errors="replace")


def main():
    """@return None; collect related behavior and malformed-file results without rebuilding between scenarios."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--case", choices=("all", "behavior", "malformed"), default="all")
    args = parser.parse_args()
    serial = 1
    while list((ROOT / "logs").glob(f"r2tbmf-x-*-{serial}.log")):
        serial += 1
    failures = []
    if args.case in ("all", "behavior"):
        log = ROOT / f"logs/r2tbmf-x-behavior-{serial}.log"
        status, evidence = run_native(
            "[tbm-hangar-acceptance],[tbm-selected-hangar],[tbm-hangar-service],[tbm-native-orders],[tbm-hangar-orders]",
            {"OTTD_TBM_LOGISTICS_RECORD": f"tbm-hangar-398-batch-{serial}"}, log)
        print(f"Combined behavior: exit={status}, log={log.name}", flush=True)
        if status:
            failures.append(log.name)
    if args.case in ("all", "malformed"):
        old = FIXTURES / "tbm-hangar-397-2-hangar-parked.sav"
        current = FIXTURES / "tbm-hangar-reload-orders-run-2-orders-travel.sav"
        protected = [old, current, BUILD / "native-art.cfg", BUILD / "play.cfg", FIXTURES / "underground-demo-390.sav"]
        before = {path: digest(path) for path in protected}
        script = BUILD / "scripts/game_start.scr"
        original_script = script.read_bytes()
        old_projects = saved_table(old, b"TBMP")
        old_id = next(iter(old_projects))
        current_id = next(iter(saved_table(current, b"TBMP")))
        depots = saved_table(old, b"DEPT")
        source_id = next(identity for identity, row in depots.items() if row["xy"] == old_projects[old_id]["portal"])
        other_portal = depots[max(depots)]["xy"]
        assert other_portal != depots[source_id]["xy"]
        cases = [
            (old, b"TBMP", old_id, "phase", 19, "Invalid TBM project fields", ("preview", "load")),
            (current, b"TBMP", current_id, "phase", 255, "Invalid TBM project fields", ("preview", "load")),
            (current, b"TBMP", current_id, "blocked", 255, "Invalid TBM project fields", ("preview", "load")),
            (old, b"DEPT", source_id, "xy", other_portal, "Missing portal hangar depot", ("load",)),
        ]
        try:
            script.write_text("tbm_motion\n", encoding="utf-8")
            for source, chunk, identity, field, value, expected, modes in cases:
                bad = malformed(source, chunk, identity, field, value, serial)
                for mode in modes:
                    log = ROOT / f"logs/r2tbmf-x-{bad.stem}-{mode}-{serial}.log"
                    env = {"OTTD_TBM_PACE_CHECK": "underground", "OTTD_TBM_PACE_RESAVED": "1",
                           "OTTD_TBM_BAD_EXPECTED": expected,
                           "OTTD_TBM_PACE_BAD_PREVIEW" if mode == "preview" else "OTTD_TBM_PACE_BAD_LOAD": str(bad)}
                    status, evidence = run_native("[.tbm-motion-reload]", env, log)
                    passed = status == 0 and expected in evidence and "PACE_REFUSED " in evidence
                    passed &= ("unchanged=true" if mode == "preview" else "startup_dispatched=false") in evidence
                    print(f"{bad.name} {mode}: {'PASS' if passed else 'FAIL'}", flush=True)
                    if not passed:
                        failures.append(log.name)
        finally:
            script.write_bytes(original_script)
            for path, expected in before.items():
                assert digest(path) == expected, path
    if failures:
        raise SystemExit("Failed acceptance logs: " + ", ".join(failures))


if __name__ == "__main__":
    main()
