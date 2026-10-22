"""Compare the native fixture hasher with independent standard/Python identities.

@file input_sha256_check.py Explicit standalone application; no game process runs.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups9-10.
Build input_sha256_check.cpp into test-cache; supply that exact image explicitly.
"""

import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile

from artifact_paths import repository_root


def verify_vectors(application, root):
    """Exercise streaming/padding boundaries and both exact original Space saves.

    @param application Explicit freshly built standalone native hash probe.
    @param root Checkout owning all test scratch and retained save inputs.
    @return Number of exact independently checked native digests.
    """
    vectors = [b"", b"abc", b"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", b"a" * 1000000]
    known = ["e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
             "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
             "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
             "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"]
    vectors.extend(bytes(index % 256 for index in range(length)) for length in (1, 55, 56, 57, 63, 64, 65, 127, 128, 129, 1024))
    cache = root / "test-cache/cleanup"
    cache.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="native-hash-", dir=cache) as directory:
        paths, expected = [], []
        # Independently known vectors protect both implementations from sharing
        # a mistaken expected value; boundary vectors then cover every padding
        # transition and the probe's multi-block streaming loop.
        for index, payload in enumerate(vectors):
            path = Path(directory) / f"vector-{index}.bin"
            path.write_bytes(payload)
            paths.append(path)
            digest = hashlib.sha256(payload).hexdigest()
            if index < len(known):
                assert digest == known[index]
            expected.append(digest)
        for configuration in ("RelWithDebInfo", "Debug"):
            path = root / "src/tests/fixtures/checkpoints/space" / configuration / "current.sav"
            paths.append(path)
            expected.append(hashlib.sha256(path.read_bytes()).hexdigest())
        result = subprocess.run([str(application), *map(str, paths)], capture_output=True, text=True, timeout=15)
        assert result.returncode == 0, result.stderr
        assert result.stdout.splitlines() == expected, (result.stdout, expected)
        # A missing selected input fails instead of yielding an empty/prefix hash.
        failure = subprocess.run([str(application), str(Path(directory) / "missing.bin")],
                                 capture_output=True, text=True, timeout=15)
        assert failure.returncode != 0
    return len(expected)


def main():
    """Require an explicit native probe and print only the focused parity result.

    @return None; scratch is removed inside this repository after comparison.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--application", type=Path, required=True)
    args = parser.parse_args()
    root = repository_root()
    print("Native/Python SHA256 vectors verified:", verify_vectors(args.application.resolve(), root))


if __name__ == "__main__":
    main()
