# Atomic native asteroid retirement

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group28.

## Publication boundary

`RetireAsteroidBatch` accepts strictly sorted live IDs whose next straight step
crosses the actual map edge. It refuses active mining pursuits/leases. It copies
the existing field/index, applies exact original size credits in ID order,
releases copied ownerless targets and filters sorted company knowledge once.
All fallible work and validation precede native Survey/Visit cleanup and
nonthrowing publication. Mineral notifications follow successful publication.
Borrowed body pointers are invalid across any transaction, including pointers
to surviving bodies. No normal-loop caller is enabled yet.

Field validation now joins surviving replacement weight plus pending credit to
a possible distribution of retired original sizes. Bounds, parity and cumulative
size buckets must admit that distribution. Unknown retired replacements refund
their own spent weight; they cannot mint weight or recycle a size that was never
eligible. This checks necessary aggregate history without replaying lifetime
events or retaining tombstones. Balanced high-ID components remain accepted.

## Verification

Focused tags `[asteroid-retirement],[asteroid-live-validation],
[asteroid-renewal-proposal],[asteroid-field-wire] --warn NoTests` pass:

- RelWithDebInfo: 10,700 assertions in20 cases.
- Debug: 10,664 assertions in20 cases.

Logs: `logs/space-fixes/retirement-gate-proxy-{release|debug}.log`.
Builds: `build-retirement-proxy-{release|debug}.log` in the same directory.
Six retirement cases cover complete batch accounting/index/knowledge, invalid
and repeated batches, actual preparation allocation refusal, stopped mining
release with earned cargo, notification after successful publication, and
retirement with an exhausted issuance cursor. Geometry projections and bounded
lifetime-history inputs are declared components, not elapsed gameplay claims.

The test executable alone links scoped ordinary allocation faults. Debug skips
MSVC iterator-proxy-sized allocations because its standard library allocates
those inside noexcept constructors; an injected failure there terminates before
engine error handling. Release faults every ordinary size, including genuine
allocations sharing the proxy size. Different allocation counts explain the
assertion totals. The initial Debug abort is retained in
`retirement-gate-final-debug.log`; the corrected gate is the proxy-named log.
The normal game never links this fixture.

`space_fixes_checks.py --save space-mining-holds-405.sav --cargo-origin
--asteroid-retirement` uses original natural packets, declares body0's last safe
geometry and calls the actual native batch. Release and Debug save445 and reload
in separate applications. Independent wire checks preserve all19 packets,
18 natural origins,82 units, three hulls' original missions/stop states and the
complete retired ASTR payload/ledger/streams across reload. Original source SHA256:
`69cd64ada1b0dd3f5ee84ffee5a7859148ae5459185860ffbbc1f353aed653ad`.

Rebuildable runtime artifacts:
`test-cache/space-fixes/retirement-native-{release|debug}/cargo-origins-current.sav`
and `retirement-reload-{release|debug}/cargo-origins-current.sav`.
Logs use the same run names; raw checks use
`retirement-{cargo|mining|field}-wire-{release|debug}.log`.
Driver stdout uses distinct `-driver.log` files to avoid locking native logs.

`space_transport_gate_checks.py --case weight-inflation` preserves the genuine
accepted candidate's version and all other families while claiming two cheap
historical births refunded as Large. Both configurations reject it through
full loading and preview against the distinct original440 world, retaining
the open world's owners. Logs: `retirement-weight-{release|debug}-{preview|full}.log`.
The original valid high-ID history also loads under the tightened validator
(`retirement-wide-history-release.log`). CRLF-aware `git diff --check` passes.

No financial trial, normal-loop exit, replacement birth or exact reproduction
of the unavailable personal save is claimed. Group28B supplies creation and
failed-attempt closure; Group29 alone connects ordinary simulation opportunities.
