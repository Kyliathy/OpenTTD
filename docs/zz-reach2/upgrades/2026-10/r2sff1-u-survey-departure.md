# Survey target departure

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group26.
Prerequisite: [native mining safety](r2sff1-t-mining-safety-publication.md).

`ForgetSpaceBodySurveyOrders` releases only affected current natural Survey/Visit
approaches and uncommitted scan/travel work. Original instructions/cursor,
position, stopped choice, cargo and already earned company disclosure survive.
Only the later body transaction removes the live knowledge receipt. Future
orders and unrelated current targets retain their original state.

Ownerless `ForgetSpaceSurveyBodyTargets` releases its exact lease and incomplete
scan into explicit no-target Return, or retains an already waiting countdown.
`ReleaseUnsafeSpaceSurveyPreviews` prepares pre-translation offset release.
`AdvanceSpaceSurveyPreviews` advances original Return/Waiting without resolving
a body; an empty field remains valid. Selection walks surviving stable IDs with
ascending wrap across the whole live field, independent of compact vector order.
Observation confers no native hull, packet, reserve, company disclosure or bill.

Source445 (`SpaceSurveyDeparture`) admits no-target Return/Waiting using unchanged
34-byte ASTR preview rows. Earlier source eras still require live targets; valid
old rows migrate intact. Current admission rejects a missing non-sentinel target,
no-target pursuit/attachment, hidden scan timer, orphaned lease and positions
beyond the physical last centre. Field lifecycle442, mining444 and all packet/
order widths remain unchanged. No new registry or saved sidecar was introduced.

The original body-motion loop still uses its previous reflection policy until
Group29. New pre-exit helpers are prepared for Group28's atomic retirement and
Group29's one normal-loop call. Explicit component erasures/projections in tests
are not executed terminal events or elapsed-time gameplay evidence.

Both RelWithDebInfo and Debug pass **13,405 assertions in25 cases** using
`[space-survey-departure],[asteroid-field-wire],[asteroid-live-validation],`
`[space-mining-departure] --warn NoTests`. Logs:
`logs/space-fixes/survey-departure-gate-{release|debug}.log`.
The survey-only relevant gate is5,750 assertions in17 cases. Eight new survey
cases cover all four ownerless phases, last-body Return/Waiting, deterministic
selection, pre-edge release, malformed no-target capability, actual native
partial scans and current/future Visit ownership. A3,000-opportunity original
preview run retains all four phases, repeated scans and unchanged finite reserves.
The native survey's first completed scan earns its real company receipt; losing
the second partial scan preserves that receipt until actual fixture erasure.

Actual404 loads and resaves445 with every original field/preview record retained
(`survey-release-old-404.log`). A genuine444 checkpoint was saved before the
version change at `test-cache/space-fixes/survey-source-444/cargo-origins-current.sav`.
The native `--survey-departure` mode releases one actual saved attachment as a
declared pre-erasure component: original point/home/completed count survive,
the original body stays live and no scan is earned. Both configurations save and
preview445 (`survey-{release|debug}-no-target.log`). Separate full-load/preview/
resave runs (`survey-{release|debug}-reload.log`) retain the complete ASTR payload,
including original rows, lifecycle trailer and all four field RNG words. Native
checks retain original packets, owners, clock and four global RNG words.

Three faults each pass full-load and valid-corrupt-valid preview refusal in both
configurations: old444 sentinel forgery, active no-target pursuit and orphaned
original body lease. All **twelve native refusals pass**. Logs:
`logs/space-fixes/survey-{release|debug}-{older|active|lease}-{preview|full}.log`.
An independent preview keeps a different actual440 factory world open and retains
its native owners (`survey-release-isolation.log`). No full regression was run.

Rebuild target `openttd_space_fixes_test`, with configurations sequentially.
Use `src/tests/multilayer/acceptance/space_fixes_checks.py --cargo-origin`
and `--survey-departure` for the deliberate component. Paired refusal uses
`survey_departure_wire_checks.py --source <444> --departure <445> --case older|active|lease --run <unique>`.
Artifacts live in `test-cache/space-fixes/<run>/cargo-origins-current.sav`;
corrupt copies in `test-cache/space-fixes/survey-wire-cases/`. Driver output must
use a distinct `-driver.log`, leaving the native log unlocked. Existing110-second
native process deadlines remain unchanged. CRLF-aware `git diff --check` passes.

| Configuration | Focused executable SHA256 |
| --- | --- |
| RelWithDebInfo | `4b2caf672acd9801565cbaf5f44786a72bcbce529b6cb037239943ef9dd9cb4f` |
| Debug | `5a1f0a14e25a65067ea5def55c1f1e0ab78f6b8b3d41a3d7d39e9ad7ccb62860` |

Build logs: `logs/space-fixes/build-survey-departure-gate-release.log` and
`build-survey-departure-debug.log`. The normal game build remains pending.
