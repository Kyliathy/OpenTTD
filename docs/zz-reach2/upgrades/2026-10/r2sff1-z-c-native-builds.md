# Ordinary build and focused acceptance receipts

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md),31.1.
Both ordinary applications/test targets and both focused native targets are
built. Both ordinary applications pass the immutable-heavy load/resave smoke
check. The focused and ordinary test checks below complete31.1.

Current `openttd_space_fixes_test` builds are complete in both configurations
(`build-policy-final-{release|debug}.log`). Focused affected suites now pass:

| Scope | Release assertions/cases | Debug assertions/cases | Log stem |
| --- | --- | --- | --- |
| Straight motion, attachment, departure, sparse identity and inspection |342,058/29|342,058/29|`acceptance-geometry`|
| Renewal, proposals, retirement, creation and actual field loop |37,957/30|37,921/30|`acceptance-lifecycle`|
| Typed cargo, saved origins/custody, field and mining transport |1,635/27|1,635/27|`acceptance-cargo`|
| Policy, selectors, original returns/service, continuation and safety |117,274/47|117,274/47|`acceptance-orders-registered-release` / `acceptance-orders-debug`|

Logs live in `logs/space-fixes/<stem>-{release|debug}.log` except the explicitly
named order logs. Debug intentionally excludes MSVC noexcept container-proxy
allocation sizes from the real allocation-failure window; that established
test-only distinction accounts for the lifecycle assertion count. No product
capacity/failure predicate differs between configurations. All four invocations
are focused cases with `--warn NoTests`, not an unfiltered regression.

The first Release orders command also named nonexistent standalone construction
tags. Those source files expose functions through the real native application
observer, not their own Catch cases. Warning-as-failure correctly returned2;
all47 actual order cases passed. The corrected filtered rerun returns0. Keep
`acceptance-orders-release.log` as diagnostic history.

The ordinary game has linked in Release. Its real unmodified application loads
and resaves the immutable `space-heavy-440.sav` through console commands, without
an observer or assigned world state. Passing run:
`ordinary-heavy-build-fixed-release`; its report is under
`test-cache/space-fixes/<run>/report.json`, native/driver logs under
`logs/space-fixes/<run>{|-driver}.log`. Application SHA256 at that invocation:
`c958c17408eb1eb0038df2377d38b8a48815ef160c57bab06ef94890f08f124c`.

Rebuild ordinary targets sequentially by configuration:

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' --build build --config RelWithDebInfo --target openttd openttd_test --parallel 2 -- /nodeReuse:false /p:CL_MPCount=4
```

Current Release build log: `build-ordinary-acceptance-release.log`. Shared header
changes recompile the ordinary test target; this is not an unfiltered test run.
Release ordinary application/test builds now finish successfully. Its completed
`openttd_test.exe` passes motion/generation/catalogue/preview checks
(110,658 assertions,16 cases) in `ordinary-motion-producers-release.log`, and a
separate straight-motion/lifecycle/mining-transport registration check
(244,715 assertions,17 cases) in `ordinary-registration-release.log`.

Debug reuses the already-current Debug engine objects/generated files (also used
by the final focused native binary). Direct MSBuild with
`/p:BuildProjectReferences=false` excludes shared CMake/version/string generators
while the independent Release test compilation runs. Debug application build
passes without warnings/errors in `build-ordinary-project-debug.log`; immutable
heavy smoke run `ordinary-heavy-build-debug` passes. The independent Debug test
project completed under `build-ordinary-test-project-debug.log`. Commands:

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe' build/openttd.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:BuildProjectReferences=false /m:2 /nodeReuse:false /p:CL_MPCount=2
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe' build/openttd_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:BuildProjectReferences=false /m:2 /nodeReuse:false /p:CL_MPCount=2 /v:minimal
```

Do not run concurrent shared generators or change their product headers during
these compiles. Both links now pass. Debug repeats the completed ordinary
motion/generation/preview suite (110,658 assertions/16 cases) and registration
suite (244,715 assertions/17 cases), matching Release. Logs are
`ordinary-motion-producers-debug.log` and `ordinary-registration-debug.log`.
These checks retain the historical pure reflection helper while the new
straight lifecycle alone owns the installed native field. Future builds
with changed product sources must rebuild their corresponding engine first;
the explicit no-reference invocation is not a general dependency bypass.

Exact six-binary/current-source fingerprints are recorded in
`logs/space-fixes/acceptance-build-identities.json`; all Git operations used to
record that receipt were read-only.

`space_ordinary_checks.py` reuses `space_demo_checks.ordinary`'s null-driver and
native exit-save pattern, with outputs kept under this plan's private test-cache
and log locations. Run one immutable source with `--mode load` or `advance`;
advance offers3000 actual ordinary driver calls and retains the engine's exit
save. Original sources/configuration remain unchanged. Each child deadline is
110seconds; each run identity must be fresh. The initial smoke command failed
before launching because generic ConfigParser rejected legitimate repeated AI
slot names. The passing implementation preserves the original native config
structure and edits only the disposable GUI autosave setting. Preserve that
failed driver log; no source configuration was rewritten.

Reviewing affected tooling also found that `construction_reload_checks.py`'s
explicit argument object lacked the newer independent observer flags. It now
supplies all flags as disabled, so a construction replay cannot accidentally
invoke mining/lifecycle modes. The actual working checkpoint/reload comparison
passes in `acceptance-construction-working-release-{checkpoint|reload}.log`;
its comparison JSON is beside the private reload save.

Current scoped whitespace check: `acceptance-build-whitespace.log`. Broader
report replay, two-role gameplay/refusal matrix, expiry/spawn replay, ordinary
old/Classic compatibility, peer and work-bound gates remain open in the plan.
