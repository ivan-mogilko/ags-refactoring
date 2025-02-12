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

Game::Game(LoadedGame &&loadedgame)
{
    static_cast<GameBasicProperties &>(*this) = std::move(static_cast<GameBasicProperties &&>(loadedgame));
    static_cast<GameObjectData &>(*this) = std::move(static_cast<GameObjectData &&>(loadedgame));
    static_cast<GameExtendedProperties &>(*this) = std::move(static_cast<GameExtendedProperties &&>(loadedgame));

    // Apply sprite flags read from original format (sequential array)
    SpriteInfos.resize(loadedgame.SpriteCount);
    for (size_t i = 0; i < loadedgame.SpriteCount; ++i)
    {
        SpriteInfos[i].Flags = loadedgame.SpriteFlags[i];
    }

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
