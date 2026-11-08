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
#include "ac/audioplayback.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/dynobj/cc_dynamicobjectref.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "media/audio/audio.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern RoomStruct thisroom;
extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
extern CCAudioChannel ccDynamicAudio;
extern CCAudioClip ccDynamicAudioClip;

SoundClip *GetPlaybackValidate(ScriptAudioPlayback *sc_playback, const char *api_name)
{
    auto *clip = AudioChans::GetPlayback(sc_playback->GetID());
    if (!clip)
        debug_script_error("%s: invalid playback object", api_name);
    return clip;
}

ScriptAudioChannel *AudioPlayback_GetChannel(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Channel");
    if (clip && clip->GetAudioChannelID() >= 0 && clip->GetAudioChannelID() < MAX_GAME_CHANNELS)
    {
        return &scrAudioChannel[clip->GetAudioChannelID()];
    }
    return nullptr;
}

bool AudioPlayback_GetIsPlaying(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.IsPlaying");
    return clip ? clip->IsReady() : false;
}

bool AudioPlayback_GetIsPaused(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.IsPaused");
    return clip ? clip->IsPaused() : false;
}

int AudioPlayback_GetPanning(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Panning");
    return clip ? clip->GetPanning() : 0;
}

void AudioPlayback_SetPanning(ScriptAudioPlayback *sc_playback, int panning)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Panning");
    if (!clip)
        return;

    if ((panning < -100) || (panning > 100))
    {
        debug_script_error("AudioChannel.Panning: panning value must be between -100 and 100 (passed=%d)", panning);
        panning = Math::Clamp(panning, -100, 100);
    }

    clip->SetPanning(panning);
}

ScriptAudioClip* AudioPlayback_GetPlayingClip(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.PlayingClip");
    return (clip && clip->GetSourceClipID() >= 0) ? &game.audioClips[clip->GetSourceClipID()] : nullptr;
}

int AudioPlayback_GetPlayingType(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.PlayingType");
    return clip ? clip->GetSourceAudioType() : AUDIOTYPE_UNDEFINED;
}

int AudioPlayback_GetPosition(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Position");
    return clip ? clip->GetPos() : 0;
}

int AudioPlayback_GetPositionMs(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.PositionMs");
    return clip ? clip->GetPosMs() : 0;
}

int AudioPlayback_GetLengthMs(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.LengthMs");
    return clip ? clip->GetLengthMs() : 0;
}

int AudioPlayback_GetVolume(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Volume");
    return clip ? clip->GetVolume100() : 0;
}

void AudioPlayback_SetVolume(ScriptAudioPlayback *sc_playback, int volume)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Volume");
    if (!clip)
        return;

    if ((volume < 0) || (volume > 100))
    {
        debug_script_error("AudioChannel.Volume: new value out of range (supplied: %d, range: 0..100)", volume);
        volume = Math::Clamp(volume, 0, 100);
    }

    clip->SetVolume100(volume);
}

int AudioPlayback_GetSpeed(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Speed");
    return clip ? clip->GetSpeed() : 0;
}

void AudioPlayback_SetSpeed(ScriptAudioPlayback *sc_playback, int speed)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Speed");
    if (clip)
        clip->SetSpeed(speed);
}

void AudioPlayback_Stop(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Stop");
    if (clip)
    {
        int chan_id = clip->GetAudioChannelID();
        if (chan_id == SCHAN_SPEECH && play.IsNonBlockingVoiceSpeech())
            stop_voice_nonblocking();
        else if (chan_id >= 0 && chan_id < TOTAL_AUDIO_CHANNELS)
            stop_or_fade_out_channel(chan_id, -1);
    }
}

void AudioPlayback_Pause(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Pause");
    if (clip)
        clip->Pause();
}

void AudioPlayback_Resume(ScriptAudioPlayback *sc_playback)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Resume");
    if (clip)
        clip->Resume();
}

void AudioPlayback_Seek(ScriptAudioPlayback *sc_playback, int pos)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.Seek");
    if (!clip)
        return;

    if (pos < 0)
    {
        debug_script_error("AudioChannel.Seek: invalid seek position %d", pos);
        pos = 0;
    }

    clip->Seek(pos);
}

void AudioPlayback_SeekMs(ScriptAudioPlayback *sc_playback, int posms)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.SeekMs");
    if (!clip)
        return;

    if (posms < 0)
    {
        debug_script_error("AudioChannel.SeekMs: invalid seek position %d", posms);
        posms = 0;
    }

    clip->SeekMs(posms);
}

void AudioPlayback_SetRoomLocation(ScriptAudioPlayback *sc_playback, int xpos, int ypos, int max_dist)
{
    auto *clip = GetPlaybackValidate(sc_playback, "AudioPlayback.SetRoomLocation");
    if (clip)
    {
        if ((max_dist < 1) || (thisroom.Width <= max_dist))
        {
            max_dist = ((xpos > thisroom.Width / 2) ? xpos : (thisroom.Width - xpos)) - AMBIENCE_FULL_DIST;
        }

        clip->SetEmitterPosition((xpos > 0) ? xpos : -1, ypos, max_dist);
        if (xpos > 0)
        {
            update_directional_sound_vol();
        }
        else
        {
            clip->ApplyDirectionalModifier(0);
        }
    }
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_api.h"
#include "script/script_runtime.h"

RuntimeScriptValue Sc_AudioPlayback_GetChannel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptAudioPlayback, ScriptAudioChannel, ccDynamicAudio, AudioPlayback_GetChannel);
}

