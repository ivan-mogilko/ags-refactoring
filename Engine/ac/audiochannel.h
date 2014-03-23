//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__AUDIOCHANNEL_H
#define __AGS_EE_AC__AUDIOCHANNEL_H

#include "ac/dynobj/scriptaudioclip.h"
#include "core/types.h"
#include "media/audio/audiodefines.h"
#include "media/audio/soundclip.h"

namespace AGS
{
namespace Engine
{

class AudioChannel
{
public:
    AudioChannel()
        : _id(-1)
        , _lastSoundPlayed(-1)
    {
    }

    inline SoundClipRef GetClip() const
    {
        return _clip;
    }

    inline int GetId() const
    {
        return _id;
    }

    inline int GetLastSoundPlayed() const
    {
        return _lastSoundPlayed;
    }

    inline bool HasClip() const
    {
        return _clip != NULL;
    }

    void RemoveClip()
    {
        _clip.Reset();
        _lastSoundPlayed = -1;
    }

    void SetClip(SoundClipUPtr &clip, int audio_id = -1)
    {
        _clip = clip;
        _lastSoundPlayed = audio_id;
    }

    void SetClip(AudioChannel &channel)
    {
        SetClip(channel._clip);
    }

    void SetId(int id)
    {
        _id = id;
    }

private:
    int                 _id;
    SoundClipUPtr       _clip;
    int                 _lastSoundPlayed;
private:
    // AudioChannel should not be copied
    AudioChannel(const AudioChannel &); // non-copyable
    AudioChannel& operator=(const AudioChannel &); // not copy-assignable
};

} // namespace Engine
} // namespace AGS

typedef AGS::Engine::AudioChannel ScriptAudioChannel;

int     AudioChannel_GetID(ScriptAudioChannel *channel);
int     AudioChannel_GetIsPlaying(ScriptAudioChannel *channel);
int     AudioChannel_GetPanning(ScriptAudioChannel *channel);
void    AudioChannel_SetPanning(ScriptAudioChannel *channel, int newPanning);
ScriptAudioClip* AudioChannel_GetPlayingClip(ScriptAudioChannel *channel);
int     AudioChannel_GetPosition(ScriptAudioChannel *channel);
int     AudioChannel_GetPositionMs(ScriptAudioChannel *channel);
int     AudioChannel_GetLengthMs(ScriptAudioChannel *channel);
int     AudioChannel_GetVolume(ScriptAudioChannel *channel);
int     AudioChannel_SetVolume(ScriptAudioChannel *channel, int newVolume);
void    AudioChannel_Stop(ScriptAudioChannel *channel);
void    AudioChannel_Seek(ScriptAudioChannel *channel, int newPosition);
void    AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xPos, int yPos);

#endif // __AGS_EE_AC__AUDIOCHANNEL_H
