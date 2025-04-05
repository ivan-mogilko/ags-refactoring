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
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <vector>
#include "ac/gui.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_screen.h"
#include "ac/guicontrol.h"
#include "ac/inventoryitem.h"
#include "ac/invwindow.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/properties.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/dynobj/cc_gui.h"
#include "ac/dynobj/cc_guicontrol.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "device/mousew32.h"
#include "gfx/gfxfilter.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"
#include "util/geometry.h"
#include "util/string_compat.h"


using namespace AGS::Common;
using namespace AGS::Engine;


extern RoomStruct thisroom;
extern int cur_mode,cur_cursor;
extern std::vector<ScriptGUI> scrGui;
std::vector<std::vector<int>> StaticGUIControlsHandles;
extern GameSetupStruct game;
extern IGraphicsDriver *gfxDriver;

extern CCGUI ccDynamicGUI;
extern CCGUIControl ccDynamicGUIControl;
extern CCGUIButton ccDynamicGUIButton;
extern CCGUIInvWindow ccDynamicGUIInvWindow;
extern CCGUILabel ccDynamicGUILabel;
extern CCGUIListBox ccDynamicGUIListBox;
extern CCGUISlider ccDynamicGUISlider;
extern CCGUITextBox ccDynamicGUITextBox;


int ifacepopped=-1;  // currently displayed pop-up GUI (-1 if none)
int mouse_on_iface=-1;   // mouse cursor is over this interface
// cursor position relative to a focused gui control
int mouse_ifacebut_xoffs =- 1, mouse_ifacebut_yoffs =- 1;

int eip_guinum, eip_guiobj;


namespace AGS
{
namespace Engine
{
namespace GUIE
{

void MarkAllGUIForUpdate(bool redraw, bool reset_over_ctrl)
{
    for (auto &gui : guis)
    {
        if (redraw)
        {
            gui.MarkChanged();
            for (int i = 0; i < gui.GetControlCount(); ++i)
                gui.GetControl(i)->MarkChanged();
        }
        if (reset_over_ctrl)
            gui.ResetOverControl();
    }
}

void MarkForTranslationUpdate()
{
    for (auto &btn : guibuts)
    {
        if (btn.IsTranslated())
            btn.MarkChanged();
    }
    for (auto &lbl : guilabels)
    {
        if (lbl.IsTranslated())
            lbl.MarkChanged();
    }
    for (auto &list : guilist)
    {
        if (list.IsTranslated())
            list.MarkChanged();
    }
}

void MarkForFontUpdate(int font)
{
    const bool update_all = (font < 0);
    for (auto &btn : guibuts)
    {
        if (update_all || btn.Font == font)
            btn.OnResized();
    }
    for (auto &lbl : guilabels)
    {
        if (update_all || lbl.Font == font)
            lbl.OnResized();
    }
    for (auto &list : guilist)
    {
        if (update_all || list.Font == font)
            list.OnResized();
    }
    for (auto &tb : guitext)
    {
        if (update_all || tb.Font == font)
            tb.OnResized();
    }
}

void MarkSpecialLabelsForUpdate(GUILabelMacro macro)
{
    for (auto &lbl : guilabels)
    {
        if ((lbl.GetTextMacros() & macro) != 0)
        {
            lbl.MarkChanged();
        }
    }
}

void MarkInventoryForUpdate(int char_id, bool is_player)
{
    for (auto &btn : guibuts)
    {
        if (btn.GetPlaceholder() != kButtonPlace_None)
            btn.MarkChanged();
    }
    for (auto &inv : guiinv)
    {
        if ((char_id < 0) || (inv.CharId == char_id) || (is_player && inv.CharId < 0))
        {
            inv.MarkChanged();
        }
    }
}

} // namespace GUI
} // namespace Engine
} // namespace AGS


ScriptGUI* GUI_AsTextWindow(ScriptGUI *tehgui)
{ // Internally both GUI and TextWindow are implemented by same class
    return guis[tehgui->id].IsTextWindow() ? &scrGui[tehgui->id] : nullptr;
}

int GUI_GetPopupStyle(ScriptGUI *tehgui)
{
    return guis[tehgui->id].PopupStyle;
}

void GUI_SetVisible(ScriptGUI *tehgui, int isvisible) {
  if (isvisible)
    InterfaceOn(tehgui->id);
  else
    InterfaceOff(tehgui->id);
}

int GUI_GetVisible(ScriptGUI *tehgui) {
  return (guis[tehgui->id].IsVisible()) ? 1 : 0;
}

bool GUI_GetShown(ScriptGUI *tehgui) {
    return guis[tehgui->id].IsDisplayed();
}

int GUI_GetX(ScriptGUI *tehgui) {
  return guis[tehgui->id].X;
}

void GUI_SetX(ScriptGUI *tehgui, int x) {
  guis[tehgui->id].SetAt(x, guis[tehgui->id].Y);
}

int GUI_GetY(ScriptGUI *tehgui) {
  return guis[tehgui->id].Y;
}

void GUI_SetY(ScriptGUI *tehgui, int y) {
    guis[tehgui->id].SetAt(guis[tehgui->id].X, y);
}

void GUI_SetPosition(ScriptGUI *tehgui, int x, int y) {
    guis[tehgui->id].SetAt(x, y);
}

void GUI_SetSize(ScriptGUI *sgui, int w, int h) {
  if ((w < 1) || (h < 1))
    quitprintf("!SetGUISize: invalid dimensions (tried to set to %d x %d)", w, h);

  GUIMain *tehgui = &guis[sgui->id];

  if ((tehgui->Width == w) && (tehgui->Height == h))
    return;

  tehgui->SetSize(w, h);
  tehgui->MarkChanged();
}

