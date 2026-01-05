//
// Created by bened on 07/10/2025.
//
#pragma once
#include <array>
#include <string>

#include "MessagePublisher.hpp"
#include "MessageSubscriber.hpp"
#include "Utils/Button.hpp"
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
        Timestamp ts;
        int sample_rate_hz; // number of valid samples in 'samples'
        array<float, Constants::SamplingFrameSize> samples; // mono PCM, 16-bit
    };

    struct ButtonEvent : BaseEvent
    {
        Timestamp ts;
        UtilsButton::ButtonState state;
    };

    struct BeatEvent : BaseEvent
    {
        array<bool, 2> bands; //beats in different frequency bands
    };

    enum class AnimCmdType : uint8_t { Next, Prev, SetIndex, SetName, Brightness };

    struct AnimCmd : BaseEvent
    {
        AnimCmdType type;
        uint16_t u16{}; // index or brightness (0..100%)
    };

    // 1) A simple typelist
    template <typename...>
    struct TypeList
    {
    };

    // 2) Define your app’s message set ONCE
    using AppMessages = TypeList<AudioFrame, BeatEvent, ButtonEvent>;

    // 3) PublisherFrom<List> -> MessagePublisher<...>
    template <typename List>
    struct PublisherFrom;

    template <typename... Ts>
    struct PublisherFrom<TypeList<Ts...>>
    {
        // keeping your requested name:
        using type = zbus_cpp::MessagePublisher<Ts...>;
    };

    // 4) SubscriberFrom<List> -> MessageSubscriber<...>
    template <typename List>
    struct SubscriberFrom;

    template <typename... Ts>
    struct SubscriberFrom<TypeList<Ts...>>
    {
        using type = zbus_cpp::MessageSubscriber<Ts...>;
    };

    // 5) Final “app types” (use these everywhere; no template lists repeated)
    using AppPublisher = PublisherFrom<AppMessages>::type;
    using AppSubscriber = SubscriberFrom<AppMessages>::type;
}
