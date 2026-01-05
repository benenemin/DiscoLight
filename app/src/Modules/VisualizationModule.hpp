//
// Created by bened on 02/11/2025.
//

#pragma once

#include "Animations/AnimationControl.hpp"
#include "Core/EventTypes.hpp"
#include "Utils/LoadSwitch.hpp"

namespace Modules
{
    template <size_t LedCount>
    class VisualizationThread final
    {
    public:
        VisualizationThread(Core::EventTypes::AppSubscriber& subscriber, Logger& logger,
                            Animations::AnimationControl<LedCount>& animationControl, LoadSwitch &loadSwitch)
            : logger_(logger), animation_control_(animationControl), subscriber_(subscriber), load_switch_(loadSwitch)
        {
        }

        void Initialize(const int prio)
        {
            animation_control_.Initialize();
        }

        void Start()
        {
            animation_control_.Start(10'000);
            subscriber_.Subscribe<Core::EventTypes::BeatEvent>([&](const Core::EventTypes::BeatEvent& event)
            {
                Notify();
            });
            subscriber_.Subscribe<Core::EventTypes::ButtonEvent>([&](const Core::EventTypes::ButtonEvent& event)
            {
                logger_.info("Button event: %d", event.state);
                if (event.state == UtilsButton::ButtonState::ReleasedShort)
                {
                    animation_control_.IterateAnimation();
                }
            });
            this->logger_.info("Visualization module started.");
        }

    private:
        void Notify()
        {
            animation_control_.ProcessNextBeat();
        }

        Logger& logger_;

        float last_event_ms_ = 0;
        Animations::AnimationControl<LedCount>& animation_control_;
        Core::EventTypes::AppSubscriber& subscriber_;
        LoadSwitch &load_switch_;
    };
}
