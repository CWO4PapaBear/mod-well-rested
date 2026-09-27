# Upstream contribution plan

Keep this module as an independent public repository on `main`.

## CoA fork

Target: `jealous-sound/azerothcore-wotlk-coa`, base branch **main**. The current owner authorizes preparing a contribution after tests pass. Do not directly push to someone else's main or claim acceptance before their maintainer merges it.

1. Compile and validate against the recorded CoA revision and current target main.
2. Fork the CoA repository and create a small integration branch. GitHub cannot compare arbitrary unrelated module/core histories as an ordinary cross-repository PR.
3. The integration PR vendors this module under `modules/mod-well-rested`, with its license intact and no client archives, private configurations or unrelated modules. The character schema is integrated with the normal pending-migration updater.
4. Include compilation, SQL rollback and gameplay evidence; disclose that optional buff UI is separate. Attach any created PR to the Codex task.
5. Update this standalone module for fixes; keep any CoA-specific adaptation separate.

## Main AzerothCore project

The standard route for an optional module is the module catalogue submission. A core PR is appropriate only if a generic core bug or missing hook is found. The initial design uses existing hooks and does not require a core PR.

- https://www.azerothcore.org/wiki/create-a-module
- https://www.azerothcore.org/module-submit.html

Draft contribution: [CoA PR #5426](https://github.com/jealous-sound/azerothcore-wotlk-coa/pull/5426), targeting `main` at `abd08cd3bab318444aebb715f2b47a5e08500ded`. The owner accepted the deployed version on September 27, 2026. Current-main build/runtime validation, multiplayer load checks, and matching client distribution remain prerequisites for merge readiness. The PR is not merged or deployed by this source publication.
