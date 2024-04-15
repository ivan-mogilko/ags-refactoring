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

#include <string.h>
#include "ac/global_plugin.h"
#include "ac/mouse.h"

int pluginSimulatedClick = NONE;

void PluginSimulateMouseClick(int pluginButtonID) {
    pluginSimulatedClick = pluginButtonID - 1;
}

//=============================================================================
// Stubs for plugin functions.

void ScriptStub_ShellExecute()
{
}
void srSetSnowDriftRange(int min_value, int max_value)
{
}
void srSetSnowDriftSpeed(int min_value, int max_value)
{
}
void srSetSnowFallSpeed(int min_value, int max_value)
{
}
void srChangeSnowAmount(int amount)
{
}
void srSetSnowBaseline(int top, int bottom)
{
}
void srSetSnowTransparency(int min_value, int max_value)
{
}
void srSetSnowDefaultView(int view, int loop)
{
}
void srSetSnowWindSpeed(int value)
{
}
void srSetSnowAmount(int amount)
{
}
void srSetSnowView(int kind_id, int event, int view, int loop)
{
}
void srChangeRainAmount(int amount)
{
}
void srSetRainView(int kind_id, int event, int view, int loop)
{
}
void srSetRainDefaultView(int view, int loop)
{
}
void srSetRainTransparency(int min_value, int max_value)
{
}
void srSetRainWindSpeed(int value)
{
}
void srSetRainBaseline(int top, int bottom)
{
}
void srSetRainAmount(int amount)
{
}
void srSetRainFallSpeed(int min_value, int max_value)
{
}
void srSetWindSpeed(int value)
{
}
void srSetBaseline(int top, int bottom)
{
}
int JoystickCount()
{
    return 0;
}
int Joystick_Open(int a)
{
    return 0;
}
int Joystick_IsButtonDown(int a)
{
    return 0;
}
void Joystick_EnableEvents(int a)
{
}
void Joystick_DisableEvents()
{
}
void Joystick_Click(int a)
{
}
int Joystick_Valid()
{
    return 0;
}
int Joystick_Unplugged()
{
    return 0;
}
int DrawAlpha(int destination, int sprite, int x, int y, int transparency)
{
    return 0;
}
int GetAlpha(int sprite, int x, int y)
{
    return 0;
}
int PutAlpha(int sprite, int x, int y, int alpha)
{
    return 0;
}
int Blur(int sprite, int radius)
{
    return 0;
}
int HighPass(int sprite, int threshold)
{
    return 0;
}
int DrawAdd(int destination, int sprite, int x, int y, float scale)
{
    return 0;
}

int GetFlashlightInt()
{
    return 0;
}
void SetFlashlightInt1(int Param1)
{
}
void SetFlashlightInt2(int Param1, int Param2)
{
}
void SetFlashlightInt3(int Param1, int Param2, int Param3)
{
}
void SetFlashlightInt5(int Param1, int Param2, int Param3, int Param4, int Param5)
{
}

bool wjuIsOnPhone()
{
  return false;
}
void wjuFakeKeypress(int keypress)
{
}
void wjuIosSetAchievementValue(char* name, int value)
{
}
int wjuIosGetAchievementValue(char* name)
{
  return -1;
}
void wjuIosShowAchievements()
{
}
void wjuIosResetAchievements()
{
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// void ()
RuntimeScriptValue Sc_ScriptStub_ShellExecute(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(ScriptStub_ShellExecute);
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetSnowDriftRange(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowDriftRange);
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetSnowDriftSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowDriftSpeed);
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetSnowFallSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowFallSpeed);
}

// void (int amount)
RuntimeScriptValue Sc_srChangeSnowAmount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srChangeSnowAmount);
}

// void (int top, int bottom)
RuntimeScriptValue Sc_srSetSnowBaseline(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowBaseline);
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetSnowTransparency(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowTransparency);
}

// void (int view, int loop)
RuntimeScriptValue Sc_srSetSnowDefaultView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowDefaultView);
}

// void (int value)
RuntimeScriptValue Sc_srSetSnowWindSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetSnowWindSpeed);
}

// void (int amount)
RuntimeScriptValue Sc_srSetSnowAmount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetSnowAmount);
}

// void (int kind_id, int event, int view, int loop)
RuntimeScriptValue Sc_srSetSnowView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(srSetSnowView);
}

