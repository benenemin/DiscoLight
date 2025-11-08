//
// Created by bened on 07/10/2025.
//
#pragma once

namespace Utils::TimeStamp
{
    struct Timestamp
    {
        float GetUs() const
        {
            return nSec/1000.0f;
        }

        float GetMs() const
        {
            return GetUs()/1000.0f;
        }

        uint64_t nSec {};
    };
}
