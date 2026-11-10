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
#ifndef __AC_SCRIPTCAMERA_H
#define __AC_SCRIPTCAMERA_H

#include "ac/dynobj/cc_dynamicautoobjectref.h"

// ScriptCamera keeps a reference to actual room Camera in script.
struct ScriptCamera final : public CCDynamicAutoObjectRef
{
public:
    ScriptCamera(int id) : CCDynamicAutoObjectRef(id) {}

    const char *GetType() override;
};

// Unserialize camera from the memory stream
ScriptCamera *Camera_Unserialize(int handle, AGS::Common::Stream *in, size_t data_sz);

#endif // __AC_SCRIPTCAMERA_H
