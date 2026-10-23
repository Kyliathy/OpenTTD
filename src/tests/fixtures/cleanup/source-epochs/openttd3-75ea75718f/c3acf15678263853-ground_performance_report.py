"""Verify measured ordinary ground map baselines before declaring active-workload budgets.

@file ground_performance_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21C;master11.6.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Empty ordinary maps remain baseline evidence;dense work and real joins have separate gates.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import PROTOCOL, parse_fields, summarize


def accept(names, output, selection=None, checks=None):
    """Recheck real counts,all retained samples,OS peaks and source identity;freeze explicit later budgets."""
    protocol = json.loads((ROOT / PROTOCOL).read_text(encoding="utf-8"))
    records, budgets, refusals, selection_ref = {}, [], [], None
    if selection is not None:
        selection_ref = identity(ROOT / selection)
        index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
        assert index["complete"] is True and index["protocol"] == identity(ROOT / PROTOCOL)
        verify_reference(index["controller"])
        expected_all = {(cfg, profile, axis) for cfg in protocol["configurations"] for profile in protocol["profiles"]
                        for axis in (protocol["compact_axis"], protocol["representative_axis"], *protocol["large_candidates"])}
        assert len(index["records"]) == len(expected_all)
        assert {(r["configuration"], r["profile"], r["axis"]) for r in index["records"]} == expected_all
        names = list(names)
        for row in index["records"]:
            if row["status"] == "observed":
                names.append(row["report"]["path"])
                continue
            assert row["status"] == "refused"
            actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
            assert actual["status"] == "refused" and actual["children_reaped"] is True and actual["bootstrap_claim"] is False
            assert (actual["configuration"], actual["profile"], actual["axis"]) == (row["configuration"], row["profile"], row["axis"])
            assert actual["protocol"] == identity(ROOT / PROTOCOL)
            for ref in (actual["controller"], actual["binary"], actual["config"], actual["startup"], actual["native"]["log"], *actual["sources"]):
                verify_reference(ref)
            memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
            samples, failure = memory["samples"], actual["failure"]
            assert samples and failure.startswith(("TimeoutError:", "RuntimeError: native workload crossed"))
            if failure.startswith("TimeoutError:"):
                assert actual["wall_seconds"] >= protocol["child_timeout_seconds"]
            else:
                assert any(r["system"]["physical_available"] < protocol["minimum_physical_available_bytes"] or
                           r["system"]["commit_available"] < protocol["minimum_commit_available_bytes"] or
                           r["process"]["private_bytes"] > protocol["maximum_child_private_bytes"] for r in samples)
            refusals.append({"report": row["report"], "configuration": row["configuration"], "profile": row["profile"],
                             "axis": row["axis"], "failure": failure, "children_reaped": True})
    for name in names:
        ref = identity(ROOT / name)
        actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        key = (actual["configuration"], actual["profile"], actual["axis"])
        assert key not in records and actual["status"] == "observed" and actual["failure"] is None
        assert actual["classification"] == "native_ground_performance_observation" and actual["bootstrap_claim"] is False
        assert actual["hardware"]["cpu"] and actual["hardware"]["logical_processors"] > 0 and actual["hardware"]["physical_total_bytes"] > 0
        assert actual["protocol"] == identity(ROOT / PROTOCOL) and actual["mode"] == "generation" and actual["input"] is None
        for reference in (actual["controller"], actual["binary"], actual["config"], actual["startup"], *actual["sources"]):
            verify_reference(reference)
        native = actual["native"]
        assert native["argv"] == [str(verify_reference(actual["binary"])), "[.ground-performance-map]"] and native["cwd"] == "build"
        assert actual["children_reaped"] and native["exit_status"] == 0 and actual["wall_seconds"] < 115
        text = verify_reference(native["log"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == native["assertions"] > 0 and int(counts[2]) == native["cases"] == 1
        assert actual["tick_samples"] == parse_fields(text, "GROUND_PERFORMANCE_TICK")
        assert actual["owners"] == parse_fields(text, "GROUND_PERFORMANCE_COUNTS")
        assert actual["ready"] == parse_fields(text, "GROUND_PERFORMANCE_READY")[0]
        assert actual["synchronous_saves"] == parse_fields(text, "GROUND_PERFORMANCE_SAVE")
        assert actual["commands"] == parse_fields(text, "GROUND_PERFORMANCE_COMMAND")
        assert len(actual["tick_samples"]) == 640 and [r["index"] for r in actual["tick_samples"]] == list(range(640))
        for field in ("simulation_ns", "input_ns", "windows_ns"):
            assert actual["timings"][field] == summarize([r[field] for r in actual["tick_samples"]])
        for row in actual["owners"]:
            assert row["width"] == row["height"] == actual["axis"] and row["map_layers"] == 4
            assert row["map_tiles"] == row["width"] * row["height"] * 4
            assert row["packed_tile_payload_estimate"] == row["map_tiles"] * 12
            assert row["companies"] == 1 and row["fleet_parts"] == row["jobs"] == row["claims"] == row["utility_consumers"] == 0
            if actual["profile"] == "classic":
                assert row["geology_terrain_bytes"] == row["deposits"] == row["vents"] == 0
            else:
                assert row["geology_terrain_bytes"] > 0 and row["deposits"] >= 10 and row["vents"] > 0
        assert actual["owners"][1]["tick"] - actual["owners"][0]["tick"] == 768
        for reference in actual["saves"].values():
            verify_reference(reference)
        for phase in ("initial", "final"):
            assert all(actual["saves"][f"{phase}-world"][field] == actual["saves"][f"{phase}-repeat"][field] for field in ("bytes", "sha256"))
        memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
        samples = memory["samples"]
        assert samples and all(a["seconds"] < b["seconds"] for a, b in zip(samples, samples[1:]))
        assert actual["peak_working_bytes"] == max(r["process"]["lifetime_peak_working_bytes"] for r in samples)
        assert actual["peak_commit_bytes"] == max(r["process"]["lifetime_peak_commit_bytes"] for r in samples)
        assert actual["sampled_peak_private_bytes"] == max(r["process"]["private_bytes"] for r in samples) <= protocol["maximum_child_private_bytes"]
        for field, minimum in (("physical_available", "minimum_physical_available_bytes"), ("commit_available", "minimum_commit_available_bytes")):
            assert actual[minimum] == min(r["system"][field] for r in samples) >= protocol[minimum]
        records[key] = {"report": ref, "actual": actual}
    expected = {(cfg, profile, axis) for cfg in protocol["configurations"] for profile in protocol["profiles"] for axis in (128, 512, 1024)}
    assert expected.issubset(records)
    contents, content_signature = [], None
    for row in records.values():
        runtime = verify_reference(row["actual"]["config"]).parent
        references = {"baseset/openttd.grf": identity(runtime / "baseset/openttd.grf"),
                      "newgrf/multilayer_industry.grf": identity(runtime / "newgrf/multilayer_industry.grf"),
                      "baseset/orig_dos.obg": identity(runtime / "baseset/orig_dos.obg")}
        for name in ("orig_extra.grf", "trg1.grf", "trgc.grf", "trgh.grf", "trgi.grf", "trgt.grf"):
            references["baseset/" + name] = identity(runtime / "baseset" / name)
        signature = {key: {field: ref[field] for field in ("bytes", "sha256")} for key, ref in references.items()}
        if content_signature is None:
            content_signature = signature
        assert signature == content_signature, "Performance profiles/builds used different physical native content"
        contents.append({"report": row["report"], "runtime_content": references})
    for cfg in protocol["configurations"]:
        for axis in sorted({key[2] for key in records if key[0] == cfg}):
            if (cfg, "classic", axis) not in records or (cfg, "multilayer_industry", axis) not in records:
                continue
            baseline = records[(cfg, "classic", axis)]["actual"]
            # Freeze before active stress:one native27ms opportunity of added
            # optimized simulation work;Debug gets an explicit10x diagnostic
            # increment. Both retain the measured disabled process baseline.
            increment = 27000000 * (10 if cfg == "Debug" else 1)
            budgets.append({"configuration": cfg, "axis": axis, "disabled_baseline": records[(cfg, "classic", axis)]["report"],
                            "simulation_p99_ns": baseline["timings"]["simulation_ns"]["p99_ns"] + increment,
                            "simulation_max_ns": baseline["timings"]["simulation_ns"]["maximum_ns"] + increment * 2,
                            "incremental_peak_commit_bytes": 256 * 1024 ** 2,
                            "peak_commit_bytes": min(protocol["maximum_child_private_bytes"], baseline["peak_commit_bytes"] + 256 * 1024 ** 2),
                            "command_p99_ns": increment * 2, "route_p99_ns": increment * 4,
                            "save_ns": 15000000000 * (2 if cfg == "Debug" else 1),
                            "startup_or_load_ns": 80000000000, "join_reconstruction_ns": 45000000000,
                            "reason": "measured disabled baseline plus one optimized native27ms opportunity;Debug10x diagnostic margin;256MiB extra commit;bounded interactive/file/join deadlines"})
    maxima = [{"configuration": cfg, "profile": profile, "axis": max(key[2] for key in records if key[:2] == (cfg, profile))}
              for cfg in protocol["configurations"] for profile in protocol["profiles"]]
    assert checks is not None
    checks_ref = identity(ROOT / checks)
    data = verify_reference(checks_ref).read_bytes()
    text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    assert re.search(r"Ran 5 tests in [\d.]+s\s+OK\s*$", text)
    hardware = [row["actual"]["hardware"] for row in records.values()]
    assert all(row == hardware[0] for row in hardware)
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21C;ordinary native map baseline and predeclared later ground budgets",
              "classification": "ground_performance_baselines", "bootstrap_claim": False, "controller": identity(__file__),
              "protocol": identity(ROOT / PROTOCOL), "records": list(records.values()), "budgets": budgets,
              "runtime_content": contents,
              "selection": selection_ref, "candidate_refusals": refusals, "largest_completed_under_policy": maxima,
              "hardware": hardware[0], "focused_checks": {"script": identity(ROOT / "src/tests/multilayer/ground_performance_checks.py"), "log": checks_ref, "tests": 5},
              "native_cases": len(records), "native_assertions": sum(v["actual"]["native"]["assertions"] for v in records.values()),
              "measured_ticks": len(records) * 640, "remaining": "dense active/route/command/utility21D;actual file/join21E;attribution21F;larger candidate admission separately retained",
              "uncertainty": "640samples/five sequential intervals per process;OS cache and concurrent desktop activity uncontrolled;timer/Catch/monitor overhead retained;startup includes initialization/content;10ms sampling may miss private transients;OS lifetime working/commit peaks retained;headless excludes raster cost"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": result["native_cases"], "measured_ticks": result["measured_ticks"]}), flush=True)


def main():
    """Seal explicit actual baseline reports without replacing any prior measurement."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", default=[])
    parser.add_argument("--selection")
    parser.add_argument("--checks", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output, args.selection, args.checks)


if __name__ == "__main__":
    main()
