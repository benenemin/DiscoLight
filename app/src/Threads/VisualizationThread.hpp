//
// Created by bened on 02/11/2025.
//

#pragma once
#include "Visualization/LedControl.hpp"
#include "Visualization/LedStripController.hpp"

namespace Threads
{
    template<size_t LedCount>
class VisualizationThread final : SubscriberBase<BeatEvent>
{
public:
    VisualizationThread(MessageBus &message_bus, ThreadWorker &threadWorker, Logger &logger, Visualization::LedStripController<LedCount> &ledStrip, Visualization::LedControl &led)
    : SubscriberBase(message_bus, threadWorker), logger_(logger), led_strip_(ledStrip), led_(led)
{

    }

    void Initialize(const int prio) override
    {
        SubscriberBase::Initialize(prio);
    }

    void Start() override
    {
        SubscriberBase::Start();
        this->logger_.info("Visualization module started.");
    }

protected:
    void Notify(BeatEvent& event) override
    {
        auto delta = event.ts.GetMs() - last_event_ms_;
        last_event_ms_ = event.ts.GetMs();

        if (delta < 200.0f)
        {
            return;
        }
        logger_.info("Visualization module here.");
        led_.Set(true);
        led_strip_.FlashColor(0, 0, 255);
        k_sleep(K_MSEC(50));
        led_.Set(false);
        led_strip_.FlashColor(0, 0, 0);
    }

private:
   Logger &logger_;
    Visualization::LedStripController<LedCount>& led_strip_;
    Visualization::LedControl& led_;

        float last_event_ms_ = 0;
};
}
