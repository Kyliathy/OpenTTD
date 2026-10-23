#requires -Version 5.1
<#
 * @file cleanup_check.ps1 Actual cleaner scope/export/preview fixture burst.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group27.
 * @see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
 * Run powershell -NoProfile -File src/tests/multilayer/support/cleanup_check.ps1.
 * Native game/build output is untouched; all scratch is below test-cache/cleanup.
#>
[CmdletBinding()]
param()
. (Join-Path $PSScriptRoot 'cleanup_fixture.ps1')
try {
    $fixture = New-CleanupFixture 'scope'
    $discard = @(
        'logs/current-proof.json', 'test-cache/temporary.bin',
        'mlm/runtime/native/first/main/native-art.cfg',
        'mlm/runtime/native/first/main/save/automatic.sav',
        'mlm/runtime/demo/player/save.sav',
        'mlm/runtime/host/first/host/autosave.ss0',
        'mlm/runtime/client/first/client/scenario.scn.bak',
        'mlm/runtime/.runs/old/marker',
        'build-probe/compiler.obj', 'build-probe-tree/build/old.pdb',
        'build/Debug/openttd.exe', 'build/RelWithDebInfo/openttd_test.exe',
        'build/Debug/runtime.dll', 'build/vcpkg_installed/x64/lib/dependency.lib',
        'build/play.cfg', 'build/private.cfg', 'build/CMakeCache.txt',
        'build/save/manual.sav', 'build/scripts/game_start.scr',
        'build/baseset/copied.grf', 'build/lang/english.lng',
        'mlm/reviews/acceptance/data-home/old/report.json',
        'mlm/reviews/space/continuation/run/saves/input.sav',
        'mlm/reviews/space/capture/run/images/frame.png',
        'mlm/reviews/space/capture/run/images/unique.ppm'
    )
    foreach ($relative in $discard) { Write-CheckFile (Join-Path $fixture $relative) }
    $sourceText = [IO.File]::ReadAllText((Join-Path $fixture 'src/source.cpp'))
    Write-CheckFile (Join-Path $fixture 'build-probe-tree/src/old-copy.cpp') $sourceText
    $discard += 'build-probe-tree/src/old-copy.cpp'
    Write-CheckFile (Join-Path $fixture 'src/helper.py') '"""@file helper.py @see r2fc2-file-cleanup3.md; source-backed cache fixture."""'
    Write-CheckFile (Join-Path $fixture 'src/__pycache__/helper.cpython-312.pyc') 'classified synthetic bytecode'
    $discard += 'src/__pycache__/helper.cpython-312.pyc'
    $preserve = @(
        'src/source.cpp', 'src/helper.py', 'docs/notes.txt', 'media/source.png',
        'src/tests/fixtures/demos/original.sav',
        'src/tests/fixtures/__pycache__/immutable.pyc',
        'savegames/user-original.sav', 'OpenTTD-baseline/build/save/original.sav',
        'OpenTTD-baseline/build/openttd_console.exe',
        '!kit/OpenTTD/save/legacy.sav', 'build-unknown/preserved.txt',
        'mlm/reviews/README.md', 'test-cache/.gitignore', 'src/orphan/__pycache__/unknown.cpython-312.pyc'
    )
    foreach ($relative in $preserve) {
        if (-not (Test-Path -LiteralPath (Join-Path $fixture $relative))) { Write-CheckFile (Join-Path $fixture $relative) }
    }
    $script:CleanupTracked = @('test-cache/.gitignore')

    # One allocation is wholly selected, another has a link in a durable owner.
    # Distinct freed bytes must count the first once and the second zero times.
    $shared = Join-Path $fixture 'mlm/runtime/native/first/main/openttd_console.exe'
    Write-CheckFile $shared 'shared executable'
    $null = New-Item -ItemType HardLink -Path (Join-Path $fixture 'mlm/runtime/native/first/main/same-executable.exe') -Value $shared
    $outsideShared = Join-Path $fixture 'mlm/runtime/native/first/main/outside-shared.exe'
    Write-CheckFile $outsideShared 'allocation kept by a durable outside link'
    $outsideLink = Join-Path $fixture 'savegames/preserved-link.bin'
    $null = New-Item -ItemType HardLink -Path $outsideLink -Value $outsideShared

    $before = @(Get-CheckSnapshot $fixture)
    $plan = @(Get-DevelopmentCleanupPlan $fixture $false)
    Set-CleanupEstimates $plan
    $native = @($plan | Where-Object { $_.Kind -eq 'All runtime profiles' })[0]
    $expectedNative = $native.LogicalBytes - (Get-Item -LiteralPath $shared).Length - (Get-Item -LiteralPath $outsideShared).Length
    Assert-Check ($native.ReclaimableBytes -eq $expectedNative) 'hard-link estimates account for a durable outside owner'
    $preview = Invoke-DevelopmentCleanup -Repository $fixture -Apply -DryRun
    $whatIf = Invoke-DevelopmentCleanup -Repository $fixture -Apply -WhatIf
    $after = @(Get-CheckSnapshot $fixture)
    Assert-Check (($before -join "\n") -ceq ($after -join "\n")) 'planning, DryRun and WhatIf write no fixture bytes or paths'
    Assert-Check ($preview.Preview -and $whatIf.Preview) 'explicit preview switches override Apply'
    Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'src/tests/fixtures/cleanup/save-exports.json'))) 'preview commits no archive or journal'
    $keepBuild = @(Get-DevelopmentCleanupPlan $fixture $true)
    Assert-Check (@($keepBuild | Where-Object { $_.Kind -like 'Audited*' }).Count -eq 0) 'keep-build option retains complete audited roots'
    $listing = & { Show-CleanupPaths $plan } 6>&1 | Out-String -Width 4096
    foreach ($entry in $plan) { Assert-Check ($listing.Contains($entry.Path)) 'preview lists every absolute deletion target' }
    foreach ($relative in @('src', 'media', 'savegames', 'OpenTTD-baseline', '!kit', 'build-unknown', 'mlm/reviews', 'mlm/reviews/README.md', 'test-cache', 'test-cache/.gitignore')) {
        Assert-Refused { Assert-CleanupTarget (Join-Path $fixture $relative) $fixture } 'target' "protected target: $relative"
    }
    Assert-Refused { Assert-CleanupContainment ($fixture + '-other') $fixture } 'escapes' 'similarly named sibling remains outside this checkout'
    Assert-Refused { Assert-CleanupUntracked $plan @((Join-Path $fixture 'build-probe/compiler.obj')) } 'tracked' 'one tracked candidate blocks its target'

    $result = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    Assert-Check ($result.Failed.Count -eq 0) 'actual export-and-delete pipeline completed every target'
    foreach ($relative in $discard) { Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture $relative))) "removed generated target: $relative" }
    foreach ($relative in $preserve) { Assert-Check (Test-Path -LiteralPath (Join-Path $fixture $relative)) "preserved durable input: $relative" }
    # Expectations come from the independently authored original fixture text,
    # not from the exporter's chosen filename or its own reported hash.
    foreach ($relative in @(
        'mlm/runtime/native/first/main/save/automatic.sav',
        'mlm/runtime/demo/player/save.sav',
        'mlm/runtime/host/first/host/autosave.ss0',
        'mlm/runtime/client/first/client/scenario.scn.bak',
        'build/save/manual.sav',
        'mlm/reviews/space/continuation/run/saves/input.sav',
        'mlm/runtime/native/first/main/native-art.cfg',
        'build/play.cfg', 'build/private.cfg', 'build/scripts/game_start.scr'
    )) { $null = Assert-Exported $fixture $relative 'independent fixture bytes' }
    Assert-Check ([IO.File]::ReadAllText($outsideLink) -eq 'allocation kept by a durable outside link') 'durable hard-link bytes survive removal'
    Assert-Check ([IO.File]::ReadAllText((Join-Path $fixture '.git')) -eq 'fixture git marker') 'checkout Git marker stays outside all deletion targets'
    $repeat = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    Assert-Check ($repeat.Completed.Count -eq 0) 'repeated apply is idempotent'
    Write-Host "Passed $checks cleanup scope/export checks; real workspace output was untouched."
} finally { Complete-CleanupChecks }
