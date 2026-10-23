<#
.SYNOPSIS Capture layer art from a gallery save using PrintWindow and game-window input only.
.DESCRIPTION Implements r2mlm13 Groups 3, 4, 9 and 10. Run after telling the user before starting the game.
.PARAMETER Build Build folder containing the executable and baseset.
.PARAMETER Config Debug or RelWithDebInfo.
.PARAMETER Cfg Config path, relative to Build when not absolute.
.PARAMETER Save Gallery save; defaults to Build/save/art_gallery.sav.
.PARAMETER Scenario Purpose-named case beneath each explicit review feature.
.PARAMETER SizeX Tiles across one map plane.
.PARAMETER SizeY Tiles down one map plane.
.PARAMETER Zooms Console zoom levels to capture.
.PARAMETER InterfaceScale Toolbar scale, 1 or 2.
.PARAMETER LayerButton Layer-selector click point in client coordinates.
.PARAMETER RoadButton Main-toolbar road button in client coordinates.
.PARAMETER RailButton Main-toolbar rail button in client coordinates.
.PARAMETER RoadTunnelTool Road-toolbar layer-tunnel button in client coordinates.
.PARAMETER RoadElevatorTool Road-toolbar elevator button in client coordinates.
.PARAMETER RailPortalTool Rail-toolbar layer-portal button in client coordinates.
.PARAMETER LapSeconds Burst duration per kind; defaults to railtunnel=40, roadtunnel=50, elevator=100.
.PARAMETER IntervalMs Milliseconds between traffic frames.
.PARAMETER NoUi Skip toolbar, menu and cursor captures.
.PARAMETER NoTraffic Skip timed traffic bursts.
@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md
#>
[CmdletBinding()]
param(
	[string]$Build = (Join-Path $PSScriptRoot '../../../build'),
	[ValidateSet('Debug', 'RelWithDebInfo')][string]$Config = 'Debug',
	[string]$Cfg = 'art_1x.cfg',
	[string]$Save,
	[string]$Scenario = 'manual-art',
	[ValidateRange(64, 4096)][int]$SizeX = 64,
	[ValidateRange(64, 4096)][int]$SizeY = 64,
	[ValidateRange(0, 5)][int[]]$Zooms = @(1, 2, 3),
	[ValidateSet(1, 2)][int]$InterfaceScale = 1,
	[ValidateCount(2, 2)][int[]]$LayerButton,
	[ValidateCount(2, 2)][int[]]$RoadButton,
	[ValidateCount(2, 2)][int[]]$RailButton,
	[ValidateCount(2, 2)][int[]]$RoadTunnelTool,
	[ValidateCount(2, 2)][int[]]$RoadElevatorTool,
	[ValidateCount(2, 2)][int[]]$RailPortalTool,
	[hashtable]$LapSeconds = @{ railtunnel = 40; roadtunnel = 50; elevator = 100 },
	[ValidateRange(1, 10000)][int]$IntervalMs = 400,
	[switch]$NoUi,
	[switch]$NoTraffic
)

$ErrorActionPreference = 'Stop'
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
. "$repoRoot/mlm/tools/run/ottd_window.ps1"
$Build = [System.IO.Path]::GetFullPath($Build)
if (-not $Save) { $Save = Join-Path $Build 'save/art_gallery.sav' }
$Save = (Resolve-Path -LiteralPath $Save).Path
if (-not [System.IO.Path]::IsPathRooted($Cfg)) { $Cfg = Join-Path $Build $Cfg }
$Cfg = (Resolve-Path -LiteralPath $Cfg).Path
$transport = & python -B "$repoRoot/mlm/tools/run/window_context.py" --feature multilayer-art --scenario $Scenario -- -c $Cfg -g $Save
if ($LASTEXITCODE -ne 0) { throw 'Could not prepare feature capture directories.' }
$context = $transport | ConvertFrom-Json
$portalOut = $context.paths.portals.images
$terrainOut = $context.paths.geology.images
$controlOut = $context.paths.construction.images
if (-not (Test-Path -LiteralPath (Join-Path $Build "$Config/openttd.exe"))) { throw 'The selected game executable does not exist.' }
if (-not $NoUi) {
	#UI coordinates depend on interface size and window placement; never guess clicks.
	foreach ($point in @($LayerButton, $RoadButton, $RailButton, $RoadTunnelTool, $RoadElevatorTool, $RailPortalTool)) {
		if ($null -eq $point -or $point.Count -ne 2) { throw 'Provide all six UI click points, or use -NoUi.' }
	}
}
if (-not $NoTraffic) {
	foreach ($kind in @('railtunnel', 'roadtunnel', 'elevator')) {
		if (-not $LapSeconds.ContainsKey($kind) -or $LapSeconds[$kind] -le 0) { throw "Provide a positive LapSeconds duration for $kind." }
	}
}

