// SPDX-License-Identifier: GPL-2.0-or-later
#include "../src/WellRestedState.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "failed line " << __LINE__ << ": " #x << '\n'; std::exit(1); } } while (false)
int main()
{
    using namespace WellRested;
    Policy p; State s;
    CHECK(Advance(s,p,1000,false).started == false);
    CHECK(Advance(s,p,1000,true).started);
    CHECK(s.restMs == 0);
    Advance(s,p,899999,true); CHECK(s.remainingMs == 0);
    CHECK(Advance(s,p,1,true).earned); CHECK(s.remainingMs == 7200000);
    CHECK(Bonus(s,p,100) == 108);
    Advance(s,p,60000,false); CHECK(s.remainingMs == 7140000 && s.restMs == 0);
    Logout(s); auto saved = s.remainingMs;
    // No updates while offline, and persisted time starts unchanged on login.
    State loaded; loaded.remainingMs = saved; CHECK(loaded.remainingMs == 7140000);
    Advance(loaded,p,7140000,false); CHECK(loaded.remainingMs == 0);
    CHECK(Bonus(loaded,p,100) == 100);
    State interrupted;
    Advance(interrupted,p,0,true); Advance(interrupted,p,899999,true);
    Advance(interrupted,p,1,false); Advance(interrupted,p,0,true);
    Advance(interrupted,p,1,true); CHECK(interrupted.remainingMs == 0 && interrupted.restMs == 1);
    Logout(interrupted); CHECK(interrupted.restMs == 0 && !interrupted.wasInInn);
    State refreshed; Advance(refreshed,p,0,true);
    Advance(refreshed,p,900000,true); Advance(refreshed,p,900000,true);
    CHECK(refreshed.remainingMs == 7200000); // refresh, never add duration
    CHECK(refreshed.restMs == 0 && refreshed.wasInInn); // next 15-minute cycle started
    Advance(refreshed,p,1000,true);
    CHECK(refreshed.restMs == 1000 && refreshed.remainingMs == 7199000); // both clocks visible
    State longTick; Advance(longTick,p,0,true); Advance(longTick,p,900001,true);
    CHECK(longTick.remainingMs == 7199999 && longTick.restMs == 1);
    State tiny; tiny.remainingMs = 1000; unsigned total = 0;
    for (unsigned i=0;i<100;++i) total += Bonus(tiny,p,1);
    CHECK(total == 108 && tiny.fraction == 0);
    CHECK(Bonus(tiny,p,0)==0);
    CHECK(Bonus(tiny,p,0xffffffffu)==0xffffffffu);
    State dead; dead.remainingMs = 10000; dead.restMs = 9000; dead.wasInInn = true;
    Advance(dead,p,1000,false); CHECK(dead.remainingMs==9000 && dead.restMs==0);
    std::cout << "PASS: thresholds, interruption, offline pause, expiry, refresh, long ticks, fractional XP and saturation\n";
}
