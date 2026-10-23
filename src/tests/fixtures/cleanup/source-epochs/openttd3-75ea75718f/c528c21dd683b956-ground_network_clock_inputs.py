"""Seal exact paid originals earned under legal native starting clock policies.

@file ground_network_clock_inputs.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group19C.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This source handoff preserves declared fixture lineage;actual networking follows separately.
"""

import argparse
import json
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_clock_source import MODES, TAG
from ground_network_report import verify_reference
from ground_network_trace import world
from construction_provider_return import identity
from save_tables import saved_table


def accept(report_paths, output_path):
    """Verify all independent starting-mode producers and retain their original owners.

    @param report_paths Eight actual source reports:four modes in each native configuration.
    @param output_path Absent immutable input-index JSON beneath repository logs.
    @return Exact accepted sources;no paid world or existing receipt is rewritten.
    """
    records = {cfg: {} for cfg in ("Debug", "RelWithDebInfo")}
    assertions = 0
    # Each mode needs its own legal fresh-world producer in both builds.
    # A modified existing paid world cannot satisfy starting-policy acceptance.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in records
        assert focus in MODES and focus not in records[cfg]
        assert (report["units"], report["rate"]) == MODES[focus]
        assert report["tag"] == TAG
        assert report["native_version"] == 434
        assert report["native"]["cases"] == 1
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        for reference in [report["controller"], report["binary"], report["native"]["log"],
                          report["original"], report["pure_repeat"], *report["sources"]]:
            verify_reference(reference)
        original = ROOT / report["original"]["path"]
        assert world(original) == world(ROOT / report["pure_repeat"]["path"])
        settings = saved_table(original, b"PATS")[0]
        units, rate = MODES[focus]
        assert settings["economy.timekeeping_units"] == units
        assert settings["economy.minutes_per_calendar_year"] == rate
        clock = saved_table(original, b"DATE")[0]
        project = saved_table(original, b"TBMP")[0]
        job = saved_table(original, b"XJOB")[0]
        hauler = saved_table(original, b"TBXA")[0]
        assert report["clock"] == clock
        assert report["project"] == project
        assert report["job"] == job
        assert job["company"] == project["company"] == 0
        assert project["id"] == hauler["project"] == 0
        assert project["job"] == job["id"] == 0
        assert project["blocked"] == 0
        assert 0 < job["cursor"] < job["routes"]
        facts = report["observations"]
        assert facts["earth"] == job["earth"] > 0
        assert facts["rock"] == job["rock"] > 0
        assert facts["work"] == job["cost"] > 0
        assert facts["setup"] > 0
        assert facts["cash"] < 10000000 - facts["setup"]
        assert clock["tick_counter"] == facts["tick"]
        calendar = (clock["date"] - 723180) * 74 + clock["date_fract"]
        assert calendar == (0 if rate == 0 else facts["tick"] * 12 // rate)
        assert clock["economy_date"] * 74 + clock["economy_date_fract"] == facts["tick"]
        records[cfg][focus] = {"report": identity(path), "actual": report,
                               "input": report["original"], "clock": clock["tick_counter"],
                               "project": project, "job": job, "hauler": hauler}
        assertions += report["native"]["assertions"]
    assert all(set(modes) == set(MODES) for modes in records.values())
    # Cross-build byte-complete equality confirms that source preparation and
    # native timer dispatch are deterministic before any joining claim exists.
    for focus in MODES:
        assert world(ROOT / records["Debug"][focus]["input"]["path"]) == world(
            ROOT / records["RelWithDebInfo"][focus]["input"]["path"])
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md19C.1;legal public next-game modes before declared paid owners;actual native timers and earned unfinished cuts;not normal-newgame bootstrap or networking acceptance",
              "controller": identity(__file__), "records": records,
              "native_cases": 8, "native_assertions": assertions,
              "all_chunk_pure_pairs": 8, "all_chunk_cross_build_pairs": 4,
              "remaining": "Actual fresh native peer initial/next/later state,clock ratios and continuing paid work for every selected mode."}
    target = (ROOT / output_path).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "native_assertions": assertions}), flush=True)
    return result


def main():
    """Seal only complete actual source reports without executing native gameplay.

    @return None;missing source,clock,paid-owner or provenance evidence refuses publication.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
