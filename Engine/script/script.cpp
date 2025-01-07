//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdio.h>
#include <string.h>
#include "script/script.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/dialog.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_character.h"
#include "ac/global_dialog.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_hotspot.h"
#include "ac/global_object.h"
#include "ac/global_room.h"
#include "ac/global_video.h"
#include "ac/invwindow.h"
#include "ac/mouse.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "script/cc_common.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "main/game_run.h"
#include "script/script_runtime.h"
#include "util/string_compat.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern int gameHasBeenRestored, displayed_room;
extern unsigned int load_new_game;
extern RoomObject*objs;
extern CharacterInfo*playerchar;
extern bool logScriptTOC;

ExecutingScript scripts[MAX_SCRIPT_AT_ONCE];
ExecutingScript *curscript = nullptr; // non-owning ptr

PScript gamescript;
PScript dialogScriptsScript;
UInstance gameinst;
UInstance roominst;
UInstance dialogScriptsInst;
UInstance gameinstFork;
UInstance roominstFork;

int num_scripts=0;
int post_script_cleanup_stack = 0;

int inside_script=0,in_graph_script=0;
int no_blocking_functions = 0; // set to 1 while in rep_Exec_always

NonBlockingScriptFunction repExecAlways(REP_EXEC_ALWAYS_NAME, 0);
NonBlockingScriptFunction lateRepExecAlways(LATE_REP_EXEC_ALWAYS_NAME, 0);
NonBlockingScriptFunction getDialogOptionsDimensionsFunc("dialog_options_get_dimensions", 1);
NonBlockingScriptFunction renderDialogOptionsFunc("dialog_options_render", 1);
NonBlockingScriptFunction getDialogOptionUnderCursorFunc("dialog_options_get_active", 1);
NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc("dialog_options_mouse_click", 4);
NonBlockingScriptFunction runDialogOptionKeyPressHandlerFunc("dialog_options_key_press", 3);
NonBlockingScriptFunction runDialogOptionTextInputHandlerFunc("dialog_options_text_input", 2);
NonBlockingScriptFunction runDialogOptionRepExecFunc("dialog_options_repexec", 1);
NonBlockingScriptFunction runDialogOptionCloseFunc("dialog_options_close", 1);

ScriptSystem scsystem;

std::vector<PScript> scriptModules;
std::vector<UInstance> moduleInst;
std::vector<UInstance> moduleInstFork;
std::vector<RuntimeScriptValue> moduleRepExecAddr;
size_t numScriptModules = 0;


static bool DoRunScriptFuncCantBlock(ccInstance *sci, NonBlockingScriptFunction* funcToRun, bool hasTheFunc);


void run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun) {

    update_script_mouse_coords();

    int room_changes_was = play.room_changes;
    funcToRun->AtLeastOneImplementationExists = false;

    // run modules
    // modules need a forkedinst for this to work
    for (size_t i = 0; i < numScriptModules; ++i) {
        funcToRun->ModuleHasFunction[i] = DoRunScriptFuncCantBlock(moduleInstFork[i].get(), funcToRun, funcToRun->ModuleHasFunction[i]);

        if (room_changes_was != play.room_changes)
            return;
    }

    funcToRun->GlobalScriptHasFunction = DoRunScriptFuncCantBlock(gameinstFork.get(), funcToRun, funcToRun->GlobalScriptHasFunction);

    if (room_changes_was != play.room_changes)
        return;

    funcToRun->RoomHasFunction = DoRunScriptFuncCantBlock(roominstFork.get(), funcToRun, funcToRun->RoomHasFunction);
}

// Returns 0 normally, or -1 to indicate that the event has
// become invalid and another interaction should not be run
// (eg. a room change occured)
    // FIXME: refactor this nonsense! check_interaction_only here is used as
    // both function argument and return value!

