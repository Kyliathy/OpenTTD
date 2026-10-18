"""Decode original orbital construction/facility receipts for independent save comparisons.

@file orbital_save_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group14
Version gates follow the native SPST owner. Reading creates no gameplay object,
packet, utility grant or work; absent legacy fields have their explicit native defaults.
"""
from launch_center_checks import riff_location


def structures(raw, version):
    """Read complete original SPST records with explicit source-era field gates.

    @param raw Original decompressed native save stream.
    @param version Genuine source header, from415 through438; never rewritten to emulate history.
    @return Semantic native structure/history/issuance records and exact kind-byte offsets.
    """
    assert 415 <= version <= 438
    _, body, _, end = riff_location(raw, b"SPST")
    position = body

    def read(width):
        """Read one complete fixed-width native field.

        @param width Exact big-endian byte count. @return Its original unsigned value.
        """
        nonlocal position
        assert 0 <= width <= end - position
        value = int.from_bytes(raw[position:position + width], "big")
        position += width
        return value

    def fields(specification):
        """Read a fixed ordered schema without resolving native identities.

        @param specification Ordered field-name/byte-width pairs. @return Exact semantic values.
        """
        return {name: read(width) for name, width in specification}

    def packets():
        """Read one exclusive container and its exact original routing references.

        @return Saved quantity and every native packet-ID/next-hop pair; no packets are created.
        """
        quantity, count = read(2), read(2)
        assert count <= 128
        return dict(quantity=quantity, references=[[read(4), read(2)] for _ in range(count)])

    def service(present):
        """Decode one real service job or the native empty legacy default.

        @param present Whether the genuine source schema contains this job.
        @return Original job, including unknown model for genuine pre436 histories.
        """
        spec = (("craft", 4), ("generation", 8), ("ticks", 2), ("paused", 1), ("interval", 8), ("serial", 8))
        row = fields(spec) if present else dict(craft=2**20 - 1, generation=0, ticks=0, paused=0, interval=0, serial=0)
        row["inputs"] = [packets() for _ in range(2)] if present else [dict(quantity=0, references=[]) for _ in range(2)]
        # Old reservations retain unknown role; loading cannot upgrade their
        # rights by guessing the current model of a surviving native hull.
        row["model"] = read(2) if present and version >= 436 else 65535
        return row

    def manufacture():
        """Read a contained paid factory job, including immutable clone/replacement intent.

        @return Exact original request, reserved inputs, paid work and once-only output receipt.
        """
        row = fields((("phase", 1), ("model", 2), ("serial", 8), ("source", 2), ("source_generation", 8),
                      ("capital", 4), ("berth", 1), ("reservation_interval", 8)))
        row["inputs"] = [packets() for _ in range(4)]
        row.update(work=0, interval=0, energy=0, product=2**20 - 1, product_generation=0, consumed=[0] * 4, clone=None, replacement=None)
        if version >= 427:
            row.update(fields((("work", 4), ("interval", 8), ("energy", 8), ("product", 4), ("product_generation", 8))))
            row["consumed"] = [read(2) for _ in range(4)]
        # Optional intentions contain only bounded original configuration or
        # retirement receipts; neither can become an extra physical hull owner.
        if version >= 428 and read(1):
            clone = fields((("source", 4), ("generation", 8)))
            length = read(1)
            assert length <= 124
            clone["name"] = [read(1) for _ in range(length)]
            count = read(1)
            assert count <= 32
            clone["orders"] = [fields((("action", 1), ("kind", 1), ("id", 4), ("generation", 8))) for _ in range(count)]
            row["clone"] = clone
        if version >= 429 and read(1):
            row["replacement"] = fields((("source", 4), ("generation", 8), ("source_model", 2), ("resume", 1), ("settled", 1), ("credit", 4)))
        return row

    result = dict(next=read(8), count=read(2), history=[0] * 4, rows={})
    if version >= 422:
        result["history"] = [read(8) for _ in range(4)]
    offsets = {}
    # Every original site has variable exclusive containers. Walk each declared
    # extent so a coincidental ID in stock or a captured name cannot select a row.
    for _ in range(result["count"]):
        site = read(2)
        row = fields((("generation", 8), ("owner", 1)))
        offsets[site] = position
        row.update(fields((("kind", 1), ("phase", 1), ("x", 2), ("y", 2), ("content", 2), ("capital", 4), ("work", 4), ("age", 2))))
        row["consumed"], row["inputs"] = [], []
        for _ in range(2):
            row["consumed"].append(read(2))
            row["inputs"].append(packets())
        row["module"] = None
        if version >= 416 and read(1):
            module = fields((("activation", 2), ("age", 2)))
            count = read(1)
            assert count <= 64
            module["stock"] = {}
            # Sparse stock preserves each original label's exclusive packet
            # references; a repeated label cannot silently replace its owner.
            for _ in range(count):
                cargo = read(1)
                assert cargo not in module["stock"]
                module["stock"][cargo] = packets()
            module.update(producer=2**32 - 1, consumer=2**32 - 1, enabled=1, completed=0)
            if version >= 417:
                module.update(fields((("producer", 4), ("consumer", 4), ("enabled", 1), ("completed", 8))))
            module["service"] = service(version >= 417)
            module["assembly_consumer"] = read(4) if version >= 418 else 2**32 - 1
            module.update(source=65535, source_generation=0)
            if version >= 421:
                module.update(fields((("source", 2), ("source_generation", 8))))
            module["dock_service"] = service(version >= 421)
            module.update(manufacture_serial=0, manufacture=None)
            if version >= 426:
                module["manufacture_serial"] = read(8)
                if read(1):
                    module["manufacture"] = manufacture()
            row["module"] = module
        row.update(fraction=0, remainder=0, tick=0, interval=0)
        if version >= 418:
            row.update(fields((("fraction", 2), ("remainder", 2), ("tick", 8), ("interval", 8))))
        assert site not in result["rows"]
        result["rows"][site] = row
    assert position == end, (version, position, end)
    return result, offsets


def construction_summary(records, kind):
    """Expose an actual kind's reservation, spent bill and facility readiness.

    @param records Independently decoded complete structure records.
    @param kind Exact native module kind. @return The unique delivered site's original economic state.
    """
    # Reservation scenarios deliberately retain another unfunded competing
    # blueprint of the same kind. Only the site holding delivered or already
    # consumed material supplies this matrix's economic milestone.
    (site, row), = [(site, row) for site, row in records["rows"].items()
                   if row["kind"] == kind and (any(row["consumed"]) or any(slot["quantity"] for slot in row["inputs"]))]
    return dict(site=site, **row)
