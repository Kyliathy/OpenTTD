"""Recover only the incidental tracked Python bytecode changed by importing the tool.

@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md
"""

from pathlib import Path
import subprocess


def main():
    """Copy the original cache blob back into its repository path using read-only Git.

    @return None. Source changes and all generated art are untouched.
    """
    root = Path(__file__).resolve().parent.parent
    relative = "docs/zz-reach2/upgrades/2026-10/tools/__pycache__/sprite_quantize.cpython-310.pyc"
    target = (root / relative).resolve()
    assert root in target.parents
    original = subprocess.check_output(["git", "show", "HEAD:" + relative], cwd=root)
    target.write_bytes(original)
    print("Recovered the original tracked quantiser cache; no Git write command used.")


if __name__ == "__main__":
    main()
