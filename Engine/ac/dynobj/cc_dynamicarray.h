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
#ifndef __CC_DYNAMICARRAY_H
#define __CC_DYNAMICARRAY_H

#include <vector>
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/stream.h"


#define ARRAY_MANAGED_TYPE_FLAG    0x80000000
#define ARRAY_SHARED_MEMORY        0x40000000

struct CCDynamicArray final : AGSCCDynamicObject
{
public:
    static const char *TypeName;

    struct Header
    {
        uint32_t ElemCount = 0u;
        // TODO: refactor and store "elem size" instead
        uint32_t TotalSize = 0u;
        uint32_t Flags = 0u;
    };

    inline const Header &GetHeader(void *address) const
    {
        return _hdr;
    }

    CCDynamicArray() = default; // ?
    ~CCDynamicArray();
    // return the type name of the object
    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz);
    // Create managed array object and return a pointer to the beginning of a buffer
    static DynObjectRef Create(int numElements, int elementSize, bool isManagedType);
    static DynObjectRef Create(uint8_t *shared_data, int numElements, int elementSize, bool isManagedType);

private:
    Header _hdr;
    uint8_t *_data = nullptr;

    // The size of the serialized header
    static const size_t FileHeaderSz = sizeof(uint32_t) * 3;

    static DynObjectRef CreateImpl(uint8_t *data, int numElements, int elementSize, bool isManagedType, bool shared);

    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(void *address) override;
    // Write object data into the provided stream
    void Serialize(void *address, AGS::Common::Stream *out) override;
};

// Helper functions for setting up dynamic arrays.
namespace DynamicArrayHelpers
{
    // Create array of managed strings
    DynObjectRef CreateStringArray(const std::vector<const char*>);
};

#endif