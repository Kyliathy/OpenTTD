# Native Mine policy controls

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group30.

## Delivered controls and disclosure

The bounded inspector has one three-button row: Follow route/home, Nearest
receiver and Wait until full. It edits the selected existing Mine, falling back
to the current Mine only when no explicit row is selected. Selecting a non-Mine
never silently edits another instruction. The native generation-checked command
alone changes policy; its pure admission controls enabling. Saved policy alone
controls the lowered button. No optimistic window policy, mineral, target,
countdown or gameplay state is introduced. Existing Recall/reorder/delete/service
controls keep their original commands and meanings.

The scrollable details retain original queue instructions and disclose the
current effective destination separately. Detaching, same-mineral seeking,
cargo return, service travel and actual mineral/receiver/service/departure waits
read the original continuation. A real native refusal overrides a generic phase
label. Historical body IDs cannot appear as a current live body destination.
No size-credit internals or unauthorized reserve amounts are shown. When no Mine
is selected, the inspector explains how to add/select one.

Presentation refresh clears only a retired body's actionable selection; the
current hull/order remains. An open inspector never substitutes a new body's
highlight. Explicit hull inspection follows its actual live effective body,
then a still-live original/legacy target. A newly opened unselected inspector
may choose its initial live default. Stable widget IDs0-11 remain unchanged;
policy IDs12-14 are named in `widgets/space_survey_widget.h` and registered in
the widget source list. English strings/tooltips use the existing localization
generator and native wrapping.

## Focused verification

RelWithDebInfo and Debug each pass **1,024 assertions in5 cases** with
`[space-mining-policy] --warn NoTests`, covering the actual command's argument
encoding, identity/ownership/service gates, query purity, active work preservation
and cargo-preserving cancellation. Logs:
`logs/space-fixes/mining-controls-policy-final-release.log` and
`mining-controls-policy-debug.log`.

Actual retained application layout checks pass **668 assertions** per build
using `space_fixes_checks.py --save space-heavy-440.sav --layout` at1920x1027,
200% GUI scale and default font. They include each policy control's native
hit-tested rectangle/client bounds plus unchanged field/fleet/orders/clock across
inspector opening and capture. This layout scenario also offers its existing
74 ordinary opportunities; it is not a policy-click or new continuation proof.
Logs use `mining-controls-layout-final-release` and
`mining-controls-layout-debug`, with separate `-driver.log` output.

The actual800x760 native inspector capture fits all four action rows and the
resizer. Its completed legacy queue correctly disables policy editing until a
Mine row is selected. Text and the complete fleet/order contents retain the
existing pixel scrollbar. Source SHA256:
`0c858ed753a847cc98c449f47ecbdc0efa4627f995d0dbde7227c4b230887028`.

Captures/reports/runtime inputs are reproducible under
`test-cache/space-fixes/mining-controls-layout-final-release/` and
`mining-controls-layout-debug/`. Native `inspector.ppm` contains the original
palette-derived pixels. For tools that cannot display PPM, convert losslessly
beside it with `python -c "from PIL import Image; Image.open('PATH/inspector.ppm').save('PATH/inspector.png')"`.
The Debug PNG was visually reviewed; no image-generation or replacement UI
stands in for native rendering. Auxiliary artifacts remain in test-cache.

Sequential final build logs are
`logs/space-fixes/build-mining-controls-final-{release|debug}.log`.
Final focused executable SHA256:

- Release: `0e124c7deb744dccc5b4e66765e76b6fdb51db464a4b46ee7ca8e6d12eb86e00`.
- Debug: `ddccfb29f40b755e0ed71007010e0c8afb5e46a4ce2d301908a3b75698d2738c`.

CRLF-aware `git diff --check` passes. No full regression or financial trial ran.
Group30B retains actual policy press/release, foreign/non-Mine checks, live
partial-load retarget/nearest return, resulting status capture and separate
saved continuation/reload acceptance. Original30.5 was split into that HARD
gate because input, multi-stage gameplay and transport are distinct from the
MEDIUM implementation. No acceptance requirement was removed or marked done
by this layout-only evidence. Canonical `space-heavy-440.sav`,
`space-heavy-service-440.sav` and `space-missions-440.sav` are available for that
pass; do not resume the separate financial simulations or grant missing assets.
