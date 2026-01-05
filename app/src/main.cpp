/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include "zephyr/arch/xtensa/thread_stack.h"
#include "zephyr/drivers/gpio.h"

#include "Constants.hpp"

#include "Core/MessagePublisher.hpp"
#include "Core/MessageSubscriber.hpp"
#include "Core/ThreadWorker.hpp"
#include "Modules/AudioSamplingModule.hpp"
#include "Modules/AudioProcessingModule.hpp"
#include "Modules/VisualizationModule.hpp"
#include "Modules/InputModule.hpp"
#include "ADC/AdcReader.hpp"
#include "Core/EventTypes.hpp"
#include "Visualization/LedControl.hpp"
#include "Visualization/LedStripController.hpp"
#include "SignalProcessing/FftProcessor.hpp"
#include "SignalProcessing/LpFilter.hpp"


#include "Utils/Logger.hpp"
#include "Utils/PeriodicTimer.hpp"
#include "Utils/Button.hpp"
#include "Utils/LoadSwitch.hpp"

using Adc::AdcReader;

LOG_MODULE_REGISTER(App, CONFIG_APP_LOG_LEVEL);


//////////////////////////////////////////////////////////////////////////////////
/************************ device constants **************************************/
//////////////////////////////////////////////////////////////////////////////////

/* Data of ADC io-channels specified in devicetree. */
static constexpr adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
                         DT_SPEC_AND_COMMA)
};

static const device* const strip = DEVICE_DT_GET(STRIP_NODE);
static gpio_dt_spec ctrlButton = GPIO_DT_SPEC_GET(CTRL_BTN_NODE, gpios);
static gpio_dt_spec signalLed= GPIO_DT_SPEC_GET(DT_NODELABEL(sigled), gpios);
static gpio_dt_spec loadSwitch = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(load_switch), gpios, {0});

ZBUS_SUBSCRIBER_DEFINE(app_sub, 8);

ZBUS_CHAN_DEFINE_WITH_ID(
    AudioChannelBus,
    0x348756834,
    Core::EventTypes::AudioFrame,
    NULL, NULL,
    ZBUS_OBSERVERS(app_sub),
    ZBUS_MSG_INIT({})
);

ZBUS_CHAN_DEFINE_WITH_ID(
    BeatChannelBus,
    0x348756835,
    Core::EventTypes::BeatEvent,
    NULL, NULL,
    ZBUS_OBSERVERS(app_sub),
    ZBUS_MSG_INIT({})
);

ZBUS_CHAN_DEFINE_WITH_ID(
    ButtonChannelBus,
    0x348756836,
    Core::EventTypes::ButtonEvent,
    NULL, NULL,
    ZBUS_OBSERVERS(app_sub),
    ZBUS_MSG_INIT({})
);


//////////////////////////////////////////////////////////////////////////////////
/****************************** DI **********************************************/
//////////////////////////////////////////////////////////////////////////////////

auto publisher = zbus_cpp::MessagePublisher(
    zbus_cpp::Topic<Core::EventTypes::AudioFrame>{&AudioChannelBus},
    zbus_cpp::Topic<Core::EventTypes::BeatEvent>{&BeatChannelBus},
    zbus_cpp::Topic<Core::EventTypes::ButtonEvent>{&ButtonChannelBus});

K_THREAD_STACK_DEFINE(messaging_thread_stack, Constants::SamplingFrameSize * 4 + 2048);
auto a = ThreadWorker(*messaging_thread_stack, K_THREAD_STACK_SIZEOF(messaging_thread_stack));
auto subscriber = zbus_cpp::MessageSubscriber(
    a,
    &app_sub,
    zbus_cpp::Topic<Core::EventTypes::AudioFrame>{&AudioChannelBus},
    zbus_cpp::Topic<Core::EventTypes::BeatEvent>{&BeatChannelBus},
    zbus_cpp::Topic<Core::EventTypes::ButtonEvent>{&ButtonChannelBus});

auto timer = PeriodicTimer();
auto adcLogger = Logger("ADC_READER");
auto adc_reader = AdcReader(&adc_channels[0], timer, adcLogger);

auto audioSamplingLogger = Logger("AUDIO_SAMPLING");
auto audioSamplingModule = Modules::AudioSamplingModule(adc_reader, publisher, subscriber, audioSamplingLogger);

auto ledLogger = Logger("LED_CONTROL");
auto led = Visualization::LedControl(&signalLed, ledLogger);
auto ledStripLogger = Logger("LED_STRIP");
auto ledStripController = Visualization::LedStripController(strip, ledStripLogger);

auto lpFilter = LpFilter();
auto fftProcessor = FftProcessor();
auto beatDetector = BeatDetector(fftProcessor, lpFilter);
auto audioProcessingLogger = Logger("AUDIO_PROCESSING");
auto audioProcessingModule = Modules::AudioProcessingModule(publisher, subscriber, beatDetector, audioProcessingLogger);

auto visualizationLogger = Logger("VISUALIZATION");
auto frameTimer = PeriodicTimer();
auto loadSwitchLogger = Logger("LOAD_SWITCH");
auto loadSwitchControl = LoadSwitch(&loadSwitch, loadSwitchLogger);
auto animationControl = Animations::AnimationControl(frameTimer, ledStripController, led);
auto visualizationModule = Modules::VisualizationModule(subscriber, visualizationLogger, animationControl, loadSwitchControl);

auto animCtrlButton = UtilsButton::Button(ctrlButton);
auto buttonLogger = Logger("BUTTON");
auto inputModule = Modules::InputModule(publisher, animCtrlButton, buttonLogger);


///////////////////////////////////////////////////////////////////////////////////
/************************ application start **************************************/
///////////////////////////////////////////////////////////////////////////////////

int main()
{
    LOG_INF("App started.");

    subscriber.Initialize(1);
    subscriber.Start();

    //Module initialization
    inputModule.Initialize();
    audioSamplingModule.Initialize();
    audioProcessingModule.Initialize();
    visualizationModule.Initialize();

    //Module startup
    audioProcessingModule.Start();
    audioSamplingModule.Start();
    visualizationModule.Start();

    LOG_INF("Application running...");

    //take main thread to sleep
    k_sleep(K_FOREVER);
}
