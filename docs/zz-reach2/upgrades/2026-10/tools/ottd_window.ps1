<#
Drive an OpenTTD window for screen checks without touching the user's mouse or screen.
Written for docs/zz-reach2/upgrades/2026-10/r2rvm-review-map.md (### Commands used by every group, window capture)
and used by the in-game art reviews of docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md.
Dot-source it, then call Start-Ottd, Save-OttdShot, Send-OttdHover, Send-OttdClick, Send-OttdKey and Stop-Ottd.
Captures use PrintWindow on the game window only, never a copy of the screen, and input is posted to the
game window, never sent through the real mouse or keyboard.
#>
Add-Type -AssemblyName System.Drawing
if (-not ('OttdWin' -as [type])) {
	Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class OttdWin {
	[StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
	[StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
	[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
	[DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT p);
	[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
	[DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr w, IntPtr l);
}
'@
}

<#
.SYNOPSIS Find the running game started from a build folder.
.PARAMETER Build Build folder whose RelWithDebInfo or Debug openttd.exe is wanted.
.OUTPUTS The process, or nothing.
#>
function Get-Ottd([string]$Build) {
	Get-Process openttd -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$Build\*" } | Select-Object -First 1
}

<#
.SYNOPSIS Start the game and wait until its window has drawn.
.PARAMETER Build Build folder, used as working and config folder.
.PARAMETER Config Debug or RelWithDebInfo.
.PARAMETER Arguments Game arguments, for example -c baseline.cfg -g -G 1234.
.PARAMETER Settle Seconds to wait after the window appears.
.OUTPUTS The process.
#>
function Start-Ottd([string]$Build, [string]$Config, [string[]]$Arguments, [int]$Settle = 12) {
	$p = Start-Process -FilePath "$Build\$Config\openttd.exe" -ArgumentList $Arguments -WorkingDirectory $Build -PassThru
	$deadline = (Get-Date).AddSeconds(60)
	#The main window handle appears once the video driver has created the window.
	while ((Get-Date) -lt $deadline -and -not $p.HasExited) { $p.Refresh(); if ($p.MainWindowHandle -ne [IntPtr]::Zero) { break }; Start-Sleep -Milliseconds 500 }
	Start-Sleep -Seconds $Settle
	$p.Refresh()
	return $p
}

<#
.SYNOPSIS Capture the game window only, even when another window covers it.
.PARAMETER Process The game process.
.PARAMETER Path PNG file to write.
.OUTPUTS The offset of the client area inside the capture, as "x,y".
#>
function Save-OttdShot($Process, [string]$Path) {
	$h = $Process.MainWindowHandle
	$r = New-Object OttdWin+RECT
	[void][OttdWin]::GetWindowRect($h, [ref]$r)
	$o = New-Object OttdWin+POINT
	[void][OttdWin]::ClientToScreen($h, [ref]$o)
	$bmp = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
	$g = [System.Drawing.Graphics]::FromImage($bmp)
	$hdc = $g.GetHdc()
	#Flag 2 (PW_RENDERFULLCONTENT) renders the window itself, not the screen above it.
	[void][OttdWin]::PrintWindow($h, $hdc, 2)
	$g.ReleaseHdc($hdc)
	$bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
	$g.Dispose(); $bmp.Dispose()
	return "$($o.X - $r.Left),$($o.Y - $r.Top)"
}

<#
.SYNOPSIS Pack client coordinates into a mouse message parameter.
.PARAMETER X Client X.
.PARAMETER Y Client Y.
.OUTPUTS The lParam.
#>
function Get-OttdPoint([int]$X, [int]$Y) {
	return [IntPtr](($Y -shl 16) -bor $X)
}

<#
.SYNOPSIS Move the game's own cursor to a client point, so hovering shows a tooltip.
.PARAMETER Process The game process.
.PARAMETER X Client X.
.PARAMETER Y Client Y.
#>
function Send-OttdHover($Process, [int]$X, [int]$Y) {
	[void][OttdWin]::PostMessage($Process.MainWindowHandle, 0x0200, [IntPtr]0, (Get-OttdPoint $X $Y))
}

<#
.SYNOPSIS Left-click a client point of the game window.
.PARAMETER Process The game process.
.PARAMETER X Client X.
.PARAMETER Y Client Y.
#>
function Send-OttdClick($Process, [int]$X, [int]$Y) {
	$lp = Get-OttdPoint $X $Y
	[void][OttdWin]::PostMessage($Process.MainWindowHandle, 0x0200, [IntPtr]0, $lp)
	Start-Sleep -Milliseconds 200
	[void][OttdWin]::PostMessage($Process.MainWindowHandle, 0x0201, [IntPtr]1, $lp)
	Start-Sleep -Milliseconds 100
	[void][OttdWin]::PostMessage($Process.MainWindowHandle, 0x0202, [IntPtr]0, $lp)
}

<#
.SYNOPSIS Press one key in the game window, as the keyboard would: key down, its character, key up.
.PARAMETER Process The game process.
.PARAMETER VirtualKey Windows virtual-key code, for example 0x4D for M.
.PARAMETER Char Character the key types, or 0 for a key that types none.
#>
function Send-OttdKey($Process, [int]$VirtualKey, [int]$Char = 0) {
	$h = $Process.MainWindowHandle
	[void][OttdWin]::PostMessage($h, 0x0100, [IntPtr]$VirtualKey, [IntPtr]1)
	#Letter hotkeys reach the game through WM_CHAR, which the real keyboard sends right after the key down.
	if ($Char -ne 0) { [void][OttdWin]::PostMessage($h, 0x0102, [IntPtr]$Char, [IntPtr]1) }
	Start-Sleep -Milliseconds 100
	[void][OttdWin]::PostMessage($h, 0x0101, [IntPtr]$VirtualKey, [IntPtr]0xC0000001)
}

<#
.SYNOPSIS Close the game without saving anything.
.PARAMETER Process The game process.
#>
function Stop-Ottd($Process) {
	if ($Process -and -not $Process.HasExited) { Stop-Process -Id $Process.Id -Force }
}
