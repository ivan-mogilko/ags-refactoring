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
// AGS game build utility.
// Uses a collection of AGS cli tools, compiles and builds the generic game
// package, using AGS game project and miscelaneous project files as input.
// 
//=============================================================================
#include <cstdio>
#include "platform/platform.h"
#if (AGS_PLATFORM_OS_WINDOWS)
#include "platform/windows/windows.h"
#endif
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/string_utils.h"
#include "util/textstreamreader.h"
#include "util/textstreamwriter.h"

const char *HELP_STRING = ""
    "agsbuild v0.1.0 - AGS game project build tool\n"
    "Copyright (c) 2026 AGS Team and contributors\n"
    "Usage: agsbuild [<OPTIONS>] <input-game.agf> <out-dir> <temp-dir>\n";

using namespace AGS::Common;

const char *ToolAgfExport       = "agfexport";
const char *ToolAgf2Dta         = "agf2dta";
const char *ToolDlgAsc          = "agf2dlgasc";
const char *ToolAgsPak          = "agspak";
const char *ToolCrm2Ash         = "crm2ash";
const char *ToolCrmPak          = "crmpak";
const char *ToolTrac            = "trac";
const char *ScriptCompiler      = "agscc";
const char *ScriptAPIHeader     = "agsdefns.sh";



bool StartProcessAndWait(const String &exe_path, const String &params)
{
    printf("Running: %s %s\n", exe_path.GetCStr(), params.GetCStr());
#if (AGS_PLATFORM_OS_WINDOWS)
    STARTUPINFOW si = { sizeof(si), NULL, NULL, NULL, NULL, CREATE_NO_WINDOW, NULL };
    PROCESS_INFORMATION pi;

    HANDLE hPipeRead, hPipeWrite;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
    sa.bInheritHandle = TRUE; 
    sa.lpSecurityDescriptor = NULL; 
    bool pipe_created = CreatePipe(&hPipeRead, &hPipeWrite, &sa, 1024u) == TRUE;
    // NOTE: we only check the pipe result to know if we should dispose of it.
    // if pipe failed, we still can run the process, although we won't see it's output
    if (pipe_created)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = INVALID_HANDLE_VALUE;
        si.hStdOutput = hPipeWrite;
        si.hStdError = hPipeWrite;
    }

    String cmd_utf8 = String::FromFormat("%s.exe %s", exe_path.GetCStr(), params.GetCStr());
    std::vector<wchar_t> cmd(cmd_utf8.GetLength() + 1);
    StrUtil::ConvertUtf8ToWstr(cmd_utf8.GetCStr(), cmd.data(), cmd.size());
    if (CreateProcessW(NULL, cmd.data(), NULL, NULL, TRUE /* bInheritHandles */, CREATE_NO_WINDOW, NULL, NULL, &si, &pi) != TRUE)
    {
        printf("\tProcess failed with err code: %u\n", GetLastError());
        return false;
    }

    if (pipe_created)
    {
        char buffer[1024];
        DWORD proc_result = 0;

        printf("-----------------------------------------------------------------------\n");
        do
        {
            proc_result = WaitForSingleObject(pi.hProcess, 1);
            DWORD pipe_has_bytes = 0;
            if ((PeekNamedPipe(hPipeRead, NULL, 0, NULL, &pipe_has_bytes, NULL) == TRUE) && (pipe_has_bytes > 0))
            {
                DWORD bytes_read = 0;
                while (pipe_has_bytes > 0
                    && ReadFile(hPipeRead, buffer, sizeof(buffer) - 1, &bytes_read, NULL) == TRUE && bytes_read > 0 && bytes_read != MAXDWORD)
                {
                    buffer[bytes_read] = 0;
                    printf("%s", buffer);
                    pipe_has_bytes -= bytes_read;
                }
            }
        }
        while (proc_result == WAIT_TIMEOUT);
        printf("-----------------------------------------------------------------------\n");

        if (hPipeRead != INVALID_HANDLE_VALUE)
            CloseHandle(hPipeRead);
        if (hPipeWrite != INVALID_HANDLE_VALUE)
            CloseHandle(hPipeWrite);
    }
    else
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    printf("\tExit code: %d\n\n", exit_code);
    return exit_code == 0;
#else
    return false;
#endif
}

void ReadStringListFromTextFile(const String &filename, std::vector<String> &list)
{
    auto text_reader = TextStreamReader(File::OpenFileRead(filename));
    for (String line = text_reader.ReadLine(); !line.IsEmpty(); line = text_reader.ReadLine())
        list.push_back(line);
}

