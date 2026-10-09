"""Read and write the indexed and RGB PNGs the layer-art tools share.

No third-party image library. Only the filter and chunk types these
scripts actually meet are implemented.
@see docs/zz-reach2/upgrades/2026-10/r2mlm6-layer-graphics-assets.md
"""

import struct
import zlib


def _chunk(tag, payload):
    """One PNG chunk.

    @param tag Four-byte chunk name.
    @param payload Chunk data.
    @return Length, tag, payload and CRC.
    """
    body = tag + payload
    return struct.pack(">I", len(payload)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def read_png(path):
    """Decode a PNG into width, height, colour type, palette and pixels.

    @param path File to read.
    @return width, height, colour type, palette triples or None, and a flat pixel buffer.
    Indexed pixels are one byte each. RGB pixels are three bytes each.
    """
    raw = open(path, "rb").read()
    if raw[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path} is not a PNG")
    pos = 8
    width = height = bit_depth = colour_type = None
    palette = None
    idat = []
    while pos < len(raw):
        length = struct.unpack(">I", raw[pos:pos + 4])[0]
        tag = raw[pos + 4:pos + 8]
        payload = raw[pos + 8:pos + 8 + length]
        pos += 12 + length
        if tag == b"IHDR":
            width, height, bit_depth, colour_type = struct.unpack(">IIBB", payload[:10])
        elif tag == b"PLTE":
            palette = [tuple(payload[i:i + 3]) for i in range(0, len(payload), 3)]
        elif tag == b"IDAT":
            idat.append(payload)
        elif tag == b"IEND":
            break
    if bit_depth != 8 or colour_type not in (2, 3):
        raise ValueError(f"{path} is not 8-bit RGB or indexed")
    channels = 1 if colour_type == 3 else 3
    inflated = zlib.decompress(b"".join(idat))
    stride = width * channels
    rows = []
    src = 0
    previous = bytearray(stride)
    for _ in range(height):
        filter_type = inflated[src]
        src += 1
        row = bytearray(inflated[src:src + stride])
        src += stride
        #Undo the PNG row filter so the pixels are plain indices or RGB.
        if filter_type == 1:
            for i in range(channels, stride):
                row[i] = (row[i] + row[i - channels]) & 255
        elif filter_type == 2:
            for i in range(stride):
                row[i] = (row[i] + previous[i]) & 255
        elif filter_type == 3:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                row[i] = (row[i] + ((left + previous[i]) // 2)) & 255
        elif filter_type == 4:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                up = previous[i]
                up_left = previous[i - channels] if i >= channels else 0
                row[i] = (row[i] + _paeth(left, up, up_left)) & 255
        elif filter_type != 0:
            raise ValueError(f"unsupported PNG filter {filter_type}")
        rows.append(bytes(row))
        previous = row
    return width, height, colour_type, palette, b"".join(rows)


def _paeth(left, up, up_left):
    """Paeth predictor used by PNG filter 4.

    @param left Pixel to the left.
    @param up Pixel above.
    @param up_left Pixel above-left.
    @return Predicted byte.
    """
    estimate = left + up - up_left
    distance_left = abs(estimate - left)
    distance_up = abs(estimate - up)
    distance_up_left = abs(estimate - up_left)
    if distance_left <= distance_up and distance_left <= distance_up_left:
        return left
    if distance_up <= distance_up_left:
        return up
    return up_left


def write_rgb_png(path, width, height, rgb):
    """Write an 8-bit RGB PNG.

    @param path Destination.
    @param width Image width.
    @param height Image height.
    @param rgb Flat RGB bytes, three per pixel, row major.
    """
    _write(path, width, height, 2, None, rgb, 3)


def write_indexed_png(path, width, height, indices, palette):
    """Write an 8-bit indexed PNG whose palette order is the DOS palette.

    @param path Destination.
    @param width Image width.
    @param height Image height.
    @param indices One palette index per pixel.
    @param palette Sequence of 256 RGB triples.
    """
    payload = b"".join(bytes(colour) for colour in palette)
    _write(path, width, height, 3, payload, indices, 1)


def _write(path, width, height, colour_type, palette_payload, pixels, channels):
    """Write one PNG with no row filter and a fixed compression level.

    @param path Destination.
    @param width Image width.
    @param height Image height.
    @param colour_type PNG colour type.
    @param palette_payload PLTE bytes, or None.
    @param pixels Flat pixel buffer.
    @param channels Bytes per pixel.
    """
    stride = width * channels
    raw = b"".join(b"\x00" + pixels[y * stride:(y + 1) * stride] for y in range(height))
    ihdr = struct.pack(">IIBBBBB", width, height, 8, colour_type, 0, 0, 0)
    parts = [b"\x89PNG\r\n\x1a\n", _chunk(b"IHDR", ihdr)]
    if palette_payload is not None:
        parts.append(_chunk(b"PLTE", palette_payload))
    parts.append(_chunk(b"IDAT", zlib.compress(raw, 9)))
    parts.append(_chunk(b"IEND", b""))
    open(path, "wb").write(b"".join(parts))
