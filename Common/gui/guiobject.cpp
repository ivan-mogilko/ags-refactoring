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

#include "ac/common.h" // quit
#include "gui/guimain.h"
#include "gui/guiobject.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

GUIObject::GUIObject()
{
    Id          = -1;
    ParentId    = -1;
    Flags       = kGUICtrl_DefFlags;
    X           = 0;
    Y           = 0;
    _width       = 0;
    _height      = 0;
    ZOrder      = -1;
    IsActivated   = false;
    _transparency = 0;
    _scEventCount = 0;
    _hasChanged = true;
}

String GUIObject::GetScriptName() const
{
    return Name;
}

int GUIObject::GetEventCount() const
{
    return _scEventCount;
}

String GUIObject::GetEventName(int event) const
{
    if (event < 0 || event >= _scEventCount)
        return "";
    return _scEventNames[event];
}

String GUIObject::GetEventArgs(int event) const
{
    if (event < 0 || event >= _scEventCount)
        return "";
    return _scEventArgs[event];
}

bool GUIObject::IsOverControl(int x, int y, int leeway) const
{
    return x >= X && y >= Y && x < (X + _width + leeway) && y < (Y + _height + leeway);
}

void GUIObject::SetClickable(bool on)
{
    if (on != ((Flags & kGUICtrl_Clickable) != 0))
    {
        Flags = (Flags & ~kGUICtrl_Clickable) | kGUICtrl_Clickable * on;
        MarkStateChanged(false, false); // update cursor-over-control only
    }
}

void GUIObject::SetEnabled(bool on)
{
    if (on != ((Flags & kGUICtrl_Enabled) != 0))
    {
        Flags = (Flags & ~kGUICtrl_Enabled) | kGUICtrl_Enabled * on;
        MarkStateChanged(true, true); // may change looks, and update cursor-over-control
    }
}

void GUIObject::SetSize(int width, int height)
{
    if (_width != width || _height != height)
    {
        _width = width;
        _height = height;
        OnResized();
    }
}

void GUIObject::SetTranslated(bool on)
{
    if (on != ((Flags & kGUICtrl_Translated) != 0))
    {
        Flags = (Flags & ~kGUICtrl_Translated) | kGUICtrl_Translated * on;
        MarkChanged();
    }
}

void GUIObject::SetVisible(bool on)
{
    if (on != ((Flags & kGUICtrl_Visible) != 0))
    {
        Flags = (Flags & ~kGUICtrl_Visible) | kGUICtrl_Visible * on;
        MarkStateChanged(false, true); // for software mode, and to update cursor-over-control
    }
}

void GUIObject::SetTransparency(int trans)
{
    if (_transparency != trans)
    {
        _transparency = trans;
        MarkParentChanged(); // for software mode
    }
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUIObject::WriteToFile(Stream *out) const
{
    out->WriteInt32(Flags);
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(_width);
    out->WriteInt32(_height);
    out->WriteInt32(ZOrder);
    Name.Write(out);
    out->WriteInt32(_scEventCount);
    for (int i = 0; i < _scEventCount; ++i)
        EventHandlers[i].Write(out);
}

void GUIObject::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    Flags    = in->ReadInt32();
    // reverse particular flags from older format
    if (gui_version < kGuiVersion_350)
        Flags ^= kGUICtrl_OldFmtXorMask;
    X        = in->ReadInt32();
    Y        = in->ReadInt32();
    _width    = in->ReadInt32();
    _height   = in->ReadInt32();
    ZOrder   = in->ReadInt32();
    if (gui_version < kGuiVersion_350)
    { // NOTE: reading into actual variables only for old savegame support
        IsActivated = in->ReadInt32() != 0;
    }

    if (gui_version >= kGuiVersion_unkn_106)
        Name.Read(in);
    else
        Name.Free();

    for (int i = 0; i < _scEventCount; ++i)
    {
        EventHandlers[i].Free();
    }

    if (gui_version >= kGuiVersion_unkn_108)
    {
        int evt_count = in->ReadInt32();
        if (evt_count > _scEventCount)
            quit("Error: too many control events, need newer version");
        for (int i = 0; i < evt_count; ++i)
        {
            EventHandlers[i].Read(in);
        }
    }
}

void GUIObject::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    // Properties
    Flags = in->ReadInt32();
    // reverse particular flags from older format
    if (svg_ver < kGuiSvgVersion_350)
        Flags ^= kGUICtrl_OldFmtXorMask;
    X = in->ReadInt32();
    Y = in->ReadInt32();
    _width = in->ReadInt32();
    _height = in->ReadInt32();
    ZOrder = in->ReadInt32();
    // Dynamic state
    IsActivated = in->ReadBool() ? 1 : 0;
    if (svg_ver >= kGuiSvgVersion_36023)
    {
        _transparency = in->ReadInt32();
        in->ReadInt32(); // reserve 3 ints
        in->ReadInt32();
        in->ReadInt32();
    }
}

void GUIObject::WriteToSavegame(Stream *out) const
{
    // Properties
    out->WriteInt32(Flags);
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(_width);
    out->WriteInt32(_height);
    out->WriteInt32(ZOrder);
    // Dynamic state
    out->WriteBool(IsActivated != 0);
    out->WriteInt32(_transparency);
    out->WriteInt32(0); // reserve 3 ints
    out->WriteInt32(0);
    out->WriteInt32(0);
}


HorAlignment ConvertLegacyGUIAlignment(LegacyGUIAlignment align)
{
    switch (align)
    {
    case kLegacyGUIAlign_Right:
        return kHAlignRight;
    case kLegacyGUIAlign_Center:
        return kHAlignCenter;
    default:
        return kHAlignLeft;
    }
}

LegacyGUIAlignment GetLegacyGUIAlignment(HorAlignment align)
{
    switch (align)
    {
    case kHAlignRight:
        return kLegacyGUIAlign_Right;
    case kHAlignCenter:
        return kLegacyGUIAlign_Center;
    default:
        return kLegacyGUIAlign_Left;
    }
}

} // namespace Common
} // namespace AGS
