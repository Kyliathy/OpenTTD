"""Refuse independently malformed switched-work fields through a valid native compression envelope.

@file geothermal_tube_work_refusals.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BDDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Each invocation owns one field fault and independent preview/full native processes.
Original packets, invoices, demand fractions and saved inputs retain their existing owners.
"""
import argparse
import json
import lzma
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from geothermal_tube_work_files import ROOT, SOURCES, OWNER_CHUNKS, compare_chunks, read_work
from geothermal_tube_binding_files import world_chunks
from local_electrical_feed_checks import identity
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk
from runner import current_run_context

FAULTS = ("missing-column", "raw-boolean", "wrong-source", "wrong-feed", "external-role", "fraction", "future-work")


def corrupt_work(context, source, fault):
    """Mutate one actual active switched binding while preserving original file type/version and every other chunk.

    @param context Reserved invocation-owned saves and diagnostics.
    @param source Unchanged genuine434 work file with original used/retired history.
    @param fault One missing field, raw scalar, role/reference, fraction or work-history failure.
    @return Separately written structurally parseable current copy; no mutation repairs its semantic fault.
    """
    version, stream = read_stream(source)
    original_header = source.read_bytes()[:8]
    assert version == 434 and original_header[:4] == b"OTTX"
    records = chunks(stream)
    table = next(row for row in records if row["name"] == b"TUVD")
    assert len(table["rows"]) >= 2 and all(len(row) == 41 for row in table["rows"][1:])
    active = [index for index, row in enumerate(table["rows"][1:], 1) if row[32] == 1]
    assert len(active) == 1
    if fault == "missing-column":
        assert table["rows"][0].count(b"geothermal_source") == 1
        table["rows"][0] = table["rows"][0].replace(b"geothermal_source", b"xeothermal_source")
    else:
        row = bytearray(table["rows"][active[0]])
        assert row[33:37] != bytes([255]) * 4
        if fault == "raw-boolean":
            row[32] = 2
        elif fault == "wrong-source":
            row[33:37] = (999).to_bytes(4, "big")
        elif fault == "wrong-feed":
            assert row[37:41] != bytes(4)
            row[37:41] = bytes(4)
        elif fault == "external-role":
            row[33:41] = bytes([255]) * 8
        elif fault == "fraction":
            row[12:14] = (1000).to_bytes(2, "big")
        elif fault == "future-work":
            prepared = int.from_bytes(row[14:22], "big")
            assert prepared < (1 << 64) - 2
            row[22:30] = (prepared + 1).to_bytes(8, "big")
        else:
            raise ValueError(fault)
        table["rows"][active[0]] = bytes(row)
    table["edited"] = True
    # Keep the original native LZMA tag and version. A zlib-tagged LZMA copy
    # would only test decompression failure and never reach the owned fields.
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in records) + bytes(4)
    saved = context.output("saves", f"bad-switched-work-{fault}.sav")
    saved.write_bytes(original_header + lzma.compress(encoded))
    assert read_stream(saved) == (version, encoded)
    before, after = world_chunks(source), world_chunks(saved)
    assert before.keys() == after.keys()
    assert [name for name in before if before[name] != after[name]] == [b"TUVD"]
    return saved


def main():
    """Verify one semantic current-file refusal and complete valid-world recovery in independent native processes.

    @return None; wrong refusal cause, changed original, missing packet/history or empty native selection aborts acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--writer-report", type=Path, required=True)
    parser.add_argument("--fault", choices=FAULTS, required=True)
    args = parser.parse_args()
    writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
    checkpoint = next(point for point in writer["checkpoints"] if point["phase"] == 2 and point["step"] == 0)
    source = ROOT / checkpoint["input"]["path"]; original = identity(source); assert original == checkpoint["input"]
    context = current_run_context("tubes", "geothermal-work-files"); context.prepare_runtime()
    corrupt = corrupt_work(context, source, args.fault)
    if args.fault == "missing-column":
        preview_error = full_error = "Missing capsule power field"
    elif args.fault == "raw-boolean":
        preview_error = full_error = "Invalid capsule power order"
    elif args.fault in ("fraction", "future-work"):
        preview_error = full_error = "Invalid capsule power structure"
    else:
        preview_error, full_error = "Invalid capsule power structure", "Invalid metered capsule authority"
    proofs = []
    # Preview and failed full-load/retry are separate native processes; each
    # must reacquire the exact original capsule and independently check every
    # original owner, packet, carry, invoice, cash, clock and manual-stop state.
    for action, reason in (("preview-refusal", preview_error), ("full-refusal", full_error)):
        saved, proof = read_work(context, args.config, checkpoint, source, action, action, corrupt, reason)
        proof["recovered_owner_chunks_exact"] = compare_chunks(source, saved, OWNER_CHUNKS | {b"VEHS"})
        proofs.append(proof)
    assert identity(source) == original
    report = dict(_documentation="r2mlvs17-underground-transport-tubes.md6BDDC.2; valid native LZMA envelope, actual active switched fields and exact semantic refusal/recovery",
                  configuration=args.config, fault=args.fault, writer=identity(args.writer_report), original=original, corrupt=identity(corrupt),
                  binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), controller=identity(__file__),
                  sources=[identity(ROOT / name) for name in (*SOURCES, "src/tests/multilayer/utilities/geothermal_tube_work_files.py")], native=proofs)
    destination = context.output("reports", f"geothermal-work-refusal-{args.config}-{args.fault}.json")
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(destination, flush=True)


if __name__ == "__main__":
    main()
