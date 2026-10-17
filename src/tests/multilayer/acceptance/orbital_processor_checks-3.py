"""Audit paid processor construction, saved material ledgers and exact ordinary continuations.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group11
The dated Core supplies inherited starting inventory; actual commands fund every new processor and mined input.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
SHARED = runpy.run_path(str(HERE / "orbital_processor_checks-1.py"))
SCOPED = runpy.run_path(str(HERE / "orbital_processor_checks-2.py"))
AUDIT = runpy.run_path(str(HERE / "orbital_runtime_checks-3.py"))
LAYOUT = runpy.run_path(str(HERE / "heavy_contained_service_checks-4.py"))
TABLES = runpy.run_path(str(HERE / "heavy_joined_checks-2.py"))
SOURCES = tuple(dict.fromkeys((*AUDIT["SOURCES"], "src/tests/orbital_processor_native.cpp", "src/tests/assembly_native_fixture.h",
                             *(f"src/tests/multilayer/acceptance/orbital_processor_checks-{n}.py" for n in (1, 2, 3)),
                             "src/tests/multilayer/acceptance/heavy_contained_service_checks-4.py",
                             "src/tests/multilayer/acceptance/heavy_joined_checks-2.py",
                             "src/space_orders.cpp", "src/space_manifest.cpp", "src/space_vehicle.cpp", "src/launch_center.cpp",
                             "src/launch_operations.cpp", "src/space_production.cpp", "src/multilayer_space_content.cpp",
                             "src/multilayer_vehicle_catalogue.cpp", "src/saveload/cargopacket_sl.cpp", "src/saveload/utility_power_sl.cpp")))
# Independent published bills qualify saved history rather than trusting native console totals.
BILLS = {"OSTL": ({"NORE": 4}, {"STEL": 2}, 32), "OGDS": ({"STEL": 2, "RARE": 1}, {"GOOD": 4}, 16),
         "OREF": ({"NORE": 4}, {"RFMT": 2}, 0), "PICE": ({"WICE": 2}, {"PROP": 1}, 0)}


def read(path):
    """Read exact retained evidence.

    @param path Explicit selector or semantic report; no directory discovery is allowed.
    @return Unchanged parsed evidence.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def records(raw):
    """Decode bounded processing metadata independently of native objects.

    @param raw Exact decompressed438 stream.
    @return Original generation-qualified rows and paid/material history.
    """
    _, position, _, end = riff_location(raw, b"SPRC")

    def take(width, label=False):
        """Read one admitted original field.

        @param width Exact fixed field width inside SPRC.
        @param label Whether these original bytes represent a stable four-byte meaning.
        @return Stable ASCII label or native little-endian unsigned scalar.
        """
        nonlocal position
        assert 0 <= width <= end - position
        value = raw[position:position + width]; position += width
        return value.decode("ascii") if label else int.from_bytes(value, "little")

    def counters(cap):
        """Decode one canonical bounded historical map.

        @param cap Original maximum distinct recipe or cargo meanings.
        @return Sorted unique original label/quantity pairs.
        """
        count = take(1); assert count <= cap; result = {}
        # Strict original ordering prevents duplicated meanings from concealing a second material or spent-work owner.
        for _ in range(count):
            key = take(4, True); value = take(8)
            assert not result or key > next(reversed(result))
            result[key] = value
        return result

    retired = take(8); retired_batches = counters(4); count = take(2); result = {}
    assert retired == 0 and not retired_batches and count <= 65535
    # Each row remains metadata for one original module, never an alternative stock or electrical account.
    for _ in range(count):
        row = dict(site=take(2), generation=take(8), choice=take(4, True), source=take(2), source_generation=take(8),
                   consumer=take(4), revision=take(8), baseline=take(8), enabled=take(1), tick=take(8), paid=take(8), discarded=take(8))
        electrical = take(1); assert row["enabled"] <= 1 and electrical <= 1
        row["electrical"] = dict(recipe=take(4, True), consumer=take(4), work=take(4), power=take(8), pending=take(8),
                                 sequence=take(8), interval=take(8), interval_paid=take(8)) if electrical else None
        row.update(batches=counters(4), consumed=counters(8), produced=counters(8), residue=counters(4))
        assert row["site"] not in result; result[row["site"]] = row
        consumed, produced, residue = {}, {}, {}
        # Reconstruct all historical quantities from independent published bills; retained packets and committed inputs cannot represent the same units.
        for recipe, batches in row["batches"].items():
            assert recipe in BILLS
            inputs, outputs, waste = BILLS[recipe]; residue[recipe] = batches * waste
            for cargo, amount in inputs.items():
                consumed[cargo] = consumed.get(cargo, 0) + batches * amount
            for cargo, amount in outputs.items():
                produced[cargo] = produced.get(cargo, 0) + batches * amount
        assert row["consumed"] == consumed and row["produced"] == produced and row["residue"] == residue
        receipt = row["electrical"]
        if receipt:
            assert receipt["recipe"] == row["choice"] and receipt["consumer"] == row["consumer"]
            assert receipt["work"] == 74 and receipt["power"] == 55500 and receipt["pending"] <= 55500
            assert row["paid"] == sum(row["batches"].values()) * 55500 + row["discarded"] + receipt["pending"]
            assert row["baseline"] == 0
        else:
            assert row["paid"] == row["discarded"] == 0 and row["baseline"] <= 74 and row["consumer"] == 2**32 - 1
    assert position == end
    return result


