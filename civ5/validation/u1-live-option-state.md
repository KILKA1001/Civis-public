# U1: live BNW TopPanel option transition

The selected Gandhi save is restored by `SelectedBnwPreviewHost` into one
`LiveSessionState`; `restore_science` resolves `GAMEOPTION_NO_SCIENCE` into
`state.science.no_science`, and `restore_city_yields` resolves
`GAMEOPTION_NO_HAPPINESS` into `state.cities.no_happiness`. The UI frame now
copies those owned values on every capture. BNW `TopPanel.lua` renders the
corresponding localized disabled labels before querying either yield.

When the live science or happiness quote is absent, those confirmed disabled
labels remain visible. The opposite state still shows `N/A` until a complete
current quote arrives. This handles option changes across frames without
borrowing stale values from the selected save or a preview fixture.

The saved setup does not provide a current science rate, happiness source
breakdown, culture rate, or Golden Age threshold. The existing runtime must
publish their respective current GameCore queries before U1 can mark those
numeric TopPanel metrics complete. The selected game's option values are read
from the restored owner at runtime; this report does not infer them from the
save filename or a different party.

Checks: warning-enabled syntax check of the changed sources and focused
TopPanel asset/presentation suite (123 checks, pass). The live-frame source
also passed syntax checking; its whole target was not rebuilt while the U4
renderer build was active.
