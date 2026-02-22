//
// Created by bened on 21/02/2026.
//

#pragma once

#include <zephyr/timing/timing.h>

#include "Utils/TimeStamp.hpp"

namespace Utils
{
class MonotonicClock final
{
public:
    void Initialize()
    {
        timing_init();
        timing_start();
    }

    [[nodiscard]] TimeStamp::Timestamp Now() const
    {
        TimeStamp::Timestamp ts{};
        ts.nSec = timing_cycles_to_ns(timing_counter_get());
        return ts;
    }
};
} // namespace Utils
