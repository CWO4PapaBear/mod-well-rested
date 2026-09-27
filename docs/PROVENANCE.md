# Provenance and limitations

Ascension client review found 997614 (periodic helper, 900,000 ms), 997615 (Resting, 15-minute duration) and 997616 (Well Rested, 2-hour duration, 8% monster XP). These are reference evidence only. No DBC, art, proprietary script, copied tooltip or official backend is distributed here, and no custom spell ID is claimed.

AzerothCore reference headers reviewed at `baefaab94e954c9351127fabc17d30e3218ac911` (master): PlayerScript.h, Player.h and SharedDefines.h.

CoA reference fetched at `47dd22ffe6d0a82e6886487f38acff7c5cfeb117` (main). It contains extensive code and a world database baseline, but is an independent reconstruction. Its README explicitly disclaims complete official gameplay parity. No matching inn-timer implementation was found in the searched source, modules and documentation; this is not proof the live Ascension server or a deployed CoA server is nonfunctional.

The owner chose continuous online rest and offline-paused reward time. Other documented lifecycle defaults are our initial design, not assertions about Ascension behavior.
