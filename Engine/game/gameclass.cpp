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
#include "game/gameclass.h"

using namespace AGS::Common;

Game::Game(LoadedGame &&loadedgame)
{
    static_cast<GameBasicProperties &>(*this) = std::move(static_cast<GameBasicProperties &&>(loadedgame));
    static_cast<GameExtendedProperties &>(*this) = std::move(static_cast<GameExtendedProperties &&>(loadedgame));

    GameObjectData &gameobj = static_cast<GameObjectData&>(loadedgame);

    dict = std::move(gameobj.dict);
    for (const auto &chinfo : gameobj.chars)
        chars.emplace_back(chinfo);
    invinfo = std::move(gameobj.invinfo);
    mcurs = std::move(gameobj.mcurs);
    charScripts = std::move(gameobj.charScripts);
    invScripts = std::move(gameobj.invScripts);
    memcpy(lipSyncFrameLetters, gameobj.lipSyncFrameLetters, sizeof(lipSyncFrameLetters));

    propSchema = std::move(gameobj.propSchema);
    charProps = std::move(gameobj.charProps);
    invProps = std::move(gameobj.invProps);
    audioclipProps = std::move(gameobj.audioclipProps);
    dialogProps = std::move(gameobj.dialogProps);
    guiProps = std::move(gameobj.guiProps);
    for (int i =0; i < kGUIControlTypeNum; ++i)
        guicontrolProps[i] = std::move(gameobj.guicontrolProps[i]);

    viewNames = std::move(gameobj.viewNames);
    invScriptNames = std::move(gameobj.invScriptNames);
    dialogScriptNames = std::move(gameobj.dialogScriptNames);

    roomNumbers = std::move(gameobj.roomNumbers);
    roomNames = std::move(gameobj.roomNames);

    for (const auto &clipinfo : gameobj.audioClips)
        audioClips.emplace_back(clipinfo);
    audioClipTypes = std::move(gameobj.audioClipTypes);

    // Fixup inventory arrays, must be at least MAX_INV for compliance with engine logic
    invinfo.resize(std::max(numinvitems, MAX_INV));
    invScripts.resize(std::max(numinvitems, MAX_INV));
    invProps.resize(std::max(numinvitems, MAX_INV));
    invScriptNames.resize(std::max(numinvitems, MAX_INV));

    ApplySpriteFlags(loadedgame.SpriteFlags);
    OnResolutionSet();
}

void Game::ApplySpriteFlags(const std::vector<uint8_t> &sprflags)
{
    SpriteInfos.resize(sprflags.size());
    for (size_t i = 0; i < sprflags.size(); ++i)
    {
        SpriteInfos[i].Flags = sprflags[i];
    }
}

void Game::OnResolutionSet()
{
    _relativeUIMult = 1; // NOTE: this is remains of old logic, currently unused.
}

void Game::ReadFromSavegame(Stream *in)
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

void Game::WriteForSavegame(Stream *out)
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
