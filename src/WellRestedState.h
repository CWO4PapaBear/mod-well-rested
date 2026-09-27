// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace WellRested
{
struct Policy
{
    std::uint32_t restMs = 15 * 60 * 1000;
    std::uint32_t rewardMs = 2 * 60 * 60 * 1000;
    std::uint32_t bonusPercent = 8;
};
struct State
{
    std::uint32_t restMs = 0;
    std::uint32_t remainingMs = 0;
    std::uint32_t fraction = 0;
    bool wasInInn = false;
};
struct Events { bool started = false; bool earned = false; bool expired = false; };

// Only online player updates call this. Leaving an inn, death or logout breaks
// unfinished rest. Earned time survives death/logout and counts down online.
inline Events Advance(State& s, Policy const& p, std::uint32_t elapsed, bool eligibleInn)
{
    Events event;
    auto before = s.remainingMs;
    s.remainingMs -= std::min(s.remainingMs, elapsed);
    event.expired = before && !s.remainingMs;
    if (event.expired) s.fraction = 0;
    if (!eligibleInn)
    {
        s.restMs = 0;
        s.wasInInn = false;
        return event;
    }
    if (!s.wasInInn)
    {
        s.wasInInn = true;
        s.restMs = 0;
        event.started = true;
        return event; // Do not credit time before the first observed inn update.
    }
    auto needed = p.restMs - std::min(s.restMs, p.restMs);
    if (elapsed >= needed)
    {
        // A long tick must not grant a fresh two hours at its end if the rest
        // threshold was reached earlier in that tick. Repeated completions refresh.
        auto sinceLastCompletion = (elapsed - needed) % p.restMs;
        s.remainingMs = p.rewardMs - std::min(p.rewardMs, sinceLastCompletion);
        s.restMs = sinceLastCompletion;
        s.fraction = 0;
        event.earned = true;
        event.expired = false;
    }
    else s.restMs += elapsed;
    return event;
}
inline std::uint32_t Bonus(State& s, Policy const& p, std::uint32_t amount)
{
    if (!s.remainingMs || !amount) return amount;
    std::uint64_t scaled = std::uint64_t(amount) * p.bonusPercent + s.fraction;
    std::uint64_t total = std::uint64_t(amount) + scaled / 100;
    if (total > std::numeric_limits<std::uint32_t>::max())
    {
        s.fraction = 0;
        return std::numeric_limits<std::uint32_t>::max();
    }
    s.fraction = scaled % 100;
    return static_cast<std::uint32_t>(total);
}
inline void Logout(State& s) { s.restMs = 0; s.wasInInn = false; }
}