// Returns 0 normally, or -1 to indicate that the event has
// become invalid and another handler should not be run
// (eg. a room change occured)
int run_interaction_script(const ObjectEvent &obj_evt, const InteractionEvents *nint, int evnt, int chkAny)
{
    if (evnt < 0 || static_cast<size_t>(evnt) >= nint->Events.size() ||
            !nint->Events[evnt].IsEnabled())
    {
        // No response enabled for this event
        // If there is a response for "Any Click", then abort now so as to
        // run that instead
        if (chkAny >= 0 && nint->Events[chkAny].IsEnabled())
            return 0;

        // Otherwise, run unhandled_event
        run_unhandled_event(obj_evt, evnt);
        return 0;
    }

    // FIXME: refactor this nonsense! check_interaction_only here is used as
    // both function argument and return value!
    if (play.check_interaction_only) {
        play.check_interaction_only = 2; // CHECKME: wth is "2"?
        return -1;
    }

    const int room_was = play.room_changes;

    QueueScriptFunction(obj_evt.ScType, ScriptFunctionRef(nint->ScriptModule, nint->Events[evnt].FunctionName),
        obj_evt.ParamCount, obj_evt.Params, nint->Events[evnt].Enabled);

    // if the room changed within the action
    if (room_was != play.room_changes)
        return -1;
    return 0;
}

void setup_builtin_type_aliases()
{
    // TODO: some sort of a manager or library that can return all builtin types??
    const char *typenames[] = {
        "AudioChannel", "AudioClip", "GUI", "GUIControl", "Hotspot", "Inventory", "Object",
        "Region", "WalkableArea", "Walkbehind", "Camera", "DateTime", "DialogOptionsRendering",
        "Dictionary", "DrawingSurface", "DynamicSprite", "File", "Joystick", "Overlay",
        "MaskPathfinder", "Set", "String", "VideoPlayer", "ViewFrame", "Viewport", 
        // FIXME: these will require distinct dynamic managers, derived from GUIControl's manager!
        "Button", "Label", "InvWindow", "Slider", "TextBox", "ListBox", "TextWindowGUI",
        nullptr
    };

    std::vector<String> aliases;
    for (const char **t = typenames; *t; ++t)
        aliases.push_back(*t);
    ccInstance::AddGlobalTypeAliases(aliases);
}

int create_global_script() {
    constexpr int kscript_create_error = -3; // FIXME: use global script error code

    ccSetOption(SCOPT_AUTOIMPORT, 1);

    // NOTE: this function assumes that the module lists have their elements preallocated!

    std::vector<ccInstance*> all_insts; // gather all to resolve exports below
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        moduleInst[i] = ccInstance::CreateFromScript(scriptModules[i]);
        if (!moduleInst[i])
            return kscript_create_error;
        all_insts.push_back(moduleInst[i].get()); // this is only for temp reference
    }

    gameinst = ccInstance::CreateFromScript(gamescript);
    if (!gameinst)
        return kscript_create_error;
    all_insts.push_back(gameinst.get()); // this is only for temp reference

    if (dialogScriptsScript)
    {
        dialogScriptsInst = ccInstance::CreateFromScript(dialogScriptsScript);
        if (!dialogScriptsInst)
            return kscript_create_error;
        all_insts.push_back(dialogScriptsInst.get()); // this is only for temp reference
    }

    // Resolve the script imports after all the scripts have been loaded 
    for (auto &inst : all_insts)
    {
        if (!inst->ResolveScriptImports())
            return kscript_create_error;
        if (!inst->ResolveImportFixups())
            return kscript_create_error;
    }

    // Create the forks for 'repeatedly_execute_always' after resolving
    // because they copy their respective originals including the resolve information
    for (size_t module_idx = 0; module_idx < numScriptModules; module_idx++)
    {
        auto fork = moduleInst[module_idx]->Fork();
        if (!fork)
            return kscript_create_error;

        moduleInstFork[module_idx] = std::move(fork);
        moduleRepExecAddr[module_idx] = moduleInst[module_idx]->GetSymbolAddress(REP_EXEC_NAME);
    }

    gameinstFork = gameinst->Fork();
    if (gameinstFork == nullptr)
        return kscript_create_error;

    ccSetOption(SCOPT_AUTOIMPORT, 0);

    // Register built-in types under simple aliases for dynamic cast feature
    // TODO: maybe find a better place to do this? should be done after JointRTTI is created though
    setup_builtin_type_aliases();

    // Optionally dump script's TOC into the log
    if (logScriptTOC)
    {
        for (const auto &inst : all_insts)
        {
            if (inst->GetScript()->sctoc)
                Debug::Printf(PrintScriptTOC(*inst->GetScript()->sctoc, inst->GetScript()->sectionNames[0].c_str()));
        }
    }

    return 0;
}

