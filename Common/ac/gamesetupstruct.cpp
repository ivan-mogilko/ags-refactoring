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
#include "ac/gamesetupstruct.h"
#include "ac/audiocliptype.h"
#include "ac/dialogtopic.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "ac/wordsdictionary.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "script/cc_common.h"
#include "util/string_utils.h"

using namespace AGS::Common;


const char *GetScriptAPIName(ScriptAPIVersion v)
{
    switch (v)
    {
    case kScriptAPI_v321: return "v3.2.1";
    case kScriptAPI_v330: return "v3.3.0";
    case kScriptAPI_v334: return "v3.3.4";
    case kScriptAPI_v335: return "v3.3.5";
    case kScriptAPI_v340: return "v3.4.0";
    case kScriptAPI_v341: return "v3.4.1";
    case kScriptAPI_v350: return "v3.5.0-alpha";
    case kScriptAPI_v3507: return "v3.5.0-final";
    case kScriptAPI_v360: return "v3.6.0-alpha";
    case kScriptAPI_v36026: return "v3.6.0-final";
    case kScriptAPI_v361: return "v3.6.1";
    case kScriptAPI_v362: return "v3.6.2";
    case kScriptAPI_v399: return "3.99.x";
    case kScriptAPI_v400: return "4.0.0-alpha8";
    case kScriptAPI_v400_07: return "4.0.0-alpha12";
    case kScriptAPI_v400_14: return "4.0.0-alpha18";
    default: return "unknown";
    }
}

void AdjustFontInfoUsingFlags(FontInfo &finfo, const uint32_t flags)
{
    finfo.Flags = flags;
    if ((flags & FFLG_SIZEMULTIPLIER) != 0)
    {
        finfo.SizeMultiplier = finfo.Size;
        finfo.Size = 0;
    }
}

//-----------------------------------------------------------------------------

void GameBasicProperties::ReadFromFile(Stream *in, GameDataVersion game_ver, SerializeInfo &info)
{
    // NOTE: historically the struct was saved by dumping whole memory
    // into the file stream, which added padding from memory alignment;
    // here we mark the padding bytes, as they do not belong to actual data.
    gamename.ReadCount(in, LEGACY_GAME_NAME_LENGTH);
    in->ReadInt16(); // alignment padding to int32 (gamename: 50 -> 52 bytes)
    in->ReadArrayOfInt32(options, MAX_OPTIONS);
    in->Read(&paluses[0], sizeof(paluses));
    // colors are an array of chars
    in->Read(&defpal[0], sizeof(defpal));
    numviews = in->ReadInt32();
    numcharacters = in->ReadInt32();
    playercharacter = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    numinvitems = in->ReadInt16();
    in->ReadInt16(); // alignment padding to int32
    numdialog = in->ReadInt32();
    numdlgmessage = in->ReadInt32();
    numfonts = in->ReadInt32();
    color_depth = in->ReadInt32();
    target_win = in->ReadInt32();
    dialog_bullet = in->ReadInt32();
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hot cross color
    uniqueid = in->ReadInt32();
    numgui = in->ReadInt32();
    numcursors = in->ReadInt32();
    GameResolutionType resolution_type = (GameResolutionType)in->ReadInt32();
    assert(resolution_type == kGameResolution_Custom);
    game_resolution.Width = in->ReadInt32();
    game_resolution.Height = in->ReadInt32();

    default_lipsync_frame = in->ReadInt32();
    invhotdotsprite = in->ReadInt32();
    hotdot = in->ReadInt32();
    hotdotouter = in->ReadInt32();
    in->ReadArrayOfInt32(reserved, NUM_INTS_RESERVED);
    info.ExtensionOffset = static_cast<uint32_t>(in->ReadInt32());

    in->ReadArrayOfInt32(info.HasMessages.data(), NUM_LEGACY_GLOBALMES);
    info.HasWordsDict = in->ReadInt32() != 0;
    in->ReadInt32(); // globalscript (dummy 32-bit pointer value)
    in->ReadInt32(); // chars (dummy 32-bit pointer value)
    info.HasCCScript = in->ReadInt32() != 0;

    StrUtil::ReadCStrCount(guid, in, MAX_GUID_LENGTH);
    StrUtil::ReadCStrCount(saveGameFileExtension, in, MAX_SG_EXT_LENGTH);
    saveGameFolderName.ReadCount(in, LEGACY_MAX_SG_FOLDER_LEN);
}

