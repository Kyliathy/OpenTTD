"""Run focused actual native generator/riser art boundaries without substituting rendered fixtures for paid-world acceptance.

@file geothermal_machinery_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native fixture floor/material boundaries are explicit; actual ordinary paid-world rasters remain separate84BB.3/84C obligations.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native

CASES = {"generators": ("[geothermal-generator-art-construction]", "[geothermal-generator-art-operation]",
                        "[geothermal-art-state]", "[geothermal-generator-geometry]", "[m1-sprite-selection]"),
         "risers": ("[geothermal-riser-art-construction]", "[geothermal-riser-art-flow]", "[geothermal-riser-definition]",
                    "[geothermal-riser-construction]", "[geothermal-riser-link]", "[m1-sprite-selection]"),
         "machinery": ("[geothermal-generator-art-construction]", "[geothermal-generator-art-operation]", "[geothermal-riser-art-construction]",
                       "[geothermal-riser-art-flow]", "[geothermal-art-state]", "[m1-sprite-selection]")}
SOURCES = ("src/geothermal_sprite.h", "src/geothermal_sprite.cpp", "src/geothermal_generator.cpp", "src/multilayer_sprite.cpp", "src/industry_cmd.cpp",
           "src/tests/geothermal_generator_sprite.cpp", "src/tests/geothermal_generator_fixture.h",
           "src/tests/geothermal_generator_operating_fixture.h", "src/tests/multilayer/geothermal_machinery_checks.py")
RISER_SOURCES = ("src/geothermal_riser_sprite.cpp", "src/geothermal_riser_construction.cpp", "src/geothermal_riser.h",
                 "src/tests/geothermal_riser_sprite.cpp", "src/tests/geothermal_riser_fixture.h", "src/tests/geothermal_electrical_fixture.h")


def focused(configuration, part):
    """Verify one bounded family of actual native state/selector/purity boundaries, each in an isolated process.

    @param configuration Current compiled Debug or optimized test executable.
    @param part Exact implemented machinery family; unsupported future families are not selected by prefix.
    @return None; failed native ownership/pose/purity checks leave no passing family report.
    """
    context = current_run_context("graphics", "geothermal-machinery"); proofs = []
    # Isolated fixtures retain original native pool/stock/service ownership;
    # the next case cannot borrow a prior completed machine or source grant.
    for index, case in enumerate(CASES[part]):
        _, proof = native(context, configuration, case, f"{part}-{index}"); proofs.append({"case": case, **proof})
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BB; actual native boundary selectors, not original paid-world raster acceptance",
              "configuration": configuration, "part": part, "native": proofs, "controller": identity(__file__),
              "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES + (RISER_SOURCES if part != "generators" else ())],
              "remaining": "Actual loaded-world pixels, native transparency/occlusion/layer/picking and current four-target acceptance remain84BB.3/84C; runtime metadata promotion remains84B.2/84D."}
    path = context.output("reports", f"geothermal-machinery-{configuration}-{part}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def main():
    """Select exactly one current native machinery verification burst.

    @return None; missing configuration/family or any native refusal stops before a report.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--part", choices=tuple(CASES), required=True); args = parser.parse_args(); focused(args.config, args.part)


if __name__ == "__main__":
    main()