void cancel_all_scripts()
{
    for (int i = 0; i < num_scripts; ++i)
    {
        auto &sc = scripts[i];
        if (sc.Inst)
        {
            (sc.ForkedInst) ?
                sc.Inst->AbortAndDestroy() :
                sc.Inst->Abort();
        }
        sc = {}; // FIXME: store in vector and erase?
    }
    num_scripts = 0;
    // in case the script is running on non-blocking thread (rep-exec-always etc)
    auto inst = ccInstance::GetCurrentInstance();
    if (inst)
        inst->Abort();
}

ccInstance *GetScriptInstanceByType(ScriptType sc_type)
{
    if (sc_type == kScTypeGame)
        return gameinst.get();
    else if (sc_type == kScTypeRoom)
        return roominst.get();
    return nullptr;
}

bool DoesScriptFunctionExist(ccInstance *sci, const String &fn_name)
{
    return sci->GetSymbolAddress(fn_name).Type == kScValCodePtr;
}

bool DoesScriptFunctionExistInModules(const String &fn_name)
{
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        if (DoesScriptFunctionExist(moduleInst[i].get(), fn_name))
            return true;
    }
    return DoesScriptFunctionExist(gameinst.get(), fn_name);
}

// Reports a warning in case a requested event handler function was not run
// TODO: find a way to pass event's name too, but they are not clearly defined as strings inside the engine atm...
static void WarnEventFunctionNotFound(const ScriptFunctionRef &fn_ref, bool in_room)
{
    String message = String::FromFormat("WARNING: requested event handler function '%s' not found",
            fn_ref.FuncName.GetCStr());
    if (in_room)
        message.AppendFmt(" (Room %d)", displayed_room);
    else
        message.AppendFmt(" (%s)", fn_ref.ModuleName.GetCStr());
    debug_script_warn("%s", message.GetCStr());
}

void QueueScriptFunction(ScriptType sc_type, const String &fn_name,
    size_t param_count, const RuntimeScriptValue *params, std::weak_ptr<bool> result)
{
    QueueScriptFunction(sc_type, ScriptFunctionRef(fn_name), param_count, params, result);
}

void QueueScriptFunction(ScriptType sc_type, const ScriptFunctionRef &fn_ref,
    size_t param_count, const RuntimeScriptValue *params, std::weak_ptr<bool> result)
{
    if (inside_script)
    {
        // queue the script for the run after current script is finished
        curscript->RunAnother(sc_type, fn_ref, param_count, params, result);
    }
    else
    {
        // if no script is currently running, run the requested script right away
        bool res = RunScriptFunctionAuto(sc_type, fn_ref, param_count, params);
        auto recv_result = result.lock();
        if (recv_result)
        {
            if (!res)
                WarnEventFunctionNotFound(fn_ref, sc_type == kScTypeRoom);
            *recv_result = res;
        }
    }
}

static bool DoRunScriptFuncCantBlock(ccInstance *sci, NonBlockingScriptFunction* funcToRun, bool hasTheFunc)
{
    if (!hasTheFunc)
        return(false);

    no_blocking_functions++;
    ccInstError result = sci->CallScriptFunction(funcToRun->FunctionName, funcToRun->ParamCount, funcToRun->Params);

    if (result == kInstErr_FuncNotFound)
    {
        // the function doens't exist, so don't try and run it again
        hasTheFunc = false;
    }
    else if ((result != kInstErr_None) && (result != kInstErr_Aborted))
    {
        quit_with_script_error(funcToRun->FunctionName);
    }
    else
    {
        funcToRun->AtLeastOneImplementationExists = true;
    }
    // this might be nested, so don't disrupt blocked scripts
    cc_clear_error();
    no_blocking_functions--;
    return(hasTheFunc);
}

