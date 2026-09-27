# mod-well-rested

Independent AzerothCore module: spend **15 continuous online minutes inside an inn** to earn **8% additional monster-kill XP for two hours of online play**. Earned time pauses while logged out.

**Development preview. Disabled by default.** The chat-only version passed stock AzerothCore and CoA module compilation, a full Bear Cave PTR image build, and PTR activation. Gameplay acceptance is still pending. Native timed buff icons are now staged as an optional extension; their updated server build and client installation require separate validation.

## Scope

- No HeroFreePick, More Minions, Lua engine, launcher or Ascension client dependency.
- Inn trigger required; merely standing in a capital city does not count.
- Leaving the inn, dying, entering combat or logging out resets unfinished rest.
- Earned time survives death and logout; time continues to count down while online, including while dead.
- Another complete inn stay refreshes the reward to two hours; durations and percentages do not stack with themselves.
- Monster-kill XP only. No quest, exploration, battleground or player-kill bonus.
- Uses the standard kill-XP hook; other rate/rested systems retain their normal place in the core pipeline. Cross-module ordering/rounding must be tested on each target server.
- All ordinary player classes are eligible; no Bear Cave play-style dependency. Mode-specific restrictions, if desired, belong in a separate integration layer.
- Stock-client presentation remains chat notifications by default. Optional native **Resting** and **Well Rested** buff icons show both countdowns. The inn countdown automatically restarts after every completion and refreshes the earned reward. Display spells are dummy auras, never an additional XP multiplier.

## Installation after validation

Clone this repository as `modules/mod-well-rested` in an AzerothCore checkout, then configure/build/install that server normally. The loader is `Addmod_well_restedScripts()`; no core patch is required by the initial design.

```bash
git clone https://github.com/CWO4PapaBear/mod-well-rested.git modules/mod-well-rested
```

Import `data/sql/db-characters/base/001_well_rested.sql` into the **characters** database using your normal credentials. It creates only two module-owned tables; it does not change core tables or overwrite spell IDs. Then copy `conf/mod_well_rested.conf.dist` to the server's module configuration directory as `mod_well_rested.conf`, set `WellRested.Enable = 1`, and restart after validation. Changes to these settings require a restart.

State is per character GUID. Only earned remaining time and fractional XP are persisted; unfinished rest is intentionally not persisted. Saves occur on earning/expiry, normal character save/logout, and at most once per minute while the bonus is active. A crash can restore up to the last successful save's remaining time (normally at most a minute); this is not an exactly-once reward ledger. Back up the characters database before testing. Missing schema/version prevents enabling the module.

To disable, set `WellRested.Enable = 0` and restart. Retain the module tables for rollback. No world SQL or client files are needed for chat-only operation. Native buff icons require the optional matching spell data described below.

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

## Optional native buff icons

This extension requires matching WotLK `Spell.dbc` records on **both server and every client**. It does not require an addon, HeroFreePick, or Ascension assets. The generator supplies original dummy-aura records and text, using stock sleep/restorative icons by default. No proprietary DBC or artwork is included in this repository.

First check that spell IDs **910100 and 910101** are unused in your effective server/client DBCs, world `spell_dbc`, script bindings, and any module ID registry. These are suggested IDs, not a globally reserved allocation. Configure different IDs if needed. The generator refuses an existing ID and writes only a new output file:

```bash
python3 tools/build_display_dbc.py /path/to/current/Spell.dbc /path/to/staged/Spell.dbc
```

Run against each current server/client input separately so unrelated rows and strings remain intact. Install the client result in your normal cumulative MPQ patch after backing it up, and the server result at its configured DBC path during maintenance. Use the current source data, never replace an entire current DBC with an older server/client copy. Confirm duration records 347 (900000 ms) and 367 (7200000 ms), and icon records 44 and 117, exist on your client. Artwork overrides are optional operator-managed data; `--rest-icon` and `--reward-icon` select installed icon IDs.

Then configure and rebuild/restart the module:

```ini
WellRested.RestingSpell = 910100
WellRested.RewardSpell = 910101
```

The server validates the two dummy-aura definitions before enabling. It marks these presentation auras as nonpersistent; only the module character table owns the earned clock. Login rebuilds the display from saved remaining time. Leaving the inn, combat, death or logout clears unfinished rest. The earned reward and its icon survive death; both continue counting while online. Timers synchronize on transitions and bounded periodic checks without sending a packet every world tick. Buffs cannot be right-click cancelled because removing a presentation icon must not discard or manufacture earned state.

Keep the tooltip's bonus (`--bonus`, default 8) aligned with `WellRested.BonusPercent`. Runtime max/remaining durations come from module configuration. Setting both spell IDs to zero retains chat-only compatibility. On rollback, retain the module character table and the harmless client definitions; do not erase earned time. Validate inn departure, automatic repeated cycles, relog, death, combat, XP attribution and normal buff-frame/addon rendering in game before release.
