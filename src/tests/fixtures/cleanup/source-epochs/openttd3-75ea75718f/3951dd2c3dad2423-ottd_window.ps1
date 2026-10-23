<#
Drive an OpenTTD window for screen checks without touching the user's mouse or screen.
Written for docs/zz-reach2/upgrades/2026-10/map-layers/r2rvm-review-map.md (### Commands used by every group, window capture)
and used by the in-game art reviews of docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md.
Dot-source it, then call Start-Ottd, Save-OttdShot, Send-OttdHover, Send-OttdClick, Send-OttdKey and Stop-Ottd.
Art tools also use Get-OttdBitmap, Get-OttdCentre, Save-OttdCrop, Save-OttdBurst,
Switch-OttdConsole and Invoke-OttdConsole (r2mlm13 Group 3).
Captures use PrintWindow on the game window only, never a copy of the screen, and input is posted to the
game window, never sent through the real mouse or keyboard.
#>
Add-Type -AssemblyName System.Drawing
if (-not ('OttdWin' -as [type])) {
	Add-Type @'
using System;
using System.Runtime.InteropServices;
/** Native window-only capture/input boundary; @see r2fc-file-cleanup.md. */
public static class OttdWin {
	/** Win32 rectangle in window/client pixels; no screen capture is performed. */
	[StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
	/** Win32 point used to relate client coordinates to the owned window. */
	[StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
	/** Measure the owned window. @param hWnd Window handle. @param rect Returned bounds. @return Native success flag. */
	[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
	/** Measure client bounds. @param hWnd Window handle. @param rect Returned client rectangle. @return Native success flag. */
	[DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
	/** Translate a client point. @param hWnd Window handle. @param p Point updated in place. @return Native success flag. */
	[DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT p);
	/** Render the owned window only. @param hWnd Window handle. @param hdc Bitmap device context. @param flags Native capture flags. @return Native success flag. */
	[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
	/** Resize without taking input focus. @param hWnd Window handle. @param after Z-order neighbor. @param x Left. @param y Top. @param width Outer width. @param height Outer height. @param flags Position policy. @return Native success flag. */
	[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int width, int height, uint flags);
	/** Post input solely to the game window. @param hWnd Window handle. @param msg Input message. @param w First message value. @param l Second message value. @return Native success flag. */
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
.SYNOPSIS Quote one native Windows argument so paths with spaces remain a single argument.
.PARAMETER Value Argument value, passed directly to the executable.
.OUTPUTS Windows command-line representation.
#>
function ConvertTo-OttdArgument([string]$Value) {
	if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') { return $Value }
	#Double backslashes before quotes and at the closing quote, following native argv parsing.
	$escaped = [regex]::Replace($Value, '(\\*)"', '$1$1\"')
	$escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
	return '"' + $escaped + '"'
}

<#
.SYNOPSIS Start the game and wait until its window has drawn.
.PARAMETER Build Build folder, used as working and config folder.
.PARAMETER Config Debug or RelWithDebInfo.
.PARAMETER Arguments Game arguments, for example -c baseline.cfg -g -G 1234.
.PARAMETER Settle Seconds to wait after the window appears.
.PARAMETER Context Optional shared context prepared by the mixed-feature capture caller.
.PARAMETER Feature Explicit review owner for a standalone manual capture.
.PARAMETER Scenario Explicit review case within that feature.
.OUTPUTS The process.
#>
function Start-Ottd([string]$Build, [string]$Config, [string[]]$Arguments, [int]$Settle = 12, $Context = $null, [string]$Feature = "multilayer-art", [string]$Scenario = "manual-window") {
	if ($null -eq $Context) {
		$transport = & python -B "$PSScriptRoot/window_context.py" --feature $Feature --scenario $Scenario -- @Arguments
		if ($LASTEXITCODE -ne 0) { throw 'Could not prepare isolated capture runtime.' }
		$Context = $transport | ConvertFrom-Json
	}
	$quotedArguments = @($Context.arguments | ForEach-Object { ConvertTo-OttdArgument $_ })
	$previousEnvironment = @{}
	try {
		# Transport only this invocation's native variables; restore the shell's
		# environment immediately after the owned interactive process inherits it.
		foreach ($entry in $Context.environment.PSObject.Properties) {
			$previousEnvironment[$entry.Name] = [Environment]::GetEnvironmentVariable($entry.Name, 'Process')
			[Environment]::SetEnvironmentVariable($entry.Name, $entry.Value, 'Process')
		}
		# This entry point explicitly opens the game window for PrintWindow/UI
		# interaction. Shell/background acceptance uses native_launch.py instead.
		$p = Start-Process -FilePath "$Build\$Config\openttd.exe" -ArgumentList $quotedArguments -WorkingDirectory $Context.runtime -PassThru
	} finally {
		foreach ($entry in $previousEnvironment.GetEnumerator()) {
			[Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process')
		}
	}
	$p | Add-Member -NotePropertyName MlmContext -NotePropertyValue $Context
	$Context | Add-Member -NotePropertyName configuration -NotePropertyValue $Config -Force
	$deadline = (Get-Date).AddSeconds(60)
	#The main window handle appears once the video driver has created the window.
	while ((Get-Date) -lt $deadline -and -not $p.HasExited) { $p.Refresh(); if ($p.MainWindowHandle -ne [IntPtr]::Zero) { break }; Start-Sleep -Milliseconds 500 }
	Start-Sleep -Seconds $Settle
	$p.Refresh()
	if ($p.HasExited -or $p.MainWindowHandle -eq [IntPtr]::Zero) { throw 'The game exited or did not create its window.' }
	return $p
}

<#
.SYNOPSIS Resolve a capture under one of this invocation's explicit feature directories.
.PARAMETER Process Process carrying the shared MlmContext from Start-Ottd.
.PARAMETER Path Requested PNG destination, including any declared burst subdirectory.
.OUTPUTS Canonical absolute destination; linked output directories are refused.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 5C.
#>
function Assert-OttdCapturePath($Process, [string]$Path) {
	$target = [System.IO.Path]::GetFullPath($Path)
	$owners = @($Process.MlmContext.paths.PSObject.Properties | Where-Object {
		$target.StartsWith($_.Value.images.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)
	})
	# The declared feature boundary protects immutable inputs and other runs;
	# only PNG output is accepted by these image-writing APIs.
	if ($owners.Count -ne 1 -or [System.IO.Path]::GetExtension($target) -ne '.png') { throw 'Capture must be a PNG inside this invocation feature directory.' }
	$directory = [System.IO.Path]::GetDirectoryName($target)
	$ancestor = $directory
	# Refuse junctions before creating any nested burst folders. Checking every
	# existing descendant prevents a linked child from redirecting the write.
	while ($ancestor.StartsWith($owners[0].Value.images, [StringComparison]::OrdinalIgnoreCase)) {
		if (Test-Path -LiteralPath $ancestor) {
			$item = Get-Item -LiteralPath $ancestor -Force
			if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Capture directories cannot be links.' }
		}
		$ancestor = [System.IO.Path]::GetDirectoryName($ancestor)
	}
	if (Test-Path -LiteralPath $target) { throw 'Refuse replacing a prior captured frame.' }
	[void][System.IO.Directory]::CreateDirectory($directory)
	return $target
}

<#
.SYNOPSIS Bind a completed window capture to its producer, inputs and feature.
.PARAMETER Process Owned game process with invocation metadata.
.PARAMETER Path Exact PNG just emitted by the native window observer.
.PARAMETER Writer Actual capture mechanism, including crop when applicable.
.OUTPUTS None; append provenance in the feature report directory.
#>
function Write-OttdCaptureRecord($Process, [string]$Path, [string]$Writer) {
	$context = $Process.MlmContext
	$owner = $context.paths.PSObject.Properties | Where-Object {
		$Path.StartsWith($_.Value.images.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)
	} | Select-Object -First 1
	$record = [ordered]@{
		_documentation = 'Owner: docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md'
		producer = 'mlm/tools/run/ottd_window.ps1'; native_writer = $Writer
		feature = $owner.Name; scenario = $context.scenario; run_id = $context.run_id
		configuration = $context.configuration; inputs = $context.inputs
		path = $Path.Substring($context.root.Length + 1).Replace('\', '/')
		sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
	}
	$record | ConvertTo-Json -Depth 6 -Compress | Add-Content -LiteralPath (Join-Path $owner.Value.reports 'window-captures.jsonl') -Encoding UTF8
}

<#
.SYNOPSIS Capture the game window only, even when another window covers it.
.PARAMETER Process The game process.
.PARAMETER Path PNG file to write.
.OUTPUTS The offset of the client area inside the capture, as "x,y".
#>
function Save-OttdShot($Process, [string]$Path) {
	$Path = Assert-OttdCapturePath $Process $Path
	$capture = Get-OttdBitmap $Process
	try { $capture.Bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png) }
	finally { $capture.Bitmap.Dispose() }
	Write-OttdCaptureRecord $Process $Path "PrintWindow"
	return "$($capture.ClientX),$($capture.ClientY)"
}

<#
.SYNOPSIS Measure the game window and its client area in capture pixels.
.PARAMETER Process The game process.
.OUTPUTS Window size, client size and client offset.
#>
function Get-OttdGeometry($Process) {
	$Process.Refresh()
	$h = $Process.MainWindowHandle
	if ($Process.HasExited -or $h -eq [IntPtr]::Zero) { throw 'The game has no live window to capture.' }
	$r = New-Object OttdWin+RECT
	$client = New-Object OttdWin+RECT
	$o = New-Object OttdWin+POINT
	#Measure actual client bounds rather than guessing border widths across DPI settings.
	if (-not [OttdWin]::GetWindowRect($h, [ref]$r) -or -not [OttdWin]::GetClientRect($h, [ref]$client) -or -not [OttdWin]::ClientToScreen($h, [ref]$o)) { throw 'Could not measure the game window.' }
	return [pscustomobject]@{
		Handle = $h; Width = $r.Right - $r.Left; Height = $r.Bottom - $r.Top
		ClientX = $o.X - $r.Left; ClientY = $o.Y - $r.Top
		ClientWidth = $client.Right - $client.Left; ClientHeight = $client.Bottom - $client.Top
	}
}

<#
.SYNOPSIS Capture the game with PrintWindow; the caller owns and disposes the bitmap.
.PARAMETER Process The game process.
.OUTPUTS Bitmap and client geometry.
#>
function Get-OttdBitmap($Process) {
	$geometry = Get-OttdGeometry $Process
	$bmp = New-Object System.Drawing.Bitmap $geometry.Width, $geometry.Height
	$g = [System.Drawing.Graphics]::FromImage($bmp)
	try {
		$hdc = $g.GetHdc()
		try {
			#PW_RENDERFULLCONTENT paints only the game window, even behind other windows.
			if (-not [OttdWin]::PrintWindow($geometry.Handle, $hdc, 2)) { throw 'PrintWindow failed.' }
		} finally { $g.ReleaseHdc($hdc) }
	} catch { $bmp.Dispose(); throw }
	finally { $g.Dispose() }
	return [pscustomobject]@{
		Bitmap = $bmp; ClientX = $geometry.ClientX; ClientY = $geometry.ClientY
		ClientWidth = $geometry.ClientWidth; ClientHeight = $geometry.ClientHeight
	}
}

<#
.SYNOPSIS Resize only the game client area without moving, focusing or raising its window.
.PARAMETER Process The game process owned by this check.
.PARAMETER Width Requested client width in capture pixels.
.PARAMETER Height Requested client height in capture pixels.
.OUTPUTS Measured geometry after the resize.
#>
function Resize-Ottd($Process, [int]$Width, [int]$Height) {
	$geometry = Get-OttdGeometry $Process
	$windowWidth = $Width + $geometry.Width - $geometry.ClientWidth
	$windowHeight = $Height + $geometry.Height - $geometry.ClientHeight
	#Preserve position, z-order and focus while letting the driver handle its ordinary resize path.
	if (-not [OttdWin]::SetWindowPos($geometry.Handle, [IntPtr]::Zero, 0, 0, $windowWidth, $windowHeight, 0x16)) { throw 'Could not resize the game window.' }
	Start-Sleep -Milliseconds 500
	$actual = Get-OttdGeometry $Process
	if ($actual.ClientWidth -ne $Width -or $actual.ClientHeight -ne $Height) { throw 'The game did not accept the requested client size.' }
	return $actual
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
	#Queue movement and press together; a delayed press can use an intervening real mouse position.
	[void][OttdWin]::PostMessage($Process.MainWindowHandle, 0x0200, [IntPtr]0, $lp)
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
.SYNOPSIS Open or close the in-game console with the key's layout-independent scan code.
.PARAMETER Process The game process.
#>
function Switch-OttdConsole($Process) {
	$h = $Process.MainWindowHandle
	#Scan code 41 identifies the console key; TranslateMessage supplies its character when needed.
	[void][OttdWin]::PostMessage($h, 0x0100, [IntPtr]0xC0, [IntPtr](1 -bor (41 -shl 16)))
	Start-Sleep -Milliseconds 100
	[void][OttdWin]::PostMessage($h, 0x0101, [IntPtr]0xC0, [IntPtr](0xC0000001 -bor (41 -shl 16)))
}

<#
.SYNOPSIS Run one console command, closing the console after Return.
.PARAMETER Process The game process.
.PARAMETER Command Console command, without line breaks.
#>
function Invoke-OttdConsole($Process, [string]$Command) {
	if ($Command -match '[\r\n]') { throw 'A console command must occupy one line.' }
	Switch-OttdConsole $Process
	Start-Sleep -Milliseconds 300
	#Typed characters go only to the game's edit box, never to the user's keyboard.
	foreach ($character in $Command.ToCharArray()) { [void][OttdWin]::PostMessage($Process.MainWindowHandle, 0x0102, [IntPtr][int]$character, [IntPtr]1) }
	Send-OttdKey $Process 0x0D 0
	Start-Sleep -Milliseconds 300
	Switch-OttdConsole $Process
	Start-Sleep -Milliseconds 500
}

<#
.SYNOPSIS Find the client centre in capture coordinates and in client coordinates.
.PARAMETER Process The game process.
.OUTPUTS X/Y for crops; ClientX/ClientY for mouse messages.
#>
function Get-OttdCentre($Process) {
	$geometry = Get-OttdGeometry $Process
	$clientX = [int][Math]::Floor($geometry.ClientWidth / 2)
	$clientY = [int][Math]::Floor($geometry.ClientHeight / 2)
	return [pscustomobject]@{ X = $geometry.ClientX + $clientX; Y = $geometry.ClientY + $clientY; ClientX = $clientX; ClientY = $clientY }
}

<#
.SYNOPSIS Save a rectangle of the PrintWindow capture.
.PARAMETER Process The game process.
.PARAMETER Path Destination PNG.
.PARAMETER Left Capture-coordinate left edge.
.PARAMETER Top Capture-coordinate top edge.
.PARAMETER Width Crop width.
.PARAMETER Height Crop height.
#>
function Save-OttdCrop($Process, [string]$Path, [int]$Left, [int]$Top, [int]$Width, [int]$Height) {
	$Path = Assert-OttdCapturePath $Process $Path
	$capture = Get-OttdBitmap $Process
	try {
		#A clipped crop would mislabel a scene and hide a vehicle at the edge.
		if ($Left -lt 0 -or $Top -lt 0 -or $Width -le 0 -or $Height -le 0 -or $Left + $Width -gt $capture.Bitmap.Width -or $Top + $Height -gt $capture.Bitmap.Height) { throw 'The requested crop leaves the game window.' }
		$rect = New-Object System.Drawing.Rectangle $Left, $Top, $Width, $Height
		$cropped = $capture.Bitmap.Clone($rect, $capture.Bitmap.PixelFormat)
		try { $cropped.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png) }
		finally { $cropped.Dispose() }
	} finally { $capture.Bitmap.Dispose() }
	Write-OttdCaptureRecord $Process $Path "PrintWindow/crop"
}

<#
.SYNOPSIS Capture a timed burst; intervals are measured from each frame's start.
.PARAMETER Process The game process.
.PARAMETER Folder Destination folder.
.PARAMETER Seconds Duration in seconds.
.PARAMETER IntervalMs Time between frames, in milliseconds.
.PARAMETER Left Capture-coordinate left edge.
.PARAMETER Top Capture-coordinate top edge.
.PARAMETER Width Crop width.
.PARAMETER Height Crop height.
.OUTPUTS Number of frames written.
#>
function Save-OttdBurst($Process, [string]$Folder, [double]$Seconds, [int]$IntervalMs, [int]$Left, [int]$Top, [int]$Width, [int]$Height) {
	# A nonpositive interval cannot produce meaningful timed traffic evidence.
	if ($Seconds -le 0 -or $IntervalMs -le 0) { throw 'Burst duration and interval must be positive.' }
	[void](Assert-OttdCapturePath $Process (Join-Path $Folder 'frame-0000.png'))
	$watch = [System.Diagnostics.Stopwatch]::StartNew()
	$count = 0
	# Retain one current frame per requested interval, measuring from the frame
	# start so capture overhead does not silently stretch the traffic timeline.
	while ($watch.Elapsed.TotalSeconds -lt $Seconds) {
		$frameStart = $watch.Elapsed.TotalMilliseconds
		Save-OttdCrop $Process (Join-Path $Folder ('frame-{0:D4}.png' -f $count)) $Left $Top $Width $Height
		$count++
		$delay = [int][Math]::Ceiling($frameStart + $IntervalMs - $watch.Elapsed.TotalMilliseconds)
		if ($delay -gt 0) { Start-Sleep -Milliseconds $delay }
	}
	return $count
}

<#
.SYNOPSIS Close the game without saving anything.
.PARAMETER Process The game process.
#>
function Stop-Ottd($Process) {
	if ($Process -and -not $Process.HasExited) { Stop-Process -Id $Process.Id -Force }
}