void GameBasicProperties::WriteToFile(Stream *out, const SerializeInfo &info) const
{
    // NOTE: historically the struct was saved by dumping whole memory
    // into the file stream, which added padding from memory alignment;
    // here we mark the padding bytes, as they do not belong to actual data.
    gamename.WriteCount(out, LEGACY_GAME_NAME_LENGTH);
    out->WriteInt16(0); // alignment padding to int32
    out->WriteArrayOfInt32(options, MAX_OPTIONS);
    out->Write(&paluses[0], sizeof(paluses));
    // colors are an array of chars
    out->Write(&defpal[0], sizeof(defpal));
    out->WriteInt32(numviews);
    out->WriteInt32(numcharacters);
    out->WriteInt32(playercharacter);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt16(numinvitems);
    out->WriteInt16(0); // alignment padding to int32
    out->WriteInt32(numdialog);
    out->WriteInt32(numdlgmessage);
    out->WriteInt32(numfonts);
    out->WriteInt32(color_depth);
    out->WriteInt32(target_win);
    out->WriteInt32(dialog_bullet);
    out->WriteInt16(0); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    out->WriteInt16(0); // [DEPRECATED] uint16 value of a inv cursor hot cross color
    out->WriteInt32(uniqueid);
    out->WriteInt32(numgui);
    out->WriteInt32(numcursors);
    out->WriteInt32(0); // [DEPRECATED] resolution type
    out->WriteInt32(game_resolution.Width);
    out->WriteInt32(game_resolution.Height);
    out->WriteInt32(default_lipsync_frame);
    out->WriteInt32(invhotdotsprite);
    out->WriteInt32(hotdot);
    out->WriteInt32(hotdotouter);
    out->WriteArrayOfInt32(reserved, NUM_INTS_RESERVED);
    out->WriteByteCount(0, sizeof(int32_t) * NUM_LEGACY_GLOBALMES);
    out->WriteInt32(info.HasWordsDict ? 1 : 0);
    out->WriteInt32(0); // globalscript (dummy 32-bit pointer value)
    out->WriteInt32(0); // chars (dummy 32-bit pointer value)
    out->WriteInt32(info.HasCCScript ? 1 : 0);
}

//-----------------------------------------------------------------------------

GameSetupStruct::GameSetupStruct(LoadedGame &&loadedgame)
{
    static_cast<GameBasicProperties &>(*this) = std::move(static_cast<GameBasicProperties &&>(loadedgame));
    static_cast<GameObjectData &>(*this) = std::move(static_cast<GameObjectData &&>(loadedgame));
    static_cast<GameExtendedProperties &>(*this) = std::move(static_cast<GameExtendedProperties &&>(loadedgame));

    ApplySpriteFlags(loadedgame.SpriteFlags);
    OnResolutionSet();
}

void GameSetupStruct::ApplySpriteFlags(const std::vector<uint8_t> &sprflags)
{
    SpriteInfos.resize(sprflags.size());
    for (size_t i = 0; i < sprflags.size(); ++i)
    {
        SpriteInfos[i].Flags = sprflags[i];
    }
}

void GameSetupStruct::OnResolutionSet()
{
    _relativeUIMult = 1; // NOTE: this is remains of old logic, currently unused.
}

void GameSetupStruct::ReadFromSavegame(Stream *in)
{
    // of GameSetupStruct
    in->ReadArrayOfInt32(options, OPT_HIGHESTOPTION_321 + 1);
    options[OPT_LIPSYNCTEXT] = in->ReadInt32();
    // of GameSetupStructBase
    playercharacter = in->ReadInt32();
    dialog_bullet = in->ReadInt32();
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hot cross color
    invhotdotsprite = in->ReadInt32();
    default_lipsync_frame = in->ReadInt32();
}

void GameSetupStruct::WriteForSavegame(Stream *out)
{
    // of GameSetupStruct
    out->WriteArrayOfInt32(options, OPT_HIGHESTOPTION_321 + 1);
    out->WriteInt32(options[OPT_LIPSYNCTEXT]);
    // of GameSetupStructBase
    out->WriteInt32(playercharacter);
    out->WriteInt32(dialog_bullet);
    out->WriteInt16(0); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    out->WriteInt16(0); // [DEPRECATED] uint16 value of a inv cursor hot cross color
    out->WriteInt32(invhotdotsprite);
    out->WriteInt32(default_lipsync_frame);
}

