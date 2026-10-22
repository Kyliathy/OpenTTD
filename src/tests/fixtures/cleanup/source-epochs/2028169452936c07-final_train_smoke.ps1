#Capture the owned Debug game while the connected train crosses its Surface-SW portal.
$ErrorActionPreference = 'Stop'
. 'D:/Codez/Sandbox/OpenTTD/docs/zz-reach2/upgrades/2026-10/tools/ottd_window.ps1'
$game = Get-Process -Name openttd_console | Where-Object { $_.Path -eq 'D:\Codez\Sandbox\OpenTTD\build\Debug\openttd_console.exe' -and $_.MainWindowHandle -ne [IntPtr]::Zero } | Select-Object -First 1
if (-not $game) { throw 'The owned Debug game is not open.' }
Invoke-OttdConsole $game 'scrollto 50778'
Invoke-OttdConsole $game 'zoomto 1'
Invoke-OttdConsole $game 'unpause'
Save-OttdShot $game 'D:/Codez/Sandbox/OpenTTD/build/makinpr2-review/final-neighbour-start.png'
Save-OttdBurst $game 'D:/Codez/Sandbox/OpenTTD/build/makinpr2-review/final-neighbour-traffic' 24 500 120 90 480 260
Invoke-OttdConsole $game 'pause'
Save-OttdShot $game 'D:/Codez/Sandbox/OpenTTD/build/makinpr2-review/final-neighbour-end.png'
Invoke-OttdConsole $game 'save makinpr2-final-verification'
Invoke-OttdConsole $game 'quit'
