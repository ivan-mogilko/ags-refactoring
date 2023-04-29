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
#include <memory.h>
#include "cc_dynamicstruct.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/memorystream.h"

using namespace AGS::Common;

const char *CCDynamicStruct::TypeName = "UserObject";

// return the type name of the object
const char *CCDynamicStruct::GetType()
{
    return TypeName;
}

DynObjectRef CCDynamicStruct::Create(size_t size)
{
    uint8_t *data = new uint8_t[size + MemHeaderSz];
    memset(data, 0, size + MemHeaderSz);
    Header &hdr = reinterpret_cast<Header&>(*data);
    hdr.Size = size;
    void *obj_ptr = &data[MemHeaderSz];
    int32_t handle = ccRegisterManagedObject(obj_ptr, this);
    if (handle == 0)
    {
        delete[] data;
        return DynObjectRef();
    }
    return DynObjectRef(handle, obj_ptr, this);
}

int CCDynamicStruct::Dispose(const char* address, bool /*force*/)
{
    delete[] (address - MemHeaderSz);
    return 1;
}

int CCDynamicStruct::Serialize(const char* address, char *buffer, int bufsize)
{
    const Header &hdr = GetHeader(address);
    int sz_to_write = hdr.Size + FileHeaderSz;
    if (sz_to_write > bufsize)
    {
        // buffer not big enough, ask for a bigger one
        return -sz_to_write;
    }

    memcpy(buffer, address, hdr.Size);
    return hdr.Size;
}

void CCDynamicStruct::Unserialize(int index, Stream *in, size_t data_sz)
{
    uint8_t *data = new uint8_t[(data_sz - FileHeaderSz) + MemHeaderSz];
    Header &hdr = reinterpret_cast<Header&>(*data);
    hdr.Size = data_sz;
    in->Read(data + MemHeaderSz, data_sz - FileHeaderSz);
    ccRegisterUnserializedObject(index, &data[MemHeaderSz], this);
}

const char* CCDynamicStruct::GetFieldPtr(const char* address, intptr_t offset)
{
    return address + offset;
}

void CCDynamicStruct::Read(const char* address, intptr_t offset, void *dest, int size)
{
    memcpy(dest, address + offset, size);
}

uint8_t CCDynamicStruct::ReadInt8(const char* address, intptr_t offset)
{
    return *(uint8_t*)(address + offset);
}

int16_t CCDynamicStruct::ReadInt16(const char* address, intptr_t offset)
{
    return *(int16_t*)(address + offset);
}

int32_t CCDynamicStruct::ReadInt32(const char* address, intptr_t offset)
{
    return *(int32_t*)(address + offset);
}

float CCDynamicStruct::ReadFloat(const char* address, intptr_t offset)
{
    return *(float*)(address + offset);
}

void CCDynamicStruct::Write(const char* address, intptr_t offset, void *src, int size)
{
    memcpy((void*)(address + offset), src, size);
}

void CCDynamicStruct::WriteInt8(const char* address, intptr_t offset, uint8_t val)
{
    *(uint8_t*)(address + offset) = val;
}

void CCDynamicStruct::WriteInt16(const char* address, intptr_t offset, int16_t val)
{
    *(int16_t*)(address + offset) = val;
}

void CCDynamicStruct::WriteInt32(const char* address, intptr_t offset, int32_t val)
{
    *(int32_t*)(address + offset) = val;
}

void CCDynamicStruct::WriteFloat(const char* address, intptr_t offset, float val)
{
    *(float*)(address + offset) = val;
}


CCDynamicStruct globalDynamicStruct;


// Allocates managed struct containing two ints: X and Y
DynObjectRef ScriptStructHelpers::CreatePoint(int x, int y)
{
    auto ref = globalDynamicStruct.Create(sizeof(int32_t) * 2);
    ref.Mgr->WriteInt32((const char*)ref.Obj, 0, x);
    ref.Mgr->WriteInt32((const char*)ref.Obj, sizeof(int32_t), y);
    return ref;
}
