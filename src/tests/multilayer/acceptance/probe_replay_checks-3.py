"""Qualify original paid view/queued twins and independent full socket peers beyond RNG-only synchronization.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20
Native hull receipts, finite bodies, unique packet holdings, descent jobs, company finances and payment/Industry owners remain authoritative.
"""
import argparse
import json
from pathlib import Path
import re
import runpy

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_replay_checks-1.py"))
FLOW = runpy.run_path(str(HERE / "probe_conservation_checks-3.py"))
CLASS = runpy.run_path(str(HERE / "probe_class_mission_checks-2.py"))
AUDIT = LEAF["AUDIT"]
ORDER = (("focused", "hidden"), ("local", "visible"), ("local", "hidden"), ("queue", "visible"), ("queue", "hidden"), ("recall-queue", "visible"), ("recall-queue", "hidden"))
SOURCES = tuple(dict.fromkeys((*CLASS["SOURCES"], "src/tests/probe_replay_native.cpp", "src/tests/probe_extraction.cpp", "src/tests/heavy_network_native.cpp",
                             "src/company_cmd.h", "src/company_cmd.cpp", "src/network/network.cpp", "src/command_type.h", "src/command.cpp", "src/space_vehicle_cmd.h", "src/asteroid_gui.cpp", "src/network/network_command.cpp",
                             "src/network/network_server.cpp", "src/network/network_client.cpp", "src/timer/timer_game_tick.cpp",
                             "src/saveload/geology_sl.cpp", "src/saveload/layer_mine_sl.cpp", "src/saveload/multilayer_sl.cpp", "src/saveload/map_sl.cpp", "src/saveload/misc_sl.cpp", "src/saveload/industry_sl.cpp", "src/saveload/layer_processor_sl.cpp", "src/saveload/economy_sl.cpp",
                             *(f"src/tests/multilayer/acceptance/probe_replay_checks-{n}.py" for n in (1, 2, 3)))))


