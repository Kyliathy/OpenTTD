#Round-trip the completed old-scenario month without advancing simulation.
$ErrorActionPreference = 'Stop'
$scriptFile = 'D:/Codez/Sandbox/OpenTTD/build/scripts/game_start.scr'
$originalScript = [IO.File]::ReadAllBytes($scriptFile)
try {
    [IO.File]::WriteAllText($scriptFile, "save final_legacy_month_roundtrip`nquit`n", (New-Object Text.UTF8Encoding($false)))
    & 'C:/Program Files/Git/bin/bash.exe' '/d/Codez/Sandbox/OpenTTD/docs/zz-reach2/upgrades/2026-10/tools/run_dedicated.sh' '/d/Codez/Sandbox/OpenTTD/build' 'Debug' 'dedicated-logs/final_legacy_month_roundtrip.log' '-c' 'legacy-month.cfg' '-g' 'save/final_legacy_month.sav'
    if ($LASTEXITCODE -ne 0) { throw 'Played legacy scenario reload failed.' }
} finally {
    [IO.File]::WriteAllBytes($scriptFile, $originalScript)
}
