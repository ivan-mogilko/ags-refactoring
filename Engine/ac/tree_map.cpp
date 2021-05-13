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
#include <stdlib.h>
#include "ac/common.h"
#include "ac/tree_map.h"

TreeMap::TreeMap() {
    left = nullptr;
    right = nullptr;
    text = nullptr;
    translation = nullptr;
}

const char* TreeMap::findValue (const char* key) const {
    if (text == nullptr)
        return nullptr;

    if (strcmp(key, text) == 0)
        return translation;

    if (strcmp (key, text) < 0) {
        if (left == nullptr)
            return nullptr;
        return left->findValue (key);
    }
    else {
        if (right == nullptr)
            return nullptr;
        return right->findValue (key);
    }
}

const char *TreeMap::addText (const char* ntx, char *trans) {
    if ((ntx == nullptr) || (ntx[0] == 0) ||
        ((text != nullptr) && (strcmp(ntx, text) == 0)))
        // don't add if it's an empty string or if it's already here
        return translation;

    if (text == nullptr) {
        text = (char*)malloc(strlen(ntx)+1);
        translation = (char*)malloc(strlen(trans)+1);
        if (translation == nullptr)
            quit("load_translation: out of memory");
        strcpy(text, ntx);
        strcpy(translation, trans);
        return translation;
    }
    else if (strcmp(ntx, text) < 0) {
        // Earlier in alphabet, add to left
        if (left == nullptr)
            left = new TreeMap();

        return left->addText (ntx, trans);
    }
    else if (strcmp(ntx, text) > 0) {
        // Later in alphabet, add to right
        if (right == nullptr)
            right = new TreeMap();

        return right->addText (ntx, trans);
    }
    return translation;
}

void TreeMap::clear() {
    if (left) {
        left->clear();
        delete left;
    }
    if (right) {
        right->clear();
        delete right;
    }
    if (text)
        free(text);
    if (translation)
        free(translation);
    left = nullptr;
    right = nullptr;
    text = nullptr;
    translation = nullptr;
}

TreeMap::~TreeMap() {
    clear();
}
