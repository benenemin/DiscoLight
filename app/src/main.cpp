/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include "zephyr/arch/xtensa/thread_stack.h"
#include "zephyr/drivers/gpio.h"

#include "Core/MessagePublisher.hpp"
#include "Core/MessageSubscriber.hpp"
#include "Threads/ThreadWorker.hpp"
#include "Threads/AudioSampling.hpp"
#include "Threads/AudioProcessing.hpp"
#include "Threads/VisualizationThread.hpp"
#include "Threads/ButtonInputThread.hpp"
#include "ADC/AdcReader.hpp"
#include "Core/EventTypes.hpp"
#include "Visualization/LedControl.hpp"
#include "Visualization/LedStripController.hpp"
#include "SignalProcessing/FftProcessor.hpp"
#include "SignalProcessing/LpFilter.hpp"


#include "Utils/Logger.hpp"
#include "Utils/PeriodicTimer.hpp"
#include "Utils/Button.hpp"

using Adc::AdcReader;

LOG_MODULE_REGISTER(App, CONFIG_APP_LOG_LEVEL);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define STRIP_NODE		DT_ALIAS(led_strip)
#if DT_NODE_HAS_PROP(STRIP_NODE, chain_length)
#define STRIP_NUM_PIXELS	DT_PROP(STRIP_NODE, chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define CTRL_BTN_NODE DT_ALIAS(ctrl_btn)

// #define STRIP_SPI_NODE		DT_ALIAS(led_strip_spi)
// #if DT_NODE_HAS_PROP(STRIP_SPI_NODE, chain_length)
// #define STRIP_NUM_PIXELS_SPI	DT_PROP(STRIP_SPI_NODE, chain_length)
// #else
// #error Unable to determine length of LED strip
// #endif

//////////////////////////////////////////////////////////////////////////////////
/************************ global constants **************************************/
//////////////////////////////////////////////////////////////////////////////////

/* Data of ADC io-channels specified in devicetree. */
static constexpr adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
				 DT_SPEC_AND_COMMA)
};

static const device *const strip = DEVICE_DT_GET(STRIP_NODE);
static gpio_dt_spec ctrlButton = GPIO_DT_SPEC_GET(CTRL_BTN_NODE, gpios);
//static const device *const strip_spi = DEVICE_DT_GET(STRIP_SPI_NODE);
static gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);
static constexpr int samplingInterval_us = 100; //us
static constexpr size_t samplingFrameSize = 512;

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

K_THREAD_STACK_DEFINE(messaging_thread_stack, samplingFrameSize*4 + 2048);
auto a = zbus_cpp::ThreadWorker(*messaging_thread_stack, K_THREAD_STACK_SIZEOF(messaging_thread_stack));
auto subscriber = zbus_cpp::MessageSubscriber(
	a,
	&app_sub,
	zbus_cpp::Topic<Core::EventTypes::AudioFrame>{&AudioChannelBus},
	zbus_cpp::Topic<Core::EventTypes::BeatEvent>{&BeatChannelBus},
	zbus_cpp::Topic<Core::EventTypes::ButtonEvent>{&ButtonChannelBus});

auto timer = PeriodicTimer();
auto adcLogger = Logger("ADC_READER");
auto adc_reader = AdcReader<samplingFrameSize>(&adc_channels[0], timer, adcLogger);

auto audioSamplingLogger = Logger("AUDIO_SAMPLING");
auto audioSampling = Threads::AudioSampling(adc_reader, publisher, audioSamplingLogger);

auto ledLogger = Logger("LED_CONTROL");
auto led = Visualization::LedControl(&led1, ledLogger);
auto ledStripLogger = Logger("LED_STRIP");
auto ledStripController = Visualization::LedStripController<STRIP_NUM_PIXELS>(strip, ledStripLogger);

auto lpFilter = LpFilter<samplingFrameSize>();
auto fftProcessor = FftProcessor<samplingFrameSize>();
auto beatDetector = BeatDetectorV2(fftProcessor, lpFilter);
auto audioProcessingLogger = Logger("AUDIO_PROCESSING");
auto audioProcessing = Threads::AudioProcessing(publisher, subscriber, beatDetector, audioProcessingLogger);

auto visualizationLogger = Logger("VISUALIZATION");
auto frameTimer = PeriodicTimer();
auto animationControl = Animations::AnimationControl(frameTimer, ledStripController, led);
auto visualizationModule = Threads::VisualizationThread(subscriber, visualizationLogger, animationControl);

auto animCtrlButton = UtilsButton::Button(ctrlButton);
auto buttonLogger = Logger("BUTTON");
auto buttonThread = Threads::ButtonInputThread(publisher, animCtrlButton, buttonLogger);



///////////////////////////////////////////////////////////////////////////////////
/************************ application start **************************************/
///////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	LOG_INF("App started.");

	//Module initialization
	subscriber.Initialize(4);
	audioSampling.Initialize(samplingInterval_us);
	audioProcessing.Initialize(2);
	visualizationModule.Initialize(3);

	//Module startup
	subscriber.Start();
	audioProcessing.Start();
	audioSampling.Start();
	visualizationModule.Start();

	buttonThread.Initialize();

	LOG_INF("Application running...");

	//take main thread to sleep
	k_sleep(K_FOREVER);
}

