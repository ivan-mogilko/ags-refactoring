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

#ifndef __AGS_EE_GAME__SAVEGAMEBLOCKS_H
#define __AGS_EE_GAME__SAVEGAMEBLOCKS_H

#include "game/savegame.h"
#include "gfx/bitmap.h"
#include "util/stream.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{

using Common::Stream;
using Common::String;
using Common::Bitmap;

// Supported types of save blocks
enum SavegameBlockType
{
    kSvgBlock_Undefined,
    kSvgBlock_Description,
    kNumSavegameBlocks,
    kSvgBlock_FirstType = kSvgBlock_Description,
    kSvgBlock_LastType = kSvgBlock_Description
};

namespace SavegameBlocks
{
    Bitmap *RestoreSaveImage(Stream *in);
    void SkipSaveImage(Stream *in);
    // Reads a description block
    SavegameError ReadDescription(Stream *in, SavegameDescription &desc, SavegameDescElem elems);
    // Writes a description block
    void WriteDescription(Stream *out, const String &user_text, const Bitmap *user_image);
}

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEGAMEBLOCKS_H
