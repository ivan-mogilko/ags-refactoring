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
#include "media/audio/sdldecoder.h"
#include <algorithm>

namespace AGS
{
namespace Engine
{

//-----------------------------------------------------------------------------
// SDLResampler
//-----------------------------------------------------------------------------
bool SDLResampler::Setup(SDL_AudioFormat src_fmt, int src_chans, int src_rate,
    SDL_AudioFormat dst_fmt, int dst_chans, int dst_rate)
{
    SDL_zero(_cvt);
    return SDL_BuildAudioCVT(&_cvt, src_fmt, static_cast<uint8_t>(src_chans), src_rate,
        dst_fmt, static_cast<uint8_t>(dst_chans), dst_rate) >= 0;
}

const void *SDLResampler::Convert(const void *data, size_t sz, size_t &out_sz)
{
    if (_cvt.needed == 0)
    { // no conversion necessary, return the input pointer
        out_sz = sz;
        return data;
    }
    // Reallocate if necessary and perform the conversion
    if (_buf.size() < sz * _cvt.len_mult)
    {
        size_t len = sz * _cvt.len_mult;
        _buf.resize(len);
    }
    _cvt.buf = &_buf[0];
    _cvt.len = _cvt.len_cvt = sz;
    SDL_memcpy(_cvt.buf, data, sz);
    if (SDL_ConvertAudio(&_cvt) < 0)
        return nullptr;
    out_sz = _cvt.len_cvt;
    return &_buf[0];
}

//-----------------------------------------------------------------------------
// SDLDecoder
//-----------------------------------------------------------------------------
const auto SampleDefaultBufferSize = 64 * 1024;

SDLDecoder::SDLDecoder(const std::vector<char> &data,
    const AGS::Common::String &ext_hint, bool repeat)
    : _sampleData(std::move(data))
    , _sampleExt(ext_hint)
    , _repeat(repeat)
{
}

SDLDecoder::SDLDecoder(SDLDecoder &&dec)
{
    _sampleData = (std::move(dec._sampleData));
    _sampleExt = std::move(dec._sampleExt);
    _repeat = dec._repeat;
}

bool SDLDecoder::Open(float pos_ms)
{
    Close();

    auto sample = SoundSampleUniquePtr(Sound_NewSampleFromMem(
        (uint8_t*)_sampleData.data(), _sampleData.size(), _sampleExt.GetCStr(), nullptr, SampleDefaultBufferSize));
    if (!sample) {
        return false;
    }

    _sample = std::move(sample);
    _durationMs = Sound_GetDuration(_sample.get());
    _posBytes = 0u;
    _posMs = 0u;
    if (pos_ms > 0.f) {
        Seek(pos_ms);
    }
    return true;
}

void SDLDecoder::Close()
{
    _sample.reset();
}

float SDLDecoder::Seek(float pos_ms)
{
    if (!_sample || pos_ms < 0.f) return static_cast<float>(_posMs);
    if (Sound_Seek(_sample.get(), static_cast<uint32_t>(pos_ms)) == 0)
        return static_cast<float>(_posMs); // old pos on failure (CHECKME?)
    _posMs = static_cast<uint32_t>(pos_ms);
    _posBytes = SoundHelper::BytesPerMs(_posMs,
        _sample->desired.format, _sample->desired.channels, _sample->desired.rate);
    return pos_ms; // new pos on success
}

SoundBuffer SDLDecoder::GetData()
{
    if (!_sample || _EOS) { return SoundBuffer(); }
    uint32_t old_pos = _posMs;
    auto sz = Sound_Decode(_sample.get());
    _posBytes += sz;
    _posMs = SoundHelper::MillisecondsFromBytes(_posBytes,
        _sample->desired.format, _sample->desired.channels, _sample->desired.rate);
    // If read less than the buffer size - that means
    // either we reached end of sound stream OR decoding error occured
    if (sz < _sample->buffer_size) {
        _EOS = true;
        if ((_sample->flags & SOUND_SAMPLEFLAG_ERROR) != 0) {
            return SoundBuffer();
        }
        // if repeat, then seek to start.
        else if (_repeat) {
            _EOS = Sound_Rewind(_sample.get()) == 0;
            _posBytes = 0u;
            _posMs = 0u;
        }
    }
    return SoundBuffer(_sample->buffer, sz, static_cast<float>(old_pos), static_cast<float>(
        SoundHelper::MillisecondsFromBytes(sz, _sample->desired.format, _sample->desired.channels, _sample->desired.rate)));
}


size_t SoundHelper::SplitChannels(const uint8_t *src, size_t src_len, uint8_t *dst1,
    uint8_t *dst2, size_t dst_len, SDL_AudioFormat format)
{
    size_t bps = BytesPerSample(format);
    src_len = std::min(src_len / 2, dst_len) * 2;
    switch (bps)
    {
    case 1:
        for (const uint8_t *src_end = src + src_len; src != src_end;)
        {
            *(dst1++) = *(src++);
            *(dst2++) = *(src++);
        }
        break;
    case 2:
        {
            const uint16_t *u16src = reinterpret_cast<const uint16_t*>(src);
            uint16_t *u16dst1 = reinterpret_cast<uint16_t*>(dst1);
            uint16_t *u16dst2 = reinterpret_cast<uint16_t*>(dst2);
            for (const uint16_t *src_end = u16src + src_len / sizeof(uint16_t); u16src != src_end;)
            {
                *(u16dst1++) = *(u16src++);
                *(u16dst2++) = *(u16src++);
            }
        }
        break;
    case 4:
        {
            const uint32_t *u32src = reinterpret_cast<const uint32_t*>(src);
            uint32_t *u32dst1 = reinterpret_cast<uint32_t*>(dst1);
            uint32_t *u32dst2 = reinterpret_cast<uint32_t*>(dst2);
            for (const uint32_t *src_end = u32src + src_len / sizeof(uint32_t); u32src != src_end;)
            {
                *(u32dst1++) = *(u32src++);
                *(u32dst2++) = *(u32src++);
            }
        }
        break;
    default:
        return 0;
    }
    return src_len / 2;
}

} // namespace Engine
} // namespace AGS
