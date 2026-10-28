# One-body creation and failed renewal intervals

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group28B.

## Publication contract

`PlanNativeAsteroidRenewal` inspects the actual installed field, static geometry,
bounded Space hulls and original retained staging/departure points. Copied
reserved areas have their own unsaved geometry kind; they cannot masquerade as
precise berth points. The proposal retains the complete original renewal ledger
and issuance cursor, in addition to its exact ticket and unissued body. A later
exit may change credit/history without changing that ticket.

`CommitAsteroidRenewal` validates the original active field without allocation,
then distinguishes stale authority from a recognized due interval. Stale
ledger/cursor/tickets change nothing. A recognized blocked, modified-body,
capacity, exhaustion or allocation refusal publishes exactly the defined failed
candidate-stream/timer closure, retaining every credit and original live record.
Repeated commit cannot close the same interval again. A read-only quote never
publishes anything, including when its own geometry allocation fails.

Success independently replans actual geometry, prepares both original field and
index in temporary copies, assigns one fresh ID and debits its exact1/3/5 cost.
Complete validation precedes nonthrowing publication and the successful mineral
notification. New bodies receive finite freshly sampled reserves and no receipt
or lease. No source pointer survives any publication. Saved layout/version445
is unchanged. Normal-loop wiring remains Group29.

Active field validation now sorts bounded live pointers in stack scratch instead
of allocating a vector. This permits reliable due-attempt closure under actual
preparation allocation failure. It remains independent of lifetime-issued ID
count; the bounded scratch is approximately112KiB on64-bit builds.

## Focused verification

`[asteroid-creation],[asteroid-renewal-proposal],[asteroid-renewal],
[asteroid-retirement],[asteroid-live-validation] --warn NoTests` passes:

- RelWithDebInfo: 37,431 assertions in28 cases.
- Debug: 37,395 assertions in28 cases.

Logs: `logs/space-fixes/creation-gate-{release|debug}.log`.
Seven new cases cover exact successful payment/zero-credit disarming, stale and
modified proposals, newly occupied/fully blocked alternatives, real allocation
faults during quote and commit, insufficient different-size credit, exhausted
issuance, genuine maximum live-capacity admission, and earned native mining
cargo/partial-work preservation. The same read-only admission predicate handles
the validated4096x4096 Dense component and actual native commit; the component
does not allocate a huge fictitious native map or claim physical ownership.
Exhausted issuance still permits original retirement (Group28's native case).

The test-only allocation fixture has the Debug proxy-size exclusion documented
in [retirement evidence](r2sff1-w-native-retirement.md); Release covers every
ordinary allocation size. Sequential build logs are
`build-creation-reservations-release.log` and `build-creation-debug.log`.
Adjacent shared geometry checks pass1,717 assertions in7 cases per configuration
(`creation-geometry-{release|debug}.log`). No full regression was run.

The first earned-cargo test correctly refused all entries because a new reserved
rectangle had been labeled as a point berth. The distinct ReservedArea kind
fixes the geometry contract; the retained passing case exercises a real mining
approach request without losing its earned packet or partial work.

`space_fixes_checks.py --source
test-cache/space-fixes/retirement-native-{release|debug}/cargo-origins-current.sav
--cargo-origin --asteroid-creation` declares only the due timer and commits one
actual replacement from already funded original credit. Each configuration
passes516 assertions and separately reloads the resulting445 save. Independent
wire checks retain all19 packets/82 units, original missions/stop states and
the complete new ten-body field/ledger/streams across reload. No retired origin
is rebound to the fresh ID. Runtime artifacts are
`test-cache/space-fixes/creation-native-final-release/`, `creation-native-debug/`
and `creation-reload-{release|debug}/`; each contains its reproducible runtime
inputs, report and `cargo-origins-current.sav`. Logs use those names and
`creation-{cargo|mining|field}-wire-{release|debug}.log`; driver stdout has a
separate `-driver.log`. CRLF-aware `git diff --check` passes.

Executable SHA256:

- Release: `8434940676c6afae035d97d370b23b6e8832e8d0919c488e174740c2ff2e5b0a`.
- Debug: `d8ac4833e3c411df322a624e6bc852898931b58e12181bb76b526227fb3fa36c`.

Due timers, isolated starting hull positions and lifetime-history fixtures are
declared component checkpoints. No elapsed interval, paid procurement, ordinary
loop birth, financial trial or unavailable personal-save reproduction is claimed.
