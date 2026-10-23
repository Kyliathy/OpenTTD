"""Build art-review grids and select traffic frames from PrintWindow bursts.

@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md (Group 3)
Usage: contact_sheet.py grid out.png columns zoom image.png ...
       contact_sheet.py pick scenes.csv out_folder count
"""

import argparse
import csv
from pathlib import Path

from PIL import Image, ImageDraw


def load_cell(path, box=None):
    """Read one capture, optionally retaining the portal's traffic box.

    @param path Capture PNG.
    @param box Optional left,top,right,bottom rectangle.
    @return Independent RGB image.
    """
    with Image.open(path) as source:
        if box is not None:
            left, top, right, bottom = box
            if not 0 <= left < right <= source.width or not 0 <= top < bottom <= source.height:
                raise ValueError(f"traffic box {box} leaves {path}")
            return source.convert("RGB").crop(box)
        return source.convert("RGB")


def grid(paths, columns, zoom, out, box=None):
    """Enlarge captures with nearest neighbours and label each cell by file name.

    @param paths PNG paths in reading order.
    @param columns Number of columns.
    @param zoom Positive integer enlargement.
    @param out Output PNG.
    @param box Optional common crop, used for picked traffic frames.
    @return None.
    """
    if not paths or columns <= 0 or zoom <= 0:
        raise ValueError("a grid needs images, positive columns and a positive zoom")
    cells = [load_cell(path, box) for path in paths]
    labels = [Path(path).stem for path in paths]
    measure = ImageDraw.Draw(cells[0])
    label_width = max(measure.textbbox((0, 0), label)[2] for label in labels) + 4
    cell_w = max(max(cell.width for cell in cells) * zoom, label_width)
    cell_h = max(cell.height for cell in cells) * zoom + 16
    rows = (len(cells) + columns - 1) // columns
    sheet = Image.new("RGB", (cell_w * columns, cell_h * rows), (64, 64, 64))
    draw = ImageDraw.Draw(sheet)
    for index, (label, cell) in enumerate(zip(labels, cells)):
        x, y = index % columns * cell_w, index // columns * cell_h
        sheet.paste(cell.resize((cell.width * zoom, cell.height * zoom), Image.Resampling.NEAREST), (x, y))
        draw.text((x + 2, y + cell_h - 14), label, fill="white")
    Path(out).parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out)


def changed_frames(paths, box, count):
    """Rank traffic frames by their difference from the per-channel median scene.

    @param paths Burst PNGs in chronological order, all the same size.
    @param box Portal rectangle inside each frame.
    @param count Maximum number of frames to choose.
    @return Paths ranked by descending change, stable on tied scores.
    """
    if not paths or count <= 0:
        raise ValueError("frame selection needs a nonempty burst and a positive count")
    data = []
    source_size = None
    for path in paths:
        with Image.open(path) as image:
            if source_size is not None and source_size != image.size:
                raise ValueError("all burst frames must have the same dimensions")
            source_size = image.size
        data.append(load_cell(path, box).tobytes())
    #Byte buffers avoid millions of Python RGB tuples in a long elevator burst.
    middle = len(data) // 2
    median = bytes(sorted(frame[position] for frame in data)[middle] for position in range(len(data[0])))
    scores = [sum(abs(value - base) for value, base in zip(frame, median)) for frame in data]
    ranked = sorted(range(len(paths)), key=lambda index: -scores[index])
    return [paths[index] for index in ranked[:count]]


def pick(scenes, out_folder, count):
    """Make one cropped traffic grid per scene in the capture script's CSV.

    @param scenes CSV rows: burst folder,end name,left,top,right,bottom.
    @param out_folder Output directory for the scene grids.
    @param count Maximum changed frames per grid.
    @return None.
    """
    if count <= 0:
        raise ValueError("frame count must be positive")
    with Path(scenes).open(newline="", encoding="utf-8-sig") as stream:
        for row in csv.reader(stream):
            if not row or row[0] == "folder":
                continue
            if len(row) != 6:
                raise ValueError("scene CSV rows must contain six fields")
            folder, name, *coordinates = row
            if not name or Path(name).name != name or name in (".", ".."):
                raise ValueError("scene name must be a file name")
            source_folder = Path(folder)
            if not source_folder.is_absolute():
                source_folder = Path(scenes).parent / source_folder
            paths = sorted(source_folder.glob("frame-*.png"))
            box = tuple(map(int, coordinates))
            selected = changed_frames(paths, box, count)
            grid(selected, min(count, 4), 2, Path(out_folder) / f"{name}.png", box)


def main():
    """Dispatch grid creation or traffic selection.

    @return None; invalid arguments or captures raise a clear command error.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_subparsers(dest="mode", required=True)
    grid_args = modes.add_parser("grid")
    grid_args.add_argument("out")
    grid_args.add_argument("columns", type=int)
    grid_args.add_argument("zoom", type=int)
    grid_args.add_argument("paths", nargs="+")
    pick_args = modes.add_parser("pick")
    pick_args.add_argument("scenes")
    pick_args.add_argument("out_folder")
    pick_args.add_argument("count", type=int)
    args = parser.parse_args()
    try:
        if args.mode == "grid":
            grid(args.paths, args.columns, args.zoom, args.out)
        else:
            pick(args.scenes, args.out_folder, args.count)
    except (ValueError, OSError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
