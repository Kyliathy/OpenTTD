"""Inspect actual Ground candidate differences without changing shipping expectations.

@file debug-ground-packing.py Disposable diagnostic for r2fc-file-cleanup3.md17DE.
"""

import difflib
import itertools
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "media/multilayer"))
from artifact_paths import RunContext
import pack_ground_art as packing
import regenerate_ground_sources as source

run_id = "cleanup17de-recipe-02"
state = json.loads((ROOT / "mlm/reviews/rebuild/ground-derived-inputs" / run_id / "reports/rebuild.json").read_text(encoding="utf-8"))
manifests = {}
# Reuse only actual verified source outputs from this explicit failed packing
# invocation. No stage result, signature or original source file is changed.
for requirement in state["signature"]["declarations"]["ground-derived-inputs"]["requires"]:
    name = requirement["recipe"]
    assert state["nodes"][name]["status"] == "verified"
    for asset in state["signature"]["declarations"][name]["original_parameters"]["source_assets"]:
        manifests[asset] = state["nodes"][name]["outputs"]["manifest"]
context = RunContext(ROOT, "multilayer-art", "ground-packing-debug", run_id)
report, files = packing.derive(source_manifests=manifests, context=context)
scope = source.read_scope()
# Keep rejected actual candidates as disposable diagnostics, never as a new
# expected fixture or a replacement canonical shipping file.
for role in ("png", "nfo", "header"):
    name = Path(scope[role]).name
    actual = files[context.data / name]
    expected = (ROOT / scope[role]).read_bytes()
    (ROOT / "test-cache/cleanup" / ("debug-ground-" + name)).write_bytes(actual)
    print(role, "equal", actual == expected, "actual_bytes", len(actual), "expected_bytes", len(expected))
    if role == "nfo":
        print("".join(itertools.islice(difflib.unified_diff(expected.decode("utf-8").splitlines(True), actual.decode("utf-8").splitlines(True)), 35)))
