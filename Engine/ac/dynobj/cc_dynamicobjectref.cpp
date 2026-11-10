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
#include "ac/dynobj/cc_dynamicobjectref.h"
#include "ac/gamesetupstruct.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scriptobjects.h"
#include "game/roomdata.h"
#include "media/audio/audiodefines.h"
#include "util/stream.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
extern std::vector<ScriptDialog> scrDialog;
extern std::vector<ScriptGUI> scrGui;
extern ScriptHotspot scrHotspot[MAX_ROOM_HOTSPOTS];
extern std::vector<ScriptInvItem> scrInv;
extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern ScriptRegion scrRegion[MAX_ROOM_REGIONS];
extern ScriptWalkableArea scrWalkarea[MAX_WALK_AREAS];
extern ScriptWalkbehind scrWalkbehind[MAX_WALK_BEHINDS];
extern std::vector<int> StaticCharacterArray;
extern std::vector<int> StaticDialogArray;
extern std::vector<int> StaticGUIArray;
extern std::vector<int> StaticHotspotArray;
extern std::vector<int> StaticInventoryArray;
extern std::vector<int> StaticObjectArray;


size_t CCDynamicObjectRef::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t); // always a int32 (object's numeric ID)
}

void CCDynamicObjectRef::Serialize(const void *address, Stream *out)
{
    const ScriptObjectRef *scobjref = static_cast<const ScriptObjectRef*>(address);
    out->WriteInt32(scobjref->id);
}

void CCDynamicObjectRef::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    int id = in->ReadInt32();
    if (id >= 0)
    {
        void *ref = GetObjectRef(id);
        if (ref)
        {
            int handle = ccRegisterUnserializedPersistentObject(index, ref, this);
            // WARNING: we must never resize array of handles here, because it's address
            // is registered in script exports table; any realloc will break that.
            if (_arrOfHandles && (id >= 0) && (static_cast<uint32_t>(id) < _arrOfHandles->size()))
                (*_arrOfHandles)[id] = handle;
        }
    }
}


const char *CCAudioChannel::GetType()
{
    return "AudioChannel";
}

void *CCAudioChannel::GetObjectRef(int index)
{
    return (index >= 0 && index < MAX_GAME_CHANNELS) ? &scrAudioChannel[index] : nullptr;
}

const char *CCAudioClip::GetType()
{
    return "AudioClip";
}

void *CCAudioClip::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < game.audioClips.size()) ? &game.audioClips[index] : nullptr;
}

CCCharacter::CCCharacter()
    : CCDynamicObjectRef(&StaticCharacterArray)
{
}

const char *CCCharacter::GetType()
{
    return "Character";
}

void *CCCharacter::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < game.chars.size()) ? &game.chars[index] : nullptr;
}

CCDialog::CCDialog()
    : CCDynamicObjectRef(&StaticDialogArray)
{
}

const char *CCDialog::GetType()
{
    return "Dialog";
}

void *CCDialog::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < scrDialog.size()) ? &scrDialog[index] : nullptr;
}

CCGUI::CCGUI()
    : CCDynamicObjectRef(&StaticGUIArray)
{
}

const char *CCGUI::GetType()
{
    return "GUI";
}

void *CCGUI::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < scrGui.size()) ? &scrGui[index] : nullptr;
}

CCHotspot::CCHotspot()
    : CCDynamicObjectRef(&StaticHotspotArray)
{
}

const char *CCHotspot::GetType()
{
    return "Hotspot";
}

void *CCHotspot::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < MAX_ROOM_HOTSPOTS) ? &scrHotspot[index] : nullptr;
}

ScriptInvItem CCInventory::_dummy;

CCInventory::CCInventory()
    : CCDynamicObjectRef(&StaticInventoryArray)
{
}

const char *CCInventory::GetType()
{
    return "Inventory";
}

void *CCInventory::GetObjectRef(int index)
{
    // NOTE: older versions apparently had MAX_INV script objects registered in managed memory.
    // TODO: share this algorithm with all managed object unserialization (make template function?).
    if (index >= 0 && static_cast<uint32_t>(index) < scrInv.size())
    {
        return &scrInv[index];
    }
    else
    {
        return &_dummy;
    }
}

CCObject::CCObject()
    : CCDynamicObjectRef(&StaticObjectArray)
{
}

const char *CCObject::GetType()
{
    return "Object";
}

void *CCObject::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < MAX_ROOM_OBJECTS) ? &scrObj[index] : nullptr;
}

const char *CCRegion::GetType()
{
    return "Region";
}

void *CCRegion::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < MAX_ROOM_REGIONS) ? &scrRegion[index] : nullptr;
}

const char *CCWalkableArea::GetType()
{
    return "WalkableArea";
}

void *CCWalkableArea::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < MAX_WALK_AREAS) ? &scrWalkarea[index] : nullptr;
}

const char *CCWalkbehind::GetType()
{
    return "Walkbehind";
}

void *CCWalkbehind::GetObjectRef(int index)
{
    return (index >= 0 && static_cast<uint32_t>(index) < MAX_WALK_BEHINDS) ? &scrWalkbehind[index] : nullptr;
}
