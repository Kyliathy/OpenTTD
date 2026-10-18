"""Reconcile paid processor persistence and declared full-buffer/removal refusal boundaries.

@file orbital_save_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group14.3
Paid native worlds supply actual partial inputs/work and completed products.
Explicit full-buffer codec fixtures and existing current component refusals remain
separate from paid procurement; no fixture becomes an end-to-end predecessor.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from artifact_paths import create_run_context

HERE = Path(__file__).parent
FACTORY = runpy.run_path(str(HERE / "orbital_save_checks-5.py"))
AUDIT = runpy.run_path(str(HERE / "orbital_processor_checks-3.py"))
read, accepted_report = FACTORY["read"], FACTORY["accepted_report"]
CASES = ("steel-partial", "steel-batch", "goods-input", "goods-partial", "goods-batch", "refinery-partial",
         "refinery-metal", "ice-partial", "ice-batch", "steel-outage", "full-steel", "full-goods")


def full_buffer(source, case):
    """Create a declared current-format full-output fixture without rewriting its original paid history.

    @param source Immutable actual paid completed-batch identity.
    @param case Explicit full-steel or full-goods codec case.
    @return Invocation-owned fixture identity, retaining exact original history and packet identity.
    """
    context = create_run_context("space", "orbital-save-full-output")
    context.prepare_runtime()
    version, raw = read_stream(ROOT / source["path"])
    assert version == 438
    kind, ordinal, label = (10, 9, "Steelworks") if case == "full-steel" else (11, 5, "Goodsworks")
    # Published recipe capacities own the exact full threshold; this declared
    # fixture adds output inventory only. It grants no batches, paid work,
    # consumed history, new packet identity or actual production claim.
    definitions = (ROOT / "src/multilayer_recipe.cpp").read_text()
    line, = [line for line in definitions.splitlines() if line.strip().startswith('{"Orbital ' + label + '",')]
    assert "1024, 1024," in line
    records = AUDIT["LAYOUT"]["structure_rows"](raw, 436)["rows"]
    row, = [row for row in records.values() if row["kind"] == kind]
    stock = row["stock"][ordinal]
    assert stock["count"] == 1 and stock["quantity"] in (2, 4)
    reference, = stock["packet_offsets"]
    packet = int.from_bytes(raw[reference:reference + 4], "big")
    packet_row, = [row for row in FACTORY["TABLES"]["table_rows"](raw, b"CAPA") if row["index"] == packet]
    count, = [field for field in packet_row["fields"] if field["path"] == "count"]
    assert count["width"] == 2 and count["count"] == 1
    assert int.from_bytes(raw[count["position"]:count["end"]], "big") == stock["quantity"]
    changed = bytearray(raw)
    changed[count["position"]:count["end"]] = (1024).to_bytes(2, "big")
    position = stock["quantity_offset"]
    changed[position:position + 2] = (1024).to_bytes(2, "big")
    assert AUDIT["records"](raw) == AUDIT["records"](changed)
    path = context.saves / ("declared-" + case + "-438.sav")
    path.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    assert identity(ROOT / source["path"]) == source
    return identity(path)


def execute(configuration, case):
    """Round-trip one original processing boundary and independently reconcile its packet/history owners.

    @param configuration Existing qualified ordinary native product.
    @param case Exact partial material/work, completed output, outage or declared full-buffer case.
    @return Current complete persistence receipt with explicit paid-versus-declared scope.
    """
    owner, row = FACTORY["SELECT"]["producer"](configuration, "orbital-processor-verification")
    stage = {"goods-input": "goods-active", "full-steel": "steel-batch", "full-goods": "goods-batch"}.get(case, case)
    original = row["checkpoints"]["native/" + stage]
    assert identity(ROOT / original["path"]) == original
    declared = case.startswith("full-")
    source = full_buffer(original, case) if declared else original
    boundary = FACTORY["ordinary_boundary"](configuration, source)
    proof = accepted_report(boundary)
    before, after = [AUDIT["snapshot"](saved) for saved in (source, proof["after"])]
    assert before == after
    # Reconstruct all material/residue totals from the published recipe bills.
    # Exact CAPA/stock joins prove physical custody; exact original utility
    # families and native admission preserve the single actual grant authority.
    if declared:
        recipe, label = ("OSTL", "STEL") if case == "full-steel" else ("OGDS", "GOOD")
        assert after[recipe]["stock"][label] == 1024
    target = ROOT / "logs" / ("orbital-save-" + ("release" if configuration == "RelWithDebInfo" else "debug") + "-processing-" + case + "-1.json")
    assert not target.exists()
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, case=case,
                                    original=original, producer=owner, source=source, ordinary=boundary, after=proof["after"],
                                    decoded=after, declared_full_buffer_codec_fixture=declared,
                                    paid_full_buffer_production_acceptance=False, initial_content_procurement_acceptance=False,
                                    sources=[identity(Path(__file__)), identity(ROOT / "src/multilayer_recipe.cpp")]), indent=2) + "\n", encoding="utf-8")
    print(configuration, case, target.relative_to(ROOT).as_posix(), flush=True)


def bind():
    """Bind both current processing matrices and the unchanged qualified unsafe-removal contracts.

    @return Final persistence acceptance with exact current refusal and dated paid producer boundaries.
    """
    configurations = []
    # Every configuration supplies its own exact physical reconstructions and
    # current removal/output refusal contracts. Fixture inventory never funds
    # paid output counters or becomes a later chain's predecessor.
    for configuration, label in (("RelWithDebInfo", "release"), ("Debug", "debug")):
        cases = []
        for case in CASES:
            path = ROOT / "logs" / ("orbital-save-" + label + "-processing-" + case + "-1.json")
            proof = read(path)
            assert proof["configuration"] == configuration and proof["complete"]
            ordinary = accepted_report(proof["ordinary"])
            for reference in [proof["source"], proof["after"], ordinary["application"], ordinary["terminal"], *ordinary["native_processes"], *proof["sources"]]:
                assert identity(ROOT / reference["path"]) == reference
            assert len(ordinary["comparison"]["exact_families"]) == 32
            cases.append(dict(report=identity(path), **proof))
        reference = read(ROOT / "logs" / ("orbital-art-refusal-" + label + "-1.json"))["report"]
        refusals = accepted_report(reference)
        binary = read(ROOT / "logs/orbital-art-ordinary-binary-verification.json")["builds"][configuration]["test_application"]
        assert refusals["executable"] == identity(ROOT / binary["path"]) == binary
        assert "[orbital-recipe-runtime]" in refusals["tags"]
        assert "[orbital-recipe-inventory]" in refusals["tags"]
        assert "[orbital-removal-contract]" in refusals["tags"]
        for original in refusals["sources"]:
            assert identity(ROOT / original["path"]) == original
        configurations.append(dict(configuration=configuration, cases=cases, current_component_refusals=reference,
                                   refusal_cases=refusals["cases"], refusal_assertions=refusals["assertions"]))
    return dict(_documentation=__doc__, complete=True, configurations=configurations, ordinary_processes=24,
                exact_owner_and_metadata_comparisons=24 * 32, reused_current_refusal_boundary=identity(ROOT / "logs/orbital-art-refusal-verification.json"),
                declared_codec_cases=["full-steel", "full-goods"], paid_processor_producer=identity(ROOT / "logs/orbital-processor-verification.json"),
                end_to_end_acceptance=False, initial_content_procurement_acceptance=False)


def main():
    """Run at most four sequential cases per burst or bind the complete explicit matrix."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--cases", nargs="+", choices=CASES)
    parser.add_argument("--bind", type=Path)
    args = parser.parse_args()
    if args.bind:
        assert args.configuration is None and args.cases is None
        output = (ROOT / args.bind).resolve()
        assert output.is_relative_to(ROOT / "logs") and not output.exists()
        output.write_text(json.dumps(bind(), indent=2) + "\n", encoding="utf-8")
        print(identity(output), flush=True)
    else:
        assert args.configuration is not None and args.cases is not None and 1 <= len(args.cases) <= 4
        # Short explicit bursts keep the actual load deadlines bounded and
        # allow checkpointing without rerunning earlier successful members.
        for case in args.cases:
            execute(args.configuration, case)


if __name__ == "__main__":
    main()
