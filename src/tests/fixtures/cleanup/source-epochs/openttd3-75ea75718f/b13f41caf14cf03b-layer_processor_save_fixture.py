#!/usr/bin/env python3
"""Actual native processor save/load, physical stock digests and unchanged packet provenance.

@file layer_processor_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 16B
"""

import argparse
import hashlib
import lzma
import subprocess

from geology_save_fixture import chunks, encode_chunk
from geology_generation_fixture import read_stream
from multilayer_state_fixture import AREA, ROOT, log_path, run_case, save_path


# Each producer declares its feature; saved-input names never select output ownership.
from functools import partial
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import output_save_path
CONTEXT = current_run_context("processors", "layer-processor-save-fixture")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime("grftest.cfg")
output_save_path = partial(output_save_path, context=CONTEXT)
save_path = partial(save_path, context=CONTEXT)
run_case = partial(run_case, context=CONTEXT)
log_path = partial(log_path, context=CONTEXT)


def processor_chunks(path):
    """Read owner-specific scalar/counter transport and the existing packet descriptor bytes.

    @param path Actual writer output; no source is modified.
    @return Exact native side-state and existing packet chunks.
    """
    version, stream = read_stream(path)
    assert version >= 380, "processor descriptors require their delivered save version"
    names = {b"LPCH", b"LPCR", b"LPCC", b"CAPA"}
    return {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}


def main():
    """Reload missing-input/full-output/pending-transport checkpoints and refuse malformed choices/counters.

    @return None; every supplied source hash and startup script is preserved across success or failure.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--refusals-only", action="store_true", help="run only malformed processor files")
    parser.add_argument("--case", choices=("missing-input", "full-output", "in-transit"), help="focus one changed positive checkpoint, then malformed files")
    args = parser.parse_args()
    startup = RUNTIME / "scripts/game_start.scr"
    original = startup.read_bytes()
    sources = [save_path(f"r2mlvs6-ds-processor-{name}.sav") for name in ("missing-input", "full-output", "in-transit")]
    hashes = {source: hashlib.sha256(source.read_bytes()).hexdigest() for source in sources}
    try:
        # The stock digest uses actual Industry input/output slots and stable labels, independently of runtime cargo-slot mapping.
        for source in sources:
            if args.refusals_only:
                continue
            if args.case and source.stem != f"r2mlvs6-ds-processor-{args.case}":
                continue  # Reuse unchanged positive checkpoint evidence after a fixture-only correction.
            checkpoint = source.with_suffix(".sav.txt").read_text(encoding="utf-8").splitlines()[1]
            before = processor_chunks(source)
            result = run_case(source.stem, ["-g", source.as_posix()], checkpoint, prefix="r2mlvs6-dz")
            assert processor_chunks(result) == before, "recipe/batch counters or actual packet provenance changed"
            print(f"{source.name}: real reload retained exact stock/choice/history digest and packet bytes", flush=True)
        source = sources[0]
        # Mutate only this family's transport; ordinary physical stock and cargo packets remain immutable evidence.
        for name in ("missing-header", "foreign-choice", "forged-product"):
            original_bytes = source.read_bytes()
            assert original_bytes[:4] == b"OTTX", "fixture mutation needs actual LZMA writer output"
            _, stream = read_stream(source)
            records = chunks(stream)
            if name == "missing-header":
                next(row for row in records if row["name"] == b"LPCH")["remove"] = True
                error = "Missing processor header"
            elif name == "foreign-choice":
                table = next(row for row in records if row["name"] == b"LPCR")
                row = bytearray(table["rows"][1])
                assert len(row) == 33, "unexpected scalar descriptor order"
                row[3:7] = b"XXXX"
                table["rows"][1] = bytes(row)
                table["edited"] = True
                error = "Invalid native processor state"
            else:
                table = next(row for row in records if row["name"] == b"LPCC")
                row = bytearray(table["rows"][-1])
                assert len(row) == 15 and row[2] == 2, "unexpected product counter descriptor order"
                row[-8:] = (3).to_bytes(8, "big")
                table["rows"][-1] = bytes(row)
                table["edited"] = True
                error = "Invalid native processor state"
            encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"]
                               for row in records if not row.get("remove")) + bytes(4)
            candidate = output_save_path(f"r2mlvs6-ea-{name}-input.sav")
            candidate.write_bytes(original_bytes[:8] + lzma.compress(encoded))
            try:
                run_case(name, ["-g", candidate.as_posix()], error, accepted=False, prefix="r2mlvs6-ea")
            except subprocess.CalledProcessError as refusal:
                # A dedicated refused startup may exit 2; require the exact owner error and no resulting save.
                assert refusal.returncode == 2
                evidence = log_path(f"r2mlvs6-ea-{name}.log").read_text(encoding="utf-8", errors="replace")
                assert error in evidence
                assert not output_save_path(f"r2mlvs6-ea-{name}.sav").exists()
    finally:
        startup.write_bytes(original)
        # Source writer output is supplied evidence, never the mutation destination.
        for source, digest in hashes.items():
            assert hashlib.sha256(source.read_bytes()).hexdigest() == digest, "source processor save changed"


if __name__ == "__main__":
    main()
