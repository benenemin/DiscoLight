//
// Created by bened on 11/11/2025.
//

#pragma once


struct led_rgb;

namespace Animations
{
  template<size_t LedCount>
  class IAnimation
  {
    public:
    virtual ~IAnimation() = default;

    using LedChain = array<led_rgb, LedCount>;
    virtual void ProcessNextFrame(LedChain& leds) = 0;
    virtual void ProcessNextBeat() = 0;
  };
}
