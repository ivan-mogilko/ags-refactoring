//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/dynobj/cc_dynamicautoobjectref.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

int CCDynamicAutoObjectRef::Dispose(void *address, bool force)
{
    delete this;
    return 1;
}

size_t CCDynamicAutoObjectRef::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t); // always a int32 (object's numeric ID)
}

void CCDynamicAutoObjectRef::Serialize(const void *address, Stream *out)
{
    out->WriteInt32(_objID);
}

void CCDynamicAutoObjectRef::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    _objID = in->ReadInt32();
    ccRegisterUnserializedObject(index, this, this);
}
