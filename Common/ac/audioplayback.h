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
#ifndef __AGS_EE_AC__AUDIOPLAYBACK_H
#define __AGS_EE_AC__AUDIOPLAYBACK_H

#include "ac/dynobj/scriptaudioclip.h"
#include "ac/dynobj/scriptaudioplayback.h"

bool AudioPlayback_GetIsPlaying(ScriptAudioPlayback *sc_playback);
bool AudioPlayback_GetIsPaused(ScriptAudioPlayback *sc_playback);
int AudioPlayback_GetPanning(ScriptAudioPlayback *sc_playback);
void AudioPlayback_SetPanning(ScriptAudioPlayback *sc_playback, int panning);
ScriptAudioClip* AudioPlayback_GetPlayingClip(ScriptAudioPlayback *sc_playback);
int AudioPlayback_GetPlayingType(ScriptAudioPlayback *sc_playback);
int AudioPlayback_GetPosition(ScriptAudioPlayback *sc_playback);
int AudioPlayback_GetPositionMs(ScriptAudioPlayback *sc_playback);
int AudioPlayback_GetLengthMs(ScriptAudioPlayback *sc_playback);
int AudioPlayback_GetVolume(ScriptAudioPlayback *sc_playback);
void AudioPlayback_SetVolume(ScriptAudioPlayback *sc_playback, int volume);
int AudioPlayback_GetSpeed(ScriptAudioPlayback *sc_playback);
void AudioPlayback_SetSpeed(ScriptAudioPlayback *sc_playback, int speed);
void AudioPlayback_Stop(ScriptAudioPlayback *sc_playback);
void AudioPlayback_Pause(ScriptAudioPlayback *sc_playback);
void AudioPlayback_Resume(ScriptAudioPlayback *sc_playback);
void AudioPlayback_Seek(ScriptAudioPlayback *sc_playback, int pos);
void AudioPlayback_SeekMs(ScriptAudioPlayback *sc_playback, int posms);
void AudioPlayback_SetRoomLocation(ScriptAudioPlayback *sc_playback, int xpos, int ypos, int max_dist);


#endif // __AGS_EE_AC__AUDIOPLAYBACK_H
