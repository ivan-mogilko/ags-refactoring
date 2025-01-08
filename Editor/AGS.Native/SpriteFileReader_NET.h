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
#pragma once

#include <memory>
#include "ac/spritefile.h"
#include "gfx/bitmap.h"

namespace AGS
{
namespace Native
{

typedef AGS::Common::Bitmap AGSBitmap;

public ref class SpriteFile
{
public:
    enum class StorageFlags
    {
        OptimizeForSize = 0x01
    };

    ref class RawSpriteData
    {
    public:
        RawSpriteData()
        {
            _hdr = new AGS::Common::SpriteDatHeader();
            _data = new std::vector<uint8_t>();
        }

        ~RawSpriteData()
        {
            this->!RawSpriteData();
        }

        !RawSpriteData()
        {
            delete _hdr;
            delete _data;
        }

        AGS::Common::SpriteDatHeader *GetHeader()
        {
            return _hdr;
        }

        std::vector<uint8_t> *GetData()
        {
            return _data;
        }

    private:
        AGS::Common::SpriteDatHeader *_hdr = nullptr;
        std::vector<uint8_t> *_data = nullptr;
    };
};

public ref class NativeBitmap
{
public:
    NativeBitmap() {}
    NativeBitmap(AGSBitmap *bitmap)
        : _bitmap(bitmap) {}
    NativeBitmap(int width, int height, int colorDepth)
    {
        _bitmap = new AGSBitmap(width, height, colorDepth);
    }

    ~NativeBitmap()
    {
        this->!NativeBitmap();
    }

    !NativeBitmap()
    {
        delete _bitmap;
    }

    static NativeBitmap ^CreateCopy(NativeBitmap ^src, int colorDepth)
    {
        AGSBitmap *copy = AGS::Common::BitmapHelper::CreateBitmapCopy(src->GetNativePtr(), colorDepth);
        // FIXME: I suppose this should be done by the graphic library,
        // unconditionally when blitting low-depth to 32-bit dest!!
        if (colorDepth == 32 && src->GetNativePtr()->GetColorDepth() < 32)
            AGS::Common::BitmapHelper::MakeOpaque(copy);
        return gcnew NativeBitmap(copy); // NativeBitmap will own a copy
    }

    AGSBitmap *GetNativePtr()
    {
        return _bitmap;
    }

private:
    AGSBitmap *_bitmap = nullptr;
};

public ref class SpriteFileReader : public SpriteFile
{
public:
    SpriteFileReader(System::String ^filename);
    SpriteFileReader(System::String ^spritesetFilename, System::String ^indexFilename);
    ~SpriteFileReader() { this->!SpriteFileReader(); }
    !SpriteFileReader();

    System::Drawing::Bitmap ^LoadSprite(int spriteIndex);
    NativeBitmap  ^LoadSpriteAsNativeBitmap(int spriteIndex);
    RawSpriteData ^LoadSpriteAsRawData(int spriteIndex);
    bool           LoadSpriteAsRawData(int spriteIndex, RawSpriteData ^rawData);

private:
    AGS::Common::SpriteFile *_nativeReader = nullptr;
    std::vector<AGS::Common::GraphicResolution> *_metrics = nullptr;
};

} // namespace Native
} // namespace AGS
