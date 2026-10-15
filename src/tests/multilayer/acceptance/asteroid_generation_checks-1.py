"""Verify native new-world generation and ordinary frozen-input reloads in a bounded group.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
The original ASTR header/body offsets remain unchanged; schema424 appends26
bytes of historical input rather than recomputing it from present terrain.
"""
import argparse
import json
import re
import struct
import subprocess

from space_preview_checks import ROOT, chunks, identity
from asteroid_population_checks import configure, launch, saved_settings
from runner import current_run_context, run_native


def field_record(path):
    """Decode the actual424 field and its exact fixed-size historical input.

    @param path Actual native save to inspect; no generated content is re-authored.
    @return Dimensions, bodies, seed/time and frozen initial input from the sole ASTR owner.
    """
    version, data = chunks(path)
    assert version == 424, version
    payload = data[b"ASTR"][3:]
    generation, width, height, seed, ticks, density, bodies, surveys = struct.unpack_from(">HIIQQBII", payload)
    assert len(payload) == 51 + bodies * 40 + surveys * 34 + 26
    rows = []
    # Read saved body authority in stable ID order; resource amounts and current positions come from real generated records.
    for body in range(bodies):
        offset = 51 + body * 40
        identifier, resource, size, x, y = struct.unpack_from(">IBBii", payload, offset)
        initial, remaining = struct.unpack_from(">II", payload, offset + 24)
        assert identifier == body and resource < 3 and size < 3
        rows.append({"id": identifier, "class": resource, "size": size, "x": x, "y": y, "initial": initial, "remaining": remaining})
    kind, site_x, site_y, rotation, preferred_x, preferred_y, distance_squared = struct.unpack(">BIIBiiQ", payload[-26:])
    return {"generation": generation, "width": width, "height": height, "seed": seed, "ticks": ticks,
            "density": density, "bodies": rows, "surveys": surveys, "input": {"kind": kind, "site_x": site_x,
            "site_y": site_y, "rotation": rotation, "preference": [preferred_x, preferred_y], "distance_squared": distance_squared}}


def verify(configuration):
    """Run focused producers, then real Industry/Classic creation and unchanged ordinary reloads.

    @param configuration Completed RelWithDebInfo or Debug build to execute.
    @return Accepted report path; independent motion/legacy/corruption runs consume this exact report in file2.
    """
    context = current_run_context("space", "asteroid-generation-verify")
    focused = context.for_feature("space", "asteroid-generation-focused")
    log = focused.logs / "asteroid-generation-focused.log"
    with log.open("wb") as sink:
        run_native(focused, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                             "[asteroid-generation],[space-preview],[asteroid-catalogue]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", log.read_text(encoding="utf-8", errors="replace"))
    assert passed, log
    results, owners = [], [focused]
    # Every population column must retain its exact saved inputs; Classic must remain empty despite local population settings.
    for profile, density, name, expected_count in (
            ("multilayer_industry", 0, "sparse", 6), ("multilayer_industry", 1, "normal", 10),
            ("multilayer_industry", 2, "dense", 14), ("classic", 1, "normal", 0)):
        scenario = profile.replace("_", "-") + "-" + name
        created = context.for_feature("space", "asteroid-generation-new-" + scenario)
        configure(created, profile, name)
        save = launch(created, configuration)
        original = identity(save)
        field = field_record(save)
        assert len(field["bodies"]) == expected_count
        if expected_count:
            assert field["input"]["kind"] in (2, 3)
            assert field["density"] == density
            assert [body["class"] for body in field["bodies"][:3]] == [0, 1, 2]
        else:
            assert field["generation"] == 0 and field["input"]["kind"] == 0
        counts, reserve = [0, 0, 0], [0, 0, 0]
        # Independent cargo-class ledgers expose lost/duplicated classes and preserve finite generated quantities.
        for body in field["bodies"]:
            counts[body["class"]] += 1
            reserve[body["class"]] += body["initial"]
            assert body["initial"] == body["remaining"]
        reloaded = context.for_feature("space", "asteroid-generation-reload-" + scenario)
        configure(reloaded, profile, "dense" if density != 2 else "sparse")
        after = launch(reloaded, configuration, save)
        assert chunks(after)[1][b"ASTR"] == chunks(save)[1][b"ASTR"]
        assert saved_settings(chunks(after)[1][b"PATS"])["game_creation.asteroid_density"] == density
        assert identity(save) == original
        results.append({"profile": profile, "density": density, "source": original, "reload": identity(after),
                        "class_counts": counts, "reserves": reserve, "field": field})
        owners += [created, reloaded]
    report = context.reports / "asteroid-generation.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs10 Group2; real new-game input generation and unchanged saved-field reload.",
        "configuration": configuration, "focused_assertions": int(passed.group(1)), "focused_cases": int(passed.group(2)),
        "focused_log": identity(log), "cases": results,
        "native_reports": [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]}, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one built configuration and print its accepted bounded generation report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    print(verify(args.configuration).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
