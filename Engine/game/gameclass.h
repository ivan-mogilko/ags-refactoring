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
// Game runtime class.
// TODO: merge with GamePlayState.
//
//=============================================================================
#ifndef __AGS_EE_GAME__GAMECLASS_H
#define __AGS_EE_GAME__GAMECLASS_H
#include "ac/gamedata.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "game/characterclass.h"

struct LoadedGame;

// TODO: split GameSetupStruct into struct used to hold loaded game data, and actual runtime object
class Game : public GameBasicProperties, public GameExtendedProperties
{
    using Stream = AGS::Common::Stream;
    using String = AGS::Common::String;
    using UInteractionEvents = AGS::Common::UInteractionEvents;
    using HGameFileError = AGS::Common::HGameFileError;
public:
    std::unique_ptr<WordsDictionary> dict;
    std::vector<Character> chars;
    std::vector<InventoryItemInfo> invinfo;
    std::vector<MouseCursor> mcurs;
    std::vector<UInteractionEvents> charScripts;
    std::vector<UInteractionEvents> invScripts;
    // Lip-sync data
    char lipSyncFrameLetters[MAXLIPSYNCFRAMES][50] = { { 0 } };

    // Custom properties (design-time state)
    AGS::Common::PropertySchema propSchema;
    std::vector<AGS::Common::StringIMap> charProps;
    std::vector<AGS::Common::StringIMap> invProps;
    std::vector<AGS::Common::StringIMap> audioclipProps;
    std::vector<AGS::Common::StringIMap> dialogProps;
    std::vector<AGS::Common::StringIMap> guiProps;
    std::vector<AGS::Common::StringIMap> guicontrolProps[AGS::Common::kGUIControlTypeNum];

    // NOTE: although the view names are stored in game data, they are never
    // used, nor registered as script exports; numeric IDs are used to
    // reference views instead.
    std::vector<Common::String> viewNames;
    std::vector<Common::String> invScriptNames;
    std::vector<Common::String> dialogScriptNames;

    // Existing room numbers
    std::vector<int> roomNumbers;
    // Saved room names, known during the game compilation;
    // may be used to learn the total number of registered rooms
    std::map<int, Common::String> roomNames;

    std::vector<ScriptAudioClip> audioClips;
    std::vector<AudioClipType> audioClipTypes;

    // TODO: why we do not use this in the engine instead of
    // loaded_game_file_version?
    GameDataVersion   filever = kGameVersion_Undefined;
    String            compiled_with; // version of AGS this data was created by
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

    Game() = default;
    Game(LoadedGame &&loadedgame);
    Game(Game &&gss) = default;
    ~Game() = default;

    Game &operator =(Game &&gss) = default;

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
    void ReadFromSavegame(Stream *in);
    void WriteForSavegame(Stream *out);

private:
    void ApplySpriteFlags(const std::vector<uint8_t> &sprflags);
    void OnResolutionSet();

    // Multiplier for various UI drawing sizes, meant to keep UI elements readable
    int _relativeUIMult = 1;
};

#endif // __AGS_EE_GAME__GAMECLASS_H
