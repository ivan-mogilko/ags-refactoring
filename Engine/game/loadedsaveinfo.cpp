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

#include "game/loadedsaveinfo.h"


namespace AGS
{
namespace Engine
{

LoadedSaveInfo::LoadedSaveInfo()
    : AudioTypeCount(0)
    , AudioClipCount(0)
    , CharCount(0)
    , DialogCount(0)
    , GUICount(0)
    , GUIBtnCount(0)
    , GUILblCount(0)
    , GUIInvCount(0)
    , GUISldCount(0)
    , GUITbxCount(0)
    , GUILbxCount(0)
    , InvItemCount(0)
    , MouseCurCount(0)
{
}

} // namespace Engine
} // namespace AGS
