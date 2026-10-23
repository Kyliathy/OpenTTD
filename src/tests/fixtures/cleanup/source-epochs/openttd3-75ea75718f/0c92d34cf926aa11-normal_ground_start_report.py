"""Seal genuine ordinary starting worlds without claiming a completed bootstrap loop.

@file normal_ground_start_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Pure writes retain all126chunks;distinct new games have one explicitly checked native DATE.id.
"""

import argparse
import hashlib
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_report import verify_reference
from construction_provider_return import identity
from save_tables import chunks, read_stream, saved_table


def native_world(path):
    """Read every complete native starting chunk without loading or modifying the world."""
    version, stream = read_stream(path)
    assert version == 434
    result = {row["name"].decode("ascii"): row["raw"] for row in chunks(stream)}
    assert len(result) == 126
    return result


def accept(report_paths, output):
    """Verify actual generated company/profile/terrain/content and exact untouched starts.

    @param report_paths One actual ordinary generation report from each native build.
    @param output New immutable diagnostic handoff beneath repository logs.
    @return Starting-world foundation;paid readiness and full bootstrap stay pending.
    """
    records, worlds, dates, game_ids = {}, {}, {}, {}
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        assert report["classification"] == "normal_start_foundation"
        assert report["bootstrap_claim"] is False and report["shortcuts"] == []
        assert report["tag"] == "[.normal-ground-newgame]" and report["children_reaped"] is True
        assert report["native_version"] == 434
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        for reference in [report["controller"], report["binary"], report["config"], report["startup"], report["content"], *report["sources"]]:
            verify_reference(reference)
        text = verify_reference(report["native"]["log"]).read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == report["native"]["assertions"] > 0 and int(counts[2]) == report["native"]["cases"] == 1
        assert "MLVS_PROFILE id=1 version=1" in text
        assert "MLVS_GEOLOGY generator=1" in text
        saves = {key: verify_reference(value) for key, value in report["saves"].items()}
        worlds[cfg] = native_world(saves["normal-start.sav"])
        assert worlds[cfg] == native_world(saves["normal-start-repeat.sav"])
        owners = report["starting_owners"]
        assert (owners["seed"], owners["width"], owners["height"], owners["year"]) == (420, 128, 128, 1980)
        assert owners["cash"] == owners["loan"] == 100000 and owners["max_loan"] == 300000
        assert owners["opened"] == owners["fleet"] == 0
        assert owners["solid"] == 2 * 126 * 126
        companies = saved_table(saves["normal-start.sav"], b"PLYR")
        assert set(companies) == {0}
        assert companies[0]["money"] == companies[0]["current_loan"] == owners["cash"]
        assert companies[0]["is_ai"] == 0
        assert all(value == 0 for value in companies[0]["yearly_expenses"])
        assert saved_table(saves["normal-start.sav"], b"DATE")[0]["tick_counter"] == owners["tick"] == 1280
        dates[cfg] = saved_table(saves["normal-start.sav"], b"DATE")[0]
        game_ids[cfg] = bytes.fromhex(dates[cfg]["id"]).decode("ascii")
        assert re.fullmatch(r"[0-9A-F]{32}", game_ids[cfg])
        registry = saved_table(saves["normal-start.sav"], b"NGRF")
        assert len(registry) == len(report["activated_content"]) == 1
        assert report["activated_content"][0]["registry"] == registry[0]
        for content in report["activated_content"]:
            package = verify_reference(content["file"])
            assert list(hashlib.md5(package.read_bytes()).digest()) == content["registry"]["ident.md5sum"]
        records[cfg] = {"report": identity(path), "actual": report,
                        "starting_input": report["saves"]["normal-start.sav"]}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    changed = [name for name in worlds["Debug"] if worlds["Debug"][name] != worlds["RelWithDebInfo"][name]]
    # SwitchToMode(NewGame) calls GenerateSavegameId. Native GenerateUid
    # deliberately uses OS randomness/steady time independently of game RNG.
    # Check that one exact field and every remaining DATE byte,not an ignored
    # chunk or a blanket clock/initialization exception. Within one original
    # world's later lineage this identity must remain exact,like pure writes.
    assert changed == ["DATE"], f"Unexpected ordinary generated owner differences: {changed}"
    assert game_ids["Debug"] != game_ids["RelWithDebInfo"]
    assert dates["Debug"].keys() == dates["RelWithDebInfo"].keys()
    assert {key: value for key, value in dates["Debug"].items() if key != "id"} == {
        key: value for key, value in dates["RelWithDebInfo"].items() if key != "id"}
    before_id, after_id = game_ids["Debug"].encode(), game_ids["RelWithDebInfo"].encode()
    assert worlds["Debug"]["DATE"].count(before_id) == worlds["RelWithDebInfo"]["DATE"].count(after_id) == 1
    assert worlds["Debug"]["DATE"].replace(before_id, after_id, 1) == worlds["RelWithDebInfo"]["DATE"]
    assert records["Debug"]["actual"]["starting_owners"] == records["RelWithDebInfo"]["actual"]["starting_owners"]
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6B;actual ordinary generated normal player/profile/package/loan and untouched rock;complete native equality across builds,not paid readiness or bootstrap acceptance",
              "controller": identity(__file__), "records": records, "classification": "normal_start_foundation",
              "bootstrap_claim": False, "native_cases": 2,
              "native_assertions": sum(row["actual"]["native"]["assertions"] for row in records.values()),
              "pure_pairs": 2, "complete_cross_build_pairs": 1, "complete_chunks_equal": 125, "ignored_chunks": [],
              "native_newgame_ids": game_ids, "other_date_fields_equal": len(dates["Debug"]) - 1,
              "native_identity_owner_sources": [identity(ROOT / name) for name in ("src/misc.cpp", "src/openttd.cpp", "src/saveload/misc_sl.cpp")],
              "remaining": "6C actual original-world financing/paid construction/hauler/readiness/access;6D complete original S1/S2 conservation/control/save evidence;original6.1/master11.4 stay unchecked."}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Publish only exact native starting-world foundation evidence."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