int GUI_GetWidth(ScriptGUI *sgui) {
  return guis[sgui->id].Width;
}

int GUI_GetHeight(ScriptGUI *sgui) {
  return guis[sgui->id].Height;
}

void GUI_SetWidth(ScriptGUI *sgui, int newwid) {
  GUI_SetSize(sgui, newwid, GUI_GetHeight(sgui));
}

void GUI_SetHeight(ScriptGUI *sgui, int newhit) {
  GUI_SetSize(sgui, GUI_GetWidth(sgui), newhit);
}

void GUI_SetZOrder(ScriptGUI *tehgui, int z) {
  guis[tehgui->id].ZOrder = z;
  update_gui_zorder();
}

int GUI_GetZOrder(ScriptGUI *tehgui) {
  return guis[tehgui->id].ZOrder;
}

void GUI_SetClickable(ScriptGUI *tehgui, int clickable) {
  guis[tehgui->id].SetClickable(clickable != 0);
}

int GUI_GetClickable(ScriptGUI *tehgui) {
  return guis[tehgui->id].IsClickable() ? 1 : 0;
}

int GUI_GetID(ScriptGUI *tehgui) {
  return tehgui->id;
}

const char *GUI_GetScriptName(ScriptGUI *tehgui)
{
    return CreateNewScriptString(guis[tehgui->id].Name);
}

GUIObject* GUI_GetiControls(ScriptGUI *tehgui, int idx) {
  if ((idx < 0) || (idx >= guis[tehgui->id].GetControlCount()))
    return nullptr;
  return guis[tehgui->id].GetControl(idx);
}

int GUI_GetControlCount(ScriptGUI *tehgui) {
  return guis[tehgui->id].GetControlCount();
}

int GUI_GetPopupYPos(ScriptGUI *tehgui)
{
    return guis[tehgui->id].PopupAtMouseY;
}

void GUI_SetPopupYPos(ScriptGUI *tehgui, int newpos)
{
    if (!guis[tehgui->id].IsTextWindow())
        guis[tehgui->id].PopupAtMouseY = newpos;
}

void GUI_SetTransparency(ScriptGUI *tehgui, int trans) {
  if ((trans < 0) | (trans > 100))
    quit("!SetGUITransparency: transparency value must be between 0 and 100");

  guis[tehgui->id].SetTransparencyAsPercentage(trans);
}

int GUI_GetTransparency(ScriptGUI *tehgui) {
  return GfxDef::LegacyTrans255ToTrans100(guis[tehgui->id].Transparency);
}

void GUI_Centre(ScriptGUI *sgui) {
  GUIMain *tehgui = &guis[sgui->id];
  int x = play.GetUIViewport().GetWidth() / 2 - tehgui->Width / 2;
  int y = play.GetUIViewport().GetHeight() / 2 - tehgui->Height / 2;
  tehgui->SetAt(x, y);
}

void GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn) {
  if (guis[tehgui->id].BgImage != slotn) {
    guis[tehgui->id].BgImage = slotn;
    guis[tehgui->id].MarkChanged();
  }
}

int GUI_GetBackgroundGraphic(ScriptGUI *tehgui) {
  if (guis[tehgui->id].BgImage < 1)
    return 0;
  return guis[tehgui->id].BgImage;
}

void GUI_SetBackgroundColor(ScriptGUI *tehgui, int newcol)
{
    if (guis[tehgui->id].BgColor != newcol)
    {
        guis[tehgui->id].BgColor = newcol;
        guis[tehgui->id].MarkChanged();
    }
}

int GUI_GetBackgroundColor(ScriptGUI *tehgui)
{
    return guis[tehgui->id].BgColor;
}

void GUI_SetBorderColor(ScriptGUI *tehgui, int newcol)
{
    if (guis[tehgui->id].IsTextWindow())
        return;
    if (guis[tehgui->id].FgColor != newcol)
    {
        guis[tehgui->id].FgColor = newcol;
        guis[tehgui->id].MarkChanged();
    }
}

int GUI_GetBorderColor(ScriptGUI *tehgui)
{
    if (guis[tehgui->id].IsTextWindow())
        return 0;
    return guis[tehgui->id].FgColor;
}

void GUI_SetTextColor(ScriptGUI *tehgui, int newcol)
{
    if (!guis[tehgui->id].IsTextWindow())
        return;
    if (guis[tehgui->id].FgColor != newcol)
    {
        guis[tehgui->id].FgColor = newcol;
        guis[tehgui->id].MarkChanged();
    }
}

int GUI_GetTextColor(ScriptGUI *tehgui)
{
    if (!guis[tehgui->id].IsTextWindow())
        return 0;
    return guis[tehgui->id].FgColor;
}

int GUI_GetTextPadding(ScriptGUI *tehgui)
{
    return guis[tehgui->id].Padding;
}

void GUI_SetTextPadding(ScriptGUI *tehgui, int newpos)
{
    if (guis[tehgui->id].IsTextWindow())
        guis[tehgui->id].Padding = newpos;
}

int GetGUIAt(int xx, int yy) {
    // Test in the opposite order (from closer to further)
    for (auto g = play.gui_draw_order.crbegin(); g < play.gui_draw_order.crend(); ++g) {
        if (guis[*g].IsInteractableAt(xx, yy))
            return *g;
    }
    return -1;
}

