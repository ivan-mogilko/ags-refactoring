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
// Engine initialization
//

#include "core/platform.h"

#include <errno.h>
#if AGS_PLATFORM_OS_WINDOWS
#include <process.h>  // _spawnl
#endif

#include "main/mainheader.h"
#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_game.h"
#include "ac/gui.h"
#include "ac/lipsync.h"
#include "ac/objectcache.h"
#include "ac/path_helper.h"
#include "ac/sys_events.h"
#include "ac/roomstatus.h"
#include "ac/speech.h"
#include "ac/spritecache.h"
#include "ac/translation.h"
#include "ac/viewframe.h"
#include "ac/dynobj/scriptobject.h"
#include "ac/dynobj/scriptsystem.h"
#include "core/assetmanager.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "gfx/graphicsdriver.h"
#include "gfx/gfxdriverfactory.h"
#include "gfx/ddb.h"
#include "main/config.h"
#include "main/game_file.h"
#include "main/game_start.h"
#include "main/engine.h"
#include "main/engine_setup.h"
#include "main/graphics_mode.h"
#include "main/main.h"
#include "main/main_allegro.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/util/pe.h"
#include "util/directory.h"
#include "util/error.h"
#include "util/misc.h"
#include "util/path.h"

#include "util/extractor.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern char check_dynamic_sprites_at_exit;
extern int our_eip;
extern volatile char want_exit, abort_engine;
extern bool justRunSetup;
extern GameSetup usetup;
extern GameSetupStruct game;
extern int proper_exit;
extern char pexbuf[STD_BUFFER_SIZE];
extern SpriteCache spriteset;
extern ObjectCache objcache[MAX_ROOM_OBJECTS];
extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern ViewStruct*views;
extern int displayed_room;
extern int eip_guinum;
extern int eip_guiobj;
extern SpeechLipSyncLine *splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;
extern ScriptSystem scsystem;
extern IGraphicsDriver *gfxDriver;
extern Bitmap **actsps;
extern color palette[256];
extern CharacterExtras *charextra;
extern CharacterInfo*playerchar;
extern Bitmap **guibg;
extern IDriverDependantBitmap **guibgbmp;

ResourcePaths ResPaths;

t_engine_pre_init_callback engine_pre_init_callback = nullptr;

#define ALLEGRO_KEYBOARD_HANDLER

bool engine_init_allegro()
{
    Debug::Printf(kDbgMsg_Info, "Initializing allegro");

    our_eip = -199;
    // Initialize allegro
    set_uformat(U_ASCII);
    if (install_allegro(SYSTEM_AUTODETECT, &errno, atexit))
    {
        const char *al_err = get_allegro_error();
        const char *user_hint = platform->GetAllegroFailUserHint();
        platform->DisplayAlert("Unable to initialize Allegro system driver.\n%s\n\n%s",
            al_err[0] ? al_err : "Allegro library provided no further information on the problem.",
            user_hint);
        return false;
    }
    return true;
}

void engine_setup_allegro()
{
    // Setup allegro using constructed config string
    const char *al_config_data = "[mouse]\n"
        "mouse_accel_factor = 0\n";
    override_config_data(al_config_data, ustrsize(al_config_data));
}

void winclosehook() {
  want_exit = 1;
  abort_engine = 1;
  check_dynamic_sprites_at_exit = 0;
}

void engine_setup_window()
{
    Debug::Printf(kDbgMsg_Info, "Setting up window");

    our_eip = -198;
    set_window_title("Adventure Game Studio");
    set_close_button_callback (winclosehook);
    our_eip = -197;

    platform->SetGameWindowIcon();
}

// Fills map with game settings, to e.g. let setup application(s)
// display correct properties to the user
static void fill_game_properties(StringOrderMap &map)
{
    map["title"] = game.gamename;
    map["guid"] = game.guid;
    map["legacy_uniqueid"] = StrUtil::IntToString(game.uniqueid);
    map["legacy_resolution"] = StrUtil::IntToString(game.GetResolutionType());
    map["legacy_letterbox"] = StrUtil::IntToString(game.options[OPT_LETTERBOX]);;
    map["resolution_width"] = StrUtil::IntToString(game.GetDefaultRes().Width);
    map["resolution_height"] = StrUtil::IntToString(game.GetDefaultRes().Height);
    map["resolution_bpp"] = StrUtil::IntToString(game.GetColorDepth());
    map["render_at_screenres"] = StrUtil::IntToString(
        game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_UserDefined ? -1 :
        (game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_Enabled ? 1 : 0));
}

// Starts up setup application, if capable.
// Returns TRUE if should continue running the game, otherwise FALSE.
bool engine_run_setup(const ConfigTree &cfg, int &app_res)
{
    app_res = EXIT_NORMAL;
#if AGS_PLATFORM_OS_WINDOWS
    {
            Debug::Printf(kDbgMsg_Info, "Running Setup");

            ConfigTree cfg_with_meta = cfg;
            fill_game_properties(cfg_with_meta["gameproperties"]);
            ConfigTree cfg_out;
            SetupReturnValue res = platform->RunSetup(cfg_with_meta, cfg_out);
            if (res != kSetup_Cancel)
            {
                String cfg_file = PreparePathForWriting(GetGameUserConfigDir(), DefaultConfigFileName);
                if (cfg_file.IsEmpty())
                {
                    platform->DisplayAlert("Unable to write into directory '%s'.\n%s",
                        GetGameUserConfigDir().FullDir.GetCStr(), platform->GetDiskWriteAccessTroubleshootingText());
                }
                else if (!IniUtil::Merge(cfg_file, cfg_out))
                {
                    platform->DisplayAlert("Unable to write to the configuration file (error code 0x%08X).\n%s",
                        platform->GetLastSystemError(), platform->GetDiskWriteAccessTroubleshootingText());
                }
            }
            if (res != kSetup_RunGame)
                return false;

            // TODO: investigate if the full program restart may (should) be avoided

            // Just re-reading the config file seems to cause a caching
            // problem on Win9x, so let's restart the process.
            allegro_exit();
            char quotedpath[MAX_PATH];
            snprintf(quotedpath, MAX_PATH, "\"%s\"", appPath.GetCStr());
            _spawnl(_P_OVERLAY, appPath, quotedpath, NULL);
    }
#endif
    return true;
}

void engine_force_window()
{
    // Force to run in a window, override the config file
    // TODO: actually overwrite config tree instead
    if (force_window == 1)
    {
        usetup.Screen.DisplayMode.Windowed = true;
        usetup.Screen.DisplayMode.ScreenSize.SizeDef = kScreenDef_ByGameScaling;
    }
    else if (force_window == 2)
    {
        usetup.Screen.DisplayMode.Windowed = false;
        usetup.Screen.DisplayMode.ScreenSize.SizeDef = kScreenDef_MaxDisplay;
    }
}

// Scans given directory for the AGS game config. If such config exists
// and it contains data file name, then returns one.
// Otherwise returns empty string.
static String find_game_data_in_config(const String &path)
{
    // First look for config
    ConfigTree cfg;
    String def_cfg_file = Path::ConcatPaths(path, DefaultConfigFileName);
    if (IniUtil::Read(def_cfg_file, cfg))
    {
        String data_file = INIreadstring(cfg, "misc", "datafile");
        Debug::Printf("Found game config: %s", def_cfg_file.GetCStr());
        Debug::Printf(" Cfg: data file: %s", data_file.GetCStr());
        // Only accept if it's a relative path
        if (!data_file.IsEmpty() && is_relative_filename(data_file))
            return Path::ConcatPaths(path, data_file);
    }
    return ""; // not found in config
}

