#!/usr/bin/env python3
"""Actual game-loader round trips of command-funded native mine saves.

@file layer_mine_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 16
"""

import hashlib
import argparse
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
CONTEXT = current_run_context("mines", "layer-mine-save-fixture")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime("grftest.cfg")
output_save_path = partial(output_save_path, context=CONTEXT)
save_path = partial(save_path, context=CONTEXT)
run_case = partial(run_case, context=CONTEXT)
log_path = partial(log_path, context=CONTEXT)


def owned_chunks(path):
    """Read immutable native mine/geology transport from an actual save.

    @param path Actual game-writer save, never an arbitrary input to rewrite.
    @return Version and exact bytes of each authority's relevant descriptor family.
    """
    version, stream = read_stream(path)
    names = {b"LMNH", b"LMNR", b"GDEP", b"GSPN"}
    return version, {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}


def main():
    """Reload construction, partial and exhausted mines through the real activated game.

    @return None; any loader failure or changed binding/reserve raises after restoring startup bytes.
    """
    startup = RUNTIME / "scripts/game_start.scr"
    parser = argparse.ArgumentParser()
    parser.add_argument("--refusals-only", action="store_true", help="run only the malformed mine files")
    args = parser.parse_args()
    original = startup.read_bytes()
    sources = [save_path(f"r2mlvs6-cr-mine-{name}.sav") for name in ("construction", "partial", "depleted")]
    hashes = {source: hashlib.sha256(source.read_bytes()).hexdigest() for source in sources}
    try:
        # Each source was written by the actual save owner after command funding, not assembled from invented mine rows.
        for source in sources:
            if args.refusals_only:
                continue  # A focused malformed-file burst reuses unchanged passing round-trip evidence.
            before_version, before = owned_chunks(source)
            assert before_version >= 379 and len(before) == 4, "required actual descriptor family missing"
            result = run_case(source.stem, ["-g", source.as_posix()], "MLVS_METADATA schema=1 profile=1",
                              prefix="r2mlvs6-cz")
            after_version, after = owned_chunks(result)
            assert after_version == before_version, "game save version changed unexpectedly"
            assert after == before, "mine binding or geological reserve changed during actual reload/save"
            print(f"{source.name}: real game round trip preserved all mine/geology rows", flush=True)
        source = sources[1]
        # Mutations target only owned descriptor transport; physical industry/geology chunks retain their original bytes.
        for name, error in (("missing-header", "Missing mine header"), ("wrong-count", "Incomplete mine family"),
                            ("wrong-company", "Invalid finite mine state")):
            original_bytes = source.read_bytes()
            assert original_bytes[:4] == b"OTTX", "fixture mutation requires the writer's actual LZMA stream"
            _, stream = read_stream(source)
            records = chunks(stream)
            header = next(row for row in records if row["name"] == b"LMNH")
            if name == "missing-header":
                header["remove"] = True
            elif name == "wrong-count":
                header["payload"] = (1).to_bytes(4, "big") + (2).to_bytes(4, "big")
                header["edited"] = True
            else:
                table = next(row for row in records if row["name"] == b"LMNR")
                row = bytearray(table["rows"][1])
                assert len(row) == 46, "unexpected mine scalar descriptor order"
                row[6] = 255  # Only the saved claimant changes; the actual geology still names the real company.
                table["rows"][1] = bytes(row)
                table["edited"] = True
            encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"]
                               for row in records if not row.get("remove")) + bytes(4)
            candidate = output_save_path(f"r2mlvs6-dl-{name}-input.sav")
            candidate.write_bytes(original_bytes[:8] + lzma.compress(encoded))
            try:
                run_case(name, ["-g", candidate.as_posix()], error, accepted=False, prefix="r2mlvs6-dl")
            except subprocess.CalledProcessError as refusal:
                # Dedicated startup exits 2 on a refused save; require the precise owned refusal and no resulting save.
                assert refusal.returncode == 2, "unexpected process failure"
                evidence = log_path(f"r2mlvs6-dl-{name}.log").read_text(encoding="utf-8", errors="replace")
                assert error in evidence, "expected mine refusal missing"
                assert not output_save_path(f"r2mlvs6-dl-{name}.sav").exists(), "refused world was saved"
                print(f"{name}: real game rejected corrupt mine transport", flush=True)
    finally:
        startup.write_bytes(original)
        # Supplied evidence remains immutable across both successful and refused reloads.
        for source, digest in hashes.items():
            assert hashlib.sha256(source.read_bytes()).hexdigest() == digest, "source save changed"


if __name__ == "__main__":
    main()
