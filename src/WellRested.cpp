// SPDX-License-Identifier: GPL-2.0-or-later
#include "WellRestedState.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DataMap.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include <algorithm>

namespace
{
constexpr char StateKey[] = "mod-well-rested.state";
WellRested::Policy policy;
bool enabled = false; // Written on startup only, before players can log in.
bool announce = true;
uint32 restingSpell = 0;
uint32 rewardSpell = 0;
bool icons = false;
struct PlayerState : DataMap::Base
{
    WellRested::State timer;
    uint32 saveMs = 0;
    uint32 iconSyncMs = 0;
};
// These are display-only auras. The timer and XP hook below remain authoritative.
// Never use MOD_XP_PCT here: that would multiply the reward a second time.
bool ValidDisplaySpell(SpellInfo const* info)
{
    if (!info || info->Effects[0].Effect != SPELL_EFFECT_APPLY_AURA ||
        info->Effects[0].ApplyAuraName != SPELL_AURA_DUMMY ||
        info->Effects[1].Effect || info->Effects[2].Effect ||
        !info->HasAttribute(SPELL_ATTR0_NO_AURA_CANCEL) ||
        !info->HasAttribute(SPELL_ATTR3_ALLOW_AURA_WHILE_DEAD) || info->IsPassive())
        return false;
    return true;
}
class WellRestedSpellData final : public GlobalScript
{
    uint32 const restId;
    uint32 const rewardId;
public:
    WellRestedSpellData() : GlobalScript("WellRestedSpellData"),
        restId(sConfigMgr->GetOption<uint32>("WellRested.RestingSpell", 0)),
        rewardId(sConfigMgr->GetOption<uint32>("WellRested.RewardSpell", 0)) { }
    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        // Public loader hook supplies mutable metadata before players log in.
        // Only our configured, validated display spells must not be persisted.
        if (restId && rewardId && restId != rewardId &&
            (info->Id == restId || info->Id == rewardId) && ValidDisplaySpell(info))
            info->AttributesCu |= SPELL_ATTR0_CU_AURA_CANNOT_BE_SAVED;
    }
};
void ClearIcons(Player* player)
{
    if (!icons) return;
    player->RemoveAurasDueToSpell(restingSpell);
    player->RemoveAurasDueToSpell(rewardSpell);
}
void SyncIcon(Player* player, uint32 spell, uint32 remaining, uint32 maximum, bool force)
{
    if (!remaining)
    {
        player->RemoveAurasDueToSpell(spell);
        return;
    }
    Aura* aura = player->GetAura(spell, player->GetGUID());
    bool created = !aura;
    if (!aura) aura = player->AddAura(spell, player);
    if (!aura) return; // Retry on the next bounded presentation update.
    int64 drift = int64(aura->GetDuration()) - remaining;
    if (created || force || aura->GetMaxDuration() != int32(maximum) || drift > 1500 || drift < -1500)
    {
        aura->SetMaxDuration(int32(maximum));
        aura->SetDuration(int32(remaining));
    }
}
void SyncIcons(Player* player, PlayerState const& state, bool force)
{
    if (!icons) return;
    SyncIcon(player, restingSpell, state.timer.wasInInn ? policy.restMs - state.timer.restMs : 0, policy.restMs, force);
    SyncIcon(player, rewardSpell, state.timer.remainingMs, policy.rewardMs, force);
}
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
        restingSpell = sConfigMgr->GetOption<uint32>("WellRested.RestingSpell", 0);
        rewardSpell = sConfigMgr->GetOption<uint32>("WellRested.RewardSpell", 0);
        if (restingSpell || rewardSpell)
        {
            if (!restingSpell || !rewardSpell || restingSpell == rewardSpell ||
                !ValidDisplaySpell(sSpellMgr->GetSpellInfo(restingSpell)) ||
                !ValidDisplaySpell(sSpellMgr->GetSpellInfo(rewardSpell)))
            {
                LOG_ERROR("module.well_rested", "Disabled: invalid display spell pair; install matching dummy-aura definitions and client data.");
                return;
            }
            icons = true;
            LOG_INFO("module.well_rested", "Timed buff icons enabled: Resting {}, Well Rested {} (display only).", restingSpell, rewardSpell);
        }
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
        // Discard any core-saved display durations. Our persisted timer controls
        // the earned reward; unfinished inn progress always starts over on login.
        ClearIcons(player);
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
        bool wasInInn = state->timer.wasInInn;
        auto event = WellRested::Advance(state->timer, policy, elapsed, inn);
        state->iconSyncMs = std::min<uint64>(uint64(state->iconSyncMs) + elapsed, 1000);
        bool changed = wasInInn != state->timer.wasInInn || event.earned || event.expired;
        if (changed || state->iconSyncMs >= 1000)
        {
            SyncIcons(player, *state, changed);
            state->iconSyncMs = 0;
        }
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
            ClearIcons(player);
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
    new WellRestedSpellData();
    new WellRestedWorld();
    new WellRestedPlayer();
}