ScriptGUI *GUI_GetAtScreenXY(int xx, int yy) {
    int guiid = GetGUIAt(xx, yy);
    if (guiid < 0)
        return nullptr;
    return &scrGui[guiid];
}

void GUI_Click(ScriptGUI *scgui, int mbut)
{
    process_interface_click(scgui->id, -1, mbut);
}

void GUI_ProcessClick(int x, int y, int mbut)
{
    int guiid = gui_get_interactable(x, y);
    if (guiid >= 0)
    { // simulate mouse click at the given coordinates
        guis[guiid].Poll(x, y);
        gui_on_mouse_down(guiid, mbut, x, y);
        gui_on_mouse_up(guiid, mbut, x, y);
    }
}

int GUI_GetBlendMode(ScriptGUI *gui) {
    return guis[gui->id].BlendMode;
}

void GUI_SetBlendMode(ScriptGUI *gui, int blend_mode) {
    guis[gui->id].BlendMode = ValidateBlendMode("GUI.BlendMode", blend_mode);
}

int GUI_GetShader(ScriptGUI *gui)
{
    return guis[gui->id].ShaderID;
}

void GUI_SetShader(ScriptGUI *gui, int shader_id)
{
    guis[gui->id].ShaderID = shader_id;
}

float GUI_GetRotation(ScriptGUI *gui) {
    return guis[gui->id].Rotation;
}

void GUI_SetRotation(ScriptGUI *gui, float rotation) {
    guis[gui->id].SetRotation(rotation);
}

float GUI_GetScaleX(ScriptGUI *gui) {
    return guis[gui->id].GetScale().X;
}

void GUI_SetScaleX(ScriptGUI *gui, float scalex) {
    guis[gui->id].SetScale(scalex, guis[gui->id].GetScale().Y);
}

float GUI_GetScaleY(ScriptGUI *gui) {
    return guis[gui->id].GetScale().Y;
}

void GUI_SetScaleY(ScriptGUI *gui, float scaley) {
    guis[gui->id].SetScale(guis[gui->id].GetScale().X, scaley);
}

void GUI_SetScale(ScriptGUI *gui, float scalex, float scaley) {
    guis[gui->id].SetScale(scalex, scaley);
}

int GUI_GetProperty(ScriptGUI *gui, const char *property)
{
    return get_int_property(game.guiProps[gui->id], play.guiProps[gui->id], property);
}

const char* GUI_GetTextProperty(ScriptGUI *gui, const char *property)
{
    return get_text_property_dynamic_string(game.guiProps[gui->id], play.guiProps[gui->id], property);
}

bool GUI_SetProperty(ScriptGUI *gui, const char *property, int value)
{
    return set_int_property(play.guiProps[gui->id], property, value);
}

bool GUI_SetTextProperty(ScriptGUI *gui, const char *property, const char *value)
{
    return set_text_property(play.guiProps[gui->id], property, value);
}

//=============================================================================

void remove_popup_interface(int ifacenum) {
    if (ifacepopped != ifacenum) return;
    ifacepopped=-1; UnPauseGame();
    guis[ifacenum].SetConceal(true);
    if (mousey<=guis[ifacenum].PopupAtMouseY)
        Mouse::SetPosition(Point(mousex, guis[ifacenum].PopupAtMouseY+2));
    if ((!IsInterfaceEnabled()) && (cur_cursor == cur_mode))
        // Only change the mouse cursor if it hasn't been specifically changed first
        set_mouse_cursor(CURS_WAIT);
    else if (IsInterfaceEnabled())
        set_default_cursor();

    if (ifacenum==mouse_on_iface) mouse_on_iface=-1;
}

void process_interface_click(int ifce, int btn, int mbut) {
    if (btn < 0) {
        // click on GUI background
        RuntimeScriptValue params[]{ RuntimeScriptValue().SetScriptObject(&scrGui[ifce], &ccDynamicGUI),
            RuntimeScriptValue().SetInt32(mbut) };
        QueueScriptFunction(kScTypeGame, ScriptFunctionRef(guis[ifce].ScriptModule, guis[ifce].OnClickHandler), 2, params);
        return;
    }

    int btype = guis[ifce].GetControlType(btn);
    int rtype=kGUIAction_None,rdata=0;
    if (btype==kGUIButton) {
        GUIButton*gbuto=(GUIButton*)guis[ifce].GetControl(btn);
        rtype=gbuto->ClickAction[kGUIClickLeft];
        rdata=gbuto->ClickData[kGUIClickLeft];
    }
    else if ((btype==kGUISlider) || (btype == kGUITextBox) || (btype == kGUIListBox))
        rtype = kGUIAction_RunScript;
    else quit("unknown GUI object triggered process_interface");

    if (rtype==kGUIAction_None) ;
    else if (rtype==kGUIAction_SetMode)
        set_cursor_mode(rdata);
    else if (rtype==kGUIAction_RunScript) {
        GUIObject *theObj = guis[ifce].GetControl(btn);
        // if the object has a special handler script then run it;
        // otherwise, run interface_click
        if ((theObj->GetEventCount() > 0) &&
            (!theObj->EventHandlers[0].IsEmpty()) &&
            DoesScriptFunctionExistInModules(theObj->EventHandlers[0]))
        {
            // control-specific event handler
            const ScriptFunctionRef fn_ref(guis[ifce].ScriptModule, theObj->EventHandlers[0]);
            if (theObj->GetEventArgs(0).FindChar(',') != String::NoIndex)
            {
                RuntimeScriptValue params[]{ RuntimeScriptValue().SetScriptObject(theObj, &ccDynamicGUIControl),
                    RuntimeScriptValue().SetInt32(mbut) };
                QueueScriptFunction(kScTypeGame, fn_ref, 2, params);
            }
            else
            {
                RuntimeScriptValue params[]{ RuntimeScriptValue().SetScriptObject(theObj, &ccDynamicGUIControl) };
                QueueScriptFunction(kScTypeGame, fn_ref, 1, params);
            }
        }
        else
        {
            RuntimeScriptValue params[]{ ifce , btn };
            QueueScriptFunction(kScTypeGame, "interface_click", 2, params);
        }
    }
}