// Scans for game data in several common locations.
// When it does so, it first looks for game config file, which contains
// explicit directions to game data in its settings.
// If such config is not found, it scans same location for *any* game data instead.
String search_for_game_data_file(String &was_searching_in)
{
    Debug::Printf("Looking for the game data.\n Cwd: %s\n Path arg: %s",
        Directory::GetCurrentDirectory().GetCStr(),
        cmdGameDataPath.GetCStr());
    // 1. From command line argument, which may be a directory or actual file
    if (!cmdGameDataPath.IsEmpty())
    {
        if (Path::IsFile(cmdGameDataPath))
            return cmdGameDataPath; // this path is a file
        if (!Path::IsDirectory(cmdGameDataPath))
            return ""; // path is neither file nor directory
        was_searching_in = cmdGameDataPath;
        Debug::Printf("Searching in (cmd arg): %s", was_searching_in.GetCStr());
        // first scan for config
        String data_path = find_game_data_in_config(cmdGameDataPath);
        if (!data_path.IsEmpty())
            return data_path;
        // if not found in config, lookup for data in same dir
        return FindGameData(cmdGameDataPath);
    }

    // 2. Look in other known locations
    // 2.1. Look for attachment in the running executable
    if (!appPath.IsEmpty() && Common::AssetManager::IsDataFile(appPath))
    {
        Debug::Printf("Found game data embedded in executable");
        was_searching_in = Path::GetDirectoryPath(appPath);
        return appPath;
    }

    // 2.2 Look in current working directory
    String cur_dir = Directory::GetCurrentDirectory();
    was_searching_in = cur_dir;
    Debug::Printf("Searching in (cwd): %s", was_searching_in.GetCStr());
    // first scan for config
    String data_path = find_game_data_in_config(cur_dir);
    if (!data_path.IsEmpty())
        return data_path;
    // if not found in config, lookup for data in same dir
    data_path = FindGameData(cur_dir);
    if (!data_path.IsEmpty())
        return data_path;

    // 2.3 Look in executable's directory (if it's different from current dir)
    if (Path::ComparePaths(appDirectory, cur_dir) == 0)
        return ""; // no luck
    was_searching_in = appDirectory;
    Debug::Printf("Searching in (exe dir): %s", was_searching_in.GetCStr());
    // first scan for config
    data_path = find_game_data_in_config(appDirectory);
    if (!data_path.IsEmpty())
        return data_path;
    // if not found in config, lookup for data in same dir
    return FindGameData(appDirectory);
}

void engine_init_fonts()
{
    Debug::Printf(kDbgMsg_Info, "Initializing TTF renderer");

    init_font_renderer();
}

void engine_init_mouse()
{
    int res = minstalled();
    if (res < 0)
        Debug::Printf(kDbgMsg_Info, "Initializing mouse: failed");
    else
        Debug::Printf(kDbgMsg_Info, "Initializing mouse: number of buttons reported is %d", res);
    Mouse::SetSpeed(usetup.mouse_speed);
}

void engine_locate_speech_pak()
{
    play.want_speech=-2;

    if (!usetup.no_speech_pack) {
        String speech_file = "speech.vox";
        String speech_filepath = find_assetlib(speech_file);
        if (!speech_filepath.IsEmpty()) {
            Debug::Printf("Initializing speech vox");
            if (AssetMgr->AddLibrary(speech_filepath) != Common::kAssetNoError) {
                platform->DisplayAlert("Unable to read voice pack, file could be corrupted or of unknown format.\nSpeech voice-over will be disabled.");
                return;
            }
            // TODO: why is this read right here??? move this to InitGameState!
            Stream *speechsync = AssetMgr->OpenAsset("syncdata.dat");
            if (speechsync != nullptr) {
                // this game has voice lip sync
                int lipsync_fmt = speechsync->ReadInt32();
                if (lipsync_fmt != 4)
                {
                    Debug::Printf(kDbgMsg_Info, "Unknown speech lip sync format (%d).\nLip sync disabled.", lipsync_fmt);
                }
                else {
                    numLipLines = speechsync->ReadInt32();
                    splipsync = (SpeechLipSyncLine*)malloc (sizeof(SpeechLipSyncLine) * numLipLines);
                    for (int ee = 0; ee < numLipLines; ee++)
                    {
                        splipsync[ee].numPhonemes = speechsync->ReadInt16();
                        speechsync->Read(splipsync[ee].filename, 14);
                        splipsync[ee].endtimeoffs = (int*)malloc(splipsync[ee].numPhonemes * sizeof(int));
                        speechsync->ReadArrayOfInt32(splipsync[ee].endtimeoffs, splipsync[ee].numPhonemes);
                        splipsync[ee].frame = (short*)malloc(splipsync[ee].numPhonemes * sizeof(short));
                        speechsync->ReadArrayOfInt16(splipsync[ee].frame, splipsync[ee].numPhonemes);
                    }
                }
                delete speechsync;
            }
            Debug::Printf(kDbgMsg_Info, "Voice pack found and initialized.");
            play.want_speech=1;
        }
        else if (Path::ComparePaths(ResPaths.DataDir, ResPaths.VoiceDir2) != 0)
        {
            // If we have custom voice directory set, we will enable voice-over even if speech.vox does not exist
            Debug::Printf(kDbgMsg_Info, "Voice pack was not found, but explicit voice directory is defined: enabling voice-over.");
            play.want_speech=1;
        }
        ResPaths.SpeechPak.Name = speech_file;
        ResPaths.SpeechPak.Path = speech_filepath;
    }
}

void engine_locate_audio_pak()
{
    play.separate_music_lib = 0;
    String music_file = game.GetAudioVOXName();
    String music_filepath = find_assetlib(music_file);
    if (!music_filepath.IsEmpty())
    {
        if (AssetMgr->AddLibrary(music_filepath) == kAssetNoError)
        {
            Debug::Printf(kDbgMsg_Info, "%s found and initialized.", music_file.GetCStr());
            play.separate_music_lib = 1;
            ResPaths.AudioPak.Name = music_file;
            ResPaths.AudioPak.Path = music_filepath;
        }
        else
        {
            platform->DisplayAlert("Unable to initialize digital audio pack '%s', file could be corrupt or of unsupported format.",
                music_file.GetCStr());
        }
    }
    else if (Path::ComparePaths(ResPaths.DataDir, ResPaths.AudioDir2) != 0)
    {
        Debug::Printf(kDbgMsg_Info, "Audio pack was not found, but explicit audio directory is defined.");
    }
}

// Assign asset locations to the AssetManager
void engine_assign_assetpaths()
{
    AssetMgr->AddLibrary(ResPaths.GamePak.Path, ",audio"); // main pack may have audio bundled too
    // The asset filters are currently a workaround for limiting search to certain locations;
    // this is both an optimization and to prevent unexpected behavior.
    // - empty filter is for regular files
    // audio - audio clips
    // voice - voice-over clips
    // NOTE: we add extra optional directories first because they should have higher priority
    // TODO: maybe change AssetManager library order to stack-like later (last added = top priority)?
    if (!ResPaths.DataDir2.IsEmpty() && Path::ComparePaths(ResPaths.DataDir2, ResPaths.DataDir) != 0)
        AssetMgr->AddLibrary(ResPaths.DataDir2, ",audio,voice"); // dir may have anything
    if (!ResPaths.AudioDir2.IsEmpty() && Path::ComparePaths(ResPaths.AudioDir2, ResPaths.DataDir) != 0)
        AssetMgr->AddLibrary(ResPaths.AudioDir2, "audio");
    if (!ResPaths.VoiceDir2.IsEmpty() && Path::ComparePaths(ResPaths.VoiceDir2, ResPaths.DataDir) != 0)
        AssetMgr->AddLibrary(ResPaths.VoiceDir2, "voice");

    AssetMgr->AddLibrary(ResPaths.DataDir, ",audio,voice"); // dir may have anything
    if (!ResPaths.AudioPak.Path.IsEmpty())
        AssetMgr->AddLibrary(ResPaths.AudioPak.Path, "audio");
    if (!ResPaths.SpeechPak.Path.IsEmpty())
        AssetMgr->AddLibrary(ResPaths.SpeechPak.Path, "voice");
}

