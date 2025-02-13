//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// GameEntity script API.
//
//=============================================================================
#include "ac/string.h" // FIXME: refactor/move CreateNewScriptString to where ScriptString is declared
#include "ac/dynobj/scriptobjects.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

extern ScriptString myScriptStringImpl;

const char *GameEntity_GetTypeName(ScriptGameEntity *e)
{
    return CreateNewScriptString(e->GetTypeName());
}

const char *GameEntity_GetScriptName(ScriptGameEntity *e)
{
    return CreateNewScriptString(e->GetScriptName());
}

RuntimeScriptValue Sc_GameEntity_GetTypeName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGameEntity, const char, myScriptStringImpl, GameEntity_GetTypeName);
}

RuntimeScriptValue Sc_GameEntity_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGameEntity, const char, myScriptStringImpl, GameEntity_GetScriptName);
}

void RegisterEntityAPI()
{
    ScFnRegister entity_api[] = {
        { "GameEntity::get_TypeName",   API_FN_PAIR(GameEntity_GetTypeName) },
        { "GameEntity::get_ScriptName", API_FN_PAIR(GameEntity_GetScriptName) },
    };

    ccAddExternalFunctions(entity_api);
}
