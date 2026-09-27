# mod-well-rested

Independent AzerothCore module: spend **15 continuous online minutes inside an inn** to earn **8% additional monster-kill XP for two hours of online play**. Earned time pauses while logged out.

**Development preview. Disabled by default.** Timer/XP arithmetic tests pass. Isolated MySQL schema validation also passes. Stock AzerothCore module compilation and the full Bear Cave PTR image build pass. CoA compilation, activation preflight and in-game testing remain. Not installed on Bear Cave PTR. Do not treat source publication as production readiness.

## Scope

- No HeroFreePick, More Minions, Lua engine, launcher or Ascension client dependency.
- Inn trigger required; merely standing in a capital city does not count.
- Leaving the inn, dying, entering combat or logging out resets unfinished rest.
- Earned time survives death and logout; time continues to count down while online, including while dead.
- Another complete inn stay refreshes the reward to two hours; durations and percentages do not stack with themselves.
- Monster-kill XP only. No quest, exploration, battleground or player-kill bonus.
- Uses the standard kill-XP hook; other rate/rested systems retain their normal place in the core pipeline. Cross-module ordering/rounding must be tested on each target server.
- All ordinary player classes are eligible; no Bear Cave play-style dependency. Mode-specific restrictions, if desired, belong in a separate integration layer.
- Initial stock-client presentation is chat notifications. A buff icon/countdown is **not yet included**. Optional client presentation must not apply another XP aura and double the bonus.

## Installation after validation

Clone this repository as `modules/mod-well-rested` in an AzerothCore checkout, then configure/build/install that server normally. The loader is `Addmod_well_restedScripts()`; no core patch is required by the initial design.

```bash
git clone https://github.com/CWO4PapaBear/mod-well-rested.git modules/mod-well-rested
```

Import `data/sql/db-characters/base/001_well_rested.sql` into the **characters** database using your normal credentials. It creates only two module-owned tables; it does not change core tables or overwrite spell IDs. Then copy `conf/mod_well_rested.conf.dist` to the server's module configuration directory as `mod_well_rested.conf`, set `WellRested.Enable = 1`, and restart after validation. Changes to these settings require a restart.

State is per character GUID. Only earned remaining time and fractional XP are persisted; unfinished rest is intentionally not persisted. Saves occur on earning/expiry, normal character save/logout, and at most once per minute while the bonus is active. A crash can restore up to the last successful save's remaining time (normally at most a minute); this is not an exactly-once reward ledger. Back up the characters database before testing. Missing schema/version prevents enabling the module.

To disable, set `WellRested.Enable = 0` and restart. Retain the module tables for rollback. No world SQL or client files are needed for this preview.

## Tests

```bash
g++ -std=c++17 -Wall -Wextra -Werror tests/state_test.cpp -o /tmp/well-rested-test
/tmp/well-rested-test
```

See [upstream plan](docs/UPSTREAM.md), [provenance](docs/PROVENANCE.md), and [acceptance checklist](docs/ACCEPTANCE.md). Licensed GPL-2.0-or-later; original implementation, no proprietary client resources included.

Run the synthetic schema checks in a disposable, network-isolated Docker container (use an already available MySQL image):

```bash
python3 tests/schema_test.py --image mysql:8.4
```

This test creates and removes only its randomly named test container. It does not connect to a live server database.
