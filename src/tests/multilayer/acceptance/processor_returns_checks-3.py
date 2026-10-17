"""Independently decode actual product, paid-shell, descent and collection owners.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12B
Read-only same-file tables and original wire layouts qualify conserved physical packets.
"""
from pathlib import Path
import re
import runpy

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
TABLES = runpy.run_path(str(HERE / "heavy_joined_checks-2.py"))["table_rows"]
LAYOUT = runpy.run_path(str(HERE / "heavy_contained_service_checks-4.py"))["structure_rows"]
RECIPES = runpy.run_path(str(HERE / "orbital_processor_checks-3.py"))["records"]


def snapshot(saved):
    """Decode one actual438 checkpoint without native object allocation.

    @param saved Exact retained ordinary native save identity.
    @return Independent packet quantities/provenance, actual paid custody and producer history.
    """
    assert identity(ROOT / saved["path"]) == saved
    version, raw = read_stream(ROOT / saved["path"]); assert version == 438

    def integer(position, width):
        """Read an original network-order scalar.

        @param position Exact same-file field start.
        @param width Original admitted scalar width.
        @return Detached unsigned value.
        """
        return int.from_bytes(raw[position:position + width], "big")

    _, start, _, end = riff_location(raw, b"CDIM"); labels = {}
    for p in range(start + 1, end, 6):
        label = raw[p + 1:p + 5].decode("ascii"); labels[raw[p]] = label
        if label in ("STEL", "GOOD"):
            assert raw[p + 5] == (16 if label == "STEL" else 8)
    packets = {}
    for row in TABLES(raw, b"CAPA"):
        fields = {f["path"]: integer(f["position"], f["end"] - f["position"]) for f in row["fields"]}
        packets[row["index"]] = {k: fields[k] for k in ("count", "feeder_share", "source", "source_type", "source_id")}
    owned = set(); provenance = {}; feeder = {}

    def container(label, references):
        """Count one exclusive original holding and retain its actual source meaning.

        @param label Same-file canonical cargo meaning.
        @param references Actual CAPA indices, with native index+1 already decoded.
        @return Physical complete-unit quantity.
        """
        quantity = 0
        # A packet may finance only one station, module, carrier, descent or handling owner. Aggregated provenance also proves that collection never relabels products as mined freight.
        for reference in references:
            assert reference not in owned; owned.add(reference); packet = packets[reference]; assert packet["count"] > 0
            quantity += packet["count"]
            key = f"{label}/{packet['source']}/{packet['source_type']}/{packet['source_id']}"
            provenance[key] = provenance.get(key, 0) + packet["count"]
            feeder[label] = feeder.get(label, 0) + packet["feeder_share"]
        return quantity

    stations, facilities = {}, {}
    for row in TABLES(raw, b"STNN"):
        stock = {"STEL": 0, "GOOD": 0, "PROP": 0}
        for field in row["fields"]:
            if field["path"] == "facilities":
                facilities[str(row["index"])] = integer(field["position"], field["width"])
            match = re.search(r"\.goods\[(\d+)\]\.cargo\[\d+\]\.second$", field["path"])
            # Every native next-hop bucket contributes, including invalid-destination buckets. Catchment or a displayed cached total cannot replace actual waiting packets.
            if match and labels[int(match[1])] in stock:
                label = labels[int(match[1])]; refs = [integer(p, 4) - 1 for p in range(field["position"], field["end"], 4)]
                stock[label] += container(label, refs)
        stations[str(row["index"])] = stock
    structures = LAYOUT(raw, 436)["rows"]; modules = {}
    #438 retains the original436 SPST layout. This parser selector changes no header or saved state; each retained input/output is still its original physical participant.
    for site in (0, 2, 3, 4):
        stock = {}
        for ordinal, entry in structures[site].get("stock", {}).items():
            refs = [integer(p, 4) for p in entry["packet_offsets"]]
            stock[labels[ordinal]] = container(labels[ordinal], refs)
            assert stock[labels[ordinal]] == entry["quantity"]
        modules[str(site)] = stock
    carriers = {}
    for row in TABLES(raw, b"VEHS"):
        if row["index"] not in (18, 19):
            continue
        fields = row["fields"]
        cargo, = [f for f in fields if f["path"].endswith(".cargo_type")]
        references, = [f for f in fields if f["path"].endswith(".cargo.packets")]
        refs = [integer(p, 4) - 1 for p in range(references["position"], references["end"], 4)]
        label = labels[integer(cargo["position"], 1)]
        carriers[str(row["index"])] = dict(label=label, quantity=container(label, refs))
    _, start, _, end = riff_location(raw, b"LCHB"); p = start + 10; hubs = {}
    for _ in range(integer(start + 8, 2)):
        center, count = integer(p, 2), integer(p + 10, 1); p += 11
        hubs[str(center)] = [integer(q, 2) for q in range(p, p + 10 * count, 10)]; p += 10 * count
    assert p == end
    _, start, _, end = riff_location(raw, b"SPOD"); pods = {}
    assert start + 10 + 65 * integer(start + 8, 2) == end
    for p in range(start + 10, end, 65):
        pods[str(integer(p, 2))] = dict(generation=integer(p + 2, 8), custody=integer(p + 11, 1), craft=integer(p + 32, 4), capital=integer(p + 60, 4))
    next_pod = integer(start, 8)
    _, start, _, end = riff_location(raw, b"SPDJ"); p = start + 10; descents = []
    for _ in range(integer(start + 8, 2)):
        count = integer(p + 61, 1); label = labels[integer(p + 48, 1)]
        quantity = container(label, [integer(q, 4) for q in range(p + 62, p + 62 + 6 * count, 6)])
        assert quantity == integer(p + 49, 2)
        descents.append(dict(pod=integer(p, 2), generation=integer(p + 2, 8), sender=integer(p + 36, 4), sender_generation=integer(p + 40, 8),
                             model=integer(p + 56, 2), weight=integer(p + 58, 1), mass=integer(p + 59, 2), label=label, quantity=quantity,
                             remaining=integer(p + 51, 2), point=[integer(p + 28, 4), integer(p + 32, 4)]))
        p += 62 + 6 * count
    assert p == end
    _, start, _, end = riff_location(raw, b"LHND"); p = start + 11; handling = []
    for _ in range(integer(start + 8, 2)):
        count = integer(p + 34, 1); label = labels[integer(p + 28, 1)]
        quantity = container(label, [integer(q, 4) for q in range(p + 35, p + 35 + 6 * count, 6)])
        handling.append(dict(center=integer(p + 8, 2), member=integer(p + 18, 2), label=label, quantity=quantity, reverse=integer(p + 29, 1), remaining=integer(p + 30, 2)))
        p += 35 + 6 * count
    assert p + 11 * integer(start + 10, 1) == end
    return dict(stations=stations, facilities=facilities, modules=modules, carriers=carriers, hubs=hubs, pods=pods, next_pod=next_pod,
                descents=descents, handling=handling, provenance=provenance, feeder=feeder, recipes=RECIPES(raw))