void replace_macro_tokens(const char *text, String &fixed_text) {
    const char*curptr=&text[0];
    char tmpm[3];
    const char*endat = curptr + strlen(text);
    fixed_text.Empty();
    char tempo[STD_BUFFER_SIZE];

    while (1) {
        if (curptr[0]==0) break;
        if (curptr>=endat) break;
        if (curptr[0]=='@') {
            const char *curptrWasAt = curptr;
            char macroname[21]; int idd=0; curptr++;
            for (idd=0;idd<20;idd++) {
                if (curptr[0]=='@') {
                    macroname[idd]=0;
                    curptr++;
                    break;
                }
                // unterminated macro (eg. "@OVERHOTSPOT"), so abort
                if (curptr[0] == 0)
                    break;
                macroname[idd]=curptr[0];
                curptr++;
            }
            macroname[idd]=0;
            tempo[0]=0;
            if (ags_stricmp(macroname,"gamename")==0)
                snprintf(tempo, sizeof(tempo), "%s", play.game_name.GetCStr());
            else if (ags_stricmp(macroname,"overhotspot")==0) {
                // While game is in Wait mode, no overhotspot text
                if (!IsInterfaceEnabled())
                    tempo[0] = 0;
                else
                    GetLocationNameInBuf(mousex, mousey, tempo);
            }
            else { // not a macro, there's just a @ in the message
                curptr = curptrWasAt + 1;
                snprintf(tempo, sizeof(tempo), "%s", "@");
            }

            fixed_text.Append(tempo);
        }
        else {
            tmpm[0]=curptr[0]; tmpm[1]=0;
            fixed_text.Append(tmpm);
            curptr++;
        }
    }
}

bool sort_gui_less(const int g1, const int g2)
{
    return (guis[g1].ZOrder < guis[g2].ZOrder) ||
        ((guis[g1].ZOrder == guis[g2].ZOrder) && (g1 < g2));
}

void update_gui_zorder()
{
    std::sort(play.gui_draw_order.begin(), play.gui_draw_order.end(), sort_gui_less);
}

void prepare_gui_runtime(bool startup)
{
    // Trigger all guis and controls to recalculate their dynamic state;
    // here we achieve this by sending "On Resize" event, although there could
    // be a better way for this.
    for (auto &gui : guis)
    {
        for (int i = 0; i < gui.GetControlCount(); ++i)
        {
            GUIObject *guio = gui.GetControl(i);
            guio->IsActivated = false;
            guio->OnResized();
        }
    }
    // Reset particular states after loading game data
    if (startup)
    {
        // labels are not clickable by default
        // CHECKME: why are we doing this at all?
        for (auto &label : guilabels)
        {
            label.SetClickable(false);
        }
    }
    play.gui_draw_order.resize(guis.size());
    std::iota(play.gui_draw_order.begin(), play.gui_draw_order.end(), 0);
    update_gui_zorder();

    GUI::Options.DisabledStyle = static_cast<GuiDisableStyle>(game.options[OPT_DISABLEOFF]);
    GUIE::MarkAllGUIForUpdate(true, true);
}

void set_array_all_gui_controls_size()
{
    StaticGUIControlsHandles.resize(game.numgui);
    for (int i = 0; i < game.numgui; ++i) {
        auto const &gui = guis[i];
        StaticGUIControlsHandles[i] = std::vector<int>();
        StaticGUIControlsHandles[i].resize(gui.GetControlCount());
    }
}

void export_all_gui_controls()
{
    set_array_all_gui_controls_size();

    for (int i = 0; i < game.numgui; ++i)
    {
        auto const &gui = guis[i];
        for (int j = 0; j < gui.GetControlCount(); j++)
        {
            GUIObject *guio = gui.GetControl(j);
            IScriptObject *mgr;
            switch (gui.GetControlType(j))
            {
            case kGUIButton: mgr = &ccDynamicGUIButton; break;
            case kGUILabel: mgr = &ccDynamicGUILabel; break;
            case kGUIInvWindow: mgr = &ccDynamicGUIInvWindow; break;
            case kGUISlider: mgr = &ccDynamicGUISlider; break;
            case kGUITextBox: mgr = &ccDynamicGUITextBox; break;
            case kGUIListBox: mgr = &ccDynamicGUIListBox; break;
            default: mgr = nullptr; break;
            }

            if (!mgr)
                continue;

            int handle = ccRegisterPersistentObject(guio, mgr); // add ref for engine
            StaticGUIControlsHandles[i][j] = handle;
            if (!guio->Name.IsEmpty())
            {
                ccAddExternalScriptObjectHandle(guio->Name, &StaticGUIControlsHandles[i][j]);
            }
        }
    }
}

