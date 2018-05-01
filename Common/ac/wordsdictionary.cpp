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

#include <algorithm>
#include <string.h>
#include "ac/wordsdictionary.h"
#include "util/stream.h"
#include "util/string_compat.h"

using AGS::Common::Stream;

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
                    char tempst[30];

                    wordnum[aa] = wordnum[bb];
                    wordnum[bb] = temp;
                    strcpy(tempst, word[aa]);
                    strcpy(word[aa], word[bb]);
                    strcpy(word[bb], tempst);
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

void decrypt_text(char*toenc) {
  int adx = 0;

  while (1) {
    toenc[0] -= passwencstring[adx];
    if (toenc[0] == 0)
      break;

    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void read_string_decrypt(Stream *in, char *buf, size_t buf_sz) {
  size_t len = in->ReadInt32();
  size_t slen = std::min(buf_sz - 1, len);
  in->Read(buf, slen);
  if (len > slen)
      in->Seek(len - slen);
  buf[slen] = 0;
  decrypt_text(buf);
}

#include "util/file.h"
#include "util/textstreamreader.h"
using namespace Common;
void read_dictionary (WordsDictionary *dict, Stream *out, const char *dict_tra_file) {
  dict->allocate_memory(out->ReadInt32());
  if (dict->num_words == 0)
    return;

  Stream *DictOut = NULL;
  Stream *DictIn = NULL;
  const String dict_filename = dict_tra_file;
  // Choose the dict-hack mode: if the dictionary translation file already exists, then
  // read it to overriden in-game dictionary. Otherwise, create one for writing
  // in-game dictionary as we read one from the game file.
  if (!dict_filename.IsEmpty())
  {
    if (File::TestReadFile(dict_filename))
      DictIn = File::OpenFileRead(dict_filename);
    else
      DictOut = File::CreateFile(dict_filename);
  }
  TextStreamReader txt_read(DictIn);

  for (int ii = 0; ii < dict->num_words; ii++) {
    read_string_decrypt (out, dict->word[ii], MAX_PARSER_WORD_LENGTH);
    if (dict->word[ii])
    {
      if (DictOut)
      {
        DictOut->Write(dict->word[ii], strlen(dict->word[ii]));
        DictOut->Write("\n", 1);
      }
      else if (DictIn)
      {
        String s = txt_read.ReadLine();
        if (!s.IsEmpty())
        {
          s.TruncateToLeft(MAX_PARSER_WORD_LENGTH - 1);
          memcpy(dict->word[ii], s.GetCStr(), s.GetLength() + 1);
        }
      }
    }
    dict->wordnum[ii] = out->ReadInt16();
  }
  delete DictOut;
  // DictIn is deleted by TextStreamReader
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
    out->WriteInt16(dict->wordnum[ii]);//__putshort__lilendian(dict->wordnum[ii], writeto);
  }
}