def snapshot(saved):
    """Join actual saved stock to the same file's dimensions, packet table and processing owners.

    @param saved Exact438 native checkpoint identity.
    @return Independently decoded processor histories, physical quantities and paid construction bills.
    """
    path = ROOT / saved["path"]; assert identity(path) == saved
    version, raw = read_stream(path); assert version == 438
    processing = records(raw)
    # Schema438 deliberately retains the original436 SPST layout. This argument selects its parser layout; no header or input bytes are rewritten.
    structures = LAYOUT["structure_rows"](raw, 436)["rows"]
    _, body, _, end = riff_location(raw, b"CDIM"); count = raw[body]; assert body + 1 + 6 * count == end
    dimensions = {}; labels = set()
    for offset in range(body + 1, end, 6):
        ordinal = raw[offset]; label = raw[offset + 1:offset + 5].decode("ascii"); weight = raw[offset + 5]
        assert ordinal not in dimensions and label not in labels and weight > 0
        dimensions[ordinal] = (label, weight); labels.add(label)
    assert dimensions[9] == ("STEL", 16) and dimensions[5] == ("GOOD", 8)
    packets = {}
    # Native table schemas and row identities select real counts; cached stock totals alone cannot establish conservation.
    for packet in TABLES["table_rows"](raw, b"CAPA"):
        field, = [field for field in packet["fields"] if field["path"] == "count"]
        assert field["width"] == 2 and field["count"] == 1
        packets[packet["index"]] = int.from_bytes(raw[field["position"]:field["end"]], "big")
    owned = set()

    def quantity(offsets):
        """Sum exclusive physical packets from original native references.

        @param offsets Original packet/next-hop field positions within one actual structure container.
        @return Positive same-file physical quantity, admitting empty containers as zero.
        """
        total = 0
        # No two structure containers may claim one packet; retained input cannot also fund construction or output.
        for offset in offsets:
            packet = int.from_bytes(raw[offset:offset + 4], "big")
            assert packet not in owned and packets[packet] > 0
            owned.add(packet); total += packets[packet]
        return total

    result = {}
    # Join every real processor to its original paid module and all its actual material containers, without asking the active cargo registry.
    for site, record in processing.items():
        row = structures[site]; assert row["generation"] == record["generation"] and "stock" in row
        stock = {}
        for ordinal, part in row["stock"].items():
            amount = quantity(part["packet_offsets"]); assert amount == part["quantity"]
            if amount:
                stock[dimensions[ordinal][0]] = amount
        start = row["start"]; position = start + 29; consumed, held = [], []
        for _ in range(2):
            consumed.append(int.from_bytes(raw[position:position + 2], "big"))
            cached = int.from_bytes(raw[position + 2:position + 4], "big"); count = int.from_bytes(raw[position + 4:position + 6], "big")
            assert count <= 128; position += 6
            amount = quantity([position + 6 * index for index in range(count)]); assert cached == amount
            held.append(amount); position += 6 * count
        construction = dict(capital=int.from_bytes(raw[start + 19:start + 23], "big"), work=int.from_bytes(raw[start + 23:start + 27], "big"),
                            consumed=consumed, held=held, activation=int.from_bytes(raw[position + 1:position + 3], "big"))
        result[record["choice"]] = dict(record=record, stock=stock, construction=construction)
    assert len(result) == len(processing)
    return result