void unexport_gui_controls(int ee)
{
    for (int ff = 0; ff < guis[ee].GetControlCount(); ff++)
    {
        GUIObject *guio = guis[ee].GetControl(ff);
        if (!guio->Name.IsEmpty())
            ccRemoveExternalSymbol(guio->Name);
        if (!ccUnRegisterManagedObject(guio))
            quit("unable to unregister guicontrol object");
    }
}

void unexport_all_gui_controls()
{
    for (int i = 0; i < game.numgui; ++i)
    {
        unexport_gui_controls(i);
    }
    StaticGUIControlsHandles.clear();
}

void update_gui_disabled_status()
{
    // update GUI display status (perhaps we've gone into an interface disabled state)
    const GuiDisableStyle disabled_state_was = GUI::Context.DisabledState;
    GUI::Context.DisabledState = IsInterfaceEnabled() ?
        kGuiDis_Undefined : GUI::Options.DisabledStyle;

    if (disabled_state_was != GUI::Context.DisabledState)
    {
        // Mark guis for redraw and reset control-under-mouse detection
        GUIE::MarkAllGUIForUpdate(GUI::Options.DisabledStyle != kGuiDis_Unchanged, true);
        if (GUI::Options.DisabledStyle != kGuiDis_Unchanged)
        {
            invalidate_screen();
        }
    }
}

static bool should_skip_adjust_for_gui(const GUIMain &gui)
{
    return
        // not shown
        !gui.IsDisplayed() ||
        // completely offscreen
        !IsRectInsideRect(play.GetUIViewport(), RectWH(gui.X, gui.Y, gui.Width, gui.Height)) ||
        // fully transparent (? FIXME: this only checks background, but not controls)
        ((gui.BgColor == 0) && (gui.BgImage < 1)) || (gui.Transparency == 255);
}

int adjust_x_for_guis(int x, int y, bool assume_blocking) {
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) &&
        ((GUI::Context.DisabledState != kGuiDis_Undefined) || assume_blocking))
        return x; // All GUI off (or will be when the message is displayed)
    // If it's covered by a GUI, move it right a bit
    for (const auto &gui : guis) {
        if (should_skip_adjust_for_gui(gui))
            continue;
        // higher, lower or to the right from the message (?)
        if ((gui.X > x) || (gui.Y > y) || (gui.Y + gui.Height < y))
            continue;
        // try to deal with full-width GUIs across the top
        // FIXME: using a harcoded width in pixels...
        if (gui.X + gui.Width >= 280)
            continue;
        // Fix coordinates if x is inside the gui
        if (x < gui.X + gui.Width)
            x = gui.X + gui.Width + 2;
    }
    return x;
}

int adjust_y_for_guis(int y, bool assume_blocking) {
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) &&
        ((GUI::Context.DisabledState >= 0) || assume_blocking))
        return y; // All GUI off (or will be when the message is displayed)
    // If it's covered by a GUI, move it down a bit
    for (const auto &gui : guis) {
        if (should_skip_adjust_for_gui(gui))
            continue;
        // lower than the message
        if (gui.Y > y)
            continue;
        // try to deal with full-height GUIs down the left or right
        // FIXME: using a harcoded height in pixels...
        if (gui.Height > 50)
            continue;
        // Fix coordinates if y is inside the gui
        if (y < gui.Y + gui.Height)
            y = gui.Y + gui.Height + 2;
    }
    return y;
}

int gui_get_interactable(int x,int y)
{
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) && (GUI::Context.DisabledState >= 0))
        return -1;
    return GetGUIAt(x, y);
}

int gui_on_mouse_move(const int mx, const int my)
{
    int mouse_over_gui = -1;
    // If all GUIs are off, skip the loop
    if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) && (GUI::Context.DisabledState >= 0)) ;
    else {
        // Scan for mouse-y-pos GUIs, and pop one up if appropriate
        // Also work out the mouse-over GUI while we're at it
        // CHECKME: not sure why, but we're testing forward draw order here -
        // from farthest to nearest (this was in original code?)
        for (int guin : play.gui_draw_order) {
            if (guis[guin].IsInteractableAt(mx, my)) mouse_over_gui=guin;

            if (guis[guin].PopupStyle!=kGUIPopupMouseY) continue;
            if (play.complete_overlay_on > 0) break;  // interfaces disabled
            if (ifacepopped==guin) continue;
            if (!guis[guin].IsVisible()) continue;
            // Don't allow it to be popped up while skipping cutscene
            if (play.fast_forward) continue;

            if (mousey < guis[guin].PopupAtMouseY) {
                set_mouse_cursor(CURS_ARROW);
                guis[guin].SetConceal(false);
                ifacepopped=guin; PauseGame();
                break;
            }
        }
    }
    return mouse_over_gui;
}

// Tells if GUI common controls should react to this mouse button.
// TODO: redesign this all, pass mouse button into GUI OnMouseButtonDown instead,
// handle the button difference there, per control.
inline bool gui_control_should_handle_button(int mbut)
{
    return (game.options[OPT_GUICONTROLMOUSEBUT] == 0) || (mbut == kMouseLeft);
}

void gui_on_mouse_hold(const int wasongui, const int wasbutdown)
{
    for (int i = 0; i < guis[wasongui].GetControlCount(); ++i)
    {
        GUIObject *guio = guis[wasongui].GetControl(i);
        if (!guio->IsActivated)
            continue;
        // We only handle "hold" event for Sliders, and only if mouse button is not restricted
        if (guis[wasongui].GetControlType(i) != kGUISlider)
            continue;
        if (!gui_control_should_handle_button(wasbutdown))
            continue;
        // GUI Slider repeatedly activates while being dragged
        guio->IsActivated = false;
        force_event(AGSEvent_GUI(wasongui, i, static_cast<eAGSMouseButton>(wasbutdown)));
        break;
    }
}

