<#
/** @file compress-runtime-languages.ps1 Recover native test headroom while preserving every historical byte and path.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CB/15CC.
 * Only completed20261018native homes inside this Git repository are eligible.
 * Native NTFS compression is reversible; all language hashes are checked before/after.
 */
#>
$ErrorActionPreference='Stop'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (-not (Test-Path -LiteralPath (Join-Path $repository '.git'))) { throw 'Expected workspace Git repository' }
$runtime=[IO.Path]::GetFullPath((Join-Path $repository 'mlm/runtime/native'))
$initial=(Get-PSDrive -Name D).Free
$records=New-Object 'System.Collections.Generic.List[object]'
$targetFree=8GB
# Completed old homes cannot be the currently executing20261019 producer.
# Restrict the operation to copied language packs; configs, scripts, saves,
# executables, evidence and current build inputs retain all their bytes/paths.
foreach ($nativeHome in Get-ChildItem -LiteralPath $runtime -Directory) {
    if (-not $nativeHome.Name.StartsWith('20261018-')) { continue }
    $language=Join-Path $nativeHome.FullName 'main/lang'
    if (-not (Test-Path -LiteralPath $language)) { continue }
    $absolute=[IO.Path]::GetFullPath($language)
    if (-not $absolute.StartsWith($runtime+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Language target escaped native runtime' }
    # Junctions or copied nested repositories cannot expand this exact old
    # runtime boundary into user directories or another repository.
    foreach ($parent in @($runtime,$nativeHome.FullName,(Join-Path $nativeHome.FullName 'main'),$language)) {
        if ((Get-Item -LiteralPath $parent).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Refusing runtime link' }
        if (Test-Path -LiteralPath (Join-Path $parent '.git')) { throw 'Refusing nested repository' }
    }
    $files=@(Get-ChildItem -LiteralPath $language -File -Filter '*.lng')
    if ($files.Count -eq 0) { continue }
    $before=@{}
    foreach ($file in $files) {
        if ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Refusing linked language path' }
        $before[$file.Name]=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    }
    $output=& compact.exe /C /Q (Join-Path $language '*.lng')
    if ($LASTEXITCODE -ne 0) { throw "Native compression failed at $language" }
    foreach ($file in $files) {
        if ((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -ne $before[$file.Name]) { throw 'Language content changed' }
    }
    $records.Add([pscustomobject]@{Directory=$language;Files=$files.Count;Hashes=$before;NativeOutput=$output})
    if ($records.Count % 50 -eq 0) { Write-Output "Compressed $($records.Count) completed language homes; free=$((Get-PSDrive -Name D).Free)" }
    if ((Get-PSDrive -Name D).Free -ge $targetFree) { break }
}
$final=(Get-PSDrive -Name D).Free
[pscustomobject]@{Documentation='Reversible NTFS compression of completed copied language packs; exact SHA256 retained';InitialFree=$initial;FinalFree=$final;ObservedFreeDelta=$final-$initial;Records=$records} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $repository 'logs/surface-entry-runtime-language-compression.json') -Encoding UTF8
Write-Output "Completed $($records.Count) homes; observed free-space change=$($final-$initial); free=$final"
