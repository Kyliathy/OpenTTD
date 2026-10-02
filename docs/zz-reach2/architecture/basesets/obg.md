# OBG (OpenTTD Base Graphics)

An `.obg` file is the metadata for one graphics base set. The scanner is `BaseMedia<GraphicsSet>::GetExtension` (`src/gfxinit.cpp`), which returns `.obg`. Parsing is `GraphicsSet::FillSetDetails` in the same file, which calls `BaseSet<GraphicsSet>::FillSetDetails` in `src/base_media_func.h` with empty filenames forbidden. The legacy example is `docs/obg_format.txt`. Keys below are the ones those functions read. Unknown keys are ignored.

Startup selection, duplicates, and what happens when no set exists are in [archi-basesets.md](archi-basesets.md).

## Sections

| Section | Required to accept the file |
| --- | --- |
| `[metadata]` | Yes. If the group is missing, the set is rejected. |
| `[files]` | Yes. Each key in the table below must exist and have a value. |
| `[md5s]` | Yes for every filename named in `[files]`. |
| `[origin]` | No. A missing entry logs at `Error` and clears that file's warning text. The set is still added. |

## `[metadata]`

| Key | Required | What the parser does |
| --- | --- | --- |
| `name` | Yes, non-empty | Stored as `GraphicsSet::name`. This is the `-I` and config name. Length is not checked. `docs/obg_format.txt` says "preferably less than 16 characters"; that is advice only. |
| `shortname` | Yes, non-empty | `DecodeShortName` keeps the first four bytes and packs them little-endian into `uint32_t`. A longer value is truncated. A shorter non-empty value is kept. The legacy text says the short name is 4 characters and does not mention truncation. |
| `version` | Yes, non-empty | One or more base-10 `uint32` values separated by `.`, stored in `version`. Compared later as a vector. The legacy text says "read as single integer". A single integer still parses. `1.2` is also valid. A trailing dot is rejected. |
| `description` | Yes, non-empty | The fallback description, stored under an empty language key. |
| `description.<language>` | No | Any key that starts with `description.` (the prefix is 12 characters). The suffix is the language key, case sensitive. `GetDescription` tries the full ISO code, then the first two characters, then the bare `description`. The legacy example says `description.en_US` is used for `en_GB`. It is not, unless a `description.en` key exists. |
| `palette` | Yes, non-empty | Read only by `GraphicsSet::FillSetDetails`, after the shared reader succeeds. First character `D` or `d` is `PaletteType::DOS`. Any other first character is `PaletteType::Windows`. `DOS` and `Windows` from the legacy text both work. The check is not a closed list. |
| `blitter` | No | First character `3` is `BLT_32BPP` (the enum comment: the set has 8bpp and 32bpp sprites). Anything else, including a missing key, is `BLT_8BPP`. `32bpp` and `8bpp` from the legacy text match that rule. |
| `url` | No | When the key is present, the value is stored on `url`. The options UI opens it in a browser. |
| `fallback` | No | Present, and the value is not exactly `0` or `false`, means this set is used only when a better set is absent. Missing means not a fallback. `false` is case sensitive. |

`GetMandatoryItem` rejects a missing key and a key whose value is empty. An unquoted empty value is an empty optional in `IniLoadFile::LoadFromDisk` (`src/ini_load.cpp`), so it fails the mandatory check.

## `[files]`

Keys are `_graphics_file_names` in `src/gfxinit.cpp`, in `GraphicsFileType` order. Each value is a filename, case sensitive, relative to the metadata file's directory. The MD5 key in `[md5s]` is that filename string, not the slot name.

| Key | `GraphicsFileType` | Load |
| --- | --- | --- |
| `base` | `Base` | `LoadGrfFile` at sprite 0. Old-style GRF, not a user NewGRF. |
| `logos` | `Logos` | `LoadGrfFile` at sprite 4793. |
| `arctic` | `Arctic` | `LoadGrfFileIndexed` when the landscape is arctic. |
| `tropical` | `Tropical` | `LoadGrfFileIndexed` when the landscape is tropical. |
| `toyland` | `Toyland` | `LoadGrfFileIndexed` when the landscape is toyland. |
| `extra` | `Extra` | NewGRF, loaded beside the hardcoded fallback `OPENTTD.GRF`, then both are removed from `_grfconfig`. |

Temperate games load `base`, `logos`, and `extra` only. The climate file overwrites some temperate sprites when the landscape is not temperate (`LoadSpriteTables`).

`docs/obg_format.txt` says `extra` must be a NewGRF using Actions 5, 7, 9, and A, with a GRFID starting with `FF`, so it cannot be selected as a user NewGRF. The `.obg` parser does not check actions or the GRFID. Action 8 does: a GRFID whose first byte is `0xFF` sets `GRFConfigFlag::System` (`src/newgrf/newgrf_act8.cpp`), and `FillGRFDetails` (`src/newgrf_config.cpp`) then refuses the file as a user NewGRF.

The MD5 is 32 hex digits (`MD5_HASH_BYTES` is 16). `ConvertHexToBytes` requires the hex length to be exactly twice the hash size. For a container version 2 GRF the hash covers the data section only (`GraphicsSet::CheckMD5`, `GRFGetSizeOfDataSection`). Otherwise the whole file is hashed. A mismatch logs and still adds the set (`found_files` increases, `valid_files` does not). A missing file does the same with `found_files` left unchanged, so `DetermineBestSet` will skip the set. For shortnames `TTDD` and `TTDW` those log lines are `Error` instead of `Critical`.

## `[origin]`

Lookup is the payload filename, then the key `default`. The string is `MD5File::missing_warning`, shown by `CheckExternalFiles` and by `-h`. It is not required for the set to parse.

## Config keys that are not in the `.obg`

`docs/obg_format.txt` says `graphicsset = <name>` under `[misc]` selects the set. `GraphicsSetLoadConfig` still reads that, then overwrites it from `[graphicsset]`:

| Key | Meaning |
| --- | --- |
| `name` | Same string as metadata `name`. |
| `shortname` | Eight hex digits. Saved with `byteswap` of the packed shortname (`GraphicsSetSaveConfig`). Loaded only when the value length is exactly 8. |
| `extra_version` | Version of the extra NewGRF, not the `.obg` `version`. |
| `extra_params` | Parameter list for the extra NewGRF. Applied on the shortname startup path when `GRFConfig::IsCompatible` accepts `extra_version`. |

`-I` matches `name` and does not apply `extra_params`.

## Stale claims in `docs/obg_format.txt`

- `version` is not limited to a single integer.
- `description.en_US` is not the `en_GB` translation. Lookup is full code, then two characters, then the bare description.
- `openttd -d grf=1` does not enable these parse logs. They use facility `misc`. Critical lines show at the default debug level.
- `openttd -h` lists parsed sets, including ones with missing or corrupt files.
- A missing MD5 rejects the file. It does not keep the set and warn.
- `palette` and `blitter` are first-character tests, not the two-word enums the text lists. The documented words still match.
- `graphicsset` under `[misc]` is the legacy read. The saved group is `[graphicsset]`.
- The `FF` GRFID rule is enforced when the extra file is scanned as a NewGRF, not while the `.obg` is parsed.