void WriteStringListToTextFile(const String &filename, const std::vector<String> &list)
{
    auto text_writer = TextStreamWriter(File::CreateFile(filename));
    for (const auto &line : list)
        text_writer.WriteLine(line);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("%s\n", HELP_STRING);
        return -1;
    }

    const String src_agf = argv[1];
    const String dst_dir = argv[2];
    const String src_dir = Path::GetParent(src_agf);
    const String sys_dir = Directory::GetCurrentDirectory();
    const String temp_dir = argc >= 4 ? argv[3] : Path::ConcatPaths(dst_dir, "_temp");
    const bool temp_dir_owned = argc < 4;

    //-------------------------------------------------------------------------
    // Preliminary checks
    //-------------------------------------------------------------------------
    if (!File::IsFile(src_agf))
    {
        printf("Error: input project file '%s' not found or not a valid file", src_agf.GetCStr());
        return -1;
    }
    if (!File::IsDirectory(src_dir))
    {
        printf("Error: input project directory '%s' not found or not a valid directory", src_dir.GetCStr());
        return -1;
    }
    if (!File::IsDirectory(dst_dir))
    {
        printf("Error: output directory '%s' not found or not a valid directory", dst_dir.GetCStr());
        return -1;
    }

    //-------------------------------------------------------------------------
    // Prepare
    //-------------------------------------------------------------------------
    if (temp_dir_owned && !Directory::CreateDirectory(temp_dir))
    {
        printf("Error: failed to create temporary output directory '%s'", temp_dir.GetCStr());
        return -1;
    }

    std::vector<String> pak_temp_files;
    // Export all the necessary information
    {
        const String tool_agfexport = Path::ConcatPaths(sys_dir, ToolAgfExport);
        const String tool_dlgasc = Path::ConcatPaths(sys_dir, ToolDlgAsc);
        StartProcessAndWait(tool_agfexport, String::FromFormat("autoash %s %s/_AutoGenerated.ash", src_agf.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("custom-data-dir %s %s/_custom-data-dir.txt", src_agf.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("font-list %s %s/_fonts.txt", src_agf.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("game-cfg %s %s/acsetup.cfg", src_agf.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("glvar %s %s/_GlobalVariables.ash %s/_GlobalVariables.asc", src_agf.GetCStr(), temp_dir.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("header-list %s %s/_ash.txt", src_agf.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("plugin-list %s %s/_plugins.txt", src_agf.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("room-list %s %s/_rooms.txt", src_agf.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("script-list %s %s/_asc.txt", src_agf.GetCStr(), temp_dir.GetCStr()));
        StartProcessAndWait(tool_agfexport, String::FromFormat("tra-list %s %s/_translations.txt", src_agf.GetCStr(), temp_dir.GetCStr()));

        StartProcessAndWait(tool_dlgasc, String::FromFormat("%s %s/__DialogScripts.asc", src_agf.GetCStr(), temp_dir.GetCStr()));

        //pak_temp_files.push_back("acsetup.cfg");
    }

    // Build game28.dta
    {
        const String tool_agf2dta = Path::ConcatPaths(sys_dir, ToolAgf2Dta);
        StartProcessAndWait(tool_agf2dta, String::FromFormat("%s %s", src_agf.GetCStr(), temp_dir.GetCStr()));
        pak_temp_files.push_back("game28.dta");
    }

    // Compile game scripts
    String all_script_headers;
    std::vector<String> sc_module_objects;
    {
        const String tool_agscc = Path::ConcatPaths(sys_dir, ScriptCompiler);
        std::vector<String> sc_headers, sc_modules;
        ReadStringListFromTextFile(Path::ConcatPaths(temp_dir, "_ash.txt"), sc_headers);
        ReadStringListFromTextFile(Path::ConcatPaths(temp_dir, "_asc.txt"), sc_modules);
        File::CopyFile(Path::ConcatPaths(sys_dir, ScriptAPIHeader), Path::ConcatPaths(temp_dir, ScriptAPIHeader), true);
        StartProcessAndWait(tool_agscc, String::FromFormat("%s/_GlobalVariables.asc -H %s/%s;%s/_GlobalVariables.ash -o %s/_GlobalVariables.o", temp_dir.GetCStr(), temp_dir.GetCStr(), ScriptAPIHeader, temp_dir.GetCStr(), temp_dir.GetCStr()));
        sc_module_objects.push_back("_GlobalVariables.o");
        {
            String headers = String::FromFormat("%s/%s;%s/_GlobalVariables.ash;%s/_AutoGenerated.ash", temp_dir.GetCStr(), ScriptAPIHeader, temp_dir.GetCStr(), temp_dir.GetCStr());
            for (size_t sm = 0; sm < sc_modules.size(); ++sm)
            {
                if (sm < sc_headers.size())
                    headers.AppendFmt(";%s/%s", src_dir.GetCStr(), sc_headers[sm].GetCStr());
                sc_module_objects.push_back(Path::ReplaceExtension(sc_modules[sm], "o"));
                StartProcessAndWait(tool_agscc, String::FromFormat("%s/%s -H %s -o %s/%s",
                    src_dir.GetCStr(), sc_modules[sm].GetCStr(),
                    headers.GetCStr(), temp_dir.GetCStr(), sc_module_objects.back().GetCStr()));
            }
            all_script_headers = headers;
        }
        StartProcessAndWait(tool_agscc, String::FromFormat("%s/__DialogScripts.asc -H %s -o %s/__DialogScripts.o", temp_dir.GetCStr(), all_script_headers.GetCStr(), temp_dir.GetCStr()));

        std::copy(sc_module_objects.begin(), sc_module_objects.end(), std::back_inserter(pak_temp_files));
        pak_temp_files.push_back("__DialogScripts.o");
    }

    // Compile room scripts and prepare output room CRM files
    {
        const String tool_crm2ash = Path::ConcatPaths(sys_dir, ToolCrm2Ash);
        const String tool_crmpak = Path::ConcatPaths(sys_dir, ToolCrmPak);
        const String tool_agscc = Path::ConcatPaths(sys_dir, ScriptCompiler);
        std::vector<String> rooms;
        ReadStringListFromTextFile(Path::ConcatPaths(temp_dir, "_rooms.txt"), rooms);
        for (const auto &r_file : rooms)
        {
            const String r_header = Path::ReplaceExtension(r_file, "ash");
            const String r_script = Path::ReplaceExtension(r_file, "asc");
            const String r_scripto = Path::ReplaceExtension(r_file, "o");

            // We must copy crm file into the temp dir, because we need to either replace
            // compiled script in it, or cut it from it, either way the crm file will get modified
            File::CopyFile(Path::ConcatPaths(src_dir, r_file), Path::ConcatPaths(temp_dir, r_file), true);
            StartProcessAndWait(tool_crmpak, String::FromFormat("%s/%s -d CompScript3", temp_dir.GetCStr(), r_file.GetCStr()));

            StartProcessAndWait(tool_crm2ash, String::FromFormat("%s/%s %s/%s", temp_dir.GetCStr(), r_file.GetCStr(), temp_dir.GetCStr(), r_header.GetCStr()));
            const String headers = String::FromFormat("%s;%s/%s", all_script_headers.GetCStr(), temp_dir.GetCStr(), r_header.GetCStr());
            StartProcessAndWait(tool_agscc, String::FromFormat("%s/%s -H %s -o %s/%s",
                src_dir.GetCStr(), r_script.GetCStr(),
                headers.GetCStr(), temp_dir.GetCStr(), r_scripto.GetCStr()));

            pak_temp_files.push_back(r_file);
            pak_temp_files.push_back(r_scripto);
        }
    }

    // Generate hard-links for all the files necessary to be packed in the dedicated directory;
    // then pack them up into *.ags
    {
        // Create clear directory for packing *.ags
        const String &pak_dir = Path::ConcatPaths(temp_dir, "_package");
        // FIXME: maybe better delete files but keep directory?
        if (File::IsDirectory(pak_dir))
            Directory::DeleteDirectory(pak_dir);
        Directory::CreateDirectory(pak_dir);

        // Write ScriptModules.lst
        {
            std::vector<String> sco_without_global;
            for (const auto scm : sc_module_objects)
                if (scm != "GlobalScript.o")
                    sco_without_global.push_back(scm);
            WriteStringListToTextFile(Path::ConcatPaths(pak_dir, "ScriptModules.lst"), sco_without_global);
        }

        // Link output files to the pack dir
        {
            for (const auto &tmp_file : pak_temp_files)
                File::LinkFile(Path::ConcatPaths(temp_dir, tmp_file), Path::ConcatPaths(pak_dir, tmp_file), true);
        }

        // Link source game files to the pack dir
        {
            File::LinkFile(Path::ConcatPaths(src_dir, "acsprset.spr"), Path::ConcatPaths(pak_dir, "acsprset.spr"), true);
            File::LinkFile(Path::ConcatPaths(src_dir, "sprindex.dat"), Path::ConcatPaths(pak_dir, "sprindex.dat"), true);

            std::vector<String> font_files;
            ReadStringListFromTextFile(Path::ConcatPaths(temp_dir, "_fonts.txt"), font_files);
            for (const auto &ff : font_files)
            {
                String src_file = Path::ConcatPaths(src_dir, ff);
                if (File::IsFile(src_file))
                    File::LinkFile(src_file, Path::ConcatPaths(pak_dir, ff), true);
            }
        }

        // Package files
        const String tool_agspak = Path::ConcatPaths(sys_dir, ToolAgsPak);
        // FIXME: how to know the game file name? need to find out from the Game.agf using agfexport?
        // and optionally let pass game file name as a argument
        StartProcessAndWait(tool_agspak, String::FromFormat("-c %s/game.ags %s -r", temp_dir.GetCStr(), pak_dir.GetCStr()));
    }
    

    //-------------------------------------------------------------------------
    // Cleanup
    //-------------------------------------------------------------------------
    if (temp_dir_owned)
    {
        Directory::DeleteDirectory(temp_dir);
    }

    return 0;
}
