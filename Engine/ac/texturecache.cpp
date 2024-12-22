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
#include "ac/texturecache.h"

namespace AGS
{
namespace Engine
{

const std::shared_ptr<Texture> TextureCache::Get(const uint32_t &sprite_id)
{
    assert(sprite_id != UINT32_MAX); // only valid sprite IDs may be stored
    if (sprite_id == UINT32_MAX)
        return nullptr;

    // Begin getting texture data, first check the MRU cache
    auto txdata = ResourceCache::Get(sprite_id);
    if (txdata)
        return txdata;

    // If not found in MRU cache, try the short-term cache, which
    // may still hold it so long as there are active textures on screen
    const auto found = _txRefs.find(sprite_id);
    if (found != _txRefs.end())
    {
        txdata = found->second.lock();
        // If found, then cache the texture again, and return
        if (txdata)
        {
            Put(sprite_id, txdata);
            return txdata;
        }
    }
    return nullptr;
}

// Gets existing texture, or load a sprite and create texture from it;
// optionally, if "source" bitmap is provided, then use it
std::shared_ptr<Texture> TextureCache::GetOrLoad(uint32_t sprite_id, Bitmap *source, bool opaque)
{
    assert(sprite_id != UINT32_MAX); // only valid sprite IDs may be stored
    if (sprite_id == UINT32_MAX)
        return nullptr;

    // Try getting existing texture first
    auto txdata = Get(sprite_id);
    if (txdata)
        return txdata;

    // If not in any cache, then try loading the sprite's bitmap,
    // and create a texture data from it
    Bitmap *bitmap = source;
    std::unique_ptr<Bitmap> tmp_source;
    if (!source)
    {
        // Following is a logic of not keeping raw sprite in the cache,
        // and thus potentially saving much RAM.
        // - if texture cache's capacity is > 3/4 of raw sprite cache,
        //   then there's little to no practical reason to keep a raw image.
        // This may be adjusted, or added more rules, as seems necessary.
        bool skip_rawcache =
            GetMaxCacheSize() > (3 * (_spriteset.GetMaxCacheSize() / 4));

        if (_spriteset.IsSpriteLoaded(sprite_id) || !skip_rawcache)
        { // if it's already there, or we are not allowed to skip, then cache normally
            bitmap = _spriteset[sprite_id];
        }
        else
        { // if skipping, ask it to only load, but not keep in raw cache
            tmp_source = _spriteset.LoadSpriteNoCache(sprite_id);
            bitmap = tmp_source.get();
        }
        if (!bitmap)
            return nullptr;
    }

    assert(_gfxDriver);
    if (!_gfxDriver)
        return nullptr;
    txdata.reset(_gfxDriver->CreateTexture(bitmap, opaque));
    if (!txdata)
        return nullptr;

    txdata->ID = sprite_id;
    _txRefs[sprite_id] = txdata;
    Put(sprite_id, txdata);
    return txdata;
}

// Deletes the cached item
void TextureCache::Dispose(const uint32_t &sprite_id)
{
    assert(sprite_id != UINT32_MAX); // only valid sprite IDs may be stored
    // Reset sprite ID for any remaining shared txdata
    DetachSharedTexture(sprite_id);
    ResourceCache::Dispose(sprite_id);
}

// Removes the item from the cache and returns to the caller.
std::shared_ptr<Texture> TextureCache::Remove(const uint32_t &sprite_id)
{
    assert(sprite_id != UINT32_MAX); // only valid sprite IDs may be stored
    // Reset sprite ID for any remaining shared txdata
    DetachSharedTexture(sprite_id);
    return ResourceCache::Remove(sprite_id);
}

size_t TextureCache::CalcSize(const std::shared_ptr<Texture> &item)
{
    assert(item);
    return item ? item->GetMemSize() : 0u;
}

// Marks a shared texture with the invalid sprite ID,
// this logically disconnects this texture from the cache,
// and the game objects will be forced to recreate it on the next update
void TextureCache::DetachSharedTexture(uint32_t sprite_id)
{
    const auto found = _txRefs.find(sprite_id);
    if (found != _txRefs.end())
    {
        auto txdata = found->second.lock();
        if (txdata)
            txdata->ID = UINT32_MAX;
        _txRefs.erase(sprite_id);
    }
}

} // namespace Engine
} // namespace AGS
