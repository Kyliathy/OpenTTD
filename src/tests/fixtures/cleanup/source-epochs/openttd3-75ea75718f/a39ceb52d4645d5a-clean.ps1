#requires -Version 5.1
<#
.SYNOPSIS
Preview or remove disposable OpenTTD development output.
.DESCRIPTION
Dry run is the default. Every candidate path is listed with its estimated
reclaimable space. Use -Apply to delete; -DryRun and -WhatIf override -Apply.

DEFAULT CLEANUP:
- All old mlm/runtime/native/ process homes, including executable copies.
  Historical report paths/hashes do not require retaining runnable binaries.
- Stale runtime invocation markers and routing-check scratch directories.
- build-probe/ and build-probe-tree/: ignored experimental source/build trees.
- Main build's compiler-intermediate directories and x64/ build state.
- Generated PDB/ILK/OBJ/PCH/LIB/EXP files directly in build configurations.
- Reproducible acceptance/data-home and demo-guide-load-N review outputs.
- Raw review PPM captures when a PNG with the same basename is present.

KEPT:
- Git metadata, tracked files, source, fixtures, source media and !kit/.
- OpenTTD-baseline/, user saves/configs, and the main build's game/test
  executables, DLLs, language/base assets, CMake configuration and dependencies.
- Other review runs: some contain the only saved inputs for unfinished work
  or source/packed-art validators. Original playable demos remain in fixtures.
- Logs and PNG captures. Old proof records may refer to deleted generated
  output; fresh verification must regenerate that output.

Deleting intermediates/debug symbols does not prevent playing the existing
build. The next compile regenerates them and takes longer; debugger symbols
return after rebuilding. -KeepBuildIntermediates skips that part of cleanup.

Space reporting counts each Windows file ID once. The old native runtimes
contain hard links, so their apparent 9.78 GiB is about 1.99 GiB of distinct
data. Links outside the deletion set keep their allocation alive.
Allocated disk space can differ from file lengths; -Apply also reports the
observed drive free-space change, including unrelated drive activity.

