#include <stdio.h>
#include <memory>
#include <vector>
#include "data/agfreader.h"
#include "util/file.h"
#include "util/stream.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;
namespace AGF = AGS::AGF;


const char *HELP_STRING = "Usage: agf2dialogs <in-game.agf> <out-dir>\n";

int main(int argc, char *argv[])
{
    printf("agf2dialogs v0.1.0 - AGS project (AGF) dialogs extractor\n"\
        "Copyright (c) 2021 AGS Team and contributors\n");
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (ags_stricmp(arg, "--help") == 0 || ags_stricmp(arg, "/?") == 0 || ags_stricmp(arg, "-?") == 0)
        {
            printf("%s\n", HELP_STRING);
            return 0; // display help and bail out
        }
    }
    if (argc < 3)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    const char *src = argv[1];
    const char *dst = argv[2];
    printf("Input game AGF: %s\n", src);
    printf("Output directory: %s\n", dst);

    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//
    AGF::AGFReader reader;
    HError err = reader.Open(src);
    if (!err)
    {
        printf("Error: failed to open source AGF:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Get dialog scripts one by one and save into files
    //-----------------------------------------------------------------------//
    AGF::Dialogs p_dialogs;
    AGF::Dialog p_dialog;
    std::vector<AGF::DocElem> elems;
    p_dialogs.GetAll(reader.GetGameRoot(), elems);
    // Now load dialogs one by one, convert and append
    for (const auto &el : elems)
    {
        int id = p_dialog.ReadID(el);
        String name = p_dialog.ReadScriptName(el);
        const String script = p_dialog.ReadScript(el);
        if (script.IsNullOrSpace())
            continue;
        
        String filename = String::FromFormat("dialog%03d.ads", id);
        String path = String::FromFormat("%s/%s", dst, filename.GetCStr());
        std::unique_ptr<Stream> out(File::CreateFile(path));
        if (!out)
        {
            printf("Error: failed to open file for writing: %s\n", path.GetCStr());
            return -1;
        }

        // Prepend dialog script with a comment containing its script name;
        // this was not normally present in dialog scripts made in AGS,
        // but we do this for user's convenience.
        String comment = String::FromFormat("// %s (%d)\n", name.GetCStr(), id);
        out->Write(comment.GetCStr(), comment.GetLength());
        out->Write(script.GetCStr(), script.GetLength());
        out->Close();
        printf("Dialog written: %s\n", filename.GetCStr());
    }

    //-----------------------------------------------------------------------//
    // Write script body
    //-----------------------------------------------------------------------//
    printf("Done.\n");
    return 0;
}
