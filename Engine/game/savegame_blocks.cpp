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

#include "ac/character.h"
#include "ac/common.h"
#include "ac/dialogtopic.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/gui.h"
#include "ac/movelist.h"
#include "ac/mouse.h"
#include "ac/roomstatus.h"
#include "ac/roomstruct.h"
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "ac/dynobj/cc_serializer.h"
#include "debug/out.h"
#include "game/savegame_blocks.h"
#include "gui/animatingguibutton.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "main/main.h"
#include "media/audio/audio.h"
#include "media/audio/soundclip.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "script/cc_error.h"
#include "script/script.h"
#include "util/filestream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern color palette[256];
extern DialogTopic *dialog;
extern AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
extern int numAnimButs;
extern ViewStruct *views;
extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int numscreenover;
extern Bitmap *dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];
extern roomstruct thisroom;
extern RoomStatus troom;
extern Bitmap *raw_saved_screen;
extern MoveList *mls;


namespace AGS
{
namespace Engine
{

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


inline bool AssertFormat(int original, int value, const char *test_name, int test_id = -1)
{
    if (value != original)
    {
        Out::FPrint("Restore game error: format consistency assertion failed at %s (%d)", test_name, test_id);
        return false;
    }
    return true;
}

inline bool AssertGameContent(int original_val, int new_val, bool strict = false)
{
    return new_val == original_val ||
        (!strict &&
         ((game.options[OPT_SAVEGAMECOMPAT] & kSvgCompat_MissingContent) && new_val < original_val)
        );
}

bool AssertGameContent(int original_val, int new_val, const char *content_name, bool strict = false)
{
    if (!AssertGameContent(original_val, new_val))
    {
        Out::FPrint("Restore game error: mismatching number of %s (game: %d, save: %d)",
            content_name, original_val, new_val);
        return false;
    }
    return true;
}

inline bool AssertGameContStrict(int original_val, int new_val, const char *content_name)
{
    return AssertGameContent(original_val, new_val, content_name, true);
}

bool AssertGameObjectContent(int original_val, int new_val, const char *content_name,
                                    const char *obj_type, int obj_id, bool strict = false)
{
    if (!AssertGameContent(original_val, new_val))
    {
        Out::FPrint("Restore game error: mismatching number of %s in %s #%d (game: %d, save: %d)",
            content_name, obj_type, obj_id, original_val, new_val);
        return false;
    }
    return true;
}

bool AssertGameObjectContent2(int original_val, int new_val, const char *content_name,
                                    const char *obj1_type, int obj1_id, const char *obj2_type, int obj2_id, bool strict = false)
{
    if (!AssertGameContent(original_val, new_val))
    {
        Out::FPrint("Restore game error: mismatching number of %s in %s #%d, %s #%d (game: %d, save: %d)",
            content_name, obj1_type, obj1_id, obj2_type, obj2_id, original_val, new_val);
        return false;
    }
    return true;
}

inline bool AssertContentMatch(int expected, int actual, const char *reference, const char *dependant)
{
    if (actual != expected)
    {
        Out::FPrint("Restore game error: number of %s does not match number of %s (expected: %d, got: %d)",
            dependant, reference, expected, actual);
        return false;
    }
    return true;
}


SavegameError WriteGameState(Stream *out)
{
    out->WriteInt32(ScreenResolution.ColorDepth);

    // Game base
    game.WriteForSavegame(out);
    // Game palette
    // TODO: probably no need to save this for hi/true-res game
    out->WriteArray(palette, sizeof(color), 256);

    if (loaded_game_file_version <= kGameVersion_272)
    {
        // Global variables
        out->WriteInt32(numGlobalVars);
        for (int i = 0; i < numGlobalVars; ++i)
            globalvars[i].Write(out);
    }

    // Game state
    play.WriteForSavegame(out);
    // Other dynamic values
    out->WriteInt32(frames_per_second);
    out->WriteInt32(loopcounter);
    out->WriteInt32(ifacepopped);
    out->WriteInt32(game_paused);
    // Mouse cursor
    out->WriteInt32(cur_mode);
    out->WriteInt32(cur_cursor);
    out->WriteInt32(mouse_on_iface);
    // Viewport
    out->WriteInt32(offsetx);
    out->WriteInt32(offsety);
    return kSvgErr_NoError;
}

SavegameError ReadGameState(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    // CHECKME: is this still essential? if yes, is there possible workaround?
    if (in->ReadInt32() != ScreenResolution.ColorDepth)
        return kSvgErr_DifferentColorDepth;

    // Game base
    game.ReadFromSavegame(in);
    // Game palette
    in->ReadArray(palette, sizeof(color), 256);

    if (loaded_game_file_version <= kGameVersion_272)
    {
        // Legacy interaction global variables
        if (!AssertGameContStrict(numGlobalVars, in->ReadInt32(), "Global Variables"))
            return kSvgErr_GameContentAssertion;
        for (int i = 0; i < numGlobalVars; ++i)
            globalvars[i].Read(in);
    }

    // Game state
    play.ReadFromSavegame(in, false);

    // Other dynamic values
    r_data.FPS = in->ReadInt32();
    loopcounter = in->ReadInt32();
    ifacepopped = in->ReadInt32();
    game_paused = in->ReadInt32();
    // Mouse cursor state
    r_data.CursorMode = in->ReadInt32();
    r_data.CursorID = in->ReadInt32();
    mouse_on_iface = in->ReadInt32();
    // Viewport state
    offsetx = in->ReadInt32();
    offsety = in->ReadInt32();
    return kSvgErr_NoError;
}

SavegameError WriteAudio(Stream *out)
{
    // Game content assertion
    out->WriteInt32(game.audioClipTypeCount);
    out->WriteInt32(game.audioClipCount);
    // Audio types
    for (int i = 0; i < game.audioClipTypeCount; ++i)
    {
        game.audioClipTypes[i].WriteToSavegame(out);
        out->WriteInt32(play.default_audio_type_volumes[i]);
    }

    // Audio clips and crossfade
    for (int i = 0; i <= MAX_SOUND_CHANNELS; i++)
    {
        if ((channels[i] != NULL) && (channels[i]->done == 0) && (channels[i]->sourceClip != NULL))
        {
            out->WriteInt32(((ScriptAudioClip*)channels[i]->sourceClip)->id);
            out->WriteInt32(channels[i]->get_pos());
            out->WriteInt32(channels[i]->priority);
            out->WriteInt32(channels[i]->repeat ? 1 : 0);
            out->WriteInt32(channels[i]->vol);
            out->WriteInt32(channels[i]->panning);
            out->WriteInt32(channels[i]->volAsPercentage);
            out->WriteInt32(channels[i]->panningAsPercentage);
            out->WriteInt32(channels[i]->speed);
        }
        else
        {
            out->WriteInt32(-1);
        }
    }
    out->WriteInt32(crossFading);
    out->WriteInt32(crossFadeVolumePerStep);
    out->WriteInt32(crossFadeStep);
    out->WriteInt32(crossFadeVolumeAtStart);
    // CHECKME: why this needs to be saved?
    out->WriteInt32(current_music_type);

    // Ambient sound
    for (int i = 0; i < MAX_SOUND_CHANNELS; ++i)
        ambient[i].WriteToFile(out);
    return kSvgErr_NoError;
}

SavegameError ReadAudio(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    // Game content assertion
    r_data.SaveInfo.AudioTypeCount = in->ReadInt32();
    r_data.SaveInfo.AudioClipCount = in->ReadInt32();
    if (!AssertGameContent(game.audioClipTypeCount, r_data.SaveInfo.AudioTypeCount, "Audio Clip Types"))
        return kSvgErr_GameContentAssertion;
    if (!AssertGameContent(game.audioClipCount, r_data.SaveInfo.AudioClipCount, "Audio Clips"))
        return kSvgErr_GameContentAssertion;

    // Audio types
    for (int i = 0; i < r_data.SaveInfo.AudioTypeCount; ++i)
    {
        game.audioClipTypes[i].ReadFromSavegame(in);
        play.default_audio_type_volumes[i] = in->ReadInt32();
    }

    // Audio clips and crossfade
    for (int i = 0; i <= MAX_SOUND_CHANNELS; ++i)
    {
        RestoredData::ChannelInfo &chan_info = r_data.AudioChans[i];
        chan_info.Pos = 0;
        chan_info.ClipID = in->ReadInt32();
        if (chan_info.ClipID >= 0)
        {
            chan_info.Pos = in->ReadInt32();
            if (chan_info.Pos < 0)
                chan_info.Pos = 0;
            chan_info.Priority = in->ReadInt32();
            chan_info.Repeat = in->ReadInt32();
            chan_info.Vol = in->ReadInt32();
            chan_info.Pan = in->ReadInt32();
            chan_info.VolAsPercent = in->ReadInt32();
            chan_info.PanAsPercent = in->ReadInt32();
            chan_info.Speed = 1000;
            chan_info.Speed = in->ReadInt32();
        }
    }
    crossFading = in->ReadInt32();
    crossFadeVolumePerStep = in->ReadInt32();
    crossFadeStep = in->ReadInt32();
    crossFadeVolumeAtStart = in->ReadInt32();
    // preserve legacy music type setting
    current_music_type = in->ReadInt32();
    
    // Ambient sound
    for (int i = 0; i < MAX_SOUND_CHANNELS; ++i)
        ambient[i].ReadFromFile(in);
    for (int i = 1; i < MAX_SOUND_CHANNELS; ++i)
    {
        if (ambient[i].channel == 0)
        {
            r_data.DoAmbient[i] = 0;
        }
        else
        {
            r_data.DoAmbient[i] = ambient[i].num;
            ambient[i].channel = 0;
        }
    }
    return kSvgErr_NoError;
}

SavegameError WriteCharacters(Stream *out)
{
    out->WriteInt32(game.numcharacters);
    for (int i = 0; i < game.numcharacters; ++i)
    {
        game.chars[i].WriteToFile(out);
        charextra[i].WriteToFile(out);
        Properties::WriteValues(play.charProps[i], out);
        if (loaded_game_file_version <= kGameVersion_272)
            game.intrChar[i]->WriteTimesRunToSavedgame(out);
        // character movement path cache
        mls[CHMLSOFFS + i].WriteToFile(out);
    }
    return kSvgErr_NoError;
}

SavegameError ReadCharacters(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    r_data.SaveInfo.CharCount = in->ReadInt32();
    if (!AssertGameContent(game.numcharacters, r_data.SaveInfo.CharCount, "Characters"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.CharCount; ++i)
    {
        game.chars[i].ReadFromFile(in);
        charextra[i].ReadFromFile(in);
        Properties::ReadValues(play.charProps[i], in);
        if (loaded_game_file_version <= kGameVersion_272)
            game.intrChar[i]->ReadTimesRunFromSavedgame(in);
        // character movement path cache
        mls[CHMLSOFFS + i].ReadFromFile(in);
    }
    return kSvgErr_NoError;
}

SavegameError WriteDialogs(Stream *out)
{
    out->WriteInt32(game.numdialog);
    for (int i = 0; i < game.numdialog; ++i)
    {
        dialog[i].WriteToSavegame(out);
    }
    return kSvgErr_NoError;
}

SavegameError ReadDialogs(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    r_data.SaveInfo.DialogCount = in->ReadInt32();
    if (!AssertGameContent(game.numdialog, r_data.SaveInfo.DialogCount, "Dialogs"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.DialogCount; ++i)
    {
        dialog[i].ReadFromSavegame(in);
    }
    return kSvgErr_NoError;
}

static const int FormatConsistencyCheck = 0xbeefcafe;

SavegameError WriteGUI(Stream *out)
{
    // GUI state
    out->WriteInt32(game.numgui);
    for (int i = 0; i < game.numgui; ++i)
        guis[i].WriteToSavegame(out);

    out->WriteInt32(FormatConsistencyCheck);

    out->WriteInt32(numguibuts);
    for (int i = 0; i < numguibuts; ++i)
        guibuts[i].WriteToSavegame(out);

    out->WriteInt32(FormatConsistencyCheck);

    out->WriteInt32(numguilabels);
    for (int i = 0; i < numguilabels; ++i)
        guilabels[i].WriteToSavegame(out);

    out->WriteInt32(FormatConsistencyCheck);

    out->WriteInt32(numguiinv);
    for (int i = 0; i < numguiinv; ++i)
        guiinv[i].WriteToSavegame(out);

    out->WriteInt32(FormatConsistencyCheck);

    out->WriteInt32(numguislider);
    for (int i = 0; i < numguislider; ++i)
        guislider[i].WriteToSavegame(out);

    out->WriteInt32(FormatConsistencyCheck);

    out->WriteInt32(numguitext);
    for (int i = 0; i < numguitext; ++i)
        guitext[i].WriteToSavegame(out);

    out->WriteInt32(FormatConsistencyCheck);

    out->WriteInt32(numguilist);
    for (int i = 0; i < numguilist; ++i)
        guilist[i].WriteToSavegame(out);

    out->WriteInt32(FormatConsistencyCheck);

    // Animated buttons
    out->WriteInt32(numAnimButs);
    for (int i = 0; i < numAnimButs; ++i)
        animbuts[i].WriteToFile(out);
    return kSvgErr_NoError;
}

SavegameError ReadGUI(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    // GUI state
    r_data.SaveInfo.GUICount = in->ReadInt32();
    if (!AssertGameContent(game.numgui, r_data.SaveInfo.GUICount, "GUIs"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.GUICount; ++i)
        guis[i].ReadFromSavegame(in);

    if (!AssertFormat(FormatConsistencyCheck, in->ReadInt32(), "GUI Buttons"))
        return kSvgErr_InconsistentFormat;

    r_data.SaveInfo.GUIBtnCount = in->ReadInt32();
    if (!AssertGameContent(numguibuts, r_data.SaveInfo.GUIBtnCount, "GUI Buttons"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.GUIBtnCount; ++i)
        guibuts[i].ReadFromSavegame(in);

    if (!AssertFormat(FormatConsistencyCheck, in->ReadInt32(), "GUI Labels"))
        return kSvgErr_InconsistentFormat;

    r_data.SaveInfo.GUILblCount = in->ReadInt32();
    if (!AssertGameContent(numguilabels, r_data.SaveInfo.GUILblCount, "GUI Labels"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.GUILblCount; ++i)
        guilabels[i].ReadFromSavegame(in);

    if (!AssertFormat(FormatConsistencyCheck, in->ReadInt32(), "GUI InvWindows"))
        return kSvgErr_InconsistentFormat;

    r_data.SaveInfo.GUIInvCount = in->ReadInt32();
    if (!AssertGameContent(numguiinv, r_data.SaveInfo.GUIInvCount, "GUI InvWindows"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.GUIInvCount; ++i)
        guiinv[i].ReadFromSavegame(in);

    if (!AssertFormat(FormatConsistencyCheck, in->ReadInt32(), "GUI Sliders"))
        return kSvgErr_InconsistentFormat;

    r_data.SaveInfo.GUISldCount = in->ReadInt32();
    if (!AssertGameContent(numguislider, r_data.SaveInfo.GUISldCount, "GUI Sliders"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.GUISldCount; ++i)
        guislider[i].ReadFromSavegame(in);

    if (!AssertFormat(FormatConsistencyCheck, in->ReadInt32(), "GUI TextBoxes"))
        return kSvgErr_InconsistentFormat;

    r_data.SaveInfo.GUITbxCount = in->ReadInt32();
    if (!AssertGameContent(numguitext, r_data.SaveInfo.GUITbxCount, "GUI TextBoxes"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.GUITbxCount; ++i)
        guitext[i].ReadFromSavegame(in);

    if (!AssertFormat(FormatConsistencyCheck, in->ReadInt32(), "GUI ListBoxes"))
        return kSvgErr_InconsistentFormat;

    r_data.SaveInfo.GUILbxCount = in->ReadInt32();
    if (!AssertGameContent(numguilist, r_data.SaveInfo.GUILbxCount, "GUI ListBoxes"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.GUILbxCount; ++i)
        guilist[i].ReadFromSavegame(in);

    if (!AssertFormat(FormatConsistencyCheck, in->ReadInt32(), "Animated Buttons"))
        return kSvgErr_InconsistentFormat;

    // Animated buttons
    int anim_count = in->ReadInt32();
    if (anim_count > MAX_ANIMATING_BUTTONS)
    {
        Out::FPrint("Restore game error: incompatible number of animated buttons (count: %d, max: %d)",
            anim_count, MAX_ANIMATING_BUTTONS);
        return kSvgErr_IncompatibleEngine;
    }
    numAnimButs = anim_count;
    for (int i = 0; i < numAnimButs; ++i)
        animbuts[i].ReadFromFile(in);
    return kSvgErr_NoError;
}

SavegameError WriteInventory(Stream *out)
{
    out->WriteInt32(game.numinvitems);
    for (int i = 0; i < game.numinvitems; ++i)
    {
        game.invinfo[i].WriteToSavegame(out);
        Properties::WriteValues(play.invProps[i], out);
        if (loaded_game_file_version <= kGameVersion_272)
            game.intrInv[i]->WriteTimesRunToSavedgame(out);
    }
    return kSvgErr_NoError;
}

SavegameError ReadInventory(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    r_data.SaveInfo.InvItemCount = in->ReadInt32();
    if (!AssertGameContent(game.numinvitems, r_data.SaveInfo.InvItemCount, "Inventory Items"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.InvItemCount; ++i)
    {
        game.invinfo[i].ReadFromSavegame(in);
        Properties::ReadValues(play.invProps[i], in);
        if (loaded_game_file_version <= kGameVersion_272)
            game.intrInv[i]->ReadTimesRunFromSavedgame(in);
    }
    return kSvgErr_NoError;
}

SavegameError WriteMouseCursors(Stream *out)
{
    out->WriteInt32(game.numcursors);
    for (int i = 0; i < game.numcursors; ++i)
    {
        game.mcurs[i].WriteToSavegame(out);
    }
    return kSvgErr_NoError;
}

SavegameError ReadMouseCursors(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    r_data.SaveInfo.MouseCurCount = in->ReadInt32();
    if (!AssertGameContent(game.numcursors, r_data.SaveInfo.MouseCurCount, "Mouse Cursors"))
        return kSvgErr_GameContentAssertion;
    for (int i = 0; i < r_data.SaveInfo.MouseCurCount; ++i)
    {
        game.mcurs[i].ReadFromSavegame(in);
    }
    return kSvgErr_NoError;
}

SavegameError WriteViews(Stream *out)
{
    out->WriteInt32(game.numviews);
    for (int view = 0; view < game.numviews; ++view)
    {
        out->WriteInt32(views[view].numLoops);
        for (int loop = 0; loop < views[view].numLoops; ++loop)
        {
            out->WriteInt32(views[view].loops[loop].numFrames);
            for (int frame = 0; frame < views[view].loops[loop].numFrames; ++frame)
            {
                out->WriteInt32(views[view].loops[loop].frames[frame].sound);
                out->WriteInt32(views[view].loops[loop].frames[frame].pic);
            }
        }
    }
    return kSvgErr_NoError;
}

SavegameError ReadViews(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    int view_count = in->ReadInt32();
    if (!AssertGameContent(game.numviews, view_count, "Views"))
        return kSvgErr_GameContentAssertion;
    r_data.SaveInfo.Views.resize(view_count);
    for (int view = 0; view < view_count; ++view)
    {
        int loop_count = in->ReadInt32();
        if (!AssertGameObjectContent(views[view].numLoops, loop_count, "Loops", "View", view))
            return kSvgErr_GameContentAssertion;
        r_data.SaveInfo.Views[view].resize(loop_count);
        for (int loop = 0; loop < loop_count; ++loop)
        {
            int frame_count = in->ReadInt32();
            if (!AssertGameObjectContent2(views[view].loops[loop].numFrames, frame_count,
                "Frame", "View", view, "Loop", loop))
                return kSvgErr_GameContentAssertion;
            r_data.SaveInfo.Views[view][loop] = frame_count;
            for (int frame = 0; frame < frame_count; ++frame)
            {
                views[view].loops[loop].frames[frame].sound = in->ReadInt32();
                views[view].loops[loop].frames[frame].pic = in->ReadInt32();
            }
        }
    }
    return kSvgErr_NoError;
}

SavegameError WriteDynamicSprites(Stream *out)
{
    const size_t ref_pos = out->GetPosition();
    out->WriteInt32(0); // number of dynamic sprites
    out->WriteInt32(0); // top index
    int count = 0;
    int top_index = 0;
    for (int i = 1; i < spriteset.elements; ++i)
    {
        if (game.spriteflags[i] & SPF_DYNAMICALLOC)
        {
            count++;
            top_index = i;
            out->WriteInt32(i);
            out->WriteInt32(game.spriteflags[i]);
            serialize_bitmap(spriteset[i], out);
        }
    }
    const size_t end_pos = out->GetPosition();
    out->Seek(ref_pos, kSeekBegin);
    out->WriteInt32(count);
    out->WriteInt32(top_index);
    out->Seek(end_pos, kSeekBegin);
    return kSvgErr_NoError;
}

SavegameError ReadDynamicSprites(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    const int spr_count = in->ReadInt32();
    // ensure the sprite set is at least large enough
    // to accomodate top dynamic sprite index
    const int top_index = in->ReadInt32();
    if (top_index >= MAX_SPRITES)
    {
        Out::FPrint("Restore game error: incompatible sprite top index (id: %d, max: %d)", top_index, MAX_SPRITES - 1);
        return kSvgErr_IncompatibleEngine;
    }
    spriteset.enlargeTo(top_index);
    for (int i = 0; i < spr_count; ++i)
    {
        int id = in->ReadInt32();
        if (id < 1 || id >= MAX_SPRITES)
        {
            Out::FPrint("Restore game error: incompatible sprite index (id: %d, range: %d - %d)",
                id, 1, MAX_SPRITES - 1);
            return kSvgErr_IncompatibleEngine;
        }
        int flags = in->ReadInt32();
        add_dynamic_sprite(id, read_serialized_bitmap(in));
        game.spriteflags[id] = flags;
    }
    return kSvgErr_NoError;
}

SavegameError WriteOverlays(Stream *out)
{
    out->WriteInt32(numscreenover);
    for (int i = 0; i < numscreenover; ++i)
    {
        screenover[i].WriteToFile(out);
        serialize_bitmap(screenover[i].pic, out);
    }
    return kSvgErr_NoError;
}

SavegameError ReadOverlays(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    int over_count = in->ReadInt32();
    if (over_count > MAX_SCREEN_OVERLAYS)
    {
        Out::FPrint("Restore game error: incompatible number of overlays (count: %d, max: %d)",
            over_count, MAX_SCREEN_OVERLAYS);
        return kSvgErr_IncompatibleEngine;
    }
    numscreenover = over_count;
    for (int i = 0; i < numscreenover; ++i)
    {
        screenover[i].ReadFromFile(in);
        if (screenover[i].hasSerializedBitmap)
            screenover[i].pic = read_serialized_bitmap(in);
    }
    return kSvgErr_NoError;
}

SavegameError WriteDynamicSurfaces(Stream *out)
{
    out->WriteInt32(MAX_DYNAMIC_SURFACES);
    for (int i = 0; i < MAX_DYNAMIC_SURFACES; ++i)
    {
        if (dynamicallyCreatedSurfaces[i] == NULL)
        {
            out->WriteInt8(0);
        }
        else
        {
            out->WriteInt8(1);
            serialize_bitmap(dynamicallyCreatedSurfaces[i], out);
        }
    }
    return kSvgErr_NoError;
}

SavegameError ReadDynamicSurfaces(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    if (!AssertGameContent(MAX_DYNAMIC_SURFACES, in->ReadInt32(), "Dynamic Surfaces"))
        return kSvgErr_GameContentAssertion;
    // load into a temp array since ccUnserialiseObjects will destroy
    // it otherwise
    r_data.DynamicSurfaces.resize(MAX_DYNAMIC_SURFACES);
    for (int i = 0; i < MAX_DYNAMIC_SURFACES; ++i)
    {
        if (in->ReadInt8() == 0)
            r_data.DynamicSurfaces[i] = NULL;
        else
            r_data.DynamicSurfaces[i] = read_serialized_bitmap(in);
    }
    return kSvgErr_NoError;
}

SavegameError WriteScriptModules(Stream *out)
{
    // write the data segment of the global script
    int data_len = gameinst->globaldatasize;
    out->WriteInt32(data_len);
    if (data_len > 0)
        out->Write(gameinst->globaldata, data_len);
    // write the script modules data segments
    out->WriteInt32(numScriptModules);
    for (int i = 0; i < numScriptModules; ++i)
    {
        data_len = moduleInst[i]->globaldatasize;
        out->WriteInt32(data_len);
        if (data_len > 0)
            out->Write(moduleInst[i]->globaldata, data_len);
    }
    return kSvgErr_NoError;
}

SavegameError ReadScriptModules(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    // read the global script data segment
    int data_len = in->ReadInt32();
    if (!AssertGameContent(pp.GlScDataSize, data_len))
    {
        Out::FPrint("Restore game error: mismatching size of global script data (game: %d, save: %d)",
            pp.GlScDataSize, data_len);
        return kSvgErr_GameContentAssertion;
    }
    r_data.GlobalScript.Len = data_len;
    r_data.GlobalScript.Data.reset(new char[data_len]);
    in->Read(r_data.GlobalScript.Data.get(), data_len);

    int module_count = in->ReadInt32();
    if (!AssertGameContent(numScriptModules, module_count, "Script Modules"))
        return kSvgErr_GameContentAssertion;
    r_data.ScriptModules.resize(module_count);
    for (int i = 0; i < module_count; ++i)
    {
        data_len = in->ReadInt32();
        if (!AssertGameContent(pp.ScMdDataSize[i], data_len))
        {
            Out::FPrint("Restore game error: mismatching size of global script data (game: %d, save: %d)",
                pp.ScMdDataSize[i], data_len);
            return kSvgErr_GameContentAssertion;
        }
        r_data.ScriptModules[i].Len = data_len;
        r_data.ScriptModules[i].Data.reset(new char[data_len]);
        in->Read(r_data.ScriptModules[i].Data.get(), data_len);
    }
    return kSvgErr_NoError;
}

SavegameError WriteRoomStates(Stream *out)
{
    // write the room state for all the rooms the player has been in
    out->WriteInt32(MAX_ROOMS);
    for (int i = 0; i < MAX_ROOMS; ++i)
    {
        if (isRoomStatusValid(i))
        {
            RoomStatus *roomstat = getRoomStatus(i);
            if (roomstat->beenhere)
            {
                out->WriteInt32(i);
                roomstat->WriteToSavegame(out);
                out->WriteInt32(FormatConsistencyCheck);
            }
            else
                out->WriteInt32(-1);
        }
        else
            out->WriteInt32(-1);
    }
    return kSvgErr_NoError;
}

SavegameError ReadRoomStates(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    int roomstat_count = in->ReadInt32();
    for (; roomstat_count > 0; --roomstat_count)
    {
        int id = in->ReadInt32();
        if (id >= MAX_ROOMS)
        {
            Out::FPrint("Restore game error: incompatible saved room index (id: %d, range: %d - %d)",
                id, 0, MAX_ROOMS - 1);
            return kSvgErr_IncompatibleEngine;
        }
        else if (id >= 0)
        {
            RoomStatus *roomstat = getRoomStatus(id);
            roomstat->ReadFromSavegame(in);
            if (!AssertFormat(FormatConsistencyCheck, in->ReadInt32(), "Room States", id))
                return kSvgErr_InconsistentFormat;
        }
    }
    return kSvgErr_NoError;
}

SavegameError WriteThisRoom(Stream *out)
{
    out->WriteInt32(displayed_room);
    if (displayed_room < 0)
        return kSvgErr_NoError;

    // modified room backgrounds
    for (int i = 0; i < MAX_BSCENE; ++i)
    {
        out->WriteBool(play.raw_modified[i] != 0);
        if (play.raw_modified[i])
            serialize_bitmap(thisroom.ebscene[i], out);
    }
    out->WriteBool(raw_saved_screen != NULL);
    if (raw_saved_screen)
        serialize_bitmap(raw_saved_screen, out);

    // room region state
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        out->WriteInt32(thisroom.regionLightLevel[i]);
        out->WriteInt32(thisroom.regionTintLevel[i]);
    }
    for (int i = 0; i < MAX_WALK_AREAS + 1; ++i)
    {
        out->WriteInt32(thisroom.walk_area_zoom[i]);
        out->WriteInt32(thisroom.walk_area_zoom2[i]);
    }

    // room object movement paths cache
    out->WriteInt32(thisroom.numsprs + 1);
    for (int i = 0; i < thisroom.numsprs + 1; ++i)
    {
        mls[i].WriteToFile(out);
    }

    // room music volume
    out->WriteInt32(thisroom.options[ST_VOLUME]);

    // persistent room's indicator
    const bool persist = displayed_room < MAX_ROOMS;
    out->WriteBool(persist);
    // write the current troom state, in case they save in temporary room
    if (!persist)
        troom.WriteToSavegame(out);
    return kSvgErr_NoError;
}

SavegameError ReadThisRoom(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    displayed_room = in->ReadInt32();
    if (displayed_room < 0)
        return kSvgErr_NoError;

    // modified room backgrounds
    for (int i = 0; i < MAX_BSCENE; ++i)
    {
        play.raw_modified[i] = in->ReadBool();
        if (play.raw_modified[i])
            r_data.RoomBkgScene[i] = read_serialized_bitmap(in);
        else
            r_data.RoomBkgScene[i] = NULL;
    }
    if (in->ReadBool())
        raw_saved_screen = read_serialized_bitmap(in);

    // room region state
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        r_data.RoomLightLevels[i] = in->ReadInt32();
        r_data.RoomTintLevels[i] = in->ReadInt32();
    }
    for (int i = 0; i < MAX_WALK_AREAS + 1; ++i)
    {
        r_data.RoomZoomLevels1[i] = in->ReadInt32();
        r_data.RoomZoomLevels2[i] = in->ReadInt32();
    }

    // room object movement paths cache
    int objmls_count = in->ReadInt32();
    if (objmls_count > CHMLSOFFS)
    {
        Out::FPrint("Restore game error: incompatible number of room object move lists (count: %d, max: %d)",
            objmls_count, CHMLSOFFS);
        return kSvgErr_IncompatibleEngine;
    }
    for (int i = 0; i < objmls_count; ++i)
    {
        mls[i].ReadFromFile(in);
    }

    // save the new room music vol for later use
    r_data.RoomVolume = in->ReadInt32();

    // read the current troom state, in case they saved in temporary room
    if (!in->ReadBool())
        troom.ReadFromSavegame(in);

    return kSvgErr_NoError;
}

SavegameError WriteManagedPool(Stream *out)
{
    ccSerializeAllObjects(out);
    return kSvgErr_NoError;
}

SavegameError ReadManagedPool(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    if (ccUnserializeAllObjects(in, &ccUnserializer))
    {
        Out::FPrint("Restore game error: managed pool deserialization failed: %s", ccErrorString);
        return kSvgErr_GameObjectInitFailed;
    }
    return kSvgErr_NoError;
}

SavegameError WritePluginData(Stream *out)
{
    // [IKM] Plugins expect FILE pointer! // TODO something with this later...
    platform->RunPluginHooks(AGSE_SAVEGAME, (long)((Common::FileStream*)out)->GetHandle());
    return kSvgErr_NoError;
}

SavegameError ReadPluginData(Stream *in, int32_t blk_ver, const PreservedParams &pp, RestoredData &r_data)
{
    // [IKM] Plugins expect FILE pointer! // TODO something with this later
    platform->RunPluginHooks(AGSE_RESTOREGAME, (long)((Common::FileStream*)in)->GetHandle());
    return kSvgErr_NoError;
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
        WriteGameState,
        ReadGameState
    },
    {
        kSvgBlock_GameState_Audio, "Audio",
        0,
        WriteAudio,
        ReadAudio
    },
    {
        kSvgBlock_GameState_Characters, "Characters",
        0,
        WriteCharacters,
        ReadCharacters
    },
    {
        kSvgBlock_GameState_Dialogs, "Dialogs",
        0,
        WriteDialogs,
        ReadDialogs
    },
    {
        kSvgBlock_GameState_GUI, "GUI",
        0,
        WriteGUI,
        ReadGUI
    },
    {
        kSvgBlock_GameState_InventoryItems, "Inventory Items",
        0,
        WriteInventory,
        ReadInventory
    },
    {
        kSvgBlock_GameState_MouseCursors, "Mouse Cursors",
        0,
        WriteMouseCursors,
        ReadMouseCursors
    },
    {
        kSvgBlock_GameState_Views, "Views",
        0,
        WriteViews,
        ReadViews
    },
    {
        kSvgBlock_GameState_DynamicSprites, "Dynamic Sprites",
        0,
        WriteDynamicSprites,
        ReadDynamicSprites
    },
    {
        kSvgBlock_GameState_Overlays, "Overlays",
        0,
        WriteOverlays,
        ReadOverlays
    },
    {
        kSvgBlock_GameState_DynamicSurfaces, "Dynamic Surfaces",
        0,
        WriteDynamicSurfaces,
        ReadDynamicSurfaces
    },
    {
        kSvgBlock_GameState_ScriptModules, "Script Modules",
        0,
        WriteScriptModules,
        ReadScriptModules
    },
    {
        kSvgBlock_RoomStates_AllRooms, "Room States",
        0,
        WriteRoomStates,
        ReadRoomStates
    },
    {
        kSvgBlock_RoomStates_ThisRoom, "Running Room State",
        0,
        WriteThisRoom,
        ReadThisRoom
    },
    {
        kSvgBlock_ManagedPool, "Managed Pool",
        0,
        WriteManagedPool,
        ReadManagedPool
    },
    {
        kSvgBlock_PluginData, "Plugin Data",
        0,
        WritePluginData,
        ReadPluginData
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
