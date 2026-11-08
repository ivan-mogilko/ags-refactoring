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
#include <cmath>
#include "ac/common_defines.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "media/audio/soundclip.h"
#include "media/audio/audio_core.h"

SoundClip::SoundClip(int slot, AudioFileType snd_type, bool loop)
    : _slot(slot)
    , _soundType(snd_type)
    , _repeat(loop)
{
    _state = PlaybackState::PlayStateInitial;
    _pos = _posMs = -1;
    _paramsChanged = true;

    const auto player = audio_core_get_player(slot);
    _lengthMs = (int)std::round(player->GetDurationMs());
    freq = static_cast<int>(player->GetFrequency());
}

SoundClip::~SoundClip()
{
    if (_slot >= 0)
        audio_core_slot_stop(_slot);
}

void SoundClip::SetClipInfo(int clip_id, int audio_type, const String &filename, AudioClipBundle bundle_type)
{
    _sourceClipID = clip_id;
    _sourceClipType = audio_type;
    _fileName = filename;
    _bundlingType = bundle_type;
}

bool SoundClip::Play()
{
    if (!IsReady())
        return false;
    _state = PlaybackState::PlayStatePlaying;
    return true;
}

bool SoundClip::PlayFrom(int position)
{
    Seek(position);
    return Play();
}

void SoundClip::Pause()
{
    if (!IsReady())
        return;
    auto player = audio_core_get_player(_slot);
    player->Pause();
    _state = player->GetPlayStateNormal();
}

void SoundClip::Resume()
{
    if (!IsReady())
        return;
    _state = PlaybackState::PlayStatePlaying;
}

int SoundClip::PosmsToPos(int pos_ms)
{
    switch (_soundType)
    {
    case eAudioFileWAV: // Pos is in samples
        return static_cast<int>((static_cast<int64_t>(pos_ms) * freq) / 1000);
    case eAudioFileMIDI: /* TODO: reimplement */
    case eAudioFileMOD:  /* TODO: reimplement */
        return 0;  /* better say that it does not work than return wrong value */
    default:
        return pos_ms;
    }
}

int SoundClip::PosToPosms(int pos_)
{
    switch (_soundType)
    {
    case eAudioFileWAV: // Pos is in samples
        return static_cast<int>((static_cast<int64_t>(pos_) * 1000) / freq);
    case eAudioFileMIDI: /* TODO: reimplement */
    case eAudioFileMOD:  /* TODO: reimplement */
        return 0;  /* better say that it does not work than return wrong value */
    default:
        return pos_;
    }
}

void SoundClip::Seek(int pos_)
{
    // TODO: we probably would need a separate implementation eventually
    SeekMs(PosToPosms(pos_));
}

void SoundClip::SeekMs(int pos_ms)
{
    if (_slot < 0) { return; }
    auto player = audio_core_get_player(_slot);
    player->Pause();
    // TODO: for backward compatibility and MOD/XM music support
    // need to reimplement seeking to a position which units
    // are defined according to the sound type
    player->Seek((float)pos_ms);
    float posms_f = player->GetPositionMs();
    _posMs = static_cast<int>(posms_f);
    _pos = PosmsToPos(_posMs);
}

bool SoundClip::Update()
{
    if (!IsReady()) return false;

    auto player = audio_core_get_player(_slot);
    if (_paramsChanged)
    {
        auto vol_f = static_cast<float>(GetFinalVolume()) / 255.0f;
        if (vol_f < 0.0f) { vol_f = 0.0f; }
        if (vol_f > 1.0f) { vol_f = 1.0f; }

        auto speed_f = static_cast<float>(_speed) / 1000.0f;
        if (speed_f <= 0.0) { speed_f = 1.0f; }

        // Sets the pan position, ranging from -100 (left) to +100 (right)
        auto panning_f = (static_cast<float>(_panning) / 100.0f);
        if (panning_f < -1.0f) { panning_f = -1.0f; }
        if (panning_f > 1.0f) { panning_f = 1.0f; }

        player->SetVolume(vol_f);
        player->SetSpeed(speed_f);
        player->SetPanning(panning_f);
        _paramsChanged = false;
    }

    PlaybackState core_state = player->GetPlayStateNormal();
    float posms_f = player->GetPositionMs();
    _posMs = static_cast<int>(posms_f);
    _pos = PosmsToPos(_posMs);
    if (_state == core_state || IsPlaybackDone(core_state))
    {
        _state = core_state;
        return IsReady();
    }

    switch (_state)
    {
    case PlaybackState::PlayStatePlaying:
        player->Play();
        _state = player->GetPlayStateNormal();
        break;
    default: /* do nothing */
        break;
    }
    return IsReady();
}
