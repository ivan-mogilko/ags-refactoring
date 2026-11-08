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
#include "ac/audiochannel.h"
#include "ac/audioplayback.h"
#include "ac/dynobj/cc_dynamicobjectref.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "media/audio/audio.h"

using namespace AGS::Common;

extern CCAudioClip ccDynamicAudioClip;

bool AssertChannel(ScriptAudioChannel *channel, const char *api_name)
{
    if (channel->id < 0 || channel->id >= TOTAL_AUDIO_CHANNELS)
    {
        debug_script_error("%s: invalid channel %d", api_name, channel->id);
        return false;
    }
    return true;
}

int AudioChannel_GetID(ScriptAudioChannel *channel)
{
    return channel->id;
}

ScriptAudioPlayback *AudioChannel_GetPlayback(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.Playback"))
    {
        SoundClip *clip = AudioChans::GetChannel(channel->id);
        return static_cast<ScriptAudioPlayback*>
            (clip ? ccGetObjectAddressFromHandle(clip->GetSourceClipID()) : nullptr);
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
// Deprecated API, audio channel controlled playback.
// Delegates actual work to AudioPlayback API. It's left to be decided if or
// when to remove this.
//-----------------------------------------------------------------------------

bool AudioChannel_GetIsPlaying(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.IsPlaying"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetIsPlaying(&delegate_playback);
    }
    return false;
}

bool AudioChannel_GetIsPaused(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.IsPaused"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetIsPaused(&delegate_playback);
    }
    return false;
}

int AudioChannel_GetPanning(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.Panning"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetPanning(&delegate_playback);
    }
    return false;
}

void AudioChannel_SetPanning(ScriptAudioChannel *channel, int panning)
{
    if (AssertChannel(channel, "AudioChannel.Panning"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        AudioPlayback_SetPanning(&delegate_playback, panning);
    }
}

ScriptAudioClip* AudioChannel_GetPlayingClip(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.PlayingClip"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetPlayingClip(&delegate_playback);
    }
    return nullptr;
}

int AudioChannel_GetPlayingType(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.PlayingType"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetPlayingType(&delegate_playback);
    }
    return AUDIOTYPE_UNDEFINED;
}

int AudioChannel_GetPosition(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.Position"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetPosition(&delegate_playback);
    }
    return 0;
}

int AudioChannel_GetPositionMs(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.PositionMs"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetPositionMs(&delegate_playback);
    }
    return 0;
}

int AudioChannel_GetLengthMs(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.LengthMs"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetLengthMs(&delegate_playback);
    }
    return 0;
}

int AudioChannel_GetVolume(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.Volume"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetVolume(&delegate_playback);
    }
    return 0;
}

void AudioChannel_SetVolume(ScriptAudioChannel *channel, int volume)
{
    if (AssertChannel(channel, "AudioChannel.Volume"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_SetVolume(&delegate_playback, volume);
    }
}

int AudioChannel_GetSpeed(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.Speed"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_GetSpeed(&delegate_playback);
    }
    return 0;
}

void AudioChannel_SetSpeed(ScriptAudioChannel *channel, int speed)
{
    if (AssertChannel(channel, "AudioChannel.Speed"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        return AudioPlayback_SetSpeed(&delegate_playback, speed);
    }
}

void AudioChannel_Stop(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.Stop"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        AudioPlayback_Stop(&delegate_playback);
    }
}

void AudioChannel_Pause(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.Pause"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        AudioPlayback_Pause(&delegate_playback);
    }
}

void AudioChannel_Resume(ScriptAudioChannel *channel)
{
    if (AssertChannel(channel, "AudioChannel.Resume"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        AudioPlayback_Resume(&delegate_playback);
    }
}

void AudioChannel_Seek(ScriptAudioChannel *channel, int pos)
{
    if (AssertChannel(channel, "AudioChannel.Seek"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        AudioPlayback_Seek(&delegate_playback, pos);
    }
}

void AudioChannel_SeekMs(ScriptAudioChannel *channel, int posms)
{
    if (AssertChannel(channel, "AudioChannel.SeekMs"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        AudioPlayback_SeekMs(&delegate_playback, posms);
    }
}

void AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xpos, int ypos, int max_dist)
{
    if (AssertChannel(channel, "AudioChannel.SetRoomLocation"))
    {
        ScriptAudioPlayback delegate_playback(AudioChans::GetPlaybackIDFromChan(channel->id));
        AudioPlayback_SetRoomLocation(&delegate_playback, xpos, ypos, max_dist);
    }
}

void AudioChannel_SetRoomLocation2(ScriptAudioChannel *channel, int xPos, int yPos)
{
    AudioChannel_SetRoomLocation(channel, xPos, yPos, 0);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetID);
}

RuntimeScriptValue Sc_AudioChannel_GetPlayback(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptAudioChannel, ScriptAudioPlayback, AudioChannel_GetPlayback);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetIsPlaying(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptAudioChannel, AudioChannel_GetIsPlaying);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPanning);
}

