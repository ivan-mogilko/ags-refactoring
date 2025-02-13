//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_AC_DYNOBJ__SCRIPTAUDIOCLIP_H
#define __AGS_EE_AC_DYNOBJ__SCRIPTAUDIOCLIP_H
#include "ac/audioclipinfo.h"
#include "ac/dynobj/scriptobjects.h"

class ScriptAudioClip : public ScriptGameEntity, public AudioClipInfo
{
public:
    ScriptAudioClip(const AudioClipInfo &info)
        : AudioClipInfo(info)
    {}

    AGS::Common::String GetTypeName() const override
    {
        return "AudioClip";
    }

    AGS::Common::String GetScriptName() const override
    {
        return scriptName;
    }
};

#endif // __AGS_EE_AC_DYNOBJ__SCRIPTAUDIOCLIP_H
