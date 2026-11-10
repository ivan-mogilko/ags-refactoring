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
#ifndef __AC_SCRIPTVIEWPORT_H
#define __AC_SCRIPTVIEWPORT_H

#include "ac/dynobj/cc_dynamicautoobjectref.h"

// ScriptViewport keeps a reference to actual room Viewport in script.
struct ScriptViewport final : public CCDynamicAutoObjectRef
{
public:
    ScriptViewport(int id) : CCDynamicAutoObjectRef(id) {}

    const char *GetType() override;
};

// Unserialize viewport from the memory stream
ScriptViewport *Viewport_Unserialize(int handle, AGS::Common::Stream *in, size_t data_sz);

#endif // __AC_SCRIPTVIEWPORT_H
