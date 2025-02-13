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
// A collection of structs wrapping a reference to particular game object
// types. These are allocated in the script managed pool and exported
// to the script.
//
// For historical reasons must be at least 8-bytes large (actual contents
// are not restricted now).
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H
#define __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H

#include "util/string.h"

class ScriptGameEntity
{
public:
    virtual AGS::Common::String GetTypeName() const = 0;
    virtual AGS::Common::String GetScriptName() const = 0;
};

class ScriptSimpleRef
{
public:
    int id = -1;
    int reserved = 0;
};

class ScriptSimpleEntity : public ScriptGameEntity, public ScriptSimpleRef
{
};

class ScriptAudioChannel : public ScriptSimpleRef {};

class ScriptDialog : public ScriptSimpleEntity
{
public:
    AGS::Common::String GetTypeName() const override;
    AGS::Common::String GetScriptName() const override;
};

class ScriptGUI : public ScriptSimpleEntity
{
public:
    AGS::Common::String GetTypeName() const override;
    AGS::Common::String GetScriptName() const override;
};

class ScriptHotspot : public ScriptSimpleEntity
{
public:
    AGS::Common::String GetTypeName() const override;
    AGS::Common::String GetScriptName() const override;
};

class ScriptInvItem : public ScriptSimpleEntity
{
public:
    AGS::Common::String GetTypeName() const override;
    AGS::Common::String GetScriptName() const override;
};

class ScriptObject : public ScriptSimpleEntity
{
public:
    AGS::Common::String GetTypeName() const override;
    AGS::Common::String GetScriptName() const override;
};

class ScriptRegion : public ScriptSimpleEntity
{
public:
    AGS::Common::String GetTypeName() const override;
    AGS::Common::String GetScriptName() const override;
};

class ScriptWalkableArea : public ScriptSimpleEntity
{
public:
    AGS::Common::String GetTypeName() const override;
    AGS::Common::String GetScriptName() const override;
};

class ScriptWalkbehind : public ScriptSimpleEntity
{
public:
    AGS::Common::String GetTypeName() const override;
    AGS::Common::String GetScriptName() const override;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H
