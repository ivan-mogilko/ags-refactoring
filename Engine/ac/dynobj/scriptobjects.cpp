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
#include "ac/dynobj/scriptobjects.h"
#include "game/gameclass.h"
#include "game/roomstruct.h"
#include "gui/guimain.h"

using namespace AGS::Common;

extern Game game;
extern std::vector<GUIMain> guis;
extern RoomStruct thisroom;


String ScriptDialog::GetTypeName() const
{
    return "Dialog";
}

String ScriptDialog::GetScriptName() const
{
    return game.dialogScriptNames[id];
}

String ScriptGUI::GetTypeName() const
{
    return "GUI";
}

String ScriptGUI::GetScriptName() const
{
    return guis[id].Name;
}

String ScriptHotspot::GetTypeName() const
{
    return "Hotspot";
}

String ScriptHotspot::GetScriptName() const
{
    return thisroom.Hotspots[id].ScriptName;
}

String ScriptInvItem::GetTypeName() const
{
    return "InventoryItem";
}

String ScriptInvItem::GetScriptName() const
{
    return game.invScriptNames[id];
}

String ScriptObject::GetTypeName() const
{
    return "Object";
}

String ScriptObject::GetScriptName() const
{
    return thisroom.Objects[id].ScriptName;
}

String ScriptRegion::GetTypeName() const
{
    return "Region";
}

String ScriptRegion::GetScriptName() const
{
    return {};
}

String ScriptWalkableArea::GetTypeName() const
{
    return "WalkableArea";
}

String ScriptWalkableArea::GetScriptName() const
{
    return {};
}

String ScriptWalkbehind::GetTypeName() const
{
    return "Walkbehind";
}

String ScriptWalkbehind::GetScriptName() const
{
    return {};
}
