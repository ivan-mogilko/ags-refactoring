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

#include "ac/dynobj/scriptloadedsaveinfo.h"

using namespace AGS::Engine;

ScriptLoadedSaveInfo::ScriptLoadedSaveInfo()
    : _cancelRestore(false)
{
}

const char *ScriptLoadedSaveInfo::GetType()
{
    return "LoadedSaveInfo";
}

int ScriptLoadedSaveInfo::Serialize(const char *address, char *buffer, int bufsize)
{
    // LoadedSaveInfo is not persistent
    return 0;
}

void ScriptLoadedSaveInfo::Unserialize(int index, const char *serializedData, int dataSize)
{
    // LoadedSaveInfo is not persistent
    ccRegisterUnserializedObject(index, this, this);
}

void ScriptLoadedSaveInfo::Set(const LoadedSaveInfo &info)
{
    _cancelRestore = false;
    _info.reset(new LoadedSaveInfo(info));
}

void ScriptLoadedSaveInfo::Reset()
{
    _cancelRestore = false;
    _info.reset();
}
