# Sound effects and music after a set is bound

This page is the playback detail behind [archi-audio.md](archi-audio.md): how a chosen sound set becomes mixer channels, how a chosen music set becomes a `MusicSongInfo` handed to `MusicDriver`, and how the two outputs meet.

Scanning, checksums, and the `.obs` / `.obm` text format are out of scope. Here the used set is already `BaseSounds::GetUsedSet()` or `BaseMusic::GetUsedSet()`.

## Sound effects

### Binding the samples file

`InitializeSound` (`src/sound.cpp`) logs and calls `OpenBankFile` with `BaseSounds::GetUsedSet()->files[0].filename`. That file is the set's `samples` entry (`BaseSet<SoundsSet>::GetFilenames()` returns that single name). An empty filename (a nosound configuration) returns immediately and leaves every slot empty.

`OpenBankFile` keeps one `RandomAccessFile` alive for the whole session (`original_sound_file`). It reads a dword count. Bit 31 set means `SoundSource::BasesetNewFormat` (Opus and WAV allowed); clear means `SoundSource::BasesetOldFormat` (legacy WAV quirks and one raw PCM sample). The count is divided by 8 and must equal `ORIGINAL_SAMPLE_COUNT` (73). Any other count is logged and ignored: the array stays zeroed, so later plays no-op, and NewGRF sound slots can still be allocated on top.

Each of the 73 records stores `file`, `file_offset`, `file_size`, and `source`. PCM is not decoded here.

`SndCopyToPool` (`InitializeSoundPool` in `src/newgrf_sound.cpp`, from NewGRF init) allocates 73 `SoundEntry` slots and copies them through `_sound_idx`. Baseset order and the `SoundFx` order used by the game and NewGRFs differ: the first two baseset samples land at pool index 39, and the enum names for `0x02`–`0x28` are off by two relative to the file. `_sound_base_vol` supplies the per-effect volume written onto the pool slot. `priority` is set to 0.

`ChangeSoundSet` points `BaseSounds` at the new set, closes every mixer channel, re-reads the bank, and overwrites pool slots whose `grf_container_ver` is 0. Slots a NewGRF replaced are left alone. A channel already in flight keeps a `shared_ptr` to its PCM, so the device thread is not left with a freed buffer.

### Decode on first play

`StartSound` calls `SetBankSource`. If `sound->file` is still set, `LoadSound` runs once:

| `SoundSource` | Path |
| --- | --- |
| `BasesetOldFormat` | Seek to `file_offset`, read the name byte string, `LoadSoundData(..., new_format=false)` |
| `BasesetNewFormat` | Same, with `new_format=true` |
| `NewGRF` | `LoadNewGRFSound` |

`LoadSoundData` checks `file_size`, allocates a `shared_ptr<vector<byte>>`, and walks `ProviderManager<SoundLoader>` until one `Load` returns true. Providers sort by ascending priority number (the opposite convention from driver autoprobe, where a higher number is tried first):

| Loader | Priority | Accepts |
| --- | --- | --- |
| `SoundLoader_Wav` | 0 | RIFF/WAVE, PCM format 1, mono, 8- or 16-bit. Old-format files are forced to 11025 Hz. 8-bit samples are converted from unsigned to signed. |
| `SoundLoader_Opus` | 10 | New format only, at least 57 bytes, Ogg Opus header. `op_read` decodes mono 16-bit at 48000 Hz. |
| `SoundLoader_Raw` | `INT_MAX` | Old format only. The whole blob is 8-bit mono at 11025 Hz (the non-RIFF "jackhammer" sample). Unsigned to signed, same as WAV. |

Each attempt seeks back to the start of the sound, so a WAV rejection does not consume the bytes Opus needs. An empty result logs and returns false. `SetBankSource` then clears `file` so a broken sample is not decoded again.

On success the loader has set `channels` (must be 1), `bits_per_sample` (8 or 16), and `rate`. If `rate` differs from `MxGetRate()`, `SoundResampler_Soxr` resamples. One extra sample is appended for the mixer's linear interpolator.

`MxSetChannelRawSrc` attaches that buffer. `MxSetChannelVolume` combines `sound->volume * caller_volume` with sinusoidal pan. `MxActivateChannel` sets the channel's bit in `_active_channels`.

### Where the sample is heard

World sounds (`SndPlayTileFx`, `SndPlayVehicleFx`) are silent unless some viewport contains the coordinates. Pan is the source's horizontal fraction across that viewport. Zoom attenuates via `_vol_factor_by_zoom` (full volume at the closest three steps, then 190, 134, and 8). GUI sounds (`SndPlayFx`) use pan 0.5 and full volume.

Eight channels is the cap. A ninth simultaneous effect returns from `MxAllocateChannel` and is discarded. There is no priority preemption: `SoundEntry::priority` is stored as 0 for baseset copies and is not consulted by `StartSound`.

The device side is always "fill this stereo buffer":

