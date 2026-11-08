//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// SoundClip - an interface for an audio playback configuration and control.
//
// SoundClip plays two-part role:
// - as a link between audioplayer api in script
//       and playback logic in the engine;
// - as a link between playback logic and the audio backend,
//       with which it syncs at certain times (see below).
// 
// SoundClip's state and parameter updates sync with the audio core in
// batches, only when the engine updates the game, never while the user script
// is being executed. The sync is performed by calling update().
// This is to ensure that the clip reference, state and properties don't change
// in the middle of the script's command sequence.
//
// SoundClip features two position units for pos telling and seek:
// one is milliseconds, and another a sound type specific position, which is:
//  * MIDI - the beat number
//  * MOD / XM / S3M - the pattern number
//  * WAV / VOC - the sample number
//  * OGG / MP3 / FLAC - milliseconds
//
// TODO: one of the biggest problems with sound clips currently is that it
// provides several methods of applying volume, which may ignore or override
// each other, and does not shape a consistent interface.
// Improving this situation is only possible with massive refactory of
// sound clip use, taking backwards-compatible audio system in account.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__SOUNDCLIP_H__
#define __AGS_EE_MEDIA__SOUNDCLIP_H__
#include "ac/dynobj/scriptaudioclip.h"
#include "media/audio/audiodefines.h"
#include "util/geometry.h"
#include "util/string.h"

using namespace AGS::Common;

class SoundClip final
{
public:
    SoundClip(int slot, AudioFileType snd_type, bool loop);
    ~SoundClip();

    bool Play();
    bool PlayFrom(int position);
    void Pause();
    void Resume();
    // Seeks to the position, where pos units depend on the audio type
    void Seek(int pos);
    // Seeks to the position in milliseconds
    void SeekMs(int pos_ms);
    // Synchronize this SoundClip with the audio subsystem:
    // - start scheduled playback;
    // - apply all accumulated sound parameters;
    // - read and save current position;
    // Returns if the clip is still playing, otherwise it's finished
    bool Update();

    // Gets ID of the source audio clip
    inline int GetSourceClipID() const { return _sourceClipID; }
    // Gets ID of the audio type, associated with the source clip
    inline int GetSourceAudioType() const { return _sourceClipType; }
    // Gets the source filename (if available)
    inline const String &GetFileName() const { return _fileName; }
    // Gets the source bundling type (if available)
    inline AudioClipBundle GetBundlingType() const { return _bundlingType; }
    // Gets handle of a managed object, referencing this playback from script
    inline int GetScriptHandle() const { return _scHandle; }
    // Gets ID of this clip in the game's list of active playbacks
    inline int GetAudioPlaybackID() const { return _audioPlaybackID; }
    // Gets ID of the audio channel this clip is playing on
    inline int GetAudioChannelID() const { return _audioChannelID; }
    // Gets this playback's priority
    inline int GetPriority() const { return _priority; }
    // Sets playback's clip reference info
    void SetClipInfo(int clip_id, int audio_type, const String &filename, AudioClipBundle bundle_type);
    // Set playback's script handle and channel reference
    void SetPlaybackInfo(int sc_handle, int play_id, int ch_id, int priority);

    // Gets if the playback is valid (playing or ready to play)
    inline bool IsReady() const
    {
        return IsPlaybackReady(_state);
    }
    // Gets if the playback is currently paused
    inline bool IsPaused() const
    {
        return _state == PlayStatePaused;
    }
    // Gets if the playback is looped
    inline bool IsRepeat() const { return _repeat; }
    inline void SetRepeat(bool repeat) { _repeat = repeat; }

    // Get sound file format type
    inline AudioFileType GetSoundType() const { return _soundType; }
    // Gets clip's volume property, as percentage (0 - 100);
    // note this may not be the real volume of playback (which could e.g. be muted)
    inline int GetVolume100() const { return _vol100; }
    // Gets clip's volume measured in 255 units
    inline int GetVolume255() const { return _vol255; }
    // Gets clip's panning (-100 - +100)
    inline int GetPanning() const { return _panning; }
    // Gets clip's playback speed in clip ms per real second
    inline int GetSpeed() const { return _speed; }
    // Gets if clip is muted (without changing the volume setting)
    inline bool IsMuted() const { return _muted; }

    // Gets current position in clip's type meaning
    int GetPos() { return _pos; }
    // Gets current position in ms
    int GetPosMs() { return _posMs; }
    // Gets total clip length in ms (or 0)
    int GetLengthMs() { return _lengthMs; }

