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

size_t CCDynamicStruct::CalcSerializeSize(const char * address)
{
    const Header &hdr = GetHeader(address);
    return hdr.Size + FileHeaderSz;
}

void CCDynamicStruct::Serialize(const char *address, AGS::Common::Stream *out)
{
    const Header &hdr = GetHeader(address);
    out->Write(address, hdr.Size);
}

void CCDynamicStruct::Unserialize(int index, Stream *in, size_t data_sz)
{
    uint8_t *data = new uint8_t[(data_sz - FileHeaderSz) + MemHeaderSz];
    Header &hdr = reinterpret_cast<Header&>(*data);
    hdr.Size = data_sz;
    in->Read(data + MemHeaderSz, data_sz - FileHeaderSz);
    ccRegisterUnserializedObject(index, &data[MemHeaderSz], this);
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
