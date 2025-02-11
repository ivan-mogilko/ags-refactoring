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
//
// GameSetupStruct is a contemporary main game data. 
//
//=============================================================================
#ifndef __AGS_CN_AC__GAMESETUPSTRUCT_H
#define __AGS_CN_AC__GAMESETUPSTRUCT_H

#include <array>
#include <map>
#include <vector>
#include <allegro.h>
#include "ac/audiocliptype.h"
#include "ac/characterinfo.h" // TODO: constants to separate header
#include "ac/gamestructdefines.h"
#include "ac/inventoryiteminfo.h"
#include "ac/mousecursor.h"
#include "ac/wordsdictionary.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "game/customproperties.h"
#include "game/interactions.h"
#include "game/main_game_file.h" // TODO: constants to separate header or split out reading functions
#include "game/plugininfo.h"
#include "gfx/gfx_def.h"
#include "gui/guidefines.h"
#include "gui/guimain.h"


// GameBasicProperties contains most basic game settings
struct GameBasicProperties
{
    static const int  LEGACY_GAME_NAME_LENGTH = 50;
    static const int  MAX_OPTIONS = 100;
    static const int  NUM_INTS_RESERVED = 14;
    // TODO: this is left purely to load older format version, revise later
    static const int  NUM_LEGACY_GLOBALMES = 500;

    Common::String gamename;
    int     options[MAX_OPTIONS] = { 0 };
    uint8_t paluses[256] = { 0 };
    RGB     defpal[256] = {};
    int     numviews = 0;
    int     numcharacters = 0;
    int     playercharacter = -1;
    int     numinvitems = 0;
    int     numdialog = 0;
    int     numdlgmessage = 0;    // [DEPRECATED]
    int     numfonts = 0;
    int     color_depth = 0;      // in bytes per pixel (ie. 1, 2, 4)
    int     target_win = 0;
    int     dialog_bullet = 0;    // 0 for none, otherwise slot num of bullet point
    int     hotdot = 0;           // inv cursor hotspot dot color
    int     hotdotouter = 0;      // inv cursor hotspot cross color
    int     uniqueid = 0;         // random key identifying the game
    int     numgui = 0;
    int     numcursors = 0;
    Size    game_resolution;
    int     default_lipsync_frame = 0; // used for unknown chars
    int     invhotdotsprite = 0;
    int     reserved[NUM_INTS_RESERVED] = { 0 };
    char    guid[MAX_GUID_LENGTH] = { 0 };
    char    saveGameFileExtension[MAX_SG_EXT_LENGTH] = { 0 };
    // NOTE: saveGameFolderName is generally used to create game subdirs in common user directories
    Common::String saveGameFolderName;

    // Game resolution is a size of a native game screen in pixels.
    // This is the "game resolution" that developer sets up in AGS Editor.
    // It is in the same units in which sprite and font sizes are defined.
    //
    // Graphic renderer may scale and stretch game's frame as requested by
    // player or system, which will not affect native coordinates in any way.
    const Size &GetGameRes() const { return game_resolution; }
    // Get game's native color depth (bits per pixel)
    inline int GetColorDepth() const { return color_depth * 8; }

    // Tells whether the serialized game data contains certain components
    struct SerializeInfo
    {
        bool HasCCScript = false;
        bool HasWordsDict = false;
        // NOTE: Global messages are cut out, but we still have to check them
        // so long as we keep support of loading an older game data
        std::array<int, NUM_LEGACY_GLOBALMES> HasMessages{};
        // File offset at which game data extensions begin
        uint32_t ExtensionOffset = 0u;
    };

    void ReadFromFile(Common::Stream *in, GameDataVersion game_ver, SerializeInfo &info);
    void WriteToFile(Common::Stream *out, const SerializeInfo &info) const;
};

// GameExtendedProperties contain extension data for the game settings
struct GameExtendedProperties
{
    // Character face direction ratio (y/x)
    float   faceDirectionRatio = 1.f;
};

// GameObjectData contains properties of separate game objects and components
struct GameObjectData
{
    using UInteractionEvents = AGS::Common::UInteractionEvents;
    using HGameFileError = AGS::Common::HGameFileError;
public:
    std::unique_ptr<WordsDictionary> dict;
    std::vector<CharacterInfo> chars;
    // This array is used only to read data into;
    // font parameters are then put and queried in the fonts module
    // TODO: split into installation params (used only when reading) and runtime params
    std::vector<FontInfo> fonts;
    InventoryItemInfo invinfo[MAX_INV]{};
    std::vector<MouseCursor> mcurs;
    std::vector<UInteractionEvents> charScripts;
    std::vector<UInteractionEvents> invScripts;
    // Lip-sync data
    char lipSyncFrameLetters[MAXLIPSYNCFRAMES][50] = { { 0 } };

    // Custom properties (design-time state)
    AGS::Common::PropertySchema propSchema;
    std::vector<AGS::Common::StringIMap> charProps;
    AGS::Common::StringIMap invProps[MAX_INV];
    std::vector<AGS::Common::StringIMap> audioclipProps;
    std::vector<AGS::Common::StringIMap> dialogProps;
    std::vector<AGS::Common::StringIMap> guiProps;
    std::vector<AGS::Common::StringIMap> guicontrolProps[AGS::Common::kGUIControlTypeNum];

    // NOTE: although the view names are stored in game data, they are never
    // used, nor registered as script exports; numeric IDs are used to
    // reference views instead.
    std::vector<Common::String> viewNames;
    Common::String invScriptNames[MAX_INV];
    std::vector<Common::String> dialogScriptNames;

    // Existing room numbers
    std::vector<int> roomNumbers;
    // Saved room names, known during the game compilation;
    // may be used to learn the total number of registered rooms
    std::map<int, Common::String> roomNames;

