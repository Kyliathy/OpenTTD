"""Prepare the pinned official GRF encoder pair in the project's disposable cache.

@file prepare_graphics_encoder.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
The immutable OpenTTD6.2.0 Windows archive and its publisher's SHA256 are used
directly. No global installation, source publication or old CMake cache is needed.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[4]
URL = "https://github.com/OpenTTD/grfcodec/releases/download/6.2.0/grfcodec-6.2.0-windows-win64.zip"
SHA256 = "a60a402bbfc37ce326cb89b9017af58adb8cab8f99e5b0201f016006873d0553"


def prepare():
    """Materialize and admit the exact original graphics encoders without global writes.

    @return Absolute cache directory containing the three admitted official executables.
    @throws RuntimeError If selected pins, publisher bytes or actual tool versions disagree.
    """
    # The M1 scope owns the complete base-set encoder pair. Other scopes may
    # declare only Python/Pillow conversion pins; any explicit encoder pin must
    # still agree rather than silently choosing a different tool for one append.
    for family in ("m1", "space", "orbital", "launch", "ground"):
        scope = json.loads((ROOT / f"media/multilayer/{family}-pack-scope.json").read_text(encoding="utf-8"))
        if scope["toolchain"].get("grfcodec", "6.2.0") != "6.2.0" or scope["toolchain"].get("nforenum", "6.2.0") != "6.2.0":
            raise RuntimeError("this bounded preparation supplies only the selected original6.2.0 encoder pair")
    cache = ROOT / "test-cache/graphics-toolchain"
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / "grfcodec-6.2.0-windows-win64.zip"
    if not archive.is_file():
        request = urllib.request.Request(URL, headers={"User-Agent": "OpenTTD-selected-tool-preparation"})
        with urllib.request.urlopen(request, timeout=30) as response:
            data = response.read()
        if hashlib.sha256(data).hexdigest() != SHA256:
            raise RuntimeError("downloaded official encoder archive differs from the publisher SHA256")
        archive.write_bytes(data)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise RuntimeError("cached encoder archive differs from the publisher SHA256")
    target = cache / "grfcodec620"
    target.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as package:
        # Validate every resolved output before extracting any member; archive
        # paths cannot escape this disposable preparation owner.
        for entry in package.infolist():
            (target / entry.filename).resolve().relative_to(target.resolve())
        package.extractall(target)
    tools = {}
    # Exact binaries come from the independently pinned archive. Probe the two
    # versioned encoders; GRFID's paired archive identity supplies its provenance.
    for name, arguments in (("grfcodec", ["-v"]), ("nforenum", ["--version"]), ("grfid", ["--version"])):
        matches = list(target.rglob(name + ".exe"))
        if len(matches) != 1:
            raise RuntimeError("official encoder archive has ambiguous or absent executable: " + name)
        executable = matches[0]
        result = subprocess.run([str(executable), *arguments], capture_output=True, text=True, timeout=10,
                                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        version = (result.stdout + result.stderr).strip()
        if name != "grfid" and "6.2.0" not in version:
            raise RuntimeError("prepared official encoder failed version admission: " + name)
        tools[name] = {"path": str(executable), "sha256": hashlib.sha256(executable.read_bytes()).hexdigest(), "version": version}
    directories = {Path(row["path"]).parent for row in tools.values()}
    if len(directories) != 1:
        raise RuntimeError("official paired tools do not share one executable directory")
    receipt = {"_documentation": __doc__, "source_url": URL, "archive_sha256": SHA256,
               "publisher_release": "https://github.com/OpenTTD/grfcodec/releases/tag/6.2.0", "tools": tools}
    (cache / "grfcodec620-preparation.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return directories.pop()


if __name__ == "__main__":
    print(prepare())
