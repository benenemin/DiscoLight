//
// Created by bened on 07/12/2025.
//
#pragma once
#include "Utils/Button.hpp"

namespace Modules
{
    class InputModule
    {
    public:
        explicit InputModule(AppPublisher& publisher, UtilsButton::Button& button,
                                   Logger& logger)
            : button_(button), logger_(logger), publisher_(publisher)
        {
        }

        int Initialize() const
        {
            timing_init();
            timing_start();
            const auto ret = this->button_.Initialize([this](const UtilsButton::ButtonState evt)
            {
                logger_.info("button pressed: %d", evt);
                auto buttonEvent = Core::EventTypes::ButtonEvent();
                auto timestamp = Timestamp();
                timestamp.nSec = timing_cycles_to_ns(timing_counter_get());

                buttonEvent.ts = timestamp;
                buttonEvent.state = evt;
                if (const auto err = publisher_.Publish(buttonEvent))
                {
                    logger_.error("publishing error: %d", err);
                }
            });

            logger_.info("ButtonInputThread Initialized");
            return ret;
        }

    private:
        UtilsButton::Button& button_;
        Logger& logger_;
        AppPublisher& publisher_;
    };
};
