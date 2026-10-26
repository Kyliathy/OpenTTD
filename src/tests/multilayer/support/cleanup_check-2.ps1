#requires -Version 5.1
<#
 * @file cleanup_check-2.ps1 Current-directory and clean-Git deletion boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Cleanup implementation.
 * @see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
 * Git state is an explicit fixture double; deletion uses the real cleaner.
 * All fixture bytes and test output belong below test-cache/cleanup.
#>
[CmdletBinding()]
param()
. (Join-Path $PSScriptRoot 'cleanup_fixture.ps1')
try {
    $fixture = New-CleanupFixture 'current-root'
    $output = Join-Path $fixture 'logs/output.log'
    Write-CheckFile $output 'disposable output retained on refusal'
    Assert-Check ((Resolve-CleanupRepository $fixture) -eq $fixture) 'current directory and Git root agree'
    $before = @(Get-CheckSnapshot $fixture)

    # Pending source work must prevent deletion regardless of whether it is
    # staged, unstaged, deleted, untracked or inside a familiar output directory.
    foreach ($status in @(' M src/source.cpp', 'M  src/source.cpp', 'D  src/source.cpp', '?? src/new.cpp', '?? logs/new.txt')) {
        $script:CleanupGitChanges = @($status)
        Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'clean Git working tree' "pending Git work blocks apply: $status"
    }
    $preview = Invoke-DevelopmentCleanup -Repository $fixture -Apply -DryRun
    $whatIf = Invoke-DevelopmentCleanup -Repository $fixture -Apply -WhatIf
    Assert-Check ($preview.Preview -and $whatIf.Preview) 'pending Git work still permits both explicit preview modes'
    $after = @(Get-CheckSnapshot $fixture)
    Assert-Check (($before -join "`n") -ceq ($after -join "`n")) 'dirty refusals and previews change no fixture paths or bytes'
    $script:CleanupGitChanges = @()

    Set-Location -LiteralPath (Join-Path $fixture 'src')
    Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'Run clean.ps1 from its repository root' 'running from a subdirectory cannot delete parent output'
    $sibling = New-CleanupFixture 'other-checkout'
    Write-CheckFile (Join-Path $sibling 'logs/output.log') 'other checkout output'
    Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'Run clean.ps1 from its repository root' 'explicit repository cannot redirect cleanup away from the current directory'
    Assert-Refused { & (Join-Path $repository 'clean.ps1') -DryRun } 'Run clean.ps1 from its repository root' 'actual CLI refuses an absolute script path launched from another directory'
    Assert-Check ([IO.File]::ReadAllText($output) -eq 'disposable output retained on refusal') 'original checkout output survives wrong-directory invocations'
    Assert-Check ([IO.File]::ReadAllText((Join-Path $sibling 'logs/output.log')) -eq 'other checkout output') 'current sibling checkout output survives wrong-directory invocations'
    Set-Location -LiteralPath $fixture

    $script:CleanupGitRoot = $scratch
    Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'root of the intended OpenTTD repository' 'parent Git ownership does not authorize a copied script directory'
    $script:CleanupGitRoot = $null
    $marker = Join-Path $fixture 'CMakeLists.txt'
    Remove-Item -LiteralPath $marker
    Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'root of the intended OpenTTD repository' 'missing source marker blocks deletion'
    Write-CheckFile $marker '# @see r2fc2-file-cleanup3.md; restored fixture source marker.'

    Push-Location Env:
    try {
        Assert-Refused { Resolve-CleanupRepository $fixture } 'filesystem repository root' 'a non-filesystem current location cannot select cleanup owners'
    } finally { Pop-Location }

    # Alternate Git directories or indexes can hide pending/tracked files. Each
    # override must refuse before any selected filesystem output is removed.
    foreach ($name in @('GIT_DIR', 'GIT_WORK_TREE', 'GIT_COMMON_DIR', 'GIT_INDEX_FILE')) {
        $previous = [Environment]::GetEnvironmentVariable($name)
        try {
            [Environment]::SetEnvironmentVariable($name, 'injected-other-checkout')
            Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } "Clear $name" "$name cannot redirect ownership checks"
        } finally { [Environment]::SetEnvironmentVariable($name, $previous) }
    }

    $build = Join-Path $fixture 'build'
    $null = [IO.Directory]::CreateDirectory($build)
    $plan = @(Get-DevelopmentCleanupPlan $fixture $false)
    Assert-Refused { Assert-CleanupUntracked $plan @($build) } 'tracked content' 'empty tracked submodule root remains protected'
    Assert-Refused { Assert-CleanupUntracked $plan @((Join-Path $build 'missing.cpp')) } 'tracked content' 'missing tracked descendant still protects its containing target'

    # A Git edit arriving after planning must be caught by the actual final
    # removal boundary, even when generated output itself has not changed.
    $entry = @($plan | Where-Object { $_.Path -eq (Join-Path $fixture 'logs') })[0]
    Set-CleanupEstimates @($entry)
    $script:CleanupGitChanges = @(' M src/source.cpp')
    Assert-Refused { Remove-CleanupTarget $entry $fixture } 'clean Git working tree' 'new source edits after planning block final removal'
    Assert-Check (Test-Path -LiteralPath $output) 'late Git refusal preserves selected output'
    $script:CleanupGitChanges = @()
    $cleaned = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    Assert-Check ($cleaned.Failed.Count -eq 0) 'clean current checkout can delete its declared disposable output'
    Assert-Check (-not (Test-Path -LiteralPath $output)) 'successful clean removes the selected output'
    Write-Host "Passed $checks cleanup directory/Git checks; real workspace output was untouched."
} finally { Complete-CleanupChecks }
