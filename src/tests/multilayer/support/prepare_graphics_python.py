"""Prepare the existing graphics scope's exact Windows Python without global installation.

@file prepare_graphics_python.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
The official immutable3.10.0 embedded distribution and its receipt live only in
test-cache/graphics-toolchain. Existing compatible Pillow bytes remain installed
with the host3.10 interpreter; their exact required version is checked, not changed.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import urllib.request
import zipfile

import PIL

ROOT = Path(__file__).resolve().parents[4]
URL = "https://www.python.org/ftp/python/3.10.0/python-3.10.0-embed-amd64.zip"


def prepare():
    """Materialize the pinned interpreter and bind the installed compatible Pillow package.

    @return Absolute isolated interpreter path after exact version/encoder admission.
    @throws RuntimeError If the host ABI or selected scope cannot supply the exact required tools.
    """
    scope = json.loads((ROOT / "media/multilayer/ground-pack-scope.json").read_text(encoding="utf-8"))
    # A different ABI or encoder cannot borrow installed extension modules and
    # silently rebase graphics expectations. Keep both original pins unchanged.
    if sys.version_info[:2] != (3, 10) or PIL.__version__ != scope["toolchain"]["pillow"]:
        raise RuntimeError("preparation requires host Python3.10 with the scope's exact Pillow")
    if scope["toolchain"]["python"] != "3.10.0":
        raise RuntimeError("this bounded preparation only supplies the original3.10.0 scope")
    cache = ROOT / "test-cache/graphics-toolchain"
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / "python-3.10.0-embed-amd64.zip"
    if not archive.is_file():
        urllib.request.urlretrieve(URL, archive)
    target = cache / "python310"
    target.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as package:
        # Only this invocation's cache may receive archive files. Validate every
        # absolute destination before extracting any entry, including nested ones.
        for entry in package.infolist():
            (target / entry.filename).resolve().relative_to(target.resolve())
        package.extractall(target)
    paths = ["python310.zip", ".", str(Path(PIL.__file__).resolve().parents[1]),
             str(ROOT / "src/tests/multilayer/support"), str(ROOT / "src/tests/multilayer"),
             str(ROOT / "src/tests"), str(ROOT / "media/multilayer")]
    (target / "python310._pth").write_text("\n".join(paths) + "\n", encoding="utf-8")
    executable = target / "python.exe"
    actual = subprocess.check_output([str(executable), "-c", "import sys,PIL; print(sys.version.split()[0],PIL.__version__)"], text=True).strip()
    if actual != f"3.10.0 {scope['toolchain']['pillow']}":
        raise RuntimeError("isolated graphics interpreter/encoder failed exact admission")
    receipt = {"_documentation": __doc__, "source_url": URL, "archive_sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
               "interpreter_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(), "admitted": actual}
    (cache / "python310-preparation.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return executable


if __name__ == "__main__":
    print(prepare())
