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
// AGS package file pack/unpack utility.
// 
// TODO:
// * append cmdline option (create new file / append to existing)
// * proper unified error codes for the AGS tools?
// 
//=============================================================================
#include "commands.h"
#include "data/include_utils.h"
#include "util/cmdlineopts.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

const char *BIN_STRING = "agspak v0.3.0 - AGS game packaging tool\n"
    "Copyright (c) 2025 AGS Team and contributors\n";

const char *HELP_STRING = "Usage:\n"
   //--------------------------------------------------------------------------------|
    "  agspak <input-dir> <pak-file> [OPTIONS]\n"
    "      - packs the dir contents into the pak file\n"
    "  agspak <COMMAND> [<CMD_OPTIONS>]\n"
    "      - executes an operation with the existing pak file\n"
    "\n"
    "Packing options:\n"
    "  -f, --pattern-file <file>\n"
    "                         use pattern file with the include/exclude patterns\n"
    "  -p <MB>                split game assets between partitions of this size max\n"
    "  -r                     recursive mode: include all subdirectories too\n"
    "\n"
    "Commands:\n"
    "  -u, --unpack <pak-file> <out-dir>\n"
    "                         unpackage all the pak file's contents into the dir\n"
    "\n"
//  "Command options:\n"
//  "\n"
    "Other options:\n"
    "  -v, --verbose          print operation details"
    ;


int DoPakCommand(const CmdLineOpts::ParseResult &cmdargs, bool verbose)
{
    // Parse the command
    // FIXME: CmdLineOpts utility currently do not let us detect whether there was
    // a '-N' or '--NAME' argument on a specified position;
    // figure out a better way of handling this (is there?).
    char command = 0;
    for (const auto &opt : cmdargs.Opt)
    {
        if (opt == "-u" || opt == "--unpack")
        {
            command = 'u'; // unpack
            break;
        }
    }

    // Run supported commands
    switch (command)
    {
    case 'u': // unpack
        {
            const String &src = cmdargs.PosArgs[0];
            const String &dst = cmdargs.PosArgs[1];
            if (!src.IsEmpty() && !dst.IsEmpty())
                return AGSPak::Command_Unpack(src, dst);
            break; // not enough args
        }
    default:
        printf("Error: command not specified\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    printf("Error: not enough arguments\n");
    printf("%s\n", HELP_STRING);
    return -1;
}

int DoPackage(const CmdLineOpts::ParseResult &cmdargs, bool verbose)
{
    if (cmdargs.PosArgs.size() < 2)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    // a include pattern file that should be inside the input-dir
    // TO-DO: support nested include pattern files in input-dir
    bool has_include_pattern_file = false;
    String include_pattern_file_name;

    const bool do_subdirs = cmdargs.Opt.count("-r");
    size_t part_size_mb = 0;
    for (const auto &opt_with_value : cmdargs.OptWithValue)
    {
        if (opt_with_value.first == "-p")
        {
            part_size_mb = StrUtil::StringToInt(opt_with_value.second);
        }
        else if (opt_with_value.first == "-f" || opt_with_value.first == "--pattern-file")
        {
            has_include_pattern_file = true;
            include_pattern_file_name = opt_with_value.second;
        }
    }

    const String &src = cmdargs.PosArgs[0];
    const String &dst = cmdargs.PosArgs[1];
    return AGSPak::Command_Pack(src, dst, include_pattern_file_name, do_subdirs, part_size_mb, verbose);
}

int main(int argc, char *argv[])
{
    printf("%s\n", BIN_STRING);

    CmdLineOpts::ParseResult cmdargs = CmdLineOpts::Parse(argc, argv, {"-p", "-f", "--pattern-file"});
    if (cmdargs.HelpRequested)
    {
        printf("%s\n", HELP_STRING);
        return 0; // display help and bail out
    }
    if (cmdargs.PosArgs.size() < 1)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    // FIXME: CmdLineOpts utility currently do not let us detect whether there was
    // a '-N' or '--NAME' argument on a specified position; find out whether this is
    // a limitation of this implementation, or a general convention on parsing args;
    // figure out a better way of handling this (is there?).
    const bool is_explicit_command =
        cmdargs.Opt.count("-u") > 0 || cmdargs.Opt.count("--unpack") > 0;
    const bool verbose = cmdargs.Opt.count("-v") || cmdargs.Opt.count("--verbose");

    if (is_explicit_command)
    {
        return DoPakCommand(cmdargs, verbose);
    }
    else
    {
        return DoPackage(cmdargs, verbose);
    }
}
