// SPDX-License-Identifier: GPL-2.0-or-later
#include "WellRestedState.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DataMap.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <algorithm>

namespace
{
constexpr char StateKey[] = "mod-well-rested.state";
WellRested::Policy policy;
bool enabled = false; // Written on startup only, before players can log in.
bool announce = true;
struct PlayerState : DataMap::Base
{
    WellRested::State timer;
    uint32 saveMs = 0;
};
void Save(Player* player, PlayerState const& state)
{
    // Synchronous ordering prevents an older asynchronous save from overwriting
    // logout state after a quick reconnect. Values are numeric, never SQL input.
    CharacterDatabase.DirectExecute(
        "INSERT INTO mod_well_rested_character (guid,remaining_ms,fraction) VALUES ({},{},{}) "
        "ON DUPLICATE KEY UPDATE remaining_ms=VALUES(remaining_ms),fraction=VALUES(fraction)",
        player->GetGUID().GetCounter(), state.timer.remainingMs, state.timer.fraction);
}
}
class WellRestedWorld final : public WorldScript
{
public:
    WellRestedWorld() : WorldScript("WellRestedWorld") { }
    void OnStartup() override
    {
        if (!sConfigMgr->GetOption<bool>("WellRested.Enable", false)) return;
        auto version = CharacterDatabase.Query("SELECT version FROM mod_well_rested_schema WHERE id=1");
        if (!version || version->Fetch()[0].Get<uint32>() != 1)
        {
            LOG_ERROR("module.well_rested", "Disabled: import the version-1 character SQL before enabling WellRested.");
            return;
        }
        auto rest = sConfigMgr->GetOption<uint32>("WellRested.RestSeconds", 900);
        auto reward = sConfigMgr->GetOption<uint32>("WellRested.RewardSeconds", 7200);
        auto bonus = sConfigMgr->GetOption<uint32>("WellRested.BonusPercent", 8);
        if (!rest || rest > 86400 || !reward || reward > 604800 || !bonus || bonus > 1000)
        {
            LOG_ERROR("module.well_rested", "Disabled: invalid duration or XP bonus configuration.");
            return;
        }
        policy = {rest * 1000, reward * 1000, bonus};
        announce = sConfigMgr->GetOption<bool>("WellRested.Announce", true);
        enabled = true;
        LOG_INFO("module.well_rested", "Enabled: {} seconds in an inn grants {}% monster XP for {} online seconds.", rest, bonus, reward);
    }
};
class WellRestedPlayer final : public PlayerScript
{
public:
    WellRestedPlayer() : PlayerScript("WellRestedPlayer") { }
    void OnPlayerLogin(Player* player) override
    {
        if (!enabled) return;
        auto* state = player->CustomData.GetDefault<PlayerState>(StateKey);
        auto result = CharacterDatabase.Query(
            "SELECT remaining_ms,fraction FROM mod_well_rested_character WHERE guid={}", player->GetGUID().GetCounter());
        if (result)
        {
            state->timer.remainingMs = std::min(result->Fetch()[0].Get<uint32>(), policy.rewardMs);
            state->timer.fraction = state->timer.remainingMs ? result->Fetch()[1].Get<uint32>() % 100 : 0;
        }
        if (announce && state->timer.remainingMs)
            ChatHandler(player->GetSession()).PSendSysMessage("Well Rested: {}% monster-kill XP bonus; {} minutes remaining (paused offline).", policy.bonusPercent, (state->timer.remainingMs + 59999) / 60000);
    }
    void OnPlayerUpdate(Player* player, uint32 elapsed) override
    {
        if (!enabled) return;
        auto* state = player->CustomData.Get<PlayerState>(StateKey);
        if (!state) return;
        // CITY/FACTION_AREA alone do not qualify. Use the core's inn trigger flag.
        bool inn = player->IsAlive() && !player->IsInCombat() && player->HasRestFlag(REST_FLAG_IN_TAVERN);
        auto event = WellRested::Advance(state->timer, policy, elapsed, inn);
        if (announce && event.started)
            ChatHandler(player->GetSession()).PSendSysMessage("Resting: stay in this inn for {} minutes to earn Well Rested.", policy.restMs / 60000);
        if (announce && event.earned)
            ChatHandler(player->GetSession()).PSendSysMessage("Well Rested: {}% additional monster-kill XP for {} online minutes.", policy.bonusPercent, policy.rewardMs / 60000);
        if (announce && event.expired)
            ChatHandler(player->GetSession()).SendSysMessage("Your Well Rested bonus has expired.");
        state->saveMs = std::min<uint64>(uint64(state->saveMs) + elapsed, 60000);
        if (event.earned || event.expired || (state->timer.remainingMs && state->saveMs >= 60000))
        {
            Save(player, *state);
            state->saveMs = 0;
        }
    }
    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 source) override
    {
        if (!enabled || source != XPSOURCE_KILL || !victim || victim->GetTypeId() != TYPEID_UNIT) return;
        if (auto* state = player->CustomData.Get<PlayerState>(StateKey))
            amount = WellRested::Bonus(state->timer, policy, amount);
    }
    void OnPlayerSave(Player* player) override
    {
        if (enabled)
            if (auto* state = player->CustomData.Get<PlayerState>(StateKey)) Save(player, *state);
    }
    void OnPlayerBeforeLogout(Player* player) override
    {
        if (!enabled) return;
        if (auto* state = player->CustomData.Get<PlayerState>(StateKey))
        {
            WellRested::Logout(state->timer);
            Save(player, *state);
        }
    }
    void OnPlayerDelete(ObjectGuid guid, uint32 /*account*/) override
    {
        if (enabled) CharacterDatabase.DirectExecute("DELETE FROM mod_well_rested_character WHERE guid={}", guid.GetCounter());
    }
};
void Addmod_well_restedScripts()
{
    new WellRestedWorld();
    new WellRestedPlayer();
}
