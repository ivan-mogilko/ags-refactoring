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

#include "ac/dynobj/cc_dynamicobject.h"
#include "util/stream.h"


struct CCDynamicStruct final : ICCDynamicObject
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
    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    int Serialize(const char *address, char *buffer, int bufsize) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz);

    // Create managed object and return a pointer to the beginning of a buffer
    DynObjectRef Create(size_t size);

    // Support for reading and writing object values by their relative offset
    const char* GetFieldPtr(const char *address, intptr_t offset) override;
    void    Read(const char *address, intptr_t offset, void *dest, int size) override;
    uint8_t ReadInt8(const char *address, intptr_t offset) override;
    int16_t ReadInt16(const char *address, intptr_t offset) override;
    int32_t ReadInt32(const char *address, intptr_t offset) override;
    float   ReadFloat(const char *address, intptr_t offset) override;
    void    Write(const char *address, intptr_t offset, void *src, int size) override;
    void    WriteInt8(const char *address, intptr_t offset, uint8_t val) override;
    void    WriteInt16(const char *address, intptr_t offset, int16_t val) override;
    void    WriteInt32(const char *address, intptr_t offset, int32_t val) override;
    void    WriteFloat(const char *address, intptr_t offset, float val) override;

private:
    // The size of the object's header in memory, prepended to the object data
    static const size_t MemHeaderSz = sizeof(uint32_t) * 1;
    // The size of the serializedheader
    static const size_t FileHeaderSz = sizeof(uint32_t) * 0;
};

extern CCDynamicStruct globalDynamicStruct;


// Helper functions for setting up custom managed structs based on CCDynamicStruct.
namespace ScriptStructHelpers
{
    // Creates a managed Point object, represented as a pair of X and Y coordinates.
    DynObjectRef CreatePoint(int x, int y);
};

#endif // __AGS_EE_DYNOBJ__CCDYNAMICSTRUCT_H
