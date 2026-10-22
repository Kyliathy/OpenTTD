#Create final four-layer saves and upgrade copies using only repository output paths.
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path 'D:/Codez/Sandbox/OpenTTD').Path
$buildFolder = Join-Path $repo 'build'
$startupScript = Join-Path $buildFolder 'scripts/game_start.scr'
$originalScript = [IO.File]::ReadAllBytes($startupScript)
$encoding = New-Object Text.UTF8Encoding($false)
$runner = '/d/Codez/Sandbox/OpenTTD/docs/zz-reach2/upgrades/2026-10/tools/run_dedicated.sh'
$bash = 'C:/Program Files/Git/bin/bash.exe'
try {
    foreach ($mapSize in @(@(8, 8), @(9, 10))) {
        [IO.File]::WriteAllText((Join-Path $buildFolder 'final-map-review.cfg'), "[game_creation]`nmap_x = $($mapSize[0])`nmap_y = $($mapSize[1])`n", $encoding)
        foreach ($seed in @(1234, 98765)) {
            $caseName = "${seed}_$($mapSize[0])x$($mapSize[1])"
            [IO.File]::WriteAllText($startupScript, "save final_${caseName}`nquit`n", $encoding)
            & $bash $runner '/d/Codez/Sandbox/OpenTTD/build' 'RelWithDebInfo' "dedicated-logs/final_${caseName}.log" '-c' 'final-map-review.cfg' '-G' $seed
            if ($LASTEXITCODE -ne 0) { throw "Generation failed: $caseName" }
            [IO.File]::WriteAllText($startupScript, "save final_upgraded_${caseName}`nquit`n", $encoding)
            & $bash $runner '/d/Codez/Sandbox/OpenTTD/build' 'RelWithDebInfo' "dedicated-logs/final_upgraded_${caseName}.log" '-c' 'final-map-review.cfg' '-g' "/d/Codez/Sandbox/OpenTTD-baseline/build/save/baseline_${caseName}.sav"
            if ($LASTEXITCODE -ne 0) { throw "Upgrade failed: $caseName" }
            [IO.File]::WriteAllText($startupScript, "save final_roundtrip_${caseName}`nquit`n", $encoding)
            & $bash $runner '/d/Codez/Sandbox/OpenTTD/build' 'RelWithDebInfo' "dedicated-logs/final_roundtrip_${caseName}.log" '-c' 'final-map-review.cfg' '-g' "save/final_upgraded_${caseName}.sav"
            if ($LASTEXITCODE -ne 0) { throw "Round trip failed: $caseName" }
        }
    }
} finally {
    #Restore the prior startup script even when a run fails, so later GUI starts remain usable.
    [IO.File]::WriteAllBytes($startupScript, $originalScript)
}
