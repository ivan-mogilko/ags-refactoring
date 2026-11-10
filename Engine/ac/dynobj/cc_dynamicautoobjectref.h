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
//
// CCDynamicAutoObjectRef is parent class for dynamic object managers that:
// - are exported as script objects themselves (handled in managed pool);
// - refer to dynamically created game objects;
// - contain only ID to the object data stored separately by the engine;
// - can be "invalidated" in order to detach them from disposed game data.
// - Dispose() simply deletes its own instance (delete this).
//
// Examples: overlays, dynamic sprites, etc.
//
// The inheriting types are still allowed to override disposal (if they
// must delete something else besides themselves), and serialization:
// some of them do not need to save id at all, and reset themselves when
// being restored.
// 
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__DYNAUTOOBJREFTYPES_H
#define __AGS_EE_DYNOBJ__DYNAUTOOBJREFTYPES_H

#include "ac/dynobj/cc_agsdynamicobject.h"

class CCDynamicAutoObjectRef : public AGSCCDynamicObject
{
public:
    CCDynamicAutoObjectRef() = default;
    CCDynamicAutoObjectRef(int id) : _objID(id) {}

    // Get referenced object's index; negative means that the script object
    // was detached from the game data (most likely because it got removed)
    // The related script functions must test for this state, and handle
    // this situation as if object is no longer available.
    int GetID() const { return _objID; }
    // Changes a referenced object's index.
    // This operation is used when the game objects are stored in a plain
    // list, where removing an object will require to shift other objects's
    // indexes (not a good thing, but exists currently).
    void SetID(int id) { _objID = id; }
    // Tests if this script object is detached from the game data.
    // The related script functions must test for this state, and handle
    // this situation as if object is no longer available.
    inline bool IsValid() const { return _objID < 0; }
    // Invalidates this script object, detaching it from the game data.
    // This lets to keep a reference in script, but not connected to a
    // real object in game.
    void Invalidate() { _objID = -1; }

    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

protected:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    uint32_t _objID = -1;
};


#endif // __AGS_EE_DYNOBJ__DYNAUTOOBJREFTYPES_H
