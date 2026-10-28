# Native lifecycle work and live storage

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group33.
Architecture: [desync invariants](../../architecture/desync/archi-desync.md#invariants).

The focused native `[.asteroid-lifecycle-work]` case extends the original
10/160/10,240/14,336-body, clustered and narrow-map component fixtures.
`asteroid_lifecycle_work_checks.py` selects exactly one case/configuration per
invocation with a110-second child deadline, writing native samples under
`test-cache/space-fixes/<run>/native-lifecycle-work.json` and logs/identity
receipts under `logs/space-fixes/`.

New opt-in counters distinguish actual exit/interception forecasts, mineral
selection calls, recognized native renewal attempts, entry candidates examined
and retirement participant visits. They use the existing disposable
thread-local observer and never affect admission, saved state or RNG. New
`InspectAsteroidStorage` / `SpaceSpatialIndex::InspectStorage` queries report
actual rows and container capacities, excluding allocator-specific heap
overhead. Fixed geometric bucket headers are reported separately from live
body slots. Empty buckets already release their former cluster arrays.

The component suite generates the canonical exact initial population and uses
the actual matching Map dimensions. It measures eight native field opportunities,
one large sorted retirement burst,64 high-ID native replacement cycles, and16
fully timed empty-field intervals. Maximum supported live fields additionally
exercise actual capacity refusal. A retained actual native packet allocation
measures inline historical Source storage through all high-ID retirements;
this component allocation is not claimed as paid mining. Group32B supplies the
independent ordinary paid high-ID extraction/custody evidence.

Geometry, a historical issuance prefix and direct due checkpoints are explicitly
declared component inputs. The16 empty-field intervals use actual native timer
opportunities; the small case also earns32 real insufficient-credit intervals.
The latter retains a valid still-populated field with one Small credit, while
survivors are recentered between intervals so unrelated exits cannot fund it.
An actually empty field conserves its original weighted budget; these distinct
conditions are not conflated into a forged one-credit empty world.

The genuine NoCompatibleAsteroid application's `work-retry` mode measures only
the actual `StateGameLoop` calls for five74-opportunity retry periods. Observer
copies, assertions and validation are excluded from its counter scope. Genuine
matching exit events may add legitimate early wakes. It retains actual hull,
holding, queue, permits and original idle upkeep, assigning no geometry, time,
stock or work. Output is `native-retry-work.json` under the private native run.

Initial verification corrected the native source helper's
explicit include. The first small component run reached its isolated packet
allocation without the native pool's mandatory CanAllocateItem preflight;
that test-only setup is corrected. Diagnostic logs remain separate from final
passing operation/storage samples. No gameplay rule has been changed by this
measurement work.

## Release results

All seven explicit cases passed. Run stems are
`lifecycle-work-<normal|largest-normal|largest-dense|clustered|narrow-tall|narrow-wide>-release`;
the successful small run is `lifecycle-work-pool-small-release`. Each native
sample is under that run's private `test-cache/space-fixes/` directory, and
`logs/space-fixes/<run>-work.json` binds its executable/source/log/sample hashes.
Assertions were1540 small,1600 normal,21760 largest-normal,29959 largest-dense,
58634 clustered, and1600 for each narrow orientation. Individual processes
completed well within110 seconds.

Eight actual field opportunities measured exactly80,1280,81920 or114688 body
steps and matching index updates for10/160/10,240/14,336 live bodies. Motion
performed zero nearest-mineral selections, interception forecasts and renewal
attempts. The sorted burst removed N-1 actual bodies/disclosure/index rows;
both supported maximum-population cases closed capacity refusal without credit
debit or ID issuance.

After64 native high-ID turnovers every case retained one live body, one body
slot, one lookup row, eight MSVC hash buckets, one index row/slot and zero
knowledge rows/capacity. Empty previously visited buckets retained zero slots.
The fixed geometric bucket headers were16 for64x64,256 for the160-body normal
and narrow maps, and65,536 for4096x4096. They are map geometry, not lifetime-ID
storage. One native packet remained allocated throughout: Source is16 inline
bytes and CargoPacket is48 bytes in this build. These are object/container
measurements, not allocator-specific total heap or whole-game memory claims.

The66 explicit component birth attempts include initial funding, first high-ID
issuance and64 turnovers. The small example examined134 actual entry candidates;
all cases stayed within the native bounded16-entry proposal/two-proposal commit
budget. Every retired identity disappeared and packet provenance stayed stable.

The16 actual empty-field intervals earned23536 ticks per ordinary/narrow case,
or22659 in the dense/clustered cases, whose earlier capacity failure correctly
consumed its own attempt. Every due interval created at most one different-size
body and retiring it restored the original weighted credit. The small case's32
insufficient-credit intervals earned46631 native ticks, closed32 attempts and
examined zero entry candidates, retaining exactly one Small credit and nine
original bodies without new issuance.

`mining-retry-work-release` passed2619 assertions in the real retained native
application:370 ordinary opportunities, eight actual actors,1597 survivor body
steps/index updates,2960 native craft ticks and attachment visits, one original
exit/eight retirement participant visits, and five mineral selections. It made
zero interception forecasts or renewal attempts during this wait, preserved
the original cargo/queue and charged original idle upkeep. The counter scope
excludes observer/validation overhead.

## Debug parity and completed gate

All seven Debug cases passed under `lifecycle-work-<case>-debug`, and
`mining-retry-work-debug` passed the same2619 native application assertions.
Every generation/motion/burst/capacity/birth/turnover/empty-interval operation
count matches its corresponding Release sample exactly; every reported high-ID
storage row/capacity matches too. The genuine retry counters, actual cargo/queue
preservation and original idle upkeep also match exactly in both configurations.
Only diagnostic component nanoseconds differ; no FPS claim is made.

The ordinary and focused Debug executables built successfully before these
focused checks. The final ordinary Release test rebuild also passed in
`build-final-ordinary-tests-release.log`. Current build logs are
`build-lifecycle-work-{release,debug}.log`, with the Release include/pool setup
corrections in `build-lifecycle-work-includes-release.log` and
`build-lifecycle-work-pool-release.log`. Native cases remained focused and no
full regression was run. Whitespace validation passed in
`lifecycle-work-whitespace.log`. All33.1-33.4 behavior/storage/work conditions
are verified; final documentation and current identity publication continue
in Group34.