// void (int amount)
RuntimeScriptValue Sc_srChangeRainAmount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srChangeRainAmount);
}

// void (int kind_id, int event, int view, int loop)
RuntimeScriptValue Sc_srSetRainView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(srSetRainView);
}

// void (int view, int loop)
RuntimeScriptValue Sc_srSetRainDefaultView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetRainDefaultView);
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetRainTransparency(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetRainTransparency);
}

// void (int value)
RuntimeScriptValue Sc_srSetRainWindSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetRainWindSpeed);
}

// void (int top, int bottom)
RuntimeScriptValue Sc_srSetRainBaseline(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetRainBaseline);
}

// void (int amount)
RuntimeScriptValue Sc_srSetRainAmount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetRainAmount);
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetRainFallSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetRainFallSpeed);
}

// void (int value)
RuntimeScriptValue Sc_srSetWindSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetWindSpeed);
}

// void (int top, int bottom)
RuntimeScriptValue Sc_srSetBaseline(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetBaseline);
}

// int ()
RuntimeScriptValue Sc_JoystickCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(JoystickCount);
}

// int (int a)
RuntimeScriptValue Sc_Joystick_Open(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Joystick_Open);
}

// int (int a)
RuntimeScriptValue Sc_Joystick_IsButtonDown(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Joystick_IsButtonDown);
}

// void (int a)
RuntimeScriptValue Sc_Joystick_EnableEvents(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Joystick_EnableEvents);
}

// void ()
RuntimeScriptValue Sc_Joystick_DisableEvents(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Joystick_DisableEvents);
}

// void (int a)
RuntimeScriptValue Sc_Joystick_Click(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Joystick_Click);
}

// int ()
RuntimeScriptValue Sc_Joystick_Valid(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Joystick_Valid);
}

// int ()
RuntimeScriptValue Sc_Joystick_Unplugged(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Joystick_Unplugged);
}

// int (int destination, int sprite, int x, int y, int transparency)
RuntimeScriptValue Sc_DrawAlpha(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT5(DrawAlpha);
}

// int (int sprite, int x, int y)
RuntimeScriptValue Sc_GetAlpha(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(GetAlpha);
}

// int (int sprite, int x, int y, int alpha)
RuntimeScriptValue Sc_PutAlpha(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT4(PutAlpha);
}

// int (int sprite, int radius)
RuntimeScriptValue Sc_Blur(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Blur);
}

// int (int sprite, int threshold)
RuntimeScriptValue Sc_HighPass(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(HighPass);
}

// int (int destination, int sprite, int x, int y, float scale)
RuntimeScriptValue Sc_DrawAdd(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT4_PFLOAT(DrawAdd);
}

// int ()
RuntimeScriptValue Sc_GetFlashlightInt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetFlashlightInt);
}

// void (int Param1)
RuntimeScriptValue Sc_SetFlashlightInt1(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetFlashlightInt1);
}

// void (int Param1, int Param2)
RuntimeScriptValue Sc_SetFlashlightInt2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetFlashlightInt2);
}

// void (int Param1, int Param2, int Param3)
RuntimeScriptValue Sc_SetFlashlightInt3(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetFlashlightInt3);
}

// void (int Param1, int Param2, int Param3, int Param4, int Param5)
RuntimeScriptValue Sc_SetFlashlightInt5(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(SetFlashlightInt5);
}

// bool ()
RuntimeScriptValue Sc_wjuIsOnPhone(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(wjuIsOnPhone);
}

// void (int)
RuntimeScriptValue Sc_wjuFakeKeypress(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(wjuFakeKeypress);
}

// void (char*, int)
RuntimeScriptValue Sc_wjuIosSetAchievementValue(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT(wjuIosSetAchievementValue, char);
}

// int (char*)
RuntimeScriptValue Sc_wjuIosGetAchievementValue(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(wjuIosGetAchievementValue, char);
}

// void ()
RuntimeScriptValue Sc_wjuIosShowAchievements(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(wjuIosShowAchievements);
}

// void ()
RuntimeScriptValue Sc_wjuIosResetAchievements(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(wjuIosResetAchievements);
}


RuntimeScriptValue Sc_PluginStub_Void(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue().SetInt32(0);
}

RuntimeScriptValue Sc_PluginStub_Int0(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue().SetInt32(0);
}

