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
// Sound decoder accompanied by various SDL-based sound utilities.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__SDLDECODER_H
#define __AGS_EE_MEDIA__SDLDECODER_H
#include <memory>
#include <vector>
#include <SDL_sound.h>
#include "util/stream.h"
#include "util/string.h"
#ifdef AUDIO_CORE_DEBUG
#include "debug/out.h"
#endif

namespace AGS
{
namespace Engine
{

using AGS::Common::Stream;
using AGS::Common::String;

// RAII wrapper over SDL_Sound sample
struct SoundSampleDeleterFunctor
{
    void operator()(Sound_Sample* p)
    {
        Sound_FreeSample(p);
#ifdef AUDIO_CORE_DEBUG
        AGS::Common::Debug::Printf("SoundSampleDeleterFunctor");
#endif
    }
};

using SoundSampleUniquePtr = std::unique_ptr<Sound_Sample, SoundSampleDeleterFunctor>;

// AudioFrameRecord describes parameters of a single audio chunk
struct AudioFrameRecord
{
    size_t Size = 0u;
    float Timestamp = -1.f; // negative means undefined
    float DurationMs = 0.f;

    AudioFrameRecord() = default;
    AudioFrameRecord(size_t sz, float ts, float dur_ms)
        : Size(sz), Timestamp(ts), DurationMs(dur_ms) {}
};

// A thin *non-owning* wrapper over a array containing constant sound data;
// meant to group and pass the buffer pointer and associated parameters.
// TODO: add full audio format
struct SoundBufferPtr
{
public:
    SoundBufferPtr() = default;
    SoundBufferPtr(const void *data, size_t sz, float ts = -1.f, float dur_ms = 0.f)
        : _data(data), _rec(sz, ts, dur_ms) {}
    SoundBufferPtr(const SoundBufferPtr &buf) = default;
    SoundBufferPtr(SoundBufferPtr &&buf) noexcept
        : _data(buf._data), _rec(buf._rec) { buf = SoundBufferPtr(); }
    SoundBufferPtr &operator=(const SoundBufferPtr &buf) = default;
    operator bool() const { return _data && Size() > 0; }

    const void *Data() const { return _data; }
    const AudioFrameRecord &GetFrameRecord() const { return _rec; }
    size_t  Size() const { return _rec.Size; }
    float   Timestamp() const { return _rec.Timestamp; }
    float   DurationMs() const { return _rec.DurationMs; }

protected:
    const void *_data = nullptr;
    AudioFrameRecord _rec;
};

// A sound buffer, holding audio data and associated parameters
struct SoundBuffer final : SoundBufferPtr
{
    SoundBuffer() = default;
    SoundBuffer(size_t sz, float ts = -1.f, float dur_ms = 0.f)
    {
        _buf.resize(sz);
        _data = _buf.data();
        _rec = AudioFrameRecord(sz, ts, dur_ms);
    }
    SoundBuffer(const void *data, size_t sz, float ts = -1.f, float dur_ms = 0.f)
    {
        _buf.resize(sz);
        _data = _buf.data();
        memcpy(_buf.data(), data, sz);
        _rec = AudioFrameRecord(sz, ts, dur_ms);
    }
    SoundBuffer(const SoundBuffer &buf)
        : SoundBufferPtr()
    {
        *this = buf;
    }
    SoundBuffer(SoundBuffer &&buf) noexcept
    {
        _buf = std::move(buf._buf);
        _data = _buf.data();
        _rec = std::move(buf._rec);
        buf = SoundBuffer();
    }

    SoundBuffer &operator=(const SoundBuffer &buf)
    {
        _buf = buf._buf;
        _data = _buf.data();
        _rec = buf._rec;
        return *this;
    }

    void AssignData(const void *data, size_t sz, float ts = -1.f, float dur_ms = 0.f)
    {
        if (_buf.size() != sz)
        {
            _buf.resize(sz);
            _data = _buf.data();
        }
        memcpy(_buf.data(), data, sz);
        _rec = AudioFrameRecord(sz, ts, dur_ms);
    }

