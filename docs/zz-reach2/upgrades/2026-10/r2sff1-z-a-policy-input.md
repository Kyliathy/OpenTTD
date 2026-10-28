# Actual Mine policy input gate

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), task30B.1.
Group30B's live continuation/replay gate remains open.

`VerifySpaceMiningPolicyControls` uses the existing retained window, native
hit-tested mouse press/release and registered generation-checked policy command.
It prefers an existing actual21, and separately exercises original15 and an
actual unsupported role. If a legacy mission has no current Mine, the real UI
adds one under the existing queue-restart contract. Ordinary `StateGameLoop`
opportunities then earn a partial holding and partial next work. Preservation
assertions apply to subsequent policy clicks, not to that explicit queue edit.

Each of the three choices changes only the expected saved policy/legitimate
pin. The actual held packet identities/sources/quantities, current work, pose,
home, stopped state, travel/cost remainder, field, balance, clock and four
global RNG words stay unchanged during input. Saved lowered state agrees with
native policy. A wrong generation refuses without changing the original fleet.
Real unsupported and non-Mine controls stay disabled/inert; a spectator views
the original foreign-owned hull without modifying its owner/company record.

Both RelWithDebInfo and Debug pass these actual applications:

| Original input / mode | Native outcome | Assertions |
| --- | --- | --- |
| `space-heavy-440.sav --mining-controls edit` | Actual21 earns a partial holding/work after452 ordinary opportunities; all three policy choices and stale/foreign/non-Mine gates pass | 2,901 |
| `space-mining-working-405.sav --mining-controls edit` | Original15 retains its actual holding; two ordinary opportunities establish the typed work context | 201 |
| `space-factory-440.sav --mining-controls unsupported` | Original deployed non-miner remains unchanged after disabled native input | 36 |

Release run names are `policy-input-heavy-fixed-release`,
`policy-input-legacy-release` and `policy-input-unsupported-release`.
Debug names are `policy-input-{heavy|legacy|unsupported}-debug`.
Rebuild with `openttd_space_fixes_test`; sequential build logs are
`logs/space-fixes/build-policy-input-spectator-release.log` and
`build-policy-input-debug.log`. All native logs use the run names under
`logs/space-fixes/`, with separate `-driver.log` output. Each runtime/report and
original-pixel `policy-*-{top|bottom}.ppm` capture lives under
`test-cache/space-fixes/<run>/`; lossless viewing conversion is documented in
[control implementation](r2sff1-z-mining-controls.md).

The edit runs save genuine `policy-current.sav` before the foreign/non-Mine
negative checks. These contain actual partial work and FullLoadNearest policy;
they are inputs for the next lifecycle/transport checks, not a claim that those
checks are already complete. Source bytes remain untouched. Reports bind the
original input, configuration and exact executable fingerprints.

The first heavy input run reached all three successful choices but its
spectator fixture changed only the local company. Native mouse dispatch
correctly requires its current actor to match that observer. The corrected
fixture backs up both as COMPANY_SPECTATOR, leaving every actual owner intact.
The original abort is retained in `policy-input-heavy-release.log`; the
`heavy-fixed-release` run is the passing gate. No product permission check was
weakened. CRLF-aware `git diff --check` passes; no full regression or financial
trial ran.
