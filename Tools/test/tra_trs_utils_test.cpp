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
#include <cstring>
#include <memory>
#include <vector>

#include "data/tra_utils.h"
#include "gtest/gtest.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

namespace
{

// A PO source with each of the cases ReadPO handles
const char *TrsSource = R"(// AGS TRANSLATION SOURCE FILE"
"//$GameID=1234"
//$GameName=Game
//$NormalFont=2
//$SpeechFont=DEFAULT
//$TextDirection=RIGHT
//$Encoding=UTF-8
//$Language=it
//$AutoTranslateParserSaid=ON
//$Font1=Font3;Size=12;Outline=AUTO;AutoOutline=ROUND;AutoOutlineThickness=2;HeightDefinition=CUSTOM;CustomHeight=9;VerticalOffset=1;LineSpacing=4;CharacterSpacing=1;
//$Font0=File=x.ttf;SizeMultiplier=2;
Hello
Ciao
Unsure
Incerto
//#OBSOLETE
msgid "Old"
msgstr "Vecchio"

msgctxt "PARSERWORD:5"
msgid "look"
msgstr "guarda, osserva"

msgid "Apple"
msgid_plural "Apples"
msgstr[0] "Mela"
msgstr[1] "Mele"

msgid "Line\nbreak \"quoted\" \101"
msgstr "Caff\303\251\tbar"

msgid "Untranslated"
msgstr ""

msgid "Hello"
msgstr "Salve")";

TranslationSource ReadTestTRS()
{
    std::vector<uint8_t> data(TrsSource, TrsSource + strlen(TrsSource));
    TranslationSource trs;
    EXPECT_TRUE(ReadTRS(trs, AsStream<VectorStream>(data)));
    return trs;
}

TranslationSource ReadTRA(const std::vector<uint8_t> &data)
{
    TranslationSource trs;
    HError err = ReadTraData(trs, AsStream<VectorStream>(data));
    EXPECT_TRUE(err) << err->FullMessage().GetCStr();
    return trs;
}

std::vector<uint8_t> WriteTestTRA(const TranslationSource &trs)
{
    std::vector<uint8_t> data;
    EXPECT_TRUE(WriteTRA(trs, AsStream<VectorStream>(data, kStream_Write)));
    return data;
}

std::vector<uint8_t> WriteTestTRS(const TranslationSource &trs)
{
    std::vector<uint8_t> data;
    EXPECT_TRUE(WriteTRS(trs, AsStream<VectorStream>(data, kStream_Write)));
    return data;
}

void ExpectSameTranslation(const TranslationSource &src_trs, const TranslationSource &dst_trs)
{
    const auto &src_opts = src_trs.Options;
    const auto &dst_opts = dst_trs.Options;
    EXPECT_EQ(src_opts.GameUid, dst_opts.GameUid);
    EXPECT_STREQ(src_opts.GameName.GetCStr(), dst_opts.GameName.GetCStr());
    EXPECT_EQ(src_trs.Entries, dst_trs.Entries);
    EXPECT_EQ(src_trs.ParserDict.GetWords(), dst_trs.ParserDict.GetWords());
    EXPECT_EQ(src_opts.NormalFont, dst_opts.NormalFont);
    EXPECT_EQ(src_opts.SpeechFont, dst_opts.SpeechFont);
    EXPECT_EQ(src_opts.RightToLeft, dst_opts.RightToLeft);
    EXPECT_EQ(src_opts.OptFlags, dst_opts.OptFlags);
    EXPECT_STREQ(src_opts.EncodingHint.GetCStr(), dst_opts.EncodingHint.GetCStr());
    EXPECT_STREQ(src_opts.LanguageHint.GetCStr(), dst_opts.LanguageHint.GetCStr());
    ASSERT_EQ(src_opts.FontOverrides.size(), dst_opts.FontOverrides.size());
    for (const auto &fover : src_opts.FontOverrides)
    {
        ASSERT_EQ(1u, dst_opts.FontOverrides.count(fover.first));
        const FontOverride &a = fover.second;
        const FontOverride &b = dst_opts.FontOverrides.at(fover.first);
        EXPECT_EQ(a.Fields, b.Fields);
        EXPECT_EQ(a.Finfo.FontID, b.Finfo.FontID);
        EXPECT_STREQ(a.Finfo.FileName.GetCStr(), b.Finfo.FileName.GetCStr());
        EXPECT_EQ(a.Finfo.Flags, b.Finfo.Flags);
        EXPECT_EQ(a.Finfo.Size, b.Finfo.Size);
        EXPECT_EQ(a.Finfo.SizeMultiplier, b.Finfo.SizeMultiplier);
        EXPECT_EQ(a.Finfo.Outline, b.Finfo.Outline);
        EXPECT_EQ(a.Finfo.AutoOutlineStyle, b.Finfo.AutoOutlineStyle);
        EXPECT_EQ(a.Finfo.AutoOutlineThickness, b.Finfo.AutoOutlineThickness);
        EXPECT_EQ(a.Finfo.CustomHeight, b.Finfo.CustomHeight);
        EXPECT_EQ(a.Finfo.YOffset, b.Finfo.YOffset);
        EXPECT_EQ(a.Finfo.LineSpacing, b.Finfo.LineSpacing);
        EXPECT_EQ(a.Finfo.CharacterSpacing, b.Finfo.CharacterSpacing);
    }
}

} // namespace

