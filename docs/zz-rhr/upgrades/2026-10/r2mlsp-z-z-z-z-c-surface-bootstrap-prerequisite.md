# Surface bootstrap prerequisite

Owner: [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan11 Group15B](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#group-15b---deliver-a-genuine-surface-and-launch-entry), [Plan6 Group13C](../../../zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md#group-13c---supply-paid-surface-bootstrap-cargo).

The previous goal turn made real progress: Group14 is complete with94accepted
processes and15B.1 audited actual initial-content declarations. The route question
is optional; continue with **physical Surface production as written**, unless the
user chooses otherwise. Keep the full master objective active. Nothing permits
reclassifying injected stock or prefab facilities as actual procurement.

Current code reveals an existing, explicitly scheduled prerequisite:
**Plan6 Group13C**, paid Surface bootstrap cargo. Plan1 Group10's bounded external
seed breaks the access/manufacturing bootstrap cycle; its actual station service
is absent. Completing the first four seed labels lets real access/raw-material/
factory work follow. It does not replace the required Surface production chain
with purchased orbital hardware. Plan18's first-plant note independently names
the same missing producer. Do not implement a private acceptance-only supplier.

Active task: **Plan6 13C.1**, pure complete order admission. New files:

- `src/surface_supply.h`: detached request/view/quote and actual-owner capture API.
- `src/surface_supply.cpp`: whole STEL/RFMT/ELEC/BATY quote, actual owned Surface
  rail/lorry origin, profile/registry/date, company funds, shared usage input,
  native32-bit stock headroom, packet room and checked native Q16 price inflation.
- `src/tests/surface_supply.cpp`: explicit component fixtures, actual command-funded
  station plus detached boundary faults. No gameplay procurement/save acceptance.

Both source lists register these files. No order/packet/quota/payment mutation,
UI, command identity or saved schema was added.13C.2/.3/.4 and15B.2/.3 remain open.
The quote's usage argument is read-only input for the upcoming sole native quota
owner; it is not an implemented alternate ledger or a settlement permission.

The first Release test build compiled production admission but caught a missing
explicit `core/backup_type.hpp` include in the new test fixture. It is corrected.
The second Release build uses
`logs/surface-supply-admission-build-RelWithDebInfo-2.log`; check its actual live
session before starting any replacement. At note creation, session69910 is live.
No tests have passed yet. Debug build/focused verification still follows Release.
Use only `[surface-supply-admission],[multilayer-bootstrap]` initially, plus the
actual producer boundary required by the relevant Area; no full regression.
All native/test children must be bounded; wait>=50s when polling a live handle.

The initial game binaries remain unchanged. Original test executables are retained
before rebuilding under
`mlm/reviews/surface-supply/admission-build/20261018-1/original-binaries/`.
These preserve the dated Group14/13L evidence. Future consumers must distinguish
dated binary receipts from the newly linked test build. No Git write was run.

After compaction reread only Plan6 Global/Resource production Area and the nearest
**Paid Surface bootstrap cargo Local** above13C, Plan1 Group10 seed contract's
STEL/RFMT/ELEC/BATY portion, current native station/cargo/payment owners needed
for this task, and this handoff. Relevant Globals: AD-7, explicit layer semantics,
industry command ownership. For the parent consumer, retain AD-12 and Plan11's
15B entry limits; its art/old producer graphs need no replay. Increment the active
13C compaction note when meaningful work preceded compaction. Split after>3.

Next auxiliary prefix: `r2mlsp-z-z-z-z-d-` in this tracked, VSCode-excluded archive.

**13C.1 complete:** both test builds now pass4cases/724assertions. Final members:
`logs/surface-supply-admission-release-2.json` and
`logs/surface-supply-admission-debug-1.json`; binder
`logs/surface-supply-admission-verification.json` SHA256
`f4eb2dd8e9cba5949a362f81dd5ff6f3fa70f1cf890f07079582439612c67de4`.
`logs/surface-supply-admission-build-verification.json` binds final Releasebuild5
and Debugbuild2, unchanged actual game apps and exact retained prior test products.
All handles69910/72500/42534/24745/14928/34590 are terminal.13C.1 is X;
master13C and13C.2-.4 remain unchecked. No quota/order/cargo/save state exists yet.

The runtime fixture also needed existing generated-language setup **before**
`ExcavationFixture` replaces CargoSpecs. Use
`MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); Rig rig;`.
Loading language inside the derived ctor is too late and asserts during native
cargo sorting. This is fixture ordering, not a game/language-pack defect. The
new actual station tests use direct native creation with declared fixture cash;
they do not claim a paid gameplay station or a later unassisted predecessor.
All four labels test both the exact introduction year and the preceding year.

Next13C.2 must implement the real sole order/quota owner and actual non-paying
company-supply purpose. Current `SourceType` values0/1/2/3 mean Industry/Town/HQ/
Asteroid; none represents company supplies. Existing fixture Append calls do not
supply payment/caps. Never reuse Asteroid/HQ identity or fake feeder credit as a
purpose. Any new purpose must survive real packet split/merge/transport and avoid
both final income and subsidies while allowing ordinary material consumption.
Respect saved old meanings and copied/full admission. Keep orders/quotas and
physical packets under their own original native authority, not a second stock
registry. Settlement/refund/retry/lifecycle and persistence are still required.
If this becomes too large, split13C into bounded children with their own Local
reading before exceeding three meaningful compactions.
