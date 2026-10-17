#requires -Version 5.1
<#
.SYNOPSIS
Exercise development cleanup against disposable files inside this repository.
.DESCRIPTION
No Pester or game build is required. Live runtimes/builds/reviews and the local
baseline archive are never changed. Run:
powershell -NoProfile -File src/tests/multilayer/support/cleanup_check.ps1
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
. (Join-Path $repository 'clean.ps1')
$scratchOwner = Join-Path $repository 'mlm/runtime/routing-checks'
$scratch = Assert-CleanupContainment (Join-Path $scratchOwner ('cleanup-' + [Guid]::NewGuid().ToString('N'))) $repository
Assert-CleanupNoLinks $scratch
$null = [IO.Directory]::CreateDirectory($scratch)
$checks = 0

function Assert-Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw "Check failed: $Message" }
    $script:checks++
}

function Assert-Refused([scriptblock]$Action, [string]$Message) {
    $refused = $false
    try { & $Action | Out-Null } catch { $refused = $true }
    Assert-Check $refused $Message
}

function Write-CheckFile([string]$Path, [string]$Text = 'fixture bytes') {
    $null = Assert-CleanupContainment $Path $scratch
    $null = [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path))
    [IO.File]::WriteAllText($Path, $Text)
}

$junction = $null
try {
    $fixture = Join-Path $scratch 'OpenTTD'
    $discard = @(
        'mlm/runtime/native/first/main/native-art.cfg',
        'mlm/runtime/native/first/main/save/automatic.sav',
        'mlm/runtime/.runs/old/marker',
        'mlm/runtime/routing-checks/old/output.txt',
        'build-probe/compiler.obj',
        'build-probe-tree/src/old-copy.cpp',
        'build-probe-tree/build/old.pdb',
        'build/openttd.dir/Debug/main.obj',
        'build/openttd_lib.dir/Debug/library.lib',
        'build/openttd_test.dir/Debug/test.obj',
        'build/x64/Debug/linker.state',
        'build/Debug/openttd.pdb',
        'build/RelWithDebInfo/openttd_test.pdb',
        'mlm/reviews/acceptance/data-home/old/report.json'
    )
    foreach ($number in 10..17) { $discard += "mlm/reviews/acceptance/demo-guide-load-$number/run/saves/resave.sav" }
    foreach ($relative in $discard) { Write-CheckFile (Join-Path $fixture $relative) }
    $preserve = @(
        'src/source.cpp', 'docs/plan.md', 'media/source.png',
        'src/tests/fixtures/demos/original.sav',
        'build/Debug/openttd.exe', 'build/RelWithDebInfo/openttd_test.exe',
        'build/Debug/runtime.dll', 'build/vcpkg_installed/x64/lib/dependency.lib',
        'build/play.cfg', 'build/private.cfg', 'build/CMakeCache.txt',
        'build/save/manual.sav', 'build/scripts/game_start.scr',
        'build/baseset/original.grf', 'build/lang/english.lng',
        'OpenTTD-baseline/build/save/original.sav',
        'OpenTTD-baseline/build/openttd_console.exe',
        'mlm/runtime/demo/player/save.sav',
        'mlm/reviews/space/continuation/run/saves/input.sav',
        'mlm/reviews/space/capture/run/images/frame.png',
        'mlm/reviews/space/capture/run/images/unique.ppm',
        'mlm/reviews/README.md', 'logs/current-proof.json'
    )
    foreach ($relative in $preserve) { Write-CheckFile (Join-Path $fixture $relative) }
    $pairedRaw = Join-Path $fixture 'mlm/reviews/space/capture/run/images/frame.ppm'
    Write-CheckFile $pairedRaw 'raw capture with a retained PNG'
    $discard += 'mlm/reviews/space/capture/run/images/frame.ppm'

    # Two paths share an allocation entirely inside the deletion set. A second
    # allocation also has a preserved outside link, so deleting its native path
    # must not be reported as freeing that allocation.
    $shared = Join-Path $fixture 'mlm/runtime/native/first/main/openttd_console.exe'
    Write-CheckFile $shared 'shared executable'
    $second = Join-Path $fixture 'mlm/runtime/native/first/main/same-executable.exe'
    $null = New-Item -ItemType HardLink -Path $second -Value $shared
    $outsideShared = Join-Path $fixture 'mlm/runtime/native/first/main/outside-shared.exe'
    Write-CheckFile $outsideShared 'allocation retained by a saved outside link'
    $outsideLink = Join-Path $fixture 'build/save/preserved-link.bin'
    $null = New-Item -ItemType HardLink -Path $outsideLink -Value $outsideShared
    $preserve += 'build/save/preserved-link.bin'

    $before = @(Get-CleanupFiles $fixture | ForEach-Object { $_.FullName })
    $plan = @(Get-DevelopmentCleanupPlan $fixture $false)
    Set-CleanupEstimates $plan
    $after = @(Get-CleanupFiles $fixture | ForEach-Object { $_.FullName })
    Assert-Check (@(Compare-Object $before $after).Count -eq 0) 'planning changes no files'
    $native = @($plan | Where-Object { $_.Kind -eq 'Old native runtimes' })[0]
    $expectedNative = $native.LogicalBytes - (Get-Item -LiteralPath $shared).Length - (Get-Item -LiteralPath $outsideShared).Length
    Assert-Check ($native.ReclaimableBytes -eq $expectedNative) 'hard links count once and outside links prevent claimed savings'
    $listing = & { Show-CleanupPaths $plan } 6>&1 | Out-String -Width 4096
    foreach ($entry in $plan) { Assert-Check ($listing.Contains($entry.Path)) 'dry run lists every full deletion path' }
    $keepCaches = @(Get-DevelopmentCleanupPlan $fixture $true)
    Assert-Check (@($keepCaches | Where-Object { $_.Kind -in @('Compiler intermediates', 'Generated symbols/linker output') }).Count -eq 0) 'keep-intermediates option omits caches and debug symbols'
    foreach ($relative in @('build', 'build/save', 'build/Debug/openttd.exe', 'src', 'OpenTTD-baseline', 'mlm/reviews', 'mlm/reviews/space/continuation')) {
        Assert-Refused { Assert-CleanupTarget (Join-Path $fixture $relative) $fixture } "refuse protected target $relative"
    }
    Assert-Refused { Assert-CleanupContainment ($fixture + '-other') $fixture } 'similarly named sibling is outside the checkout'
    Assert-Refused { Assert-CleanupUntracked $plan @((Join-Path $fixture 'build-probe/compiler.obj')) } 'tracked candidate prevents cleanup'
    $nestedGit = Join-Path $fixture 'build-probe/.git'
    Write-CheckFile $nestedGit 'gitdir: elsewhere'
    Assert-Refused { Get-DevelopmentCleanupPlan $fixture $false } 'probe containing a nested repository is refused'
    Remove-Item -LiteralPath $nestedGit
    $outside = Join-Path $scratch 'outside-native'
    Write-CheckFile (Join-Path $outside 'keep.txt')
    $junction = Join-Path $fixture 'mlm/runtime/native/redirect'
    $null = New-Item -ItemType Junction -Path $junction -Target $outside
    Assert-Refused { Get-DevelopmentCleanupPlan $fixture $false } 'junction in a deletion tree is refused'
    Assert-Check (Test-Path -LiteralPath (Join-Path $outside 'keep.txt')) 'junction target survives'
    [IO.Directory]::Delete($junction)
    $junction = $null

    foreach ($entry in $plan) {
        Assert-CleanupTarget $entry.Path $fixture
        $null = @(Get-CleanupFiles $entry.Path)
        Assert-CleanupUntracked @($entry) @()
        Remove-Item -LiteralPath $entry.Path -Recurse -Force
    }
    foreach ($relative in $discard) { Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture $relative))) "discarded $relative" }
    foreach ($relative in $preserve) {
        Assert-Check (Test-Path -LiteralPath (Join-Path $fixture $relative)) "preserved $relative"
    }
    Assert-Check ([IO.File]::ReadAllText($outsideLink) -eq 'allocation retained by a saved outside link') 'preserved hard link retains its bytes'
    Set-CleanupEstimates $plan -ReuseIdentities
    Assert-Check ($native.ReclaimableBytes -eq $expectedNative) 'completed cleanup can report saved metadata after paths disappear'
    Assert-Check (@(Get-DevelopmentCleanupPlan $fixture $false).Count -eq 0) 'cleanup is repeatable'
    Write-Host "Passed $checks cleanup behavior checks. Live development output was untouched."
} finally {
    if ($null -ne $junction -and (Test-Path -LiteralPath $junction)) { [IO.Directory]::Delete($junction) }
    $null = Assert-CleanupContainment $scratch $scratchOwner
    $null = @(Get-CleanupFiles $scratch)
    Remove-Item -LiteralPath $scratch -Recurse -Force
}
