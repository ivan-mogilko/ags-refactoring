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

#ifndef __AGS_EE_DYNOBJ__SCRIPTLOADEDSAVEINFO_H
#define __AGS_EE_DYNOBJ__SCRIPTLOADEDSAVEINFO_H

#include <memory>
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "game/loadedsaveinfo.h"

typedef std::auto_ptr<AGS::Engine::LoadedSaveInfo> ALoadedSaveInfo;

class ScriptLoadedSaveInfo : public AGSCCDynamicObject
{
public:
    ScriptLoadedSaveInfo();

    // return the type name of the object
    virtual const char *GetType();
    // serialize the object into buffer (which is bufsize bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    // unserialize the object from buffer using dataSize bytes
    virtual void Unserialize(int index, const char *serializedData, int dataSize);

    // Tells if the game restoration was cancelled from game script
    bool IsRestoreCancelled() const { return _cancelRestore; }
    // Copies LoadedSaveInfo to the managed object
    void Set(const AGS::Engine::LoadedSaveInfo &info);
    // Deletes LoadedSaveInfo (this does not dispose managed object itself)
    void Reset();

private:
    // A save load resolution got from the game script
    bool            _cancelRestore;
    // Description of the loaded save contents
    ALoadedSaveInfo _info;
};


#endif // __AGS_EE_DYNOBJ__SCRIPTLOADEDSAVEINFO_H