#pragma once
#include <tuple>
#include <array>
#include <type_traits>
#include <zephyr/kernel.h>

#include "Topic.hpp"
#include "EventTypes.hpp"   // defines Core::EventTypes::{AudioFrame,FeatureFrame,...}

namespace Core {

// ---------- Queue depths (Kconfig-overridable) ----------
#if defined(CONFIG_APP_Q_DEPTH_AUDIO)
inline constexpr std::size_t Q_AUDIO = CONFIG_APP_Q_DEPTH_AUDIO;
#else
inline constexpr std::size_t Q_AUDIO = 4;
#endif

#if defined(CONFIG_APP_Q_DEPTH_FEATURE)
inline constexpr std::size_t Q_FEATURE = CONFIG_APP_Q_DEPTH_FEATURE;
#else
inline constexpr std::size_t Q_FEATURE = 4;
#endif

#if defined(CONFIG_APP_Q_DEPTH_BEAT)
inline constexpr std::size_t Q_BEAT = CONFIG_APP_Q_DEPTH_BEAT;
#else
inline constexpr std::size_t Q_BEAT = 8;
#endif

#if defined(CONFIG_APP_Q_DEPTH_TICK)
inline constexpr std::size_t Q_TICK = CONFIG_APP_Q_DEPTH_TICK;
#else
inline constexpr std::size_t Q_TICK = 2;
#endif

#if defined(CONFIG_APP_Q_DEPTH_ANIMCMD)
inline constexpr std::size_t Q_ANIMCMD = CONFIG_APP_Q_DEPTH_ANIMCMD;
#else
inline constexpr std::size_t Q_ANIMCMD = 4;
#endif

// ---------- 1) Declare the event set ONCE ----------
using EventList = std::tuple<
  Core::EventTypes::AudioFrame,
  Core::EventTypes::FeatureFrame,
  Core::EventTypes::BeatEvent,
  Core::EventTypes::TickEvent,
  Core::EventTypes::AnimCmd
>;

// ---------- 2) Matching topic storage (same order!) ----------
using Topics = std::tuple<
  Topic<Core::EventTypes::AudioFrame,   Q_AUDIO,   OverflowPolicy::DropOldest>,
  Topic<Core::EventTypes::FeatureFrame, Q_FEATURE, OverflowPolicy::DropNewest>,
  Topic<Core::EventTypes::BeatEvent,    Q_BEAT,    OverflowPolicy::DropOldest>,
  Topic<Core::EventTypes::TickEvent,    Q_TICK,    OverflowPolicy::DropOldest>,
  Topic<Core::EventTypes::AnimCmd,      Q_ANIMCMD, OverflowPolicy::Block>
>;

// ---------- 3) Utilities ----------
namespace detail {

// Compile-time index of T in a tuple
template <typename T, typename Tuple, std::size_t I = 0>
constexpr std::size_t index_of() {
  if constexpr (I >= std::tuple_size_v<Tuple>) {
    static_assert(I < std::tuple_size_v<Tuple>,
                  "MessageBus: event type T is not listed in EventList");
    return I; // unreachable
  } else if constexpr (std::is_same_v<T, std::tuple_element_t<I, Tuple>>) {
    return I;
  } else {
    return index_of<T, Tuple, I + 1>();
  }
}

template <typename T>
concept InEventList = requires {
  std::tuple_element_t<index_of<T, EventList>(), EventList>{};
};

} // namespace detail

// ---------- 4) The bus (no topics exposed) ----------
class MessageBus {
public:
  static MessageBus& instance() {
    static MessageBus bus;
    return bus;
  }

  void Initialize() {
    if (initialized_) return;
    static constexpr std::array<const char*, std::tuple_size_v<Topics>> NAMES{
      "AUDIO_FRAME", "FEATURE_FRAME", "BEAT_EVENT", "TICK_EVENT", "ANIM_CMD"
    };
    init_topics(NAMES);
    initialized_ = true;
  }

  // ----- Public API: publish / subscribe by EVENT TYPE only -----
  template <typename T>
  requires detail::InEventList<T>
  bool Publish(const T& ev, k_timeout_t to = K_NO_WAIT) {
    return topic_ref<T>().publish(ev, to);
  }

  template <typename T>
  requires detail::InEventList<T>
  bool Subscribe(T& out, k_timeout_t to = K_FOREVER) {
    return topic_ref<T>().subscribe(out, to);
  }

  template <typename T>
  requires detail::InEventList<T>
  bool TrySubscribe(T& out) {
    return Subscribe<T>(out, K_NO_WAIT);
  }

private:
  MessageBus() = default;

  template <typename T>
  static constexpr std::size_t idx() {
    return detail::index_of<T, EventList>();
  }

  template <typename T>
  auto& topic_ref() {
    return std::get<idx<T>()>(topics_);
  }

  template <std::size_t... Is>
  void init_topics_impl(const std::array<const char*, std::tuple_size_v<Topics>>& names,
                        std::index_sequence<Is...>) {
    (std::get<Is>(topics_).init(names[Is]), ...);
  }

  void init_topics(const std::array<const char*, std::tuple_size_v<Topics>>& names) {
    init_topics_impl(names, std::make_index_sequence<std::tuple_size_v<Topics>>{});
  }

private:
  bool   initialized_{false};
  Topics topics_{};
};

} // namespace Core