$layerNames = @('surface', 'underground', 'space', 'deep')
$portalLines = & python -B "$repoRoot/mlm/tools/savegames/savegame_plane_digest.py" $Save --portals --size-x $SizeX --size-y $SizeY
if ($LASTEXITCODE -ne 0) { throw 'Could not read portal scenes from the save.' }
$ends = @($portalLines | ForEach-Object {
	if ($_ -notmatch '^portal tile (\d+) layer (\d+) x (\d+) y (\d+) shaft (NE|SE|SW|NW) door (NE|SE|SW|NW) transport (rail|road) target (\d+)$') { throw "Unexpected portal listing: $_" }
	$layer = [int]$Matches[2]
	$target = [int]$Matches[8]
	#Portal kinds are derived from transport and endpoints, as in r2mlm12 D-3.
	$kind = if ($Matches[7] -eq 'rail') { 'railtunnel' } elseif ($layer -eq 3 -or $target -eq 3) { 'elevator' } else { 'roadtunnel' }
	[pscustomobject]@{ Tile = [int]$Matches[1]; Layer = $layer; X = [int]$Matches[3]; Y = [int]$Matches[4]; Door = $Matches[6].ToLowerInvariant(); Kind = $kind }
})
if ($ends.Count -eq 0) { throw 'The gallery save has no layer portal ends.' }

<#
.SYNOPSIS Find the integer middle of a set of portal ends.
.PARAMETER Ends Ends on one loop and layer.
.OUTPUTS Plane X/Y of the middle tile.
#>
function Get-ArtMiddle($Ends) {
	$x = $Ends | Measure-Object -Property X -Minimum -Maximum
	$y = $Ends | Measure-Object -Property Y -Minimum -Maximum
	return [pscustomobject]@{ X = [int][Math]::Floor(($x.Minimum + $x.Maximum) / 2); Y = [int][Math]::Floor(($y.Minimum + $y.Maximum) / 2) }
}

<#
.SYNOPSIS Scroll to a tile on a specific storage layer.
.PARAMETER Process Game process.
.PARAMETER Layer Storage-layer value.
.PARAMETER X Plane X.
.PARAMETER Y Plane Y.
#>
function Show-ArtTile($Process, [int]$Layer, [int]$X, [int]$Y) {
	$tile = $Layer * $SizeX * $SizeY + $Y * $SizeX + $X
	Invoke-OttdConsole $Process "scrollto instant $tile"
}

<#
.SYNOPSIS Click a supplied client point and wait for the tool window to draw.
.PARAMETER Process Game process.
.PARAMETER Point Two client coordinates.
#>
function Open-ArtTool($Process, [int[]]$Point) {
	Send-OttdClick $Process $Point[0] $Point[1]
	Start-Sleep -Milliseconds 300
}

<#
.SYNOPSIS Capture a tool's cursor at the centre without moving the real mouse.
.PARAMETER Process Game process.
.PARAMETER Point Tool button click point.
.PARAMETER Name Cursor scene name.
#>
function Save-ArtCursor($Process, [int[]]$Point, [string]$Name) {
	Open-ArtTool $Process $Point
	$centre = Get-OttdCentre $Process
	Send-OttdHover $Process $centre.ClientX $centre.ClientY
	Start-Sleep -Milliseconds 300
	Save-OttdCrop $Process (Join-Path $controlOut "ui-cursor-$Name.png") ($centre.X - 48) ($centre.Y - 48) 96 96
	Send-OttdKey $Process 0x1B 0
}