- SDL2 (`fill_sound_buffer`): `MxMixSamples(stream, len / 4)` because the device is stereo 16-bit, four bytes per frame. Requested rate defaults to 44100, buffer to 1024 frames (`hz`, `samples`).
- XAudio2: `StreamingVoiceContext::SubmitBuffer` mixes into a byte buffer of `samples * 4` and resubmits it from `OnBufferEnd`. The first buffer is submitted from `Start`. Failure to create the engine, mastering voice, or source voice returns an error string to the factory.
- Win32 WaveOut: the same mix into a wave header, from the WaveOut callback.
- Allegro: `SoundDriver_Allegro::MainLoop` pulls `get_audio_stream_buffer` and mixes when the stream wants data. `GameLoop` is what calls `MainLoop`.
- Null: `Start` succeeds, `MainLoop` is empty, nothing calls `MxMixSamples`.

## Music

### Binding the used set

`MusicSet::songinfo[NUM_SONGS_AVAILABLE]` is already filled when the set was loaded (`NUM_SONGS_AVAILABLE` is 31: one theme plus three classes of ten). Each occupied slot has a filename, a display name, a track number, and a `MusicTrackType`:

- `MTT_STANDARDMIDI` — the file is a Standard MIDI File.
- `MTT_MPSMIDI` — the file is a CAT and `cat_index` selects one entry, in the original MPS GM layout. Optional `override_start` / `override_end` are MIDI-tick trims.

`InitializeMusic` calls `MusicSystem::BuildPlaylists`. That copies `songinfo[i]` into a `PlaylistEntry` (the `MusicSongInfo` plus the `MusicSet*` and index) when `songname` is non-empty. Those entries are then placed into:

- index 0 → theme-only playlist (title screen)
- indexes 1–30 → "all", and into Old / New / Ezy by `(i - 1) / 10`
- custom playlists from `_settings_client.music.custom_1` and `custom_2` (stored 1-based; 0 means unused)

`ChangeMusicSet` calls `BaseMusic::SetSetByName`, rebuilds those playlists, and restarts the active playlist if playback is enabled. Switching sets does not change the `MusicDriver` instance.

### From playlist to driver

`MusicSystem::Play` always sets `_settings_client.music.playing` and calls `StopSong`, then `PlaySong` with the current `PlaylistEntry`. On the title screen the theme song is marked `loop`. An empty playlist returns after the stop.

`MusicLoop`, called at the end of `GameLoop`, runs `CheckStatus`: the title screen forces the theme playlist, and a playing flag with `IsSongPlaying() == false` advances to the next track and calls `Play` again.

Drivers consume `MusicSongInfo` in one of two ways.

**Parse in-process.** `MusicDriver_Win32::PlaySong` calls `MidiFile::LoadSong`. Failure returns without starting the timer. Success stores the parsed blocks and a multimedia timer drains them to `midiOut`. Volume and transport flags are mutex-guarded; the timer thread does not take the game lock. DirectMusic (`dmusic`, autoprobe priority 10 on Windows) is the other in-process Windows MIDI stack and is selected ahead of `win32` when both start.

**Standard MIDI file on disk.** FluidSynth, `extmidi`, Cocoa, Allegro, and BeOS call `MidiFile::GetSMFFile`. A `MTT_STANDARDMIDI` song resolves to a baseset or old-GM path. A `MTT_MPSMIDI` song is converted once and written under the autodownload baseset directory as `<cat_index>.mid`. An empty return means `PlaySong` stops and reports nothing playing.

FluidSynth is the driver that shares the sound device. `Start` creates the synth, loads a SoundFont (`soundfont=` parameter, otherwise FluidSynth's default, otherwise a short list of distro paths), and registers `RenderMusicStream` with `MxSetMusicSource`. The returned mixer rate is written into `synth.sample-rate`. `PlaySong` adds the SMF to a FluidSynth player. `Stop` clears the music callback before destroying the synth, so `MxMixSamples` cannot call into a freed player. The render callback try-locks; a busy synth skips music for that buffer.

`extmidi` forks a command (`cmd=`, otherwise `timidity`) and must be stopped while the sound driver still exists. `Driver::Type::Music` is enumerated before `Sound` in `ShutdownDrivers` for that reason. `MusicDriver_ExtMidi::Start` returns an error when the active video or sound driver is Allegro, which crashes that combination.

Null music implements `PlaySong` as an empty function and `IsSongPlaying` as true. `MusicLoop` therefore leaves the playlist where it is instead of treating every tick as a finished song. That still does not touch the simulation.

## Where the two outputs meet

| Output | Who produces PCM | Who consumes it |
| --- | --- | --- |
| Sound effects | `LoadSound` → `MixerChannel` | Every real sound driver, inside `MxMixSamples` |
| FluidSynth music | `fluid_synth_write_s16` via `MxSetMusicSource` | The same `MxMixSamples` buffer, written before effects are added |
| Win32 / DirectMusic / Cocoa / Allegro / BeOS / extmidi | The OS MIDI device or an external process | Speakers, bypassing the mixer |

Effect channels are added after the music callback, so a software-synth song sits under effects. A hardware MIDI song is a second device; its relative loudness is `MusicDriver::SetVolume` (the music slider), while effect loudness is `SetEffectVolume` inside the mixer. The two sliders meet in one buffer only for FluidSynth.

`MxInitialize`, called from the sound driver's `Start`, sets `_music_stream` back to null. A sound-driver restart invalidates a previously installed FluidSynth callback until that music driver starts again. The startup order in `openttd.cpp` (sound `SelectDriver`, then music `SelectDriver`) matches that constraint.
