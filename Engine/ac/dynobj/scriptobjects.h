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
//
// A collection of structs wrapping a reference to particular game object
// types. These are allocated in the script managed pool and exported
// to the script.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H
#define __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H

struct ScriptObjectRef
{
    int id = -1;
};

struct ScriptAudioChannel : public ScriptObjectRef {};
struct ScriptCharacter : public ScriptObjectRef {};
struct ScriptDialog : public ScriptObjectRef {};
struct ScriptGUI : public ScriptObjectRef {};
struct ScriptHotspot : public ScriptObjectRef {};
struct ScriptInvItem : public ScriptObjectRef {};
struct ScriptObject : public ScriptObjectRef {};
struct ScriptRegion : public ScriptObjectRef {};
struct ScriptWalkableArea : public ScriptObjectRef {};
struct ScriptWalkbehind : public ScriptObjectRef {};

#endif // __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H