void gui_on_mouse_up(const int wasongui, const int wasbutdown, const int mx, const int my)
{
    GUIMain &gui = guis[wasongui];
    gui.OnMouseButtonUp();

    for (int i = 0; i < gui.GetControlCount(); ++i)
    {
        GUIObject *guio = gui.GetControl(i);
        if (!guio->IsActivated)
            continue;

        guio->IsActivated = false;
        if (!IsInterfaceEnabled())
            break;

        bool click_handled = false;
        int cttype = gui.GetControlType(i);
        if ((cttype == kGUIButton) || (cttype == kGUISlider) || (cttype == kGUIListBox))
        {
            if (gui_control_should_handle_button(wasbutdown))
            {
                click_handled = true;
                force_event(AGSEvent_GUI(wasongui, i, static_cast<eAGSMouseButton>(wasbutdown)));
            }
        }
        else if (cttype == kGUIInvWindow)
        {
            click_handled = true;
            Point guipt = gui.GetGraphicSpace().WorldToLocal(mx, my);
            mouse_ifacebut_xoffs = guipt.X - (guio->X);
            mouse_ifacebut_yoffs = guipt.Y - (guio->Y);
            int iit = offset_over_inv((GUIInvWindow*)guio);
            if (iit >= 0)
            {
                play.used_inv_on = iit;
                if (game.options[OPT_HANDLEINVCLICKS])
                {
                    // Let the script handle the click
                    // LEFTINV is 5, RIGHTINV is 6
                    force_event(AGSEvent_Script(kTS_MouseClick, wasbutdown + 4, mx, my));
                }
                else if (wasbutdown == kMouseRight) // right-click is always Look
                {
                    RunInventoryInteraction(iit, MODE_LOOK);
                }
                else if (cur_mode == MODE_HAND)
                {
                    SetActiveInventory(iit);
                }
                else
                {
                    RunInventoryInteraction(iit, cur_mode);
                }
            }
        }
        else
        {
            quit("clicked on unknown control type");
        }

        // Built-in behavior for PopupAtY guis: hide one if interacted with any control
        if ((gui.PopupStyle == kGUIPopupMouseY) && click_handled)
            remove_popup_interface(wasongui);
        break;
    }

    run_on_event(kScriptEvent_GUIMouseUp, wasongui, wasbutdown, mx - gui.X, my - gui.Y);
}

void gui_on_mouse_down(const int guin, const int mbut, const int mx, const int my)
{
    debug_script_log("Mouse click over GUI %d", guin);

    // TODO: redesign this, pass mouse button into GUI OnMouseButtonDown instead,
    // handle the button difference there, per control.
    int over_ctrl = guis[guin].GetControlUnderMouse();
    if (over_ctrl >= 0)
    {
        // NOTE: we make exception for InvWindow control, as it has a special action for RMB
        if (gui_control_should_handle_button(mbut) ||
            (guis[guin].GetControlType(over_ctrl) == kGUIInvWindow))
        {
            guis[guin].OnMouseButtonDown(mx, my);
        }
    }
    else
    {
        // run GUI click handler if not on any control
        if (!guis[guin].OnClickHandler.IsEmpty())
            force_event(AGSEvent_GUI(guin, -1, static_cast<eAGSMouseButton>(mbut)));
    }

    run_on_event(kScriptEvent_GUIMouseDown, guin, mbut, mx - guis[guin].X, my - guis[guin].Y);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "ac/dynobj/scriptstring.h"
#include "ac/dynobj/scriptuserobject.h"
#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"


ScriptGUI *GUI_GetByName(const char *name)
{
    return static_cast<ScriptGUI*>(ccGetScriptObjectAddress(name, ccDynamicGUI.GetType()));
}

ScriptUserObject *GUI_ScreenToGUIPoint(ScriptGUI *tehgui, int scrx, int scry, bool clipToGUI)
{
    GUIMain &gui = guis[tehgui->id];
    Point pt = gui.GetGraphicSpace().WorldToLocal(scrx, scry);
    if (clipToGUI && !RectWH(gui.X, gui.Y, gui.Width, gui.Height).IsInside(pt))
        return nullptr;
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
}

ScriptUserObject *GUI_GUIToScreenPoint(ScriptGUI *tehgui, int guix, int guiy, bool clipToGUI)
{
    GUIMain &gui = guis[tehgui->id];
    if (clipToGUI && !RectWH(gui.X, gui.Y, gui.Width, gui.Height).IsInside(guix, guiy))
        return nullptr;
    Point pt = gui.GetGraphicSpace().LocalToWorld(guix, guiy);
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
}


RuntimeScriptValue Sc_GUI_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptGUI, ccDynamicGUI, GUI_GetByName, const char);
}

// void GUI_Centre(ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_Centre(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptGUI, GUI_Centre);
}

// ScriptGUI *(int xx, int yy)
RuntimeScriptValue Sc_GUI_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptGUI, ccDynamicGUI, GUI_GetAtScreenXY);
}

// void (ScriptGUI *tehgui, int xx, int yy)
RuntimeScriptValue Sc_GUI_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptGUI, GUI_SetPosition);
}

// void (ScriptGUI *sgui, int widd, int hitt)
RuntimeScriptValue Sc_GUI_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptGUI, GUI_SetSize);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBackgroundGraphic);
}

