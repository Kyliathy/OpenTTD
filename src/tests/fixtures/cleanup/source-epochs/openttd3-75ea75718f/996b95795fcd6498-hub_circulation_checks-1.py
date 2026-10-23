"""Verify real repeated Goods circulation/relink with retained packet history and no income.

@file hub_circulation_checks-1.py Exact Group19 predecessor, native per-tick census, replay and ordinary26-owner resaves.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group20.
"""
import argparse
import json
import os
import re
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, Reader, chunks, identity, ordinary
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def packet_history(snapshot, selected):
    """Read canonical saved CargoPacket fields without invoking an in-vehicle getter on station cargo.

    @param snapshot Actual native423 snapshot.
    @param selected Packet IDs observed in its actual Goods owners at this saved boundary.
    @return Weighted units/feeder share by all saved provenance, age, routing and travel fields.
    """
    version, data = chunks(snapshot); assert version == 423
    reader = Reader(data[b"CAPA"]); header = Reader(reader.take(reader.gamma() - 1)); columns = []
    # CAPA is the public flat native table, with exact scalar types from VarFileType. Reject a changed/nested schema instead of guessing widths or normalizing histories.
    while True:
        kind = header.byte()
        if kind == 0:
            break
        name = header.take(header.gamma()).decode("utf-8")
        assert kind in (1, 2, 3, 4, 5, 6, 7, 8)
        columns.append((name, (1, 1, 2, 2, 4, 4, 8, 8)[kind - 1], kind in (1, 3, 5, 7)))
    assert header.pos == len(header.data)
    assert {name for name, _, _ in columns} == {"source", "source_xy", "loaded_at_xy", "count", "periods_in_transit", "feeder_share", "source_type", "source_id", "travelled.x", "travelled.y"}
    aggregate = {}; found = set(); packet = -1
    # Select only IDs emitted by the real before/after owner census. Splits/merges can change physical IDs; their quantities and canonical encoded travel vectors must still be preserved.
    while True:
        length = reader.gamma()
        if length == 0:
            break
        # CAPA uses the native dense Table framing: the row ordinal is its actual pool ID, and a one-byte empty row preserves a vacant slot. VEHS's sparse-index prefix must not be copied here.
        packet += 1
        row = Reader(reader.take(length - 1))
        if not row.data:
            continue
        fields = {name: int.from_bytes(row.take(width), "big", signed=signed) for name, width, signed in columns}
        assert row.pos == len(row.data)
        if packet not in selected:
            continue
        assert packet not in found; found.add(packet)
        key = tuple((name, value) for name, value in sorted(fields.items()) if name not in ("count", "feeder_share"))
        count, share = aggregate.get(key, (0, 0)); aggregate[key] = (count + fields["count"], share + fields["feeder_share"])
    assert reader.pos == len(reader.data) and found == selected
    return aggregate


def native(context, configuration, source):
    """Observe actual repeated circulation, never reconstructed packet ages or fees.

    @param context Explicit native runtime/artifact owner.
    @param configuration Current built native configuration.
    @param source Exact accepted paid Group19 settlement, or its saved before input.
    @return Native before/after snapshots for whole-owner comparison.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("hub_circulation_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_HUB_CIRCULATION_SOURCE=str(source))
    log = context.logs / "native-hub-circulation.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.hub-circulation]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    text = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in text and "HUB_CIRCULATION native=true cycles=32 legs=64" in text, log
    assert "canonical_age_preserved=true" in text and "no_income=true" in text and "actual_fees=true" in text
    before, after = context.saves / "hub-circulation-before-423.sav", context.saves / "hub-circulation-after-423.sav"
    packets = {edge: {int(packet) for observed, packet in re.findall(r"HUB_CARGO edge=(before|after) packet=(\d+)", text) if observed == edge} for edge in ("before", "after")}
    assert packets["before"] and packets["after"]
    assert packet_history(before, packets["before"]) == packet_history(after, packets["after"]), "Actual encoded packet age/provenance/routing/travel/feeder history changed."
    return before, after


def main():
    """Compare actual per-tick packet/ledger invariants with replay and ordinary before/after owners."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--predecessor", type=Path, required=True)
    args = parser.parse_args()
    predecessor = args.predecessor.resolve(); original_report = identity(predecessor)
    proof = json.loads(predecessor.read_text(encoding="utf-8"))
    assert proof["configuration"] == args.configuration and proof["stage"] == "settle"
    source = ROOT / proof["after"]["path"]; original = identity(source); assert original == proof["after"]
    context = current_run_context("space", "hub-circulation-verify")
    before, after = native(context.for_feature("space", "hub-circulation-native"), args.configuration, source)
    replay_before, replay_after = native(context.for_feature("space", "hub-circulation-replay"), args.configuration, before)
    paused_saves = {}
    # Every actual financial/packet/member-generation/Space owner survives saved-input replay and ordinary loading; no verifier normalizes histories or masks split/merge effects.
    for edge, expected_path, replay_path in (("before", before, replay_before), ("after", after, replay_after)):
        assert chunks(expected_path)[0] == 423 and chunks(replay_path)[0] == 423
        expected, actual = chunks(expected_path)[1], chunks(replay_path)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("replay", edge, family)
        paused = context.for_feature("space", "hub-circulation-paused-" + edge)
        ordinary(paused, {"name": "hub-circulation-" + edge, "snapshot": identity(expected_path)}, args.configuration)
        saved = paused.saves / "ordinary-resave.sav"; actual = chunks(saved)[1]
        for family in EXACT:
            assert expected[family] == actual[family], ("ordinary", edge, family)
        paused_saves[edge] = identity(saved)
    assert identity(source) == original and identity(predecessor) == original_report
    report = context.reports / "hub-circulation.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group20. Original recovered40 Goods alternate through32 actual reverse/outbound handling cycles and completed relinks. Per-tick native Goods census includes historical stock and assigned packets, rejects double ownership, and preserves recorded transit age/source/routing/horizontal-distance/feeder-share history while allowing legitimate splits/merges. Actual company fees remain expenses, with no final income or delivered cargo. No private packet age/job timer/cash/vehicle manipulation.",
        "configuration": args.configuration, "predecessor": original_report, "source": original, "before": identity(before), "after": identity(after),
        "replay_before": identity(replay_before), "replay_after": identity(replay_after), "ordinary_resaves": paused_saves,
        "cycles": 32, "legs": 64, "exact_owner_families": len(EXACT)}, indent=2) + "\n", encoding="utf-8")
    print(f"Hub circulation: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
