"""Refuse old proof-cache access while exercising actual selected source producers.

@file source_input_guard.py Focused verification tooling; no file relocation/deletion.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17.
"""

import builtins
from contextlib import contextmanager
from functools import partial
import io
import os
from pathlib import Path
from unittest.mock import patch


def guarded_open(original, root, file, *args, **kwargs):
    """Preserve actual file I/O except access to disposable old proof/log storage.

    @param original Actual original builtins/io opener, captured before patching.
    @param root Exact repository whose logs must be dispensable after cleanup.
    @param file Actual path or already-open descriptor supplied by the producer.
    @param args Original mode/buffering/encoding arguments, passed through unchanged.
    @param kwargs Original keyword I/O options, passed through unchanged.
    @return Actual file stream; old proof/log paths raise before any file access.
    """
    # Descriptor-based process streams remain valid. Path-based source reads
    # cannot use yesterday's logs merely because those bytes still exist here.
    if isinstance(file, (str, os.PathLike)) and Path(file).resolve().is_relative_to(root / "logs"):
        raise AssertionError("selected source accessed old proof/log storage: " + str(file))
    return original(file, *args, **kwargs)


@contextmanager
def no_old_proofs(root):
    """Exercise original producers with real I/O and old proof/log access refused.

    @param root Exact selected source checkout.
    @yield None while original file readers/writers enforce the cleanup boundary.
    """
    builtin_open, io_open, path_open = builtins.open, io.open, Path.open

    def selected_path_open(path, *args, **kwargs):
        """Guard pathlib's cached Python3.10 opener before delegating actual I/O.

        @param path Actual Path instance receiving the open request.
        @param args Original mode/buffering/encoding options passed unchanged.
        @param kwargs Original keyword I/O options passed unchanged.
        @return Actual original Path stream after the old-proof boundary check.
        """
        return guarded_open(path_open, root, path, *args, **kwargs)

    with patch("builtins.open", partial(guarded_open, builtin_open, root)), \
            patch("io.open", partial(guarded_open, io_open, root)), \
            patch.object(Path, "open", selected_path_open):
        yield
