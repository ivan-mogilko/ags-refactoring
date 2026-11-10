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
#include "ac/dynobj/scriptmotionpath.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/game.h"

using namespace AGS::Common;

MoveList *ScriptMotionPath::GetMoveList() const
{
    return IsValid() ? get_movelist(_objID) : nullptr;
}

const char *ScriptMotionPath::GetType()
{
    return "MotionPath";
}

DynObjectRef ScriptMotionPath::Create(int mlist_id)
{
    ScriptMotionPath *motion_path = new ScriptMotionPath(mlist_id);
    int handle = ccRegisterManagedObject(motion_path, motion_path);
    return DynObjectRef(handle, motion_path, motion_path);
}
