# Selected UnitPanel secondary reads

This module is a read owner. It does not select a unit, issue combat actions,
publish a UI frame, or grant full-turn readiness. The camera and current feed
remain unchanged until the existing Task14/15 source tests pass.

## Original source

Selected retail BNW `Assets/DLC/Expansion2/UI/InGame/WorldView/UnitPanel.lua`
SHA256: `5b4be7346a6570196e94e638094aa3d14227e8637d7e335a73983d7b35764524`.
SDK files are pinned by `runtime::kBnwUnitMovementSources` and the test checks
those hashes when the actual installed SDK root is supplied.

- UnitPanel.lua625-635: air movement displays native Range; the rebase tooltip
  uses Range multiplied by AIR_UNIT_REBASE_RANGE_MULTIPLIER and divided by100.
- UnitPanel.lua654-659: air main strength reads GetBaseRangedCombatStrength;
  nonair nonembarked main strength reads GetBaseCombatStrength.
- UnitPanel.lua687-700: nonair units use GetBaseRangedCombatStrength for the
  separate ranged row, which is shown only when the result is positive. Air
  does not duplicate this value into the separate ranged row.
- Subsequent rows may instead use actual spreads, religious conversion,
  RemoveHeresy, cargo or tourism. Those branches are outside this module.
- SDK CvUnit.cpp9669: GetRange reads selected UnitInfo.Range plus the current
  unit m_iExtraRange operand.
- SDK CvUnit.cpp10689: GetBaseRangedCombatStrength reads UnitInfo.RangedCombat.
  The saved m_iBaseRangedCombat scalar is not its source.
- SDK CvUnitClasses.cpp165/188 and487/631: selected native UnitEntry caches
  Range/RangedCombat via Results.GetInt and returns those values. The actual
  RulesRowView integer getter preserves that query's default-zero behavior.

## Binding and limits

The live bridge pins session/state addresses and the monotonic LiveSession state
generation, selected source payload/rules
identity, current type hash and ID, and the current maintenance type binding.
A copied value has player, unit, turn, frame and existing native unit-world
revision stamps. `matches` repeats the actual read; stale current range or
rules values cannot be accepted because a previous result looks plausible.
Repeated replacements cannot revive an expired binding by reusing an address;
a fresh explicit bridge is required after a generation change.
Unknown binding and out-of-range operands remain unavailable with blockers.
A known native zero is kept as zero so the eventual consumer can reproduce
original row visibility; it is never assigned to an unsupported branch.

The source test captures actual Gandhi's explicit selected land unit16385.
Its air test is labeled `DETACHED_ACTUAL_DB_AIR_FIXTURE`: it reads the actual
selected UNIT_FIGHTER Info with a supplied extra-range operand. It does not
claim Gandhi has an air unit or that any air frame was rendered.

EnemyUnitPanel requires current visible hovered target selection and actual
attack/defense/ranged strength plus damage query owners. Base strength alone
cannot provide that prediction. Religious conversion needs current religion
and belief modifiers; those values are not inferred here.

## No-argument Info names

`bnw_info_default_form` reads the literal first form from the selected catalog.
The source pins include CvLocalization.h (default UTF8/Compose form1),
CvInfos.cpp (localized Description cache), CvLuaUnit.cpp and CvUnit.cpp
(GetName/UnitInfo Description chain). The actual RU warrior row contains
six pipe-separated grammatical alternatives. This bounded helper handles
literal no-argument Info labels only; missing names and parameter composition
fail explicitly. The caller retains its own separate policy for custom literal
names and absent-key passthrough. This is not a full engine grammar replacement.
