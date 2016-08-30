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

#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
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

enum SavegameBlockFlags
{
    kSvgBlk_None       = 0,
    // An optional block, safe to skip if not supported
    kSvgBlk_Optional   = 0x0001
};

struct SavegameBlockInfo
{
    // Opening and closing signatures
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
    }
    else
    {
        StrUtil::SkipSmallString(in);
        StrUtil::SkipSmallString(in);
        StrUtil::SkipSmallString(in);
        StrUtil::SkipString(in);
        StrUtil::SkipSmallString(in);
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

} // namespace SavegameBlocks
} // namespace Engine
} // namespace AGS
