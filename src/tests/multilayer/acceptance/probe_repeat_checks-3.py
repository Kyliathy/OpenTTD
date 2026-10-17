"""Independently decode actual paid probe return, shell custody and onboard service owners.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group12
Same-file packet identities finance each conserved quantity; native runtime caches supply no evidence here.
"""
from pathlib import Path
import runpy

from space_preview_checks import ROOT, read_stream
from launch_center_checks import riff_location

BASE = runpy.run_path(str(Path(__file__).with_name("probe_extraction_checks-2.py")))
TABLES = BASE["TABLES"]


def snapshot(saved):
    """Join paid receipts, real compartments, descent jobs and original asteroid packets.

    @param saved Exact original ordinary438 save identity.
    @return Detached finite history, manifest/custody/work and conserved ore quantities.
    """
    state = BASE["snapshot"](saved); _, raw = read_stream(ROOT / saved["path"])

    def integer(position, width):
        """Read one original bounded network-order scalar.

        @param position Exact same-file field start.
        @param width Published native field width.
        @return Detached original scalar.
        """
        return int.from_bytes(raw[position:position + width], "big")

    packets = {row["index"]: {f["path"]: integer(f["position"], f["end"] - f["position"]) for f in row["fields"]} for row in TABLES(raw, b"CAPA")}
    _, start, _, end = riff_location(raw, b"CDIM")
    ore, = [raw[p] for p in range(start + 1, end, 6) if raw[p + 1:p + 5] == b"NORE"]
    _, start, _, end = riff_location(raw, b"SPSU"); p = start + 1; manifests = {}
    # Each compartment retains exact paid pod references and actual packet quantities; neither a cached count nor preview allocation supplies stock.
    for _ in range(raw[start]):
        craft = integer(p, 4); count = raw[p + 60]
        row = dict(generation=integer(p + 4, 8), ticks=integer(p + 38, 2), quota=raw[p + 40], paused=raw[p + 41], pending=raw[p + 42],
                   location=raw[p + 43], burned=integer(p + 44, 8), services=integer(p + 52, 8),
                   pods=[[integer(q, 2), integer(q + 2, 8)] for q in range(p + 61, p + 61 + 10 * count, 10)])
        p += 61 + 10 * count; supplies = []
        # Bounded original packet reference lists, rather than a synthesized material bill, finance these three physical supplies.
        for _ in range(3):
            count = raw[p]; p += 1; refs = [integer(q, 4) for q in range(p, p + 6 * count, 6)]; p += 6 * count
            supplies.append(sum(packets[ref]["count"] for ref in refs))
        row["supplies"] = supplies; manifests[str(craft)] = row
    assert p == end and manifests["16"]["generation"] == 2 and manifests["15"]["generation"] == 1
    _, start, _, end = riff_location(raw, b"SPOD"); assert start + 10 + 65 * integer(start + 8, 2) == end
    pods = {str(integer(p, 2)): dict(generation=integer(p + 2, 8), custody=raw[p + 11], craft=integer(p + 32, 4), capital=integer(p + 60, 4)) for p in range(start + 10, end, 65)}
    next_pod = integer(start, 8)
    _, start, _, end = riff_location(raw, b"SPDJ"); next_descent = integer(start, 8); p = start + 10; descents = []
    # The original saved job alone owns its whole payload until recovery. Immutable sender, mass and packet references independently qualify custody.
    for _ in range(integer(start + 8, 2)):
        count = raw[p + 61]; refs = [integer(q, 4) for q in range(p + 62, p + 62 + 6 * count, 6)]
        quantity = sum(packets[ref]["count"] for ref in refs)
        assert quantity == integer(p + 49, 2) == 20 and raw[p + 48] == ore
        assert integer(p + 36, 4) == 16 and integer(p + 40, 8) == 2 and integer(p + 56, 2) == 15 and raw[p + 58] == 16 and integer(p + 59, 2) == 320
        descents.append(dict(pod=integer(p, 2), generation=integer(p + 2, 8), job=integer(p + 10, 8), remaining=integer(p + 51, 2), quantity=quantity, packets=refs))
        p += 62 + 6 * count
    assert p == end
    original_ore = [packet for packet in packets.values() if packet["source_type"] == 3 and packet["source_id"] == 0]
    assert all(packet["source"] == 65535 and packet["feeder_share"] == 0 and packet["count"] > 0 for packet in original_ore)
    state.update(manifests=manifests, pods=pods, next_pod=next_pod, descents=descents, next_descent=next_descent, conserved_ore=sum(packet["count"] for packet in original_ore))
    return state
