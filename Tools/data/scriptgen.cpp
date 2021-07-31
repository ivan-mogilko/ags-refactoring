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
#include "data/scriptgen.h"
#include <iterator>
#include <cctype>
#include <cstring>
#include "data/room_utils.h"

namespace AGS
{
namespace DataUtil
{

//-----------------------------------------------------------------------------
// _AutoGenerated.ash
//-----------------------------------------------------------------------------

// Generates game object declarations of the given type.
// type_name defines the script name of the type.
// If array_name is provided then also declares array of objects of that type.
// If array_base is > 0, it will be added to the array size in declaration.
static String DeclareEntities(const std::vector<EntityRef> &ents,
    const char *type_name, const char *array_name = nullptr, const int array_base = 0)
{
    if (ents.size() == 0)
        return "";

    String header;
    if (array_name)
        header.Append(String::FromFormat("import %s %s[%d];\n",
            type_name, array_name, ents.size() + array_base));

    String buf;
    for (const auto &ent : ents)
    {
        String name = ent.ScriptName;
        if (name.IsEmpty())
            continue;
        buf.Format("import %s %s;\n", type_name, name.GetCStr());
        header.Append(buf);
    }
    return header;
}

// Generates game object declarations in the form of *macros*.
// check_prefix is an optional prefix that has to be present in an original
// name; if it's set then names without such prefix will be skipped.
// The macro name will be <script_name>, where "script_name" is read from the
// object data, have check_prefix stripped and converted to uppercase.
// The macro values are made equal to object's ID (numeric index).
static String DeclareEntitiesAsMacros(const std::vector<EntityRef> &ents,
    const char *check_prefix = nullptr)
{
    if (ents.size() == 0)
        return "";

    String header;
    String name;
    String buf;
    for (const auto &ent : ents)
    {
        String name = ent.ScriptName;
        // if check_prefix is defined, skip those which do not have it;
        // if it has one, remove the prefix and proceed
        if (check_prefix)
        {
            if (!name.StartsWith(check_prefix))
                continue;
            name.ClipLeft(strlen(check_prefix));
        }
        // skip if the name is empty or begins with non-alpha character
        if (name.IsEmpty() || !std::isalpha(name[0]))
            continue;

        name.MakeUpper();
        buf.Format("#define %s %d\n", name.GetCStr(), ent.ID);
        header.Append(buf);
    }
    return header;
}

// Generates game object declarations in the form of *enum*.
// enum_name defines the name of the enumeration.
// const_prefix is an optional prefix for enumeration members.
// The actual constant name will be <const_prefix><script_name>, where
// "script_name" is read from the object data. The constant values are made
// equal to object's ID (numeric index).
static String DeclareEntitiesAsEnum(const std::vector<EntityRef> &ents,
    const char *enum_name, const char *const_prefix = nullptr)
{
    if (ents.size() == 0)
    {
        // no elements, make sure the enum has something in it
        return String::FromFormat("enum %s {\n  eDummy%s__ = 99  // $AUTOCOMPLETEIGNORE$ \n};\n", enum_name);
    }

    String header;
    String const_name;
    String buf;
    buf.Format("enum %s {\n", enum_name);
    header.Append(buf);
    bool first = true;
    for (const auto &ent : ents)
    {
        String name = ent.ScriptName;
        if (name.IsEmpty())
            continue;

        if (const_prefix)
            const_name = const_prefix;
        // skip if the name begins with non-alpha character
        else if (!std::isalpha(name[0]))
            continue;

        const_name.Append(name);
        buf.Format("%s  %s = %d", first ? "" : ",\n", const_name.GetCStr(), ent.ID);
        header.Append(buf);
        first = false;
    }
    header.Append("\n};\n");
    return header;
}

// Generates GUI and GUI Control declarations.
// For backward compatibility also declares uppercase macros which translate
// into the call to FindGUIID function, used to lookup for GUI pointer using
// a "script name" string.
static String DeclareGUI(const std::vector<GUIRef> &guis)
{
    if (guis.size() == 0)
        return "";

    String header;
    header.Append(String::FromFormat("import GUI gui[%d];\n", guis.size()));

    String buf;
    String macro_name;
    for (const auto &gui : guis)
    {
        String name = gui.ScriptName;
        if (name.IsEmpty())
            continue;

        buf.Format("import GUI %s;\n", name.GetCStr());
        header.Append(buf);

        if (name.GetAt(0) == 'g')
        {
            macro_name = name.Mid(1);
            macro_name.MakeUpper();
            buf.Format("#define %s FindGUIID(\"%s\")\n", macro_name.GetCStr(), macro_name.GetCStr());
            header.Append(buf);
        }

        for (const auto &ent : gui.Controls)
        {
            String name = ent.ScriptName;
            if (name.IsEmpty())
                continue;
            String class_name = ent.TypeName;
            buf.Format("import %s %s;\n", class_name.GetCStr(), name.GetCStr());
            header.Append(buf);
        }
    }
    return header;
}

// Generates whole game autoheader by merging various object and array declarations
String MakeGameAutoScriptHeader(const GameRef &game)
{
    String header;
    // Audio clips
    header.Append(DeclareEntities(game.AudioClips, "AudioClip"));
    // Audio types
    header.Append(DeclareEntitiesAsEnum(game.AudioTypes, "AudioType", "eAudioType"));
    // Characters
    header.Append(DeclareEntities(game.Characters, "Character", "character"));
    header.Append(DeclareEntitiesAsMacros(game.Characters, "c"));
    // Cursors
    header.Append(DeclareEntitiesAsEnum(game.Cursors, "CursorMode", "eMode"));
    // Dialogs
    std::vector<EntityRef> dialogs; // TODO: look for better solution later
    std::copy(game.Dialogs.begin(), game.Dialogs.end(), std::back_inserter(dialogs));
    header.Append(DeclareEntities(dialogs, "Dialog", "dialog"));
    // Fonts
    header.Append(DeclareEntitiesAsEnum(game.Fonts, "FontType", "eFont"));
    // GUI
    header.Append(DeclareGUI(game.GUI));
    // Inventory items (array is 1-based)
    header.Append(DeclareEntities(game.Inventory, "InventoryItem", "inventory", 1));
    // Views
    header.Append(DeclareEntitiesAsMacros(game.Views));
    return header;
}


//-----------------------------------------------------------------------------
// _GlobalVariables.ash/asc
//-----------------------------------------------------------------------------

String MakeVariablesScriptHeader(std::vector<Variable> &vars)
{
    String header, buf;
    for (const auto &var : vars)
    {
        buf.Format("import %s %s;\n", var.Type.GetCStr(), var.Name.GetCStr());
        header.Append(buf);
    }
    return header;
}

String MakeVariablesScriptBody(std::vector<Variable> &vars)
{
    String body, buf;
    // Generate declarations and init simple vars
    for (const auto &var : vars)
    {
        if ((var.Type.Compare("int") == 0 || var.Type.Compare("bool") == 0 || var.Type.Compare("float") == 0) &&
            !var.Value.IsEmpty())
        {
            buf.Format("%s %s = %s;\n", var.Type.GetCStr(), var.Name.GetCStr(), var.Value.GetCStr());
        }
        else
        {
            buf.Format("%s %s;\n", var.Type.GetCStr(), var.Name.GetCStr());
        }
        body.Append(buf);
        buf.Format("export %s;\n", var.Name.GetCStr());
        body.Append(buf);
    }

    // Generate initialization of String vars
    body.Append("function game_start() {\n");
    for (const auto &var : vars)
    {
        if (var.Type.Compare("String") == 0)
        {
            String value = var.Value;
            value.Replace("\\", "\\\\");
            value.Replace("\"", "\\\"");
            buf.Format("  %s = \"%s\";\n", var.Name.GetCStr(), value.GetCStr());
            body.Append(buf);
        }
    }
    body.Append("}\n");
    return body;
}


//-----------------------------------------------------------------------------
// Room.ash
//-----------------------------------------------------------------------------

String MakeRoomScriptHeader(const RoomScNames &data)
{
    String header;
    // Room Object names
    for (const auto &obj : data.ObjectNames)
    {
        if (!obj.IsEmpty())
        {
            header.Append("import Object ");
            header.Append(obj);
            header.Append(";\n");
        }
    }
    // Hotspot names
    for (const auto &hot : data.HotspotNames)
    {
        if (!hot.IsEmpty())
        {
            header.Append("import Hotspot ");
            header.Append(hot);
            header.Append(";\n");
        }
    }
    return header;
}

} // namespace DataUtil
} // namespace AGS
