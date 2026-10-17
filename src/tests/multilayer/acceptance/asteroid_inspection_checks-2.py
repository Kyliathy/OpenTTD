"""Qualify read-only native inspector captures, company-isolated survey amounts and exact original saved owners.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14
Actual widget rendering and lossless capture provenance qualify functional presentation; final asset delivery remains Group14B.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy

from PIL import Image
from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "asteroid_inspection_checks-1.py"))
BASE = runpy.run_path(str(HERE / "probe_exit_checks-2.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*BASE["SOURCES"], "src/asteroid_inspection.h", "src/asteroid_inspection.cpp", "src/asteroid_gui.cpp", "src/lang/english.txt",
                             "src/tests/asteroid_inspection.cpp", "src/tests/asteroid_inspection_native.cpp", "src/tests/space_survey.cpp", "src/tests/native_art_render_fixture.h",
                             "src/tests/multilayer/support/runner.py", "media/multilayer/contracts/multilayer_sprite_manifest.json",
                             *(f"src/tests/multilayer/acceptance/asteroid_inspection_checks-{n}.py" for n in (1, 2)))))


def knowledge(saved):
    """Decode the original sparse company authorization without opening native pools or copying reserve authority.

    @param saved Exact source/current ordinary save identity.
    @return Original source schema and body-to-company bit masks; pre-knowledge legacy saves authorize no company.
    """
    assert identity(ROOT / saved["path"]) == saved; version, raw = read_stream(ROOT / saved["path"])
    if version < 413:
        return version, {}
    _, start, _, end = riff_location(raw, b"SPKN"); count = int.from_bytes(raw[start:start + 2], "big"); assert start + 2 + count * 12 == end
    rows = {int.from_bytes(raw[p:p + 4], "big"): int.from_bytes(raw[p + 4:p + 12], "big") for p in range(start + 2, end, 12)}
    assert len(rows) == count and all(mask > 0 for mask in rows.values())
    return version, rows


def configuration(name, selectors, references):
    """Qualify native/ordinary presentation ancestry and lossless current widget captures for one executable.

    @param name Current native executable configuration.
    @param selectors Exact native-a/native-b/own/focused selectors in order.
    @param references Direct checked evidence; historical indexes/processes remain immutable dated boundaries.
    @return Actual counts, current checkpoints and hash-bound native pixels.
    """
    rows = {}; proofs = []; focused = None; comparisons = 0
    # Each displayed state starts from its exact original paid/scan producer or its own unchanged ordinary result, never another scenario's guessed resource or knowledge.
    for scope, path in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](path); AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["scope"] == scope and len(selected["reports"]) == len(LEAF["SCOPES"][scope])
        for phase, relative in zip(LEAF["SCOPES"][scope], selected["reports"]):
            path = ROOT / relative; proof = LEAF["read"](path); AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["scope"] == scope and proof["phase"] == phase and not proof["added_stock"] and not proof["final_art"]
            assert proof["dated_producers"] == LEAF["predecessor"](name, phase)[0] and len(proof["native_reports"]) == 1
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [LEAF["TAGS"] if scope == "focused" else "[.asteroid-inspection-native]"]
            proof["_path"] = identity(path); proofs.append(proof)
            if scope == "focused":
                focused = proof; continue
            category = "own" if scope == "own" else "native"; original_dated, original_source, original_craft = LEAF["predecessor"](name, phase)
            assert proof["original"] == original_craft and proof["source"] in process["inputs"]
            assert proof["before"] in process["artifacts"] and proof["after"] in process["artifacts"]
            AUDIT["compare"](proof["before"], proof["after"]); comparisons += 1
            version, authorized = knowledge(proof["before"]); assert version == knowledge(proof["after"])[0] == 438
            assert authorized == knowledge(proof["after"])[1] == knowledge(proof["source"])[1]
            assert bool(authorized.get(0, 0) & (1 << proof["observer"])) == (phase == "known")
            capture, = proof["captures"]; assert capture["native"] in process["artifacts"] and len(proof["conversions"]) == 1
            # Conversion changes only encoding. Both containers retain the exact native pixels and explicit producer; the original PPM remains reviewable.
            with Image.open(ROOT / capture["native"]["path"]) as native, Image.open(ROOT / capture["png"]["path"]) as encoded:
                assert native.size == encoded.size and native.tobytes() == encoded.tobytes()
                assert list(native.size) == capture["dimensions"] and hashlib.sha256(native.tobytes()).hexdigest() == capture["pixel_sha256"]
                assert native.width >= 760 and native.height >= 480
            conversions = [json.loads(line) for line in (ROOT / proof["conversions"][0]["path"]).read_text(encoding="utf-8").splitlines()]
            converted, = conversions; AUDIT["inspect"](converted, references)
            assert converted["pixels_unchanged"] and converted["source_container"] == capture["native"] and proof["native_reports"][0] in converted["native_reports"]
            assert {key: converted[key] for key in ("path", "sha256")} == capture["png"]
            if category == "own":
                parent = rows["native/" + phase]; assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
                AUDIT["compare"](proof["source"], proof["before"]); comparisons += 1
                assert capture["pixel_sha256"] == parent["captures"][0]["pixel_sha256"]
            else:
                assert proof["parent"] is None and proof["source"] == original_source
            rows[category + "/" + phase] = proof
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 12 and focused and comparisons == 14
    checkpoints = {key + "/" + boundary: proof[boundary] for key, proof in rows.items() for boundary in ("before", "after")}
    return dict(configuration=name, processes=12, captures=11, focused_assertions=focused["assertions"], focused_cases=focused["cases"], native_assertions=sum(p["assertions"] for p in proofs if p is not focused),
                selectors=[identity(path) for path in selectors], reports=[p["_path"] for p in proofs], checkpoints=checkpoints,
                capture_records={key: proof["captures"][0] for key, proof in rows.items()}, physical_owner_comparisons=420, metadata_comparisons=28)


def main():
    """Freeze current functional inspector evidence only after owners, authorization and rendered pixels agree in both builds."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=4, required=True); parser.add_argument("--debug", type=Path, nargs=4, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); references = {}
    release = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references); debug = configuration("Debug", [path.resolve() for path in args.debug], references)
    # Each original and ordinary-resaved view has identical native owners and actual pixels between executable configurations; a clean RNG result alone would not prove cargo/knowledge conservation.
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    for key, capture in release["capture_records"].items():
        assert capture["pixel_sha256"] == debug["capture_records"][key]["pixel_sha256"] and capture["dimensions"] == debug["capture_records"][key]["dimensions"]
    dated = list({ident["path"]: ident for phase in (*LEAF["ENTRIES"], "scan", "construction") for ident in LEAF["predecessor"]("RelWithDebInfo", phase)[0]}.values())
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()),
                                         dated_producer_bindings=dated, cross_configuration_physical_comparisons=660, cross_configuration_metadata_comparisons=44,
                                         cross_configuration_pixel_comparisons=11, company_quantity_isolation=True, native_owner_preservation=True,
                                         final_art=False, deferred_art_group="14B", inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
