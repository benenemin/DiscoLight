//
// Created by bened on 30/12/2025.
//

#pragma once

namespace Detail
{
    template <typename... Ts>
    constexpr std::size_t max_sizeof() noexcept {
        std::size_t m = 1U;
        ((m = (m < sizeof(Ts)) ? sizeof(Ts) : m), ...);
        return m;
    }
}
