# Acceptance gates

- [x] Independent state-engine tests: 15-minute boundary, inn interruption, offline pause, expiry, refresh without stacking, long ticks, fractional XP and overflow.
- [x] Module library compilation against stock AzerothCore baefaab94e954c9351127fabc17d30e3218ac911.
- [ ] CoA module compilation and full PTR worldserver build; no activation as part of builds.
- [x] Isolated MySQL 8.4.11 schema apply twice, independent GUID values, transactional rollback and deletion.
- [ ] In-engine save/relog, backup restoration and missing-schema startup rejection.
- [ ] Inn vs city; leave/re-enter; combat/death/logout interruption; no offline rest accrual.
- [ ] Reward survives relog/restart with remaining online duration; death and refresh behavior.
- [ ] Kill bonus once only; quest/exploration/PvP unaffected; pets and group kills; other XP modules and ordinary rested XP.
- [ ] Two characters remain independent; character deletion cleanup; database failure behavior and crash bounds.
- [ ] Optional buff icon/UI design and no duplicate XP aura, if that presentation is requested.
- [ ] In-game owner acceptance before enabling or proposing upstream integration.
