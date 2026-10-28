# Straight motion and terminal geometry

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Groups 9-10.
Existing ownership: [map-layer architecture](../../architecture/map-layers/archi-map-layers.md#generated-space-bodies-and-current-mining-simulation).

## Physical boundary

`SpacePlaneBounds` describes the inclusive native centre rectangle from `(0,0)`
to `((width-1)*65536,(height-1)*65536)`. Supported dimensions remain powers of two
from 64 to 4096, including 64x4096 and 4096x64. The body's centre may reach the
last tile centre. The next outward step crosses an edge and must be retired by
the later lifecycle owner before publishing that proposed position.

`SpaceTravelBand` still owns its existing middle-half initial placement and
legacy reflection contract. The new geometry neither changes generation nor
reinterprets static site placement. The live controller remains disconnected
until the plan's safety/persistence gates pass. Historical reflection tests
continue to characterize that still-connected old controller; their reflection
requirement is superseded when Group 29 publishes the new lifecycle.

| Body art | Centre example on a 64x64 plane | Terminal result |
| --- | --- | --- |
| Small | Centre reaches X=63 tiles moving east | It remains valid; the next positive centre step exits. |
| Medium | Same centre, heading, speed and carry | Same exit tick. Sprite width does not reverse or delay motion. |
| Large | Same centre, heading, speed and carry | Same exit tick. Border clipping of art is allowed. |

All sizes keep their original signed velocity. Projected body/module overlap
does not damage, delete or move a paid module. Attached craft still require their
own safe centre/occupancy checks in Group 10; body art cannot authorize an
off-map hull transform.

## Pure exports and arithmetic

`StepSpaceMotionStraight` returns proposed motion plus a crossed-edge bit set.
The result is a proposal, not permission to publish a live body or attachment.
`ForecastSpaceMotionExit` uses the same axis arithmetic and returns the first
outside tick, the complete last-safe motion/carry state and the crossed edges.
`AsteroidExitForecast` is an alias of this pure geometry record;
`ForecastAsteroidExit` reads only the body's existing motion. Size, mineral,
reserve, leases, knowledge and renewal credits are untouched.

For a moving axis at distance `d` Q16 coordinate units from its inclusive edge,
the first outside opportunity is
`ceil(((d+1)*ticks_per_day - remainder) / abs(velocity))`.
Ceiling division uses quotient plus nonzero remainder, avoiding an addition
overflow. Signed components are widened before taking magnitude, including
`INT32_MIN`. The earliest moving-axis exit bounds both axes' last-safe travel;
the canonical unsigned carry is retained. A stationary axis has no exit, and
a completely stationary motion returns a valid forecast with no exit tick.

Invalid positions, noncanonical remainders and unsupported bounds return
absence. Analytic horizons are 64-bit: starting at X=0 on a 4096-wide plane at
one Q16 unit/day takes 19,859,374,154 opportunities to exit. The last safe X is
268,369,920 with remainder 73. These queries allocate no body, advance no field
clock and consume no RNG.

## Focused verification

Status: Group 9 passed 243,907 assertions in four focused cases. The release
command completed in under one second; log: `logs/space-fixes/straight-motion.log`.
Test executable SHA256:
`d0447a0d74125f5d1f3c4f7fcece0a87812dd51b639ce580c7d8aa1619292b82`.

`space_straight_motion.cpp` compares the analytic forecast to repeated pure
steps for the complete canonical heading/speed catalogue, all art sizes, unequal
carries and both narrow orientations. Separate literal cases cover passage
through the old band, exact last-centre arrival and following exit, every corner,
stationary axes, long horizons, extreme signed components and invalid input.
An actual generated-field copy retains its band, starter input and every body
after inspection, with the global RNG unchanged.

Build `openttd_space_fixes_test`, then run only `[space-straight-motion]` from
`build/`, redirecting its output to `logs/space-fixes/straight-motion.log`.
No runtime asteroid trajectory is changed by this batch.