void engine_init_keyboard()
{
#ifdef ALLEGRO_KEYBOARD_HANDLER
    Debug::Printf(kDbgMsg_Info, "Initializing keyboard");

    install_keyboard();
#endif
#if AGS_PLATFORM_OS_LINUX
    setlocale(LC_NUMERIC, "C"); // needed in X platform because install keyboard affects locale of printfs
#endif
}

void engine_init_timer()
{
    Debug::Printf(kDbgMsg_Info, "Install timer");

    skipMissedTicks();
}

bool try_install_sound(int digi_id, int midi_id, String *p_err_msg = nullptr)
{
    Debug::Printf(kDbgMsg_Info, "Trying to init: digital driver ID: '%s' (0x%x), MIDI driver ID: '%s' (0x%x)",
        AlIDToChars(digi_id).s, digi_id, AlIDToChars(midi_id).s, midi_id);

    if (install_sound(digi_id, midi_id, nullptr) == 0)
        return true;
    // Allegro does not let you try digital and MIDI drivers separately,
    // and does not indicate which driver failed by return value.
    // Therefore we try to guess.
    if (p_err_msg)
        *p_err_msg = get_allegro_error();
    if (midi_id != MIDI_NONE)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to init one of the drivers; Error: '%s'.\nWill try to start without MIDI", get_allegro_error());
        if (install_sound(digi_id, MIDI_NONE, nullptr) == 0)
            return true;
    }
    if (digi_id != DIGI_NONE)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to init one of the drivers; Error: '%s'.\nWill try to start without DIGI", get_allegro_error());
        if (install_sound(DIGI_NONE, midi_id, nullptr) == 0)
            return true;
    }
    Debug::Printf(kDbgMsg_Error, "Failed to init sound drivers. Error: %s", get_allegro_error());
    return false;
}

// Attempts to predict a digital driver Allegro would chose, and get its maximal voices
std::pair<int, int> autodetect_driver(_DRIVER_INFO *driver_list, int (*detect_audio_driver)(int), const char *type)
{
    for (int i = 0; driver_list[i].driver; ++i)
    {
        if (driver_list[i].autodetect)
        {
            int voices = detect_audio_driver(driver_list[i].id);
            if (voices != 0)
                return std::make_pair(driver_list[i].id, voices);
            Debug::Printf(kDbgMsg_Warn, "Failed to detect %s driver %s; Error: '%s'.",
                    type, AlIDToChars(driver_list[i].id).s, get_allegro_error());
        }
    }
    return std::make_pair(0, 0);
}

// Decides which audio driver to request from Allegro.
// Returns a pair of audio card ID and max available voices.
std::pair<int, int> decide_audiodriver(int try_id, _DRIVER_INFO *driver_list,
    int(*detect_audio_driver)(int), int &al_drv_id, const char *type)
{
    if (try_id == 0) // no driver
        return std::make_pair(0, 0);
    al_drv_id = 0; // the driver id will be set by library if one was found
    if (try_id > 0)
    {
        int voices = detect_audio_driver(try_id);
        if (al_drv_id == try_id && voices != 0) // found and detected
            return std::make_pair(try_id, voices);
        if (voices == 0) // found in list but detect failed
            Debug::Printf(kDbgMsg_Error, "Failed to detect %s driver %s; Error: '%s'.", type, AlIDToChars(try_id).s, get_allegro_error());
        else // not found at all
            Debug::Printf(kDbgMsg_Error, "Unknown %s driver: %s, will try to find suitable one.", type, AlIDToChars(try_id).s);
    }
    return autodetect_driver(driver_list, detect_audio_driver, type);
}

void engine_init_audio()
{
    Debug::Printf("Initializing sound drivers");
    int digi_id = usetup.digicard;
    int midi_id = usetup.midicard;
    int digi_voices = -1;
    int midi_voices = -1;
    // MOD player would need certain minimal number of voices
    // TODO: find out if this is still relevant?
    if (usetup.mod_player)
        digi_voices = NUM_DIGI_VOICES;

    Debug::Printf(kDbgMsg_Info, "Sound settings: digital driver ID: '%s' (0x%x), MIDI driver ID: '%s' (0x%x)",
        AlIDToChars(digi_id).s, digi_id, AlIDToChars(midi_id).s, midi_id);

    // First try if drivers are supported, and switch to autodetect if explicit option failed
    _DRIVER_INFO *digi_drivers = system_driver->digi_drivers ? system_driver->digi_drivers() : _digi_driver_list;
    std::pair<int, int> digi_drv = decide_audiodriver(digi_id, digi_drivers, detect_digi_driver, digi_card, "digital");
    _DRIVER_INFO *midi_drivers = system_driver->midi_drivers ? system_driver->midi_drivers() : _midi_driver_list;
    std::pair<int, int> midi_drv = decide_audiodriver(midi_id, midi_drivers, detect_midi_driver, midi_card, "MIDI");

    // Now, knowing which drivers we suppose to install, decide on which voices we reserve
    digi_id = digi_drv.first;
    midi_id = midi_drv.first;
    const int max_digi_voices = digi_drv.second;
    const int max_midi_voices = midi_drv.second;
    if (digi_voices > max_digi_voices)
        digi_voices = max_digi_voices;
    // NOTE: we do not specify number of MIDI voices, so don't have to calculate available here

    reserve_voices(digi_voices, midi_voices);
    // maybe this line will solve the sound volume? [??? wth is this]
    set_volume_per_voice(1);

    String err_msg;
    bool sound_res = try_install_sound(digi_id, midi_id, &err_msg);
    if (!sound_res)
    {
        Debug::Printf(kDbgMsg_Error, "Everything failed, disabling sound.");
        reserve_voices(0, 0);
        install_sound(DIGI_NONE, MIDI_NONE, nullptr);
    }
    // Only display a warning if they wanted a sound card
    const bool digi_failed = usetup.digicard != DIGI_NONE && digi_card == DIGI_NONE;
    const bool midi_failed = usetup.midicard != MIDI_NONE && midi_card == MIDI_NONE;
    if (digi_failed || midi_failed)
    {
        platform->DisplayAlert("Warning: cannot enable %s.\nProblem: %s.\n\nYou may supress this message by disabling %s in the game setup.",
            (digi_failed && midi_failed ? "game audio" : (digi_failed ? "digital audio" : "MIDI audio") ),
            (err_msg.IsEmpty() ? "No compatible drivers found in the system" : err_msg.GetCStr()),
            (digi_failed && midi_failed ? "sound" : (digi_failed ? "digital sound" : "MIDI sound") ));
    }

    usetup.digicard = digi_card;
    usetup.midicard = midi_card;

    Debug::Printf(kDbgMsg_Info, "Installed digital driver ID: '%s' (0x%x), MIDI driver ID: '%s' (0x%x)",
        AlIDToChars(digi_card).s, digi_card, AlIDToChars(midi_card).s, midi_card);

    if (digi_card == DIGI_NONE)
    {
        // disable speech and music if no digital sound
        // therefore the MIDI soundtrack will be used if present,
        // and the voice mode should not go to Voice Only
        play.want_speech = -2;
        play.separate_music_lib = 0;
    }
    if (usetup.mod_player && digi_driver->voices < NUM_DIGI_VOICES)
    {
        // disable MOD player if there's not enough digital voices
        // TODO: find out if this is still relevant?
        usetup.mod_player = 0;
    }

#if AGS_PLATFORM_OS_WINDOWS
    if (digi_card == DIGI_DIRECTX(0))
    {
        // DirectX mixer seems to buffer an extra sample itself
        use_extra_sound_offset = 1;
    }
#endif
}

