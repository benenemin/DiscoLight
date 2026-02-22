//
// Created by bened on 21/02/2026.
//

#pragma once

#include "ADC/AdcReader.hpp"
#include "Animations/AnimationControl.hpp"
#include "Core/EventTypes.hpp"
#include "Core/ThreadWorker.hpp"
#include "Modules/AudioProcessingModule.hpp"
#include "Modules/AudioSamplingModule.hpp"
#include "Modules/InputModule.hpp"
#include "Modules/VisualizationModule.hpp"
#include "SignalProcessing/BeatDetector.hpp"
#include "SignalProcessing/FftProcessor.hpp"
#include "SignalProcessing/LpFilter.hpp"
#include "Utils/Button.hpp"
#include "Utils/LoadSwitch.hpp"
#include "Utils/Logger.hpp"
#include "Utils/MonotonicClock.hpp"
#include "Utils/PeriodicTimer.hpp"
#include "Visualization/LedControl.hpp"
#include "Visualization/LedStripController.hpp"

namespace App
{
class Application final
{
public:
    Application();

    void Initialize();
    void Start();

private:
    static constexpr int kSubscriberPriority = 1;

    Utils::Logger app_logger_;
    Utils::MonotonicClock event_clock_{};
    Core::EventTypes::AppPublisher publisher_;
    Utils::Logger messaging_logger_;
    Utils::ThreadWorker messaging_worker_;
    Core::EventTypes::AppSubscriber subscriber_;

    Utils::PeriodicTimer adc_timer_{};
    Utils::Logger adc_logger_;
    Adc::AdcReader adc_reader_;
    Utils::Logger audio_sampling_logger_;
    Modules::AudioSamplingModule audio_sampling_module_;

    Utils::Logger led_logger_;
    Visualization::LedControl led_;
    Utils::Logger led_strip_logger_;
    Visualization::LedStripController led_strip_controller_;

    SignalProcessing::LpFilter lp_filter_{};
    SignalProcessing::FftProcessor fft_processor_{};
    SignalProcessing::BeatDetector beat_detector_;
    Utils::Logger audio_processing_logger_;
    Modules::AudioProcessingModule audio_processing_module_;

    Utils::Logger visualization_logger_;
    Utils::PeriodicTimer frame_timer_{};
    Utils::Logger load_switch_logger_;
    Utils::LoadSwitch load_switch_;
    Utils::Logger animation_logger_;
    Animations::AnimationControl animation_control_;
    Modules::VisualizationModule visualization_module_;

    UtilsButton::Button control_button_;
    Utils::Logger button_logger_;
    Modules::InputModule input_module_;
};
} // namespace App
