"""Freeze actual paid-hauler freight, independent refit refusals and ordinary continuation evidence.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12
Inherited historical starting stock stays distinct from new procurement; explicit native orders retain original packet custody.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
SHARED = runpy.run_path(str(HERE / "processor_freight_checks-1.py"))
FAULTS = runpy.run_path(str(HERE / "processor_freight_checks-2.py"))
SCOPED = runpy.run_path(str(HERE / "processor_freight_checks-4.py"))
AUDIT = runpy.run_path(str(HERE / "orbital_runtime_checks-3.py"))
LAYOUT = runpy.run_path(str(HERE / "heavy_contained_service_checks-4.py"))
SOURCES = tuple(dict.fromkeys((*AUDIT["SOURCES"], "src/space_manifest.cpp", "src/space_manifest.h", "src/space_vehicle.cpp", "src/space_production.cpp",
                             "src/space_production.h", "src/space_orders.cpp", "src/space_orders.h", "src/space_order_actions.cpp", "src/space_approach.cpp",
                             "src/multilayer_vehicle_catalogue.cpp", "src/multilayer_space_content.cpp", "src/saveload/vehicle_sl.cpp",
                             "src/saveload/space_cargo_preview.h", "src/tests/assembly_native_fixture.h", "src/tests/manufacturing_fixture.h",
                             "src/tests/mock_environment.h", "src/tests/processor_freight.cpp", "src/tests/processor_freight_fixture.h", "src/tests/processor_freight_native.cpp", "src/space_replacement.cpp",
                             "src/tests/manufacturing_replacement.cpp", "src/tests/manufacturing_replacement-1.cpp",
                             "src/tests/multilayer/acceptance/heavy_joined_checks-2.py", "src/tests/multilayer/acceptance/heavy_contained_service_checks-4.py",
                             *(f"src/tests/multilayer/acceptance/processor_freight_checks-{n}.py" for n in (1, 2, 3, 4)))))


def snapshot(saved):
    """Decode the original hauler, configured tonne dimensions and all packets funding its two actual endpoints.

    @param saved Exact438 positive native checkpoint identity.
    @return Independent quantities, retained feeder shares and original paid factory incarnation.
    """
    path = ROOT / saved["path"]; assert identity(path) == saved; version, raw = read_stream(path); assert version == 438
    vehicle, generation = FAULTS["hauler"](raw)
    _, body, _, end = riff_location(raw, b"SVST")
    receipt, = [p for p in range(body + 1, end, 68) if int.from_bytes(raw[p:p + 4], "big") == vehicle]
    factory = int.from_bytes(raw[receipt + 49:receipt + 51], "big"); factory_generation = int.from_bytes(raw[receipt + 51:receipt + 59], "big")
    assert int.from_bytes(raw[receipt + 43:receipt + 47], "big") == 200000
    _, body, _, end = riff_location(raw, b"CDIM"); assert body + 1 + 6 * raw[body] == end
    ordinal, = [raw[p] for p in range(body + 1, end, 6) if raw[p + 1:p + 5] == b"STEL" and raw[p + 5] == 16]
    tables = FAULTS["TABLES"]["table_rows"]; packets = {}
    # Native table metadata selects actual positive quantities and retained feeder credit. A cached stock or carrier total alone cannot fund this route.
    for row in tables(raw, b"CAPA"):
        count, = [f for f in row["fields"] if f["path"] == "count"]
        credit, = [f for f in row["fields"] if f["path"] == "feeder_share"]
        packets[row["index"]] = (int.from_bytes(raw[count["position"]:count["end"]], "big"), int.from_bytes(raw[credit["position"]:credit["end"]], "big", signed=True))
    owned = set()

    def container(references):
        """Sum one actual exclusive packet container.

        @param references Original raw CAPA identities, with native SLE index+1 already decoded when applicable.
        @return Actual quantity and feeder credit owned by only this container.
        """
        quantity = credit = 0
        # Source module, carrier and destination remain separate physical owners; one packet cannot finance two route participants at any checkpoint.
        for packet in references:
            assert packet not in owned and packets[packet][0] > 0; owned.add(packet)
            amount, feeder = packets[packet]; quantity += amount; credit += feeder
        return quantity, credit

    # Schema438 retains the exact436 SPST layout. This is a parser-layout selector only; no original header or bytes are rewritten.
    structures = LAYOUT["structure_rows"](raw, 436)["rows"]; assert structures[factory]["generation"] == factory_generation
    storage = structures[1]; assert storage["generation"] == 2 and storage["kind"] == 8
    start = storage["start"]; assert int.from_bytes(raw[start + 19:start + 23], "big") == 120000 and int.from_bytes(raw[start + 23:start + 27], "big") == 296
    position = start + 29; consumed = []
    # The receiving buffer already paid its original complete construction bill. Native activation adds facilities only; it cannot grant replacement material or a fabricated frame.
    for _ in range(2):
        consumed.append(int.from_bytes(raw[position:position + 2], "big"))
        assert raw[position + 2:position + 6] == bytes(4); position += 6
    assert consumed == [8, 8]
    amounts, credits = {}, {}
    for name, site in (("core", 0), ("storage", 1)):
        stock = structures[site].get("stock", {}).get(ordinal, dict(quantity=0, packet_offsets=[]))
        refs = [int.from_bytes(raw[p:p + 4], "big") for p in stock["packet_offsets"]]
        amount, credit = container(refs); assert amount == stock["quantity"]; amounts[name] = amount; credits[name] = credit
    hull, = [row for row in tables(raw, b"VEHS") if row["index"] == vehicle]
    cargo, capacity, references = (FAULTS["field"](hull, name) for name in ("cargo_type", "cargo_cap", "cargo.packets"))
    refs = [int.from_bytes(raw[p:p + 4], "big") - 1 for p in range(references["position"], references["end"], 4)]
    amounts["hauler"], credits["hauler"] = container(refs)
    assert not refs or raw[cargo["position"]] == ordinal
    return dict(vehicle=vehicle, generation=generation, factory=factory, factory_generation=factory_generation, storage_active="stock" in storage, quantities=amounts,
                feeder_shares=credits, capacity=int.from_bytes(raw[capacity["position"]:capacity["end"]], "big"))


def qualify(configuration, path, references):
    """Qualify one actual current component, native continuation or independent refusal process.

    @param configuration Matching current test executable build.
    @param path Explicit original semantic proof path.
    @param references Shared checked direct artifact identities; dated indexes remain direct boundaries only.
    @return Original admitted proof with its own current identity attached.
    """
    proof = SHARED["read"](path); AUDIT["inspect"](proof, references)
    assert proof["configuration"] == configuration and proof["complete"] and not proof["added_stock"] and len(proof["native_reports"]) == 1
    assert proof["dated_index"]["sha256"] == SHARED["ENTRY_SHA"]
    process = SHARED["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
    assert process["configuration"] == configuration and process["executable"] == identity(ROOT / "build" / configuration / "openttd_test.exe")
    assert process["exit_status"] == (1 if proof["expected_full_refusal"] else 0)
    assert process["argv"][1:] == [SHARED["TAGS"] if proof["scope"] == "focused" else "[.processor-freight-native]"]
    terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    if proof["expected_full_refusal"]:
        assert "Broken savegame" in terminal and "Fatal test error" not in terminal and "Assertion failed" not in terminal
    else:
        assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
    if proof["scope"] != "focused":
        assert proof["source"] in process["inputs"]
    if proof["scope"] == "preview":
        assert "preview_rejected=true authority_unchanged=true" in terminal and proof["candidate"] in process["inputs"]
    if proof["after"]:
        assert proof["after"] in process["artifacts"] and chunks(ROOT / proof["after"]["path"])[0] == 438
        assert f"PROCESSOR_FREIGHT native=true phase={proof['phase']} added_stock=false resave=" + ("true" if proof["scope"] == "resave" else "false") in terminal
    proof["_path"] = identity(path)
    return proof


def configuration(name, selectors, references):
    """Qualify exact freight scope membership, paid-hull material flow and independent reader refusals.

    @param name Matching current native build.
    @param selectors Seven exact original bounded selectors in runner order.
    @param references Deduplicated checked current direct identity graph.
    @return Qualified configuration with exact process membership and independently decoded checkpoints.
    """
    assert len(selectors) == 7; native, own, resumed, proofs, refusals = {}, {}, {}, [], {}; focused = None
    for scope, path in zip(SCOPED["GROUPS"], selectors):
        selected = SHARED["read"](path); AUDIT["inspect"](selected, references)
        assert selected["configuration"] == name and selected["complete"] and selected["scope"] == scope
        rows = [qualify(name, ROOT / p, references) for p in selected["reports"]]; proofs.extend(rows)
        if scope == "focused":
            focused, = rows; assert focused["scope"] == "focused" and focused["cases"] == 12
            continue
        if scope.startswith("reject-"):
            copied, full = rows; fault = scope.removeprefix("reject-"); claim = selected["refusal"]
            assert claim["fault"] == fault and copied["scope"] == "preview" and full["scope"] == "full-refusal"
            assert copied["valid_source"] == full["valid_source"] == copied["source"] == claim["source"] == native["loaded"]["after"]
            assert copied["candidate"] == full["source"] == full["candidate"] == claim["candidate"]
            assert copied["parent"] == full["parent"] == claim["parent"] == native["loaded"]["_path"]
            refusals[fault] = claim
            continue
        assert len(rows) == len(SCOPED["GROUPS"][scope])
        # Every same-phase own-resave repeats no controls. A resumed next action must start from that exact own output and converge with its original native action.
        for phase, proof in zip(SCOPED["GROUPS"][scope], rows):
            assert proof["phase"] == phase
            target = native if scope == "native" else own if scope == "own" else resumed
            assert phase not in target; target[phase] = proof
            assert proof["scope"] == ("resave" if target is own else "native")
            parent = native.get(SHARED["PARENTS"].get(phase)) if target is native else native[phase] if target is own else own[SHARED["PARENTS"][phase]]
            if parent:
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            else:
                assert phase == "inspected" and proof["parent"] is None and proof["source"] == SHARED["predecessor"](name)[2]
            if target is own:
                AUDIT["compare"](proof["source"], proof["after"])
            elif target is resumed:
                AUDIT["compare"](native[phase]["after"], proof["after"])
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 22
    assert len(native) == 6 and len(own) == 5 and len(resumed) == 4 and set(refusals) == set(FAULTS["FAULTS"]) and focused
    checkpoints = {operation + "/" + phase: proof["after"] for operation, rows in (("native", native), ("own", own), ("resumed", resumed)) for phase, proof in rows.items()}
    decoded = {phase: snapshot(saved) for phase, saved in checkpoints.items()}; initial = decoded["native/inspected"]; core, storage = (initial["quantities"][p] for p in ("core", "storage"))
    assert core == 2 and storage == 0 and initial["quantities"]["hauler"] == 0
    # The source, carrier and chosen destination account for exactly the same actual Steel and feeder credit through the complete round trip, including every ordinary repeated continuation.
    for key, state in decoded.items():
        phase = key.split("/")[1]; expected = dict(core=core, storage=storage, hauler=0)
        if phase in ("loaded", "travelling"):
            expected.update(core=core - 2, hauler=2)
        elif phase == "transferred":
            expected.update(core=core - 2, storage=storage + 2)
        assert state["storage_active"] == (phase != "inspected")
        assert state["quantities"] == expected and sum(state["feeder_shares"].values()) == sum(initial["feeder_shares"].values())
        assert (state["vehicle"], state["generation"], state["factory"], state["factory_generation"]) == (initial["vehicle"], initial["generation"], initial["factory"], initial["factory_generation"])
    return dict(configuration=name, selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs], processes=22,
                focused_assertions=focused["assertions"], focused_cases=12, native_assertions=sum(p["assertions"] for p in proofs if p is not focused),
                ordinary_own_resaves=5, own_continuations=4, full_copied_refusal_pairs=3, physical_owner_comparisons=270, metadata_comparisons=18,
                checkpoints=checkpoints, decoded_checkpoints=decoded, refusals=refusals)


def main():
    """Freeze both exact current freight graphs after material, refusal and ordinary continuation checks pass."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=7, required=True)
    parser.add_argument("--debug", type=Path, nargs=7, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    assert identity(ROOT / "logs/orbital-processor-verification.json")["sha256"] == "45ddf32a5b13afccea72e1af6753cfe87cfcea5119d351ba063f9ee875406747"
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    assert release["checkpoints"].keys() == debug["checkpoints"].keys()
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    result = dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / p) for p in SOURCES],
                  direct_references=list(references.values()), dated_producer_indexes=[identity(SHARED["ENTRY"]), identity(ROOT / "logs/orbital-processor-verification.json")],
                  cross_configuration_physical_comparisons=450, cross_configuration_metadata_comparisons=30,
                  orbital_cargodist_endpoints=False, native_explicit_hauler_exchange=True, inherited_starting_content_procurement=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], flush=True)


if __name__ == "__main__":
    main()