$process = $null
try {
	$process = Start-Ottd $Build $Config @('-c', $Cfg, '-g', $Save) -Context $context
	Invoke-OttdConsole $process 'pause'
	$centre = Get-OttdCentre $process
	foreach ($end in $ends) {
		Invoke-OttdConsole $process "scrollto instant $($end.Tile)"
		$name = '{0}-{1}-door-{2}' -f $end.Kind, $layerNames[$end.Layer], $end.Door
		#A general save can contain several matching ends; keep every capture rather than overwrite it.
		$matching = @($ends | Where-Object { $_.Kind -eq $end.Kind -and $_.Layer -eq $end.Layer -and $_.Door -eq $end.Door })
		if ($matching.Count -gt 1) { $name += "-tile-$($end.Tile)" }
		foreach ($zoom in $Zooms) {
			Invoke-OttdConsole $process "zoomto $zoom"
			Save-OttdCrop $process (Join-Path $portalOut "$name-z$zoom.png") ($centre.X - 160) ($centre.Y - 120) 320 240
		}
	}
	$middle = Get-ArtMiddle @($ends | Where-Object { $_.Kind -eq $ends[0].Kind })
	foreach ($layer in @(1, 2, 3)) {
		Show-ArtTile $process $layer $middle.X $middle.Y
		foreach ($zoom in $Zooms) {
			Invoke-OttdConsole $process "zoomto $zoom"
			Save-OttdCrop $process (Join-Path $terrainOut "floor-$($layerNames[$layer])-z$zoom.png") ($centre.X - 160) ($centre.Y - 120) 320 240
		}
	}
	if (-not $NoUi) {
		Invoke-OttdConsole $process 'zoomto 2'
		foreach ($layer in @(0, 1, 3, 2)) {
			Show-ArtTile $process $layer $middle.X $middle.Y
			$geometry = Get-OttdGeometry $process
			Save-OttdCrop $process (Join-Path $controlOut "ui-toolbar-$($layerNames[$layer]).png") $geometry.ClientX $geometry.ClientY $geometry.ClientWidth (24 * $InterfaceScale)
		}
		Open-ArtTool $process $LayerButton
		[void](Save-OttdShot $process (Join-Path $controlOut 'ui-layer-dropdown.png'))
		Send-OttdKey $process 0x1B 0
		Show-ArtTile $process 0 $middle.X $middle.Y
		Open-ArtTool $process $RoadButton
		[void](Save-OttdShot $process (Join-Path $controlOut 'ui-road-toolbar.png'))
		Save-ArtCursor $process $RoadTunnelTool 'roadtunnel'
		Save-ArtCursor $process $RoadElevatorTool 'elevator'
		Open-ArtTool $process $RailButton
		[void](Save-OttdShot $process (Join-Path $controlOut 'ui-rail-toolbar.png'))
		Save-ArtCursor $process $RailPortalTool 'railtunnel'
		Send-OttdKey $process 0x1B 0
	}
	if (-not $NoTraffic) {
		foreach ($group in ($ends | Group-Object -Property Kind, Layer)) {
			$kind = $group.Group[0].Kind
			$layer = $group.Group[0].Layer
			$middle = Get-ArtMiddle $group.Group
			$folder = Join-Path $portalOut "traffic-$kind-$($layerNames[$layer])"
			$scenes = @()
			foreach ($end in $group.Group) {
				#Normal-zoom RemapCoords; subtract 16 to centre the end's structure above its ground point.
				$sx = 360 + 32 * (($end.Y - $middle.Y) - ($end.X - $middle.X))
				$sy = 200 + 16 * (($end.X - $middle.X) + ($end.Y - $middle.Y)) - 16
				if ($sx - 80 -lt 0 -or $sy - 60 -lt 0 -or $sx + 80 -gt 720 -or $sy + 60 -gt 400) { throw 'The traffic loop exceeds the gallery crop; use the compact loop layout in r2mlm13 Group 4.' }
				$name = '{0}-{1}-door-{2}' -f $kind, $layerNames[$layer], $end.Door
				$matching = @($group.Group | Where-Object { $_.Door -eq $end.Door })
				if ($matching.Count -gt 1) { $name += "-tile-$($end.Tile)" }
				$scenes += [pscustomobject]@{ folder = $folder; name = $name; left = $sx - 80; top = $sy - 60; right = $sx + 80; bottom = $sy + 60 }
			}
			Show-ArtTile $process $layer $middle.X $middle.Y
			Invoke-OttdConsole $process 'zoomto 2'
			Invoke-OttdConsole $process 'unpause'
			$count = Save-OttdBurst $process $folder $LapSeconds[$kind] $IntervalMs ($centre.X - 360) ($centre.Y - 200) 720 400
			Invoke-OttdConsole $process 'pause'
			$scenes | Export-Csv -LiteralPath (Join-Path $context.paths.portals.reports 'scenes.csv') -Append -NoTypeInformation -Encoding UTF8
			Write-Output "$kind/$($layerNames[$layer]): $count traffic frames"
		}
	}
} finally {
	#Only close the process this script owns; the user's game is never reused.
	Stop-Ottd $process
}