// void | ScriptAudioChannel *channel, int newPanning
RuntimeScriptValue Sc_AudioChannel_SetPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_SetPanning);
}

// ScriptAudioClip* | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPlayingClip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptAudioChannel, ScriptAudioClip, ccDynamicAudioClip, AudioChannel_GetPlayingClip);
}

RuntimeScriptValue Sc_AudioChannel_GetPlayingType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPlayingType);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPosition);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPositionMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPositionMs);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetLengthMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetLengthMs);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetVolume);
}

// int | ScriptAudioChannel *channel, int newVolume
RuntimeScriptValue Sc_AudioChannel_SetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_SetVolume);
}

// void | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_Stop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioChannel, AudioChannel_Stop);
}

// void | ScriptAudioChannel *channel, int newPosition
RuntimeScriptValue Sc_AudioChannel_Seek(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_Seek);
}

RuntimeScriptValue Sc_AudioChannel_SeekMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_SeekMs);
}

RuntimeScriptValue Sc_AudioChannel_SetRoomLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptAudioChannel, AudioChannel_SetRoomLocation);
}

RuntimeScriptValue Sc_AudioChannel_SetRoomLocation2(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptAudioChannel, AudioChannel_SetRoomLocation2);
}

RuntimeScriptValue Sc_AudioChannel_GetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetSpeed);
}

RuntimeScriptValue Sc_AudioChannel_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_SetSpeed);
}

RuntimeScriptValue Sc_AudioChannel_Pause(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioChannel, AudioChannel_Pause);
}

RuntimeScriptValue Sc_AudioChannel_Resume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioChannel, AudioChannel_Resume);
}

RuntimeScriptValue Sc_AudioChannel_GetIsPaused(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptAudioChannel, AudioChannel_GetIsPaused);
}

void RegisterAudioChannelAPI()
{
    ScFnRegister audiochan_api[] = {
        { "AudioChannel::get_ID",             API_FN_PAIR(AudioChannel_GetID) },
        { "AudioChannel::get_Playback",       API_FN_PAIR(AudioChannel_GetPlayback) },

        //
        // Deprecated API, audio channel controlled playback.
        // This is now part of AudioPlayback. It's left to be decided if or when to remove this.
        //
        { "AudioChannel::Pause^0",            API_FN_PAIR(AudioChannel_Pause) },
        { "AudioChannel::Resume^0",           API_FN_PAIR(AudioChannel_Resume) },
        { "AudioChannel::Seek^1",             API_FN_PAIR(AudioChannel_Seek) },
        { "AudioChannel::SeekMs^1",           API_FN_PAIR(AudioChannel_SeekMs) },
        { "AudioChannel::SetRoomLocation^2",  API_FN_PAIR(AudioChannel_SetRoomLocation2) },
        { "AudioChannel::SetRoomLocation^3",  API_FN_PAIR(AudioChannel_SetRoomLocation) },
        { "AudioChannel::Stop^0",             API_FN_PAIR(AudioChannel_Stop) },
        { "AudioChannel::get_IsPaused",       API_FN_PAIR(AudioChannel_GetIsPaused) },
        { "AudioChannel::get_IsPlaying",      API_FN_PAIR(AudioChannel_GetIsPlaying) },
        { "AudioChannel::get_LengthMs",       API_FN_PAIR(AudioChannel_GetLengthMs) },
        { "AudioChannel::get_Panning",        API_FN_PAIR(AudioChannel_GetPanning) },
        { "AudioChannel::set_Panning",        API_FN_PAIR(AudioChannel_SetPanning) },
        { "AudioChannel::get_PlayingClip",    API_FN_PAIR(AudioChannel_GetPlayingClip) },
        { "AudioChannel::get_PlayingType",    API_FN_PAIR(AudioChannel_GetPlayingType) },
        { "AudioChannel::get_Position",       API_FN_PAIR(AudioChannel_GetPosition) },
        { "AudioChannel::get_PositionMs",     API_FN_PAIR(AudioChannel_GetPositionMs) },
        { "AudioChannel::get_Volume",         API_FN_PAIR(AudioChannel_GetVolume) },
        { "AudioChannel::set_Volume",         API_FN_PAIR(AudioChannel_SetVolume) },
        { "AudioChannel::get_Speed",          API_FN_PAIR(AudioChannel_GetSpeed) },
        { "AudioChannel::set_Speed",          API_FN_PAIR(AudioChannel_SetSpeed) },
    };

    ccAddExternalFunctions(audiochan_api);
}
