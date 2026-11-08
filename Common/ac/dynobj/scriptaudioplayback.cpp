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
#include "ac/dynobj/scriptaudioplayback.h"
#include "media/audio/audio.h"

using namespace AGS::Common;

const char *ScriptAudioPlayback::GetType()
{
    return "AudioPlayback";
}

int ScriptAudioPlayback::Dispose(void* /*address*/, bool force)
{
    // since the managed object is being deleted, remove the
    // reference so it doesn't try and dispose something else
    // with that handle later
    if (IsValid())
    {
        // TODO!!
        /*
        auto *over = get_overlay(static_cast<int>(_objID));
        if (over)
        {
            over->DetachScriptObject();
        }
        */
    }

    // if this is being removed voluntarily (ie. pointer out of
    // scope) then remove the associateed overlay
    // Otherwise, it's a Restore Game or something so don't
    if (!force)
    {
        // TODO!!!
        // Remove();
        _objID = UINT32_MAX;
    }

    delete this;
    return 1;
}
