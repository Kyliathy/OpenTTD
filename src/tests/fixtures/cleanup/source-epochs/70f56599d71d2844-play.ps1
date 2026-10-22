<#
Start this branch's game in a new game or in one of the saves we have, picked by number.
Written for the player guide docs/zz-reach2/upgrades/2026-10/r2mlvs-how-to.md (1. Start the game).
@file play.ps1 Player launcher and named demo navigation.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md delivered excavation checkpoint.
@see docs/zz-reach2/upgrades/2026-10/d2um-underground-mines.md demo walkthrough and checks.

The game runs in the build folder with the config build\play.cfg. A -c config there makes the build folder the
game's personal folder too: the game finds its base graphics and the Multilayer Industry package there, and the
saves you make land in build\save, where the menu lists them the next time. The script never writes a save. The game
rewrites play.cfg when you quit, so settings you change in the game stay for the next start.

Examples:
	.\play.ps1                              show the menu and ask for a number
	.\play.ps1 1                            start menu entry 1 without asking
	.\play.ps1 1 -Seed 1234                 a new game on a fixed map
	.\play.ps1 demo                        current 50-cell TBM demo, ready to unpause
	.\play.ps1 underground-mines           the same demo by its permanent menu name
	.\play.ps1 tbm-work                    current 50-cell project, ready to unpause
	.\play.ps1 tbm-plan                    paused 44-cell project for trying the extension preview
	.\play.ps1 tbm-exit                    completed excavation, ready to build exit B
	.\play.ps1 tbm-tunnel                  completed A-to-B tunnel; original TBM parked at B
	.\play.ps1 underground-mines-legacy    preserved older bent-route demo
	.\play.ps1 makinpr2-connected           a save by name from build\save, or the path of any .sav
	.\play.ps1 3 -Config Debug              the Debug build: slow, but it stops at any failed internal check
	.\play.ps1 -All                         also list the generated test saves
	.\play.ps1 1 -GameArgs '-d','misc=3'    extra openttd.exe arguments; -d opens the game's own log window
	.\play.ps1 1 -DryRun                    print the command instead of starting the game
