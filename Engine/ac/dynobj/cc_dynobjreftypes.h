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
// A collection of dynamic object managers, which handle a trivial
// ScriptSimpleRef type of script object. ScriptSimpleRef contains strictly
// a ID reference to the internal object instance, and its managed handles
// may be stored in a array of handles which is also exported to script.
//
// CCDynamicObjectRef is a parent class for such managers.
// 
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__DYNOBJREFTYPES_H
#define __AGS_EE_DYNOBJ__DYNOBJREFTYPES_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/dynobj/scriptobjects.h"

class CCDynamicObjectRef : public AGSCCDynamicObject
{
public:
    CCDynamicObjectRef(std::vector<int> *const arr_of_handles = nullptr);

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
