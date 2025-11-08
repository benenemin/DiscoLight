//
// Created by bened on 08/10/2025.
//

#pragma once

#include <cstddef>
#include <type_traits>
#include <zephyr/kernel.h>

namespace Core {
// Overflow behavior when a publisher hits a full queue.
enum class OverflowPolicy
{
  Block,         // Wait according to timeout passed to publish()
  DropNewest,    // Give up if full (non-blocking publish fails)
  DropOldest,    // Pop one oldest then retry once (recommended for realtime)
};

// Typed topic (no dynamic allocation).
template <typename T, std::size_t Depth, OverflowPolicy Policy>
class Topic {
  static_assert(std::is_trivially_copyable_v<T>,
                "Topic elements must be trivially copyable (POD)");
  static_assert(Depth > 0, "Depth must be > 0");

public:
  constexpr Topic() = default;
  using message_type = T;
  // Must be called at boot before first use.
  void init(const char* name)
  {
    name_ = name;
    k_msgq_init(&q_, buffer_, sizeof(T), Depth);
  }

  // Non-copyable / non-movable (unique queue owner)
  Topic(const Topic&) = delete;
  Topic& operator=(const Topic&) = delete;

  // Publish with policy applied. Default is K_NO_WAIT (hot path friendly).
  bool publish(const T& msg, const k_timeout_t to = K_NO_WAIT)
  {
    if (Policy == OverflowPolicy::Block)
    {
      return k_msgq_put(&q_, &msg, to) == 0;
    }

    if (Policy == OverflowPolicy::DropNewest)
    {
      return k_msgq_put(&q_, &msg, K_NO_WAIT) == 0;
    }

    if (k_msgq_put(&q_, &msg, K_NO_WAIT) == 0)
    {
      return true;
    }
    // queue full: drop one oldest and retry once
    T scratch{};
    k_msgq_get(&q_, &scratch, K_NO_WAIT);
    return k_msgq_put(&q_, &msg, K_NO_WAIT) == 0;
  }

  // Blocking or timed receive (subscriber side).
  bool subscribe(T& out, const k_timeout_t to = K_FOREVER)
  {
    return k_msgq_get(&q_, &out, to) == 0;
  }

  // Helpers
  void purge()
  {
    k_msgq_purge(&q_);
  }

  int pending()
  {
    return k_msgq_num_used_get(&q_);
  }

  static constexpr std::size_t capacity()
  {
    return Depth;
  }

  const char* name() const
  {
    return name_;
  }

private:
  alignas(4) char buffer_[Depth * sizeof(T)]{}; // static storage
  k_msgq q_{};
  const char* name_{"<unnamed>"};
};

} // Core
