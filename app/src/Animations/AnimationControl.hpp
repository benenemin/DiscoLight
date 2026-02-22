//
// Created by bened on 11/11/2025.
//

#pragma once

#include <zephyr/kernel.h>

#include "Animations/BeatFlash.hpp"
#include "Animations/BeatPulse.hpp"
#include "Animations/BeatRipples.hpp"
#include "Animations/CometChase.hpp"
#include "Animations/IAnimation.hpp"
#include "Animations/LarsonScanner.hpp"
#include "Animations/RainbowWheel.hpp"
#include "Animations/SegmentChase.hpp"
#include "Animations/SolarCorona.hpp"
#include "Animations/TwinWaveInterference.hpp"
#include "Utils/Logger.hpp"
#include "Utils/PeriodicTimer.hpp"
#include "Visualization/LedControl.hpp"
#include "Visualization/LedStripController.hpp"

namespace Animations
{
enum AnimationType
{
    BeatPulseType = 0,
    LarsonScannerType,
    // RainbowWheelType,
    // CometChaseType,
    // SegmentChaseType,
    // TwinWaveInterferenceType,
    SolarCoronaType,
    BeatFlashType,
    NumAnimations
};

struct AnimationState
{
    AnimationType current_type;
    bool cycle_animations;
};

class AnimationControl final
{
public:
    explicit AnimationControl(Utils::PeriodicTimer& frame_timer,
                              Visualization::LedStripController& led_strip,
                              Visualization::LedControl& led,
                              Utils::Logger& logger)
        : current_animation_type_(BeatPulseType),
          anim_pulse_(0, 255, 6, 1),
          current_animation_(&anim_pulse_),
          frame_timer_(frame_timer),
          led_strip_(led_strip),
          led_(led),
          logger_(logger)
    {
        animation_state_.current_type = current_animation_type_;
        animation_state_.cycle_animations = false;
    }

    void Initialize()
    {
        k_mutex_init(&mutex_);
        frame_timer_.init([this] { NextFrame(); });
        logger_.info("Animation control initialized. Current animation: %s.",
                     AnimationTypeName(current_animation_type_));
    }

    void Start(const int period_us) const
    {
        frame_timer_.start(period_us);
        logger_.info("Animation frame timer started with period %d us.", period_us);
    }

    void ProcessNextBeat()
    {
        if (k_mutex_lock(&mutex_, K_MSEC(50)) != 0)
        {
            logger_.warning("Skipping beat; animation mutex lock timed out.");
            return;
        }

        current_animation_->ProcessNextBeat();
        k_mutex_unlock(&mutex_);
        logger_.debug("Processed beat for animation: %s.", AnimationTypeName(current_animation_type_));

        led_.Set(true);
        k_sleep(K_MSEC(50));
        led_.Set(false);
    }

    void IterateAnimation()
    {
        const auto selected_animation_type = animation_state_.current_type + 1;
        if (selected_animation_type >= NumAnimations)
        {
            animation_state_.cycle_animations = true;
            current_animation_type_ = BeatPulseType;
            animation_state_.current_type = BeatPulseType;
            SelectAnimation(current_animation_type_);
            logger_.info("Animation cycle mode enabled.");
            return;
        }

        animation_state_.cycle_animations = false;
        current_animation_type_ = static_cast<AnimationType>(selected_animation_type);
        animation_state_.current_type = static_cast<AnimationType>(selected_animation_type);
        SelectAnimation(current_animation_type_);
        logger_.info("Animation cycle mode disabled.");
    }

    void SelectAnimation(const AnimationType animation)
    {
        k_mutex_lock(&mutex_, K_FOREVER);
        switch (animation)
        {
        case BeatPulseType:
            current_animation_ = &anim_pulse_;
            break;
        case LarsonScannerType:
            current_animation_ = &larson_scanner_;
            break;
        // case RainbowWheelType:
        //     current_animation_ = &rainbow_wheel_;
        //     break;
        // case CometChaseType:
        //     current_animation_ = &comet_chase_;
        //     break;
        // case SegmentChaseType:
        //     current_animation_ = &segment_chase_;
        //     break;
        // case TwinWaveInterferenceType:
        //     current_animation_ = &twin_wave_interference_;
        //     break;
        case SolarCoronaType:
            current_animation_ = &solar_corona_;
            break;
        case BeatFlashType:
            current_animation_ = &beat_flash_;
            break;
        default:
            current_animation_ = &anim_pulse_;
            break;
        }
        k_mutex_unlock(&mutex_);
        logger_.info("Selected animation: %s.", AnimationTypeName(animation));
    }

private:
    static const char* AnimationTypeName(const AnimationType type)
    {
        switch (type)
        {
        case BeatPulseType:
            return "BeatPulse";
        case LarsonScannerType:
            return "LarsonScanner";
        case SolarCoronaType:
            return "SolarCorona";
        case BeatFlashType:
            return "BeatFlash";
        default:
            return "Unknown";
        }
    }

    void NextFrame()
    {
        if (animation_state_.cycle_animations && frame_counter_ % 1000 == 0)
        {
            frame_counter_ = 0;
            current_animation_type_ = static_cast<AnimationType>((current_animation_type_ + 1) % NumAnimations);
            SelectAnimation(current_animation_type_);
            logger_.info("Auto-cycled animation to: %s.",
                         AnimationTypeName(current_animation_type_));
        }

        if (k_mutex_lock(&mutex_, K_NO_WAIT) != 0)
        {
            return;
        }
        current_animation_->ProcessNextFrame(*led_strip_.GetLeds());
        k_mutex_unlock(&mutex_);

        led_strip_.UpdateLedStrip();
        ++frame_counter_;
    }

    AnimationType current_animation_type_;
    AnimationState animation_state_{};

    BeatPulse anim_pulse_;
    LarsonScanner larson_scanner_;
    RainbowWheel rainbow_wheel_;
    BeatRipples beat_ripples_;
    CometChase comet_chase_;
    SegmentChase segment_chase_;
    TwinWaveInterference twin_wave_interference_;
    SolarCorona solar_corona_;
    BeatFlash beat_flash_;
    IAnimation* current_animation_;

    Utils::PeriodicTimer& frame_timer_;
    Visualization::LedStripController& led_strip_;
    Visualization::LedControl& led_;
    Utils::Logger& logger_;
    int frame_counter_{0};
    k_mutex mutex_{};
};
} // namespace Animations