RuntimeScriptValue Sc_AudioPlayback_GetIsPlaying(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioPlayback, AudioPlayback_GetIsPlaying);
}

RuntimeScriptValue Sc_AudioPlayback_GetPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioPlayback, AudioPlayback_GetPanning);
}

RuntimeScriptValue Sc_AudioPlayback_SetPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioPlayback, AudioPlayback_SetPanning);
}

RuntimeScriptValue Sc_AudioPlayback_GetPlayingClip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptAudioPlayback, ScriptAudioClip, ccDynamicAudioClip, AudioPlayback_GetPlayingClip);
}

RuntimeScriptValue Sc_AudioPlayback_GetPlayingType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioPlayback, AudioPlayback_GetPlayingType);
}

RuntimeScriptValue Sc_AudioPlayback_GetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioPlayback, AudioPlayback_GetPosition);
}

RuntimeScriptValue Sc_AudioPlayback_GetPositionMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioPlayback, AudioPlayback_GetPositionMs);
}

RuntimeScriptValue Sc_AudioPlayback_GetLengthMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioPlayback, AudioPlayback_GetLengthMs);
}

RuntimeScriptValue Sc_AudioPlayback_GetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioPlayback, AudioPlayback_GetVolume);
}

RuntimeScriptValue Sc_AudioPlayback_SetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioPlayback, AudioPlayback_SetVolume);
}

RuntimeScriptValue Sc_AudioPlayback_Stop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioPlayback, AudioPlayback_Stop);
}

RuntimeScriptValue Sc_AudioPlayback_Seek(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioPlayback, AudioPlayback_Seek);
}

RuntimeScriptValue Sc_AudioPlayback_SeekMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioPlayback, AudioPlayback_SeekMs);
}

RuntimeScriptValue Sc_AudioPlayback_SetRoomLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptAudioPlayback, AudioPlayback_SetRoomLocation);
}

RuntimeScriptValue Sc_AudioPlayback_GetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioPlayback, AudioPlayback_GetSpeed);
}

RuntimeScriptValue Sc_AudioPlayback_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioPlayback, AudioPlayback_SetSpeed);
}

RuntimeScriptValue Sc_AudioPlayback_Pause(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioPlayback, AudioPlayback_Pause);
}

RuntimeScriptValue Sc_AudioPlayback_Resume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioPlayback, AudioPlayback_Resume);
}

RuntimeScriptValue Sc_AudioPlayback_GetIsPaused(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptAudioPlayback, AudioPlayback_GetIsPaused);
}

void RegisterAudioPlaybackAPI()
{
    ScFnRegister audioplayer_api[] = {
        { "AudioPlayback::Pause^0",            API_FN_PAIR(AudioPlayback_Pause) },
        { "AudioPlayback::Resume^0",           API_FN_PAIR(AudioPlayback_Resume) },
        { "AudioPlayback::Seek^1",             API_FN_PAIR(AudioPlayback_Seek) },
        { "AudioPlayback::SeekMs^1",           API_FN_PAIR(AudioPlayback_SeekMs) },
        { "AudioPlayback::SetRoomLocation^3",  API_FN_PAIR(AudioPlayback_SetRoomLocation) },
        { "AudioPlayback::Stop^0",             API_FN_PAIR(AudioPlayback_Stop) },
        { "AudioPlayback::get_Channel",        API_FN_PAIR(AudioPlayback_GetChannel) },
        { "AudioPlayback::get_IsPaused",       API_FN_PAIR(AudioPlayback_GetIsPaused) },
        { "AudioPlayback::get_IsPlaying",      API_FN_PAIR(AudioPlayback_GetIsPlaying) },
        { "AudioPlayback::get_LengthMs",       API_FN_PAIR(AudioPlayback_GetLengthMs) },
        { "AudioPlayback::get_Panning",        API_FN_PAIR(AudioPlayback_GetPanning) },
        { "AudioPlayback::set_Panning",        API_FN_PAIR(AudioPlayback_SetPanning) },
        { "AudioPlayback::get_PlayingClip",    API_FN_PAIR(AudioPlayback_GetPlayingClip) },
        { "AudioPlayback::get_PlayingType",    API_FN_PAIR(AudioPlayback_GetPlayingType) },
        { "AudioPlayback::get_Position",       API_FN_PAIR(AudioPlayback_GetPosition) },
        { "AudioPlayback::get_PositionMs",     API_FN_PAIR(AudioPlayback_GetPositionMs) },
        { "AudioPlayback::get_Volume",         API_FN_PAIR(AudioPlayback_GetVolume) },
        { "AudioPlayback::set_Volume",         API_FN_PAIR(AudioPlayback_SetVolume) },
        { "AudioPlayback::get_Speed",          API_FN_PAIR(AudioPlayback_GetSpeed) },
        { "AudioPlayback::set_Speed",          API_FN_PAIR(AudioPlayback_SetSpeed) },
    };

    ccAddExternalFunctions(audioplayer_api);
}
