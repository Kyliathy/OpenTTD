#requires -Version 5.1
<#
 * @file cleanup_fixture.ps1 Disposable fixtures for the actual cleaner pipeline.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group27.
 * @see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
 * Git and writer discovery are explicit fixture doubles. File discovery,
 * preservation, export, identity checks and removal use the actual owners.
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..')).TrimEnd('\', '/')
. (Join-Path $repository 'clean.ps1')
$scratchOwner = Join-Path $repository 'test-cache/cleanup/cleaner-checks'
$scratch = Assert-CleanupContainment (Join-Path $scratchOwner ([Guid]::NewGuid().ToString('N'))) $repository
Assert-CleanupNoLinks $scratch
$null = [IO.Directory]::CreateDirectory($scratch)
$script:checks = 0
$script:CleanupTracked = @()
$script:CleanupWriterProcesses = @()
$script:CleanupFakeGitFiles = New-Object 'System.Collections.Generic.List[string]'
$script:CleanupJunctions = New-Object 'System.Collections.Generic.List[string]'

<#
 * Require one independent test expectation.
 * @param Condition Actual result to require.
 * @param Message Business behavior whose failure must be identifiable.
 * @return None; increments the observed assertion count on success.
#>
function Assert-Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw "Check failed: $Message" }
    $script:checks++
}

<#
 * Require the intended safety refusal rather than any accidental exception.
 * @param Action Isolated fixture operation.
 * @param Expected Required substring in the actual refusal.
 * @param Message Business boundary being tested.
 * @return None; a different or missing refusal fails this test.
#>
function Assert-Refused([scriptblock]$Action, [string]$Expected, [string]$Message) {
    try { & $Action | Out-Null } catch {
        Assert-Check ($_.Exception.Message.Contains($Expected)) ($Message + ': ' + $_.Exception.Message)
        return
    }
    throw "Expected refusal: $Message"
}

<#
 * Write independent fixture bytes only beneath the reserved scratch owner.
 * @param Path Exact fixture path.
 * @param Text Independently authored text; save fixtures intentionally need no decoder.
 * @return None; fake Git markers are separately tracked for safe teardown.
#>
function Write-CheckFile([string]$Path, [string]$Text = 'independent fixture bytes') {
    $null = Assert-CleanupContainment $Path $scratch
    $null = [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path))
    [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding $false))
    if ([IO.Path]::GetFileName($Path) -eq '.git') { $script:CleanupFakeGitFiles.Add($Path) }
}

<#
 * Build a minimal independent source/preservation shape for the real pipeline.
 * @param Name Unique case name inside this test runner's scratch.
 * @return Canonical fixture root; no Git repository is initialized or mutated.
#>
function New-CleanupFixture([string]$Name) {
    $root = Join-Path $scratch $Name
    Write-CheckFile (Join-Path $root 'CMakeLists.txt') '# @see r2fc2-file-cleanup3.md; isolated source marker.'
    Write-CheckFile (Join-Path $root 'src/source.cpp') '// @file source.cpp @see r2fc2-file-cleanup3.md; independent source bytes.'
    Write-CheckFile (Join-Path $root 'src/tests/fixtures/cleanup/migration.json') '{"_documentation":"r2fc2-file-cleanup3.md27 fixture","schema_version":1,"original_path_count":0,"payloads":[]}'
    Write-CheckFile (Join-Path $root 'src/tests/fixtures/cleanup/source-epochs.json') '{"_documentation":"r2fc2-file-cleanup3.md27 fixture","payloads":[],"generated":[],"original_candidates":0}'
    Write-CheckFile (Join-Path $root 'src/tests/fixtures/multilayer-inputs.json') '{"_documentation":"r2fc2-file-cleanup3.md27 fixture","schema_version":2,"inputs":{}}'
    Write-CheckFile (Join-Path $root '.git') 'fixture git marker'
    return $root
}

<#
 * Supply only this fixture's declared tracking state, with no Git commands.
 * @param Repository Fixture root whose containment must still be explicit.
 * @param Arguments Structured read-only request made by the actual cleaner.
 * @return Declared fixture-relative tracked paths; never real repository mutations.
#>
function Invoke-CleanupGit([string]$Repository, [string[]]$Arguments) {
    $null = Assert-CleanupContainment $Repository $scratch
    if ($Arguments[-1] -ne 'ls-files') { throw 'Unexpected fixture Git request.' }
    return $script:CleanupTracked
}

<#
 * Supply explicit writer observations instead of examining unrelated real processes.
 * @param Filter Actual cleaner's structured process filter, retained as contract input.
 * @return Declared process facts, empty except in the active-writer test.
#>
function Get-CimInstance([string]$Filter) { return $script:CleanupWriterProcesses }

<#
 * Hash the entire fixture read-only to detect any dry-run writes or changes.
 * @param Root Exact disposable source root, including its fake Git marker.
 * @return Sorted path/hash strings compared independently before and after preview.
#>
function Get-CheckSnapshot([string]$Root) {
    # Get-CleanupFiles intentionally refuses Git markers; this independent test
    # enumerator reads its known synthetic marker too, and never removes files.
    return @(Get-ChildItem -LiteralPath $Root -File -Recurse -Force | Sort-Object FullName | ForEach-Object {
        '{0}|{1}' -f $_.FullName, (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    })
}

<#
 * Verify an exported alias against independently authored original fixture bytes.
 * @param Root Exact fixture checkout.
 * @param Original Original relative save/config path that cleanup removed.
 * @param Expected Independent original text, never taken from current export output.
 * @return Retained destination after matching both its bytes and recorded hash.
#>
function Assert-Exported([string]$Root, [string]$Original, [string]$Expected) {
    $journal = Get-Content -LiteralPath (Join-Path $Root 'src/tests/fixtures/cleanup/save-exports.json') -Raw | ConvertFrom-Json
    $reference = $journal.aliases.PSObject.Properties[$Original].Value
    $target = Join-Path $Root $reference.path
    Assert-Check ([IO.File]::ReadAllText($target) -eq $Expected) "exported original bytes: $Original"
    Assert-Check ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant() -eq $reference.sha256) "exported exact alias hash: $Original"
    return $target
}

<#
 * Remove only this runner's known synthetic markers, junction entries and scratch.
 * @return None; genuine repository/worktree metadata is never traversed or deleted.
#>
function Complete-CleanupChecks {
    # Junctions are known test-created entries. Delete each entry without following
    # its target, then recheck the whole ordinary scratch tree before recursion.
    foreach ($junction in $script:CleanupJunctions) {
        $null = Assert-CleanupContainment $junction $scratch
        if (Test-Path -LiteralPath $junction) { [IO.Directory]::Delete($junction) }
    }
    foreach ($marker in $script:CleanupFakeGitFiles) {
        if (-not (Test-Path -LiteralPath $marker)) { continue }
        Assert-Check ([IO.File]::ReadAllText($marker) -eq 'fixture git marker') 'teardown removes only its own fake Git marker'
        Remove-Item -LiteralPath $marker -Force
    }
    $null = Assert-CleanupContainment $scratch $scratchOwner
    $null = @(Get-CleanupFiles $scratch)
    Remove-Item -LiteralPath $scratch -Recurse -Force
}
