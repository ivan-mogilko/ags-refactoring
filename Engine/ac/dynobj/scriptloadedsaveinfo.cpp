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

#include "ac/dynobj/scriptloadedsaveinfo.h"

using namespace AGS::Engine;

ScriptLoadedSaveInfo::ScriptLoadedSaveInfo()
    : _cancelRestore(false)
{
}

const char *ScriptLoadedSaveInfo::GetType()
{
    return "LoadedSaveInfo";
}

int ScriptLoadedSaveInfo::Serialize(const char *address, char *buffer, int bufsize)
{
    // LoadedSaveInfo is not persistent
    return 0;
}

void ScriptLoadedSaveInfo::Unserialize(int index, const char *serializedData, int dataSize)
{
    // LoadedSaveInfo is not persistent
    ccRegisterUnserializedObject(index, this, this);
}

void ScriptLoadedSaveInfo::Set(const LoadedSaveInfo &info)
{
    _cancelRestore = false;
    _info.reset(new LoadedSaveInfo(info));
}

void ScriptLoadedSaveInfo::SetRestoreCancelled(bool cancel)
{
    _cancelRestore = cancel;
}

void ScriptLoadedSaveInfo::Reset()
{
    _cancelRestore = false;
    _info.reset();
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

RuntimeScriptValue Sc_LoadedSaveInfo_GetCancelRestore(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::CancelRestore");
    API_VARGET_INT(((ScriptLoadedSaveInfo*)self)->IsRestoreCancelled());
}

RuntimeScriptValue Sc_LoadedSaveInfo_SetCancelRestore(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::CancelRestore");
    ASSERT_VARIABLE_VALUE("LoadedSaveInfo::CancelRestore");
    ((ScriptLoadedSaveInfo*)self)->SetRestoreCancelled(params[0].GetAsBool());
    return RuntimeScriptValue();
}

RuntimeScriptValue Sc_LoadedSaveInfo_IsValid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::Valid");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    return RuntimeScriptValue().SetInt32AsBool(info != NULL);
}

RuntimeScriptValue Sc_LoadedSaveInfo_AudioTypeCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::AudioTypeCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->AudioTypeCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_AudioClipCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::AudioClipCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->AudioClipCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_CharCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::CharCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->CharCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_DialogCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::DialogCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->DialogCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_GUICount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::GUICount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->GUICount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_GUIBtnCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::GUIBtnCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->GUIBtnCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_GUILblCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::GUILblCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->GUILblCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_GUIInvCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::GUIInvCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->GUIInvCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_GUISldCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::GUISldCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->GUISldCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_GUITbxCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::GUITbxCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->GUITbxCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_GUILbxCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::GUILbxCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->GUILbxCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_InvItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::InvItemCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->InvItemCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_MouseCurCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::MouseCurCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->MouseCurCount : -1);
}

RuntimeScriptValue Sc_LoadedSaveInfo_ViewCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF("LoadedSaveInfo::ViewCount");
    const LoadedSaveInfo *info = ((ScriptLoadedSaveInfo*)self)->GetInfo();
    API_VARGET_INT(info ? info->Views.size() : -1);
}

void RegisterLoadedSaveInfoAPI()
{
    ccAddExternalObjectFunction("LoadedSaveInfo::get_CancelRestore",   Sc_LoadedSaveInfo_GetCancelRestore);
    ccAddExternalObjectFunction("LoadedSaveInfo::set_CancelRestore",   Sc_LoadedSaveInfo_SetCancelRestore);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_Valid",           Sc_LoadedSaveInfo_IsValid);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_AudioTypeCount",  Sc_LoadedSaveInfo_AudioTypeCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_AudioClipCount",  Sc_LoadedSaveInfo_AudioClipCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_CharacterCount",  Sc_LoadedSaveInfo_CharCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_DialogCount",     Sc_LoadedSaveInfo_DialogCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_GUICount",        Sc_LoadedSaveInfo_GUICount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_ButtonCount",     Sc_LoadedSaveInfo_GUIBtnCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_LabelCount",      Sc_LoadedSaveInfo_GUILblCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_InvWindowCount",  Sc_LoadedSaveInfo_GUIInvCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_SliderCount",     Sc_LoadedSaveInfo_GUISldCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_TextBoxCount",    Sc_LoadedSaveInfo_GUITbxCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_ListBoxCount",    Sc_LoadedSaveInfo_GUILbxCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_InventoryCount",  Sc_LoadedSaveInfo_InvItemCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_MouseCursorCount",Sc_LoadedSaveInfo_MouseCurCount);
    ccAddExternalObjectFunction("LoadedSaveInfo::get_ViewCount",       Sc_LoadedSaveInfo_ViewCount);
}
