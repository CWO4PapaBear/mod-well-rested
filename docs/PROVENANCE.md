# Provenance and limitations

Ascension client review found 997614 (periodic helper, 900,000 ms), 997615 (Resting, 15-minute duration) and 997616 (Well Rested, 2-hour duration, 8% monster XP). These are reference evidence only. No DBC, art, proprietary script, copied tooltip or official backend is distributed here, and no proprietary spell row is copied. The optional presentation generator suggests configurable, collision-checked IDs 910100/910101.

AzerothCore reference headers reviewed at `baefaab94e954c9351127fabc17d30e3218ac911` (master): PlayerScript.h, Player.h and SharedDefines.h.

CoA reference fetched at `47dd22ffe6d0a82e6886487f38acff7c5cfeb117` (main). It contains extensive code and a world database baseline, but is an independent reconstruction. Its README explicitly disclaims complete official gameplay parity. No matching inn-timer implementation was found in the searched source, modules and documentation; this is not proof the live Ascension server or a deployed CoA server is nonfunctional.

The owner chose continuous online rest and offline-paused reward time. Other documented lifecycle defaults are our initial design, not assertions about Ascension behavior.

Native-icon follow-up: CoA `origin/main` fetched at `4c3a06dd039957ce071e6eebd710c198e0636b23`. Local Ascension SpellIcon.dbc from patch-S maps Resting icon 72961 to `Interface\icons\INV_Ascend_Tavern_67`, and Well Rested icon 7415 to `Interface\Icons\achievement_zone_jadeforest`. Both textures were found in the owner's patch-I.MPQ. These are local client reference/art assets, not distributed with this public module. Stock icons 44/117 remain the portable generator defaults.

The earlier chat-only implementation compiled successfully against both pinned stock and CoA cores in GitHub Actions run 36330945813 and activated on Bear Cave PTR. That evidence does not validate the subsequent native-icon extension.
