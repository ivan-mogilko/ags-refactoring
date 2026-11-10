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
#ifndef __AGS_EE_DYNOBJ__SCRIPTMOTIONPATH_H
#define __AGS_EE_DYNOBJ__SCRIPTMOTIONPATH_H

#include "ac/dynobj/cc_dynamicautoobjectref.h"

class MoveList;

class ScriptMotionPath : public CCDynamicAutoObjectRef
{
public:
    ScriptMotionPath() = default;
    // NOTE: move lists are identified using uint32_t in game data,
    // however character and objects reference them as int16 and int32 respectively.
    // this might need a revision.
    ScriptMotionPath(int mlist_id) : CCDynamicAutoObjectRef(mlist_id) {}
    
    MoveList *GetMoveList() const;

    const char *GetType() override;

    static DynObjectRef Create(int mlist_id);
};

#endif // __AGS_EE_DYNOBJ__SCRIPTMOTIONPATH_H
