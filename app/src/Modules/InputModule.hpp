//
// Created by bened on 07/12/2025.
//
#pragma once

#include "Core/EventTypes.hpp"
#include "Utils/Button.hpp"
#include "Utils/Logger.hpp"
#include "Utils/MonotonicClock.hpp"

namespace Modules
{
    class InputModule final
    {
    public:
        explicit InputModule(
            Core::EventTypes::AppPublisher& publisher,
            UtilsButton::Button& button,
            Utils::MonotonicClock& clock,
            Utils::Logger& logger)
            : publisher_(publisher), button_(button), clock_(clock), logger_(logger)
        {
            button_.AttachLogger(logger_);
        }

        int Initialize()
        {
            logger_.info("Initializing button input module.");
            const auto ret = button_.Initialize([this](const UtilsButton::ButtonState state)
            {
                OnButtonEvent(state);
            });

            if (ret != 0)
            {
                logger_.error("Button input module initialization failed: %d", ret);
                return ret;
            }

            logger_.info("Button input module initialized.");
            return ret;
        }

    private:
        void OnButtonEvent(UtilsButton::ButtonState state)
        {
            logger_.info("Button event received: %d", state);

            Core::EventTypes::ButtonEvent button_event{};
            button_event.ts = clock_.Now();
            button_event.state = state;
            if (const auto err = publisher_.Publish(button_event))
            {
                logger_.error("publishing error: %d", err);
                return;
            }

            logger_.debug("Button event published.");
        }

        Core::EventTypes::AppPublisher& publisher_;
        UtilsButton::Button& button_;
        Utils::MonotonicClock& clock_;
        Utils::Logger& logger_;
    };
} // namespace Modules
