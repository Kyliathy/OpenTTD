#requires -Version 5.1
<#
 * @file cleanup_check-1.ps1 Save/writer/ownership races and partial-removal refusals.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group27.
 * @see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
 * Companion to cleanup_check.ps1; each runner remains a focused bounded burst.
#>
[CmdletBinding()]
param()
. (Join-Path $PSScriptRoot 'cleanup_fixture.ps1')
$originalPreservation = (Get-Item Function:Invoke-CleanupPreservation).ScriptBlock
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
    $null = Assert-Exported $fixture 'logs/new.sav' 'unsupported new original'

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

    $fixture = New-CleanupFixture 'source-refusal'
    Write-CheckFile (Join-Path $fixture 'logs/unique.py') '"""@file unique.py @see r2fc2-file-cleanup3.md; previously unclassified source."""'
    Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'Resolve required retained inputs' 'unique source cannot be discarded as a log'
    Assert-Check (Test-Path -LiteralPath (Join-Path $fixture 'logs/unique.py')) 'unclassified source remains present'

    $fixture = New-CleanupFixture 'destination-conflict'
    $original = 'OTTD deliberately unsupported old LZO original'
    Write-CheckFile (Join-Path $fixture 'logs/current.sav') $original
    Write-CheckFile (Join-Path $fixture 'savegames/retained/logs/current.sav') 'distinct earlier save'
    $digest = (Get-FileHash -LiteralPath (Join-Path $fixture 'logs/current.sav') -Algorithm SHA256).Hash.ToLowerInvariant()
    $collision = Join-Path $fixture ('savegames/retained/by-hash/' + $digest.Substring(0,16) + '/current.sav')
    Write-CheckFile $collision 'conflicting hash owner'
    Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'conflicting retained' 'both conflicting destinations block deletion'
    Assert-Check (Test-Path -LiteralPath (Join-Path $fixture 'logs/current.sav')) 'failed export preserves source save'
    Remove-Item -LiteralPath $collision
    $null = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    $null = Assert-Exported $fixture 'logs/current.sav' $original
    Assert-Check ([IO.File]::ReadAllText((Join-Path $fixture 'savegames/retained/logs/current.sav')) -eq 'distinct earlier save') 'distinct previous save was never overwritten'

    $fixture = New-CleanupFixture 'interrupted-publication'
    Write-CheckFile (Join-Path $fixture 'logs/current.sav') 'unsupported exact original'
    $pending = Join-Path $fixture 'src/tests/fixtures/cleanup/save-exports.json.pending'
    Write-CheckFile $pending 'injected incomplete journal'
    $handle = [IO.File]::Open($pending, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        # The real exporter has copied its payload before this exclusive handle
        # interrupts its commit. The cleaner must keep the source target intact.
        Assert-Refused { Invoke-DevelopmentCleanup -Repository $fixture -Apply } 'Preservation apply refused' 'interrupted publication grants no deletion authority'
        Assert-Check (Test-Path -LiteralPath (Join-Path $fixture 'logs/current.sav')) 'interruption leaves original save intact'
    } finally { $handle.Dispose() }
    $null = Invoke-DevelopmentCleanup -Repository $fixture -Apply
    $null = Assert-Exported $fixture 'logs/current.sav' 'unsupported exact original'

    # Inject changes after the actual export, using the real final snapshot/save
    # checks to refuse. Restore the real adapter before each recovery invocation.
    foreach ($mode in @('changed', 'new')) {
        $fixture = New-CleanupFixture $mode
        Write-CheckFile (Join-Path $fixture 'logs/current.sav') 'first exact original'
        $script:MutationMode = $mode
        <#
         * Add a deterministic writer race after the actual export completed.
         * @param Operation Actual plan/apply/verify request.
         * @param Request Unchanged structured preservation proposal.
         * @param Repository Exact fixture checkout.
         * @return Actual preservation result; mutation is the only test injection.
        #>
        function Invoke-CleanupPreservation([string]$Operation, [object]$Request, [string]$Repository) {
            $result = & $originalPreservation $Operation $Request $Repository
            if ($Operation -eq 'apply') {
                if ($script:MutationMode -eq 'changed') {
                    Write-CheckFile (Join-Path $Repository 'logs/current.sav') 'second exact original'
                } else { Write-CheckFile (Join-Path $Repository 'logs/new.sav') 'new independent original' }
            }
            return $result
        }
        $refusal = Invoke-DevelopmentCleanup -Repository $fixture -Apply
        Assert-Check ($refusal.Failed.Count -eq 1) "$mode save after export blocks target removal"
        Assert-Check (Test-Path -LiteralPath (Join-Path $fixture 'logs/current.sav')) "$mode save survives refused target"
        Set-Item Function:Invoke-CleanupPreservation -Value $originalPreservation
        $recovered = Invoke-DevelopmentCleanup -Repository $fixture -Apply
        Assert-Check ($recovered.Failed.Count -eq 0) "$mode save replan/export recovers safely"
        if ($mode -eq 'changed') {
            $null = Assert-Exported $fixture 'logs/current.sav' 'second exact original'
            $first = Join-Path $fixture 'savegames/retained/logs/current.sav'
            Assert-Check ([IO.File]::ReadAllText($first) -eq 'first exact original') 'changed-save recovery retains both original versions'
        } else { $null = Assert-Exported $fixture 'logs/new.sav' 'new independent original' }
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
    Set-Item Function:Invoke-CleanupPreservation -Value $originalPreservation
    Set-Item Function:Remove-CleanupTarget -Value $originalRemoval
    Complete-CleanupChecks
}
