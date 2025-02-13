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
//
// Character runtime class.
// TODO: merge with CharacterExtras.
//
//=============================================================================
#ifndef __AGS_EE_GAME__CHARACTERCLASS_H
#define __AGS_EE_GAME__CHARACTERCLASS_H
#include "ac/characterinfo.h"
#include "ac/dynobj/scriptobjects.h"

class Character : public ScriptGameEntity, public CharacterInfo
{
public:
    Character(const CharacterInfo &chinfo)
        : CharacterInfo(chinfo)
    {}

    AGS::Common::String GetTypeName() const override
    {
        return "Character";
    }

    AGS::Common::String GetScriptName() const override
    {
        return scrname;
    }
};

#endif // __AGS_EE_GAME__CHARACTERCLASS_H