void engine_init_debug()
{
    //set_volume(255,-1);
    if ((debug_flags & (~DBG_DEBUGMODE)) >0) {
        platform->DisplayAlert("Engine debugging enabled.\n"
            "\nNOTE: You have selected to enable one or more engine debugging options.\n"
            "These options cause many parts of the game to behave abnormally, and you\n"
            "may not see the game as you are used to it. The point is to test whether\n"
            "the engine passes a point where it is crashing on you normally.\n"
            "[Debug flags enabled: 0x%02X]",debug_flags);
    }
}

void atexit_handler() {
    if (proper_exit==0) {
        platform->DisplayAlert("Error: the program has exited without requesting it.\n"
            "Program pointer: %+03d  (write this number down), ACI version %s\n"
            "If you see a list of numbers above, please write them down and contact\n"
            "developers. Otherwise, note down any other information displayed.",
            our_eip, EngineVersion.LongString.GetCStr());
    }
}

void engine_init_exit_handler()
{
    Debug::Printf(kDbgMsg_Info, "Install exit handler");

    atexit(atexit_handler);
}

void engine_init_rand()
{
    play.randseed = time(nullptr);
    srand (play.randseed);
}

void engine_init_pathfinder()
{
    init_pathfinder(loaded_game_file_version);
}

void engine_pre_init_gfx()
{
    //Debug::Printf("Initialize gfx");

    //platform->InitialiseAbufAtStartup();
}

int engine_load_game_data()
{
    Debug::Printf("Load game data");
    our_eip=-17;
    HError err = load_game_file();
    if (!err)
    {
        proper_exit=1;
        platform->FinishedUsingGraphicsMode();
        display_game_file_error(err);
        return EXIT_ERROR;
    }
    return 0;
}

bool do_extraction_work()
{
    if (justExtractMessages)
    {
        String fullpath = usetup.main_data_dir;
        if (!justExtractMessagesTo.IsEmpty())
        {
            fullpath = Path::ConcatPaths(usetup.main_data_dir, justExtractMessagesTo);
            if (!Path::IsDirectory(fullpath))
                Directory::CreateDirectory(fullpath);
        }
        ExtractGlobalMessages(game, fullpath);
        ExtractRoomMessages(0, 999, game, fullpath);
        proper_exit = 1;
        return false;
    }
    return true;
}

int engine_check_register_game()
{
    if (justRegisterGame) 
    {
        platform->RegisterGameWithGameExplorer();
        proper_exit = 1;
        return EXIT_NORMAL;
    }

    if (justUnRegisterGame) 
    {
        platform->UnRegisterGameWithGameExplorer();
        proper_exit = 1;
        return EXIT_NORMAL;
    }

    return 0;
}

void engine_init_title()
{
    our_eip=-91;
    set_window_title(game.gamename);
    Debug::Printf(kDbgMsg_Info, "Game title: '%s'", game.gamename);
}