Stop games, test runners and builds before applying. Deletion is permanent.
All targets stay inside this checkout. Git operations are strictly read-only;
no git clean, staging, commits, resets or checkouts are performed.
.PARAMETER Apply
Delete selected output. Without this switch, only preview.
.PARAMETER DryRun
Explicit preview; overrides -Apply and changes no project files.
.PARAMETER KeepBuildIntermediates
Retain main build compiler intermediates and debug symbols for faster rebuilds.
.PARAMETER ShowPaths
Also list every path when applying. Dry runs always list all paths.
.EXAMPLE
.\clean.ps1 -DryRun
Review all disposable development output and estimated space savings.
.EXAMPLE
.\clean.ps1 -Apply
Delete the reviewed output while preserving the playable main build.
.EXAMPLE
.\clean.ps1 -Apply -KeepBuildIntermediates
Clear old runs, probe builds and redundant reviews; keep compiler caches.
#>
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [switch]$Apply,
    [switch]$DryRun,
    [switch]$KeepBuildIntermediates,
    [switch]$ShowPaths
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-CleanupContainment([string]$Path, [string]$Owner) {
    # Compare whole components: OpenTTD-other must not count as OpenTTD/.
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $parent = [IO.Path]::GetFullPath($Owner).TrimEnd('\', '/')
    if (-not $full.StartsWith($parent + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Cleanup path escapes its allowed directory: $full (owner: $parent)"
    }
    return $full
}

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

function Invoke-CleanupGit([string]$Repository, [string[]]$Arguments) {
    $result = @(& git -C $Repository @Arguments)
    if ($LASTEXITCODE -ne 0) { throw "Read-only Git check failed: git $($Arguments -join ' ')" }
    return $result
}

function Assert-CleanupNoNestedRepository([string]$Path, [string]$Repository) {
    $cursor = Assert-CleanupContainment $Path $Repository
    while ($cursor -ne $Repository) {
        if (Test-Path -LiteralPath (Join-Path $cursor '.git')) { throw "Cleanup refuses a nested repository: $cursor" }
        $cursor = [IO.Path]::GetDirectoryName($cursor)
    }
}

function Initialize-CleanupFileIdentity {
    if ('OttdCleanupFileIdentity' -as [type]) { return }
    # Read metadata only (access=0). Recognize hard links without hashing or
    # reading the gigabytes of retained executable/compiler data.
    Add-Type -TypeDefinition @"
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
public static class OttdCleanupFileIdentity {
    [StructLayout(LayoutKind.Sequential)]
    private struct Info {
        public uint Attributes;
        public System.Runtime.InteropServices.ComTypes.FILETIME Creation, Access, Write;
        public uint Volume, SizeHigh, SizeLow, Links, IndexHigh, IndexLow;
    }
    public sealed class FileIdentity {
        public string Id;
        public uint Links;
    }
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern SafeFileHandle CreateFile(string path, uint access, uint share, IntPtr security, uint disposition, uint flags, IntPtr template);
    [DllImport("kernel32.dll", SetLastError=true)]
    private static extern bool GetFileInformationByHandle(SafeFileHandle handle, out Info info);
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

function Assert-CleanupTarget([string]$Path, [string]$Repository) {
    $full = Assert-CleanupContainment $Path $Repository
    Assert-CleanupNoNestedRepository $full $Repository
    $relative = $full.Substring($Repository.Length + 1).Replace('\', '/')
    # Explicit targets only. Never blanket-delete build*, reviews, logs or saves.
    $directory = $relative -match '^(mlm/runtime/(native|\.runs|routing-checks)|build-probe(-tree)?|build/(openttd|openttd_lib|openttd_test)\.dir|build/x64|mlm/reviews/acceptance/(data-home|demo-guide-load-[0-9]+))$'
    $buildFile = $relative -match '^build/(Debug|RelWithDebInfo|Release|MinSizeRel)/[^/]+\.(pdb|ilk|obj|pch|lib|exp)$'
    $rawCapture = $relative -match '^mlm/reviews/.+\.ppm$'
    if (-not ($directory -or $buildFile -or $rawCapture)) { throw "Not a disposable development target: $full" }
    Assert-CleanupNoLinks $full
    $item = Get-Item -LiteralPath $full -Force
    if ($item.PSIsContainer -ne $directory) { throw "Cleanup target changed its file/directory type: $full" }
    if ($rawCapture) {
        $png = [IO.Path]::ChangeExtension($full, '.png')
        Assert-CleanupNoLinks $png
        if (-not (Test-Path -LiteralPath $png -PathType Leaf)) { throw "Keep raw capture without a PNG counterpart: $full" }
    }
}

function New-CleanupEntry([string]$Path, [string]$Kind, [string]$Repository) {
    Assert-CleanupTarget $Path $Repository
    $files = @(Get-CleanupFiles $Path)
    [pscustomobject]@{ Path=$Path; Kind=$Kind; Directory=(Get-Item -LiteralPath $Path -Force).PSIsContainer
        Files=$files; LogicalBytes=0L; ReclaimableBytes=0L; Identities=@() }
}

function Get-DevelopmentCleanupPlan([string]$Repository, [bool]$KeepIntermediates) {
    $wholeDirectories = @(
        @('mlm/runtime/native', 'Old native runtimes'),
        @('mlm/runtime/.runs', 'Old invocation markers'),
        @('mlm/runtime/routing-checks', 'Routing-check scratch'),
        @('build-probe', 'Scratch probe builds'),
        @('build-probe-tree', 'Scratch probe builds')
    )
    if (-not $KeepIntermediates) {
        foreach ($name in @('openttd.dir', 'openttd_lib.dir', 'openttd_test.dir', 'x64')) {
            $wholeDirectories += ,@("build/$name", 'Compiler intermediates')
        }
    }
    $acceptance = Join-Path $Repository 'mlm/reviews/acceptance'
    $reviewDirectories = @()
    Assert-CleanupNoLinks $acceptance
    if (Test-Path -LiteralPath $acceptance -PathType Container) {
        foreach ($child in Get-ChildItem -LiteralPath $acceptance -Directory -Force) {
            if ($child.Name -eq 'data-home' -or $child.Name -match '^demo-guide-load-[0-9]+$') {
                $relative = $child.FullName.Substring($Repository.Length + 1)
                $wholeDirectories += ,@($relative, 'Reproducible acceptance output')
                $reviewDirectories += $child.FullName
            }
        }
    }
    foreach ($pair in $wholeDirectories) {
        $path = Join-Path $Repository $pair[0]
        Assert-CleanupNoLinks $path
        if (Test-Path -LiteralPath $path) { New-CleanupEntry $path $pair[1] $Repository }
    }
    if (-not $KeepIntermediates) {
        foreach ($configuration in @('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')) {
            $path = Join-Path $Repository "build/$configuration"
            Assert-CleanupNoLinks $path
            if (-not (Test-Path -LiteralPath $path -PathType Container)) { continue }
            foreach ($file in Get-ChildItem -LiteralPath $path -File -Force) {
                if ($file.Extension -in @('.pdb', '.ilk', '.obj', '.pch', '.lib', '.exp')) {
                    New-CleanupEntry $file.FullName 'Generated symbols/linker output' $Repository
                }
            }
        }
    }
    $reviews = Join-Path $Repository 'mlm/reviews'
    if (Test-Path -LiteralPath $reviews -PathType Container) {
        foreach ($file in Get-CleanupFiles $reviews) {
            if ($file.Extension -ne '.ppm') { continue }
            $covered = @($reviewDirectories | Where-Object { $file.FullName.StartsWith($_ + '\', [StringComparison]::OrdinalIgnoreCase) }).Count -gt 0
            if (-not $covered -and (Test-Path -LiteralPath ([IO.Path]::ChangeExtension($file.FullName, '.png')) -PathType Leaf)) {
                New-CleanupEntry $file.FullName 'Raw captures with PNG copies' $Repository
            }
        }
    }
}

function Set-CleanupEstimates([object[]]$Plan, [switch]$ReuseIdentities) {
    Initialize-CleanupFileIdentity
    $allocations = @{}
    foreach ($entry in $Plan) {
        $entry.ReclaimableBytes = 0L
        if (-not $ReuseIdentities) {
            $entry.Identities = @($entry.Files | ForEach-Object {
                $info = [OttdCleanupFileIdentity]::Inspect($_.FullName)
                [pscustomobject]@{ Id=$info.Id; Links=$info.Links; Bytes=[long]$_.Length }
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

function Assert-CleanupUntracked([object[]]$Plan, [string[]]$TrackedPaths) {
    $tracked = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($path in $TrackedPaths) { [void]$tracked.Add($path.Replace('/', '\')) }
    foreach ($entry in $Plan) {
        foreach ($file in $entry.Files) {
            if ($tracked.Contains($file.FullName.Replace('/', '\'))) { throw "Keep tracked file: $($file.FullName)" }
        }
    }
}

function Assert-CleanupStopped([string]$Repository) {
    foreach ($process in Get-CimInstance Win32_Process -Filter "Name LIKE 'openttd%' OR Name='MSBuild.exe' OR Name='cl.exe' OR Name='link.exe' OR Name='cmake.exe'") {
        $path = $process.ExecutablePath
        # Relative build arguments do not identify a working directory reliably.
        $buildTool = $process.Name -in @('MSBuild.exe', 'cl.exe', 'link.exe', 'cmake.exe')
        if ($buildTool -or -not $path -or $path.StartsWith($Repository + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "Stop games, tests and builds before cleanup (PID $($process.ProcessId), $($process.Name))."
        }
    }
}

function Format-CleanupSize([long]$Bytes) {
    if ($Bytes -ge 1GB) { return ('{0:N2} GiB' -f ($Bytes / 1GB)) }
    if ($Bytes -ge 1MB) { return ('{0:N2} MiB' -f ($Bytes / 1MB)) }
    if ($Bytes -ge 1KB) { return ('{0:N2} KiB' -f ($Bytes / 1KB)) }
    return "$Bytes B"
}

function Show-CleanupPaths([object[]]$Plan) {
    Write-Host ''
    Write-Host 'PATHS TO DELETE: estimated distinct data freed; sum of file lengths; full path'
    foreach ($entry in $Plan) {
        Write-Host ('{0,12} freed | {1,12} file lengths | {2}' -f (Format-CleanupSize $entry.ReclaimableBytes), (Format-CleanupSize $entry.LogicalBytes), $entry.Path)
    }
    Write-Host 'Hard-linked data is counted once, only when all its links are selected for deletion.'
}

# Loading functions for fixture tests never executes real cleanup.
if ($MyInvocation.InvocationName -eq '.') { return }
$repository = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\', '/')
Assert-CleanupNoLinks $repository
$gitRoot = @(Invoke-CleanupGit $repository @('rev-parse', '--show-toplevel'))
if ($gitRoot.Count -ne 1 -or [IO.Path]::GetFullPath($gitRoot[0]).TrimEnd('\', '/') -ne $repository -or
    -not (Test-Path -LiteralPath (Join-Path $repository 'src') -PathType Container) -or
    -not (Test-Path -LiteralPath (Join-Path $repository 'CMakeLists.txt') -PathType Leaf)) {
    throw 'Keep clean.ps1 at the root of the intended OpenTTD repository.'
}
Write-Host 'Inspecting disposable runtime, review and build output...'
$plan = @(Get-DevelopmentCleanupPlan $repository $KeepBuildIntermediates.IsPresent)
if ($plan.Count -eq 0) { Write-Host 'Nothing to clean.'; return }
$tracked = @(Invoke-CleanupGit $repository @('-c', 'core.quotepath=false', 'ls-files') | ForEach-Object { Join-Path $repository $_ })
Assert-CleanupUntracked $plan $tracked
Write-Host 'Checking file identities to account for shared hard-link storage...'
Set-CleanupEstimates $plan
$plan | Group-Object Kind | ForEach-Object {
    [pscustomobject]@{ Category=$_.Name; Targets=$_.Count
        LogicalGiB=[Math]::Round(($_.Group | Measure-Object LogicalBytes -Sum).Sum / 1GB, 3)
        ReclaimableGiB=[Math]::Round(($_.Group | Measure-Object ReclaimableBytes -Sum).Sum / 1GB, 3) }
} | Format-Table -AutoSize | Out-Host
$total = ($plan | Measure-Object ReclaimableBytes -Sum).Sum
Write-Host ('Estimated distinct file data reclaimable: {0:N2} GiB (actual allocated space may differ).' -f ($total / 1GB))
$preview = -not $Apply -or $DryRun -or $WhatIfPreference
if ($preview -or $ShowPaths) { Show-CleanupPaths $plan }
if ($preview) {
    Write-Host 'PREVIEW ONLY. To delete these targets, use -Apply without -DryRun/-WhatIf.'
    return
}
Assert-CleanupStopped $repository
$drive = New-Object IO.DriveInfo ([IO.Path]::GetPathRoot($repository))
$freeBefore = $drive.AvailableFreeSpace
$completed = New-Object 'System.Collections.Generic.List[object]'
foreach ($entry in $plan) {
    if (-not $PSCmdlet.ShouldProcess($entry.Path, 'Permanently delete disposable development output')) { continue }
    # Recheck the absolute target and its entire tree immediately before removal.
    Assert-CleanupTarget $entry.Path $repository
    $currentFiles = @(Get-CleanupFiles $entry.Path)
    Assert-CleanupUntracked @([pscustomobject]@{ Files=$currentFiles }) $tracked
    Remove-Item -LiteralPath $entry.Path -Recurse -Force
    $completed.Add($entry)
}
# Recalculate against entries actually removed, including any -Confirm skips.
Set-CleanupEstimates @($completed.ToArray()) -ReuseIdentities
$freed = 0L; $logical = 0L
foreach ($entry in $completed) { $freed += $entry.ReclaimableBytes; $logical += $entry.LogicalBytes }
$freeAfter = $drive.AvailableFreeSpace
Write-Host ('Deleted file lengths: {0:N2} GiB; estimated distinct file data freed: {1:N2} GiB.' -f ($logical / 1GB), ($freed / 1GB))
Write-Host ('Observed drive free-space change: {0:N2} GiB (also includes unrelated drive activity).' -f (($freeAfter - $freeBefore) / 1GB))
Write-Host 'The playable main build, sources, fixtures, saves/configs, dependencies and local baseline archive were preserved.'
