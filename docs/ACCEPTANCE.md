# Acceptance gates

- [x] Independent state-engine tests: 15-minute boundary, inn interruption, offline pause, expiry, refresh without stacking, long ticks, fractional XP and overflow.
- [ ] Full build against stock AzerothCore and CoA; no PTR activation as part of builds.
- [ ] Isolated characters schema: apply twice, save/relog, rollback from backup, missing-schema rejection.
- [ ] Inn vs city; leave/re-enter; combat/death/logout interruption; no offline rest accrual.
- [ ] Reward survives relog/restart with remaining online duration; death and refresh behavior.
- [ ] Kill bonus once only; quest/exploration/PvP unaffected; pets and group kills; other XP modules and ordinary rested XP.
- [ ] Two characters remain independent; character deletion cleanup; database failure behavior and crash bounds.
- [ ] Optional buff icon/UI design and no duplicate XP aura, if that presentation is requested.
- [ ] In-game owner acceptance before enabling or proposing upstream integration.