// Setup paths and directories that may be affected by user configuration
void engine_init_user_directories()
{
    if (!usetup.user_data_dir.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "User data directory: %s", usetup.user_data_dir.GetCStr());
    if (!usetup.shared_data_dir.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Shared data directory: %s", usetup.shared_data_dir.GetCStr());

    // if end-user specified custom save path, use it
    bool res = false;
    if (!usetup.user_data_dir.IsEmpty())
    {
        res = SetCustomSaveParent(usetup.user_data_dir);
        if (!res)
        {
            Debug::Printf(kDbgMsg_Warn, "WARNING: custom user save path failed, using default system paths");
            res = false;
        }
    }
    // if there is no custom path, or if custom path failed, use default system path
    if (!res)
    {
        SetSaveGameDirectoryPath(Path::ConcatPaths(UserSavedgamesRootToken, game.saveGameFolderName));
    }
}

#if AGS_PLATFORM_OS_ANDROID
extern char android_base_directory[256];
#endif // AGS_PLATFORM_OS_ANDROID

// TODO: remake/remove this nonsense
int check_write_access() {

  if (platform->GetDiskFreeSpaceMB() < 2)
    return 0;

  our_eip = -1895;

  // The Save Game Dir is the only place that we should write to
  String svg_dir = get_save_game_directory();
  String tempPath = String::FromFormat("%s""tmptest.tmp", svg_dir.GetCStr());
  Stream *temp_s = Common::File::CreateFile(tempPath);
  if (!temp_s)
      // TODO: The fallback should be done on all platforms, and there's
      // already similar procedure found in SetSaveGameDirectoryPath.
      // If Android has extra dirs to fallback to, they should be provided
      // by platform driver's method, not right here!
#if AGS_PLATFORM_OS_ANDROID
  {
	  put_backslash(android_base_directory);
      tempPath.Format("%s""tmptest.tmp", android_base_directory);
	  temp_s = Common::File::CreateFile(tempPath);
	  if (temp_s == NULL) return 0;
	  else SetCustomSaveParent(android_base_directory);
  }
#else
    return 0;
#endif // AGS_PLATFORM_OS_ANDROID

  our_eip = -1896;

  temp_s->Write("just to test the drive free space", 30);
  delete temp_s;

  our_eip = -1897;

  if (::remove(tempPath))
    return 0;

  return 1;
}

int engine_check_disk_space()
{
    Debug::Printf(kDbgMsg_Info, "Checking for disk space");

    if (check_write_access()==0) {
        platform->DisplayAlert("Unable to write in the savegame directory.\n%s", platform->GetDiskWriteAccessTroubleshootingText());
        proper_exit = 1;
        return EXIT_ERROR; 
    }

    return 0;
}

int engine_check_font_was_loaded()
{
    if (!font_first_renderer_loaded())
    {
        platform->DisplayAlert("No game fonts found. At least one font is required to run the game.");
        proper_exit = 1;
        return EXIT_ERROR;
    }

    return 0;
}

void engine_init_modxm_player()
{
#ifndef PSP_NO_MOD_PLAYBACK
    if (game.options[OPT_NOMODMUSIC])
        usetup.mod_player = 0;

    if (usetup.mod_player) {
        Debug::Printf(kDbgMsg_Info, "Initializing MOD/XM player");

        if (init_mod_player(NUM_MOD_DIGI_VOICES) < 0) {
            platform->DisplayAlert("Warning: install_mod: MOD player failed to initialize.");
            usetup.mod_player=0;
        }
    }
#else
    usetup.mod_player = 0;
    Debug::Printf(kDbgMsg_Info, "Compiled without MOD/XM player");
#endif
}

// Do the preload graphic if available
void show_preload()
{
    color temppal[256];
	Bitmap *splashsc = BitmapHelper::CreateRawBitmapOwner( load_pcx("preload.pcx",temppal) );
    if (splashsc != nullptr)
    {
        Debug::Printf("Displaying preload image");
        if (splashsc->GetColorDepth() == 8)
            set_palette_range(temppal, 0, 255, 0);
        if (gfxDriver->UsesMemoryBackBuffer())
            gfxDriver->GetMemoryBackBuffer()->Clear();

        const Rect &view = play.GetMainViewport();
        Bitmap *tsc = BitmapHelper::CreateBitmapCopy(splashsc, game.GetColorDepth());
        if (!gfxDriver->HasAcceleratedTransform() && view.GetSize() != tsc->GetSize())
        {
            Bitmap *stretched = new Bitmap(view.GetWidth(), view.GetHeight(), tsc->GetColorDepth());
            stretched->StretchBlt(tsc, RectWH(0, 0, view.GetWidth(), view.GetHeight()));
            delete tsc;
            tsc = stretched;
        }
        IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(tsc, false, true);
        ddb->SetStretch(view.GetWidth(), view.GetHeight());
        gfxDriver->ClearDrawLists();
        gfxDriver->DrawSprite(0, 0, ddb);
        render_to_screen();
        gfxDriver->DestroyDDB(ddb);
        delete splashsc;
        delete tsc;
        platform->Delay(500);
    }
}

int engine_init_sprites()
{
    Debug::Printf(kDbgMsg_Info, "Initialize sprites");

    HError err = spriteset.InitFile(SpriteCache::DefaultSpriteFileName, SpriteCache::DefaultSpriteIndexName);
    if (!err) 
    {
        platform->FinishedUsingGraphicsMode();
        allegro_exit();
        proper_exit=1;
        platform->DisplayAlert("Could not load sprite set file %s\n%s",
            SpriteCache::DefaultSpriteFileName.GetCStr(),
            err->FullMessage().GetCStr());
        return EXIT_ERROR;
    }

    return 0;
}

void engine_init_game_settings()
{
    our_eip=-7;
    Debug::Printf("Initialize game settings");

    int ee;

    for (ee = 0; ee < MAX_ROOM_OBJECTS + game.numcharacters; ee++)
        actsps[ee] = nullptr;

    for (ee=0;ee<256;ee++) {
        if (game.paluses[ee]!=PAL_BACKGROUND)
            palette[ee]=game.defpal[ee];
    }

    for (ee = 0; ee < game.numcursors; ee++) 
    {
        // The cursor graphics are assigned to mousecurs[] and so cannot
        // be removed from memory
        if (game.mcurs[ee].pic >= 0)
            spriteset.Precache(game.mcurs[ee].pic);

        // just in case they typed an invalid view number in the editor
        if (game.mcurs[ee].view >= game.numviews)
            game.mcurs[ee].view = -1;

        if (game.mcurs[ee].view >= 0)
            precache_view (game.mcurs[ee].view);
    }
    // may as well preload the character gfx
    if (playerchar->view >= 0)
        precache_view (playerchar->view);

    for (ee = 0; ee < MAX_ROOM_OBJECTS; ee++)
        objcache[ee].image = nullptr;

    /*  dummygui.guiId = -1;
    dummyguicontrol.guin = -1;
    dummyguicontrol.objn = -1;*/

    our_eip=-6;
    //  game.chars[0].talkview=4;
    //init_language_text(game.langcodes[0]);

    for (ee = 0; ee < MAX_ROOM_OBJECTS; ee++) {
        scrObj[ee].id = ee;
        // 64 bit: Using the id instead
        // scrObj[ee].obj = NULL;
    }

    for (ee=0;ee<game.numcharacters;ee++) {
        memset(&game.chars[ee].inv[0],0,MAX_INV*sizeof(short));
        game.chars[ee].activeinv=-1;
        game.chars[ee].following=-1;
        game.chars[ee].followinfo=97 | (10 << 8);
        game.chars[ee].idletime=20;  // can be overridden later with SetIdle or summink
        game.chars[ee].idleleft=game.chars[ee].idletime;
        game.chars[ee].transparency = 0;
        game.chars[ee].baseline = -1;
        game.chars[ee].walkwaitcounter = 0;
        game.chars[ee].z = 0;
        charextra[ee].xwas = INVALID_X;
        charextra[ee].zoom = 100;
        if (game.chars[ee].view >= 0) {
            // set initial loop to 0
            game.chars[ee].loop = 0;
            // or to 1 if they don't have up/down frames
            if (views[game.chars[ee].view].loops[0].numFrames < 1)
                game.chars[ee].loop = 1;
        }
        charextra[ee].process_idle_this_time = 0;
        charextra[ee].invorder_count = 0;
        charextra[ee].slow_move_counter = 0;
        charextra[ee].animwait = 0;
    }
    // multiply up gui positions
    guibg = (Bitmap **)malloc(sizeof(Bitmap *) * game.numgui);
    guibgbmp = (IDriverDependantBitmap**)malloc(sizeof(IDriverDependantBitmap*) * game.numgui);
    for (ee=0;ee<game.numgui;ee++) {
        guibg[ee] = nullptr;
        guibgbmp[ee] = nullptr;
    }

    our_eip=-5;
    for (ee=0;ee<game.numinvitems;ee++) {
        if (game.invinfo[ee].flags & IFLG_STARTWITH) playerchar->inv[ee]=1;
        else playerchar->inv[ee]=0;
    }
    play.score=0;
    play.sierra_inv_color=7;
    // copy the value set by the editor
    if (game.options[OPT_GLOBALTALKANIMSPD] >= 0)
    {
        play.talkanim_speed = game.options[OPT_GLOBALTALKANIMSPD];
        game.options[OPT_GLOBALTALKANIMSPD] = 1;
    }
    else
    {
        play.talkanim_speed = -game.options[OPT_GLOBALTALKANIMSPD] - 1;
        game.options[OPT_GLOBALTALKANIMSPD] = 0;
    }
    play.inv_item_wid = 40;
    play.inv_item_hit = 22;
    play.messagetime=-1;
    play.disabled_user_interface=0;
    play.gscript_timer=-1;
    play.debug_mode=game.options[OPT_DEBUGMODE];
    play.inv_top=0;
    play.inv_numdisp=0;
    play.obsolete_inv_numorder=0;
    play.text_speed=15;
    play.text_min_display_time_ms = 1000;
    play.ignore_user_input_after_text_timeout_ms = 500;
    play.ClearIgnoreInput();
    play.lipsync_speed = 15;
    play.close_mouth_speech_time = 10;
    play.disable_antialiasing = 0;
    play.rtint_enabled = false;
    play.rtint_level = 0;
    play.rtint_light = 0;
    play.text_speed_modifier = 0;
    play.text_align = kHAlignLeft;
    // Make the default alignment to the right with right-to-left text
    if (game.options[OPT_RIGHTLEFTWRITE])
        play.text_align = kHAlignRight;

    play.speech_bubble_width = get_fixed_pixel_size(100);
    play.bg_frame=0;
    play.bg_frame_locked=0;
    play.bg_anim_delay=0;
    play.anim_background_speed = 0;
    play.silent_midi = 0;
    play.current_music_repeating = 0;
    play.skip_until_char_stops = -1;
    play.get_loc_name_last_time = -1;
    play.get_loc_name_save_cursor = -1;
    play.restore_cursor_mode_to = -1;
    play.restore_cursor_image_to = -1;
    play.ground_level_areas_disabled = 0;
    play.next_screen_transition = -1;
    play.temporarily_turned_off_character = -1;
    play.inv_backwards_compatibility = 0;
    play.gamma_adjustment = 100;
    play.do_once_tokens.resize(0);
    play.music_queue_size = 0;
    play.shakesc_length = 0;
    play.wait_counter=0;
    play.key_skip_wait = SKIP_NONE;
    play.cur_music_number=-1;
    play.music_repeat=1;
    play.music_master_volume=100 + LegacyMusicMasterVolumeAdjustment;
    play.digital_master_volume = 100;
    play.screen_flipped=0;
    play.cant_skip_speech = user_to_internal_skip_speech((SkipSpeechStyle)game.options[OPT_NOSKIPTEXT]);
    play.sound_volume = 255;
    play.speech_volume = 255;
    play.normal_font = 0;
    play.speech_font = 1;
    play.speech_text_shadow = 16;
    play.screen_tint = -1;
    play.bad_parsed_word[0] = 0;
    play.swap_portrait_side = 0;
    play.swap_portrait_lastchar = -1;
    play.swap_portrait_lastlastchar = -1;
    play.in_conversation = 0;
    play.skip_display = 3;
    play.no_multiloop_repeat = 0;
    play.in_cutscene = 0;
    play.fast_forward = 0;
    play.totalscore = game.totalscore;
    play.roomscript_finished = 0;
    play.no_textbg_when_voice = 0;
    play.max_dialogoption_width = get_fixed_pixel_size(180);
    play.no_hicolor_fadein = 0;
    play.bgspeech_game_speed = 0;
    play.bgspeech_stay_on_display = 0;
    play.unfactor_speech_from_textlength = 0;
    play.mp3_loop_before_end = 70;
    play.speech_music_drop = 60;
    play.room_changes = 0;
    play.check_interaction_only = 0;
    play.replay_hotkey_unused = -1;  // StartRecording: not supported.
    play.dialog_options_x = 0;
    play.dialog_options_y = 0;
    play.min_dialogoption_width = 0;
    play.disable_dialog_parser = 0;
    play.ambient_sounds_persist = 0;
    play.screen_is_faded_out = 0;
    play.player_on_region = 0;
    play.top_bar_backcolor = 8;
    play.top_bar_textcolor = 16;
    play.top_bar_bordercolor = 8;
    play.top_bar_borderwidth = 1;
    play.top_bar_ypos = 25;
    play.top_bar_font = -1;
    play.screenshot_width = 160;
    play.screenshot_height = 100;
    play.speech_text_align = kHAlignCenter;
    play.auto_use_walkto_points = 1;
    play.inventory_greys_out = 0;
    play.skip_speech_specific_key = 0;
    play.abort_key = 324;  // Alt+X
    play.fade_to_red = 0;
    play.fade_to_green = 0;
    play.fade_to_blue = 0;
    play.show_single_dialog_option = 0;
    play.keep_screen_during_instant_transition = 0;
    play.read_dialog_option_colour = -1;
    play.speech_portrait_placement = 0;
    play.speech_portrait_x = 0;
    play.speech_portrait_y = 0;
    play.speech_display_post_time_ms = 0;
    play.dialog_options_highlight_color = DIALOG_OPTIONS_HIGHLIGHT_COLOR_DEFAULT;
    play.speech_has_voice = false;
    play.speech_voice_blocking = false;
    play.speech_in_post_state = false;
    play.narrator_speech = game.playercharacter;
    play.crossfading_out_channel = 0;
    play.speech_textwindow_gui = game.options[OPT_TWCUSTOM];
    if (play.speech_textwindow_gui == 0)
        play.speech_textwindow_gui = -1;
    strcpy(play.game_name, game.gamename);
    play.lastParserEntry[0] = 0;
    play.follow_change_room_timer = 150;
    for (ee = 0; ee < MAX_ROOM_BGFRAMES; ee++) 
        play.raw_modified[ee] = 0;
    play.game_speed_modifier = 0;
    if (debug_flags & DBG_DEBUGMODE)
        play.debug_mode = 1;
    gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);
    play.shake_screen_yoff = 0;

    memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);
    memset(&play.script_timers[0],0,MAX_TIMERS * sizeof(int));
    memset(&play.default_audio_type_volumes[0], -1, MAX_AUDIO_TYPES * sizeof(int));

    // reset graphical script vars (they're still used by some games)
    for (ee = 0; ee < MAXGLOBALVARS; ee++) 
        play.globalvars[ee] = 0;

    for (ee = 0; ee < MAXGLOBALSTRINGS; ee++)
        play.globalstrings[ee][0] = 0;

    if (!usetup.translation.IsEmpty())
        init_translation (usetup.translation, "", true);

    update_invorder();
    displayed_room = -10;

    currentcursor=0;
    our_eip=-4;
    mousey=100;  // stop icon bar popping up

    // We use same variable to read config and be used at runtime for now,
    // so update it here with regards to game design option
    usetup.RenderAtScreenRes = 
        (game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_UserDefined && usetup.RenderAtScreenRes) ||
         game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_Enabled;
}

