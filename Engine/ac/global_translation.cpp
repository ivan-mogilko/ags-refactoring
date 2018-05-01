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
#include "ac/common.h"
#include "ac/display.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "ac/translation.h"
#include "ac/tree_map.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "plugin/plugin_engine.h"
#include "util/memory.h"
#include "core/types.h"
#include "util/math.h"
#include "util/string_utils.h"

using namespace AGS::Common;

using namespace AGS::Common::Memory;

extern GameState play;
extern AGSPlatformDriver *platform;

const char *get_translation (const char *text) {
    if (text == nullptr)
        quit("!Null string supplied to CheckForTranslations");

    source_text_length = GetTextDisplayLength(text);

#if AGS_PLATFORM_64BIT
    // check if a plugin wants to translate it - if so, return that
    // TODO: plugin API is currently strictly 32-bit, so this may break on 64-bit systems
    char *plResult = Int32ToPtr<char>(pl_run_plugin_hooks(AGSE_TRANSLATETEXT, PtrToInt32(text)));
    if (plResult) {
        return plResult;
    }
#endif

    auto *transtree = get_translation_tree_writeable();
    if (transtree != nullptr) {
        // translate the text using the translation file
        const char *lookup = text;
        const char *transl = transtree->findValue(lookup);
        if (transl != nullptr)
            return transl;
        if (*lookup == '&' && usetup.tra_trynovoice)
        { // skip voice token and repeat lookup
            while (*lookup != ' ' && *lookup != 0) lookup++;
            while (*lookup == ' ' && *lookup != 0) lookup++;
            transl = transtree->findValue(lookup);
            if (transl != NULL)
            { // add new translation with voice token to the tree so that
              // it will be found next time
                char new_trans[STD_BUFFER_SIZE] = {0};
                size_t token_len = lookup - text;
                strncpy(new_trans, text, Math::Min((size_t)STD_BUFFER_SIZE - 1, token_len));
                snprintf(new_trans + token_len, STD_BUFFER_SIZE - 1 - token_len, "%s", transl);
                return transtree->addText(text, new_trans);
                // because of how translated text traversal currently works in the engine,
                // we must return pointer to the string stored in the global storage,
            }
        }
    }
    // return the original text
    return text;
}

int IsTranslationAvailable () {
    if (usetup.stealth_tra)
        return 0;
    if (get_translation_tree() != nullptr)
        return 1;
    return 0;
}

int GetTranslationName (char* buffer) {
    VALIDATE_STRING (buffer);
    strcpy(buffer, get_translation_name().GetCStr());
    return IsTranslationAvailable();
}
