"""Reject two correctly framed module-flight faults per bounded burst.

@file orbital_freight_checks-7.py Full native loader and independent same-file flight/request/receipt refusals.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11C.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("orbital_freight_checks-1.py")))["native"]
FAULTS = (("future-module", "early-receipt"), ("incompatible-receipt", "retained-payload"), ("wrong-request", "old-schema"))


def damage(context, source, fault):
    """Alter one explicit phase/domain/receipt claim without changing native family framing.

    @param context Exact malformed-input artifact owner.
    @param source Genuine423 checkpoint preserved byte-for-byte.
    @param fault One declared module history, successful payload, request ownership or old-schema fault.
    @return Fresh repository-local malformed snapshot whose full/copy validators must refuse.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 423
    changed = bytearray(stream)
    _, body, length, _ = riff_location(stream, b"SPFL")
    assert stream[body] == 1 and length == 85
    row = body + 1
    assert struct.unpack_from(">I", stream, row)[0] == 15
    if fault == "future-module":
        _, sites, _, _ = riff_location(stream, b"SPST")
        changed[row + 40:row + 48] = stream[sites:sites + 8]
    elif fault == "early-receipt":
        changed[row + 59] |= 16
    elif fault == "incompatible-receipt":
        changed[row + 59] |= 4
    elif fault == "retained-payload":
        # Give the real loaded delivery a structurally legal successful recovery receipt; actual freight/parts still aboard must independently invalidate it.
        changed[row + 12] = 4
        changed[row + 13] = 0
        changed[row + 56:row + 58] = struct.pack(">H", 222)
        changed[row + 59] |= 2 | 16
    elif fault == "wrong-request":
        _, orders, _, _ = riff_location(stream, b"SPOR")
        first = orders + 9
        assert struct.unpack_from(">I", stream, first)[0] == 15 and stream[first + 21] == 0 and stream[first + 22] == 1
        request = first + 23
        # Another actually issued owned site is a valid static approach domain, but this original rocket's exact saved flight does not own it.
        changed[request + 1:request + 5] = struct.pack(">I", 2)
        changed[request + 5:request + 13] = struct.pack(">Q", 3)
    elif fault == "old-schema":
        # SPST422/423 and SPFL84-byte framing agree; only the explicit423 module destination meaning is absent in this older schema.
        version = 422
    else:
        raise ValueError(fault)
    target = context.saves / ("module-flight-malformed-" + fault + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    """Use exact positive proof identities for both full-loader and active-authority-preserving copied-preview refusals."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--completion", type=Path, required=True, help="Exact positive orbital-module-flight-complete.json")
    parser.add_argument("--batch", type=int, required=True, choices=(1, 2, 3))
    args = parser.parse_args()
    predecessor = args.completion.resolve()
    assert predecessor.is_relative_to(ROOT)
    completed = json.loads(predecessor.read_text(encoding="utf-8"))
    assert completed["configuration"] == args.configuration
    preparation = ROOT / completed["preparation"]["path"]
    assert identity(preparation) == completed["preparation"]
    prepared = json.loads(preparation.read_text(encoding="utf-8"))
    context = current_run_context("space", "module-flight-refusals-" + str(args.batch))
    results = []
    # Each bounded burst covers two distinct claims, requiring both native validators rather than counting arbitrary startup/assertion failures as rejection.
    for fault in FAULTS[args.batch - 1]:
        record = completed["unloaded"] if fault == "incompatible-receipt" else completed["approach"] if fault == "wrong-request" else prepared["delivery"] if fault == "retained-payload" else prepared["preburn"]
        source = ROOT / record["path"]
        assert identity(source) == record
        malformed = damage(context.for_feature("space", "module-flight-damage-" + fault), source, fault)
        native(context.for_feature("space", "module-flight-reject-" + fault), args.configuration, "continue", malformed, reject=True)
        native(context.for_feature("space", "module-flight-preview-" + fault), args.configuration, "continue", source, corrupt=malformed)
        assert identity(source) == record
        results.append({"fault": fault, "source": record, "malformed": identity(malformed), "full_load_rejected": True, "independent_preview_rejected": True})
    report = context.reports / "orbital-module-flight-refusals.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group11C; framed module-flight history/receipt/shared-request refusals.",
        "configuration": args.configuration, "completion": identity(predecessor), "cases": results}, indent=2) + "\n", encoding="utf-8")
    print(f"Module flight refusals/{args.batch}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
