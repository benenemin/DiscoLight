//
// Created by bened on 07/10/2025.
//
#pragma once
#include <array>
#include <string>

#include "Utils/TimeStamp.hpp"

using namespace std;
using namespace Utils::TimeStamp;

namespace Core::EventTypes
{
    struct BaseEvent
    {
        Timestamp ts;
    };


    struct AudioFrame : BaseEvent
    {
        int  sample_rate_hz;             // number of valid samples in 'samples'
        array<float, 512> samples;    // mono PCM, 16-bit
    };

    struct FeatureFrame : BaseEvent
    {
        float     energy;                     // short-window energy (EMA/RMS)
        float     flux;                       // spectral flux or 0 if disabled
    };

    struct BeatEvent : BaseEvent
    {
        array<bool, 2> bands;                   //beats in different frequency bands
    };

    struct TickEvent : BaseEvent
    {
        uint16_t fps_target;                  // e.g., 100
        uint16_t dt_ms;                       // time since last tick (ms)
    };


    enum class AnimCmdType : uint8_t { Next, Prev, SetIndex, SetName, Brightness };
    struct AnimCmd : BaseEvent
    {
        AnimCmdType type;
        uint16_t    u16 {};                   // index or brightness (0..100%)
    };
}
