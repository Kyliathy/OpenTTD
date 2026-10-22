#Exercise an original TTO file for a month with the normal game loop and no window.
param([string]$LegacyFile = '../!kit/TT/trt00.sv0', [string]$SavePrefix = 'final_legacy', [switch]$TwoBoundaries)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path 'D:/Codez/Sandbox/OpenTTD').Path
$buildFolder = Join-Path $repo 'build'
$startupScript = Join-Path $buildFolder 'scripts/game_start.scr'
$originalScript = [IO.File]::ReadAllBytes($startupScript)
$encoding = New-Object Text.UTF8Encoding($false)
try {
    [IO.File]::WriteAllText((Join-Path $buildFolder 'legacy-month.cfg'), "[gui]`nthreaded_saves = false`n", $encoding)
    $endScript = "scripts/${SavePrefix}-month.scr"
    [IO.File]::WriteAllText((Join-Path $buildFolder $endScript), "pause`nsave ${SavePrefix}_month`nquit`n", $encoding)
    $firstScript = $endScript
    if ($TwoBoundaries) {
        #A mid-month old save needs two boundaries to guarantee at least a full month of play.
        $firstScript = "scripts/${SavePrefix}-first-boundary.scr"
        [IO.File]::WriteAllText((Join-Path $buildFolder $firstScript), "schedule on-next-calendar-month $endScript`n", $encoding)
    }
    [IO.File]::WriteAllText($startupScript, "save ${SavePrefix}_preplay`nunpause`nschedule on-next-calendar-month $firstScript`n", $encoding)
    #Debug logging uses stderr; preserve it without PowerShell treating a log line as a terminating error.
    $ErrorActionPreference = 'Continue'
    & "$buildFolder/Debug/openttd_console.exe" '-x' '-c' 'legacy-month.cfg' '-g' $LegacyFile '-vnull:ticks=3000' '-snull' '-mnull' '-Q' '-d' 'oldloader=3,script=2,sl=2,desync=2' > "$buildFolder/dedicated-logs/${SavePrefix}-month.log" 2>&1
    $ErrorActionPreference = 'Stop'
    Write-Output ('legacy month exit ' + $LASTEXITCODE)
    if ($LASTEXITCODE -ne 0) { throw 'Legacy month run failed.' }
} finally {
    [IO.File]::WriteAllBytes($startupScript, $originalScript)
}
