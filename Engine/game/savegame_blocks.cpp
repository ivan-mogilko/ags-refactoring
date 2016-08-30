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

#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "debug/out.h"
#include "game/savegame_blocks.h"
#include "main/main.h"
#include "util/string_utils.h"

extern GameSetupStruct game;

namespace AGS
{
namespace Engine
{

using namespace Common;

namespace SavegameBlocks
{

// Opening and closing signatures of the save blocks list
static const String BlockListOpenSig  = "BlockListBEG";
static const String BlockListCloseSig = "BlockListEND";

enum SavegameBlockFlags
{
    kSvgBlk_None       = 0,
    // An optional block, safe to skip if not supported
    kSvgBlk_Optional   = 0x0001
};

// Description of a single block
struct SavegameBlockInfo
{
    // Opening and closing block signatures
    static const int32_t OpenSignature  = 0xABCDEFFF;
    static const int32_t CloseSignature = 0xFEDCBAAA;

    // Block data type, determines which handler to use
    SavegameBlockType   Type;
    // Format version to pass to the block handler
    int32_t             Version;
    // Auxiliary flags
    SavegameBlockFlags  Flags;
    // Data position in stream
    size_t              DataOffset;
    // Block data size, in bytes
    size_t              DataLength;

    SavegameBlockInfo()
        : Type(kSvgBlock_Undefined)
        , Version(0)
        , Flags(kSvgBlk_None)
        , DataOffset(0)
        , DataLength(0)
    {}

