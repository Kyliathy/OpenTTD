#requires -Version 5.1
<#
.SYNOPSIS
Preview or apply verified development cleanup, exporting saves first.
.DESCRIPTION
@file clean.ps1 Repository-contained export and deletion boundary.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Groups24-28/33.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
Dry run is the default. Every candidate path is listed with its estimated
reclaimable space. Use -Apply to delete; -DryRun and -WhatIf override -Apply.

Selected owners are logs/ and test-cache/ payloads (keeping .gitignore routing
metadata), every mlm/runtime/ profile, generated
mlm/reviews/ children except README.md, audited build roots and source-backed
Python caches. Unknown root siblings, source, media, fixtures, savegames, !kit,
OpenTTD-baseline and Git metadata stay outside the deletion set.

Apply verifies durable inputs and exports discovered saves/native configs
through the existing Python preservation owner. Before each removal it rechecks
writers, tracked files, redirects, nested repositories, target types, complete
file snapshots and exact retained save identities. No process is terminated.
Unique/unclassified source material produces an explicit preservation refusal.
Partial failures stop further targets and report actual removed file lengths
and estimated distinct storage; outside hard links keep their allocation alive.
-KeepBuildIntermediates (alias -KeepBuild) retains complete audited build roots.

Stop games, test runners and builds before applying. Deletion is permanent.
All targets stay inside this checkout. Git operations are strictly read-only;
no git clean, staging, commits, resets or checkouts are performed.
.PARAMETER Apply
Delete selected output. Without this switch, only preview.
.PARAMETER DryRun
Explicit preview; overrides -Apply and changes no project files.
.PARAMETER KeepBuildIntermediates
Retain complete audited build roots, including applications and dependencies.
.PARAMETER ShowPaths
Also list every path when applying. Dry runs always list all paths.
.EXAMPLE
.\clean.ps1 -DryRun
Review all disposable development output and estimated space savings.
.EXAMPLE
.\clean.ps1 -Apply
Export protected inputs and remove the verified generated targets.
.EXAMPLE
.\clean.ps1 -Apply -KeepBuildIntermediates
Clear verified runs and diagnostics while retaining complete build trees.
#>
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [switch]$Apply,
    [switch]$DryRun,
    [Alias('KeepBuild')][switch]$KeepBuildIntermediates,
    [switch]$ShowPaths
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

<#
 * Resolve a target strictly within its explicitly authorized owner.
 * @param Path Candidate absolute/relative path.
 * @param Owner Intended enclosing directory.
 * @return Contained absolute path; equality and siblings refuse.
