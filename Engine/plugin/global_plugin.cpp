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
//
// Stubs for plugin functions.
//
//=============================================================================

#include <string.h>
#include "ac/global_plugin.h"
#include "ac/mouse.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "util/string_utils.h"
#include "util/textstreamreader.h"

using namespace AGS::Common;

int pluginSimulatedClick = NONE;

void PluginSimulateMouseClick(int pluginButtonID) {
    pluginSimulatedClick = pluginButtonID - 1;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_runtime.h"

RuntimeScriptValue Sc_PluginStub_Void(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue();
}

RuntimeScriptValue Sc_PluginStub_Int0(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue().SetInt32(0);
}

RuntimeScriptValue Sc_PluginStub_IntNeg1(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue().SetInt32(-1);
}

// Stub file format defines the text file consisting of any number of lines.
// Every line declares a stub function and its return value; the return value
// may be either numeric value or 'nil' that defines either null-pointer or
// 'void', depending on how the function is used by script interpreter.
// If no value is provided, 'nil' is used by default.
//
//     function_name [numeric_literal | nil]
//
// Numeric value is first tested as integer, then as float, therefore to force
// floating-point type of value one should always include the decimal
// separator, e.g. write "1." or "1.0" instead of just "1".
//
bool ReadStubsFromFile(const char* name)
{
    // Read the requested stub from the text file
    String stub_filename = String::FromFormat("%s.stub", name);
    Stream *stub_in = AssetManager::OpenAsset(stub_filename);
    if (!stub_in) {
        Out::FPrint("Stub file not found: %s", stub_filename.GetCStr());
        return false;
    }

    Out::FPrint("Stub file found: %s", stub_filename.GetCStr());
    TextStreamReader reader(stub_in);
    while (!reader.EOS())
    {
        String line = reader.ReadLine();
        line.Trim();
        if (line.IsEmpty())
            continue;
        String function_name = line.LeftSection(' ');
        String return_value  = line.Section(' ', 1, 1);

        RuntimeScriptValue rval;
        if (!return_value.IsEmpty() && return_value.CompareNoCase("nil") != 0)
        {
            int32_t ival;
            StrUtil::ConversionError err = StrUtil::StringToInt(return_value, ival, 0);
            if (err != StrUtil::kFailed)
            {
                if (err == StrUtil::kOutOfRange)
                    Out::FPrint("WARNING: stub declaration %s has integer return value that is too large: %s", function_name.GetCStr(), return_value.GetCStr());
                rval.SetInt32(ival);
            }
            else
            {
                float fval;
                err = StrUtil::StringToFloat(return_value, fval, 0.f);
                if (err != StrUtil::kFailed)
                {
                    if (err == StrUtil::kOutOfRange)
                        Out::FPrint("WARNING: stub declaration %s has floating-point return value that is too large: %s", function_name.GetCStr(), return_value.GetCStr());
                    rval.SetFloat(fval);
                }
                else
                    Out::FPrint("WARNING: stub declaration %s has invalid return value: %s", function_name.GetCStr(), return_value.GetCStr());
            }
        }
        rval.SetPluginArgument(rval.IValue);
        ccAddExternalScriptSymbol(function_name, rval, NULL);
    }
    return true;
}

bool RegisterPluginStubs(const char* name)
{
  if (ReadStubsFromFile(name))
    return true;

  // Stubs for plugin functions.

  if (strncmp(name, "ags_shell", strlen("ags_shell")) == 0)
  {
    // ags_shell.dll
    ccAddExternalStaticFunction("ShellExecute",                 Sc_PluginStub_Void);
    return true;
  }
  else if (strncmp(name, "ags_snowrain", strlen("ags_snowrain")) == 0)
  {
    // ags_snowrain.dll
    ccAddExternalStaticFunction("srSetSnowDriftRange",          Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowDriftSpeed",          Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowFallSpeed",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srChangeSnowAmount",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowBaseline",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowTransparency",        Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowDefaultView",         Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowWindSpeed",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowAmount",              Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowView",                Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srChangeRainAmount",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainView",                Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainDefaultView",         Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainTransparency",        Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainWindSpeed",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainBaseline",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainAmount",              Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainFallSpeed",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetWindSpeed",               Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetBaseline",                Sc_PluginStub_Void);
    return true;
  }
  else if (strncmp(name, "agsjoy", strlen("agsjoy")) == 0)
  {
    // agsjoy.dll
    ccAddExternalStaticFunction("JoystickCount",                Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::Open^1",             Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::IsButtonDown^1",     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::EnableEvents^1",     Sc_PluginStub_Void);
    ccAddExternalStaticFunction("Joystick::DisableEvents^0",    Sc_PluginStub_Void);
    ccAddExternalStaticFunction("Joystick::Click^1",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("Joystick::Valid^0",            Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::Unplugged^0",        Sc_PluginStub_Int0);
    return true;
  }
  else if (strncmp(name, "agsblend", strlen("agsblend")) == 0)
  {
    // agsblend.dll
    ccAddExternalStaticFunction("DrawAlpha",                    Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetAlpha",                     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("PutAlpha",                     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Blur",                         Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("HighPass",                     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("DrawAdd",                      Sc_PluginStub_Int0);
    return true;
  }
  else if (strncmp(name, "agsflashlight", strlen("agsflashlight")) == 0)
  {
    // agsflashlight.dll
    ccAddExternalStaticFunction("SetFlashlightTint",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightTintRed",         Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightTintGreen",       Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightTintBlue",        Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightMinLightLevel",   Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightMaxLightLevel",   Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightDarkness",        Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightDarkness",        Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightDarknessSize",    Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightDarknessSize",    Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightBrightness",      Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightBrightness",      Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightBrightnessSize",  Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightBrightnessSize",  Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightPosition",        Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightPositionX",       Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightPositionY",       Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightFollowMouse",     Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightFollowMouse",     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightFollowCharacter", Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightFollowCharacter", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightCharacterDX",     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightCharacterDY",     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightCharacterHorz",   Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightCharacterVert",   Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightMask",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightMask",            Sc_PluginStub_Int0);
    return true;
  }
  else if (strncmp(name, "agswadjetutil", strlen("agswadjetutil")) == 0)
  {
    // agswadjetutil.dll
    ccAddExternalStaticFunction("IsOnPhone",                    Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("FakeKeypress",                 Sc_PluginStub_Void);
    ccAddExternalStaticFunction("IosSetAchievementValue",       Sc_PluginStub_Void);
    ccAddExternalStaticFunction("IosGetAchievementValue",       Sc_PluginStub_IntNeg1);
    ccAddExternalStaticFunction("IosShowAchievements",          Sc_PluginStub_Void);
    ccAddExternalStaticFunction("IosResetAchievements",         Sc_PluginStub_Void);
    return true;
  }

  return false;
}