static RunScFuncResult PrepareTextScript(ccInstance *sci, const String &tsname)
{
    assert(sci);
    cc_clear_error();
    if (!DoesScriptFunctionExist(sci, tsname))
    {
        cc_error("no such function in script");
        return kScFnRes_NotFound;
    }
    if (sci->IsBeingRun())
    {
        cc_error("script is already in execution");
        return kScFnRes_ScriptBusy;
    }
    ExecutingScript exscript;
    exscript.Inst = sci;
    scripts[num_scripts] = std::move(exscript);
    curscript = &scripts[num_scripts];
    num_scripts++;
    if (num_scripts >= MAX_SCRIPT_AT_ONCE)
        quit("too many nested text script instances created");
    update_script_mouse_coords();
    inside_script++;
    return kScFnRes_Done;
}

RunScFuncResult RunScriptFunction(ccInstance *sci, const String &tsname, size_t numParam, const RuntimeScriptValue *params)
{
    assert(sci);
    int oldRestoreCount = gameHasBeenRestored;
    // TODO: research why this is really necessary, and refactor to avoid such hacks!
    // First, save the current ccError state
    // This is necessary because we might be attempting
    // to run Script B, while Script A is still running in the
    // background.
    // If CallInstance here has an error, it would otherwise
    // also abort Script A because ccError is a global variable.
    ScriptError cachedCcError = cc_get_error();

    const RunScFuncResult res = PrepareTextScript(sci, tsname);
    if (res != kScFnRes_Done)
    {
        if (res != kScFnRes_NotFound)
            quit_with_script_error(tsname);
        cc_error(cachedCcError); // restore cached error state
        return res;
    }

    const ccInstError inst_ret = curscript->Inst->CallScriptFunction(tsname, numParam, params);
    if ((inst_ret != kInstErr_None) && (inst_ret != kInstErr_FuncNotFound) && (inst_ret != kInstErr_Aborted))
    {
        quit_with_script_error(tsname);
    }

    post_script_cleanup_stack++;

    if (post_script_cleanup_stack > 50)
        quitprintf("!post_script_cleanup call stack exceeded: possible recursive function call? running %s", tsname.GetCStr());

    post_script_cleanup();

    post_script_cleanup_stack--;

    // restore cached error state
    cc_error(cachedCcError);

    // if the game has been restored, ensure that any further scripts are not run
    if ((oldRestoreCount != gameHasBeenRestored) && (eventClaimed == EVENT_INPROGRESS))
        eventClaimed = EVENT_CLAIMED;

    // Convert any instance exec error into RunScriptFunction result;
    // NOTE: only kInstErr_FuncNotFound and kInstErr_Aborted can reach here
    if (inst_ret == kInstErr_FuncNotFound)
        return kScFnRes_NotFound;
    return kScFnRes_Done;
}

bool RunScriptFunctionInModules(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    bool result = false;
    for (size_t i = 0; i < numScriptModules; ++i)
        result |= RunScriptFunction(moduleInst[i].get(), tsname, param_count, params) == kScFnRes_Done;
    result |= RunScriptFunction(gameinst.get(), tsname, param_count, params) == kScFnRes_Done;
    return result;
}

bool RunScriptFunctionInRoom(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    if (!roominst)
        return false; // room is not loaded yet

    return RunScriptFunction(roominst.get(), tsname, param_count, params) == kScFnRes_Done;
}

// Run non-claimable event in all script modules, *excluding* room;
// break if certain changes occured to the game state
static bool RunEventInModules(const String &tsname, size_t param_count, const RuntimeScriptValue *params,
    bool break_after_first)
{
    const int room_changes_was = play.room_changes;
    const int restore_game_count_was = gameHasBeenRestored;
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        const RunScFuncResult ret = RunScriptFunction(moduleInst[i].get(), tsname, param_count, params);
        if (ret != kScFnRes_NotFound)
        {
            // Break on room change or save restoration,
            // or if was told to break after the first found callback,
            // or if script execution error occured
            if ((ret != kScFnRes_Done) ||
                break_after_first ||
                (room_changes_was != play.room_changes) ||
                (restore_game_count_was != gameHasBeenRestored))
                return ret == kScFnRes_Done; // return if succeeded to run
        }
    }
    // Try global script last
    return RunScriptFunction(gameinst.get(), tsname, param_count, params) == kScFnRes_Done;
}

// Run non-claimable event in all script modules, *excluding* room;
// break if certain changes occured to the game state
static bool RunUnclaimableEvent(const String &tsname)
{
    return RunEventInModules(tsname, 0, nullptr, false);
}