#>
function Assert-CleanupContainment([string]$Path, [string]$Owner) {
    # Compare whole components: OpenTTD-other must not count as OpenTTD/.
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $parent = [IO.Path]::GetFullPath($Owner).TrimEnd('\', '/')
    if (-not $full.StartsWith($parent + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Cleanup path escapes its allowed directory: $full (owner: $parent)"
    }
    return $full
}

<#
 * Refuse redirected ownership edges before inspecting or removing content.
 * @param Path Existing/future target whose ancestors must be ordinary directories.
 * @return None; a symlink or Windows reparse point throws.
#>
function Assert-CleanupNoLinks([string]$Path) {
    # FullPath does not resolve Windows junctions. Inspect existing ancestors.
    $cursor = [IO.Path]::GetFullPath($Path)
    while ($cursor) {
        if (Test-Path -LiteralPath $cursor) {
            $item = Get-Item -LiteralPath $cursor -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Cleanup refuses a symbolic link or junction: $cursor"
            }
        }
        $cursor = [IO.Path]::GetDirectoryName($cursor)
    }
}

<#
 * Enumerate one target without crossing redirects or nested repositories.
 * @param Path Exact existing target file/directory.
 * @return Regular file records; uncertain ownership throws before deletion.
#>
function Get-CleanupFiles([string]$Path) {
    # Inspect the whole tree before allowing any recursive removal.
    Assert-CleanupNoLinks $Path
    $item = Get-Item -LiteralPath $Path -Force
    if (-not $item.PSIsContainer) { return $item }
    $pending = New-Object 'System.Collections.Generic.Stack[string]'
    $pending.Push($item.FullName)
    while ($pending.Count -gt 0) {
        foreach ($child in Get-ChildItem -LiteralPath $pending.Pop() -Force) {
            if ($child.Name -eq '.git') { throw "Cleanup refuses a nested repository: $($child.FullName)" }
            if ($child.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Cleanup refuses a symbolic link or junction: $($child.FullName)"
            }
            if ($child.PSIsContainer) { $pending.Push($child.FullName) } else { $child }
        }
    }
}

<#
 * Run structured read-only Git discovery, never a Git mutation.
 * @param Repository Intended checkout root.
 * @param Arguments Read-only rev-parse/ls-files argument array.
 * @return Git output lines after successful completion.
#>
function Invoke-CleanupGit([string]$Repository, [string[]]$Arguments) {
    $result = @(& git -C $Repository @Arguments)
    if ($LASTEXITCODE -ne 0) { throw "Read-only Git check failed: git $($Arguments -join ' ')" }
    return $result
}

<#
 * Protect independent repositories/worktrees below the intended checkout.
 * @param Path Exact proposed target.
 * @param Repository Only checkout whose generated output may be removed.
 * @return None; a nested .git marker refuses ownership.
#>
function Assert-CleanupNoNestedRepository([string]$Path, [string]$Repository) {
    $cursor = Assert-CleanupContainment $Path $Repository
    # A parent worktree marker grants ownership to another Git checkout, so even
    # a familiar generated basename beneath it cannot become this cleaner's target.
    while ($cursor -ne $Repository) {
        if (Test-Path -LiteralPath (Join-Path $cursor '.git')) { throw "Cleanup refuses a nested repository: $cursor" }
        $cursor = [IO.Path]::GetDirectoryName($cursor)
    }
}

<#
 * Compile the read-only Windows allocation identity adapter once.
 * @return None; subsequent calls reuse the loaded metadata-only type.
#>
function Initialize-CleanupFileIdentity {
    if ('OttdCleanupFileIdentity' -as [type]) { return }
    # Read metadata only (access=0). Recognize hard links without hashing or
    # reading the gigabytes of retained executable/compiler data.
    Add-Type -TypeDefinition @"
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
/** Windows allocation metadata adapter; no file is opened for writing. */
public static class OttdCleanupFileIdentity {
    /** Native BY_HANDLE_FILE_INFORMATION layout, used for identity/links/length. */
    [StructLayout(LayoutKind.Sequential)]
    private struct Info {
        public uint Attributes;
        public System.Runtime.InteropServices.ComTypes.FILETIME Creation, Access, Write;
        public uint Volume, SizeHigh, SizeLow, Links, IndexHigh, IndexLow;
    }
    /** Stable volume/file index and its observed hard-link count. */
    public sealed class FileIdentity {
        public string Id;
        public uint Links;
    }
    /** @param path Existing file. @param access Metadata-only access. @param share Sharing mask. @param security Null descriptor. @param disposition Open existing. @param flags Ordinary file. @param template Null template. @return Owned metadata handle. */
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern SafeFileHandle CreateFile(string path, uint access, uint share, IntPtr security, uint disposition, uint flags, IntPtr template);
    /** @param handle Open metadata handle. @param info Native output record. @return Whether metadata was supplied. */
    [DllImport("kernel32.dll", SetLastError=true)]
    private static extern bool GetFileInformationByHandle(SafeFileHandle handle, out Info info);
    /** @param path Existing file to identify without reading bytes. @return Volume/file ID and observed link count. */
    public static FileIdentity Inspect(string path) {
        using (var handle = CreateFile(path, 0, 7, IntPtr.Zero, 3, 0x80, IntPtr.Zero)) {
            if (handle.IsInvalid) throw new Win32Exception(Marshal.GetLastWin32Error());
            Info info;
            if (!GetFileInformationByHandle(handle, out info)) throw new Win32Exception(Marshal.GetLastWin32Error());
            return new FileIdentity { Id=String.Format("{0:x8}:{1:x8}:{2:x8}",info.Volume,info.IndexHigh,info.IndexLow), Links=info.Links };
        }
    }
}
"@
}

<#
 * Require the same exact target allowlist used for planning and removal.
 * @param Path Absolute existing target to revalidate.
 * @param Repository Canonical intended checkout root.
 * @return None; protected paths or target-type changes refuse.
#>
function Assert-CleanupTarget([string]$Path, [string]$Repository) {
    $full = Assert-CleanupContainment $Path $Repository
    Assert-CleanupNoNestedRepository $full $Repository
    $relative = $full.Substring($Repository.Length + 1).Replace('\', '/')
    $directory = $relative -in @('logs', 'test-cache', 'mlm/runtime', 'build', 'build-probe', 'build-probe-tree')
    $review = $relative -match '^mlm/reviews/[^/]+$'
    $outputChild = $relative -match '^(logs|test-cache)/[^/]+$'
    $cache = $relative -match '^(src|mlm/tools|media/multilayer|cmake)(/[^/]+)*/__pycache__$'
    # Exact generated ownership cannot admit fixtures or the retained review
    # routing contract merely because their names resemble a disposable target.
    if ($relative -in @('mlm/reviews/README.md', 'logs/.gitignore', 'test-cache/.gitignore') -or $relative.StartsWith('src/tests/fixtures/')) {
        throw "Keep protected cleanup target: $full"
    }
    # Routing metadata is authored source even when its surrounding owner is
    # generated. Removing that parent would bypass the protected file policy;
    # only its individually checked payload children may be selected instead.
    if ($relative -in @('logs', 'test-cache') -and (Test-Path -LiteralPath (Join-Path $full '.gitignore'))) {
        throw "Keep protected cleanup target containing routing metadata: $full"
    }
    if (-not ($directory -or $review -or $cache -or $outputChild)) { throw "Not a disposable development target: $full" }
    Assert-CleanupNoLinks $full
    $item = Get-Item -LiteralPath $full -Force
    # Roots and caches are directories; review children may also be generated
    # files. A type swap must not broaden the previously admitted target.
    if (($directory -or $cache) -and -not $item.PSIsContainer) {
        throw "Cleanup target changed its file/directory type: $full"
    }
}

<#
 * Snapshot one audited target without changing project files.
 * @param Path Exact absolute target.
 * @param Kind Human-readable disposable category.
 * @param Repository Canonical intended checkout root.
 * @return Entry with its full file set, metadata snapshot and storage fields.
#>
function New-CleanupEntry([string]$Path, [string]$Kind, [string]$Repository) {
    Assert-CleanupTarget $Path $Repository
    $files = @(Get-CleanupFiles $Path | Sort-Object FullName)
    # New non-save files are valuable too; compare the entire tree before apply,
    # rather than allowing a preview to authorize content created after planning.
    $snapshot = @($files | ForEach-Object { '{0}|{1}|{2}' -f $_.FullName, $_.Length, $_.LastWriteTimeUtc.Ticks })
    [pscustomobject]@{ Path=$Path; Kind=$Kind; Directory=(Get-Item -LiteralPath $Path -Force).PSIsContainer
        Files=$files; Snapshot=$snapshot; LogicalBytes=0L; ReclaimableBytes=0L; Identities=@() }
}

<#
 * Enumerate exact generated owners and source-backed Python caches.
 * @param Repository Canonical intended checkout root.
 * @param KeepIntermediates Compatibility flag retaining complete audited build roots.
 * @return Target entries; unknown root siblings and protected owners stay outside.
#>
function Get-DevelopmentCleanupPlan([string]$Repository, [bool]$KeepIntermediates) {
    $wholeDirectories = @(
        @('logs', 'Diagnostics'),
        @('test-cache', 'Scratch and test output'),
        @('mlm/runtime', 'All runtime profiles')
    )
    if (-not $KeepIntermediates) {
        $wholeDirectories += @(@('build', 'Audited native build'), @('build-probe', 'Audited probe build'), @('build-probe-tree', 'Audited probe tree'))
    }
    # This is an exact root list, never a build* or ignored-file wildcard.
    foreach ($pair in $wholeDirectories) {
        $path = Join-Path $Repository $pair[0]
        Assert-CleanupNoLinks $path
        if (-not (Test-Path -LiteralPath $path)) { continue }
        # Keep the tracked routing marker while still retiring every generated
        # payload. Arbitrary tracked children remain targets and refuse through
        # the existing Git guard; this exception protects only .gitignore.
        if ($pair[0] -in @('logs', 'test-cache') -and (Test-Path -LiteralPath (Join-Path $path '.gitignore'))) {
            foreach ($child in Get-ChildItem -LiteralPath $path -Force) {
                if ($child.Name -ne '.gitignore') { New-CleanupEntry $child.FullName $pair[1] $Repository }
            }
        } else {
            New-CleanupEntry $path $pair[1] $Repository
        }
    }
    $reviews = Join-Path $Repository 'mlm/reviews'
    Assert-CleanupNoLinks $reviews
    if (Test-Path -LiteralPath $reviews -PathType Container) {
        foreach ($child in Get-ChildItem -LiteralPath $reviews -Force) {
            if ($child.Name -ne 'README.md') { New-CleanupEntry $child.FullName 'Generated reviews' $Repository }
        }
    }
    # Classify a cache only when every file has its source counterpart. Never
    # traverse fixture originals, other repositories or redirected source trees.
    foreach ($relative in @('src', 'mlm/tools', 'media/multilayer', 'cmake')) {
        $owner = Join-Path $Repository $relative
        if (-not (Test-Path -LiteralPath $owner -PathType Container)) { continue }
        $pending = New-Object 'System.Collections.Generic.Stack[string]'
        $pending.Push($owner)
        while ($pending.Count -gt 0) {
            $current = $pending.Pop()
            Assert-CleanupNoLinks $current
            if (Test-Path -LiteralPath (Join-Path $current '.git')) { continue }
            foreach ($child in Get-ChildItem -LiteralPath $current -Directory -Force) {
                $name = $child.FullName.Substring($Repository.Length + 1).Replace('\', '/')
                if ($name -eq 'src/tests/fixtures') { continue }
                if ($child.Name -ne '__pycache__') { $pending.Push($child.FullName); continue }
                $classified = $true
                foreach ($file in Get-CleanupFiles $child.FullName) {
                    # Source-less bytecode may be unique. Keep its whole cache
                    # rather than infer that a familiar directory name is enough.
                    if ($file.Name -notmatch '^(.+)\.cpython-[0-9]+(?:\.opt-[0-9]+)?\.pyc$') { $classified = $false; break }
                    $source = Join-Path $current ($Matches[1] + '.py')
                    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { $classified = $false; break }
                }
                if ($classified) { New-CleanupEntry $child.FullName 'Source-backed Python cache' $Repository }
            }
        }
    }
}

<#
 * Account for logical bytes and distinct allocations while preserving outside links.
 * @param Plan Entries with enumerated files or previously recorded allocation facts.
 * @param ReuseIdentities Use saved facts after paths have disappeared.
 * @return None; updates storage estimates on the supplied entries.
#>
function Set-CleanupEstimates([object[]]$Plan, [switch]$ReuseIdentities) {
    Initialize-CleanupFileIdentity
    $allocations = @{}
    # Count all selected directory entries, then free an allocation only when
    # every hard link belongs to the actually removed set. Outside links survive.
    foreach ($entry in $Plan) {
        $entry.ReclaimableBytes = 0L
        if (-not $ReuseIdentities) {
            $entry.Identities = @($entry.Files | ForEach-Object {
                $info = [OttdCleanupFileIdentity]::Inspect($_.FullName)
                [pscustomobject]@{ Path=$_.FullName; Id=$info.Id; Links=$info.Links; Bytes=[long]$_.Length }
            })
            $entry.LogicalBytes = 0L
            foreach ($file in $entry.Identities) { $entry.LogicalBytes += $file.Bytes }
        }
        foreach ($file in $entry.Identities) {
            if (-not $allocations.ContainsKey($file.Id)) {
                $allocations[$file.Id] = [pscustomobject]@{ Count=0; Links=$file.Links; Bytes=$file.Bytes; LastEntry=$entry }
            }
            $allocation = $allocations[$file.Id]
            $allocation.Count++
            $allocation.LastEntry = $entry
        }
    }
    foreach ($allocation in $allocations.Values) {
        if ($allocation.Count -eq $allocation.Links) { $allocation.LastEntry.ReclaimableBytes += $allocation.Bytes }
    }
}

<#
 * Refuse every tracked file in the proposed deletion set.
 * @param Plan Fully enumerated target entries.
 * @param TrackedPaths Fresh absolute file paths from read-only Git discovery.
 * @return None; a tracked candidate prevents removal.
#>
function Assert-CleanupUntracked([object[]]$Plan, [string[]]$TrackedPaths) {
    $tracked = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($path in $TrackedPaths) { [void]$tracked.Add($path.Replace('/', '\')) }
    # Tracking is an independent safety boundary: one tracked descendant blocks
    # its entire recursive target, regardless of generated-location conventions.
    foreach ($entry in $Plan) {
        foreach ($file in $entry.Files) {
            if ($tracked.Contains($file.FullName.Replace('/', '\'))) { throw "Keep tracked file: $($file.FullName)" }
        }
    }
}

<#
 * Refuse relevant active writers without terminating any process.
 * @param Repository Canonical checkout matched against process images/arguments.
 * @return None; identified or ambiguous relative OpenTTD writers block apply.
#>
function Assert-CleanupStopped([string]$Repository) {
    $filter = "Name LIKE 'openttd%' OR Name='MSBuild.exe' OR Name='cl.exe' OR Name='link.exe' OR Name='cmake.exe' OR Name LIKE 'python%' OR Name='powershell.exe' OR Name='pwsh.exe' OR Name='grfcodec.exe' OR Name='nforenum.exe'"
    # Unrelated processes remain untouched. Relative OpenTTD test commands lack
    # reliable working-directory identity and are conservatively ambiguous.
    foreach ($process in Get-CimInstance Win32_Process -Filter $filter) {
        if ($process.ProcessId -eq $PID) { continue }
        $image = [string]$process.ExecutablePath
        $command = [string]$process.CommandLine
        $related = $image.StartsWith($Repository + '\', [StringComparison]::OrdinalIgnoreCase)
        $related = $related -or $command.IndexOf($Repository, [StringComparison]::OrdinalIgnoreCase) -ge 0
        $relativeTest = $command -match '(src[/\\]tests[/\\]|mlm[/\\]tools[/\\]|openttd[^ /\\]*\.vcxproj|openttd\.sln)'
        # An inaccessible game's image cannot establish that it belongs elsewhere;
        # preserve its possible saves until ownership can be established.
        if ($process.Name -like 'openttd*' -and -not $image) { $related = $true }
        if ($related -or $relativeTest) {
            throw "Stop this checkout's games/tests/builds before cleanup (PID $($process.ProcessId), $($process.Name))."
        }
    }
}

<#
 * Format one byte count without changing its storage meaning.
 * @param Bytes Logical or estimated distinct bytes.
 * @return Human-readable size with binary units.
#>
function Format-CleanupSize([long]$Bytes) {
    if ($Bytes -ge 1GB) { return ('{0:N2} GiB' -f ($Bytes / 1GB)) }
    if ($Bytes -ge 1MB) { return ('{0:N2} MiB' -f ($Bytes / 1MB)) }
    if ($Bytes -ge 1KB) { return ('{0:N2} KiB' -f ($Bytes / 1KB)) }
    return "$Bytes B"
}

<#
 * Display every absolute target and both storage estimates.
 * @param Plan Fully validated preview/apply entries.
 * @return None; writes display output only.
#>
function Show-CleanupPaths([object[]]$Plan) {
    Write-Host ''
    Write-Host 'PATHS TO DELETE: estimated distinct data freed; sum of file lengths; full path'
    foreach ($entry in $Plan) {
        Write-Host ('{0,12} freed | {1,12} file lengths | {2}' -f (Format-CleanupSize $entry.ReclaimableBytes), (Format-CleanupSize $entry.LogicalBytes), $entry.Path)
    }
    Write-Host 'Hard-linked data is counted once, only when all its links are selected for deletion.'
}

<#
 * Call the existing preservation owner using structured JSON and arguments.
 * @param Operation Read-only plan, explicit export apply, or final target verify.
 * @param Request Exact scopes/plan; no path becomes executable shell text.
 * @param Repository Explicit checkout containing the independently audited targets.
 * @return Decoded preservation proposal or verification facts; no scratch files.
#>
function Invoke-CleanupPreservation([string]$Operation, [object]$Request, [string]$Repository) {
    $helper = Join-Path $PSScriptRoot 'src/tests/multilayer/support/artifact_preservation.py'
    $json = ConvertTo-Json -InputObject $Request -Depth 40 -Compress
    $previousEncoding = $OutputEncoding
    try {
        $OutputEncoding = New-Object Text.UTF8Encoding $false
        $previousPreference = $ErrorActionPreference
        try {
            # Capture the complete structured child's refusal instead of letting
            # PowerShell stop at the first stderr line and hide its actual cause.
            $ErrorActionPreference = 'Continue'
            $lines = @($json | & python -B $helper --cleanup $Operation --root $Repository 2>&1)
            $code = $LASTEXITCODE
        } finally { $ErrorActionPreference = $previousPreference }
        if ($code -ne 0) { throw ("Preservation $Operation refused (exit $code): " + ($lines -join "`n")) }
        return (($lines -join "`n") | ConvertFrom-Json)
    } finally { $OutputEncoding = $previousEncoding }
}

<#
 * Require the exact observed target type, file set, metadata and allocations.
 * @param Entry Original target and its complete immutable snapshot.
 * @param Repository Canonical checkout rechecked immediately before removal.
 * @return None; changed/new/missing files require refreshed planning and export.
#>
function Assert-CleanupEntryCurrent([object]$Entry, [string]$Repository) {
    Assert-CleanupTarget $Entry.Path $Repository
    $item = Get-Item -LiteralPath $Entry.Path -Force
    if ($item.PSIsContainer -ne $Entry.Directory) { throw 'Cleanup target changed its file/directory type.' }
    $current = @(Get-CleanupFiles $Entry.Path | Sort-Object FullName)
    $snapshot = @($current | ForEach-Object { '{0}|{1}|{2}' -f $_.FullName, $_.Length, $_.LastWriteTimeUtc.Ticks })
    # Equal path sets can hide replaced files. Compare metadata and the original
    # volume/file identities so a copied timestamp cannot conceal a type swap.
    if (($snapshot.Count -ne $Entry.Snapshot.Count) -or (($snapshot -join "`n") -cne ($Entry.Snapshot -join "`n"))) {
        throw "Cleanup target files changed; refresh/export before deletion: $($Entry.Path)"
    }
    for ($index = 0; $index -lt $current.Count; $index++) {
        if ([OttdCleanupFileIdentity]::Inspect($current[$index].FullName).Id -ne $Entry.Identities[$index].Id) {
            throw "Cleanup file identity changed: $($current[$index].FullName)"
        }
    }
}

<#
 * Remove only the exact target whose final ownership/snapshot still agrees.
 * @param Entry Audited target after committed export and stopped-writer checks.
 * @param Repository Canonical checkout; no operation crosses to another shell.
 * @return None; native PowerShell errors are retained as partial-failure facts.
#>
function Remove-CleanupTarget([object]$Entry, [string]$Repository) {
    Assert-CleanupEntryCurrent $Entry $Repository
    Remove-Item -LiteralPath $Entry.Path -Recurse -Force -ErrorAction Stop
}

<#
 * Run the sole preview/export/delete pipeline on an explicitly selected checkout.
 * @param Repository Intended checkout, additionally verified by the normal CLI.
 * @param Apply Explicit export/deletion request; defaults to preview.
 * @param DryRun Force preview, even when Apply is present.
 * @param KeepBuild Retain complete audited build roots.
 * @param ShowPaths Display every absolute target during apply too.
 * @return Completed/failed targets and logical/distinct storage report.
#>
function Invoke-DevelopmentCleanup {
    [CmdletBinding(SupportsShouldProcess=$true, ConfirmImpact='Medium')]
    param([string]$Repository, [switch]$Apply, [switch]$DryRun, [switch]$KeepBuild, [switch]$ShowPaths)
    Assert-CleanupNoLinks $Repository
    $plan = @(Get-DevelopmentCleanupPlan $Repository $KeepBuild.IsPresent)
    if ($plan.Count -eq 0) {
        Write-Host 'Nothing to clean.'
        return [pscustomobject]@{ Preview=(-not $Apply); Completed=@(); Failed=@(); LogicalBytes=0L; ReclaimableBytes=0L }
    }
    $tracked = @(Invoke-CleanupGit $Repository @('-c', 'core.quotepath=false', 'ls-files') | ForEach-Object { Join-Path $Repository $_ })
    Assert-CleanupUntracked $plan $tracked
    Set-CleanupEstimates $plan
    $scopes = @($plan | ForEach-Object { $_.Path.Substring($Repository.Length + 1).Replace('\', '/') })
    $proposal = Invoke-CleanupPreservation 'plan' ([pscustomobject]@{ scopes=$scopes }) $Repository
    # Explicit DryRun/WhatIf always wins over Apply. Preview creates neither save
    # archives nor project directories and never starts a native game process.
    $preview = -not $Apply -or $DryRun -or $WhatIfPreference
    if ($preview -or $ShowPaths) { Show-CleanupPaths $plan }
    Write-Host ("Protected candidates: {0}; required-input refusals: {1}." -f @($proposal.plan.observations).Count, $proposal.retained_inputs.unresolved_required_inputs)
    if ($proposal.retained_inputs.unresolved_required_inputs -ne 0) {
        Write-Host ("PRESERVATION REFUSAL: " + $proposal.retained_inputs.refusal)
        if (-not $preview) { throw 'Resolve required retained inputs before cleanup.' }
    }
    if ($preview) {
        Write-Host 'PREVIEW ONLY. Use -Apply without -DryRun/-WhatIf to export and remove verified targets.'
        return [pscustomobject]@{ Preview=$true; Completed=@(); Failed=@(); Plan=$plan; Preservation=$proposal }
    }
    Assert-CleanupStopped $Repository
    foreach ($entry in $plan) { Assert-CleanupEntryCurrent $entry $Repository }
    $exports = Invoke-CleanupPreservation 'apply' ([pscustomobject]@{ plan=$proposal.plan }) $Repository
    $completed = New-Object 'System.Collections.Generic.List[object]'
    $failed = New-Object 'System.Collections.Generic.List[object]'
    $removed = New-Object 'System.Collections.Generic.List[object]'
    # Each removal receives fresh writer, Git, redirect, type and save checks.
    # Stop after a partial failure and report actual removed files, not promises.
    foreach ($entry in $plan) {
        if (-not $PSCmdlet.ShouldProcess($entry.Path, 'Remove verified generated output after save export')) { continue }
        try {
            Assert-CleanupEntryCurrent $entry $Repository
            $null = Invoke-CleanupPreservation 'verify' ([pscustomobject]@{ plan=$proposal.plan; scope=$entry.Path }) $Repository
            Assert-CleanupStopped $Repository
            $trackedNow = @(Invoke-CleanupGit $Repository @('-c', 'core.quotepath=false', 'ls-files') | ForEach-Object { Join-Path $Repository $_ })
            Assert-CleanupUntracked @($entry) $trackedNow
            Remove-CleanupTarget $entry $Repository
            $completed.Add($entry.Path)
        } catch {
            $failed.Add([pscustomobject]@{ Path=$entry.Path; Error=$_.Exception.Message })
        } finally {
            $identities = @($entry.Identities | Where-Object { -not (Test-Path -LiteralPath $_.Path) })
            $removed.Add([pscustomobject]@{ Identities=$identities; LogicalBytes=0L; ReclaimableBytes=0L })
            foreach ($identity in $identities) { $removed[$removed.Count - 1].LogicalBytes += $identity.Bytes }
        }
        if ($failed.Count -gt 0) { break }
    }
    Set-CleanupEstimates @($removed.ToArray()) -ReuseIdentities
    $logical = 0L; $distinct = 0L
    foreach ($entry in $removed) { $logical += $entry.LogicalBytes; $distinct += $entry.ReclaimableBytes }
    Write-Host ("Completed targets: {0}; failed: {1}; removed lengths: {2}; estimated distinct freed: {3}." -f $completed.Count, $failed.Count, (Format-CleanupSize $logical), (Format-CleanupSize $distinct))
    foreach ($failure in $failed) { Write-Host ("FAILED: {0}: {1}" -f $failure.Path, $failure.Error) }
    return [pscustomobject]@{ Preview=$false; Completed=@($completed.ToArray()); Failed=@($failed.ToArray()); LogicalBytes=$logical; ReclaimableBytes=$distinct; Exports=$exports }
}

# Loading functions for fixture tests never executes real cleanup.
if ($MyInvocation.InvocationName -eq '.') { return }
$repository = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\', '/')
Assert-CleanupNoLinks $repository
$gitRoot = @(Invoke-CleanupGit $repository @('rev-parse', '--show-toplevel'))
# Root agreement and real source markers keep a sibling, parent repository or
# arbitrary directory from being mistaken for this checkout's generated owners.
if ($gitRoot.Count -ne 1 -or [IO.Path]::GetFullPath($gitRoot[0]).TrimEnd('\', '/') -ne $repository -or
    -not (Test-Path -LiteralPath (Join-Path $repository 'src') -PathType Container) -or
    -not (Test-Path -LiteralPath (Join-Path $repository 'CMakeLists.txt') -PathType Leaf)) {
    throw 'Keep clean.ps1 at the root of the intended OpenTTD repository.'
}
$result = Invoke-DevelopmentCleanup -Repository $repository -Apply:$Apply -DryRun:$DryRun -KeepBuild:$KeepBuildIntermediates -ShowPaths:$ShowPaths -WhatIf:$WhatIfPreference
if (@($result.Failed).Count -gt 0) { throw 'Cleanup incomplete; see the completed/failed target report above.' }