TEST(TraUtils, ReadTRS)
{
    TranslationSource trs = ReadTestTRS();
    const auto &opts = trs.Options;
    EXPECT_EQ(1234, opts.GameUid);
    EXPECT_STREQ("Game", opts.GameName.GetCStr());
    EXPECT_EQ(2, opts.NormalFont);
    EXPECT_EQ(-1, opts.SpeechFont);
    EXPECT_EQ(kTextDirection_RTL, opts.RightToLeft);
    EXPECT_STREQ("UTF-8", opts.EncodingHint.GetCStr());
    EXPECT_STREQ("it", opts.LanguageHint.GetCStr());
    EXPECT_EQ(kTraOpt_AutoTranslateSaid, opts.OptFlags);
    EXPECT_EQ(2u, opts.FontOverrides.size());

    // the header, the obsolete and the untranslated entries are left out,
    // and parser words go to their own dictionary
    EXPECT_EQ(5u, trs.Dict.size());
    // a text found dst_opts keeps its first translation, same as with TRS
    EXPECT_STREQ("Ciao", trs.Dict["Hello"].GetCStr());
    // strings split on several lines are joined
    EXPECT_STREQ("Due righe", trs.Dict["Two lines"].GetCStr());
    // flags do not matter
    EXPECT_STREQ("Incerto", trs.Dict["Unsure"].GetCStr());
    // only the first plural form
    EXPECT_STREQ("Mela", trs.Dict["Apple"].GetCStr());
    // the texts are unescaped
    EXPECT_STREQ("Caff\xC3\xA9\tbar", trs.Dict["Line\nbreak \"quoted\" A"].GetCStr());
    EXPECT_EQ(0u, trs.Dict.count("Gone"));
    EXPECT_EQ(0u, trs.Dict.count("Old"));
    EXPECT_EQ(0u, trs.Dict.count("Untranslated"));
    EXPECT_EQ(0u, trs.Dict.count("look"));
    const auto &words = trs.ParserDict.GetWords();
    ASSERT_EQ(2u, words.size());
    EXPECT_EQ(5, words.find("guarda")->second);
    EXPECT_EQ(5, words.find("osserva")->second);
}

TEST(TraUtils, TRSCompilesToTRA)
{
    const TranslationSource trs = ReadTestTRS();
    ExpectSameTranslation(trs, ReadTRA(WriteTestTRA(trs)));
}

TEST(TraUtils, WriteTRS)
{
    TranslationSource trs = ReadTestTRS();
    trs.Dict["Back\\slash \"quoted\"\r\n\x01 end"] = "Tab\there";
    // the lines end in "\r\n" on Windows
    String text;
    for (const uint8_t c : WriteTestTRS(trs))
    {
        if (c != '\r')
            text.AppendChar(static_cast<char>(c));
    }
    // compared with "!=", as EXPECT_NE takes a reference to the undefined String::NoIndex
    EXPECT_TRUE(text.FindString("# $GameID=1234\n") != String::NoIndex);
    EXPECT_TRUE(text.FindString("# $NormalFont=2\n") != String::NoIndex);
    EXPECT_TRUE(text.FindString("# $TextDirection=RIGHT\n") != String::NoIndex);
    EXPECT_TRUE(text.FindString("# $Font0=File=x.ttf;SizeMultiplier=2;\n") != String::NoIndex);
    EXPECT_TRUE(text.FindString("\"Content-Type: text/plain; charset=UTF-8\\n\"\n") != String::NoIndex);
    // the texts are escaped
    EXPECT_TRUE(text.FindString("\nmsgid \"Back\\\\slash \\\"quoted\\\"\\r\\n\\001 end\"\nmsgstr \"Tab\\there\"\n") !=
                String::NoIndex);
    // one entry per parser word ID, with a placeholder text the PO reader keeps apart
    EXPECT_TRUE(text.FindString("\nmsgctxt \"PARSERWORD:5\"\nmsgid \"??? (word 5)\"\nmsgstr \"guarda, osserva\"\n") !=
                String::NoIndex);
}

TEST(TraUtils, WriteTRSCompilesToTheSameTranslation)
{
    // TRA -> TRS -> TRA
    TranslationSource trs = ReadTestTRS();
    trs.Dict["Back\\slash \"quoted\"\r\n\x01 end"] = "Tab\there";
    trs = ReadTRA(WriteTestTRA(trs));

    TranslationSource from_po;
    auto data_buf = WriteTestTRS(trs);
    ASSERT_TRUE(ReadTRS(from_po, AsStream<VectorStream>(data_buf)));
    ExpectSameTranslation(trs, from_po);
    ExpectSameTranslation(trs, ReadTRA(WriteTestTRA(from_po)));
}