// void (ScriptGUI *tehgui, int slotn)
RuntimeScriptValue Sc_GUI_SetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBackgroundGraphic);
}

RuntimeScriptValue Sc_GUI_GetBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBackgroundColor);
}

RuntimeScriptValue Sc_GUI_SetBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBackgroundColor);
}

RuntimeScriptValue Sc_GUI_GetBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBorderColor);
}

RuntimeScriptValue Sc_GUI_SetBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBorderColor);
}

RuntimeScriptValue Sc_GUI_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTextColor);
}

RuntimeScriptValue Sc_GUI_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTextColor);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetClickable);
}

// void (ScriptGUI *tehgui, int clickable)
RuntimeScriptValue Sc_GUI_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetClickable);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetControlCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetControlCount);
}

// GUIObject* (ScriptGUI *tehgui, int idx)
RuntimeScriptValue Sc_GUI_GetiControls(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(ScriptGUI, GUIObject, ccDynamicGUIControl, GUI_GetiControls);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetHeight);
}

// void (ScriptGUI *sgui, int newhit)
RuntimeScriptValue Sc_GUI_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetHeight);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetID);
}

RuntimeScriptValue Sc_GUI_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGUI, const char, myScriptStringImpl, GUI_GetScriptName);
}

RuntimeScriptValue Sc_GUI_GetPopupYPos(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetPopupYPos);
}

RuntimeScriptValue Sc_GUI_SetPopupYPos(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetPopupYPos);
}

RuntimeScriptValue Sc_GUI_GetTextPadding(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTextPadding);
}

RuntimeScriptValue Sc_GUI_SetTextPadding(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTextPadding);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTransparency);
}

// void (ScriptGUI *tehgui, int trans)
RuntimeScriptValue Sc_GUI_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTransparency);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetVisible);
}

// void (ScriptGUI *tehgui, int isvisible)
RuntimeScriptValue Sc_GUI_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetVisible);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetWidth);
}

// void (ScriptGUI *sgui, int newwid)
RuntimeScriptValue Sc_GUI_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetWidth);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetX);
}

// void (ScriptGUI *tehgui, int xx)
RuntimeScriptValue Sc_GUI_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetX);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetY);
}

// void (ScriptGUI *tehgui, int yy)
RuntimeScriptValue Sc_GUI_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetY);
}

// int (ScriptGUI *tehgui)
RuntimeScriptValue Sc_GUI_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetZOrder);
}

// void (ScriptGUI *tehgui, int z)
RuntimeScriptValue Sc_GUI_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetZOrder);
}

RuntimeScriptValue Sc_GUI_AsTextWindow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGUI, ScriptGUI, ccDynamicGUI, GUI_AsTextWindow);
}

RuntimeScriptValue Sc_GUI_GetPopupStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetPopupStyle);
}

RuntimeScriptValue Sc_GUI_Click(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_Click);
}

RuntimeScriptValue Sc_GUI_ProcessClick(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(GUI_ProcessClick);
}

RuntimeScriptValue Sc_GUI_GetShown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptGUI, GUI_GetShown);
}

// int (ScriptGUI *gui)
RuntimeScriptValue Sc_GUI_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBlendMode);
}

// void (ScriptGUI *gui, int blendMode)
RuntimeScriptValue Sc_GUI_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBlendMode);
}

RuntimeScriptValue Sc_GUI_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetShader);
}

RuntimeScriptValue Sc_GUI_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetShader);
}

RuntimeScriptValue Sc_GUI_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptGUI, GUI_GetRotation);
}

RuntimeScriptValue Sc_GUI_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptGUI, GUI_SetRotation);
}

RuntimeScriptValue Sc_GUI_GetScaleX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptGUI, GUI_GetScaleX);
}

RuntimeScriptValue Sc_GUI_SetScaleX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptGUI, GUI_SetScaleX);
}

RuntimeScriptValue Sc_GUI_GetScaleY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptGUI, GUI_GetScaleY);
}

RuntimeScriptValue Sc_GUI_SetScaleY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptGUI, GUI_SetScaleY);
}

RuntimeScriptValue Sc_GUI_SetScale(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT2(ScriptGUI, GUI_SetScale);
}

RuntimeScriptValue Sc_GUI_ScreenToGUIPoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptGUI, ScriptUserObject, GUI_ScreenToGUIPoint);
}

RuntimeScriptValue Sc_GUI_GUIToScreenPoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptGUI, ScriptUserObject, GUI_GUIToScreenPoint);
}

RuntimeScriptValue Sc_GUI_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptGUI, GUI_GetProperty, const char);
}

RuntimeScriptValue Sc_GUI_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptGUI, const char, myScriptStringImpl, GUI_GetTextProperty, const char);
}

RuntimeScriptValue Sc_GUI_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptGUI, GUI_SetProperty, const char);
}

RuntimeScriptValue Sc_GUI_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptGUI, GUI_SetTextProperty, const char, const char);
}

