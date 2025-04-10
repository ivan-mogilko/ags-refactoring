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
#include "util/math.h"
#include "util/textstreamreader.h"

namespace AGS
{
namespace Common
{

bool TextStreamReader::IsValid() const
{
    return _stream && _stream->CanRead();
}

bool TextStreamReader::EOS() const
{
    return _stream ? _stream->EOS() : true;
}

char TextStreamReader::ReadChar()
{
    return _stream->ReadInt8();
}

String TextStreamReader::ReadString(size_t length)
{
    // TODO: remove carriage-return characters
    return String::FromStreamCount(_stream.get(), length);
}

String TextStreamReader::ReadLine()
{
    // TODO
    // Probably it is possible to group Stream::ReadString with this,
    // both use similar algorythm, difference is only in terminator chars

    String str;
    int chars_read_last = 0;
    int line_break_position = -1;
    // Read a chunk of memory to buffer and seek for null-terminator,
    // if not found, repeat until EOS
    const int single_chunk_length = 3000;
    const int max_chars = 5000000;
    char char_buffer[single_chunk_length + 1];
    do
    {
        chars_read_last = _stream->Read(char_buffer, single_chunk_length);
        char *seek_ptr = char_buffer;
        int c;
        for (c = 0; c < chars_read_last && *seek_ptr != '\n'; ++c, ++seek_ptr);

        int append_length = 0;
        int str_len = str.GetLength();
        if (c < chars_read_last && *seek_ptr == '\n')
        {
            line_break_position = seek_ptr - char_buffer;
            if (str_len < max_chars)
            {
                append_length = std::min(line_break_position, max_chars - str_len);
            }
        }
        else
        {
            append_length = std::min(chars_read_last, max_chars - str_len);
        }

        if (append_length > 0)
        {
            char_buffer[append_length] = '\0';
            str.Append(char_buffer);
        }
    }
    while (!EOS() && line_break_position < 0);

    // If null-terminator was found make sure stream is positioned at the next
    // byte after line end
    if (line_break_position >= 0)
    {
        // CHECKME: what if stream does not support seek? need an algorythm fork for that
        // the seek offset should be negative
        _stream->Seek(line_break_position - chars_read_last + 1 /* beyond line feed */);
    }

    str.TrimRight('\r'); // skip carriage-return, if any
    return str;
}

String TextStreamReader::ReadAll()
{
    size_t len = Math::InRangeOrDef<size_t>(
        static_cast<uint64_t>(_stream->GetLength() - _stream->GetPosition()), SIZE_MAX);
    return ReadString(len);
}

} // namespace Common
} // namespace AGS