void engine_setup_scsystem_auxiliary()
{
    // ScriptSystem::aci_version is only 10 chars long
    strncpy(scsystem.aci_version, EngineVersion.LongString, 10);
    if (usetup.override_script_os >= 0)
    {
        scsystem.os = usetup.override_script_os;
    }
    else
    {
        scsystem.os = platform->GetSystemOSID();
    }
}

void engine_update_mp3_thread()
{
    update_mp3_thread();
    platform->Delay(50);
}

void engine_start_multithreaded_audio()
{
  // PSP: Initialize the sound cache.
  clear_sound_cache();

  // Create sound update thread. This is a workaround for sound stuttering.
  if (psp_audio_multithreaded)
  {
    if (!audioThread.CreateAndStart(engine_update_mp3_thread, true))
    {
      Debug::Printf(kDbgMsg_Info, "Failed to start audio thread, audio will be processed on the main thread");
      psp_audio_multithreaded = 0;
    }
    else
    {
      Debug::Printf(kDbgMsg_Info, "Audio thread started");
    }
  }
  else
  {
    Debug::Printf(kDbgMsg_Info, "Audio is processed on the main thread");
  }
}

void engine_prepare_to_start_game()
{
    Debug::Printf("Prepare to start game");

    engine_setup_scsystem_auxiliary();
    engine_start_multithreaded_audio();

#if AGS_PLATFORM_OS_ANDROID
    if (psp_load_latest_savegame)
        selectLatestSavegame();
#endif
}

// TODO: move to test unit
Bitmap *test_allegro_bitmap;
IDriverDependantBitmap *test_allegro_ddb;
void allegro_bitmap_test_init()
{
	test_allegro_bitmap = nullptr;
	// Switched the test off for now
	//test_allegro_bitmap = AllegroBitmap::CreateBitmap(320,200,32);
}

// Only allow searching around for game data on desktop systems;
// otherwise use explicit argument either from program wrapper, command-line
// or read from default config.
#if AGS_PLATFORM_OS_WINDOWS || AGS_PLATFORM_OS_LINUX || AGS_PLATFORM_OS_MACOS
    #define AGS_SEARCH_FOR_GAME_ON_LAUNCH
#endif

// Define location of the game data either using direct settings or searching
// for the available resource packs in common locations.
// Returns two paths:
// - startup_dir: this is where engine found game config and/or data;
// - data_path: full path of the main data pack;
// data_path's directory (may or not be eq to startup_dir) should be considered data directory,
// and this is where engine look for all game data.
HError define_gamedata_location_checkall(String &data_path, String &startup_dir)
{
    // First try if they provided a startup option
    if (!cmdGameDataPath.IsEmpty())
    {
        // If not a valid path - bail out
        if (!Path::IsFileOrDir(cmdGameDataPath))
            return new Error(String::FromFormat("Provided game location is not a valid path.\n Cwd: %s\n Path: %s",
                Directory::GetCurrentDirectory().GetCStr(),
                cmdGameDataPath.GetCStr()));
        // If it's a file, then keep it and proceed
        if (Path::IsFile(cmdGameDataPath))
        {
            Debug::Printf("Using provided game data path: %s", cmdGameDataPath.GetCStr());
            startup_dir = Path::GetDirectoryPath(cmdGameDataPath);
            data_path = cmdGameDataPath;
            return HError::None();
        }
    }

#if defined (AGS_SEARCH_FOR_GAME_ON_LAUNCH)
    // No direct filepath provided, search in common locations.
    data_path = search_for_game_data_file(startup_dir);
    if (data_path.IsEmpty())
    {
        return new Error("Engine was not able to find any compatible game data.",
            startup_dir.IsEmpty() ? String() : String::FromFormat("Searched in: %s", startup_dir.GetCStr()));
    }
    data_path = Path::MakeAbsolutePath(data_path);
    Debug::Printf(kDbgMsg_Info, "Located game data pak: %s", data_path.GetCStr());
    return HError::None();
#else
    // No direct filepath provided, bail out.
    return new Error("The game location was not defined by startup settings.");
#endif
}

