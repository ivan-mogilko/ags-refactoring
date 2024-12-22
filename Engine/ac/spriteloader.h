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
#ifndef __AGS_EE_AC__SPRITELOADER_H
#define __AGS_EE_AC__SPRITELOADER_H

#include "ac/spritecache.h"
#include "ac/texturecache.h"
#include "util/asyncjobmanager.h"

namespace AGS
{
namespace Engine
{

class SpriteLoader
{
    using sprkey_t = AGS::Common::sprkey_t;
    using SpriteFile = AGS::Common::SpriteFile;
    using SpriteCache = AGS::Common::SpriteCache;
public:
    SpriteLoader(SpriteCache *sprcache, TextureCache *txcache,
        IGraphicsDriver *gfx_driver);

    void     Start();
    void     Stop();
    void     Suspend();
    void     Resume();

    void     AddSprite(sprkey_t sprite_id);
    void     AddSprites(sprkey_t sprite_from, sprkey_t sprite_to);
    void     AddView(int view);
    void     AddViewLoop(int view, int loop);
    void     AddViewLoops(int view, int loop_from, int loop_to);

private:
    std::unique_ptr<AsyncJobManager> _asyncMgr;
    std::unique_ptr<SpriteFile> _spriteFile;
    SpriteCache *_spriteCache = nullptr;
    TextureCache *_textureCache = nullptr;
    IGraphicsDriver *_gfxDriver = nullptr;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_AC__SPRITELOADER_H
