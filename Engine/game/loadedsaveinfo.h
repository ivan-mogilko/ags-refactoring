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
//
// LoadedSaveInfo struct contains information about save that was just loaded,
// as well as possible content conflicts with current game version.
//
//=============================================================================

#ifndef __AGS_EE_GAME__LOADEDSAVEINFO_H
#define __AGS_EE_GAME__LOADEDSAVEINFO_H

#include <vector>

namespace AGS
{
namespace Engine
{

struct LoadedSaveInfo
{
    // Game content reference
    int AudioTypeCount;
    int AudioClipCount;
    int CharCount;
    int DialogCount;
    int GUICount;
    int GUIBtnCount;
    int GUILblCount;
    int GUIInvCount;
    int GUISldCount;
    int GUITbxCount;
    int GUILbxCount;
    int InvItemCount;
    int MouseCurCount;
    std::vector< std::vector<int> > Views;

    LoadedSaveInfo();
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__LOADEDSAVEINFO_H
