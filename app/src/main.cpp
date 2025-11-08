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

#include "Core/EventBus.hpp"
#include "Threads/SubscriberThread.hpp"
#include "Threads/ThreadWorker.hpp"
#include "Threads/AudioSampling.hpp"
#include "Threads/AudioProcessing.hpp"
#include "Threads/VisualizationThread.hpp"
#include "ADC/AdcReader.hpp"
#include "Visualization/LedControl.hpp"
#include "Visualization/LedStripController.hpp"

#include "Utils/Logger.hpp"
#include "Utils/PeriodicTimer.hpp"

using Core::MessageBus;
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
//static const device *const strip_spi = DEVICE_DT_GET(STRIP_SPI_NODE);
static gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);
static constexpr int samplingInterval_us = 100; //us
static constexpr size_t samplingFrameSize = 512;


//////////////////////////////////////////////////////////////////////////////////
/****************************** DI **********************************************/
//////////////////////////////////////////////////////////////////////////////////

auto timer = PeriodicTimer();
auto adcLogger = Logger("ADC_READER");
auto adc_reader = AdcReader<samplingFrameSize>(&adc_channels[0], timer, adcLogger);

auto audioSamplingLogger = Logger("AUDIO_SAMPLING");
auto audioSampling = Threads::AudioSampling(adc_reader, MessageBus::instance(), audioSamplingLogger);

auto ledLogger = Logger("LED_CONTROL");
auto led = Visualization::LedControl(&led1, ledLogger);
auto ledStripLogger = Logger("LED_STRIP");
auto ledStripController = Visualization::LedStripController<STRIP_NUM_PIXELS>(strip, ledStripLogger);

K_THREAD_STACK_DEFINE(processing_thread_stack, samplingFrameSize*4 + 2048);
auto lpFilter = LpFilter<samplingFrameSize>();
auto fftProcessor = FftProcessor<samplingFrameSize>();
auto beatDetectorLogger = Logger("BEAT_DETECTOR");
auto beatDetector = BeatDetector<samplingFrameSize, 2>(beatDetectorLogger);
auto audioProcessingLogger = Logger("AUDIO_PROCESSING");
auto audioProcessingThreadWorker = ThreadWorker(*processing_thread_stack, K_THREAD_STACK_SIZEOF(processing_thread_stack));
auto audioProcessing = Threads::AudioProcessing(MessageBus::instance(), audioProcessingThreadWorker, lpFilter, fftProcessor, beatDetector, audioProcessingLogger);

K_THREAD_STACK_DEFINE(visualization_thread_stack, samplingFrameSize);
auto visualizationWorker = ThreadWorker(*visualization_thread_stack, K_THREAD_STACK_SIZEOF(visualization_thread_stack));
auto visualizationLogger = Logger("VISUALIZATION");
auto visualizationModule = Threads::VisualizationThread(MessageBus::instance(), visualizationWorker, visualizationLogger, ledStripController, led);

///////////////////////////////////////////////////////////////////////////////////
/************************ application start **************************************/
///////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	LOG_INF("App started.");

	//Module initialization
	MessageBus::instance().Initialize();
	audioSampling.Initialize(samplingInterval_us);
	audioProcessing.Initialize(2);
	visualizationModule.Initialize(2);

	//Module startup
	audioProcessing.Start();
	audioSampling.Start();
	visualizationModule.Start();

	LOG_INF("Application running...");

	//take main thread to sleep
	k_sleep(K_FOREVER);
}