//-----------------------------------------------------------------------------

HGameFileError LoadedGame::ReadFromFile(Common::Stream *in, GameDataVersion game_ver)
{
    GameBasicProperties::SerializeInfo sinfo;
    GameBasicProperties::ReadFromFile(in, game_ver, sinfo);
    if (GetGameRes().IsNull())
        return new MainGameFileError(kMGFErr_InvalidNativeResolution);

    // Font infos
    fonts.resize(numfonts);
    for (int i = 0; i < numfonts; ++i)
    {
        FontInfo fi;
        uint32_t flags = in->ReadInt32();
        fi.Size = in->ReadInt32();
        fi.Outline = in->ReadInt32();
        fi.YOffset = in->ReadInt32();
        fi.LineSpacing = std::max(0, in->ReadInt32());
        AdjustFontInfoUsingFlags(fi, flags);
        fonts[i] = fi;
    }

    HGameFileError err = ReadSpriteFlags(in, game_ver);
    if (!err)
        return err;

    // Inventory items
    for (int i = 0; i < numinvitems; ++i)
    {
        invinfo[i].ReadFromFile(in);
    }

    // Cursors
    mcurs.resize(numcursors);
    for (int i = 0; i < numcursors; ++i)
    {
        mcurs[i].ReadFromFile(in);
    }

    // Interaction scripts
    charScripts.resize(numcharacters);
    invScripts.resize(numinvitems);
    for (size_t i = 0; i < (size_t)numcharacters; ++i)
        charScripts[i] = InteractionEvents::CreateFromStream_v361(in);
    // NOTE: new inventory items' events are loaded starting from 1 for some reason
    for (size_t i = 1; i < (size_t)numinvitems; ++i)
        invScripts[i] = InteractionEvents::CreateFromStream_v361(in);

    if (sinfo.HasWordsDict)
    {
        dict.reset(new WordsDictionary());
        read_dictionary(dict.get(), in);
    }

    if (sinfo.HasCCScript)
    {
        GlobalScript.reset(ccScript::CreateFromStream(in));
        if (!GlobalScript)
            return new MainGameFileError(kMGFErr_CreateGlobalScriptFailed, cc_get_error().ErrorString);
        err = ReadDialogScript(in, game_ver);
        if (!err)
            return err;
        err = ReadScriptModules(in, game_ver);
        if (!err)
            return err;
    }

    // Views
    Views.resize(numviews);
    for (int i = 0; i < numviews; ++i)
    {
        Views[i].ReadFromFile(in);
    }

    // Character data
    chars.resize(numcharacters);
    for (int i = 0; i < numcharacters; ++i)
    {
        chars[i].ReadFromFile(in, loaded_game_file_version);
    }
    CharEx.resize(numcharacters);

    // Lip sync data
    in->ReadArray(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50);

    // Global message data (deprecated and unused)
    for (int i = 0; i < GameBasicProperties::NUM_LEGACY_GLOBALMES; ++i)
    {
        if (!sinfo.HasMessages[i])
            continue;
        skip_string_decrypt(in);
    }

    Dialogs.resize(numdialog);
    for (int i = 0; i < numdialog; ++i)
    {
        Dialogs[i].ReadFromFile(in);
    }

    GUIRefCollection guictrl_refs(GuiControls);
    HError err2 = GUI::ReadGUI(Guis, guictrl_refs, in);
    if (!err2)
        return new MainGameFileError(kMGFErr_GameEntityFailed, err2);
    numgui = Guis.size();

    err = ReadPluginInfos(in, game_ver);
    if (!err)
        return err;

    err = ReadCustomProperties(in, game_ver);
    if (!err)
        return err;

    viewNames.resize(numviews);
    for (int i = 0; i < numviews; ++i)
        viewNames[i] = String::FromStream(in);

    for (int i = 0; i < numinvitems; ++i)
        invScriptNames[i] = String::FromStream(in);

    dialogScriptNames.resize(numdialog);
    for (int i = 0; i < numdialog; ++i)
        dialogScriptNames[i] = String::FromStream(in);

    err = ReadAudio(in, game_ver);
    if (!err)
        return err;
    ReadRoomNames(in, game_ver);
    return HGameFileError::None();
}

