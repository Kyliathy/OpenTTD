"""Verify actual2005 native Forge source prerequisites before constructing any cargo or participant.

@file forge_source_prerequisites.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FBA.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The empty author and normal native loader supply definitions; quantities and useful freight remain later tasks.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native
from geothermal_tube_binding_files import world_chunks
from shield_source_checks import SOURCES as ORIGINAL_SOURCES

SOURCES = (*ORIGINAL_SOURCES, "src/tests/tube_source_template_fixture.h", "src/tests/tube_forge_source.cpp",
           "src/tests/multilayer/forge_source_prerequisites.py", "src/transport_tube_model.h", "src/industrytype.h")
CASES = (("Every native mine produces exact finite output", "finite-producer"),
         ("Native Surface processors execute every frozen material alternative", "processor-producer"),
         ("Baseline receiver reuse validates actual cargo and production semantics", "receiver-boundary"),
         ("Native capsule startup honors all five exact launch dates", "launch-calendar"),
         ("Native resource and spoil transport coverage uses real definitions and synchronized dates", "conventional-freight"))


def main():
    """Run either the genuine empty author/native definition gate or isolated required producer boundaries.

    @return None; wrong dates/content, failed native producers or changed original inputs prevent a prerequisite report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=("native", "producers"), required=True)
    args = parser.parse_args()
    context = current_run_context("tubes", "forge-source")
    context.prepare_runtime()
    report = dict(_documentation="r2mlvs17-underground-transport-tubes.md15FBA; actual2005 native prerequisites, no source quantity or useful freight",
                  configuration=args.config, focus=args.focus, controller=identity(__file__),
                  binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES])
    if args.focus == "native":
        output, author = native(context, args.config, "[tube-forge-source-template]", "empty-world")
        match = re.search(r"FORGE_SEED year=2005 vehicles=0 industries=0 packets=0 useful_source=false path=([^\r\n]+)", output)
        assert match is not None
        source = Path(match[1])
        original = identity(source)
        assert len(world_chunks(source)) == 126
        startup = context.runtime_dir / "scripts/game_start.scr"
        startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FBA; actual original2005 native definition gate.\nforge_source_definitions_check\n", encoding="utf-8")
        output, proof = native(context, args.config, "[.tube-forge-source-definitions-native]", "native-definitions", {"OTTD_FORGE_SOURCE_INPUT": str(source)})
        match = re.search(r"FORGE_NATIVE ((?:\w+=\d+ )+)path=([^\r\n]+)", output)
        assert match is not None
        values = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", match[1])}
        assert len(values) == 16 and values["year"] == 2005 and values["cash"] == 32000000 and values["capacity"] == 80
        assert all(values[name] == 0 for name in ("tick", "loan", "vehicles", "industries", "stations", "packets"))
        deposits = [dict(label=label, **dict(zip(("cargo", "type", "deposit", "reserve", "x", "y"), map(int, row))))
                    for label, *row in re.findall(r"FORGE_RAW label=(CORE|COAL) cargo=(\d+) type=(\d+) deposit=(\d+) reserve=(\d+) x=(\d+) y=(\d+)", output)]
        assert len(deposits) == 2 and {row["label"] for row in deposits} == {"CORE", "COAL"}
        assert all(row["reserve"] > 0 for row in deposits)
        saved = Path(match[2])
        assert len(world_chunks(saved)) == 126 and identity(source) == original
        author.update(output=original)
        proof.update(input=original, output=identity(saved))
        report.update(native=[author, proof], definitions=values, deposits=deposits,
                      fixture_classification="New2005 author date/32m initial funds precede all purchases and work. Ordinary native application loading reconstructs actual cargo/engine/refit/processor/mine definitions and prices, with generated finite CORE/Coal deposits intact. No machinery, vehicle, station, stock, packet, useful delivery, manufactured RFMT, availability grant, loaded clock/account repair or TBM/bootstrap claim.")
    else:
        processes = []
        # Each exact existing producer case gets a separate native process;
        # combined-tag fixture leakage cannot authorize dependent construction.
        for case, label in CASES:
            _, proof = native(context, args.config, case, label)
            processes.append(dict(case=case, **proof))
        report.update(native=processes)
    path = context.output("reports", f"forge-source-prerequisites-{args.config}-{args.focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
