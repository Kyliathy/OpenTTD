"""Run bounded native renderer cases against the current focused executables.

@file merge_renderer_selection_checks.py
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Build the Tube, Ground consumer and Space art targets first. This observer runs
component worlds only; no protected continuation save or published art is opened.
Disposable transcripts and the executable/source-bound receipt live in test-cache.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]


def identity(path):
    """Bind a real selected input without treating old reports as current proof.

    @param path Existing executable, source or invocation transcript.
    @return Repository-relative path, content hash and exact length.
    """
    data = path.read_bytes()
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(data).hexdigest(), "size": len(data)}


def observe(name, executable, filters, expected_cases, output):
    """Run one focused child and reject missing tests, failures or excessive runtime.

    @param name Stable local observation name, never an asset identity.
    @param executable Current focused native target, built from the active checkout.
    @param filters Exact native names/tags selected for this bounded burst.
    @param expected_cases Independently counted source cases; a zero-match exit is not success.
    @param output Disposable transcript owner under the checkout's test-cache.
    @return Current terminal proof, elapsed time and transcript identity.
    """
    transcript = output / f"group51-{name}-tests.txt"
    started = time.monotonic()
    with transcript.open("w", encoding="utf-8") as stream:
        result = subprocess.run([str(executable), filters, "-r", "compact"], cwd=ROOT / "build",
                                stdout=stream, stderr=subprocess.STDOUT, timeout=110,
                                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    terminal = transcript.read_text(encoding="utf-8")
    passed = re.search(r"Passed (?:all |both )?(\d+) test cases? with (\d+) assertions?", terminal)
    # Catch may return zero for unmatched filters; require the actual selected
    # case count as well as a successful exit before admitting renderer evidence.
    if result.returncode != 0 or passed is None:
        raise AssertionError(f"{name}: native failure or missing terminal summary; see {transcript}\n{terminal[-5000:]}")
    assert int(passed[1]) == expected_cases, (name, passed[1], expected_cases)
    row = {"name": name, "cases": int(passed[1]), "assertions": int(passed[2]),
           "elapsed_seconds": round(time.monotonic() - started, 3), "transcript": identity(transcript)}
    print(f"{name}: {row['cases']} cases / {row['assertions']} assertions", flush=True)
    return row


def main():
    """Observe only Group51 selectors and retained conventional/Space representatives."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary-dir", type=Path, default=ROOT / "build/Debug", help="Current focused executable directory.")
    parser.add_argument("--output", type=Path, default=ROOT / "test-cache/merge-assets", help="Disposable transcript/receipt directory.")
    args = parser.parse_args()
    output = args.output.resolve()
    output.relative_to(ROOT / "test-cache")
    output.mkdir(parents=True, exist_ok=True)
    binaries = {name: (args.binary_dir / f"openttd_{name}_test.exe").resolve()
                for name in ("tube", "ground_consumer", "space_art")}
    results, sources = [], []
    # Each source file is its own short invocation. Generator/riser and conduit
    # matrices remain isolated, so a slow native case cannot hide in a full suite.
    for name in ("transport_tube_sprite", "transport_tube_vehicle_sprite", "transport_tube_facility_sprite",
                 "transport_tube_conduit_sprite", "geothermal_generator_sprite", "geothermal_riser_sprite",
                 "geothermal_conduit_sprite", "multilayer_collector_selection"):
        source = ROOT / "src/tests" / f"{name}.cpp"
        tags = re.findall(r'TEST_CASE\([^\n]+, "(\[[^"\n]+\])"\)', source.read_text(encoding="utf-8"))
        assert tags, name
        results.append(observe(name, binaries["tube"], ",".join(dict.fromkeys(tags)), len(tags), output))
        sources.append(identity(source))
    results.append(observe("provider-art", binaries["tube"], "[merged-tube-provider-art]", 1, output))
    results.append(observe("control-art", binaries["tube"], "[merged-native-control-art]", 1, output))
    results.append(observe("retained-ground-camera", binaries["ground_consumer"], "[merged-ground-camera]", 5, output))
    representatives = (
        "Final Space ranges resolve every authored pose and refuse missing indices",
        "Orbital per-cell foreground bounds retain world anchors and physical approach clearance",
        "Surface rocket presentation and native picking preserve layers slots flags and custody",
        "Native image dispatch selects genuine rocket poses and preserves other role families",
    )
    results.append(observe("retained-space", binaries["space_art"], ",".join(representatives), 4, output))
    dispatch = ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json"
    bindings = json.loads(dispatch.read_text(encoding="utf-8"))
    source_paths = {edge["source"] for row in bindings["families"] for edge in row.get("native_chain", [])}
    source_paths.update(("src/tests/transport_tube_provider_switch.cpp", "src/tests/utility_controls_window.cpp",
                         "src/transport_tube.h", "src/transport_tube_power.cpp", "src/transport_tube_topology.cpp",
                         "src/utility_power.cpp", "src/underground_gui.h", "src/geothermal_gui.h"))
    source_paths.update(f"src/table/{name}_sprite_assets.h" for name in ("multilayer", "ground", "space", "orbital", "launch_fleet"))
    sources.extend(identity(ROOT / path) for path in sorted(source_paths))
    receipt = {"_documentation": __doc__, "complete": True, "component_worlds_only": True,
               "cases": sum(row["cases"] for row in results), "assertions": sum(row["assertions"] for row in results),
               "executables": {name: identity(path) for name, path in binaries.items()},
               "dispatch": identity(dispatch), "sources": sources, "results": results}
    (output / "group51-renderer-verification.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {receipt['cases']} focused native cases / {receipt['assertions']} assertions.")


if __name__ == "__main__":
    main()