void RegisterGUIAPI()
{
    ScFnRegister gui_api[] = {
        { "GUI::GetAtScreenXY^2",         API_FN_PAIR(GUI_GetAtScreenXY) },
        { "GUI::GetByName",               API_FN_PAIR(GUI_GetByName) },
        { "GUI::ProcessClick^3",          API_FN_PAIR(GUI_ProcessClick) },
        { "GUI::ScreenToGUIPoint",        API_FN_PAIR(GUI_ScreenToGUIPoint) },
        { "GUI::GUIToScreenPoint",        API_FN_PAIR(GUI_GUIToScreenPoint) },

        { "GUI::Centre^0",                API_FN_PAIR(GUI_Centre) },
        { "GUI::Click^1",                 API_FN_PAIR(GUI_Click) },
        { "GUI::SetPosition^2",           API_FN_PAIR(GUI_SetPosition) },
        { "GUI::SetSize^2",               API_FN_PAIR(GUI_SetSize) },
        { "GUI::GetProperty^1",             API_FN_PAIR(GUI_GetProperty) },
        { "GUI::GetTextProperty^1",         API_FN_PAIR(GUI_GetTextProperty) },
        { "GUI::SetProperty^2",             API_FN_PAIR(GUI_SetProperty) },
        { "GUI::SetTextProperty^2",         API_FN_PAIR(GUI_SetTextProperty) },
        { "GUI::get_BackgroundGraphic",   API_FN_PAIR(GUI_GetBackgroundGraphic) },
        { "GUI::set_BackgroundGraphic",   API_FN_PAIR(GUI_SetBackgroundGraphic) },
        { "GUI::get_BackgroundColor",     API_FN_PAIR(GUI_GetBackgroundColor) },
        { "GUI::set_BackgroundColor",     API_FN_PAIR(GUI_SetBackgroundColor) },
        { "GUI::get_BorderColor",         API_FN_PAIR(GUI_GetBorderColor) },
        { "GUI::set_BorderColor",         API_FN_PAIR(GUI_SetBorderColor) },
        { "GUI::get_Clickable",           API_FN_PAIR(GUI_GetClickable) },
        { "GUI::set_Clickable",           API_FN_PAIR(GUI_SetClickable) },
        { "GUI::get_ControlCount",        API_FN_PAIR(GUI_GetControlCount) },
        { "GUI::geti_Controls",           API_FN_PAIR(GUI_GetiControls) },
        { "GUI::get_Height",              API_FN_PAIR(GUI_GetHeight) },
        { "GUI::set_Height",              API_FN_PAIR(GUI_SetHeight) },
        { "GUI::get_ID",                  API_FN_PAIR(GUI_GetID) },
        { "GUI::get_AsTextWindow",        API_FN_PAIR(GUI_AsTextWindow) },
        { "GUI::get_PopupStyle",          API_FN_PAIR(GUI_GetPopupStyle) },
        { "GUI::get_PopupYPos",           API_FN_PAIR(GUI_GetPopupYPos) },
        { "GUI::set_PopupYPos",           API_FN_PAIR(GUI_SetPopupYPos) },
        { "GUI::get_ScriptName",          API_FN_PAIR(GUI_GetScriptName) },
        { "TextWindowGUI::get_TextColor", API_FN_PAIR(GUI_GetTextColor) },
        { "TextWindowGUI::set_TextColor", API_FN_PAIR(GUI_SetTextColor) },
        { "TextWindowGUI::get_TextPadding", API_FN_PAIR(GUI_GetTextPadding) },
        { "TextWindowGUI::set_TextPadding", API_FN_PAIR(GUI_SetTextPadding) },
        { "GUI::get_Transparency",        API_FN_PAIR(GUI_GetTransparency) },
        { "GUI::set_Transparency",        API_FN_PAIR(GUI_SetTransparency) },
        { "GUI::get_Visible",             API_FN_PAIR(GUI_GetVisible) },
        { "GUI::set_Visible",             API_FN_PAIR(GUI_SetVisible) },
        { "GUI::get_Width",               API_FN_PAIR(GUI_GetWidth) },
        { "GUI::set_Width",               API_FN_PAIR(GUI_SetWidth) },
        { "GUI::get_X",                   API_FN_PAIR(GUI_GetX) },
        { "GUI::set_X",                   API_FN_PAIR(GUI_SetX) },
        { "GUI::get_Y",                   API_FN_PAIR(GUI_GetY) },
        { "GUI::set_Y",                   API_FN_PAIR(GUI_SetY) },
        { "GUI::get_ZOrder",              API_FN_PAIR(GUI_GetZOrder) },
        { "GUI::set_ZOrder",              API_FN_PAIR(GUI_SetZOrder) },
        { "GUI::get_Shown",               API_FN_PAIR(GUI_GetShown) },

        { "GUI::get_BlendMode",           API_FN_PAIR(GUI_GetBlendMode) },
        { "GUI::set_BlendMode",           API_FN_PAIR(GUI_SetBlendMode) },
        { "GUI::get_Rotation",            API_FN_PAIR(GUI_GetRotation) },
        { "GUI::set_Rotation",            API_FN_PAIR(GUI_SetRotation) },
        { "GUI::get_ScaleX",              API_FN_PAIR(GUI_GetScaleX) },
        { "GUI::set_ScaleX",              API_FN_PAIR(GUI_SetScaleX) },
        { "GUI::get_ScaleY",              API_FN_PAIR(GUI_GetScaleY) },
        { "GUI::set_ScaleY",              API_FN_PAIR(GUI_SetScaleY) },
        { "GUI::SetScale",                API_FN_PAIR(GUI_SetScale) },
        { "GUI::SetScale",                API_FN_PAIR(GUI_SetScale) },

        { "GUI::get_Shader",              API_FN_PAIR(GUI_GetShader) },
        { "GUI::set_Shader",              API_FN_PAIR(GUI_SetShader) },
    };

    ccAddExternalFunctions(gui_api);
}
