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
//
// TextureCache class stores textures created by the GraphicsDriver from plain bitmaps.
// Consists of two parts:
// * A long-term MRU cache, which keeps texture data even when it's not in immediate use,
//   and disposes less used textures to free space when reaching the configured mem limit.
// * A short-term cache of texture references, which keeps only weak refs to the textures
//   that are currently in use. This short-term cache lets to keep reusing same texture
//   so long as there's at least one object on screen that uses it.
// NOTE: because of this two-component structure, TextureCache has to override
// number of ResourceCache's parent methods. This design may probably be improved.
//
//=============================================================================
#ifndef __AGS_EE_AC__TEXTURECACHE_H
#define __AGS_EE_AC__TEXTURECACHE_H

#include "ac/spritecache.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "util/resourcecache.h"

namespace AGS
{
namespace Engine
{

class TextureCache :
    public Common::ResourceCache<uint32_t, std::shared_ptr<Texture>>
{
    using Bitmap = AGS::Common::Bitmap;
    using SpriteCache = AGS::Common::SpriteCache;
public:
    TextureCache(SpriteCache &spriteset)
        : _spriteset(spriteset)
    {}

    // TODO: separate interface for DDB factory? because we only need CreateTexture
    void SetGraphicsDriver(IGraphicsDriver *gfx_driver)
    {
        _gfxDriver = gfx_driver;
    }

    // Gets existing texture from either MRU cache, or short-term cache
    const std::shared_ptr<Texture> Get(const uint32_t &sprite_id);
    // Gets existing texture, or load a sprite and create texture from it;
    // optionally, if "source" bitmap is provided, then use it
    std::shared_ptr<Texture> GetOrLoad(uint32_t sprite_id, Bitmap *source, bool opaque);
    // Deletes the cached item
    void Dispose(const uint32_t &sprite_id);
    // Removes the item from the cache and returns to the caller.
    std::shared_ptr<Texture> Remove(const uint32_t &sprite_id);

private:
    size_t CalcSize(const std::shared_ptr<Texture> &item) override;

    // Marks a shared texture with the invalid sprite ID,
    // this logically disconnects this texture from the cache,
    // and the game objects will be forced to recreate it on the next update
    void DetachSharedTexture(uint32_t sprite_id);

    // A reference to the raw sprites cache, which is also used to load
    // sprites from the asset file.
    SpriteCache &_spriteset;
    // TODO: separate interface for DDB factory?
    IGraphicsDriver *_gfxDriver = nullptr;
    // Texture short-term cache:
    // - caches textures while they are in the immediate use;
    // - this lets to share same texture data among multiple sprites on screen.
    typedef std::weak_ptr<Texture> TexDataRef;
    std::unordered_map<uint32_t, TexDataRef> _txRefs;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_AC__TEXTURECACHE_H

