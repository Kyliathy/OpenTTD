"""Qualify current original launch selectors with explicit independent source policy.

@file launch_fleet_runtime_checks.py Original twenty-case current component owner.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JC.
Actual decoder qualification is an explicit separate prerequisite. Historical
applications/GRF receipts do not supply current source-selector acceptance.
"""

import launch_rocket_selector_checks as rocket

FILTER = "[launch-site],[launch-assembly-contract],[space-logistics-contract],[launch-facility-art],[launch-rocket-art]"


def run(context, recipe, configuration, application, policy, terminal):
    """Execute original current launch cases and independent policy/refusal comparisons.

    @param context Explicit selected launch invocation and required evidence owner.
    @param recipe Actual registered twenty-case declaration, not an arbitrary filter.
    @param configuration Independently built actual Debug or RelWithDebInfo image.
    @param application Exact current image already bound to its actual decoder result.
    @param policy Immutable original source phase/receipt/cadence contract.
    @param terminal New current-run actual stdout/stderr path.
    @return Measured actual process, assertion counts and independent policy/refusal coverage.
    """
    from space_source_review import run_native_selectors
    if ",".join(recipe["test_tags"]) != FILTER:
        raise ValueError("original launch producer selection changed")
    actual = run_native_selectors(context, recipe, configuration, application, terminal)
    text = terminal.read_text(encoding="utf-8", errors="replace")
    actual.update(source_policy_comparisons=rocket.verify_observations(text, policy),
                  detached_policy_refusals=rocket.verify_refusals(text, policy))
    return actual


def main():
    """Invoke this original wrapper with explicit current native/build prerequisites.

    @return None; current failures retain their real selected evidence and nonzero status.
    """
    from space_source_review import main as selected_main
    selected_main(default_family="launch")


if __name__ == "__main__":
    main()
