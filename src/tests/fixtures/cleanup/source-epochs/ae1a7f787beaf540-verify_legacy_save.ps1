#Load an original Transport Tycoon save and retain only repository copies.
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path 'D:/Codez/Sandbox/OpenTTD').Path
$buildFolder = Join-Path $repo 'build'
$startupScript = Join-Path $buildFolder 'scripts/game_start.scr'
$originalScript = [IO.File]::ReadAllBytes($startupScript)
$encoding = New-Object Text.UTF8Encoding($false)
$runner = '/d/Codez/Sandbox/OpenTTD/docs/zz-reach2/upgrades/2026-10/tools/run_dedicated.sh'
try {
    [IO.File]::WriteAllText($startupScript, "save final_legacy_loaded`nquit`n", $encoding)
    & 'C:/Program Files/Git/bin/bash.exe' $runner '/d/Codez/Sandbox/OpenTTD/build' 'Debug' 'dedicated-logs/final_legacy_loaded.log' '-c' 'final-map-review.cfg' '-g' '../!kit/TT/trt00.sv0'
    if ($LASTEXITCODE -ne 0) { throw 'Original Transport Tycoon load failed.' }
    [IO.File]::WriteAllText($startupScript, "save final_legacy_roundtrip`nquit`n", $encoding)
    & 'C:/Program Files/Git/bin/bash.exe' $runner '/d/Codez/Sandbox/OpenTTD/build' 'Debug' 'dedicated-logs/final_legacy_roundtrip.log' '-c' 'final-map-review.cfg' '-g' 'save/final_legacy_loaded.sav'
    if ($LASTEXITCODE -ne 0) { throw 'Legacy upgrade round trip failed.' }
} finally {
    [IO.File]::WriteAllBytes($startupScript, $originalScript)
}
