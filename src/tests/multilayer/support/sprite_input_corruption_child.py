"""Deliberately mutate only this input-boundary test's own generated payload.

@file sprite_input_corruption_child.py Actual subprocess fault fixture, never native loading proof.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18B.
"""

import os

from artifact_paths import RunContext, contained, repository_root


def main():
    """Write a controlled closing-integrity fault inside this exact test scenario.

    @return None; original suppliers, canonical assets and saved worlds remain untouched.
    """
    context = RunContext(repository_root(), os.environ["OTTD_TEST_FEATURE"],
                         os.environ["OTTD_TEST_SCENARIO"], os.environ["OTTD_TEST_RUN_ID"])
    if not context.scenario.startswith("sprite-input-check-"):
        raise ValueError("corruption fixture requires its explicit test-only scenario")
    path = contained(os.environ["OTTD_TEST_EXPECTED_SPRITES"], context.data)
    path.write_bytes(b"deliberate test-only corruption")
    print("input corruption fixture completed")


if __name__ == "__main__":
    main()