HGameFileError LoadedGame::ReadSpriteFlags(Stream *in, GameDataVersion data_ver)
{
    size_t sprcount = in->ReadInt32();
    if (sprcount > (size_t)SpriteCache::MAX_SPRITE_INDEX + 1)
        return new MainGameFileError(kMGFErr_TooManySprites, String::FromFormat("Count: %zu, max: %zu", sprcount, (size_t)SpriteCache::MAX_SPRITE_INDEX + 1));

    SpriteCount = sprcount;
    SpriteFlags.resize(sprcount);
    in->Read(SpriteFlags.data(), sprcount);
    return HGameFileError::None();
}

HGameFileError LoadedGame::ReadDialogScript(Stream *in, GameDataVersion game_ver)
{
    DialogScript.reset(ccScript::CreateFromStream(in));
    if (DialogScript == nullptr)
        return new MainGameFileError(kMGFErr_CreateDialogScriptFailed, cc_get_error().ErrorString);
    return HGameFileError::None();
}

HGameFileError LoadedGame::ReadScriptModules(Stream *in, GameDataVersion game_ver)
{
    int count = in->ReadInt32();
    ScriptModules.resize(count);
    for (int i = 0; i < count; ++i)
    {
        ScriptModules[i].reset(ccScript::CreateFromStream(in));
        if (ScriptModules[i] == nullptr)
            return new MainGameFileError(kMGFErr_CreateScriptModuleFailed, cc_get_error().ErrorString);
    }
    return HGameFileError::None();
}

HGameFileError LoadedGame::ReadPluginInfos(Stream *in, GameDataVersion game_ver)
{
    int fmt_ver = in->ReadInt32();
    if (fmt_ver != 1)
        return new MainGameFileError(kMGFErr_PluginDataFmtNotSupported, String::FromFormat("Version: %d, supported: %d", fmt_ver, 1));

    int pl_count = in->ReadInt32();
    for (int i = 0; i < pl_count; ++i)
    {
        String name = String::FromStream(in);
        size_t datasize = in->ReadInt32();
        // just check for silly datasizes
        if (datasize > PLUGIN_SAVEBUFFERSIZE)
            return new MainGameFileError(kMGFErr_PluginDataSizeTooLarge, String::FromFormat("Required: %zu, max: %zu", datasize, (size_t)PLUGIN_SAVEBUFFERSIZE));

        PluginInfo info;
        info.Name = name;
        if (datasize > 0)
        {
            info.Data.resize(datasize);
            in->Read(info.Data.data(), datasize);
        }
        PluginInfos.push_back(info);
    }
    return HGameFileError::None();
}

HGameFileError LoadedGame::ReadCustomProperties(Stream *in, GameDataVersion game_ver)
{
    if (Properties::ReadSchema(propSchema, in) != kPropertyErr_NoError)
        return new MainGameFileError(kMGFErr_InvalidPropertySchema);

    int errors = 0;

    charProps.resize(numcharacters);
    for (int i = 0; i < numcharacters; ++i)
    {
        errors += Properties::ReadValues(charProps[i], in);
    }
    for (int i = 0; i < numinvitems; ++i)
    {
        errors += Properties::ReadValues(invProps[i], in);
    }

    if (errors > 0)
        return new MainGameFileError(kMGFErr_InvalidPropertyValues);


    return HGameFileError::None();
}

HGameFileError LoadedGame::ReadAudio(Stream *in, GameDataVersion game_ver)
{
    size_t audiotype_count = in->ReadInt32();
    audioClipTypes.resize(audiotype_count);
    for (size_t i = 0; i < audiotype_count; ++i)
    {
        audioClipTypes[i].ReadFromFile(in);
    }

    size_t audioclip_count = in->ReadInt32();
    audioClips.resize(audioclip_count);
    for (size_t i = 0; i < audioclip_count; ++i)
    {
        audioClips[i].ReadFromFile(in);
    }

    in->ReadInt32(); // [DEPRECATED]
    return HGameFileError::None();
}

HGameFileError LoadedGame::ReadRoomNames(Stream *in, GameDataVersion game_ver)
{
    if ((game_ver >= kGameVersion_400_13) ||
        (options[OPT_DEBUGMODE] != 0))
    {
        uint32_t room_count = in->ReadInt32();
        roomNumbers.resize(room_count);
        for (uint32_t i = 0; i < room_count; ++i)
        {
            int room_number = in->ReadInt32();
            String room_name = String::FromStream(in);
            roomNumbers[i] = room_number;
            roomNames.insert(std::make_pair(room_number, room_name));
        }
    }
    return HGameFileError::None();
}
