"""Join every original finite asteroid debit to exactly one native packet holding by saved resource meaning.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group17
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group19
Default mission checks contain no final receiver intake. Surface delivery audits separately reconcile original Industry inputs/history.
"""
from pathlib import Path
import re
import runpy

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

TABLES = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-2.py")))["table_rows"]
LABELS = ("NORE", "WICE", "RARE")


def inventory(saved, require_packet_conservation=True):
    """Reconcile original per-body reserves and all exclusive native cargo owners independently of runtime caches.

    @param saved Exact retained ordinary438 save identity from a paid mission boundary.
    @param require_packet_conservation True requires original packets alone to cover every debit; false exposes the same strict packet ownership for a caller that independently reconciles actual final Industry input/consumption owners.
    @return Separate original reserve/packet quantities and unique holdings; a caller selecting receiver reconciliation must complete that actual native ownership equation before claiming conservation.
    """
    assert identity(ROOT / saved["path"]) == saved
    version, raw = read_stream(ROOT / saved["path"])
    assert version == 438

    def integer(position, width):
        """Read one original bounded network-order field.

        @param position Same-file field start.
        @param width Published native scalar width.
        @return Detached unsigned original value.
        """
        return int.from_bytes(raw[position:position + width], "big")

    _, start, _, end = riff_location(raw, b"CDIM")
    meanings = {raw[p]: raw[p + 1:p + 5].decode("ascii") for p in range(start + 1, end, 6)}
    assert all(label in meanings.values() for label in LABELS)
    packets = {row["index"]: {f["path"]: integer(f["position"], f["end"] - f["position"]) for f in row["fields"]} for row in TABLES(raw, b"CAPA")}
    _, start, _, _ = riff_location(raw, b"ASTR")
    bodies = {}
    # Every saved body keeps its original class and finite reserve; untouched classes must reconcile independently rather than borrowing the mined class's quantity.
    for index in range(integer(start + 27, 4)):
        p = start + 51 + 40 * index
        assert integer(p, 4) == index and raw[p + 4] < len(LABELS)
        initial, remaining = integer(p + 24, 4), integer(p + 28, 4)
        assert initial >= remaining
        bodies[index] = dict(resource=LABELS[raw[p + 4]], initial=initial, remaining=remaining)
    owners = {}

    def own(reference, holding, owner, cargo):
        """Assign an actual original raw packet to its one saved physical owner.

        @param reference Native raw CAPA identity, with table reference bias already removed.
        @param holding Actual Space hold, descent, Surface stock or Surface vehicle domain.
        @param owner Original native hull/station/job identity.
        @param cargo Same-file native cargo index belonging to this holding.
        """
        packet = packets[reference]
        if packet["source_type"] != 3:
            return
        assert reference not in owners and packet["count"] > 0
        body = bodies[packet["source_id"]]
        assert meanings[cargo] == body["resource"] and packet["source"] == 65535 and packet["feeder_share"] == 0
        owners[reference] = dict(holding=holding, owner=owner, resource=body["resource"], body=packet["source_id"], quantity=packet["count"],
                                 age=packet["periods_in_transit"], tile=packet["source_xy"])

    # Original vehicle reference lists supply both orbital holds and actual terrestrial collection vehicles; no cached aggregate can stand in for those packet identities.
    for row in TABLES(raw, b"VEHS"):
        fields = row["fields"]
        cargo_fields = [f for f in fields if f["path"].endswith(".cargo_type")]
        if not cargo_fields:
            continue
        cargo, = cargo_fields
        cargo_type = integer(cargo["position"], cargo["width"])
        for field in fields:
            if not field["path"].endswith(".cargo.packets"):
                continue
            holding = "space_hold" if field["path"].startswith("space[") else "surface_vehicle"
            for p in range(field["position"], field["end"], 4):
                own(integer(p, 4) - 1, holding, row["index"], cargo_type)
    # Native station next-hop buckets retain the goods array's own cargo meaning. Multiple buckets cannot silently count the same packet twice.
    for row in TABLES(raw, b"STNN"):
        for field in row["fields"]:
            match = re.search(r"\.goods\[(\d+)\]\.cargo\[\d+\]\.second$", field["path"])
            if not match:
                continue
            for p in range(field["position"], field["end"], 4):
                own(integer(p, 4) - 1, "surface_stock", row["index"], int(match[1]))
    _, start, _, end = riff_location(raw, b"SPDJ")
    p = start + 10
    # Each original descent owns raw packet indices, unlike table ID+1 references. Its complete actual payload remains separate from the sender's emptied hold.
    for _ in range(integer(start + 8, 2)):
        count = raw[p + 61]
        refs = [integer(q, 4) for q in range(p + 62, p + 62 + 6 * count, 6)]
        assert sum(packets[reference]["count"] for reference in refs) == integer(p + 49, 2)
        for reference in refs:
            own(reference, "descent", integer(p + 10, 8), raw[p + 48])
        p += 62 + 6 * count
    assert p == end
    original = {key for key, packet in packets.items() if packet["source_type"] == 3}
    assert set(owners) == original
    resources = {label: dict(initial=0, remaining=0, space_hold=0, descent=0, surface_stock=0, surface_vehicle=0, consumed=0, disposed=0) for label in LABELS}
    # A body-specific debit must equal its actual original packets before class totals are compared, preventing one body's missing cargo from being hidden by another's surplus.
    for body_id, body in bodies.items():
        emitted = sum(owner["quantity"] for owner in owners.values() if owner["body"] == body_id)
        # Before final industry delivery, packets alone must finance the debit. Final accepted industry stock has its original native owner and must instead be reconciled by the Surface consumer's independent reader.
        if require_packet_conservation:
            assert body["initial"] == body["remaining"] + emitted
        resources[body["resource"]]["initial"] += body["initial"]
        resources[body["resource"]]["remaining"] += body["remaining"]
    for owner in owners.values():
        resources[owner["resource"]][owner["holding"]] += owner["quantity"]
    # These station-free raw-ore worlds perform no ore processing/disposal. Service parts have their own labels; their use cannot finance another resource's equation.
    for resource in resources.values():
        if require_packet_conservation:
            assert resource["initial"] == sum(value for key, value in resource.items() if key != "initial")
    return dict(resources=resources, packets={str(key): owner for key, owner in owners.items()}, bodies=bodies)
