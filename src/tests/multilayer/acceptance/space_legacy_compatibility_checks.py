#!/usr/bin/env python3
"""Verify genuine old demos and Classic through isolated ordinary applications.

@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31F.
Each native child has the established110-second deadline. Logs live in
logs/space-fixes; saves, private assets and configuration in test-cache/space-fixes.
No source world, clock, packet, resource or paid owner is edited by this tool.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from asteroid_cargo_roundtrip_checks import compare as compare_packets
from asteroid_field_wire_checks import compare as compare_field, field_parts
from space_mining_wire_checks import compare as compare_orders, native_missions
from space_preview_checks import chunks as framed_chunks
from savegame_tables import chunks, read_stream, saved_owner_table

ROOT = Path(__file__).resolve().parents[4]
SOURCES = (
    ("405", "src/tests/fixtures/demos/space/space-mining-working-405.sav"),
    ("418", "src/tests/fixtures/demos/space/space-module-work-418.sav"),
    ("440", "src/tests/fixtures/demos/space/space-heavy-440.sav"),
    ("classic", "src/tests/fixtures/geology/r2mlvs2-z-classic-current.sav"),
)


def invoke(tool, source, configuration, run, extra):
    """Run one current application through its existing private-runtime harness.

    @param tool Existing ordinary or native observer driver filename.
    @param source Immutable repository-owned actual save.
    @param configuration Current native build configuration.
    @param run Unique artifact identity shared with the child's own native log.
    @param extra Explicit narrow mode arguments.
    @return Completed native artifact directory.
    """
    command = [sys.executable, "-B", str(Path(__file__).with_name(tool)), "--source", source,
               "--configuration", configuration, "--run", run, *extra]
    with (ROOT / "logs/space-fixes" / (run + "-driver.log")).open("w", encoding="utf-8") as log:
        subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    return ROOT / "test-cache/space-fixes" / run


def field_snapshot(path):
    """Decode actual current field records without manufacturing an expected live owner.

    @param path Ordinary native current save.
    @return Complete header, stable-ID body tuples and original renewal trailer.
    """
    version, stream = read_stream(path)
    payload = next(owner["payload"] for owner in chunks(stream) if owner["name"] == b"ASTR")
    header, rows, _, _, renewal = field_parts(payload, version)
    bodies = {}
    # Decode only saved stable identity rows; this table has no authority to
    # allocate a live resource or regenerate a missing field.
    for start in range(0, len(rows), 40):
        row = struct.unpack(">IBBiiiiBBIIII", rows[start:start + 40])
        bodies[row[0]] = row
    return struct.unpack(">HIIQQBIIiiii", header), bodies, renewal


def compare_continuation(paused, advanced, classic):
    """Check actual ordinary elapsed travel while retaining legitimate resource work.

    @param paused Current no-op ordinary migration save.
    @param advanced Separate ordinary continuation of the same genuine old source.
    @param classic Whether this original world intentionally has no Space field.
    @return Actual completed field opportunities and original surviving body counts.
    """
    before, bodies, renewal = field_snapshot(paused)
    after, moved, final_renewal = field_snapshot(advanced)
    # Even an absent field leaves ordinary calendar/gameplay ticking. Compare
    # its original saved DATE authority rather than inferring time from frames.
    assert framed_chunks(paused)[1][b"DATE"] != framed_chunks(advanced)[1][b"DATE"]
    if classic:
        assert before == after and bodies == moved == {} and renewal == final_renewal
        assert before[:5] == (0, 0, 0, 0, 0) and before[6:8] == (0, 0)
        assert renewal[:46] == bytes(46), "Classic acquired a lifecycle/credit/timer owner"
        assert native_missions(paused)[1] == native_missions(advanced)[1] == {}
        return {"field_absent": True, "renewal_uninitialized": True, "ordinary_date_advanced": True}
    elapsed = after[4] - before[4]
    assert 0 < elapsed <= 3000
    surviving = 0
    # Stable IDs join the actual original resources across sparse retirement;
    # surviving motion follows the original signed component and fractional carry.
    for identity, old in bodies.items():
        if identity not in moved:
            continue
        current = moved[identity]
        assert current[1:3] == old[1:3] and current[5:7] == old[5:7]
        assert current[9] == old[9] and current[10] <= old[10]
        for axis in range(2):
            numerator = abs(old[5 + axis]) * elapsed + old[7 + axis]
            distance, remainder = divmod(numerator, 74)
            expected = old[3 + axis] + (-distance if old[5 + axis] < 0 else distance)
            assert current[3 + axis] == expected and current[7 + axis] == remainder
        surviving += 1
    assert surviving > 0
    return {"completed_field_opportunities": elapsed, "surviving_original_bodies": surviving,
            "finite_stock_not_refilled": True, "straight_original_headings": True}


def audit_pair(source, migrated, continued, classic):
    """Audit completed ordinary saves, with a narrow check for the older empty Classic pool.

    @param source Immutable original source path.
    @param migrated Actual paused ordinary migration save.
    @param continued Actual separately elapsed ordinary save.
    @param classic True only for the named genuine version374 no-field fixture.
    @return Exact packet/mission/field migration and elapsed-continuation evidence.
    """
    if classic:
        version, stream = read_stream(source)
        capa = next(owner for owner in chunks(stream) if owner["name"] == b"CAPA")
        # The generic complete-packet comparator starts at375. This actual374
        # fixture has only its CAPA descriptor, so no packet row may appear on load.
        assert version == 374 and capa["kind"] == 3 and len(capa["rows"]) == 1
        assert saved_owner_table(migrated, b"CAPA") == {}
        packets = {"before_version": version, "packets": 0, "empty_original_pool_preserved": True}
    else:
        packets = compare_packets(source, migrated)
    result = {"source": source.relative_to(ROOT).as_posix(), "packets": packets,
              "orders": compare_orders(source, migrated),
              "continuation": compare_continuation(migrated, continued, classic)}
    if not classic:
        result["field"] = compare_field(source, migrated)
    else:
        # This genuine Classic fixture already saves all four physical planes.
        # Its inactive gameplay profile must preserve that geometry exactly;
        # absence of asteroids does not imply a one-plane saved map.
        original_map = next(owner for owner in chunks(read_stream(source)[1]) if owner["name"] == b"MAPS")["rows"][1]
        for saved in (migrated, continued):
            actual_map = next(owner for owner in chunks(read_stream(saved)[1]) if owner["name"] == b"MAPS")["rows"][1]
            assert actual_map == original_map
        assert len(original_map) == 9
        result["original_map_layers"] = struct.unpack(">IIB", original_map)[2]
    return result


def main():
    """Run one configuration's affected old sources with no full regression.

    @return Zero after ordinary migration/continuation and focused native preview/input.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--run", required=True)
    parser.add_argument("--case", choices=("all", "405", "418", "440", "classic"), default="all")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z0-9-]+", args.run):
        raise ValueError("use a unique lowercase artifact identity")
    report = {"_documentation": "r2sff1-space-fixes-features-1.md31F; genuine ordinary compatibility", "sources": []}
    # Each genuine source gets distinct paused and elapsed applications so
    # ordinary gameplay cannot be mistaken for no-op transport corruption.
    for label, source in SOURCES:
        if args.case not in ("all", label):
            continue
        loaded = invoke("space_ordinary_checks.py", source, args.configuration, args.run + "-" + label + "-load", ["--mode", "load"])
        advanced = invoke("space_ordinary_checks.py", source, args.configuration, args.run + "-" + label + "-advance", ["--mode", "advance"])
        migrated, continued = loaded / "ordinary-current.sav", advanced / "ordinary-current.sav"
        result = audit_pair(ROOT / source, migrated, continued, label == "classic")
        if label == "classic":
            # Actual native preview and shortcut dispatch must retain the
            # original allocated layers; both paused and advanced worlds are tested.
            for phase, saved in (("load", migrated), ("advance", continued)):
                invoke("space_fixes_checks.py", saved.relative_to(ROOT).as_posix(), args.configuration,
                       args.run + "-classic-" + phase + "-input",
                       ["--asteroid-preview", "--shortcuts-only", "--expect-layers", str(result["original_map_layers"])])
            result["native_preview_input_original_layers"] = True
        report["sources"].append(result)
        print("COMPATIBILITY", label, "ordinary_load_advance=true", flush=True)
        (ROOT / "logs/space-fixes" / (args.run + "-compatibility.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
