#Offline capture-contract checks; never start a game or post to a real window.
#@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md (Group 3)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
public static class OttdWin {
    public struct RECT { public int Left, Top, Right, Bottom; }
    public struct POINT { public int X, Y; }
    public struct Message { public IntPtr Handle; public uint Code; public IntPtr W, L; }
    public static List<Message> Messages = new List<Message>();
    public static bool GetWindowRect(IntPtr handle, out RECT rect) { rect = new RECT { Left=100, Top=100, Right=320, Bottom=270 }; return true; }
    public static bool GetClientRect(IntPtr handle, out RECT rect) { rect = new RECT { Right=200, Bottom=130 }; return true; }
    public static bool ClientToScreen(IntPtr handle, ref POINT point) { point.X=110; point.Y=130; return true; }
    public static bool PrintWindow(IntPtr handle, IntPtr hdc, uint flags) {
        using (Graphics graphics = Graphics.FromHdc(hdc)) { graphics.Clear(Color.FromArgb(72,96,112)); }
        return flags == 2;
    }
    public static bool PostMessage(IntPtr handle, uint code, IntPtr w, IntPtr l) {
        if (handle != (IntPtr)1) throw new Exception("Unexpected window target");
        Messages.Add(new Message { Handle=handle, Code=code, W=w, L=l });
        return true;
    }
}
public static class ArgvCheck {
    [DllImport("shell32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr CommandLineToArgvW(string line, out int count);
    [DllImport("kernel32.dll")] public static extern IntPtr LocalFree(IntPtr memory);
}
'@
. "$PSScriptRoot/../docs/zz-reach2/upgrades/2026-10/tools/ottd_window.ps1"
<# .SYNOPSIS Replace waiting while testing message construction. .PARAMETER Milliseconds Ignored wait. .PARAMETER Seconds Ignored wait. #>
function Start-Sleep([int]$Milliseconds, [int]$Seconds) { }
$fake = [pscustomobject]@{ MainWindowHandle=[IntPtr]1; HasExited=$false }
$fake | Add-Member -MemberType ScriptMethod -Name Refresh -Value { }
$folder = Join-Path $PSScriptRoot 'capture-tool-checks'
[void][System.IO.Directory]::CreateDirectory($folder)
$geometry = Get-OttdGeometry $fake
if ($geometry.Width -ne 220 -or $geometry.Height -ne 170 -or $geometry.ClientWidth -ne 200 -or $geometry.ClientHeight -ne 130) { throw 'Geometry mismatch' }
$centre = Get-OttdCentre $fake
if ($centre.X -ne 110 -or $centre.Y -ne 95 -or $centre.ClientX -ne 100 -or $centre.ClientY -ne 65) { throw 'Centre mismatch' }
$shot = Join-Path $folder 'mock-shot.png'
$offset = Save-OttdShot $fake $shot
if ($offset -ne '10,30') { throw 'Legacy shot offset changed' }
$bmp = [System.Drawing.Bitmap]::FromFile($shot)
try { if ($bmp.Width -ne 220 -or $bmp.Height -ne 170 -or $bmp.GetPixel(0,0).R -ne 72) { throw 'Shot pixels mismatch' } }
finally { $bmp.Dispose() }
$crop = Join-Path $folder 'mock-crop.png'
Save-OttdCrop $fake $crop 10 30 200 130
$bmp = [System.Drawing.Bitmap]::FromFile($crop)
try { if ($bmp.Width -ne 200 -or $bmp.Height -ne 130 -or $bmp.GetPixel(0,0).B -ne 112) { throw 'Crop mismatch' } }
finally { $bmp.Dispose() }
$rejected = $false
try { Save-OttdCrop $fake (Join-Path $folder 'invalid.png') -1 30 200 130 }
catch { $rejected = $_.Exception.Message -eq 'The requested crop leaves the game window.' }
if (-not $rejected) { throw 'Invalid crop accepted' }
Write-Output 'PASS capture geometry, legacy offset, bitmap pixels, crop dimensions and crop rejection'
[OttdWin]::Messages.Clear()
Invoke-OttdConsole $fake 'scrollto instant 4660'
$messages = [OttdWin]::Messages
$consoleKeys = @($messages | Where-Object { $_.Code -eq 0x0100 -and $_.W.ToInt64() -eq 0xC0 })
if ($consoleKeys.Count -ne 2 -or (($consoleKeys[0].L.ToInt64() -shr 16) -band 255) -ne 41) { throw 'Console scan code mismatch' }
$chars = -join @($messages | Where-Object { $_.Code -eq 0x0102 } | ForEach-Object { [char]$_.W.ToInt64() })
if ($chars -ne 'scrollto instant 4660') { throw 'Console command mismatch' }
Write-Output 'PASS layout-independent console key and command messages target only the fake window'
foreach ($value in @('', 'plain', 'two words', 'a"b', 'folder with spaces\', '\\server\folder with spaces\')) {
    $line = 'openttd.exe ' + (ConvertTo-OttdArgument $value)
    $count = 0
    $argv = [ArgvCheck]::CommandLineToArgvW($line, [ref]$count)
    try {
        if ($count -ne 2) { throw "Argument split: $value" }
        $actual = [System.Runtime.InteropServices.Marshal]::PtrToStringUni([System.Runtime.InteropServices.Marshal]::ReadIntPtr($argv, [IntPtr]::Size))
        if ($actual -cne $value) { throw "Argument changed: $value" }
    } finally { [void][ArgvCheck]::LocalFree($argv) }
}
Write-Output 'PASS native Windows argv quoting: spaces, quotes, empty values and trailing backslashes'
$burst = Join-Path $folder ('mock-burst-' + [Guid]::NewGuid().ToString('N'))
$count = Save-OttdBurst $fake $burst 0.005 1 10 30 100 100
if ($count -lt 1 -or @(Get-ChildItem -LiteralPath $burst -Filter 'frame-*.png').Count -ne $count) { throw 'Burst frame count mismatch' }
Write-Output 'PASS timed burst writes the returned number of frames'
