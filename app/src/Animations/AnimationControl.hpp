//
// Created by bened on 11/11/2025.
//

#pragma once

#include "IAnimation.hpp"
#include "Animations/BeatPulse.hpp"
#include "Animations/LarsonScanner.hpp"
#include "Animations/RainbowWeel.hpp"
#include "Animations/BeatRipples.hpp"
#include "Animations/CometChase.hpp"
#include "Animations/SegmentChase.hpp"
#include "Animations/TwinWaveInterference.hpp"
#include "Animations/SolarCorona.hpp"
#include "Animations/BeatFlash.hpp"
#include "Visualization/LedControl.hpp"
#include "Visualization/LedStripController.hpp"


namespace Visualization
{
    class LedControl;
}

namespace Animations
{
    enum AnimationType
    {
        BeatPulseType = 0,
        LarsonScannerType,
        //RainbowWheelType,
        //CometChaseType,
        //SegmentChaseType,
        //TwinWaveInterferenceType,
        SolarCoronaType,
        BeatFlashType,
        NumAnimations
    };

    struct Animation
    {
        AnimationType currentType;
        bool cycleAnimations;
    };

    template <size_t LedCount>
    class AnimationControl
    {
    public:
        explicit AnimationControl(PeriodicTimer& frameTimer, Visualization::LedStripController<LedCount>& ledStrip, Visualization::LedControl& led)
            : currentAnimationType(BeatPulseType), frame_timer_(frameTimer), led_strip_(ledStrip), led_(led), mutex_()
        {
            anim_pulse = BeatPulse<LedCount>(0 /*hue*/, 255 /*sat*/, 6 /*decay*/, 1);
            larson_scanner = LarsonScanner<LedCount>();
            rainbow_wheel = RainbowWheel < LedCount > ();
            comet_chase = CometChase < LedCount > ();
            segment_chase = SegmentChase < LedCount > ();
            solar_corona = SolarCorona < LedCount > ();
            beat_flash = BeatFlash < LedCount > ();
            currentAnimation = &beat_flash;
            animationState.currentType = currentAnimationType;
            animationState.cycleAnimations = false;
        }

        void Initialize()
        {
            k_mutex_init(&this->mutex_);
            frame_timer_.init([this] { NextFrame(); });
        }

        void Start(const int period_us) const
        {
            frame_timer_.start(period_us);
        }

        void ProcessNextBeat()
        {
            if (k_mutex_lock(&this->mutex_, K_MSEC(50)))
            {
                return;
            }
            currentAnimation->ProcessNextBeat();
            k_mutex_unlock(&this->mutex_);
            led_.Set(true);
            k_sleep(K_MSEC(50));
            led_.Set(false);
        }

        void IterateAnimation()
        {
            auto selectedAnimationType = animationState.currentType + 1;
            if (selectedAnimationType >= NumAnimations)
            {
                animationState.cycleAnimations = true;
                currentAnimationType = BeatPulseType;
                animationState.currentType = BeatPulseType;
                SelectAnimation(currentAnimationType);
                return;
            }

            animationState.cycleAnimations = false;
            currentAnimationType = static_cast<AnimationType>(selectedAnimationType);
            animationState.currentType = static_cast<AnimationType>(selectedAnimationType);
            SelectAnimation(currentAnimationType);
        }

        void SelectAnimation(const AnimationType animation)
        {
            k_mutex_lock(&this->mutex_, K_FOREVER);
            switch (animation)
            {
            case BeatPulseType:
                currentAnimation = &anim_pulse;
                break;
            case LarsonScannerType:
                currentAnimation = &larson_scanner;
                break;
            // case RainbowWheelType:
            //     currentAnimation = &rainbow_wheel;
            //     break;
            // case CometChaseType:
            //     currentAnimation = &comet_chase;
            //     break;
            // case SegmentChaseType:
            //     currentAnimation = &segment_chase;
            //     break;
            // case TwinWaveInterferenceType:
            //     currentAnimation = &twin_wave_interference;
            //     break;
            case SolarCoronaType:
                currentAnimation = &solar_corona;
                break;
            case BeatFlashType:
                currentAnimation = &beat_flash;
                break;
            default:
                currentAnimation = &anim_pulse;
            }
            k_mutex_unlock(&this->mutex_);
        }

    private:
                void NextFrame()
        {
            if (animationState.cycleAnimations && frame_counter%1000 == 0)
            {
                frame_counter = 0;
                currentAnimationType = static_cast<AnimationType>((currentAnimationType + 1) % NumAnimations);
                SelectAnimation(currentAnimationType);
            }

            if (k_mutex_lock(&this->mutex_, K_NO_WAIT) < 0)
            {
                return;
            }
            currentAnimation->ProcessNextFrame(*led_strip_.GetLeds());
            k_mutex_unlock(&this->mutex_);

            led_strip_.UpdateLedStrip();
            frame_counter++;
        }

        IAnimation<LedCount>* currentAnimation;
        AnimationType currentAnimationType;
        Animation animationState;

        BeatPulse<LedCount> anim_pulse;
        LarsonScanner<LedCount> larson_scanner;
        RainbowWheel<LedCount> rainbow_wheel;
        BeatRipples<LedCount> beat_ripples;
        CometChase<LedCount> comet_chase;
        SegmentChase<LedCount> segment_chase;
        TwinWaveInterference<LedCount> twin_wave_interference;
        SolarCorona<LedCount> solar_corona;
        BeatFlash<LedCount> beat_flash;

        PeriodicTimer& frame_timer_;
        Visualization::LedStripController<LedCount>& led_strip_;
        Visualization::LedControl& led_;

        int frame_counter = 0;

        k_mutex mutex_;
    };
};
