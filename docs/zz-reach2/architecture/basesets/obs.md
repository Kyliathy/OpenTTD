# OBS (OpenTTD Base Sounds)

An `.obs` file is the metadata for one sound base set. The scanner is `BaseMedia<SoundsSet>::GetExtension` (`src/sound.cpp`), which returns `.obs`. Parsing is the shared `BaseSet<SoundsSet>::FillSetDetails` in `src/base_media_func.h`. `SoundsSet` does not override it. Empty filenames are allowed (`allow_empty_filename` defaults to true). The only slot name is `samples` (`BaseSet<SoundsSet>::GetFilenames`). The legacy example is `docs/obs_format.txt`.

Startup selection is in [archi-basesets.md](archi-basesets.md).

## Sections

| Section | Required to accept the file |
| --- | --- |
| `[metadata]` | Yes. |
| `[files]` | Yes. The key `samples` must exist. An empty value is a valid silent set. |
| `[md5s]` | Required only when `samples` names a file. |
| `[origin]` | No. Missing text logs at `Error` and continues. |

There is no `[names]`, `[catindex]`, or `[timingtrim]` section. Those belong to `.obm` files.

## `[metadata]`

| Key | Required | What the parser does |
| --- | --- | --- |
| `name` | Yes, non-empty | `SoundsSet::name`. Matched by `-S` and by `[misc]` `soundsset`. Length is not checked. |
| `shortname` | Yes, non-empty | First four bytes, little-endian `uint32_t` (`DecodeShortName`). Longer values are truncated. |
| `version` | Yes, non-empty | Dot-separated base-10 `uint32` components, compared as a vector. The legacy text says "read as single integer". |
| `description` | Yes, non-empty | Fallback description. |
| `description.<language>` | No | Same lookup as graphics: full ISO code, then the first two characters, then the bare `description`. `description.en_US` does not cover `en_GB`. The legacy example says it does. |
| `url` | No | Stored when the key is present. |
| `fallback` | No | Any present value other than `0` or `false` marks the set as a last resort. |

`palette` and `blitter` are not read for sound sets. `GraphicsSet::FillSetDetails` is the only reader of those keys.

`media/baseset/no_sound.obs` is the shipped silent set: `name = NoSound`, `shortname = NULL`, `fallback = true`, and `samples` with an empty value. That empty value increments `found_files` and `valid_files` and skips the MD5.

## `[files]`

| Key | Payload |
| --- | --- |
| `samples` | One sample bank. The filename is case sensitive. The MD5 key is that filename. |

`docs/obs_format.txt` calls this section "files that replace sprites". The code has a single sample file. The sprite wording is leftover from the graphics text.

`InitializeSound` calls `OpenBankFile` on `files[0]`. An empty filename returns immediately and loads nothing. When a file is opened:

- The first dword is a count.
- Bit 31 set selects `SoundSource::BasesetNewFormat` (the comment in `src/sound_type.h`: Opus and WAV). Bit 31 clear selects `SoundSource::BasesetOldFormat` (original TTD, "incorrect frequency, WAV without headers").
- Bit 31 is cleared and the value is divided by 8. The result must equal `ORIGINAL_SAMPLE_COUNT` (73, `src/sound_type.h`).
- Each entry is an offset (low 31 bits, plus the file position) and a size.

A count other than 73 logs at `Debug2` on facility `misc` and leaves the bank empty. The `.obs` stays selected. `docs/obs_format.txt` says the file "Must contain exactly 73 samples". That is the load-time check, not a metadata-parse failure.

The MD5 is the whole file (`BaseSet::CheckMD5` with `SIZE_MAX`), 32 hex digits. A plain `md5sum` matches, which is what the legacy text says. A mismatch still adds the set. Shortname `TTDO` logs the mismatch or the missing file at `Error` rather than `Critical`. `orig_dos.obs` and `orig_win.obs` in `media/baseset/` both use `TTDO` and different `SAMPLE.CAT` checksums, so only one of them can be the valid file. The other remains a duplicate with fewer `valid_files`.

The in-game sound order is not the bank order. `src/sound_type.h` documents that the first two bank effects are inserted at position 39 via `_sound_idx` in `src/sound.cpp`. The `.obs` file does not carry that map.

## `[origin]`

Per-filename string, otherwise `default`. Shown when `CheckExternalFiles` reports a corrupt or missing `samples` file.

## Config

`soundsset` under `[misc]` is the current setting (`src/table/settings/misc_settings.ini`, variable `BaseSounds::ini_set`). `-S` overrides it. Both match `name`, case sensitive. The legacy text is right about this key. It is not right about graphics, which has moved to a `[graphicsset]` group.

## Stale claims in `docs/obs_format.txt`

- `version` is a vector of integers, not a single integer only.
- The `en_US` / `en_GB` description example does not match `GetDescription`.
- `openttd -d grf=1` does not show these parse logs. Facility is `misc`. Critical logs show by default.
- `openttd -h` lists every parsed `.obs`, then marks missing files unusable and MD5 mismatches corrupt.
- A missing MD5 rejects the set (`return false`). It is not a warning left on a loaded set.
- An empty `samples` value does not need an MD5. The legacy line "you have to fill the MD5 checksum for each file" does not cover that slot.
- `[origin]` may be omitted.
- "Files that replace sprites" is the wrong payload. The key is `samples`.
- "Exactly 73 samples" is enforced in `OpenBankFile`. A wrong count does not drop the `.obs`.
- The legacy text does not mention the bit-31 new bank format. The loader does.
