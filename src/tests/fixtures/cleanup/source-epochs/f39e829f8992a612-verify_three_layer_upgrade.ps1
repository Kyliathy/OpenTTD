#Upgrade the authentic version-369 review fixture without changing the original.
$ErrorActionPreference = 'Stop'
$scriptFile = 'D:/Codez/Sandbox/OpenTTD/build/scripts/game_start.scr'
$originalScript = [IO.File]::ReadAllBytes($scriptFile)
try {
    [IO.File]::WriteAllText($scriptFile, "save deep_upgraded_1234_8x8`nquit`n", (New-Object Text.UTF8Encoding($false)))
    & 'C:/Program Files/Git/bin/bash.exe' '/d/Codez/Sandbox/OpenTTD/docs/zz-reach2/upgrades/2026-10/tools/run_dedicated.sh' '/d/Codez/Sandbox/OpenTTD/build' 'RelWithDebInfo' 'dedicated-logs/deep_upgraded_1234_8x8.log' '-c' 'final-map-review.cfg' '-g' 'save/fd9_upgraded_1234_8x8.sav'
    if ($LASTEXITCODE -ne 0) { throw 'Three-layer upgrade failed.' }
} finally {
    [IO.File]::WriteAllBytes($scriptFile, $originalScript)
}
