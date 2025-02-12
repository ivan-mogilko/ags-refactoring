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
#include <cstdio>
#include "ac/audiocliptype.h"
#include "ac/dialogtopic.h"
#include "ac/game_version.h"
#include "ac/gamedata.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "ac/wordsdictionary.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "core/asset.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "game/main_game_file.h"
#include "gui/guibutton.h"
#include "gui/guilabel.h"
#include "gui/guimain.h"
#include "script/cc_common.h"
#include "util/data_ext.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/string_utils.h"
#include "font/fonts.h"

namespace AGS
{
namespace Common
{

const String MainGameSource::DefaultFilename_v3 = "game28.dta";
const String MainGameSource::DefaultFilename_v2 = "ac2game.dta";
const String MainGameSource::Signature = "Adventure Creator Game File v2";

MainGameSource::MainGameSource()
    : DataVersion(kGameVersion_Undefined)
{
}

String GetMainGameFileErrorText(MainGameFileErrorType err)
{
    switch (err)
    {
    case kMGFErr_NoError:
        return "No error.";
    case kMGFErr_FileOpenFailed:
        return "Main game file not found or could not be opened.";
    case kMGFErr_SignatureFailed:
        return "Not an AGS main game file or unsupported format.";
    case kMGFErr_FormatVersionNotSupported:
        return "Format version not supported.";
    case kMGFErr_CapsNotSupported:
        return "The game requires extended capabilities which aren't supported by the engine.";
    case kMGFErr_InvalidNativeResolution:
        return "Unable to determine native game resolution.";
    case kMGFErr_TooManySprites:
        return "Too many sprites for this engine to handle.";
    case kMGFErr_InvalidPropertySchema:
        return "Failed to deserialize custom properties schema.";
    case kMGFErr_InvalidPropertyValues:
        return "Errors encountered when reading custom properties.";
    case kMGFErr_CreateGlobalScriptFailed:
        return "Failed to load global script.";
    case kMGFErr_CreateDialogScriptFailed:
        return "Failed to load dialog script.";
    case kMGFErr_CreateScriptModuleFailed:
        return "Failed to load script module.";
    case kMGFErr_GameEntityFailed:
        return "Failed to load one or more game entities.";
    case kMGFErr_PluginDataFmtNotSupported:
        return "Format version of plugin data is not supported.";
    case kMGFErr_PluginDataSizeTooLarge:
        return "Plugin data size is too large.";
    case kMGFErr_ExtListFailed:
        return "There was error reading game data extensions.";
    case kMGFErr_ExtUnknown:
        return "Unknown extension.";
    }
    return "Unknown error.";
}

bool IsMainGameLibrary(const String &filename)
{
    // We must not only detect if the given file is a correct AGS data library,
    // we also have to assure that this library contains main game asset.
    // Library may contain some optional data (digital audio, speech etc), but
    // that is not what we want.
    AssetLibInfo lib;
    if (AssetManager::ReadDataFileTOC(filename, lib) != kAssetNoError)
        return false;
    for (size_t i = 0; i < lib.AssetInfos.size(); ++i)
    {
        if (lib.AssetInfos[i].FileName.CompareNoCase(MainGameSource::DefaultFilename_v3) == 0 ||
            lib.AssetInfos[i].FileName.CompareNoCase(MainGameSource::DefaultFilename_v2) == 0)
        {
            return true;
        }
    }
    return false;
}

// Scans given directory for game data libraries, returns first found or none.
// Uses fn_testfile callback to test the file.
// Tracks files with standard AGS package names:
// - *.ags is a standart cross-platform file pattern for AGS games,
// - ac2game.dat is a legacy file name for very old games,
// - *.exe is a MS Win executable; it is included to this case because
//   users often run AGS ports with Windows versions of games.
String FindGameData(const String &path, std::function<bool(const String&)> fn_testfile)
{
    String test_file;
    Debug::Printf("Searching for game data in: %s", path.GetCStr());
    for (FindFile ff = FindFile::OpenFiles(path); !ff.AtEnd(); ff.Next())
    {
        test_file = ff.Current();
        if (test_file.CompareRightNoCase(".ags") == 0 ||
            test_file.CompareNoCase("ac2game.dat") == 0 ||
            test_file.CompareRightNoCase(".exe") == 0)
        {
            test_file = Path::ConcatPaths(path, test_file);
            if (fn_testfile(test_file))
            {
                Debug::Printf("Found game data pak: %s", test_file.GetCStr());
                return test_file;
            }
        }
    }
    return "";
}

String FindGameData(const String &path)
{
    return FindGameData(path, IsMainGameLibrary);
}

// Begins reading main game file from a generic stream
static HGameFileError OpenMainGameFileBase(MainGameSource &src)
{
    Stream *in = src.InputStream.get();
    // Check data signature
    String data_sig = String::FromStreamCount(in, MainGameSource::Signature.GetLength());
    if (data_sig.Compare(MainGameSource::Signature))
        return new MainGameFileError(kMGFErr_SignatureFailed);
    // Read data format version and requested engine version
    src.DataVersion = (GameDataVersion)in->ReadInt32();
    src.CompiledWith = StrUtil::ReadString(in);
    if (src.DataVersion < kGameVersion_LowSupported || src.DataVersion > kGameVersion_Current)
        return new MainGameFileError(kMGFErr_FormatVersionNotSupported,
            String::FromFormat("Game was compiled with %s. Required format version: %d, supported %d - %d", src.CompiledWith.GetCStr(), src.DataVersion, kGameVersion_LowSupported, kGameVersion_Current));
    // Read required capabilities
    size_t count = in->ReadInt32();
    for (size_t i = 0; i < count; ++i)
        src.Caps.insert(StrUtil::ReadString(in));
    // Remember loaded game data version
    // NOTE: this global variable is embedded in the code too much to get
    // rid of it too easily; the easy way is to set it whenever the main
    // game file is opened.
    loaded_game_file_version = src.DataVersion;
    game_compiled_version.SetFromString(src.CompiledWith);
    return HGameFileError::None();
}

HGameFileError OpenMainGameFile(const String &filename, MainGameSource &src)
{
    // Cleanup source struct
    src = MainGameSource();
    // Try to open given file
    auto in = File::OpenFileRead(filename);
    if (!in)
        return new MainGameFileError(kMGFErr_FileOpenFailed, String::FromFormat("Tried filename: %s.", filename.GetCStr()));
    src.Filename = filename;
    src.InputStream = std::move(in);
    return OpenMainGameFileBase(src);
}

HGameFileError OpenMainGameFileFromDefaultAsset(MainGameSource &src, AssetManager *mgr)
{
    // Cleanup source struct
    src = MainGameSource();
    // Try to find and open main game file
    String filename = MainGameSource::DefaultFilename_v3;
    auto in = mgr->OpenAsset(filename);
    if (!in)
    {
        filename = MainGameSource::DefaultFilename_v2;
        in = mgr->OpenAsset(filename);
    }
    if (!in)
        return new MainGameFileError(kMGFErr_FileOpenFailed, String::FromFormat("Tried filenames: %s, %s.",
            MainGameSource::DefaultFilename_v3.GetCStr(), MainGameSource::DefaultFilename_v2.GetCStr()));
    src.Filename = filename;
    src.InputStream = std::move(in);
    return OpenMainGameFileBase(src);
}

// Lookup table for scaling 5 bit colors up to 8 bits,
// copied from Allegro 4 library, preventing an extra dependency.
static const uint8_t RGBScale5[32]
{
    0,   8,   16,  24,  33,  41,  49,  57,
    66,  74,  82,  90,  99,  107, 115, 123,
    132, 140, 148, 156, 165, 173, 181, 189,
    198, 206, 214, 222, 231, 239, 247, 255
};

// Lookup table for scaling 6 bit colors up to 8 bits,
// copied from Allegro 4 library, preventing an extra dependency.
static const uint8_t RGBScale6[64]
{
    0,   4,   8,   12,  16,  20,  24,  28,
    32,  36,  40,  44,  48,  52,  56,  60,
    65,  69,  73,  77,  81,  85,  89,  93,
    97,  101, 105, 109, 113, 117, 121, 125,
    130, 134, 138, 142, 146, 150, 154, 158,
    162, 166, 170, 174, 178, 182, 186, 190,
    195, 199, 203, 207, 211, 215, 219, 223,
    227, 231, 235, 239, 243, 247, 251, 255
};

// Remaps color number from legacy to new format:
// * palette index in 8-bit game,
// * encoded 32-bit A8R8G8B8 in 32-bit game.
static int RemapFromLegacyColourNumber(const GameBasicProperties &game, int color, bool is_bg = false)
{
    if (game.color_depth == 1)
        return color; // keep palette index

    // Special color number 0 is treated depending on its purpose:
    // * background color becomes fully transparent;
    // * foreground color becomes opaque black
    if (color == 0)
    {
        return is_bg ? 0 : (0 | (0xFF << 24));
    }

    // Special color numbers 1-31 were always interpreted as palette indexes;
    // for them we compose a 32-bit ARGB from the palette entry
    if (color >= 0 && color < 32)
    {
        const RGB &rgb = game.defpal[color];
        return rgb.b | (rgb.g << 8) | (rgb.r << 16) | (0xFF << 24);
    }

    // The rest is a R5G6B5 color; we convert it to a proper 32-bit ARGB;
    // color is always opaque when ported from legacy projects
    uint8_t red = RGBScale5[(color >> 11) & 0x1f];
    uint8_t green = RGBScale6[(color >> 5) & 0x3f];
    uint8_t blue = RGBScale5[(color) & 0x1f];
    return blue | (green << 8) | (red << 16) | (0xFF << 24);
}

void UpgradeGame(GameBasicProperties &game, GameDataVersion data_ver)
{
    if (data_ver < kGameVersion_362)
    {
        game.options[OPT_SAVESCREENSHOTLAYER] = UINT32_MAX; // all possible layers
    }
    // 32-bit color properties
    if (data_ver < kGameVersion_400_09)
    {
        game.hotdot = RemapFromLegacyColourNumber(game, game.hotdot);
        game.hotdotouter = RemapFromLegacyColourNumber(game, game.hotdotouter);
    }
}

void UpgradeFonts(LoadedGame &game, GameDataVersion data_ver)
{
    if (data_ver < kGameVersion_400_10)
    {
        for (size_t i = 0; i < game.fonts.size(); ++i)
        {
            // TODO: find a better way than relying on valid size?
            auto &fi = game.fonts[i];
            if (fi.Size > 0)
                fi.Filename.Format("agsfnt%d.ttf", i);
            else
                fi.Filename.Format("agsfnt%d.wfn", i);
        }
    }
}

// Convert audio data to the current version
void UpgradeAudio(LoadedGame &game, GameDataVersion data_ver)
{
}

// Convert character data to the current version
void UpgradeCharacters(LoadedGame &game, GameDataVersion data_ver)
{
    const int char_count = game.numcharacters;
    auto &chars = game.chars;
    // < 3.6.2 characters always followed OPT_CHARTURNWHENFACE,
    // so they have to have TURNWHENFACE enabled
    if (data_ver < kGameVersion_362)
    {
        for (int i = 0; i < char_count; i++)
        {
            chars[i].flags |= CHF_TURNWHENFACE;
        }
    }

    // 32-bit color properties
    if (data_ver < kGameVersion_400_09)
    {
        for (int i = 0; i < char_count; i++)
        {
            chars[i].talkcolor = RemapFromLegacyColourNumber(game, chars[i].talkcolor);
        }
    }
}

void UpgradeGUI(LoadedGame &game, GameDataVersion data_ver)
{
    // Previously, Buttons and Labels had a fixed Translated behavior
    if (data_ver < kGameVersion_361)
    {
        for (auto &btn : game.GuiControls.Buttons)
            btn.SetTranslated(true); // always translated
        for (auto &lbl : game.GuiControls.Labels)
            lbl.SetTranslated(true); // always translated
    }

    // 32-bit color properties
    if (data_ver < kGameVersion_400_09)
    {
        for (auto &gui : game.Guis)
        {
            gui.BgColor = RemapFromLegacyColourNumber(game, gui.BgColor, true);
            gui.FgColor = RemapFromLegacyColourNumber(game, gui.FgColor, !gui.IsTextWindow()
                /* right, treat border as background for normal gui */);
        }

        for (auto &btn : game.GuiControls.Buttons)
        {
            btn.TextColor = RemapFromLegacyColourNumber(game, btn.TextColor);
        }

        for (auto &lbl : game.GuiControls.Labels)
        {
            lbl.TextColor = RemapFromLegacyColourNumber(game, lbl.TextColor);
        }

        for (auto &list : game.GuiControls.ListBoxes)
        {
            list.TextColor = RemapFromLegacyColourNumber(game, list.TextColor);
            list.SelectedBgColor = RemapFromLegacyColourNumber(game, list.SelectedBgColor, true);
            list.SelectedTextColor = RemapFromLegacyColourNumber(game, list.SelectedTextColor);
        }

        for (auto &tbox : game.GuiControls.TextBoxes)
        {
            tbox.TextColor = RemapFromLegacyColourNumber(game, tbox.TextColor);
        }
    }
}

void UpgradeMouseCursors(LoadedGame &game, GameDataVersion data_ver)
{
}

void FixupSaveDirectory(GameBasicProperties &game)
{
    // If the save game folder was not specified by game author, create one of
    // the game name, game GUID, or uniqueid, as a last resort
    if (game.saveGameFolderName.IsEmpty())
    {
        if (!game.gamename.IsEmpty())
            game.saveGameFolderName = game.gamename;
        else if (game.guid[0])
            game.saveGameFolderName = game.guid;
        else
            game.saveGameFolderName.Format("AGS-Game-%d", game.uniqueid);
    }
    // Lastly, fixup folder name by removing any illegal characters
    game.saveGameFolderName = Path::FixupSharedFilename(game.saveGameFolderName);
}

// GameDataExtReader reads main game data's extension blocks
class GameDataExtReader : public DataExtReader
{
public:
    GameDataExtReader(LoadedGame &ents, GameDataVersion data_ver, std::unique_ptr<Stream> &&in)
        : DataExtReader(std::move(in), kDataExt_NumID8 | kDataExt_File64)
        , _ents(ents)
        , _dataVer(data_ver)
    {}

protected:
    HError ReadBlock(Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override;
    HError ReadCustomProperties(Stream *in, const char *obj_type, size_t expect_obj_count, std::vector<StringIMap> &obj_values);

    LoadedGame &_ents;
    GameDataVersion _dataVer {};
};

static HError ReadInteractionScriptModules(Stream *in, LoadedGame &ents)
{
    // Updated InteractionEvents format, which specifies script module
    // for object interaction events
    size_t num_chars = in->ReadInt32();
    if (num_chars != ents.chars.size())
        return new Error(String::FromFormat("Mismatching number of characters: read %zu expected %zu", num_chars, ents.chars.size()));
    for (size_t i = 0; i < num_chars; ++i)
        ents.charScripts[i] = InteractionEvents::CreateFromStream_v362(in);
    uint32_t num_invitems = in->ReadInt32();
    if (num_invitems != ents.numinvitems)
        return new Error(String::FromFormat("Mismatching number of inventory items: read %zu expected %zu", num_invitems, (size_t)ents.numinvitems));
    for (uint32_t i = 0; i < (uint32_t)ents.numinvitems; ++i)
        ents.invScripts[i] = InteractionEvents::CreateFromStream_v362(in);

    // Script module specification for GUI events
    uint32_t num_gui = in->ReadInt32();
    if (num_gui != ents.numgui)
        return new Error(String::FromFormat("Mismatching number of GUI: read %zu expected %zu", num_gui, (size_t)ents.numgui));
    for (size_t i = 0; i < (size_t)ents.numgui; ++i)
        ents.Guis[i].ScriptModule = StrUtil::ReadString(in);
    return HError::None();
}

HError GameDataExtReader::ReadCustomProperties(Stream *in, const char *obj_type, size_t expect_obj_count, std::vector<StringIMap> &obj_values)
{
    size_t obj_count = in->ReadInt32();
    if (obj_count != expect_obj_count)
        return new Error(String::FromFormat("Mismatching number of %s: read %zu expected %zu", obj_type, obj_count, expect_obj_count));
    obj_values.resize(obj_count);
    int errors = 0;
    for (size_t i = 0; i < obj_count; ++i)
    {
        errors += Properties::ReadValues(obj_values[i], in);
    }
    if (errors > 0)
        return new MainGameFileError(kMGFErr_InvalidPropertyValues);
    return HError::None();
}

HError GameDataExtReader::ReadBlock(Stream *in, int /*block_id*/, const String &ext_id,
    soff_t /*block_len*/, bool &read_next)
{
    read_next = true;
    auto &game = _ents;
    // Add extensions here checking ext_id, which is an up to 16-chars name, for example:
    // if (ext_id.CompareNoCase("GUI_NEWPROPS") == 0)
    // {
    //     // read new gui properties
    // }
    if (ext_id.CompareNoCase("v360_fonts") == 0)
    {
        for (FontInfo &finfo : game.fonts)
        {
            // adjustable font outlines
            finfo.AutoOutlineThickness = in->ReadInt32();
            finfo.AutoOutlineStyle =
                static_cast<enum FontInfo::AutoOutlineStyle>(in->ReadInt32());
            // reserved
            in->ReadInt32();
            in->ReadInt32();
            in->ReadInt32();
            in->ReadInt32();
        }
    }
    else if (ext_id.CompareNoCase("v360_cursors") == 0)
    {
        for (MouseCursor &mcur : game.mcurs)
        {
            mcur.animdelay = in->ReadInt32();
            // reserved
            in->ReadInt32();
            in->ReadInt32();
            in->ReadInt32();
        }
    }
    else if (ext_id.CompareNoCase("v361_objnames") == 0)
    {
        // Extended object names and script names:
        // for object types that had hard name length limits
        game.gamename = StrUtil::ReadString(in);
        game.saveGameFolderName = StrUtil::ReadString(in);
        size_t num_chars = in->ReadInt32();
        if (num_chars != game.chars.size())
            return new Error(String::FromFormat("Mismatching number of characters: read %zu expected %zu", num_chars, game.chars.size()));
        for (int i = 0; i < game.numcharacters; ++i)
        {
            auto &chinfo = game.chars[i];
            chinfo.scrname = StrUtil::ReadString(in);
            chinfo.name = StrUtil::ReadString(in);
        }
        size_t num_invitems = in->ReadInt32();
        if (num_invitems != game.numinvitems)
            return new Error(String::FromFormat("Mismatching number of inventory items: read %zu expected %zu", num_invitems, (size_t)game.numinvitems));
        for (int i = 0; i < game.numinvitems; ++i)
        {
            game.invinfo[i].name = StrUtil::ReadString(in);
        }
        size_t num_cursors = in->ReadInt32();
        if (num_cursors != game.mcurs.size())
            return new Error(String::FromFormat("Mismatching number of cursors: read %zu expected %zu", num_cursors, game.mcurs.size()));
        for (MouseCursor &mcur : game.mcurs)
        {
            mcur.name = StrUtil::ReadString(in);
        }
        size_t num_clips = in->ReadInt32();
        if (num_clips != game.audioClips.size())
            return new Error(String::FromFormat("Mismatching number of audio clips: read %zu expected %zu", num_clips, game.audioClips.size()));
        for (ScriptAudioClip &clip : game.audioClips)
        {
            clip.scriptName = StrUtil::ReadString(in);
            clip.fileName = StrUtil::ReadString(in);
        }
    }
    else if (ext_id.CompareNoCase("v362_interevents") == 0)
    {
        HError err = ReadInteractionScriptModules(in, _ents);
        if (!err)
            return err;
    }
    else if (ext_id.CompareNoCase("v362_interevent2") == 0)
    {
        // Explicit script module names
        // NOTE: that scripts may not be initialized at this time in case they are stored as separate
        // assets within the game package; we still read the names though to keep data format simpler
        String script_name = StrUtil::ReadString(in);
        if (game.GlobalScript)
            game.GlobalScript->SetScriptName(script_name.ToStdString());
        script_name = StrUtil::ReadString(in);
        if (game.DialogScript)
            game.DialogScript->SetScriptName(script_name.ToStdString());
        size_t module_count = in->ReadInt32();
        if (module_count != game.ScriptModules.size())
            return new Error(String::FromFormat("Mismatching number of script modules: read %zu expected %zu", module_count, game.ScriptModules.size()));
        for (size_t i = 0; i < module_count; ++i)
        {
            script_name = StrUtil::ReadString(in);
            if (game.ScriptModules[i])
                game.ScriptModules[i]->SetScriptName(script_name.ToStdString());
        }

        HError err = ReadInteractionScriptModules(in, _ents);
        if (!err)
            return err;
    }
    else if (ext_id.CompareNoCase("v362_guictrls") == 0)
    {
        size_t num_guibut = in->ReadInt32();
        if (num_guibut != game.GuiControls.Buttons.size())
            return new Error(String::FromFormat("Mismatching number of GUI buttons: read %zu expected %zu", num_guibut, game.GuiControls.Buttons.size()));
        for (GUIButton &but : game.GuiControls.Buttons)
        {
            // button padding
            but.TextPaddingHor = in->ReadInt32();
            but.TextPaddingVer = in->ReadInt32();
            in->ReadInt32(); // reserve 2 ints
            in->ReadInt32();
        }
    }
    // Early development version of "ags4"
    else if (ext_id.CompareNoCase("ext_ags399") == 0)
    {
        // new character properties
        for (size_t i = 0; i < (size_t)game.numcharacters; ++i)
        {
            game.CharEx[i].BlendMode = (BlendMode)_in->ReadInt32();
            // Reserved for colour options
            _in->Seek(sizeof(int32_t) * 3); // flags + tint rgbs + light level
            // Reserved for transform options (see brief list in savegame format)
            _in->Seek(sizeof(int32_t) * 11);
        }

        // new gui properties
        for (size_t i = 0; i < game.Guis.size(); ++i)
        {
            game.Guis[i].BlendMode = (BlendMode)_in->ReadInt32();
            // Reserved for colour options
            _in->Seek(sizeof(int32_t) * 3); // flags + tint rgbs + light level
            // Reserved for transform options (see list in savegame format)
            _in->Seek(sizeof(int32_t) * 11);
        }
    }
    else if (ext_id.CompareNoCase("v400_gameopts") == 0)
    {
        game.faceDirectionRatio = _in->ReadFloat32();
        // reserve few more 32-bit values (for a total of 10)
        _in->Seek(sizeof(int32_t) * 9);
    }
    else if (ext_id.CompareNoCase("v400_customprops") == 0)
    {
        game.audioclipProps.resize(game.audioClips.size());
        game.dialogProps.resize(game.numdialog);
        game.guiProps.resize(game.numgui);

        HError err = ReadCustomProperties(in, "audio clips", game.audioClips.size(), game.audioclipProps);
        if (!err)
            return err;
        err = ReadCustomProperties(in, "dialogs", game.numdialog, game.dialogProps);
        if (!err)
            return err;
        err = ReadCustomProperties(in, "guis", game.numgui, game.guiProps);
        if (!err)
            return err;

        const char *guictrl_names[kGUIControlTypeNum] = { "", "gui buttons", "gui labels", "inventory windows", "sliders", "text boxes", "list boxes" };
        size_t guictrl_counts[kGUIControlTypeNum] = { 0, game.GuiControls.Buttons.size(), game.GuiControls.Labels.size(),
            game.GuiControls.InvWindows.size(), game.GuiControls.Sliders.size(), game.GuiControls.TextBoxes.size(), game.GuiControls.ListBoxes.size() };
        for (int i = kGUIButton; i < kGUIControlTypeNum; ++i)
        {
            err = ReadCustomProperties(in, guictrl_names[i], guictrl_counts[i], game.guicontrolProps[i]);
            if (!err)
                return err;
        }
    }
    else if (ext_id.CompareNoCase("v400_fontfiles") == 0)
    {
        size_t font_count = in->ReadInt32();
        if (font_count != game.numfonts)
            return new Error(String::FromFormat("Mismatching number of fonts: read %zu expected %zu", font_count, (size_t)game.numfonts));
        for (FontInfo &finfo : game.fonts)
        {
            finfo.Filename = StrUtil::ReadString(in);
        }
    }
    else
    {
        return new MainGameFileError(kMGFErr_ExtUnknown, String::FromFormat("Type: %s", ext_id.GetCStr()));
    }
    return HError::None();
}


// Search and read only data belonging to the general game info
class GameDataExtPreloader : public DataExtReader
{
public:
    GameDataExtPreloader(GameBasicProperties &game, GameDataVersion data_ver, std::unique_ptr<Stream> &&in)
        : DataExtReader(std::move(in), kDataExt_NumID8 | kDataExt_File64)
        , _game(game)
        , _dataVer(data_ver)
    {}

protected:
    HError ReadBlock(Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override;

    GameBasicProperties &_game;
    GameDataVersion _dataVer{};
};

HError GameDataExtPreloader::ReadBlock(Stream *in, int /*block_id*/, const String &ext_id,
    soff_t /*block_len*/, bool &read_next)
{
    // Try reading only data which belongs to the general game info
    read_next = true;
    auto &game = _game;
    if (ext_id.CompareNoCase("v361_objnames") == 0)
    {
        game.gamename = StrUtil::ReadString(in);
        game.saveGameFolderName = StrUtil::ReadString(in);
        read_next = false; // we're done
    }
    SkipBlock(); // prevent assertion trigger
    return HError::None();
}


HGameFileError ReadGameData(LoadedGame &ents, std::unique_ptr<Stream> &&s_in, GameDataVersion data_ver)
{
    Stream *in = s_in.get(); // for convenience

    //-------------------------------------------------------------------------
    // The standard data section.
    //-------------------------------------------------------------------------
    HGameFileError err = ents.ReadFromFile(in, data_ver);
    // Print game title information always
    Debug::Printf(kDbgMsg_Info, "Game title: '%s'", ents.gamename.GetCStr());
    Debug::Printf(kDbgMsg_Info, "Game uid (old format): `%d`", ents.uniqueid);
    Debug::Printf(kDbgMsg_Info, "Game guid: '%s'", ents.guid);
    if (!err)
        return err;

    //-------------------------------------------------------------------------
    // All the extended data, for AGS > 3.5.0.
    //-------------------------------------------------------------------------
    GameDataExtReader reader(ents, data_ver, std::move(s_in));
    HError ext_err = reader.Read();
    return ext_err ? HGameFileError::None() : new MainGameFileError(kMGFErr_ExtListFailed, ext_err);
}

HGameFileError UpdateGameData(LoadedGame &game, GameDataVersion data_ver)
{
    UpgradeGame(game, data_ver);
    UpgradeFonts(game, data_ver);
    UpgradeAudio(game, data_ver);
    UpgradeCharacters(game, data_ver);
    UpgradeGUI(game, data_ver);
    UpgradeMouseCursors(game, data_ver);
    FixupSaveDirectory(game);
    return HGameFileError::None();
}

void PreReadGameData(GameBasicProperties &game, std::unique_ptr<Stream> &&s_in, GameDataVersion data_ver)
{
    Stream *in = s_in.get(); // for convenience
    GameBasicProperties::SerializeInfo sinfo;
    game.ReadFromFile(in, data_ver, sinfo);

    // Check for particular expansions that might have data necessary
    // for "preload" purposes
    if (sinfo.ExtensionOffset == 0u)
        return; // either no extensions, or data version is too early

    in->Seek(sinfo.ExtensionOffset, kSeekBegin);
    GameDataExtPreloader reader(game, data_ver, std::move(s_in));
    reader.Read();
}

} // namespace Common
} // namespace AGS