def qualify(configuration, path, references):
    """Verify one actual current processor child and its original terminal/save identities.

    @param configuration Current native executable build.
    @param path Exact semantic report selected by the bounded invocation.
    @param references Deduplicated direct artifact identities already checked.
    @return Original proof with its identity attached.
    """
    proof = read(path); AUDIT["inspect"](proof, references)
    assert proof["configuration"] == configuration and proof["complete"] and not proof["added_stock"]
    assert len(proof["native_reports"]) == 1
    process = read(ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
    assert process["configuration"] == configuration and process["exit_status"] == 0
    assert process["executable"] == identity(ROOT / "build" / configuration / "openttd_test.exe")
    assert process["argv"][1:] == ["[.orbital-processor-native]"] and process["inputs"] == [proof["source"]]
    assert proof["after"] in process["artifacts"]
    terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    assert f"All tests passed ({proof['assertions']} assertions in 1 test case)" in terminal
    assert "ORBITAL_PROCESSOR native=true phase=" + proof["phase"] + " added_stock=false resave=" + ("true" if proof["operation"] == "resave" else "false") in terminal
    assert proof["dated_index"]["sha256"] == SHARED["ENTRY_SHA"]
    assert chunks(ROOT / proof["after"]["path"])[0] == 438
    proof["_path"] = identity(path)
    return proof


def configuration(name, selectors, references):
    """Qualify exact native, own-resave, resumed and component-refusal membership.

    @param name Current matching Release or Debug build.
    @param selectors Eight explicit bounded scope selectors in original order.
    @param references Shared direct artifact identity graph.
    @return Complete accepted configuration and independently decoded material milestones.
    """
    assert len(selectors) == len(SCOPED["GROUPS"])
    native, own, resumed, proofs, focused = {}, {}, {}, [], None
    for scope, path in zip(SCOPED["GROUPS"], selectors):
        selected = read(path); AUDIT["inspect"](selected, references)
        assert selected["scope"] == scope and selected["configuration"] == name and selected["complete"]
        if scope == "focused":
            assert len(selected["reports"]) == 1
            focused = AUDIT["qualify"](name, ROOT / selected["reports"][0], references)
            assert focused["cases"] == 32 and focused["assertions"] == 3738
            continue
        assert len(selected["reports"]) == len(SCOPED["GROUPS"][scope])
        # Explicit scope membership identifies every independent child; neither a reused report nor an unrelated latest save can supply a missing checkpoint.
        for phase, report in zip(SCOPED["GROUPS"][scope], selected["reports"]):
            proof = qualify(name, ROOT / report, references); assert proof["phase"] == phase
            proofs.append(proof)
            target = native if scope.startswith("native-") else own if scope.startswith("own-") else resumed
            assert phase not in target; target[phase] = proof
            assert proof["operation"] == ("resave" if target is own else "native")
            parent = native.get(SHARED["PARENTS"].get(phase)) if target is native else native[phase] if target is own else own[SHARED["PARENTS"][phase]]
            if parent:
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            else:
                assert phase == "inspected" and proof["parent"] is None and proof["source"] == SHARED["predecessor"](name)[2]
            if target is own:
                AUDIT["compare"](proof["source"], proof["after"])
            elif target is resumed:
                AUDIT["compare"](native[phase]["after"], proof["after"])
    assert len(native) == 29 and len(own) == 12 and len(resumed) == 7 and focused
    assert len({p["native_reports"][0]["path"] for p in proofs}) == 48
    milestones = {phase: snapshot(native[phase]["after"]) for phase in ("steel-partial", "steel-outage", "steel-cancelled", "steel-restarted", "goods-partial", "refinery-partial", "refinery-retarget", "ice-partial", "ice-batch")}
    assert milestones["steel-partial"]["OSTL"]["record"] == milestones["steel-outage"]["OSTL"]["record"]
    assert milestones["steel-partial"]["OSTL"]["stock"] == {"NORE": 20}
    assert milestones["steel-partial"]["OSTL"]["record"]["electrical"]["pending"] == 12750
    cancelled = milestones["steel-cancelled"]["OSTL"]
    assert cancelled["stock"] == {"NORE": 20} and cancelled["record"]["discarded"] == cancelled["record"]["paid"] == 12750
    assert cancelled["record"]["electrical"]["pending"] == 0 and not cancelled["record"]["enabled"]
    restarted = milestones["steel-restarted"]["OSTL"]
    assert restarted["stock"] == {"NORE": 16, "STEL": 2} and restarted["record"]["paid"] == 68250 and restarted["record"]["discarded"] == 12750
    assert milestones["goods-partial"]["OGDS"]["stock"] == {"STEL": 2, "RARE": 20}
    assert milestones["goods-partial"]["OGDS"]["record"]["electrical"]["pending"] == 12750
    assert milestones["refinery-partial"]["OREF"]["record"]["baseline"] == milestones["ice-partial"]["PICE"]["record"]["baseline"] == 37
    retarget = milestones["refinery-retarget"]["PICE"]
    assert retarget["stock"] == {"NORE": 20} and retarget["record"]["baseline"] == 0 and not any(retarget["record"]["batches"].values())
    final = milestones["ice-batch"]; assert set(final) == {"OSTL", "OGDS", "PICE"}
    for row in final.values():
        assert row["construction"] == dict(capital=220000, work=740, consumed=[10, 16], held=[0, 0], activation=0)
    assert final["OSTL"]["record"]["batches"] == {"OSTL": 10} and final["OSTL"]["stock"] == {}
    assert final["OGDS"]["record"]["batches"] == {"OGDS": 1} and final["OGDS"]["stock"] == {"GOOD": 4, "RARE": 19}
    assert final["PICE"]["record"]["batches"] == {"OREF": 1, "PICE": 1} and final["PICE"]["stock"] == {"NORE": 16, "WICE": 18, "RFMT": 2, "PROP": 1}
    checkpoints = {operation + "/" + phase: proof["after"] for operation, rows in (("native", native), ("own", own), ("resumed", resumed)) for phase, proof in rows.items()}
    return dict(configuration=name, selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs] + [focused["_path"]],
                processes=49, focused_assertions=focused["assertions"], focused_cases=focused["cases"], native_assertions=sum(p["assertions"] for p in proofs),
                native_checkpoints=29, ordinary_own_resaves=12, own_continuations=7, checkpoints=checkpoints, material_milestones=milestones,
                physical_owner_comparisons=19 * 30, metadata_comparisons=19 * 2)


def main():
    """Freeze both current bounded verification graphs after all exact material and continuation joins pass."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=8, required=True)
    parser.add_argument("--debug", type=Path, nargs=8, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    assert identity(ROOT / "logs/orbital-runtime-verification.json")["sha256"] == "95b892b2c0d6c5abc9f5885764eb15de858cd403020ea0d2157d5857b8f6edbd"
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    assert release["checkpoints"].keys() == debug["checkpoints"].keys()
    for phase, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][phase])
    assert release["material_milestones"] == debug["material_milestones"]
    result = dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / p) for p in SOURCES],
                  direct_references=list(references.values()), dated_producer_indexes=[identity(SHARED["ENTRY"]), identity(ROOT / "logs/orbital-runtime-verification.json")],
                  physical_owner_families=[p.decode("ascii") for p in SHARED["PHYSICAL"]], cross_configuration_physical_comparisons=48 * 30,
                  cross_configuration_metadata_comparisons=48 * 2, native_paid_processor_construction=True, native_mined_inputs=True,
                  inherited_starting_content_procurement=False, component_refusals_distinct_from_native_supply=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], flush=True)


if __name__ == "__main__":
    main()