// Define location of the game data
bool define_gamedata_location()
{
    String data_path, startup_dir;
    HError err = define_gamedata_location_checkall(data_path, startup_dir);
    if (!err)
    {
        platform->DisplayAlert("ERROR: Unable to determine game data.\n%s", err->FullMessage().GetCStr());
        main_print_help();
        return false;
    }

    // On success: set all the necessary path and filename settings
    usetup.startup_dir = startup_dir;
    usetup.main_data_file = data_path;
    usetup.main_data_dir = Path::GetDirectoryPath(data_path);
    return true;
}

// Find and preload main game data
bool engine_init_gamedata()
{
    Debug::Printf(kDbgMsg_Info, "Initializing game data");
    // First, find data location
    if (!define_gamedata_location())
        return false;

    // Try init game lib
    AssetError asset_err = AssetMgr->AddLibrary(usetup.main_data_file);
    if (asset_err != kAssetNoError)
    {
        platform->DisplayAlert("ERROR: The game data is missing, is of unsupported format or corrupt.\nFile: '%s'", usetup.main_data_file.GetCStr());
        return false;
    }

    // Pre-load game name and savegame folder names from data file
    // TODO: research if that is possible to avoid this step and just
    // read the full head game data at this point. This might require
    // further changes of the order of initialization.
    HError err = preload_game_data();
    if (!err)
    {
        display_game_file_error(err);
        return false;
    }

    // Setup ResPaths, so that we know out main locations further
    ResPaths.GamePak.Path = usetup.main_data_file;
    ResPaths.GamePak.Name = Path::GetFilename(usetup.main_data_file);
    ResPaths.DataDir = usetup.install_dir.IsEmpty() ? usetup.main_data_dir : Path::MakeAbsolutePath(usetup.install_dir);
    ResPaths.DataDir2 = Path::MakeAbsolutePath(usetup.opt_data_dir);
    ResPaths.AudioDir2 = Path::MakeAbsolutePath(usetup.opt_audio_dir);
    ResPaths.VoiceDir2 = Path::MakeAbsolutePath(usetup.opt_voice_dir);

    Debug::Printf(kDbgMsg_Info, "Startup directory: %s", usetup.startup_dir.GetCStr());
    Debug::Printf(kDbgMsg_Info, "Data directory: %s", ResPaths.DataDir.GetCStr());
    if (!ResPaths.DataDir2.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Opt data directory: %s", ResPaths.DataDir2.GetCStr());
    if (!ResPaths.AudioDir2.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Opt audio directory: %s", ResPaths.AudioDir2.GetCStr());
    if (!ResPaths.VoiceDir2.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Opt voice-over directory: %s", ResPaths.VoiceDir2.GetCStr());
    return true;
}

void engine_read_config(ConfigTree &cfg)
{
    if (!usetup.conf_path.IsEmpty())
    {
        IniUtil::Read(usetup.conf_path, cfg);
        return;
    }

    // Read default configuration file
    String def_cfg_file = find_default_cfg_file();
    IniUtil::Read(def_cfg_file, cfg);

    // Disabled on Windows because people were afraid that this config could be mistakenly
    // created by some installer and screw up their games. Until any kind of solution is found.
    String user_global_cfg_file;
    // Read user global configuration file
    user_global_cfg_file = find_user_global_cfg_file();
    if (Path::ComparePaths(user_global_cfg_file, def_cfg_file) != 0)
        IniUtil::Read(user_global_cfg_file, cfg);

    // Handle directive to search for the user config inside the game directory;
    // this option may come either from command line or default/global config.
    usetup.local_user_conf |= INIreadint(cfg, "misc", "localuserconf", 0) != 0;
    if (usetup.local_user_conf)
    { // Test if the file is writeable, if it is then both engine and setup
      // applications may actually use it fully as a user config, otherwise
      // fallback to default behavior.
        usetup.local_user_conf = File::TestWriteFile(def_cfg_file);
    }

    // Read user configuration file
    String user_cfg_file = find_user_cfg_file();
    if (Path::ComparePaths(user_cfg_file, def_cfg_file) != 0 &&
        Path::ComparePaths(user_cfg_file, user_global_cfg_file) != 0)
        IniUtil::Read(user_cfg_file, cfg);

    // Apply overriding options from mobile port settings
    // TODO: normally, those should be instead stored in the same config file in a uniform way
    // NOTE: the variable is historically called "ignore" but we use it in "override" meaning here
    if (psp_ignore_acsetup_cfg_file)
        override_config_ext(cfg);
}

// Gathers settings from all available sources into single ConfigTree
void engine_prepare_config(ConfigTree &cfg, const ConfigTree &startup_opts)
{
    Debug::Printf(kDbgMsg_Info, "Setting up game configuration");
    // Read configuration files
    engine_read_config(cfg);
    // Merge startup options in
    for (const auto &sectn : startup_opts)
        for (const auto &opt : sectn.second)
            cfg[sectn.first][opt.first] = opt.second;
}

// Applies configuration to the running game
void engine_set_config(const ConfigTree cfg)
{
    config_defaults();
    apply_config(cfg);
    post_config();
}

//
// --tell command support: printing engine/game info by request
//
extern std::set<String> tellInfoKeys;
static bool print_info_needs_game(const std::set<String> &keys)
{
    return keys.count("all") > 0 || keys.count("config") > 0 || keys.count("configpath") > 0 ||
        keys.count("data") > 0 || keys.count("filepath") > 0 || keys.count("gameproperties") > 0;
}

static void engine_print_info(const std::set<String> &keys, ConfigTree *user_cfg)
{
    const bool all = keys.count("all") > 0;
    ConfigTree data;
    if (all || keys.count("engine") > 0)
    {
        data["engine"]["name"] = get_engine_name();
        data["engine"]["version"] = get_engine_version();
    }
    if (all || keys.count("graphicdriver") > 0)
    {
        StringV drv;
        AGS::Engine::GetGfxDriverFactoryNames(drv);
        for (size_t i = 0; i < drv.size(); ++i)
        {
            data["graphicdriver"][String::FromFormat("%u", i)] = drv[i];
        }
    }
    if (all || keys.count("configpath") > 0)
    {
        String def_cfg_file = find_default_cfg_file();
        String gl_cfg_file = find_user_global_cfg_file();
        String user_cfg_file = find_user_cfg_file();
        data["configpath"]["default"] = def_cfg_file;
        data["configpath"]["global"] = gl_cfg_file;
        data["configpath"]["user"] = user_cfg_file;
    }
    if ((all || keys.count("config") > 0) && user_cfg)
    {
        for (const auto &sectn : *user_cfg)
        {
            String cfg_sectn = String::FromFormat("config@%s", sectn.first.GetCStr());
            for (const auto &opt : sectn.second)
                data[cfg_sectn][opt.first] = opt.second;
        }
    }
    if (all || keys.count("data") > 0)
    {
        data["data"]["gamename"] = game.gamename;
        data["data"]["version"] = StrUtil::IntToString(loaded_game_file_version);
        data["data"]["compiledwith"] = game.compiled_with;
        data["data"]["basepack"] = ResPaths.GamePak.Path;
    }
    if (all || keys.count("gameproperties") > 0)
    {
        fill_game_properties(data["gameproperties"]);
    }
    if (all || keys.count("filepath") > 0)
    {
        data["filepath"]["exe"] = appPath;
        data["filepath"]["cwd"] = Directory::GetCurrentDirectory();
        data["filepath"]["datadir"] = Path::MakePathNoSlash(ResPaths.DataDir);
        if (!ResPaths.DataDir2.IsEmpty())
        {
            data["filepath"]["datadir2"] = Path::MakePathNoSlash(ResPaths.DataDir2);
            data["filepath"]["audiodir2"] = Path::MakePathNoSlash(ResPaths.AudioDir2);
            data["filepath"]["voicedir2"] = Path::MakePathNoSlash(ResPaths.VoiceDir2);
        }
        data["filepath"]["savegamedir"] = Path::MakePathNoSlash(GetGameUserDataDir().FullDir);
        data["filepath"]["appdatadir"] = Path::MakePathNoSlash(GetGameAppDataDir().FullDir);
    }
    String full;
    IniUtil::WriteToString(full, data);
    platform->WriteStdOut("%s", full.GetCStr());
}

// Custom resource search callback for Allegro's system driver.
// It helps us direct Allegro to our game data location, because it won't know.
static int al_find_resource(char *dest, const char* resource, int dest_size)
{
    String path = AssetMgr->FindAssetFileOnly(resource);
    if (!path.IsEmpty())
    {
        snprintf(dest, dest_size, "%s", path.GetCStr());
        return 0;
    }
    return -1;
}

// TODO: this function is still a big mess, engine/system-related initialization
// is mixed with game-related data adjustments. Divide it in parts, move game
// data init into either InitGameState() or other game method as appropriate.
int initialize_engine(const ConfigTree &startup_opts)
{
    if (engine_pre_init_callback) {
        engine_pre_init_callback();
    }

    //-----------------------------------------------------
    // Install backend
    if (!engine_init_allegro())
        return EXIT_ERROR;

    //-----------------------------------------------------
    // Locate game data and assemble game config
    if (justTellInfo && !print_info_needs_game(tellInfoKeys))
    {
        engine_print_info(tellInfoKeys, nullptr);
        return EXIT_NORMAL;
    }

    if (!engine_init_gamedata())
        return EXIT_ERROR;
    ConfigTree cfg;
    engine_prepare_config(cfg, startup_opts);
    // Test if need to run built-in setup program (where available)
    if (!justTellInfo && justRunSetup)
    {
        int res;
        if (!engine_run_setup(cfg, res))
            return res;
    }
    // Set up game options from user config
    engine_set_config(cfg);
    engine_setup_allegro();
    engine_force_window();
    if (justTellInfo)
    {
        engine_print_info(tellInfoKeys, &cfg);
        return EXIT_NORMAL;
    }

    our_eip = -190;

    //-----------------------------------------------------
    // Init auxiliary data files and other directories, initialize asset manager
    engine_init_user_directories();

    our_eip = -191;

    engine_locate_speech_pak();

    our_eip = -192;

    engine_locate_audio_pak();

    our_eip = -193;

    engine_assign_assetpaths();

    // Assign custom find resource callback for limited Allegro operations
    system_driver->find_resource = al_find_resource;

    //-----------------------------------------------------
    // Begin setting up systems
    engine_setup_window();    

    our_eip = -194;

    engine_init_fonts();

    our_eip = -195;

    engine_init_keyboard();

    our_eip = -196;

    engine_init_mouse();

    our_eip = -197;

    engine_init_timer();

    our_eip = -198;

    engine_init_audio();

    our_eip = -199;

    engine_init_debug();

    our_eip = -10;

    engine_init_exit_handler();

    engine_init_rand();

    engine_init_pathfinder();

    set_game_speed(40);

    our_eip=-20;
    our_eip=-19;

    int res = engine_load_game_data();
    if (res != 0)
        return res;

    if (!do_extraction_work())
        return EXIT_NORMAL;

    res = engine_check_register_game();
    if (res != 0)
        return res;

    engine_init_title();

    our_eip = -189;

    res = engine_check_disk_space();
    if (res != 0)
        return res;

    // Make sure that at least one font was loaded in the process of loading
    // the game data.
    // TODO: Fold this check into engine_load_game_data()
    res = engine_check_font_was_loaded();
    if (res != 0)
        return res;

    our_eip = -179;

    engine_init_modxm_player();

    engine_init_resolution_settings(game.GetGameRes());

    // Attempt to initialize graphics mode
    if (!engine_try_set_gfxmode_any(usetup.Screen))
        return EXIT_ERROR;

    SetMultitasking(0);

    // [ER] 2014-03-13
    // Hide the system cursor via allegro
    show_os_cursor(MOUSE_CURSOR_NONE);

    show_preload();

    res = engine_init_sprites();
    if (res != 0)
        return res;

    engine_init_game_settings();

    engine_prepare_to_start_game();

	allegro_bitmap_test_init();

    initialize_start_and_play_game(override_start_room, loadSaveGameOnStartup);

    quit("|bye!");
    return EXIT_NORMAL;
}

bool engine_try_set_gfxmode_any(const ScreenSetup &setup)
{
    engine_shutdown_gfxmode();

    const Size init_desktop = get_desktop_size();
    if (!graphics_mode_init_any(game.GetGameRes(), setup, ColorDepthOption(game.GetColorDepth())))
        return false;

    engine_post_gfxmode_setup(init_desktop);
    return true;
}

bool engine_try_switch_windowed_gfxmode()
{
    if (!gfxDriver || !gfxDriver->IsModeSet())
        return false;

    // Keep previous mode in case we need to revert back
    DisplayMode old_dm = gfxDriver->GetDisplayMode();
    GameFrameSetup old_frame = graphics_mode_get_render_frame();

    // Release engine resources that depend on display mode
    engine_pre_gfxmode_release();

    Size init_desktop = get_desktop_size();
    bool switch_to_windowed = !old_dm.Windowed;
    ActiveDisplaySetting setting = graphics_mode_get_last_setting(switch_to_windowed);
    DisplayMode last_opposite_mode = setting.Dm;
    GameFrameSetup use_frame_setup = setting.FrameSetup;
    
    // If there are saved parameters for given mode (fullscreen/windowed)
    // then use them, if there are not, get default setup for the new mode.
    bool res;
    if (last_opposite_mode.IsValid())
    {
        res = graphics_mode_set_dm(last_opposite_mode);
    }
    else
    {
        // we need to clone from initial config, because not every parameter is set by graphics_mode_get_defaults()
        DisplayModeSetup dm_setup = usetup.Screen.DisplayMode;
        dm_setup.Windowed = !old_dm.Windowed;
        graphics_mode_get_defaults(dm_setup.Windowed, dm_setup.ScreenSize, use_frame_setup);
        res = graphics_mode_set_dm_any(game.GetGameRes(), dm_setup, old_dm.ColorDepth, use_frame_setup);
    }

    // Apply corresponding frame render method
    if (res)
        res = graphics_mode_set_render_frame(use_frame_setup);
    
    if (!res)
    {
        // If failed, try switching back to previous gfx mode
        res = graphics_mode_set_dm(old_dm) &&
              graphics_mode_set_render_frame(old_frame);
    }

    if (res)
    {
        // If succeeded (with any case), update engine objects that rely on
        // active display mode.
        if (gfxDriver->GetDisplayMode().Windowed)
            init_desktop = get_desktop_size();
        engine_post_gfxmode_setup(init_desktop);
    }
    ags_clear_input_buffer();
    return res;
}

void engine_shutdown_gfxmode()
{
    if (!gfxDriver)
        return;

    engine_pre_gfxsystem_shutdown();
    graphics_mode_shutdown();
}

const char *get_engine_name()
{
    return "Adventure Game Studio run-time engine";
}

const char *get_engine_version() {
    return EngineVersion.LongString.GetCStr();
}

void engine_set_pre_init_callback(t_engine_pre_init_callback callback) {
    engine_pre_init_callback = callback;
}