    void SetTimestamp(float ts)
    {
        _rec.Timestamp = ts;
    }

private:
    std::vector<uint8_t> _buf;
};

// RAII wrapper over SDL resampling filter;
// initialized by passing input and desired sound format;
// tells whether conversion is necessary and performs one on command.
struct SDLResampler
{
public:
    SDLResampler() = default;
    SDLResampler(SDL_AudioFormat src_fmt, int src_chans, int src_rate,
        SDL_AudioFormat dst_fmt, int dst_chans, int dst_rate)
    {
        Setup(src_fmt, src_chans, src_rate, dst_fmt, dst_chans, dst_rate);
    }
    // Tells if conversion is necessary
    bool HasConversion() const { return _cvt.needed > 0; }
    // Setup a new conversion; returns whether setup has succeeded;
    // note that if no conversion necessary it still considered a success.
    bool Setup(SDL_AudioFormat src_fmt, int src_chans, int src_rate,
        SDL_AudioFormat dst_fmt, int dst_chans, int dst_rate);
    bool Setup(const Sound_AudioInfo &src, const Sound_AudioInfo &dst)
        { return Setup(src.format, src.channels, src.rate, dst.format, dst.channels, dst.rate); }
    // Converts given sound data, on success returns a read-only pointer to the
    // memory containing resulting data, and fills out_sz with output length value;
    // note that if no conversion is required it does not perform any operation
    // whatsoever and returns the input pointer.
    const void *Convert(const void *data, size_t sz, size_t &out_sz);

private:
    SDL_AudioCVT _cvt{};
    std::vector<uint8_t> _buf;
};

// SDLDecoder uses SDL_Sound library to decode audio and retrieve result
// in parts of the requested size.
class SDLDecoder
{
public:
    // Initializes decoder with a complete sound data loaded to memory
    SDLDecoder(std::shared_ptr<std::vector<uint8_t>> &data, const String &ext_hint, bool repeat);
    // Initializes decoder with an input stream
    SDLDecoder(const std::unique_ptr<Stream> in, const String &ext_hint, bool repeat);
    SDLDecoder(SDLDecoder&& dec);
    ~SDLDecoder() = default;

    // Tells if the decoder is in a valid state, ready to work
    bool IsValid() const { return _sample != nullptr; }
    // Gets the audio format
    SDL_AudioFormat GetFormat() const { return _sample ? _sample->desired.format : 0; }
    // Gets the number of channels
    int GetChannels() const { return _sample ? _sample->desired.channels : 0; }
    // Gets the audio rate (frequency)
    int GetFreq() const { return _sample ? _sample->desired.rate : 0; }
    // Tells if the data reading has reached EOS
    bool EOS() const { return _EOS; }
    // Gets current reading position, in ms
    float GetPositionMs() const { return _posMs; }
    // Gets total duration, in ms
    float GetDurationMs() const { return _durationMs; }

    // Try initializing the sound sample, returns the result
    bool Open(float pos_ms = 0.f);
    // Closes decoder, releases any owned resources
    void Close();
    // Seeks to the given read position; returns the new position
    float Seek(float pos_ms);
    // Returns the next chunk of data; may return empty buffer in EOS or error
    SoundBufferPtr GetData();

private:
    SDL_RWops *_rwops = nullptr;
    std::shared_ptr<std::vector<uint8_t>> _sampleData{};
    String _sampleExt = "";
    SoundSampleUniquePtr _sample = nullptr;
    float _durationMs = 0.f;
    bool _repeat = false;
    bool _EOS = false;
    size_t _posBytes = 0u;
    float _posMs = 0.f;
};


namespace SoundHelper
{
    // Tells bytes per sample from SDL_Audio format
    inline size_t BytesPerSample(SDL_AudioFormat format) { return (SDL_AUDIO_BITSIZE(format) + 7) / 8; }
    // Calculate number of bytes of sound data per millisecond
    inline size_t BytesPerMs(float ms, SDL_AudioFormat format, int chans, int freq)
    {
        return static_cast<size_t>(
            (static_cast<double>(ms) * SDL_AUDIO_BITSIZE(format) * chans * freq) / (8 * 1000));
    }
    // Calculate number of milliseconds from given number of bytes of sound data
    inline float MillisecondsFromBytes(size_t bytes, SDL_AudioFormat format, int chans, int freq)
    {
        return static_cast<float>
            (static_cast<double>(bytes) * 8 * 1000) / (SDL_AUDIO_BITSIZE(format) * chans * freq);
    }
} // namespace SoundHelper

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MEDIA__AUDIOUTILS_H
