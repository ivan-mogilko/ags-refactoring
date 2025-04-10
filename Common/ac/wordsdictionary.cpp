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
#include <stdio.h>
#include <string.h>
#include "ac/wordsdictionary.h"
#include "util/stream.h"
#include "util/string_compat.h"

using namespace AGS::Common;

WordsDictionary::WordsDictionary()
    : num_words(0)
    , word(nullptr)
    , wordnum(nullptr)
{
}

WordsDictionary::~WordsDictionary()
{
    free_memory();
}

void WordsDictionary::allocate_memory(int wordCount)
{
    num_words = wordCount;
    if (num_words > 0)
    {
        word = new char*[wordCount];
        word[0] = new char[wordCount * MAX_PARSER_WORD_LENGTH];
        wordnum = new short[wordCount];
        for (int i = 1; i < wordCount; i++)
        {
            word[i] = word[0] + MAX_PARSER_WORD_LENGTH * i;
        }
    }
}

void WordsDictionary::free_memory()
{
    if (num_words > 0)
    {
        delete [] word[0];
        delete [] word;
        delete [] wordnum;
        word = nullptr;
        wordnum = nullptr;
        num_words = 0;
    }
}

void WordsDictionary::sort () {
    int aa, bb;
    for (aa = 0; aa < num_words; aa++) {
        for (bb = aa + 1; bb < num_words; bb++) {
            if (((wordnum[aa] == wordnum[bb]) && (ags_stricmp(word[aa], word[bb]) > 0))
                || (wordnum[aa] > wordnum[bb])) {
                    short temp = wordnum[aa];
                    char tempst[MAX_PARSER_WORD_LENGTH];
                    wordnum[aa] = wordnum[bb];
                    wordnum[bb] = temp;
                    snprintf(tempst, MAX_PARSER_WORD_LENGTH, "%s", word[aa]);
                    snprintf(word[aa], MAX_PARSER_WORD_LENGTH, "%s", word[bb]);
                    snprintf(word[bb], MAX_PARSER_WORD_LENGTH, "%s", tempst);
                    bb = aa;
            }
        }
    }
}

int WordsDictionary::find_index (const char*wrem) {
    int aa;
    for (aa = 0; aa < num_words; aa++) {
        if (ags_stricmp (wrem, word[aa]) == 0)
            return aa;
    }
    return -1;
}

const char *passwencstring = "Avis Durgan";

void decrypt_text(char *buf, size_t buf_sz)
{
    int adx = 0;
    const char *p_end = buf + buf_sz;

    while (buf < p_end)
    {
        *buf -= passwencstring[adx];
        if (*buf == 0)
            break;

        adx++;
        buf++;

        if (adx > 10)
            adx = 0;
    }
}

void read_string_decrypt(Stream *in, char *buf, size_t buf_sz)
{
    size_t len = in->ReadInt32();
    size_t slen = std::min(buf_sz - 1, len);
    in->Read(buf, slen);
    if (len > slen)
        in->Seek(len - slen);
    decrypt_text(buf, slen);
    buf[slen] = 0;
}

Common::String read_string_decrypt(Common::Stream *in)
{
    std::vector<char> dec_buf;
    return read_string_decrypt(in, dec_buf);
}

String read_string_decrypt(Stream *in, std::vector<char> &dec_buf)
{
    size_t len = in->ReadInt32();
    dec_buf.resize(len + 1);
    in->Read(dec_buf.data(), len);
    decrypt_text(dec_buf.data(), len);
    dec_buf.back() = 0; // null terminate in case read string does not have one
    return String(dec_buf.data());
}

void read_dictionary(WordsDictionary *dict, Stream *out)
{
    dict->allocate_memory(out->ReadInt32());
    for (int i = 0; i < dict->num_words; ++i)
    {
        read_string_decrypt(out, dict->word[i], MAX_PARSER_WORD_LENGTH);
        dict->wordnum[i] = out->ReadInt16();
    }
}

#if defined (OBSOLETE)
// TODO: not a part of wordsdictionary, move to obsoletes
void freadmissout(short *pptr, Stream *in) {
  in->ReadArrayOfInt16(&pptr[0], 5);
  in->ReadArrayOfInt16(&pptr[7], NUM_CONDIT - 7);
  pptr[5] = pptr[6] = 0;
}
#endif

void encrypt_text(char *toenc) {
  int adx = 0, tobreak = 0;

  while (tobreak == 0) {
    if (toenc[0] == 0)
      tobreak = 1;

    toenc[0] += passwencstring[adx];
    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void write_string_encrypt(Stream *out, const char *s) {
  int stlent = (int)strlen(s) + 1;

  out->WriteInt32(stlent);
  char *enc = ags_strdup(s);
  encrypt_text(enc);
  out->WriteArray(enc, stlent, 1);
  free(enc);
}

void write_dictionary (WordsDictionary *dict, Stream *out) {
  int ii;

  out->WriteInt32(dict->num_words);
  for (ii = 0; ii < dict->num_words; ii++) {
    write_string_encrypt (out, dict->word[ii]);
    out->WriteInt16(dict->wordnum[ii]);
  }
}