// Run a single event callback, look for it in all script modules, *excluding* room;
// break after the first run callback, or in case of certain changes to the game state
static bool RunSingleEvent(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    return RunEventInModules(tsname, param_count, params, true);
}

// Run a single event callback in the specified script module;
// if the name is not provided, then tries to run it in global script.
static bool RunEventInModule(const ScriptFunctionRef &fn_ref, size_t param_count, const RuntimeScriptValue *params)
{
    if (!fn_ref.ModuleName.IsEmpty())
    {
        for (size_t i = 0; i < numScriptModules; ++i)
        {
            if (fn_ref.ModuleName.Compare(moduleInst[i]->GetScript()->GetScriptName()) == 0)
            {
                return RunScriptFunction(moduleInst[i].get(), fn_ref.FuncName, param_count, params) == kScFnRes_Done;
            }
        }
    }
    // Try global script last, for backwards compatibility
    return RunScriptFunction(gameinst.get(), fn_ref.FuncName, param_count, params) == kScFnRes_Done;
}

// Run claimable event in all script modules, *including* room;
// break if event was claimed by any of the run callbacks.
// CHECKME: should not this also break on room change / save restore, like RunUnclaimableEvent?
static bool RunClaimableEvent(const String &tsname, size_t param_count, const RuntimeScriptValue *params)
{
    // Run claimable event chain in script modules and room script
    bool eventWasClaimed;
    run_claimable_event(tsname, true, param_count, params, &eventWasClaimed);
    // Break on event claim
    if (eventWasClaimed)
        return true; // suppose if claimed then some function ran successfully
    return RunScriptFunction(gameinst.get(), tsname, param_count, params) == kScFnRes_Done;
}

bool RunScriptFunctionAuto(ScriptType sc_type, const ScriptFunctionRef &fn_ref, size_t param_count, const RuntimeScriptValue *params)
{
    // If told to use a room instance, then run only there
    if (sc_type == kScTypeRoom)
    {
        return RunScriptFunctionInRoom(fn_ref.FuncName, param_count, params);
    }
    // Rep-exec is only run in script modules, but not room script
    // (because room script has its own callback, attached to event slot)
    const String &fn_name = fn_ref.FuncName;
    if (strcmp(fn_name.GetCStr(), REP_EXEC_NAME) == 0)
    {
        return RunUnclaimableEvent(REP_EXEC_NAME);
    }
    // Claimable event is run in all the script modules and room script,
    // before running in the globalscript instance
    // FIXME: make this condition a callback parameter?
    if ((strcmp(fn_name.GetCStr(), ScriptEventCb[kTS_KeyPress].FnName) == 0) || (strcmp(fn_name.GetCStr(), ScriptEventCb[kTS_MouseClick].FnName) == 0) ||
        (strcmp(fn_name.GetCStr(), ScriptEventCb[kTS_TextInput].FnName) == 0) || (strcmp(fn_name.GetCStr(), "on_event") == 0))
    {
        return RunClaimableEvent(fn_name, param_count, params);
    }

    // Else run this event in script modules (except room) according to the function ref
    return RunEventInModule(fn_ref, param_count, params);
}

void AllocScriptModules()
{
    // NOTE: this preallocation possibly required to safeguard some algorithms
    moduleInst.resize(numScriptModules);
    moduleInstFork.resize(numScriptModules);
    moduleRepExecAddr.resize(numScriptModules);
    repExecAlways.ModuleHasFunction.resize(numScriptModules, true);
    lateRepExecAlways.ModuleHasFunction.resize(numScriptModules, true);
    getDialogOptionsDimensionsFunc.ModuleHasFunction.resize(numScriptModules, true);
    renderDialogOptionsFunc.ModuleHasFunction.resize(numScriptModules, true);
    getDialogOptionUnderCursorFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionMouseClickHandlerFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionKeyPressHandlerFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionTextInputHandlerFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionRepExecFunc.ModuleHasFunction.resize(numScriptModules, true);
    runDialogOptionCloseFunc.ModuleHasFunction.resize(numScriptModules, true);
    for (auto &val : moduleRepExecAddr)
    {
        val.Invalidate();
    }
}