    std::vector<ScriptAudioClip> audioClips;
    std::vector<AudioClipType> audioClipTypes;
};

struct LoadedGame;

// TODO: split GameSetupStruct into struct used to hold loaded game data, and actual runtime object
struct GameSetupStruct : public GameBasicProperties, GameExtendedProperties, GameObjectData
{
public:
    // TODO: why we do not use this in the engine instead of
    // loaded_game_file_version?
    GameDataVersion   filever = kGameVersion_Undefined;
    Common::String    compiled_with; // version of AGS this data was created by
    // number of accessible game audio channels (the ones under direct user control)
    int               numGameChannels = 0;
    // backward-compatible channel limit that may be exported to script and reserved by audiotypes
    int               numCompatGameChannels = 0;

    // TODO: I converted original array of sprite infos to vector here, because
    // statistically in most games sprites go in long continious sequences with minimal
    // gaps, and standard hash-map will have relatively big memory overhead compared.
    // Of course vector will not behave very well if user has created e.g. only
    // sprite #1 and sprite #1000000. For that reason I decided to still limit static
    // sprite count to some reasonable number for the time being. Dynamic sprite IDs are
    // added in sequence, so there won't be any issue with these.
    // There could be other collection types, more optimal for this case. For example,
    // we could use a kind of hash map containing fixed-sized arrays, where size of
    // array is calculated based on key spread factor.
    std::vector<SpriteInfo> SpriteInfos;

    GameSetupStruct() = default;
    GameSetupStruct(LoadedGame &&loadedgame);
    GameSetupStruct(GameSetupStruct &&gss) = default;
    ~GameSetupStruct() = default;

    GameSetupStruct &operator =(GameSetupStruct &&gss) = default;

    // Returns the expected filename of a digital audio package
    inline AGS::Common::String GetAudioVOXName() const
    {
        return "audio.vox";
    }

    // Returns a list of game options that are forbidden to change at runtime
    inline static std::array<int, 18> GetRestrictedOptions()
    {
        return std::array<int, 18> { {
                OPT_DEBUGMODE, OPT_OBSOLETE_LETTERBOX, OPT_OBSOLETE_HIRES_FONTS, OPT_SPLITRESOURCES,
                    OPT_OBSOLETE_STRICTSCRIPTING, OPT_OBSOLETE_LEFTTORIGHTEVAL, OPT_COMPRESSSPRITES, OPT_OBSOLETE_STRICTSTRINGS,
                    OPT_OBSOLETE_NATIVECOORDINATES, OPT_OBSOLETE_SAFEFILEPATHS, OPT_DIALOGOPTIONSAPI, OPT_BASESCRIPTAPI,
                    OPT_SCRIPTCOMPATLEV, OPT_OBSOLETE_RELATIVEASSETRES, OPT_GAMETEXTENCODING, OPT_KEYHANDLEAPI,
                    OPT_CUSTOMENGINETAG, OPT_VOICECLIPNAMERULE
            }};
    }

    // Returns a list of game options that must be preserved when restoring a save
    inline static std::array<int, 1> GetPreservedOptions()
    {
        return std::array<int, 1> { {
                OPT_SAVECOMPONENTSIGNORE
            }};
    }

    // TODO: move these to a distinct runtime Game class
    void ReadFromSavegame(Common::Stream *in);
    void WriteForSavegame(Common::Stream *out);

private:
    void ApplySpriteFlags(const std::vector<uint8_t> &sprflags);
    void OnResolutionSet();

    // Multiplier for various UI drawing sizes, meant to keep UI elements readable
    int _relativeUIMult = 1;
};

// Struct contains an extended data loaded for chars.
// At the runtime it goes into CharacterExtras struct, which is currently
// not exposed. This may be fixed by future refactoring, such as merging
// CharacterExtras with CharacterInfo structs.
struct CharDataEx
{
    Common::BlendMode BlendMode = Common::kBlend_Normal;
};

struct DialogTopic;
struct ViewStruct;

// LoadedGame is meant for keeping global game data loaded from the game file,
// before it is assigned to their proper positions within the program data
// (engine's or other tool).
struct LoadedGame : public GameBasicProperties, GameExtendedProperties, GameObjectData
{
    std::vector<CharDataEx> CharEx;
    std::vector<Common::GUIMain> Guis;
    Common::GUICollection   GuiControls;
    std::vector<DialogTopic> Dialogs;
    std::vector<ViewStruct> Views;
    UScript                 GlobalScript;
    UScript                 DialogScript;
    std::vector<UScript>    ScriptModules;
    std::vector<Common::PluginInfo> PluginInfos;

    // Original sprite data (when it was read into const-sized arrays)
    size_t                  SpriteCount = 0u;
    std::vector<uint8_t>    SpriteFlags; // SPF_* flags

    Common::HGameFileError ReadFromFile(Common::Stream *in, GameDataVersion game_ver);

private:
    Common::HGameFileError ReadSpriteFlags(Common::Stream *in, GameDataVersion data_ver);
    Common::HGameFileError ReadDialogScript(Common::Stream *in, GameDataVersion data_ver);
    Common::HGameFileError ReadScriptModules(Common::Stream *in, GameDataVersion data_ver);
    Common::HGameFileError ReadPluginInfos(Common::Stream *in, GameDataVersion data_ver);
    Common::HGameFileError ReadCustomProperties(Common::Stream *in, GameDataVersion data_ver);
    Common::HGameFileError ReadAudio(Common::Stream *in, GameDataVersion data_ver);
    Common::HGameFileError ReadRoomNames(Common::Stream *in, GameDataVersion data_ver);
};

#endif // __AGS_CN_AC__GAMESETUPSTRUCT_H
