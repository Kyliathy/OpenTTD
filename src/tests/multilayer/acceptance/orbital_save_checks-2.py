"""Round-trip one explicit orbital checkpoint through the current ordinary product.

@file orbital_save_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group14.1
Genuine paid inputs remain immutable. Optional same-recipe copied fixtures test
only current processor-kind transport; they never count as a paid gameplay producer.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import struct
import subprocess

from space_preview_checks import ROOT, chunks, identity, read_stream
from artifact_paths import create_run_context
from runner import console_executable, run_native

PARSER = runpy.run_path(str(Path(__file__).with_name("orbital_save_checks-1.py")))
PHYSICAL = runpy.run_path(str(Path(__file__).with_name("processor_freight_checks-1.py")))["PHYSICAL"]
UPGRADE = runpy.run_path(str(Path(__file__).with_name("orbital_save_checks-3.py")))


def read(path):
    """Read one explicitly selected evidence document.

    @param path Original repository-owned JSON. @return Its exact parsed content.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def ordinary(context, source, configuration):
    """Perform an actual paused ordinary load/save without assuming unchanged legacy framing.

    @param context Invocation-owned native runtime and logs.
    @param source Exact original save identity. @param configuration Actual qualified native product.
    @return None; the caller independently admits every original owner and legacy append.
    """
    output = context.saves / "ordinary-resave.sav"
    commands = f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n'
    (context.runtime_dir / "scripts/game_start.scr").write_text(commands, encoding="utf-8")
    executable = console_executable(context, ROOT / "build" / configuration / "openttd.exe")
    command = [str(executable), "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X",
               "-I", "original_dos", "-c", str(context.config_path), "-g", str(ROOT / source["path"])]
    with (context.logs / "ordinary.log").open("wb") as sink:
        run_native(context, command, cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=90)
    assert output.is_file()


def copied_kind(context, source, kind):
    """Create a declared current-format processor-kind codec fixture from a real inactive job.

    @param context Invocation-owned native runtime/evidence directory.
    @param source Immutable genuine438 Steelworks delivery/frame identity.
    @param kind Goodsworks11, Refinery16 or IceProcessor17, sharing that exact canonical construction recipe.
    @return New explicitly declared fixture identity; its one changed kind byte grants no paid-procurement claim.
    """
    assert kind in (11, 16, 17)
    version, raw = read_stream(ROOT / source["path"])
    assert version == 438
    records, offsets = PARSER["structures"](raw, version)
    selected = PARSER["construction_summary"](records, 10)
    assert selected["module"] is None
    # These original definitions share capital, bill, work and geometry. Admit
    # that published equivalence before changing only an inactive copied kind;
    # no active recipe, packet, work, owner or instruction may be rewritten.
    definitions = (ROOT / "src/multilayer_space_content.cpp").read_text()
    suffix = "UINT16_MAX, processor, 220000, 740, 288, TimerGameCalendar::Year{1995}, 3, 3}"
    for name in ("Steelworks", "Goodsworks", "Refinery", "IceProcessor"):
        assert "{SpaceBuildKind::" + name + ", " + suffix in definitions
    changed = bytearray(raw)
    changed[offsets[selected["site"]]] = kind
    assert sum(a != b for a, b in zip(raw, changed)) == 1
    path = context.saves / "declared-processor-kind-codec-438.sav"
    assert not path.exists()
    path.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    expected = PARSER["structures"](changed, version)[0]
    records["rows"][selected["site"]]["kind"] = kind
    assert records == expected
    return identity(path)


def execute(configuration, source, producer, kind, declared):
    """Load and save one paused original boundary, then compare all original owners.

    @param configuration Existing independently qualified native product configuration.
    @param source Exact original accepted native save identity.
    @param producer Explicit dated source index/report identity, without historical binary replay.
    @param kind Native module kind whose reservation/functionality is reported.
    @param declared Whether to use the explicit current processor-kind codec copy.
    @return Complete ordinary reconstruction receipt, retaining truthful gameplay/fixture scope.
    """
    context = create_run_context("space", "orbital-save-boundary")
    context.prepare_runtime()
    binary = read(ROOT / "logs/orbital-art-ordinary-binary-verification.json")
    executable = identity(ROOT / "build" / configuration / "openttd.exe")
    assert executable == binary["builds"][configuration]["application"]
    assert identity(ROOT / source["path"]) == source
    assert identity(ROOT / producer["path"]) == producer
    original = source
    if declared:
        source = copied_kind(context, source, kind)
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(context, source, configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    after = identity(context.saves / "ordinary-resave.sav")
    old_version, before = chunks(ROOT / source["path"])
    version, saved = chunks(ROOT / after["path"])
    assert version == 438
    old = PARSER["structures"](read_stream(ROOT / source["path"])[1], old_version)[0]
    current = PARSER["structures"](read_stream(ROOT / after["path"])[1], version)[0]
    assert old == current, (source, "semantic SPST", old, current)
    comparison = UPGRADE["compare_upgrade"](ROOT / source["path"], ROOT / after["path"], configuration, PHYSICAL, PARSER["structures"])
    assert identity(ROOT / original["path"]) == original
    assert identity(ROOT / source["path"]) == source
    output = context.reports / "orbital-save-boundary.json"
    result = dict(_documentation=__doc__, complete=True, configuration=configuration, original=original, source=source,
                  producer=producer, after=after, application=executable, old_version=old_version, version=version,
                  declared_codec_fixture=declared, paid_kind_delivery_acceptance=not declared,
                  initial_content_procurement_acceptance=False, **comparison,
                  construction=PARSER["construction_summary"](current, kind), terminal=identity(context.logs / "ordinary.log"),
                  native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                  sources=[identity(Path(__file__)), identity(Path(__file__).with_name("orbital_save_checks-1.py")), identity(Path(__file__).with_name("orbital_save_checks-3.py")),
                           identity(ROOT / "src/saveload/space_structure_sl.cpp"), identity(ROOT / "src/saveload/orbital_recipe_sl.cpp")])
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return identity(output)


def main():
    """Execute one bounded explicit member; all output remains under repository log/review owners."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", required=True, choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--producer", required=True, type=Path)
    parser.add_argument("--kind", required=True, type=int)
    parser.add_argument("--declared-kind", action="store_true")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    source, producer, output = [(ROOT / path).resolve() for path in (args.source, args.producer, args.output)]
    assert source.is_relative_to(ROOT)
    assert producer.is_relative_to(ROOT)
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    result = execute(args.configuration, identity(source), identity(producer), args.kind, args.declared_kind)
    output.write_text(json.dumps(dict(_documentation=__doc__, report=result), indent=2) + "\n", encoding="utf-8")
    print(result, flush=True)


if __name__ == "__main__":
    main()
