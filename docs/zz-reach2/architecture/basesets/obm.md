# OBM (OpenTTD Base Music)

An `.obm` file is the metadata for one music base set. The scanner is `BaseMedia<MusicSet>::GetExtension` (`src/music.cpp`), which returns `.obm`. `MusicSet::FillSetDetails` calls the shared reader, then fills `MusicSongInfo` from `[names]`, `[catindex]`, and `[timingtrim]`. Empty filenames are allowed. Music is not scanned inside tar archives (`BaseSetTraits<MusicSet>::search_in_tars` is false). The legacy example is `docs/obm_format.txt`.

Startup selection is in [archi-basesets.md](archi-basesets.md).

## Sections

| Section | Required to accept the file |
| --- | --- |
| `[metadata]` | Yes. |
| `[files]` | Yes. All 31 keys below must exist. An empty value means "no song" and counts as a present, valid file. |
| `[md5s]` | Required for each distinct non-empty filename. A shared CAT is listed once. |
| `[names]` | Required for each standard MIDI file that survived the MD5 check. Not required for empty slots or for CAT slots. A missing standard-MIDI title rejects the whole `.obm`. |
| `[catindex]` | No. Present keys select MPS MIDI inside a DOS CAT. |
| `[timingtrim]` | No. Invalid values are ignored. |
| `[origin]` | No. Missing text logs at `Error` and continues. |

## `[metadata]`

| Key | Required | What the parser does |
| --- | --- | --- |
| `name` | Yes, non-empty | `MusicSet::name`. Matched by `-M` and by `[misc]` `musicset`. |
| `shortname` | Yes, non-empty | First four bytes, little-endian `uint32_t`. |
| `version` | Yes, non-empty | Dot-separated base-10 `uint32` components. The legacy text says "read as single integer". |
| `description` | Yes, non-empty | Fallback description. |
| `description.<language>` | No | Full ISO code, then the first two characters, then the bare `description`. The legacy `en_US`-covers-`en_GB` example is wrong. |
| `url` | No | Stored when the key is present. |
| `fallback` | No | Any present value other than `0` or `false`. |

`media/baseset/no_music.obm` leaves every `[files]` value empty (`name = NoMusic`, `shortname = NULL`, `fallback = true`). That set has `num_available == 0` and is a valid fallback.

Original shortnames that downgrade missing-file logs to `Error`: `TTDD` (DOS music, also used by DOS graphics), `TTDW` (Windows music), `TTOD` (TTO music). `TTDO` is the sound shortname; it is in the same `if` in `FillSetDetails` because the function is shared.

## `[files]`

`NUM_SONGS_AVAILABLE` is 31 (`src/base_media_music.h`): one theme plus `NUM_SONG_CLASSES` (3) times `NUM_SONGS_CLASS` (10). The keys in `_music_file_names` are:

| Keys | Playlist (`MusicSystem::BuildPlaylists`) |
| --- | --- |
| `theme` | Theme only. Track number 0. Not added to All, Old style, New style, or Ezy Street. |
| `old_0` … `old_9` | Old style, and All. |
| `new_0` … `new_9` | New style, and All. |
| `ezy_0` … `ezy_9` | Ezy Street, and All. |

A missing key rejects the set (`files.<key> field missing`). A key with an unquoted empty value is stored as no file, and both `found_files` and `valid_files` increase. Several slots may name the same CAT; the MD5 is keyed by that filename once.

After the shared reader accepts the file, each slot is turned into a song only when the filename is non-empty and the MD5 result is not `NoFile`. A checksum mismatch still has a filename, so the song is still considered. `num_available` counts songs that received a name.

## `[catindex]`

The key is the `[files]` slot name (`theme`, `old_0`, …), not the filename. A non-empty integer selects `MTT_MPSMIDI`. The displayed name is `GetMusicCatEntryName` for that index inside the CAT. Index 0 is valid (`media/baseset/orig_dos.obm` sets `theme = 0`). Absence of the key selects `MTT_STANDARDMIDI`.

A non-integer index, or an index the CAT does not contain, logs at `Critical` and skips that song. The `.obm` stays loaded. The legacy text says CAT songs do not need `[names]`; the parser matches that.

## `[names]`

The key is the filename as written in `[files]`. `MusicSet::FillSetDetails` strips leading path separators from the stored path until a `[names]` key matches, so a directory prefix added by `AddFile` still finds the name from the `.obm`.

For `MTT_STANDARDMIDI`, a missing or empty name logs "Base music set song name missing" and `FillSetDetails` returns false. The set is not added. `docs/obm_format.txt` says a missing name produces warnings at startup and still implies the set loads. The code rejects the set instead.

## `[timingtrim]`

The key is the same filename key used for `[names]`, after the path strip. The value is `start:end`, both base-10 integers, with nothing after the end. On success, `override_start` and `override_end` are set. Any other shape is ignored and the song still loads. `override_end` of 0 means no end override (`MusicSongInfo` in `src/base_media_music.h`).

`MusicDriver_Win32::PlaySong` and `MusicDriver_DMusic::PlaySong` copy the two fields. FluidSynth, ExtMidi, Cocoa, Allegro, and BeMidi play through `MidiFile::GetSMFFile`, which does not read them. The legacy sentence "Not all music drivers might support this feature" still matches the code.

## `[md5s]` and `[origin]`

MD5 is the whole file, 32 hex digits. A missing or malformed checksum rejects the set before song names are read. `[origin]` is the filename, then `default`, and is optional.

There is no `loop` key. `MusicSongInfo::loop` is set by the music GUI when the theme playlist plays on the main menu (`src/music_gui.cpp`), not by the `.obm`.

## Config

`musicset` under `[misc]` is the current setting (`BaseMusic::ini_set`). `-M` overrides it. Both match `name`. `docs/obm_format.txt` is right about the section and the key, and wrong in the next words: it says this makes OpenTTD start with "that sound set".

## Stale claims in `docs/obm_format.txt`

- `version` is a vector of integers, not only a single integer.
- The `en_US` / `en_GB` description example does not match `GetDescription`.
- `openttd -d grf=1` does not show these parse logs. Facility is `misc`.
- `openttd -h` lists every parsed `.obm`, including incomplete ones.
- A missing MD5 rejects the set. It is not a pile of warnings on a loaded set.
- Empty slots do not need an MD5 or a `[names]` line. The "for each file" sentences do not cover them.
- A standard MIDI file with no `[names]` entry drops the whole set. It does not warn and continue.
- A bad `[catindex]` skips that song and keeps the set. The legacy text does not say this.
- `musicset` selects the music set. The sentence that says "sound set" is a copy error.
- `[origin]` may be omitted.
- `[timingtrim]` support is still limited to the Win32 and DirectMusic drivers. That legacy sentence is current.
