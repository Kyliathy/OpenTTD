"""Verify exact TBM rate migration and malformed fractions through native preview/full loading.

@file pacing_files.py
@see r2tbmf-tbm-fixes.md Group5
@see ../../architecture/savegame/archi-savegame.md
"""

import hashlib
import lzma
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
# Repository markers identify the source checkout after tools move; neither
# documentation depth nor the caller's working directory owns runtime output.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native, start_native

CONTEXT = current_run_context("tbm", "pacing")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import Reader, read_stream
from geology_save_fixture import chunks, encode_chunk
from tbm_logistics_save_fixture import WIDTHS, owned_chunks, saved_table, table_header

FIXTURES = ROOT / "src/tests/fixtures/haulage"


def digest(path):
    """Identify original bytes for preservation checks.

    @param path Existing repository-owned input file.
    @return SHA256 digest.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def corrupt_fraction(source, value):
    """Change only one copied project's fraction in a separately named malformed save.

    @param source Retained real 393 or 394 partial-cut file; never modified.
    @param value Out-of-range numerator to exercise the original/new divisor check.
    @return Repository-owned malformed fixture path.
    """
    original = source.read_bytes()
    assert original[:4] == b"OTTX"
    version, stream = read_stream(source)
    records = chunks(stream)
    record = next(item for item in records if item["name"] == b"TBMP")
    assert record["kind"] == 3 and len(record["rows"]) == 2
    fields = table_header(Reader(record["rows"][0]))
    row = bytearray(record["rows"][1])
    offset = 0
    changed = False
    # Use the real named scalar schema, not a guessed byte offset. All other
    # owners/fields and the file's original version stay byte-for-byte intact.
    for name, kind, nested in fields:
        assert nested is None and not kind & 16
        width = WIDTHS[kind & 15]
        if name == "work_remainder":
            assert width == 2
            row[offset:offset + width] = value.to_bytes(width, "big")
            changed = True
        offset += width
    assert changed and offset == len(row)
    record["rows"][1] = bytes(row)
    encoded = b"".join(encode_chunk(item) if item is record else item["raw"] for item in records) + bytes(4)
    target = CONTEXT.saves / f"{source.stem}-bad-fraction-{value}.sav"
    target.write_bytes(original[:8] + lzma.compress(encoded))
    assert read_stream(target)[0] == version
    expected = saved_table(source, b"TBMP")
    expected[0]["work_remainder"] = value
    assert saved_table(target, b"TBMP") == expected
    return target


def verify_migration(layer):
    """Compare every saved custody owner and exact project fields across 393->394.

    @param layer Underground or Deep native seed family.
    @return None; only the numerically equivalent fractional representation may change.
    """
    old = resolve_input_path(FIXTURES / f"tbm-pace-{layer}-393.sav")
    new = resolve_input_path(FIXTURES / f"tbm-pace-{layer}-394.sav")
    old_version, before = owned_chunks(old)
    new_version, after = owned_chunks(new)
    assert (old_version, new_version) == (393, 394)
    assert before.keys() == after.keys()
    for name in before:
        if name != b"TBMP":
            assert before[name] == after[name], (layer, name)
    expected = saved_table(old, b"TBMP")
    for row in expected.values():
        row["work_remainder"] *= 2
    assert saved_table(new, b"TBMP") == expected
    print(f"{layer}: exact fraction-only migration and preserved native owners", flush=True)


def main():
    """Run eight malformed files through actual preview and startup loading, preserving supplied inputs and startup.

    @return None; assertion/native failures retain focused logs and restore all protected bytes.
    """
    build = ROOT / "build"
    script = RUNTIME / "scripts/game_start.scr"
    original = script.read_bytes() if script.exists() else None
    protected = [resolve_input_path(FIXTURES / f"tbm-pace-{layer}-{version}.sav") for layer in ("underground", "deep") for version in (393, 394)]
    protected += [build / "native-art.cfg", build / "play.cfg", resolve_input_path(FIXTURES / "underground-demo-390.sav"), resolve_input_path(FIXTURES / "underground-demo-tbm-ui-393.sav")]
    before = {path: digest(path) for path in protected}
    try:
        script.write_text("# @see r2tbmf-tbm-fixes.md Group5 native malformed-fraction checks.\ntbm_motion\n", encoding="utf-8")
        for layer in ("underground", "deep"):
            verify_migration(layer)
            for version in (393, 394):
                divisor = (1 if layer == "underground" else 2) * (1 if version == 393 else 2)
                for value in (divisor, 65535):
                    bad = corrupt_fraction(resolve_input_path(FIXTURES / f"tbm-pace-{layer}-{version}.sav"), value)
                    for mode in ("preview", "load"):
                        env = dict(os.environ)
                        # Isolate probe modes so a queued environment option
                        # cannot turn a refusal test into a seed-writing run.
                        for name in list(env):
                            if name.startswith(("OTTD_TBM_PACE_", "OTTD_TBM_MOTION_")):
                                env.pop(name)
                        env["OTTD_TBM_PACE_CHECK"] = layer
                        env["OTTD_TBM_PACE_RESAVED"] = "1"
                        env["OTTD_TBM_PACE_BAD_PREVIEW" if mode == "preview" else "OTTD_TBM_PACE_BAD_LOAD"] = str(bad)
                        log = CONTEXT.logs / f"r2tbmf-k-{layer}-{version}-{value}-{mode}.log"
                        with log.open("wb") as output:
                            result = run_native(CONTEXT, [str(build / "RelWithDebInfo/openttd_test.exe"), "[.tbm-motion-reload]"], cwd=build,
                                                    env=env, stdout=output, stderr=subprocess.STDOUT, timeout=110)
                        result.check_returncode()
                        evidence = log.read_text(encoding="utf-8", errors="replace")
                        assert "Invalid TBM work fraction" in evidence, log
                        assert "PACE_REFUSED " in evidence, log
                        assert ("unchanged=true" if mode == "preview" else "startup_dispatched=false") in evidence, log
                        print(f"{layer} v{version} numerator={value}: native {mode} refused correctly", flush=True)
    finally:
        # Restore exact startup bytes even on failure; all legitimate seeds,
        # demo saves and configurations remain immutable throughout the suite.
        if original is None:
            script.unlink(missing_ok=True)
        else:
            script.write_bytes(original)
        for path in protected:
            assert digest(path) == before[path], path


if __name__ == "__main__":
    main()
