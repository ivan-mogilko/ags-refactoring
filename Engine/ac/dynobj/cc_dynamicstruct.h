//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Managed object, which size and contents are defined by user script
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__CCDYNAMICSTRUCT_H
#define __AGS_EE_DYNOBJ__CCDYNAMICSTRUCT_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/stream.h"


struct CCDynamicStruct final : AGSCCDynamicObject
{
public:
    static const char *TypeName;

    struct Header
    {
        // Size of the object's data
        uint32_t Size = 0u;
    };

    inline static const Header &GetHeader(const char *address)
    {
        return reinterpret_cast<const Header&>(*(address - MemHeaderSz));
    }

    // return the type name of the object
    const char *GetType() override;
    int Dispose(const char *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    // Create managed object and return a pointer to the beginning of a buffer
    DynObjectRef Create(size_t size);

private:
    // The size of the object's header in memory, prepended to the object data
    static const size_t MemHeaderSz = sizeof(uint32_t) * 1;
    // The size of the serializedheader
    static const size_t FileHeaderSz = sizeof(uint32_t) * 0;

    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const char *address) override;
    // Write object data into the provided stream
    void Serialize(const char *address, AGS::Common::Stream *out) override;
};

extern CCDynamicStruct globalDynamicStruct;


// Helper functions for setting up custom managed structs based on CCDynamicStruct.
namespace ScriptStructHelpers
{
    // Creates a managed Point object, represented as a pair of X and Y coordinates.
    DynObjectRef CreatePoint(int x, int y);
};

#endif // __AGS_EE_DYNOBJ__CCDYNAMICSTRUCT_H