    // Sets the current volume property, as percentage (0 - 100).
    inline void SetVolume100(int volume)
    {
        volume = Math::Clamp(volume, 0, 100);
        _vol100 = volume;
        _vol255 = (volume * 255) / 100;
        _paramsChanged = true;
    }
    // Sets the current volume property in units of 255
    inline void SetVolume255(int volume)
    {
        volume = Math::Clamp(volume, 0, 255);
        _vol255 = volume;
        _vol100 = (_vol255 * 100) / 255;
        _paramsChanged = true;
    }
    // Explicitly defines both percentage and 255-based volume values,
    // without calculating it from given percentage.
    inline void SetVolumeDirect(int vol_percent, int vol_absolute)
    {
        _vol255 = Math::Clamp(vol_absolute, 0, 255);
        _vol100 = Math::Clamp(vol_percent, 0, 100);
        _paramsChanged = true;
    }
    // Mutes sound clip, while preserving current volume property
    // for the future reference; when unmuted, that property is
    // used to restart previous volume.
    inline void SetMute(bool enable)
    {
        _muted = enable;
        _paramsChanged = true;
    }
    // Apply arbitrary permanent volume modifier, in absolute units (0 - 255);
    // this is distinct value that is used in conjunction with current volume
    // (can be both positive and negative).
    inline void ApplyVolumeModifier(int mod)
    {
        _volModifier = Math::Clamp(mod, 0, 255);
        _paramsChanged = true;
    }
    // Apply permanent directional volume modifier, in absolute units (0 - 255)
    // this is distinct value that is used in conjunction with current volume
    // (can be both positive and negative).
    inline void ApplyDirectionalModifier(int mod)
    {
        _directionalVolModifier = Math::Clamp(mod, 0, 255);
        _paramsChanged = true;
    }

    inline const Point &GetEmitterPosition() const { return _emitterPos; }
    inline int GetEmitterMaxDistance() const { return _maximumPossibleDistanceAway; }
    inline void SetEmitterPosition(int x, int y, int maxDistAway)
    {
        _emitterPos = Point(x, y);
        _maximumPossibleDistanceAway = maxDistAway;
    }
    inline void SetEmitterPosition(const Point &pos, int maxDistAway)
    {
        _emitterPos = pos;
        _maximumPossibleDistanceAway = maxDistAway;
    }

    inline void SetPanning(int newPanning)
    {
        _panning = Math::Clamp(newPanning, -100, 100);
        _paramsChanged = true;
    }

    inline void SetSpeed(int new_speed)
    {
        _speed = std::max(0, new_speed);
        _paramsChanged = true;
    }

private:
    // helper function for calculating volume with applied modifiers
    inline int GetFinalVolume() const
    {
        if (_muted) return 0;
        int final_vol = _vol255 + _volModifier + _directionalVolModifier;
        return final_vol >= 0 ? final_vol : 0;
    }

    int PosmsToPos(int pos_ms);
    int PosToPosms(int pos);

    // Script handle for AudioPlayback object
    int _scHandle = 0;
    // Audio playback ID (this is an index in audio system, here for the reference)
    int _audioPlaybackID = -1;
    // Audio channel's ID, where this playback is placed on.
    int _audioChannelID = -1;
    // Audio clip's ID
    int _sourceClipID = -1;
    // Audio type ID, associated with the source clip
    int _sourceClipType = AUDIOTYPE_UNDEFINED;
    // Filename and bundling type, in case this is not a common audio clip
    String _fileName;
    AudioClipBundle _bundlingType = kAudioBundle_Undefined;
    // Type of file format, used for the reference, and when applying
    // format-dependent seek and position
    AudioFileType _soundType = eAudioFileUndefined;
    bool _repeat = false;
    int _lengthMs = 0;
    // Relative playback's priority (0-100, 50 being default)
    int _priority = 50;
    // Panning and room position
    Point _emitterPos = Point(-1, -1);
    int _maximumPossibleDistanceAway = 0;

    // Audio core slot handle, link to the audio backend
    const int _slot = -1;
    // Frequency, needed for position handling
    int freq = 0;
    // Current playback _state
    PlaybackState _state = PlayStateInvalid;
    // Last synced position, in units respective to the file format type
    int _pos = 0;
    // Last synced position, in ms
    int _posMs = 0;
    // Whether playback parameters changed and have to be reapplied
    bool _paramsChanged = false;
    // current volume, in legacy units of 255
    int _vol255 = 0;
    // current volume, in percents
    int _vol100 = 0;
    // volModifier is used when there's a need to temporarily change and
    // the restore the clip's absolute volume (vol)
    int _volModifier = 0;
    // directional modifier is used to simulate positional sound origin
    int _directionalVolModifier = 0;
    // sound panning, in offset percents, from -100 to +100
    int _panning = 0;
    // speed of playback, in clip ms per real second (1000 is default)
    int _speed = 1000;
    // mute mode overrides the volume; if set, any volume assigned is stored
    // in properties, but not applied to playback itself
    bool _muted = false;
};

#endif // __AGS_EE_MEDIA__SOUNDCLIP_H__