void FreeAllScriptInstances()
{
    ccInstance::FreeInstanceStack();
    FreeRoomScriptInstance();

    // NOTE: don't know why, but Forks must be deleted prior to primary inst,
    // or bad things will happen; TODO: investigate and make this less fragile
    gameinstFork.reset();
    gameinst.reset();
    dialogScriptsInst.reset();
    moduleInstFork.clear();
    moduleInst.clear();
}

void FreeRoomScriptInstance()
{
    // NOTE: don't know why, but Forks must be deleted prior to primary inst,
    // or bad things will happen; TODO: investigate and make this less fragile
    roominstFork.reset();
    roominst.reset();
}

void FreeGlobalScripts()
{
    numScriptModules = 0;

    gamescript.reset();
    scriptModules.clear();
    dialogScriptsScript.reset();

    repExecAlways.ModuleHasFunction.clear();
    lateRepExecAlways.ModuleHasFunction.clear();
    getDialogOptionsDimensionsFunc.ModuleHasFunction.clear();
    renderDialogOptionsFunc.ModuleHasFunction.clear();
    getDialogOptionUnderCursorFunc.ModuleHasFunction.clear();
    runDialogOptionMouseClickHandlerFunc.ModuleHasFunction.clear();
    runDialogOptionKeyPressHandlerFunc.ModuleHasFunction.clear();
    runDialogOptionTextInputHandlerFunc.ModuleHasFunction.clear();
    runDialogOptionRepExecFunc.ModuleHasFunction.clear();
    runDialogOptionCloseFunc.ModuleHasFunction.clear();
}

//=============================================================================

String make_interact_func_name(const String &base, int param, int subd)
{
    // This will result in a name like "baseX_Y",
    // where 'X' is defined by "param" and 'Y' defined by "subd"
    String fname = String::FromFormat(base.GetCStr(), param);
    fname.AppendFmt("_%c", 'a' + subd);
    return fname;
}

void post_script_cleanup()
{
    // should do any post-script stuff here, like go to new room
    if (cc_has_error())
        quit(cc_get_error().ErrorString);

    ExecutingScript copyof;
    if (num_scripts > 0)
    { // save until the end of function
        copyof = std::move(scripts[num_scripts - 1]);
        copyof.ForkedInst.reset(); // don't need it further
        num_scripts--; // FIXME: store in vector and erase?
    }
    inside_script--;

    if (num_scripts > 0)
        curscript = &scripts[num_scripts-1];
    else {
        curscript = nullptr;
    }

    int old_room_number = displayed_room;

    // FIXME: sync audio in case any screen changing or time-consuming post-script actions were scheduled
    if (copyof.PostScriptActions.size() > 0) {
        sync_audio_playback();
    }

    // run the queued post-script actions
    for (const auto &act : copyof.PostScriptActions)
    {
        const int data1 = act.Data[0];

        switch (act.Type)
        {
        case ePSANewRoom:
            // only change rooms when all scripts are done
            if (num_scripts == 0) {
                new_room(data1, playerchar);
                // don't allow any pending room scripts from the old room
                // in run_another to be executed
                return;
            }
            else
                curscript->QueueAction(PostScriptAction(ePSANewRoom, data1, "NewRoom"));
            break;
        case ePSARestoreGame:
            cancel_all_scripts();
            try_restore_save(data1);
            return;
        case ePSARestoreGameDialog:
            restore_game_dialog2(data1 & 0xFFFF, (data1 >> 16));
            return;
        case ePSARunAGSGame:
            cancel_all_scripts();
            load_new_game = data1;
            return;
        case ePSARunDialog:
            if (is_in_dialog())
            {
                set_dialog_result_goto(data1);
            }
            else
            {
                do_conversation(data1);
            }
            break;
        case ePSAStopDialog:
            set_dialog_result_stop();
            break;
        case ePSARestartGame:
            cancel_all_scripts();
            restart_game();
            return;
        case ePSASaveGame:
            save_game(data1, act.Text, std::move(act.Image));
            break;
        case ePSASaveGameDialog:
            save_game_dialog2(data1 & 0xFFFF, (data1 >> 16));
            break;
        case ePSAScanSaves:
            prescan_save_slots(act.Data[0], act.Data[1], act.Data[2], act.Data[3], act.Data[4], act.Data[5]);
            break;
        default:
            quitprintf("undefined post script action found: %d", act.Type);
        }

        // if the room changed in a conversation, for example, abort
        if (old_room_number != displayed_room) {
            return;
        }
    }


    if (copyof.PostScriptActions.size() > 0) {
        sync_audio_playback();
    }

    for (const auto &script : copyof.ScFnQueue)
    {
        old_room_number = displayed_room;

        bool res = RunScriptFunctionAuto(script.ScType, script.Function, script.ParamCount, script.Params);
        auto recv_result = script.Result.lock();
        if (recv_result)
        {
            if (!res)
                WarnEventFunctionNotFound(script.Function, script.ScType == kScTypeRoom);
            *recv_result = res;
        }

        // FIXME: this is some bogus hack for "on_call" event handler
        // don't use instance + param count, instead find a way to save actual callback name!
        if (script.ScType == kScTypeRoom && script.ParamCount == 1)
        {
            play.roomscript_finished = 1;
        }

        // if they've changed rooms, cancel any further pending scripts
        if ((displayed_room != old_room_number) || (load_new_game))
            break;
    }

}

