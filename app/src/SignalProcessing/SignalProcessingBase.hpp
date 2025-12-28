//
// Created by bened on 11/11/2025.
//

#pragma once

namespace SignalProcessing
{
    template <size_t FrameSize>
    class SignalProcessingBase
    {
    protected:
        virtual ~SignalProcessingBase() = default;

    public:
        virtual int Initialize() = 0;
        virtual bool Process(array<float, FrameSize>& samples) = 0;
    };
}
