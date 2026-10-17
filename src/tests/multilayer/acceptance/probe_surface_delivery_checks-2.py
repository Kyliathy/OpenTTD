"""Qualify actual linked Surface train/lorry delivery, native refinery intake and settled ordinary payment.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group19
Original packet holdings plus actual Industry input/history finance the single mined body's debit; processed units never substitute raw ore.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_surface_delivery_checks-1.py"))
CLASS = runpy.run_path(str(HERE / "probe_class_mission_checks-2.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*CLASS["SOURCES"], "src/tests/surface_return_delivery_native.cpp", "src/industry.h", "src/industry_cmd.cpp",
                             "src/layer_industry_processor.cpp", "src/layer_industry_processor.h", "src/layer_industry_definitions.cpp",
                             "src/economy.cpp", "src/surface_handling.cpp", "src/road_cmd.cpp", "src/rail_cmd.cpp", "src/order_cmd.cpp",
                             "src/saveload/industry_sl.cpp", "src/saveload/layer_processor_sl.cpp", "src/saveload/economy_sl.cpp",
                             *(f"src/tests/multilayer/acceptance/probe_surface_delivery_checks-{n}.py" for n in (1, 2)))))


def snapshot(saved, carrier, factory):
    """Reconcile original paid ore packets with their actual final native refinery input and payment owner.

    @param saved Exact retained ordinary438 native checkpoint.
    @param carrier Actual command-built native primary identity from this process's witness.
    @param factory Actual command-funded native industry identity from the same witness.
    @return Complete single-body/resource equation, native receiver/counter/payment state and original paid custody.
    """
    state = CLASS["snapshot"](saved, 0, require_packet_conservation=False)
    _, raw = read_stream(ROOT / saved["path"])
    tables = CLASS["FLOW"]["TABLES"]

    def fields(row):
        """Read one actual table row's original scalar values.

        @param row Exact independently decoded native row extents.
        @return Detached network-order scalars keyed by original field name.
        """
        return {f["path"]: int.from_bytes(raw[f["position"]:f["end"]], "big") for f in row["fields"]}

    processors = [fields(row) for row in tables(raw, b"LPCR")]
    processor, = processors
    # Native VarFileType::Label tables serialize the four-byte label's numeric word, unlike CDIM's direct character bytes; retain that existing little-significance label meaning.
    assert processor["industry"] == factory and processor["controller"] == 0 and processor["recipe"] == int.from_bytes(b"NFMT", "little")
    industry, = [row for row in tables(raw, b"INDY") if row["index"] == factory]
    native = fields(industry)
    _, start, _, end = riff_location(raw, b"CDIM")
    cargo, = [raw[p] for p in range(start + 1, end, 6) if raw[p + 1:p + 5] == b"NORE" and raw[p + 5] == 16]
    # Same-file cargo meaning and the actual native refinery's original accepted slots authorize this input; no runtime catalogue or cached total supplies its quantity.
    intake = sum(value for key, value in native.items() if key.endswith(".waiting") and native.get(key.removesuffix(".waiting") + ".cargo") == cargo and key.startswith("accepted["))
    consumed = produced = batches = 0
    # Saved counters are history, while Industry waiting stock is physical. Their original owner and label/kind must agree before any final native debit can join the resource equation.
    for row in tables(raw, b"LPCC"):
        counter = fields(row)
        assert counter["industry"] == factory and counter["kind"] in (0, 1, 2)
        # Only this refinery's actual Nickel input debits can join the original ore equation; other recipe meanings cannot cover a missing returned unit.
        if counter["kind"] == 1 and counter["label"] == int.from_bytes(b"NORE", "little"):
            consumed += counter["count"]
        # Refined metal is a distinct product history. Track its exact credit separately so it can never be substituted for unconverted asteroid ore.
        if counter["kind"] == 2 and counter["label"] == int.from_bytes(b"RFMT", "little"):
            produced += counter["count"]
        if counter["kind"] == 0:
            batches += counter["count"]
    assert consumed == produced == batches == 0
    company, = [row for row in tables(raw, b"PLYR") if row["index"] == 0]
    delivered_field, = [f for f in company["fields"] if f["path"] == "cur_economy[0].delivered_cargo"]
    assert delivered_field["width"] == 4 and delivered_field["count"] == 64
    delivered = int.from_bytes(raw[delivered_field["position"] + cargo * 4:delivered_field["position"] + (cargo + 1) * 4], "big")
    payments = [fields(row) for row in tables(raw, b"CAPY")]
    pending = [payment for payment in payments if payment["front"] == carrier + 1]
    vehicle, = [row for row in tables(raw, b"VEHS") if row["index"] == carrier]
    domains = sorted({f["path"].split("[")[0] for f in vehicle["fields"] if ".common[" in f["path"]})
    flow = state["flow"]
    # This exact paid scenario mined only body0. Its ten final accepted units can therefore be joined to that original body's debit without guessing provenance across unrelated mined bodies or importing another stock.
    assert all(body["initial"] == body["remaining"] for key, body in flow["bodies"].items() if key != 0)
    body = flow["bodies"][0]
    emitted = sum(packet["quantity"] for packet in flow["packets"].values() if packet["body"] == 0)
    assert body["initial"] == body["remaining"] + emitted + intake + consumed
    assert all(packet["body"] == 0 for packet in flow["packets"].values())
    # Separate class equations retain untouched Ice/Rare Earth and add only actual unconverted Industry Nickel input/history to Nickel. No RFMT product is counted as ore.
    for label, resource in flow["resources"].items():
        receiver = intake + consumed if label == "NORE" else 0
        assert resource["initial"] == sum(value for key, value in resource.items() if key != "initial") + receiver
    state.update(factory=factory, carrier=carrier, industry_input=intake, raw_consumed=consumed, rfmt_produced=produced, batches=batches,
                 native_delivered=delivered, pending_payment=pending, carrier_domains=domains, processor=processor)
    return state


def configuration(name, selectors, references):
    """Qualify one current producer burst and both original physical Surface delivery chains.

    @param name Current native executable configuration.
    @param selectors Focused then rail/road setup/collect/deliver/own selectors in exact order.
    @param references Checked direct original producer/report/process/save identities.
    @return Qualified current process counts and all actual saved ownership equations.
    """
    expected = [(None, None), *((mode, phase) for mode in ("rail", "road") for phase in ("setup", "collect", "deliver", "own"))]
    assert len(selectors) == len(expected)
    rows = {}
    proofs = []
    checkpoints = {}
    decoded = {}
    focused = None
    # Every mode needs its own paid clone and exact prerequisite; one successful train pickup cannot substitute road movement or a useful final receiver.
    for (mode, phase), selector in zip(expected, selectors):
        selected = LEAF["read"](selector)
        AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name
        path = ROOT / selected["report"]["path"]
        proof = LEAF["read"](path)
        AUDIT["inspect"](proof, references)
        assert proof["complete"] and proof["configuration"] == name and len(proof["native_reports"]) == 1
        process = LEAF["read"](ROOT / proof["native_reports"][0]["path"])
        AUDIT["inspect"](process, references)
        assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
        terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
        assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
        proof["_path"] = identity(path)
        proofs.append(proof)
        if mode is None:
            assert proof["scope"] == "focused" and not proof["paid_delivery_claim"] and process["argv"][1:] == [LEAF["TAGS"]]
            focused = proof
            continue
        assert selected["mode"] == proof["mode"] == mode and selected["phase"] == proof["phase"] == phase and not proof["added_stock"]
        assert process["argv"][1:] == ["[.surface-return-delivery-native]"]
        assert proof["source"] in process["inputs"] and proof["after"] in process["artifacts"] and proof["dated_index"] == LEAF["predecessor"](name)[0]
        if phase == "setup":
            assert proof["parent"] is None and proof["source"] == LEAF["predecessor"](name)[1]
        else:
            parent = rows[(mode, LEAF["PARENTS"][phase])]
            assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            assert proof["carrier"] == parent["carrier"] and proof["factory"] == parent["factory"]
        state = snapshot(proof["after"], proof["carrier"], proof["factory"])
        assert state["extracted"] == 20 and state["manifest"]["quota"] == 1 and state["manifest"]["services"] == 1
        assert state["carrier_domains"] == (["train"] if mode == "rail" else ["roadveh"])
        resource = state["flow"]["resources"]["NORE"]
        assert resource["surface_stock"] == (20 if phase == "setup" else 10)
        assert resource["surface_vehicle"] == (10 if phase == "collect" else 0)
        assert state["industry_input"] == state["native_delivered"] == (10 if phase in ("deliver", "own") else 0)
        if phase in ("deliver", "own"):
            assert not state["pending_payment"]
        if phase == "own":
            LEAF["compare"](proof["source"], proof["after"])
        key = mode + "/" + phase
        rows[(mode, phase)] = proof
        checkpoints[key] = proof["after"]
        decoded[key] = state
    assert focused and len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 9
    return dict(configuration=name, processes=9, focused_assertions=focused["assertions"], focused_cases=focused["cases"],
                native_assertions=sum(p["assertions"] for p in proofs if p is not focused), selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs],
                checkpoints=checkpoints, decoded_checkpoints=decoded, owner_family_comparisons=70, metadata_comparisons=4)


def main():
    """Freeze useful Surface receipts only after native train/lorry ownership, complete payments and both builds agree."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=9, required=True)
    parser.add_argument("--debug", type=Path, nargs=9, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    references = {}
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    # Native final Industry buffers and payment owners join all original orbital/Surface families; ordinary saves and configurations cannot lose or duplicate accepted cargo/income.
    for key, saved in release["checkpoints"].items():
        LEAF["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / path) for path in SOURCES],
                                         direct_references=list(references.values()), dated_producer_indexes=[identity(LEAF["ENTRY"])],
                                         cross_configuration_owner_family_comparisons=280, cross_configuration_metadata_comparisons=16,
                                         raw_ore_industry_input_per_case=10, retained_raw_packets_per_case=10, raw_consumed=0, processed_rfmt=0,
                                         native_payments_settled=True, added_stock=False, inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
