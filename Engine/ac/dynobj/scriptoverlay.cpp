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
#include "ac/dynobj/scriptoverlay.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/common.h"
#include "ac/overlay.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "debug/debug_log.h"
#include "util/stream.h"

using namespace AGS::Common;

const char *ScriptOverlay::GetType()
{
    return "Overlay";
}

int ScriptOverlay::Dispose(void* /*address*/, bool force)
{
    // since the managed object is being deleted, remove the
    // reference so it doesn't try and dispose something else
    // with that handle later
    if (IsValid())
    {
        auto *over = get_overlay(static_cast<int>(_objID));
        if (over)
        {
            over->DetachScriptObject();
        }
    }

    // if this is being removed voluntarily (ie. pointer out of
    // scope) then remove the associateed overlay
    // Otherwise, it's a Restore Game or something so don't
    if ((!force) && (Overlay_GetValid(this)))
    {
        Remove();
    }

    delete this;
    return 1;
}

void ScriptOverlay::Remove()
{
    if (!IsValid())
    {
        debug_script_warn("Overlay.Remove: overlay is invalid, could have been removed earlier.");
        return;
    }
    remove_screen_overlay(static_cast<int>(_objID));
    _objID = UINT32_MAX;
}

const char *ScriptAnimatedOverlay::GetType()
{
    return "AnimatedOverlay";
}

int ScriptAnimatedOverlay::Dispose(void *address, bool force)
{
    if (!force && IsValid())
    {
        RemoveAnimatedOverlay(_objID);
    }

    return ScriptOverlay::Dispose(address, force);
}
