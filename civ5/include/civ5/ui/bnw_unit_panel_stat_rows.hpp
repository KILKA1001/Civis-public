#pragma once
#include "civ5/ui/bnw_unit_panel_preview_bridge.hpp"
#include "civ5/ui/bnw_unit_panel_stat_rows_types.hpp"
#include "civ5/ui/bnw_unit_panel_secondary_stats.hpp"
#include <array>
#include <vector>

namespace civ5::ui {
inline constexpr std::array<runtime::BnwUnitOrderSourceFile,2> kBnwUnitPanelStatRowSources{{
    {"Assets/DLC/Expansion2/UI/InGame/WorldView/UnitPanel.lua","5b4be7346a6570196e94e638094aa3d14227e8637d7e335a73983d7b35764524"},
    {"Assets/DLC/Expansion2/UI/InGame/WorldView/UnitPanel.xml","ca357d8755c2362a48f9d34371697bc83150dae2f5f2c73be11d2c211cb428fe"}
}};
// Detached Info branch projection, with no player/unit/frame or native receipt.
// Only known AIR Range/base-ranged branches are projected. Numeric tooltip
// arguments are retained; they do not grant a native locale composition owner.
BnwUnitPanelStatRowsRead read_detached_bnw_air_stat_rows(const BnwUnitPanelSecondaryInfo &,
    const TopPanelLocalizationCatalog &, std::string_view locale);
// Independent read contract only: no WorldView feed, paint, tooltip hit, click
// action or render attestation is published by this module. The caller supplies
// its actual selected catalog/locale and owns their lifetime and revision.
class BnwUnitPanelStatRowsBridge {
  public:
    BnwUnitPanelStatRowsBridge(const runtime::LiveSession &, const content::RulesDatabase &,
        const content::InfoTypeCatalog &, const runtime::SessionCheckpointIdentity &,
        std::int64_t active_player);
    std::optional<BnwUnitPanelReadRows> capture(const runtime::LiveSession &,
        std::int64_t selected, std::uint64_t frame, const TopPanelLocalizationCatalog &,
        std::string_view locale) const;
    bool matches(const runtime::LiveSession &, const BnwUnitPanelReadRows &,
        std::int64_t selected, std::uint64_t frame, const TopPanelLocalizationCatalog &,
        std::string_view locale) const noexcept;
  private:
    BnwUnitPanelPreviewBridge primary_;
    BnwUnitPanelSecondaryStatsBridge secondary_;
    const content::RulesDatabase *database_;
    const content::InfoTypeCatalog *types_;
    runtime::SessionCheckpointIdentity identity_;
};
} // namespace civ5::ui
