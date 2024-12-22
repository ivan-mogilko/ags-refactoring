//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/spriteloader.h"

namespace AGS
{
namespace Engine
{

SpriteLoader::SpriteLoader(SpriteCache *sprcache, TextureCache *txcache,
        IGraphicsDriver *gfx_driver)
    : _spriteCache(sprcache)
    , _textureCache(txcache)
    , _gfxDriver(gfx_driver)
{

}

void SpriteLoader::Start()
{

}

void SpriteLoader::Stop()
{

}

void SpriteLoader::Suspend()
{

}

void SpriteLoader::Resume()
{

}

void SpriteLoader::AddSprite(sprkey_t sprite_id)
{
}

void SpriteLoader::AddSprites(sprkey_t sprite_from, sprkey_t sprite_to)
{

}

void SpriteLoader::AddView(int view)
{

}

void SpriteLoader::AddViewLoop(int view, int loop)
{

}

void SpriteLoader::AddViewLoops(int view, int loop_from, int loop_to)
{

}

} // namespace Engine
} // namespace AGS
