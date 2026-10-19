"""Observe genuine ore-to-Steel-to-Goods production through ordinary native game loops.

@file surface_entry_freight_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.3.
One invocation runs one bounded ordinary process. Original native histories,
packet references, product origins and cash ledgers establish actual output.
"""
import argparse
import json
from pathlib import Path
import runpy
import sys

from space_preview_checks import ROOT, chunks, identity

sys.path.insert(0, str(ROOT / "src/tests"))
import tbm_logistics_save_fixture as TABLE
ORDINARY = runpy.run_path(str(Path(__file__).with_name("surface_supply_delivery_checks-2.py")))
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-3.py")))
PREFIX = "roadveh[0].common[0]."


def saved_table(path, name):
    """Decode actual named physical owners, retaining gaps left by retired dense-pool items.

    @param path Original complete native save.
    @param name Four-byte actual table chunk owner.
    @return Exact live object rows with their original native pool identities.
    """
    version, stream = TABLE.read_stream(path)
    assert version >= 375
    chunk, = [row for row in TABLE.chunks(stream) if row["name"] == name]
    assert chunk["kind"] in (3, 4)
    header = TABLE.Reader(chunk["rows"][0])
    fields = TABLE.table_header(header)
    assert header.pos == len(header.data)
    result = {}
    # SlIterateArray increments dense identity even for a zero-payload hole,
    # then skips it. Delivered packets are routinely retired; these are absent
    # owners, not malformed or zero-valued cargo packets to fabricate locally.
    for index, row in enumerate(chunk["rows"][1:]):
        if not row:
            assert chunk["kind"] == 3
            continue
        reader = TABLE.Reader(row)
        owner = reader.gamma() if chunk["kind"] == 4 else index
        result[owner] = TABLE.table_record(reader, fields)
        assert reader.pos == len(row)
    return result


def observe(path):
    """Read exact original physical truck packets and useful factory input/output owners.

    @param path Actual native save from the authentic original game's ancestry.
    @return Native financial, production, provenance and parked-product observations.
    """
    company, = saved_table(path, b"PLYR").values()
    packets = saved_table(path, b"CAPA")
    industries = saved_table(path, b"INDY")
    vehicles = saved_table(path, b"VEHS")
    trucks = []
    # Each real native primary owns its saved packet references. Counts come
    # from CAPA, then agree with all four original cargo action categories;
    # copied totals or a packet shared by two trucks cannot establish stock.
    claimed = set()
    for vehicle, fields in vehicles.items():
        if fields["type"] != 1:
            continue
        name = bytes.fromhex(fields[PREFIX + "name"]).decode("utf-8")
        assert name.startswith("Surface ") and fields[PREFIX + "owner"] == 0
        references = fields[PREFIX + "cargo.packets"]
        held = []
        for reference in references:
            packet = reference - 1
            assert packet in packets and packet not in claimed
            claimed.add(packet)
            row = packets[packet]
            cargo = fields[PREFIX + "cargo_type"]
            expected_source = {8: 9, 9: 10, 5: 11}[cargo]
            assert row["source_type"] == 0 and row["source_id"] == expected_source, row
            held.append(dict(packet=packet, count=row["count"], source_type=row["source_type"], source_id=row["source_id"]))
        amount = sum(row["count"] for row in held)
        assert amount == sum(fields[PREFIX + "cargo.action_counts"])
        trucks.append(dict(id=vehicle, name=name, cargo=fields[PREFIX + "cargo_type"], capacity=fields[PREFIX + "cargo_cap"],
                           engine=fields[PREFIX + "engine_type"], tile=fields[PREFIX + "tile"],
                           state=fields["roadveh[0].state"], status=fields[PREFIX + "vehstatus"], held=amount, packets=held,
                           stopped=bool(fields[PREFIX + "vehstatus"] & 2)))
    assert len(trucks) == 5

    def received(industry, cargo):
        """@param industry Exact real factory fields. @param cargo Original input slot. @return All retained actual accepted units and stock."""
        slot, = [index for index in range(industry["accepted.count"]) if industry[f"accepted[{index}].cargo"] == cargo]
        prefix = f"accepted[{slot}]."
        total = sum(value for name, value in industry.items() if name.startswith(prefix) and name.endswith(".accepted"))
        return total, industry[prefix + "waiting"]

    def produced(industry, cargo):
        """@param industry Exact real factory fields. @param cargo Original output slot. @return All retained production/transport and stock."""
        slot, = [index for index in range(industry["produced.count"]) if industry[f"produced[{index}].cargo"] == cargo]
        prefix = f"produced[{slot}]."
        production = sum(value for name, value in industry.items() if name.startswith(prefix) and name.endswith(".production"))
        transported = sum(value for name, value in industry.items() if name.startswith(prefix) and name.endswith(".transported"))
        return production, transported, industry[prefix + "waiting"]

    iron_received, iron_waiting = received(industries[10], 8)
    steel_produced, steel_transported, steel_waiting = produced(industries[10], 9)
    steel_received, steel_input = received(industries[11], 9)
    goods_dispatched, goods_transported, goods_waiting = produced(industries[11], 5)
    # The supported native Steel adapter counts production on conversion and
    # preserves undispatched excess. Original Goods counts production on legacy
    # dispatch; its retained output buffer completes the one-to-one equation.
    assert iron_received == steel_produced + iron_waiting
    assert steel_received == goods_dispatched + goods_waiting + steel_input
    assert industries[10]["founder"] == industries[11]["founder"] == 0
    delivered = [0] * 64
    for name, values in company.items():
        if name.endswith(".delivered_cargo"):
            delivered = [first + second for first, second in zip(delivered, values, strict=True)]
    assert delivered[8] == iron_received and delivered[9] == steel_received
    reserves = [row for row in trucks if "reserve" in row["name"]]
    ready = all(row["held"] == row["capacity"] and row["stopped"] and row["state"] == 254 for row in reserves)
    return dict(cash=company["money"], debt=company["current_loan"], expense=sum(company["yearly_expenses"]),
                income=sum(value for name, value in company.items() if name.endswith(".income")),
                trucks=trucks, iron_received=iron_received, steel_produced=steel_produced, steel_transported=steel_transported,
                steel_waiting=steel_waiting, steel_received_by_goods=steel_received, goods_dispatched=goods_dispatched,
                goods_transported=goods_transported, goods_waiting=goods_waiting,
                parked_steel=sum(row["held"] for row in reserves if row["cargo"] == 9),
                parked_goods=sum(row["held"] for row in reserves if row["cargo"] == 5), ready=ready)