def snapshot(saved, stage, quantity):
    """Independently reconcile original paid hull/manifest/descent owners and finite raw resources.

    @param saved Exact actual native438 checkpoint identity.
    @param stage Physically reached command, work, return, descent or recovered phase.
    @param quantity Actual original mission's20 or recalled partial hold's2 units.
    @return Detached native paid/mission/custody facts and all per-body/per-resource conservation equations.
    """
    assert identity(ROOT / saved["path"]) == saved
    flow = FLOW["inventory"](saved)
    version, raw = read_stream(ROOT / saved["path"])
    assert version == 438

    def integer(position, width):
        """Read a bounded original native scalar.

        @param position Same-file field offset.
        @param width Exact published native field width.
        @return Original unsigned network-order integer.
        """
        return int.from_bytes(raw[position:position + width], "big")

    _, start, _, end = riff_location(raw, b"SVST")
    receipt, = [p for p in range(start + 1, end, 68) if integer(p, 4) == 16]
    assert integer(receipt + 4, 8) == 2 and integer(receipt + 41, 2) == 15 and integer(receipt + 43, 4) == 100000 and integer(receipt + 39, 2) == 2
    assert [integer(receipt + 13, 2), integer(receipt + 15, 8)] == [4, 3]
    row, = [r for r in FLOW["TABLES"](raw, b"VEHS") if r["index"] == 16]
    fields = {re.sub(r"\[\d+\]", "", f["path"]): f for f in row["fields"]}

    def field(name):
        """Read an actual paid native hull field.

        @param name Published normalized Space field name.
        @return Original saved scalar, with no active-world state lookup.
        """
        f = fields["space." + name]
        return integer(f["position"], f["end"] - f["position"])

    assert field("mission.target") == 0 and field("common.cargo_cap") == 20
    _, start, _, end = riff_location(raw, b"SPSU")
    p = start + 1
    manifest = None
    # Walk every original manifest under its own bounded layout; only original16/gen2 may finance this mission's shells, allowance and paid service history.
    for _ in range(raw[start]):
        craft, count = integer(p, 4), raw[p + 60]
        record = dict(generation=integer(p + 4, 8), service_ticks=integer(p + 38, 2), quota=raw[p + 40], pending=raw[p + 42], services=integer(p + 52, 8),
                      pods=[[integer(q, 2), integer(q + 2, 8)] for q in range(p + 61, p + 61 + 10 * count, 10)])
        p += 61 + 10 * count
        # Each supply reference list remains native custody. No service part or allowance is granted while decoding this saved mission.
        for _ in range(3):
            p += 1 + 6 * raw[p]
        if craft == 16:
            assert manifest is None
            manifest = record
    assert p == end and manifest and manifest["generation"] == 2 and manifest["services"] == 1 and manifest["service_ticks"] == 0
    emitted = 0 if stage in ("assigned", "partial") else quantity
    phase = {"assigned": 1, "partial": 2, "loaded": 3, "recalled": 3, "region": 4, "pending": 4, "recovered": 4, "own": 4}[stage]
    assert field("mission.phase") == phase and field("mission.work_ticks") == (37 if stage == "partial" else 0) and field("extracted_units") == emitted
    resource = flow["resources"]["NORE"]
    holding = "descent" if stage == "pending" else "surface_stock" if stage in ("recovered", "own") else "space_hold"
    # Original finite debit must have exactly one actual destination at every reached phase; untouched classes and all other bodies keep their separate equations.
    for domain in ("space_hold", "descent", "surface_stock", "surface_vehicle"):
        assert resource[domain] == (emitted if domain == holding else 0), (stage, domain, resource)
    assert flow["bodies"][0]["initial"] == 1760 and flow["bodies"][0]["remaining"] == 1760 - emitted
    assert all(body["initial"] == body["remaining"] for body_id, body in flow["bodies"].items() if body_id != 0)
    recovered = stage in ("recovered", "own")
    assert manifest["quota"] == (1 if recovered else 2) and manifest["pending"] == (1 if stage == "pending" else 0)
    assert len(manifest["pods"]) == (1 if stage == "pending" or recovered else 2)
    _, start, _, end = riff_location(raw, b"SPDJ")
    jobs = integer(start + 8, 2)
    assert jobs == (1 if stage == "pending" else 0)
    if jobs:
        p = start + 10
        assert integer(p + 36, 4) == 16 and integer(p + 40, 8) == 2 and integer(p + 49, 2) == quantity and integer(p + 56, 2) == 15
        assert raw[p + 58] == 16 and integer(p + 59, 2) == quantity * 16 and integer(p + 51, 2) == 148
        assert p + 62 + 6 * raw[p + 61] == end
    return dict(flow=flow, phase=phase, work=field("mission.work_ticks"), extracted=emitted, manifest=manifest, jobs=jobs)


def process(proof, configuration, references, artifacts, source=None, focused=False):
    """Verify exact native process membership and current executable identity.

    @param proof Semantic process proof carrying its original terminal/native report identities.
    @param configuration Matching current executable configuration.
    @param references Accumulated direct immutable identities, excluding dated producer implementation graphs.
    @param artifacts Exact checkpoint/capture identities produced by this child.
    @param source Original paid input, absent for focused isolated components.
    @param focused Whether the native command is the focused component tag expression.
    @return Original native report identity, for independent process uniqueness checks.
    """
    assert len(proof["native_reports"]) == 1
    native = LEAF["read"](ROOT / proof["native_reports"][0]["path"])
    AUDIT["inspect"](native, references)
    assert native["exit_status"] == 0 and native["executable"] == identity(ROOT / "build" / configuration / "openttd_test.exe")
    assert native["argv"][1:] == [LEAF["TAGS"] if focused else "[.probe-replay-native]"]
    assert all(artifact in native["artifacts"] for artifact in artifacts)
    if source:
        assert source in native["inputs"]
    terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
    return proof["native_reports"][0]