RuntimeScriptValue Sc_PluginStub_IntNeg1(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue().SetInt32(-1);
}

RuntimeScriptValue Sc_PluginStub_NullStr(const RuntimeScriptValue *params, int32_t param_count)
{
	return RuntimeScriptValue().SetStringLiteral(NULL);
}


bool RegisterPluginStubs(const char* name)
{
  // Stubs for plugin functions.

  bool is_agsteam = (strncmp(name, "agsteam", strlen("agsteam")) == 0) || (strncmp(name, "agsteam-unified", strlen("agsteam-unified")) == 0) ||
    (strncmp(name, "agsteam-disjoint", strlen("agsteam-disjoint")) == 0);
  bool is_agsgalaxy = (strncmp(name, "agsgalaxy", strlen("agsgalaxy")) == 0) || (strncmp(name, "agsgalaxy-unified", strlen("agsgalaxy-unified")) == 0) ||
    (strncmp(name, "agsgalaxy-disjoint", strlen("agsgalaxy-disjoint")) == 0);

  if (strncmp(name, "ags_shell", strlen("ags_shell")) == 0)
  {
    // ags_shell.dll
    ccAddExternalStaticFunction("ShellExecute",                 Sc_ScriptStub_ShellExecute);
    return true;
  }
  else if (strncmp(name, "ags_snowrain", strlen("ags_snowrain")) == 0)
  {
    // ags_snowrain.dll
    ccAddExternalStaticFunction("srSetSnowDriftRange",          Sc_srSetSnowDriftRange);
    ccAddExternalStaticFunction("srSetSnowDriftSpeed",          Sc_srSetSnowDriftSpeed);
    ccAddExternalStaticFunction("srSetSnowFallSpeed",           Sc_srSetSnowFallSpeed);
    ccAddExternalStaticFunction("srChangeSnowAmount",           Sc_srChangeSnowAmount);
    ccAddExternalStaticFunction("srSetSnowBaseline",            Sc_srSetSnowBaseline);
    ccAddExternalStaticFunction("srSetSnowTransparency",        Sc_srSetSnowTransparency);
    ccAddExternalStaticFunction("srSetSnowDefaultView",         Sc_srSetSnowDefaultView);
    ccAddExternalStaticFunction("srSetSnowWindSpeed",           Sc_srSetSnowWindSpeed);
    ccAddExternalStaticFunction("srSetSnowAmount",              Sc_srSetSnowAmount);
    ccAddExternalStaticFunction("srSetSnowView",                Sc_srSetSnowView);
    ccAddExternalStaticFunction("srChangeRainAmount",           Sc_srChangeRainAmount);
    ccAddExternalStaticFunction("srSetRainView",                Sc_srSetRainView);
    ccAddExternalStaticFunction("srSetRainDefaultView",         Sc_srSetRainDefaultView);
    ccAddExternalStaticFunction("srSetRainTransparency",        Sc_srSetRainTransparency);
    ccAddExternalStaticFunction("srSetRainWindSpeed",           Sc_srSetRainWindSpeed);
    ccAddExternalStaticFunction("srSetRainBaseline",            Sc_srSetRainBaseline);
    ccAddExternalStaticFunction("srSetRainAmount",              Sc_srSetRainAmount);
    ccAddExternalStaticFunction("srSetRainFallSpeed",           Sc_srSetRainFallSpeed);
    ccAddExternalStaticFunction("srSetWindSpeed",               Sc_srSetWindSpeed);
    ccAddExternalStaticFunction("srSetBaseline",                Sc_srSetBaseline);
    return true;
  }
  else if (strncmp(name, "agsjoy", strlen("agsjoy")) == 0)
  {
    // agsjoy.dll
    ccAddExternalStaticFunction("JoystickCount",                Sc_JoystickCount);
    ccAddExternalStaticFunction("Joystick::Open^1",             Sc_Joystick_Open);
    ccAddExternalStaticFunction("Joystick::IsButtonDown^1",     Sc_Joystick_IsButtonDown);
    ccAddExternalStaticFunction("Joystick::EnableEvents^1",     Sc_Joystick_EnableEvents);
    ccAddExternalStaticFunction("Joystick::DisableEvents^0",    Sc_Joystick_DisableEvents);
    ccAddExternalStaticFunction("Joystick::Click^1",            Sc_Joystick_Click);
    ccAddExternalStaticFunction("Joystick::Valid^0",            Sc_Joystick_Valid);
    ccAddExternalStaticFunction("Joystick::Unplugged^0",        Sc_Joystick_Unplugged);
    return true;
  }
  else if (strncmp(name, "agsblend", strlen("agsblend")) == 0)
  {
    // agsblend.dll
    ccAddExternalStaticFunction("DrawAlpha",                    Sc_DrawAlpha);
    ccAddExternalStaticFunction("GetAlpha",                     Sc_GetAlpha);
    ccAddExternalStaticFunction("PutAlpha",                     Sc_PutAlpha);
    ccAddExternalStaticFunction("Blur",                         Sc_Blur);
    ccAddExternalStaticFunction("HighPass",                     Sc_HighPass);
    ccAddExternalStaticFunction("DrawAdd",                      Sc_DrawAdd);
    return true;
  }
  else if (strncmp(name, "agsflashlight", strlen("agsflashlight")) == 0)
  {
    // agsflashlight.dll
    ccAddExternalStaticFunction("SetFlashlightTint",            Sc_SetFlashlightInt3);
    ccAddExternalStaticFunction("GetFlashlightTintRed",         Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightTintGreen",       Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightTintBlue",        Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightMinLightLevel",   Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightMaxLightLevel",   Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("SetFlashlightDarkness",        Sc_SetFlashlightInt1);
    ccAddExternalStaticFunction("GetFlashlightDarkness",        Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("SetFlashlightDarknessSize",    Sc_SetFlashlightInt1);
    ccAddExternalStaticFunction("GetFlashlightDarknessSize",    Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("SetFlashlightBrightness",      Sc_SetFlashlightInt1);
    ccAddExternalStaticFunction("GetFlashlightBrightness",      Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("SetFlashlightBrightnessSize",  Sc_SetFlashlightInt1);
    ccAddExternalStaticFunction("GetFlashlightBrightnessSize",  Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("SetFlashlightPosition",        Sc_SetFlashlightInt2);
    ccAddExternalStaticFunction("GetFlashlightPositionX",       Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightPositionY",       Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("SetFlashlightFollowMouse",     Sc_SetFlashlightInt1);
    ccAddExternalStaticFunction("GetFlashlightFollowMouse",     Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("SetFlashlightFollowCharacter", Sc_SetFlashlightInt5);
    ccAddExternalStaticFunction("GetFlashlightFollowCharacter", Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightCharacterDX",     Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightCharacterDY",     Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightCharacterHorz",   Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("GetFlashlightCharacterVert",   Sc_GetFlashlightInt);
    ccAddExternalStaticFunction("SetFlashlightMask",            Sc_SetFlashlightInt1);
    ccAddExternalStaticFunction("GetFlashlightMask",            Sc_GetFlashlightInt);
    return true;
  }
  else if (strncmp(name, "agswadjetutil", strlen("agswadjetutil")) == 0)
  {
    // agswadjetutil.dll
    ccAddExternalStaticFunction("IsOnPhone",                    Sc_wjuIsOnPhone);
    ccAddExternalStaticFunction("FakeKeypress",                 Sc_wjuFakeKeypress);
    ccAddExternalStaticFunction("IosSetAchievementValue",       Sc_wjuIosSetAchievementValue);
    ccAddExternalStaticFunction("IosGetAchievementValue",       Sc_wjuIosGetAchievementValue);
    ccAddExternalStaticFunction("IosShowAchievements",          Sc_wjuIosShowAchievements);
    ccAddExternalStaticFunction("IosResetAchievements",         Sc_wjuIosResetAchievements);
    return true;
  }
  else if (is_agsteam || is_agsgalaxy)
  {
    // agsteam.dll or agsgalaxy.dll
    ccAddExternalStaticFunction("AGS2Client::IsAchievementAchieved^1", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::SetAchievementAchieved^1", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::ResetAchievement^1", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::GetIntStat^1", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::GetFloatStat^1", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::GetAverageRateStat^1", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::SetIntStat^2", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::SetFloatStat^2", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::UpdateAverageRateStat^3", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::ResetStatsAndAchievements^0", Sc_PluginStub_Void);
    ccAddExternalStaticFunction("AGS2Client::get_Initialized", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::get_CurrentLeaderboardName", Sc_PluginStub_NullStr);
    ccAddExternalStaticFunction("AGS2Client::RequestLeaderboard^3", Sc_PluginStub_Void);
    ccAddExternalStaticFunction("AGS2Client::UploadScore^1", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::geti_LeaderboardNames", Sc_PluginStub_NullStr);
    ccAddExternalStaticFunction("AGS2Client::geti_LeaderboardScores", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::get_LeaderboardCount", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("AGS2Client::GetUserName^0", Sc_PluginStub_NullStr);
    ccAddExternalStaticFunction("AGS2Client::GetCurrentGameLanguage^0", Sc_PluginStub_NullStr);
    ccAddExternalStaticFunction("AGS2Client::FindLeaderboard^1", Sc_PluginStub_Void);
    ccAddExternalStaticFunction("AGS2Client::Initialize^2", Sc_PluginStub_Int0);
    if (is_agsteam)
    {
      ccAddExternalStaticFunction("Steam::AddAchievement^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("Steam::AddStat^2", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("Steam::GetIntStat^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("Steam::GetFloatStat^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("Steam::SetIntStat^2", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("Steam::SetFloatStat^2", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("Steam::ResetAchievements^0", Sc_PluginStub_Void);
      ccAddExternalStaticFunction("Steam::ResetStats^0", Sc_PluginStub_Void);
      ccAddExternalStaticFunction("Steam::IsAchievementAchieved^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("Steam::SetAchievementAchieved^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("Steam::ResetStatsAndAchievements^0", Sc_PluginStub_Void);
	    
      ccAddExternalStaticFunction("AGSteam::IsAchievementAchieved^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::SetAchievementAchieved^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::ResetAchievement^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::GetIntStat^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::GetFloatStat^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::GetAverageRateStat^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::SetIntStat^2", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::SetFloatStat^2", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::UpdateAverageRateStat^3", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::ResetStatsAndAchievements^0", Sc_PluginStub_Void);
      ccAddExternalStaticFunction("AGSteam::get_Initialized", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::get_CurrentLeaderboardName", Sc_PluginStub_NullStr);
      ccAddExternalStaticFunction("AGSteam::RequestLeaderboard^3", Sc_PluginStub_Void);
      ccAddExternalStaticFunction("AGSteam::UploadScore^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::geti_LeaderboardNames", Sc_PluginStub_NullStr);
      ccAddExternalStaticFunction("AGSteam::geti_LeaderboardScores", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::get_LeaderboardCount", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSteam::GetUserName^0", Sc_PluginStub_NullStr);
      ccAddExternalStaticFunction("AGSteam::GetCurrentGameLanguage^0", Sc_PluginStub_NullStr);
      ccAddExternalStaticFunction("AGSteam::FindLeaderboard^1", Sc_PluginStub_Void);
    }
    else // agsgalaxy
    {
      ccAddExternalStaticFunction("AGSGalaxy::IsAchievementAchieved^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::SetAchievementAchieved^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::ResetAchievement^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::GetIntStat^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::GetFloatStat^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::GetAverageRateStat^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::SetIntStat^2", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::SetFloatStat^2", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::UpdateAverageRateStat^3", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::ResetStatsAndAchievements^0", Sc_PluginStub_Void);
      ccAddExternalStaticFunction("AGSGalaxy::get_Initialized", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::get_CurrentLeaderboardName", Sc_PluginStub_NullStr);
      ccAddExternalStaticFunction("AGSGalaxy::RequestLeaderboard^3", Sc_PluginStub_Void);
      ccAddExternalStaticFunction("AGSGalaxy::UploadScore^1", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::geti_LeaderboardNames", Sc_PluginStub_NullStr);
      ccAddExternalStaticFunction("AGSGalaxy::geti_LeaderboardScores", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::get_LeaderboardCount", Sc_PluginStub_Int0);
      ccAddExternalStaticFunction("AGSGalaxy::GetUserName^0", Sc_PluginStub_NullStr);
      ccAddExternalStaticFunction("AGSGalaxy::GetCurrentGameLanguage^0", Sc_PluginStub_NullStr);
      ccAddExternalStaticFunction("AGSGalaxy::Initialize^2", Sc_PluginStub_Int0);
    }
    return true;
  }
  return false;
}
