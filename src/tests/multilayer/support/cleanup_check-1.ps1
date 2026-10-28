#requires -Version 5.1
<#
 * @file cleanup_check-1.ps1 Disposable saves, writer/ownership races and partial failures.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group27.
 * @see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
 * Companion to cleanup_check.ps1; each runner remains a focused bounded burst.
#>
[CmdletBinding()]
param()
. (Join-Path $PSScriptRoot 'cleanup_fixture.ps1')
$originalRemoval = (Get-Item Function:Remove-CleanupTarget).ScriptBlock
try {
    $fixture = New-CleanupFixture 'ownership'
    Write-CheckFile (Join-Path $fixture 'logs/witness.log') 'ordinary generated output'
    $nested = Join-Path $fixture 'logs/nested/.git'
    Write-CheckFile $nested 'fixture git marker'
    Assert-Refused { Get-DevelopmentCleanupPlan $fixture $false } 'nested repository' 'nested Git blocks the containing target'
    Remove-Item -LiteralPath $nested
    $outside = Join-Path $scratch 'outside-target'
    Write-CheckFile (Join-Path $outside 'keep.txt') 'outside independent bytes'
    $junction = Join-Path $fixture 'logs/redirect'
    $null = New-Item -ItemType Junction -Path $junction -Target $outside
    $script:CleanupJunctions.Add($junction)
    Assert-Refused { Get-DevelopmentCleanupPlan $fixture $false } 'junction' 'redirected target is refused'
    Assert-Check ([IO.File]::ReadAllText((Join-Path $outside 'keep.txt')) -eq 'outside independent bytes') 'junction target was untouched'
    [IO.Directory]::Delete($junction)
    $plan = @(Get-DevelopmentCleanupPlan $fixture $false)
    Set-CleanupEstimates $plan
    Write-CheckFile (Join-Path $fixture 'logs/new.sav') 'unsupported new original'
    Assert-Refused { Assert-CleanupEntryCurrent $plan[0] $fixture } 'files changed' 'new save invalidates the complete target snapshot'
    $script:CleanupTracked = @('logs/new.sav')
    Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'tracked' 'new tracking state blocks recursive removal'
    $script:CleanupTracked = @()
    $script:CleanupWriterProcesses = @([pscustomobject]@{ ProcessId=987654; Name='openttd_test.exe'; ExecutablePath=(Join-Path $fixture 'build/Debug/openttd_test.exe'); CommandLine='bounded fixture writer' })
    Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'Stop this checkout' 'active relevant writer blocks apply'
    $script:CleanupWriterProcesses = @()
    $null = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'logs/new.sav'))) 'stopped writer allows deletion of disposable saves'

    $fixture = New-CleanupFixture 'type-swap'
    Write-CheckFile (Join-Path $fixture 'logs/old.log') 'old output'
    $entry = @(Get-DevelopmentCleanupPlan $fixture $false)[0]
    Set-CleanupEstimates @($entry)
    $logs = Assert-CleanupContainment (Join-Path $fixture 'logs') $fixture
    $null = @(Get-CleanupFiles $logs)
    Remove-Item -LiteralPath $logs -Recurse -Force
    Write-CheckFile $logs 'a file replaced the directory'
    Assert-Refused { Assert-CleanupEntryCurrent $entry $fixture } 'file/directory type' 'directory-to-file swap refuses removal'
    Remove-Item -LiteralPath $logs

    $fixture = New-CleanupFixture 'disposable-source'
    Write-CheckFile (Join-Path $fixture 'logs/unique.py') '"""@file unique.py @see r2fc2-file-cleanup3.md; previously unclassified source."""'
    $null = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'logs/unique.py'))) 'unclassified source inside a disposable output owner is deleted'
    Assert-Check (Test-Path -LiteralPath (Join-Path $fixture 'src/source.cpp')) 'authored source outside output owners survives'

    $fixture = New-CleanupFixture 'irrelevant-retention-state'
    $original = 'OTTD deliberately unsupported old LZO original'
    Write-CheckFile (Join-Path $fixture 'logs/current.sav') $original
    Write-CheckFile (Join-Path $fixture 'savegames/retained/logs/current.sav') 'distinct earlier save'
    $catalogue = Join-Path $fixture 'src/tests/fixtures/cleanup/source-epochs.json'
    Write-CheckFile $catalogue 'deliberately invalid, stale preservation catalogue'
    $null = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'logs/current.sav'))) 'unsupported disposable save is deleted despite stale preservation state'
    Assert-Check ([IO.File]::ReadAllText($catalogue) -eq 'deliberately invalid, stale preservation catalogue') 'cleanup does not read or repair source history catalogues'
    Assert-Check ([IO.File]::ReadAllText((Join-Path $fixture 'savegames/retained/logs/current.sav')) -eq 'distinct earlier save') 'distinct previous save was never overwritten'

    $fixture = New-CleanupFixture 'locked-unrelated-journal'
    Write-CheckFile (Join-Path $fixture 'logs/current.sav') 'unsupported exact original'
    $pending = Join-Path $fixture 'src/tests/fixtures/cleanup/save-exports.json.pending'
    Write-CheckFile $pending 'injected incomplete journal'
    $handle = [IO.File]::Open($pending, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        # An unrelated backup journal is outside the deletion owners. Cleanup
        # must not open it or depend on a backup process finishing publication.
        $null = Invoke-DevelopmentCleanup -Repository $fixture -Apply
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'logs/current.sav'))) 'locked retention journal cannot block disposable output deletion'
    } finally { $handle.Dispose() }
    Assert-Check ([IO.File]::ReadAllText($pending) -eq 'injected incomplete journal') 'unrelated locked journal remains unchanged'

    # Simulate a writer just before removal. Actual snapshot checks must refuse
    # the changed target, and a fresh invocation can delete its new contents.
    foreach ($mode in @('changed', 'new')) {
        $fixture = New-CleanupFixture $mode
        Write-CheckFile (Join-Path $fixture 'logs/current.sav') 'first exact original'
        $script:MutationMode = $mode
        <#
         * Add a deterministic writer race immediately before actual removal.
         * @param Entry Snapshot planned before the injected writer changes.
         * @param Repository Exact fixture checkout.
         * @return None; the real removal's final snapshot check must refuse.
        #>
        function Remove-CleanupTarget([object]$Entry, [string]$Repository) {
            if ($script:MutationMode -eq 'changed') {
                Write-CheckFile (Join-Path $Repository 'logs/current.sav') 'second exact original'
            } else { Write-CheckFile (Join-Path $Repository 'logs/new.sav') 'new independent original' }
            & $originalRemoval $Entry $Repository
        }
        $refusal = Invoke-DevelopmentCleanup -Repository $fixture -Apply
        Assert-Check ($refusal.Failed.Count -eq 1) "$mode save during cleanup blocks target removal"
        Assert-Check (Test-Path -LiteralPath (Join-Path $fixture 'logs/current.sav')) "$mode save survives refused target"
        Set-Item Function:Remove-CleanupTarget -Value $originalRemoval
        $recovered = Invoke-DevelopmentCleanup -Repository $fixture -Apply
        Assert-Check ($recovered.Failed.Count -eq 0) "$mode save rerun deletes the newly planned disposable output"
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'logs'))) "$mode save recovery removes the output owner"
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $fixture 'savegames'))) "$mode save recovery creates no backup copies"
    }

    $fixture = New-CleanupFixture 'partial-removal'
    Write-CheckFile (Join-Path $fixture 'logs/a.log') 'abc'
    Write-CheckFile (Join-Path $fixture 'logs/b.log') 'remaining'
    <#
     * Inject one partial native removal error after validating the actual target.
     * @param Entry Exact audited fixture target.
     * @param Repository Intended fixture checkout.
     * @return None; remove one ordinary fixture file and then fail deterministically.
    #>
    function Remove-CleanupTarget([object]$Entry, [string]$Repository) {
        Assert-CleanupEntryCurrent $Entry $Repository
        $oneFile = Assert-CleanupContainment (Join-Path $Repository 'logs/a.log') $Repository
        Remove-Item -LiteralPath $oneFile -Force
        throw 'injected partial removal'
    }
    $partial = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    Assert-Check ($partial.Failed.Count -eq 1) 'partial failure is reported and stops subsequent targets'
    Assert-Check ($partial.LogicalBytes -eq 3 -and $partial.ReclaimableBytes -eq 3) 'partial report counts actual removed bytes only'
    Assert-Check (Test-Path -LiteralPath (Join-Path $fixture 'logs/b.log')) 'unremoved target data remains available'
    Set-Item Function:Remove-CleanupTarget -Value $originalRemoval
    $recovered = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    Assert-Check ($recovered.Failed.Count -eq 0) 'rerun after resolved partial failure completes'
    Write-Host "Passed $checks cleanup refusal/recovery checks; real workspace output was untouched."
} finally {
    Set-Item Function:Remove-CleanupTarget -Value $originalRemoval
    Complete-CleanupChecks
}