def configuration(name, selectors, socket_selector, references):
    """Qualify one build's view/queue/Recall twins and real connected peers.

    @param name Current native executable configuration.
    @param selectors Exact seven ordered focused/local/queued/Recall selectors.
    @param socket_selector Exact two-peer native selector.
    @param references Direct immutable identity accumulator.
    @return Independent native evidence, conservation and owner comparison totals.
    """
    rows, proofs, decoded, checkpoints, processes = {}, [], {}, {}, []
    # Explicit mode/view membership prevents one successful visible local mission from substituting for hidden, queued or original partial Recall work.
    for (mode, view), selector in zip(ORDER, selectors, strict=True):
        selected = LEAF["read"](selector)
        AUDIT["inspect"](selected, references)
        proof = LEAF["read"](ROOT / selected["report"]["path"])
        AUDIT["inspect"](proof, references)
        assert selected["complete"] and proof["complete"] and selected["configuration"] == proof["configuration"] == name
        assert selected["mode"] == proof["mode"] == mode and selected["view"] == proof["view"] == view and not proof["added_stock"]
        stages = () if mode == "focused" else LEAF["RECALL"] if mode == "recall-queue" else LEAF["STAGES"]
        assert tuple(proof["checkpoints"]) == tuple(proof["witnesses"]) == stages
        processes.append(process(proof, name, references, [*proof["checkpoints"].values(), *proof["captures"]], proof["source"], mode == "focused"))
        if stages:
            assert (proof["dated_index"], proof["source"]) == LEAF["predecessor"](name, mode == "recall-queue")
            assert len(proof["captures"]) == len(stages) - 1
            LEAF["compare"](proof["checkpoints"]["recovered"], proof["checkpoints"]["own"])
        key = mode + "/" + view
        rows[(mode, view)] = proof
        proofs.append(selected["report"])
        # Every original reached phase independently proves its physical equation and paid incarnation before cross-mode equality is considered.
        for stage, saved in proof["checkpoints"].items():
            checkpoints[key + "/" + stage] = saved
            decoded[key + "/" + stage] = snapshot(saved, stage, 2 if mode == "recall-queue" else 20)
    comparisons = 6 * len(LEAF["OWNERS"])
    # All four native full missions converge with the local visible baseline. Recalled partial returns compare their two actual viewport policies separately.
    for mode, view in ORDER[2:5]:
        for stage in LEAF["STAGES"]:
            LEAF["compare"](rows[("local", "visible")]["checkpoints"][stage], rows[(mode, view)]["checkpoints"][stage])
            comparisons += len(LEAF["OWNERS"])
    for stage in LEAF["RECALL"]:
        LEAF["compare"](rows[("recall-queue", "visible")]["checkpoints"][stage], rows[("recall-queue", "hidden")]["checkpoints"][stage])
        comparisons += len(LEAF["OWNERS"])
    selected = LEAF["read"](socket_selector)
    AUDIT["inspect"](selected, references)
    socket_proof = LEAF["read"](ROOT / selected["report"]["path"])
    AUDIT["inspect"](socket_proof, references)
    assert socket_proof["complete"] and socket_proof["configuration"] == selected["configuration"] == name
    assert socket_proof["actual_socket_multiplayer"] and socket_proof["original_client_commands"] and not socket_proof["added_stock"]
    assert (socket_proof["dated_index"], socket_proof["source"]) == LEAF["predecessor"](name)
    for role, peer in socket_proof["peers"].items():
        processes.append(process(peer, name, references, list(peer["checkpoints"].values()), socket_proof["source"]))
        assert tuple(peer["checkpoints"]) == LEAF["STAGES"][:-1]
        for stage, saved in peer["checkpoints"].items():
            decoded["socket/" + role + "/" + stage] = snapshot(saved, stage, 20)
    assert socket_proof["peers"]["server"]["witnesses"] == socket_proof["peers"]["client"]["witnesses"]
    for stage in LEAF["STAGES"][:-1]:
        LEAF["compare"](socket_proof["peers"]["server"]["checkpoints"][stage], socket_proof["peers"]["client"]["checkpoints"][stage])
        comparisons += len(LEAF["OWNERS"])
    assert len(processes) == len({p["path"] for p in processes}) == 9
    return dict(configuration=name, selectors=[identity(p) for p in selectors], socket_selector=identity(socket_selector), reports=proofs + [selected["report"]],
                processes=9, focused_assertions=rows[("focused", "hidden")]["assertions"], focused_cases=rows[("focused", "hidden")]["cases"],
                native_assertions=sum(row["assertions"] for key, row in rows.items() if key[0] != "focused") + sum(peer["assertions"] for peer in socket_proof["peers"].values()),
                checkpoints=checkpoints, socket_checkpoints={role: peer["checkpoints"] for role, peer in socket_proof["peers"].items()}, decoded_checkpoints=decoded,
                owner_family_comparisons=comparisons, metadata_comparisons=comparisons // len(LEAF["OWNERS"]) * len(LEAF["METADATA"]))


def review_examples(release, references):
    """Bind the two reviewed native viewport examples to their original pixel-preserving conversion records.

    @param release Exact accepted Release proof, with explicit local visible/hidden report membership.
    @param references Accumulated direct immutable evidence identities.
    @return Native presentation examples; these preview sprites do not claim final asset acceptance.
    """
    from PIL import Image
    examples = []
    # Only the two explicitly reviewed partial-work views supply these examples; no filename recency selection or replacement rendering can qualify them.
    for view, member in zip(("visible", "hidden"), release["reports"][1:3], strict=True):
        proof = LEAF["read"](ROOT / member["path"])
        assert proof["mode"] == "local" and proof["view"] == view
        original, = [capture for capture in proof["captures"] if capture["path"].endswith("/replay-partial.ppm")]
        image = (ROOT / original["path"]).with_suffix(".png")
        conversion = (ROOT / member["path"]).parent / "capture-conversions.jsonl"
        record, = [json.loads(line) for line in conversion.read_text(encoding="utf-8").splitlines()]
        assert record["source_container"] == original and record["pixels_unchanged"] and record["native_reports"] == proof["native_reports"]
        assert {key: record[key] for key in ("path", "sha256")} == identity(image)
        with Image.open(ROOT / original["path"]) as source, Image.open(image) as encoded:
            assert source.size == encoded.size and source.tobytes() == encoded.tobytes()
        example = dict(view=view, original=original, png=identity(image), conversion=identity(conversion), final_art=False)
        AUDIT["inspect"](example, references)
        examples.append(example)
    return examples


def main():
    """Freeze evidence only after both current builds pass all actual view, command and connected peer boundaries."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=7, required=True)
    parser.add_argument("--debug", type=Path, nargs=7, required=True)
    parser.add_argument("--release-sockets", type=Path, required=True)
    parser.add_argument("--debug-sockets", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    references = {}
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], args.release_sockets.resolve(), references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], args.debug_sockets.resolve(), references)
    # Network latency may place original client commands on different frames in separate sessions. Compare exact owners between peers of each session; deterministic local/view/queued twins also compare exactly between builds.
    for key, saved in release["checkpoints"].items():
        LEAF["compare"](saved, debug["checkpoints"][key])
        assert release["decoded_checkpoints"][key] == debug["decoded_checkpoints"][key]
    examples = review_examples(release, references)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug],
                                         sources=[identity(ROOT / source) for source in SOURCES], direct_references=list(references.values()),
                                         reviewed_viewport_examples=examples,
                                         cross_configuration_owner_family_comparisons=len(release["checkpoints"]) * len(LEAF["OWNERS"]),
                                         cross_configuration_metadata_comparisons=len(release["checkpoints"]) * len(LEAF["METADATA"]),
                                         actual_socket_multiplayer=True, legacy_wire_unchanged=True, legacy_multiplayer_refused=True,
                                         generation_checked_missions=True, added_stock=False, inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