def execute(configuration, source, phase, require_ready):
    """Drive or restore one exact native output without a compiled gameplay hook.

    @param configuration Complete ordinary game executable.
    @param source Original actual empty/working/parked physical fleet checkpoint.
    @param phase Advance earns3000native game-loop ticks; restore earns none.
    @param require_ready Require actual useful products in all three halted depot trucks.
    @return Exact native process, physical observations and original financial conservation.
    """
    initial = observe(source)
    output, process = ORDINARY["ordinary"](configuration, source, "surface-entry-production", phase == "advance")
    final = observe(output)
    assert initial["debt"] == final["debt"] == 2000000
    assert initial["cash"] - final["cash"] == final["expense"] - initial["expense"]
    first, = saved_table(source, b"DATE").values()
    last, = saved_table(output, b"DATE").values()
    ticks = last["tick_counter"] - first["tick_counter"]
    assert first["id"] == last["id"]
    if phase == "advance":
        assert ticks == 3000
    else:
        assert ticks == 0
        before, after = chunks(source)[1], chunks(output)[1]
        changed = {key for key in before if before[key] != after[key]}
        assert changed <= {b"GLOG", b"VIEW", b"DATE"}, changed
        if b"GLOG" in changed:
            REVISION["verify_revision_log"](source, output)
        # Native Pause sets Normal (bit0). AfterLoad_LinkGraphPauseControl may
        # also hold LinkGraph (bit6) while the original saved job restarts.
        # Protect every other bit, clock and RNG field; no simulation is earned.
        for name, value in first.items():
            if name == "pause_mode":
                assert last[name] & ~64 == value & ~64 | 1, name
            else:
                assert last[name] == value, name
        assert initial == final
    if require_ready:
        assert final["ready"] and final["parked_steel"] > 0 and final["parked_goods"] > 0
        assert final["iron_received"] > 0 and final["steel_received_by_goods"] > 0 and final["income"] > 0
    return dict(process=process, ticks=ticks, initial=initial, final=final)


def main():
    """Run one bounded ordinary physical-production phase and publish fresh exact membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("advance", "restore"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--require-ready", action="store_true")
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source, args.phase, args.require_ready)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=args.phase, result=result, harness=identity(Path(__file__))), indent=2) + "\n", encoding="utf-8")
    final = result["final"]
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), ready=final["ready"], iron=final["iron_received"],
                          steel_to_goods=final["steel_received_by_goods"], steel_parked=final["parked_steel"], goods_parked=final["parked_goods"])), flush=True)


if __name__ == "__main__":
    main()
