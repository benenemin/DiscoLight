#include "App/Application.hpp"

#include <autoconf.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "Constants.hpp"

LOG_MODULE_DECLARE(App, CONFIG_APP_LOG_LEVEL);

namespace
{
constexpr adc_dt_spec kAdcChannels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
                         DT_SPEC_AND_COMMA)};

const device* const kStrip = DEVICE_DT_GET(STRIP_NODE);
gpio_dt_spec kCtrlButton = GPIO_DT_SPEC_GET(CTRL_BTN_NODE, gpios);
gpio_dt_spec kSignalLed = GPIO_DT_SPEC_GET(DT_NODELABEL(sigled), gpios);
gpio_dt_spec kLoadSwitch = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(load_switch), gpios, {0});

ZBUS_SUBSCRIBER_DEFINE(app_sub, 8);

ZBUS_CHAN_DEFINE_WITH_ID(
    AudioChannelBus,
    0x348756834,
    Core::EventTypes::AudioFrame,
    nullptr,
    nullptr,
    ZBUS_OBSERVERS(app_sub),
    ZBUS_MSG_INIT({}));

ZBUS_CHAN_DEFINE_WITH_ID(
    BeatChannelBus,
    0x348756835,
    Core::EventTypes::BeatEvent,
    nullptr,
    nullptr,
    ZBUS_OBSERVERS(app_sub),
    ZBUS_MSG_INIT({}));

ZBUS_CHAN_DEFINE_WITH_ID(
    ButtonChannelBus,
    0x348756836,
    Core::EventTypes::ButtonEvent,
    nullptr,
    nullptr,
    ZBUS_OBSERVERS(app_sub),
    ZBUS_MSG_INIT({}));

K_THREAD_STACK_DEFINE(kMessagingThreadStack, Constants::SamplingFrameSize * 4 + 2048);
} // namespace

namespace App
{
Application::Application()
    : app_logger_("APP"),
      publisher_(zbus_cpp::Topic<Core::EventTypes::AudioFrame>{&AudioChannelBus},
                 zbus_cpp::Topic<Core::EventTypes::BeatEvent>{&BeatChannelBus},
                 zbus_cpp::Topic<Core::EventTypes::ButtonEvent>{&ButtonChannelBus}),
      messaging_logger_("MESSAGING"),
      messaging_worker_(*kMessagingThreadStack, K_THREAD_STACK_SIZEOF(kMessagingThreadStack)),
      subscriber_(messaging_worker_,
                  &app_sub,
                  zbus_cpp::Topic<Core::EventTypes::AudioFrame>{&AudioChannelBus},
                  zbus_cpp::Topic<Core::EventTypes::BeatEvent>{&BeatChannelBus},
                  zbus_cpp::Topic<Core::EventTypes::ButtonEvent>{&ButtonChannelBus}),
      adc_logger_("ADC_READER"),
      adc_reader_(&kAdcChannels[0], adc_timer_, adc_logger_),
      audio_sampling_logger_("AUDIO_SAMPLING"),
      audio_sampling_module_(adc_reader_, publisher_, audio_sampling_logger_),
      led_logger_("LED_CONTROL"),
      led_(&kSignalLed, led_logger_),
      led_strip_logger_("LED_STRIP"),
      led_strip_controller_(kStrip, led_strip_logger_),
      beat_detector_(fft_processor_, lp_filter_),
      audio_processing_logger_("AUDIO_PROCESSING"),
      audio_processing_module_(
          publisher_, subscriber_, beat_detector_, event_clock_, audio_processing_logger_),
      visualization_logger_("VISUALIZATION"),
      load_switch_logger_("LOAD_SWITCH"),
      load_switch_(&kLoadSwitch, load_switch_logger_),
      animation_logger_("ANIMATION"),
      animation_control_(frame_timer_, led_strip_controller_, led_, animation_logger_),
      visualization_module_(
          subscriber_, visualization_logger_, animation_control_, load_switch_),
      control_button_(kCtrlButton),
      button_logger_("BUTTON"),
      input_module_(publisher_, control_button_, event_clock_, button_logger_)
{
    messaging_worker_.AttachLogger(messaging_logger_);
    subscriber_.AttachLogger(messaging_logger_);
    app_logger_.info("Application object constructed.");
}

void Application::Initialize()
{
    app_logger_.info("Initializing application.");

    event_clock_.Initialize();
    app_logger_.info("Monotonic clock initialized.");

    subscriber_.Initialize(kSubscriberPriority);
    app_logger_.info("Message subscriber initialized.");

    if (const auto ret = input_module_.Initialize(); ret != 0)
    {
        app_logger_.error("Input module initialization failed: %d", ret);
    }
    else
    {
        app_logger_.info("Input module initialized.");
    }

    audio_sampling_module_.Initialize();
    app_logger_.info("Audio sampling module initialization requested.");
    audio_processing_module_.Initialize();
    app_logger_.info("Audio processing module initialization requested.");
    visualization_module_.Initialize();
    app_logger_.info("Visualization module initialization requested.");

    app_logger_.info("Application initialization complete.");
}

void Application::Start()
{
    app_logger_.info("Starting application modules.");

    subscriber_.Start();
    app_logger_.info("Message subscriber started.");

    audio_processing_module_.Start();
    app_logger_.info("Audio processing module start requested.");
    audio_sampling_module_.Start();
    app_logger_.info("Audio sampling module start requested.");
    visualization_module_.Start();
    app_logger_.info("Visualization module start requested.");

    app_logger_.info("Application startup sequence complete.");
}
} // namespace App
