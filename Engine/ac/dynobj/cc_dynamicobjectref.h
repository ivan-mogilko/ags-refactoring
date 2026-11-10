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
// CCDynamicObjectRef is parent class for dynamic object managers that:
// - handle persistent game objects, using a trivial ScriptObjectRef type.
// - serialize only ID (ScriptObjectRef's member).
// - on deserialization may optionally store the object's handle in the array
//   of handles which is also exported to script as a variable
//   (e.g. "character", "inventory" etc).
//
// The only managed or persistent objects that is currently not a child of
// CCDynamicObjectRef - is CCGUIControl, because it unserializes 2 IDs
// (parent gui id and control's id).
// 
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__DYNOBJREFTYPES_H
#define __AGS_EE_DYNOBJ__DYNOBJREFTYPES_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/dynobj/scriptobjects.h"

class CCDynamicObjectRef : public AGSCCDynamicObject
{
public:
    CCDynamicObjectRef() = default;
    CCDynamicObjectRef(std::vector<int> *const arr_of_handles)
        : _arrOfHandles(arr_of_handles) {}

    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

protected:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    virtual void *GetObjectRef(int index) = 0;

    // Array of managed object handles, exported to script
    std::vector<int> *const _arrOfHandles = nullptr;
};


class CCAudioChannel final : public CCDynamicObjectRef
{
public:
    CCAudioChannel() = default;
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCAudioClip final : public CCDynamicObjectRef
{
public:
    CCAudioClip() = default;
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCCharacter final : public CCDynamicObjectRef
{
public:
    CCCharacter();
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCDialog final : public CCDynamicObjectRef
{
public:
    CCDialog();
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCGUI : public CCDynamicObjectRef
{
public:
    CCGUI();
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCTextWindowGUI final : public CCGUI
{
public:
    const char *GetType() override { return "TextWindowGUI"; }
};

class CCHotspot : public CCDynamicObjectRef
{
public:
    CCHotspot();
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCInventory : public CCDynamicObjectRef
{
public:
    CCInventory();
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;

    static ScriptInvItem _dummy; // for registering invalid restored objects
};

class CCObject : public CCDynamicObjectRef
{
public:
    CCObject();
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCRegion : public CCDynamicObjectRef
{
public:
    CCRegion() = default;
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCWalkableArea : public CCDynamicObjectRef
{
public:
    CCWalkableArea() = default;
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};

class CCWalkbehind : public CCDynamicObjectRef
{
public:
    CCWalkbehind() = default;
    const char *GetType() override;

protected:
    void *GetObjectRef(int index) override;
};


#endif // __AGS_EE_DYNOBJ__DYNOBJREFTYPES_H