#>
param(
	#Menu number, named demo, save name in build\save, or .sav path; without it the menu asks.
	[Parameter(Position = 0)][string]$Save = '',
	#Build to run: RelWithDebInfo plays smoothly; Debug is slow and stops at any failed internal check.
	[ValidateSet('RelWithDebInfo', 'Debug')][string]$Config = 'RelWithDebInfo',
	#Map seed of a new game; -1 lets the game pick a random map.
	[ValidateRange(-1, 4294967295)][long]$Seed = -1,
	#List every save, the generated test fixtures included, instead of the newest few.
	[switch]$All,
	#Extra arguments passed unchanged to openttd.exe.
	[string[]]$GameArgs = @(),
	#Print the command line and stop, without starting the game or changing play.cfg.
	[switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$build = Join-Path $PSScriptRoot 'build'
$saveDir = Join-Path $build 'save'
$playConfig = Join-Path $build 'play.cfg'
$exe = Join-Path $build "$Config\openttd.exe"
$utf8 = New-Object System.Text.UTF8Encoding $false

# Featured checkpoints have stable names and a guide, so recent autosaves cannot
# push the intended demo out of the menu. Current stages retain the same real world/machine.
$demoSaves = @(
	@{ Name = 'underground-mines'; SaveName = 'underground-demo-tbm-399';
		Snapshot = 'src\tests\fixtures\haulage\underground-demo-tbm-399.sav';
		Text = 'Start here: current 50-cell TBM project and hauler, paused and ready';
		Guide = 'docs\zz-reach2\upgrades\2026-10\d2um-underground-mines.md' }
	@{ Name = 'tbm-plan'; SaveName = 'underground-demo-planning-399';
		Snapshot = 'src\tests\fixtures\haulage\underground-demo-planning-399.sav';
		Text = 'Try planning: paused 44-cell project with room to preview six more cells';
		Guide = 'docs\zz-reach2\upgrades\2026-10\d2um-underground-mines.md' }
	@{ Name = 'tbm-exit'; SaveName = 'underground-demo-exit-399';
		Snapshot = 'src\tests\fixtures\haulage\underground-demo-exit-399.sav';
		Text = 'Build exit B: 50 cells finished, whole TBM stopped clear of the exit site';
		Guide = 'docs\zz-reach2\upgrades\2026-10\d2um-underground-mines.md' }
	@{ Name = 'tbm-tunnel'; SaveName = 'underground-demo-tunnel-399';
		Snapshot = 'src\tests\fixtures\haulage\underground-demo-tunnel-399.sav';
		Text = 'TBM fixes: public A-to-B freight tunnel, same TBM stored at exit B';
		Guide = 'docs\zz-reach2\upgrades\2026-10\d2um-underground-mines.md' }
	@{ Name = 'underground-mines-legacy'; SaveName = 'underground-demo-tbm-ui-393';
		Snapshot = 'src\tests\fixtures\haulage\underground-demo-tbm-ui-393.sav';
		Text = 'Older map: preserved bent-route checkpoint for comparison';
		Guide = 'docs\zz-reach2\upgrades\2026-10\d2um-underground-mines.md' }
)

#The saves we have, with what each holds. They predate the gameplay profiles, so they load as Classic games.
$knownSaves = @(
	@{ Name = 'makinpr2-final-verification'; Text = 'your 256 x 256 game after the cross-layer train run (paused)' },
	@{ Name = 'makinpr2-connected'; Text = 'your game with connected platforms and path signals at a portal' },
	@{ Name = 'makinpr2-connected-graphics-tested'; Text = 'the same layout, with the traffic of the portal art checks' },
	@{ Name = 'makinpr2'; Text = 'your original supplied game, unchanged' }
)

<#
.SYNOPSIS Read the lines of a text file, or none when it is missing.
.PARAMETER Path File to read.
.OUTPUTS The lines, as a list that can be changed.

@param Path File to read.
@return The lines, as a list that can be changed.
#>
function Read-Lines([string]$Path) {
	$list = New-Object System.Collections.Generic.List[string]
	if (Test-Path -LiteralPath $Path) {
		foreach ($line in [System.IO.File]::ReadAllLines($Path)) { $list.Add($line) }
	}
	return ,$list
}

<#
.SYNOPSIS Find the lines of one group in an OpenTTD config.
.PARAMETER Lines Config lines.
.PARAMETER Group Group name without brackets, for example game_creation.
.OUTPUTS The index of the group's [name] line, -1 when the group is missing, and the index after its last line.

@param Lines Config lines.
@param Group Group name without brackets, for example game_creation.
@return The index of the group's [name] line, -1 when the group is missing, and the index after its last line.
#>
function Find-IniGroup($Lines, [string]$Group) {
	$start = -1
	for ($i = 0; $i -lt $Lines.Count; $i++) {
		$trimmed = $Lines[$i].Trim()
		# Only the requested header starts this section, keeping settings in
		# other sections from changing the player's chosen profile or map.
		if ($start -lt 0 -and $trimmed -eq "[$Group]") { $start = $i; continue }
		# Once found, stop at the next section so reads/writes remain scoped.
		if ($start -ge 0 -and $trimmed.StartsWith('[')) { return @($start, $i) }
	}
	return @($start, $Lines.Count)
}

<#
.SYNOPSIS Read one setting from an OpenTTD config.
.PARAMETER Path Config file.
.PARAMETER Group Group name without brackets.
.PARAMETER Key Setting name inside the group.
.PARAMETER Default Value when the file, the group or the key is missing.
.OUTPUTS The value as text.

@param Path Config file.
@param Group Group name without brackets.
@param Key Setting name inside the group.
@param Default Value when the file, the group or the key is missing.
@return The value as text.
#>
function Get-IniValue([string]$Path, [string]$Group, [string]$Key, [string]$Default) {
	$lines = Read-Lines $Path
	$range = Find-IniGroup $lines $Group
	if ($range[0] -lt 0) { return $Default }
	$pattern = '^\s*' + [regex]::Escape($Key) + '\s*=\s*(.*?)\s*$'
	# Read only this section's exact key, preserving the player's other settings.
	for ($i = $range[0] + 1; $i -lt $range[1]; $i++) {
		if ($lines[$i] -match $pattern) { return $Matches[1] }
	}
	return $Default
}

<#
.SYNOPSIS Count the entries of one group in an OpenTTD config.
.PARAMETER Path Config file.
.PARAMETER Group Group name without brackets, for example newgrf.
.OUTPUTS The number of lines in the group that are neither blank nor comments.

@param Path Config file.
@param Group Group name without brackets, for example newgrf.
@return The number of lines in the group that are neither blank nor comments.
#>
function Get-IniEntryCount([string]$Path, [string]$Group) {
	$lines = Read-Lines $Path
	$range = Find-IniGroup $lines $Group
	if ($range[0] -lt 0) { return 0 }
	$count = 0
	for ($i = $range[0] + 1; $i -lt $range[1]; $i++) {
		$trimmed = $lines[$i].Trim()
		# Only active entries matter for content compatibility; explanatory
		# comments must not trigger a false warning about unsupported NewGRFs.
		if ($trimmed -ne '' -and -not $trimmed.StartsWith(';') -and -not $trimmed.StartsWith('#')) { $count++ }
	}
	return $count
}

<#
.SYNOPSIS Set one setting of an OpenTTD config, adding the key or its group when missing.
.PARAMETER Path Config file; it is rewritten as UTF-8 without a byte order mark, and only when the value changes.
.PARAMETER Group Group name without brackets.
.PARAMETER Key Setting name inside the group.
.PARAMETER Value New value.

@param Path Config file; it is rewritten as UTF-8 without a byte order mark, and only when the value changes.
@param Group Group name without brackets.
@param Key Setting name inside the group.
@param Value New value.
@return No value; update the requested setting while preserving unrelated preferences.
#>
function Set-IniValue([string]$Path, [string]$Group, [string]$Key, [string]$Value) {
	$lines = Read-Lines $Path
	$range = Find-IniGroup $lines $Group
	$entry = "$Key = $Value"
	if ($range[0] -lt 0) {
		$lines.Add("[$Group]")
		$lines.Add($entry)
	} else {
		$pattern = '^\s*' + [regex]::Escape($Key) + '\s*='
		$found = $false
		# Replace the existing key in place to retain unrelated user preferences.
		for ($i = $range[0] + 1; $i -lt $range[1]; $i++) {
			if ($lines[$i] -match $pattern) {
				if ($lines[$i].Trim() -eq $entry) { return }
				$lines[$i] = $entry
				$found = $true
				break
			}
		}
		if (-not $found) { $lines.Insert($range[0] + 1, $entry) }
	}
	[System.IO.File]::WriteAllLines($Path, $lines, $utf8)
}

<#
.SYNOPSIS Create play.cfg with what a first game needs. An existing one is kept as the game last wrote it.
.PARAMETER Path Config file to create.

@param Path Config file to create.
@return No value; preserve an existing configuration or create the missing defaults.
#>
function Initialize-PlayConfig([string]$Path) {
	if (Test-Path -LiteralPath $Path) { return }
	#A 256 x 256 temperate map suits both profiles; the large loan pays for a machine and its receivers.
	$lines = @(
		'; Config of play.ps1, see docs/zz-reach2/upgrades/2026-10/r2mlvs-how-to.md. The game rewrites it when you quit.',
		'[game_creation]',
		'map_x = 8',
		'map_y = 8',
		'landscape = temperate',
		'gameplay_profile = multilayer_industry',
		'[difficulty]',
		'max_loan = 20000000'
	)
	[System.IO.File]::WriteAllLines($Path, [string[]]$lines, $utf8)
}

<#
.SYNOPSIS List the other saves in build\save and its autosave folder, newest first.
.PARAMETER Known Names of the saves the menu shows already, which are left out.
.PARAMETER Everything Also list the generated test fixtures, and all of the saves instead of the newest eight.
.OUTPUTS One object per save: its name relative to build\save without .sav, its file and its time.

@param Known Names of the saves the menu shows already, which are left out.
@param Everything Also list the generated test fixtures, and all of the saves instead of the newest eight.
@return One object per save: its name relative to build\save without .sav, its file and its time.
#>
function Get-OtherSave([string[]]$Known, [bool]$Everything) {
	if (-not (Test-Path -LiteralPath $saveDir)) { return @() }
	#Saves the map reviews and the save tests generated from seeds, rather than games anybody played.
	$fixture = '^(epic|upgraded|roundtrip|fd9|final|deep_upgraded|camera|toolbar|portals-camera)[-_]|^grftest$'
	$saves = foreach ($file in Get-ChildItem -LiteralPath $saveDir -Recurse -File -Filter *.sav) {
		$relative = $file.FullName.Substring($saveDir.Length + 1)
		$name = $relative.Substring(0, $relative.Length - $file.Extension.Length)
		#Desync dumps are no games to play, the known saves have their own entries, and fixtures come only on request.
		if ($file.BaseName -like 'dmp_cmds_*' -or $Known -contains $name) { continue }
		if (-not $Everything -and $file.BaseName -match $fixture) { continue }
		[pscustomobject]@{ Name = $name; File = $file.FullName; Time = $file.LastWriteTime }
	}
	$saves = @($saves | Sort-Object Time -Descending)
	if (-not $Everything) { $saves = @($saves | Select-Object -First 8) }
	return $saves
}

<#
.SYNOPSIS Make one menu entry.
.PARAMETER Section Heading the entry is listed under.
.PARAMETER Name Short name in the first column.
.PARAMETER Text What the entry starts.
.PARAMETER Profile Gameplay profile of a new game, or empty for a save.
.PARAMETER File Save to load, or empty for a new game.
.PARAMETER Guide Player guide for a featured demo, or empty for other entries.
.OUTPUTS The entry.

@param Section Heading the entry is listed under.
@param Name Short name in the first column.
@param Text What the entry starts.
@param Profile Gameplay profile of a new game, or empty for a save.
@param File Save to load, or empty for a new game.
@param Guide Player guide for a featured demo, or empty for other entries.
@return The entry.
#>
function New-Entry([string]$Section, [string]$Name, [string]$Text, [string]$Profile, [string]$File, [string]$Guide = '') {
	[pscustomobject]@{ Section = $Section; Name = $Name; Text = $Text; Profile = $Profile; File = $File; Guide = $Guide }
}

<#
.SYNOPSIS Print the numbered menu, one heading per section.
.PARAMETER Entries Menu entries in their order; entry 1 is the first.

@param Entries Menu entries in their order; entry 1 is the first.
@return No value; print the ordered choices with one heading for each section.
#>
function Show-Menu($Entries) {
	$width = ($Entries | ForEach-Object { $_.Name.Length } | Measure-Object -Maximum).Maximum
	$section = ''
	# Keep displayed numbers tied to the same ordered choices across headings.
	for ($i = 0; $i -lt $Entries.Count; $i++) {
		$entry = $Entries[$i]
		if ($entry.Section -ne $section) {
			$section = $entry.Section
			Write-Host ''
			Write-Host "  $section" -ForegroundColor Cyan
		}
		Write-Host (("  {0,3}  {1,-$width}  {2}") -f ($i + 1), $entry.Name, $entry.Text)
	}
	Write-Host ''
}

<#
.SYNOPSIS Turn the player's choice into a menu entry.
.PARAMETER Choice A menu number, demo alias, save name in build\save, or native save path.
.PARAMETER Entries Menu entries, numbered from 1.
.OUTPUTS The chosen entry, or nothing when the choice matches no entry and no save.

@param Choice A menu number, demo alias, save name in build\save, or native save path.
@param Entries Menu entries, numbered from 1.
@return The chosen entry, or nothing when the choice matches no entry and no save.
#>
function Resolve-Choice([string]$Choice, $Entries) {
	if ($Choice -match '^\d+$') {
		$number = [int]$Choice
		#Numbers count from 1 up to the last entry shown.
		if ($number -ge 1 -and $number -le $Entries.Count) { return $Entries[$number - 1] }
		return $null
	}
	# The short aliases and permanent name select the same featured checkpoint,
	# regardless of how many newer player saves have appeared since the last run.
	$menuName = $Choice
	if ($Choice -in 'demo', 'tbm-work') { $menuName = 'underground-mines' }
	foreach ($entry in $Entries) {
		if ($entry.Name -eq $menuName) { return $entry }
	}
	if ($menuName -eq 'underground-mines') {
		throw 'The Underground mines demo save is missing from build\save and src\tests\fixtures\haulage. See docs\zz-reach2\upgrades\2026-10\d2um-underground-mines.md.'
	}
	#A name is looked up in build\save first, then taken as a path of its own.
	foreach ($candidate in @((Join-Path $saveDir $Choice), (Join-Path $saveDir "$Choice.sav"), $Choice)) {
		if (Test-Path -LiteralPath $candidate -PathType Leaf) {
			$full = (Resolve-Path -LiteralPath $candidate).ProviderPath
			# Opening the demo by its file path retains the same walkthrough
			# and starting instructions as opening it by menu number or alias.
			foreach ($entry in $Entries) {
				if ($entry.File -eq $full) { return $entry }
			}
			return New-Entry '' ([System.IO.Path]::GetFileNameWithoutExtension($full)) '' '' $full
		}
	}
	return $null
}

<#
.SYNOPSIS Quote one native Windows argument, so that a path with spaces stays a single argument.
.PARAMETER Value Argument as the game should receive it.
.OUTPUTS The argument as it goes on the command line.

@param Value Argument as the game should receive it.
@return The argument as it goes on the command line.
#>
function ConvertTo-NativeArgument([string]$Value) {
	# Only simple nonempty values can skip quoting; paths with spaces must still
	# reach the game as one argument so it loads the intended save/configuration.
	if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') { return $Value }
	#Backslashes before a quote and at the closing quote are doubled, following native argv parsing.
	$escaped = [regex]::Replace($Value, '(\\*)"', '$1$1\"')
	$escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
	return '"' + $escaped + '"'
}

<#
.SYNOPSIS Find the newest source file of the game, to tell whether the build misses changes.
.OUTPUTS The newest file under src, the unit tests left out because the game does not contain them, or nothing.

@return The newest file under src, the unit tests left out because the game does not contain them, or nothing.
#>
function Get-NewestSource {
	$src = Join-Path $PSScriptRoot 'src'
	#Code, language and settings files outside the unit tests; -Include is unreliable with -Recurse in PowerShell 5.1.
	Get-ChildItem -LiteralPath $src -Recurse -File |
		Where-Object { $_.Extension -in '.cpp', '.h', '.hpp', '.txt', '.ini' -and $_.FullName -notlike "$src\tests\*" } |
		Sort-Object LastWriteTime -Descending | Select-Object -First 1
}

if (-not (Test-Path -LiteralPath $exe)) {
	throw "$exe is missing. Build the $Config game first (COMPILING.md)."
}
if (-not $DryRun) { Initialize-PlayConfig $playConfig }

#The new-game headings show the map that play.cfg asks for, since the game may have rewritten it since.
$mapX = 1 -shl [int](Get-IniValue $playConfig 'game_creation' 'map_x' '8')
$mapY = 1 -shl [int](Get-IniValue $playConfig 'game_creation' 'map_y' '8')
$climate = Get-IniValue $playConfig 'game_creation' 'landscape' 'temperate'
$seedText = 'a random map'
if ($Seed -ge 0) { $seedText = "seed $Seed" }

$entries = New-Object System.Collections.Generic.List[object]
$newSection = "New game, $mapX x $mapY, $seedText"
$entries.Add((New-Entry $newSection 'Multilayer Industry' 'solid rock below, dug by tunnel boring machines (temperate)' 'multilayer_industry' ''))
$entries.Add((New-Entry $newSection 'Classic' "the four-layer map with free building ($climate)" 'classic' ''))
# Featured demos stay ahead of the changing recent-save list; missing artifacts
# are not shown as launchable worlds and their explicit aliases fail clearly.
foreach ($demo in $demoSaves) {
	$file = Join-Path $PSScriptRoot $demo.Snapshot
	# Featured demos reopen their verified starting state. A player's later
	# same-named save cannot silently replace it; custom saves remain selectable by path.
	if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
		$file = Join-Path $saveDir "$($demo.SaveName).sav"
	}
	if (Test-Path -LiteralPath $file -PathType Leaf) {
		$entries.Add((New-Entry 'Demos (Multilayer Industry)' $demo.Name $demo.Text '' $file $demo.Guide))
	}
}
# Supplied legacy saves retain their existing Classic entries and descriptions.
foreach ($known in $knownSaves) {
	$file = Join-Path $saveDir "$($known.Name).sav"
	if (Test-Path -LiteralPath $file) {
		$entries.Add((New-Entry 'Saves we have (Classic games, so no digging)' $known.Name $known.Text '' $file))
	}
}
$otherSection = 'More saves in build\save, newest first'
if ($All) { $otherSection = 'All other saves in build\save, newest first' }
$listedSaves = @($knownSaves | ForEach-Object { $_.Name }) + @($demoSaves | ForEach-Object { $_.SaveName })
# Omit worlds already featured so one checkpoint has one selectable menu entry.
foreach ($other in @(Get-OtherSave $listedSaves $All.IsPresent)) {
	$entries.Add((New-Entry $otherSection $other.Name $other.Time.ToString('yyyy-MM-dd HH:mm') '' $other.File))
}

if ($Save -eq '') {
	Write-Host ("OpenTTD, {0} build of {1:yyyy-MM-dd HH:mm}" -f $Config, (Get-Item -LiteralPath $exe).LastWriteTime) -ForegroundColor Green
	Show-Menu $entries
	# Keep asking until an available world is selected; an invalid choice must
	# never silently start a fresh map instead of the demo the player wanted.
	while ($true) {
		$answer = (Read-Host 'Number (Enter for 1, q to quit)').Trim()
		if ($answer -eq 'q') { return }
		if ($answer -eq '') { $answer = '1' }
		$choice = Resolve-Choice $answer $entries
		if ($null -ne $choice) { break }
		Write-Host "There is no entry or save '$answer'." -ForegroundColor Yellow
	}
} else {
	$choice = Resolve-Choice $Save $entries
	if ($null -eq $choice) { throw "There is no menu entry or save '$Save'. Run .\play.ps1 alone to see the menu." }
}

$arguments = New-Object System.Collections.Generic.List[string]
$arguments.Add('-c')
$arguments.Add($playConfig)
if ($choice.Profile -ne '') {
	#A new game starts from play.cfg, so the profile goes there first; Multilayer Industry accepts only temperate.
	if (-not $DryRun) {
		Set-IniValue $playConfig 'game_creation' 'gameplay_profile' $choice.Profile
		if ($choice.Profile -eq 'multilayer_industry') { Set-IniValue $playConfig 'game_creation' 'landscape' 'temperate' }
	}
	$arguments.Add('-g')
	if ($Seed -ge 0) {
		$arguments.Add('-G')
		$arguments.Add([string]$Seed)
	}
} else {
	$arguments.Add('-g')
	$arguments.Add($choice.File)
	if ($Seed -ge 0) { Write-Host 'A save has its own map, so -Seed is left out.' -ForegroundColor Yellow }
}
foreach ($extra in $GameArgs) { $arguments.Add($extra) }
$commandLine = (@($arguments) | ForEach-Object { ConvertTo-NativeArgument $_ }) -join ' '

if ($choice.Guide -ne '') {
	Write-Host "Demo guide: $($choice.Guide)" -ForegroundColor Cyan
	Write-Host 'Starts paused. Inspect the portal and its TBM, then unpause. Save your experiments under a new name.'
}

#The Multilayer Industry profile refuses any NewGRF but its own package, and play.cfg keeps those picked in the game.
if ($choice.Profile -eq 'multilayer_industry' -and (Get-IniEntryCount $playConfig 'newgrf') -gt 0) {
	Write-Host 'play.cfg lists NewGRFs, which Multilayer Industry refuses. Remove them in the main menu''s NewGRF Settings.' -ForegroundColor Yellow
}
$startup = Join-Path $build 'scripts\game_start.scr'
# A nonempty startup script can alter the selected world immediately, so show
# its commands before launching rather than surprising the player in the demo.
if ((Test-Path -LiteralPath $startup) -and (Get-Item -LiteralPath $startup).Length -gt 0) {
	Write-Host "The game runs these console commands at the start, from $($startup):" -ForegroundColor Yellow
	Get-Content -LiteralPath $startup | ForEach-Object { Write-Host "    $_" -ForegroundColor Yellow }
}
$built = (Get-Item -LiteralPath $exe).LastWriteTime
$newest = Get-NewestSource
# Only a newer actual game input justifies the hint; an absent source result
# must not prevent launching a valid checkpoint build.
if ($null -ne $newest -and $newest.LastWriteTime -gt $built) {
	Write-Host ("The {0} build ({1:yyyy-MM-dd HH:mm}) is older than {2} ({3:yyyy-MM-dd HH:mm}); rebuild it for the latest changes." -f $Config, $built, $newest.FullName.Substring($PSScriptRoot.Length + 1), $newest.LastWriteTime) -ForegroundColor Yellow
}

if ($DryRun) {
	Write-Host "$exe $commandLine"
	return
}
Start-Process -FilePath $exe -ArgumentList $commandLine -WorkingDirectory $build
Write-Host "Started $($choice.Name) in the $Config build." -ForegroundColor Green
Write-Host 'Your saves go to build\save and show up in this menu the next time.'
if ($choice.Profile -eq 'multilayer_industry') {
	$loan = [long](Get-IniValue $playConfig 'difficulty' 'max_loan' '300000')
	Write-Host ("Money: in Finances, Ctrl+click Borrow takes the whole loan ({0:N0}); Ctrl+Alt+C opens the cheats." -f $loan)
	Write-Host 'What to build: docs\zz-reach2\upgrades\2026-10\r2mlvs-how-to.md'
}
