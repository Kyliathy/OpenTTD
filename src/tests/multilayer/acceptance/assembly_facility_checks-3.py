"""Reject one malformed Assembly facility through full load and independent copied preview.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group6B
The425 role gate uses the unchanged421/422 SPST layout; genuine earlier frames remain inert until explicit activation.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("assembly_facility_checks-1.py")))["native"]
rows = runpy.run_path(str(Path(__file__).with_name("orbital_dock_checks-2.py")))["rows"]
KINDS = ("old-version", "source-generation", "missing-source", "panel-alias", "consumer-alias", "service-history", "unfinished")


def damage(context, source, kind):
    """Change one semantic boundary in a native copy without touching any original or unrelated family.

    @param context Exact malformed-copy artifact owner.
    @param source Genuine connected Assembly checkpoint.
    @param kind One independent schema, source, role or receipt fault.
    @return Uncompressed native malformed copy with the original framing preserved.
    """
    version, raw = read_stream(source); assert version == 425
    offsets = rows(raw, version)
    assemblies = [value for value in offsets.values() if value["kind"] == 9]
    assert len(assemblies) == 1
    assembly = assemblies[0]; changed = bytearray(raw)
    if kind == "old-version":
        version = 424
    elif kind == "source-generation":
        changed[assembly["source"] + 2:assembly["source"] + 10] = struct.pack(">Q", 0xffffffffffffffff)
    elif kind == "missing-source":
        changed[assembly["source"]:assembly["source"] + 10] = struct.pack(">HQ", 0xffff, 0)
    elif kind == "panel-alias":
        changed[assembly["producer"]:assembly["producer"] + 4] = raw[offsets[0]["producer"]:offsets[0]["producer"] + 4]
    elif kind == "consumer-alias":
        changed[assembly["consumer"]:assembly["consumer"] + 4] = raw[offsets[0]["assembly"]:offsets[0]["assembly"] + 4]
    elif kind == "service-history":
        changed[assembly["producer"] + 9:assembly["producer"] + 17] = struct.pack(">Q", 1)
    else:
        assert kind == "unfinished"
        # An Assembly owns no general stock, so its activation/age/sparse-count prefix is exactly five bytes before the original producer field.
        changed[assembly["producer"] - 5:assembly["producer"] - 3] = struct.pack(">H", 1)
    target = context.saves / ("assembly-facility-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def verify(configuration, proof_path, kind):
    """Require actual native refusal in both loading paths without changing accepted current-world authority.

    @param configuration Completed native configuration matching the connected proof.
    @param proof_path Exact accepted connect-phase report, never a guessed latest save.
    @param kind One independent fault; each invocation stays a bounded burst.
    @return Accepted rejection report with immutable original and process identities.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    assert proof["configuration"] == configuration and proof["phase"] == "connect"
    source = ROOT / proof["after"]["path"]; original = identity(source); assert original == proof["after"]
    context = current_run_context("space", "assembly-facility-reject-" + kind)
    corrupt = damage(context, source, kind)
    full = context.for_feature("space", "assembly-facility-full-" + kind)
    native(full, configuration, "connect", corrupt, reject=True)
    preview = context.for_feature("space", "assembly-facility-preview-" + kind)
    native(preview, configuration, "connect", source, corrupt=corrupt)
    assert identity(source) == original
    report = context.reports / "assembly-facility-rejection.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11 Group6B; full-load and independent copied-preview schema/source/role refusals.",
        "configuration": configuration, "kind": kind, "proof": identity(proof_path), "source": original, "malformed": identity(corrupt),
        "native_reports": [identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))]}, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one connected proof and independent malformed boundary, then print its accepted report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--kind", choices=KINDS, required=True)
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.kind).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
