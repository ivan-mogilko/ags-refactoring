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
#include "game/savegame_internal.h"
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
    kSvgBlock_Undefined = -1,
    kSvgBlock_Description,
    kSvgBlock_GameState_PlayStruct,
    kSvgBlock_GameState_Audio,
    kSvgBlock_GameState_Characters,
    kSvgBlock_GameState_Dialogs,
    kSvgBlock_GameState_GUI,
    kSvgBlock_GameState_InventoryItems,
    kSvgBlock_GameState_MouseCursors,
    kSvgBlock_GameState_Views,
    kSvgBlock_GameState_DynamicSprites,
    kSvgBlock_GameState_Overlays,
    kSvgBlock_GameState_DynamicSurfaces,
    kSvgBlock_GameState_ScriptModules,
    kSvgBlock_RoomStates_AllRooms,
    kSvgBlock_RoomStates_ThisRoom,
    kSvgBlock_ManagedPool,
    kSvgBlock_PluginData,
    kNumSavegameBlocks,
    // Range of block types that can be read in free order
    kSvgBlock_FirstRandomType = kSvgBlock_GameState_PlayStruct,
    kSvgBlock_LastRandomType = kSvgBlock_PluginData
};

namespace SavegameBlocks
{
    Bitmap *RestoreSaveImage(Stream *in);
    void SkipSaveImage(Stream *in);
    // Reads a description block
    SavegameError ReadDescription(Stream *in, SavegameDescription &desc, SavegameDescElem elems);
    // Writes a description block
    void WriteDescription(Stream *out, const String &user_text, const Bitmap *user_image);
    // Reads next block from the stream
    SavegameError ReadBlock(Stream *in, SavegameVersion svg_version, const PreservedParams &pp, RestoredData &r_data);
    // Reads a list of blocks from the stream
    SavegameError ReadBlockList(Stream *in, SavegameVersion svg_version, const PreservedParams &pp, RestoredData &r_data);
    // Writes a block of free-order type to the stream
    SavegameError WriteRandomBlock(Stream *out, SavegameBlockType type);
    // Writes a full list of common blocks to the stream
    SavegameError WriteAllCommonBlocks(Stream *out);
}

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEGAMEBLOCKS_H