    SavegameBlockInfo(SavegameBlockType type, int32_t version = 0, SavegameBlockFlags flags = kSvgBlk_None)
        : Type(type)
        , Version(version)
        , Flags(flags)
        , DataOffset(0)
        , DataLength(0)
    {}
};

static SavegameError BeginReadBlock(Stream *in, SavegameBlockInfo &binfo)
{
    size_t at = in->GetPosition();
    if (in->ReadInt32() != SavegameBlockInfo::OpenSignature)
        return kSvgErr_BlockOpenSigMismatch;
    binfo.Type = (SavegameBlockType)in->ReadInt32();
    binfo.Version = in->ReadInt32();
    binfo.Flags = (SavegameBlockFlags)in->ReadInt32();
    binfo.DataLength = in->ReadInt32();
    binfo.DataOffset = in->GetPosition();
    return kSvgErr_NoError;
}

static SavegameError EndReadBlock(Stream *in, SavegameBlockInfo &binfo)
{
    if (in->ReadInt32() != SavegameBlockInfo::CloseSignature)
        return kSvgErr_BlockCloseSigMismatch;
    return kSvgErr_NoError;
}

Bitmap *RestoreSaveImage(Stream *in)
{
    if (in->ReadInt32())
        return read_serialized_bitmap(in);
    return NULL;
}

void SkipSaveImage(Stream *in)
{
    if (in->ReadInt32())
        skip_serialized_bitmap(in);
}

SavegameError ReadDescription(Stream *in, SavegameDescription &desc, SavegameDescElem elems)
{
    SavegameBlockInfo binfo;
    SavegameError err = BeginReadBlock(in, binfo);
    if (err != kSvgErr_NoError)
        return err;
    else if (binfo.Type != kSvgBlock_Description)
        return kSvgErr_MismatchingBlockType;

    // Enviroment information
    if (elems & kSvgDesc_EnvInfo)
    {
        desc.EngineName = StrUtil::ReadSmallString(in);
        desc.EngineVersion.SetFromString(StrUtil::ReadSmallString(in));
        desc.GameGuid = StrUtil::ReadSmallString(in);
        desc.GameTitle = StrUtil::ReadString(in);
        desc.MainDataFilename = StrUtil::ReadSmallString(in);
        desc.ColorDepth = in->ReadInt32();
    }
    else
    {
        StrUtil::SkipSmallString(in);
        StrUtil::SkipSmallString(in);
        StrUtil::SkipSmallString(in);
        StrUtil::SkipString(in);
        StrUtil::SkipSmallString(in);
        in->ReadInt32(); // color depth
    }
    // User description
    if (elems & kSvgDesc_UserText)
        desc.UserText = StrUtil::ReadString(in);
    else
        StrUtil::SkipString(in);
    if (elems & kSvgDesc_UserImage)
        desc.UserImage.reset(RestoreSaveImage(in));
    else
        SkipSaveImage(in);

    return EndReadBlock(in, binfo);
}

static void BeginWriteBlock(Stream *out, SavegameBlockInfo &binfo)
{
    size_t at = out->GetPosition();
    out->WriteInt32(SavegameBlockInfo::OpenSignature);
    out->WriteInt32(binfo.Type);
    out->WriteInt32(binfo.Version);
    out->WriteInt32(binfo.Flags);
    out->WriteInt32(0); // data length is not know yet
    binfo.DataOffset = out->GetPosition();
}

static void EndWriteBlock(Stream *out, SavegameBlockInfo &binfo)
{
    // calculate the total block length, and write it back into the header
    size_t end_pos = out->GetPosition();
    binfo.DataLength = end_pos - binfo.DataOffset;
    out->Seek(binfo.DataOffset - sizeof(int32_t), kSeekBegin);
       size_t at = out->GetPosition();
    out->WriteInt32(binfo.DataLength);
    out->Seek(end_pos, kSeekBegin);
        at = out->GetPosition();
    // write the ending signature
    out->WriteInt32(SavegameBlockInfo::CloseSignature);
}

static void WriteSaveImage(Stream *out, const Bitmap *screenshot)
{
    // store the screenshot at the start to make it easily accesible
    out->WriteInt32((screenshot == NULL) ? 0 : 1);

    if (screenshot)
        serialize_bitmap(screenshot, out);
}

void WriteDescription(Stream *out, const String &user_text, const Bitmap *user_image)
{
    SavegameBlockInfo binfo(kSvgBlock_Description);
    BeginWriteBlock(out, binfo);

    // Enviroment information
    StrUtil::WriteSmallString("Adventure Game Studio run-time engine", out);
    StrUtil::WriteSmallString(EngineVersion.LongString, out);
    StrUtil::WriteSmallString(game.guid, out);
    StrUtil::WriteString(game.gamename, out);
    StrUtil::WriteSmallString(usetup.main_data_filename, out);
    // User description
    StrUtil::WriteString(user_text, out);
    WriteSaveImage(out, user_image);

    EndWriteBlock(out, binfo);
}


struct BlockHandler
{
    SavegameBlockType  Type;
    String             Name;
    int32_t            Version;
    SavegameError      (*Serialize)  (Stream*);
    SavegameError      (*Unserialize)(Stream*, int32_t blk_ver, const PreservedParams&, RestoredData&);
};

BlockHandler BlockHandlers[kNumSavegameBlocks] =
{
    {
        kSvgBlock_Description, "Description",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_PlayStruct, "Game State",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_Audio, "Audio",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_Characters, "Characters",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_Dialogs, "Dialogs",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_GUI, "GUI",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_InventoryItems, "Inventory Items",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_MouseCursors, "Mouse Cursors",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_Views, "Views",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_DynamicSprites, "Dynamic Sprites",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_Overlays, "Overlays",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_DynamicSurfaces, "Dynamic Surfaces",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_GameState_ScriptModules, "Script Modules",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_RoomStates_AllRooms, "Room States",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_RoomStates_ThisRoom, "Running Room State",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_ManagedPool, "Managed Pool",
        0,
        NULL,
        NULL
    },
    {
        kSvgBlock_PluginData, "Plugin Data",
        0,
        NULL,
        NULL
    }
};


SavegameError ReadBlock(Stream *in, SavegameVersion svg_version, const PreservedParams &pp,
                        RestoredData &r_data, SavegameBlockInfo *get_binfo)
{
    SavegameBlockInfo binfo;
    SavegameError err = BeginReadBlock(in, binfo);
    if (err != kSvgErr_NoError)
        return err;
    if (get_binfo)
        *get_binfo = binfo;

    const bool known = binfo.Type >= 0 && binfo.Type < kNumSavegameBlocks;
    const bool expected = binfo.Type >= kSvgBlock_FirstRandomType && binfo.Type <= kSvgBlock_LastRandomType;
    const bool optional = binfo.Flags & kSvgBlk_Optional;
    const bool good_version = binfo.Version >= 0 && binfo.Version <= BlockHandlers[binfo.Type].Version;
    const bool supported = known && BlockHandlers[binfo.Type].Unserialize && good_version;
    if (!expected || !supported)
    {
        Out::FPrint("%s: %s block in save (%s): type = %d (%s), v = %d, off = %u, len = %u",
            optional ? "WARNING" : "ERROR",
            known ? (supported ? "unexpected" : "unsupported") : "unknown",
            optional ? "skip" : "break",
            binfo.Type, known ? BlockHandlers[binfo.Type].Name.GetCStr() : "?", binfo.Version, binfo.DataOffset, binfo.DataLength);
        if (!optional)
            return good_version ? kSvgErr_UnsupportedBlockType : kSvgErr_DataVersionNotSupported;
    }

    if (supported)
    {
        err = BlockHandlers[binfo.Type].Unserialize(in, binfo.Version, pp, r_data);
        if (err != kSvgErr_NoError)
            return err;
        update_polled_stuff_if_runtime();
    }
    else
    {
        in->Seek(binfo.DataLength);
    }

    return EndReadBlock(in, binfo);
}

SavegameError ReadBlock(Stream *in, SavegameVersion svg_version, const PreservedParams &pp, RestoredData &r_data)
{
    return ReadBlock(in, svg_version, pp, r_data, NULL);
}

SavegameError ReadBlockList(Stream *in, SavegameVersion svg_version, const PreservedParams &pp, RestoredData &r_data)
{
    String sig;
    sig.ReadCount(in, BlockListOpenSig.GetLength());
    if (sig.Compare(BlockListOpenSig))
        return kSvgErr_BlockListOpenSigMismatch;

    bool end_found = false;
    size_t blk_index = 0;
    while (!in->EOS())
    {
        sig.ReadCount(in, BlockListCloseSig.GetLength());
        end_found = sig.Compare(BlockListCloseSig) == 0;
        if (end_found)
            break;
        in->Seek(-(int)BlockListCloseSig.GetLength());

        SavegameBlockInfo binfo;
        SavegameError err = ReadBlock(in, svg_version, pp, r_data, &binfo);
        if (err != kSvgErr_NoError)
        {
            Out::FPrint("ERROR: failed to read save block: index = %u, type = %d (%s), v = %d, off = %u, len = %u",
                blk_index, binfo.Type,
                (binfo.Type >= 0 && binfo.Type < kNumSavegameBlocks) ? BlockHandlers[binfo.Type].Name.GetCStr() : "?",
                binfo.Version, binfo.DataOffset, binfo.DataLength);
            return err;
        }
        blk_index++;
    }

    if (!end_found)
        return kSvgErr_BlockListEndNotFound;
    return kSvgErr_NoError;
}

SavegameError WriteRandomBlock(Stream *out, SavegameBlockType type)
{
    if (type < kSvgBlock_FirstRandomType || type > kSvgBlock_LastRandomType)
        return kSvgErr_UnsupportedBlockType;
    if (!BlockHandlers[type].Serialize)
        return kSvgErr_UnsupportedBlockType;

    SavegameBlockInfo binfo(type, BlockHandlers[type].Version);
    BeginWriteBlock(out, binfo);
    SavegameError err = BlockHandlers[type].Serialize(out);
    if (err != kSvgErr_NoError)
        return err;
    EndWriteBlock(out, binfo);
    return kSvgErr_NoError;
}

SavegameError WriteAllCommonBlocks(Stream *out)
{
    out->Write(BlockListOpenSig, BlockListOpenSig.GetLength());
    for (int type = kSvgBlock_FirstRandomType; type != kSvgBlock_LastRandomType; ++type)
    {
        SavegameError err = WriteRandomBlock(out, (SavegameBlockType)type);
        if (err != kSvgErr_NoError && err != kSvgErr_UnsupportedBlockType)
            return err;
        update_polled_stuff_if_runtime();
    }
    out->Write(BlockListCloseSig, BlockListCloseSig.GetLength());
    return kSvgErr_NoError;
}

} // namespace SavegameBlocks
} // namespace Engine
} // namespace AGS
