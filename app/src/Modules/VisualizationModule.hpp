//
// Created by bened on 02/11/2025.
//

#pragma once

#include "Animations/AnimationControl.hpp"
#include "Core/EventTypes.hpp"
#include "Utils/LoadSwitch.hpp"
#include "Utils/Logger.hpp"

namespace Modules
{
    class VisualizationModule final
    {
    public:
        VisualizationModule(
            Core::EventTypes::AppSubscriber& subscriber,
            Utils::Logger& logger,
            Animations::AnimationControl& animation_control,
            Utils::LoadSwitch& load_switch)
            : subscriber_(subscriber),
              animation_control_(animation_control),
              load_switch_(load_switch),
              logger_(logger)
        {
        }

        void Initialize()
        {
            logger_.info("Initializing visualization module.");
            animation_control_.Initialize();
            logger_.info("Visualization module initialized.");
        }

        void Start()
        {
            logger_.info("Starting visualization module.");
            animation_control_.Start(10'000);
            const auto beat_sub_ret = subscriber_.Subscribe<Core::EventTypes::BeatEvent>(
                [this](const Core::EventTypes::BeatEvent&)
            {
                OnBeatEvent();
            });
            if (beat_sub_ret != 0)
            {
                logger_.error("Failed to subscribe to beat events: %d", beat_sub_ret);
            }

            const auto button_sub_ret = subscriber_.Subscribe<Core::EventTypes::ButtonEvent>(
                [this](const Core::EventTypes::ButtonEvent& event)
                {
                    OnButtonEvent(event);
                });
            if (button_sub_ret != 0)
            {
                logger_.error("Failed to subscribe to button events: %d", button_sub_ret);
            }
            load_switch_.Close();
            logger_.info("Visualization module subscribed to beat and button events.");
            logger_.info("Visualization module started.");
        }

    private:
        void OnBeatEvent()
        {
            logger_.debug("Beat event received.");
            animation_control_.ProcessNextBeat();
        }

        void OnButtonEvent(const Core::EventTypes::ButtonEvent& event)
        {
            logger_.info("Button event: %d", event.state);
            if (event.state == UtilsButton::ButtonState::ReleasedShort)
            {
                logger_.info("Short button release: selecting next animation.");
                animation_control_.IterateAnimation();
                return;
            }

            if (event.state == UtilsButton::ButtonState::ReleasedLong)
            {
                logger_.info("Long button release received (no action configured).");
            }
        }

        Core::EventTypes::AppSubscriber& subscriber_;
        Animations::AnimationControl& animation_control_;
        Utils::LoadSwitch& load_switch_;
        Utils::Logger& logger_;
    };
} // namespace Modules