void quit_with_script_error(const String &fn_name)
{
    // TODO: clean up the error reporting logic. Now engine will append call
    // stack info in quit_check_for_error_state() but only in case of explicit
    // script error ("!" type), and not in other case.
    const auto &error = cc_get_error();
    if (error.IsUserError)
        quitprintf("!Error running function '%s':\n%s", fn_name.GetCStr(), error.ErrorString.GetCStr());
    else
        quitprintf("Error running function '%s':\n%s\n\n%s", fn_name.GetCStr(),
            error.ErrorString.GetCStr(), error.CallStack.GetCStr());
}

struct TempEip {
    int oldval;
    TempEip (int newval) {
        oldval = get_our_eip();
        set_our_eip(newval);
    }
    ~TempEip () { set_our_eip(oldval); }
};

// check and abort game if the script is currently
// inside the rep_exec_always function
void can_run_delayed_command() {
  if (no_blocking_functions)
    quit("!This command cannot be used within non-blocking events such as " REP_EXEC_ALWAYS_NAME);
}

void run_unhandled_event(const ObjectEvent &obj_evt, int evnt) {

    if (play.check_interaction_only)
        return;

    const char *evblockbasename = obj_evt.BlockName.GetCStr();
    const int evblocknum = obj_evt.BlockID;
    int evtype=0;

    if (ags_strnicmp(evblockbasename,"hotspot",7)==0) evtype=1;
    else if (ags_strnicmp(evblockbasename,"object",6)==0) evtype=2;
    else if (ags_strnicmp(evblockbasename,"character",9)==0) evtype=3;
    else if (ags_strnicmp(evblockbasename,"inventory",9)==0) evtype=5;
    else if (ags_strnicmp(evblockbasename,"region",6)==0)
        return;  // no unhandled_events for regions

    // clicked Hotspot 0, so change the type code
    if ((evtype == 1) & (evblocknum == 0) & (evnt != 0) & (evnt != 5) & (evnt != 6))
        evtype = 4;
    if ((evtype==1) & ((evnt==0) | (evnt==5) | (evnt==6)))
        ;  // character stands on hotspot, mouse moves over hotspot, any click
    else if ((evtype==2) & (evnt==4)) ;  // any click on object
    else if ((evtype==3) & (evnt==4)) ;  // any click on character
    else if (evtype > 0) {
        can_run_delayed_command();
        RuntimeScriptValue params[] = { evtype, evnt };
        QueueScriptFunction(kScTypeGame, "unhandled_event", 2, params);
    }
}

ExecutingScript *get_executingscript()
{
    return curscript;
}

bool get_script_position(ScriptPosition &script_pos)
{
    ccInstance *cur_instance = ccInstance::GetCurrentInstance();
    if (cur_instance)
    {
        cur_instance->GetScriptPosition(script_pos);
        return true;
    }
    return false;
}

String cc_format_error(const String &message)
{
    if (currentline > 0)
        return String::FromFormat("Error (line %d): %s", currentline, message.GetCStr());
    else
        return String::FromFormat("Error (line unknown): %s", message.GetCStr());
}
