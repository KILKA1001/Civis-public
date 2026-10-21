#pragma once

#include "civ5/ui/ui_runtime.hpp"
#include "civ5/runtime/research_context.hpp"
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/treasury.hpp"
#include "civ5/runtime/maintenance.hpp"
#include <map>

namespace civ5::runtime { struct ScienceQuote; }

namespace civ5::ui {

struct EconomicBoundFields;

struct OverviewRow {
    // For EconomicGeneralInfo this is the selected XML value control ID.
    std::string label;
    std::string value;
    // The fallback remains readable if the caller has not mounted text yet.
    // Only exact, semantically matching active BNW keys are assigned.
    std::string label_key{};
};
struct OverviewCityRow {
    std::int64_t id = -1, population = -1;
    // Only populated when the saved name matches the owned live city record.
    std::optional<std::string> name;
};
struct OverviewPage {
    std::vector<OverviewRow> rows;
    std::vector<OverviewCityRow> cities;
    std::string unavailable_reason;
    // Native EconomicGeneralInfo.lua hides the penalty row at nonnegative GPT.
    // Unset means the live net quote has not established visibility yet.
    std::optional<bool> science_lost_visible;
};
struct OverviewFrameState {
    std::int64_t player_id = -1;
    std::map<std::string, OverviewPage> pages;
    // Complete outgoing route names from the same owned treasury/city frame.
    // Absent when any endpoint cannot be tied to exactly one named live city.
    std::optional<std::vector<std::string>> named_outgoing_trade_routes;
    // Foreign routes ending at this player, in owned CvGameTrade slot order.
    // The list is absent if any incoming endpoint lacks a matched live city
    // and saved name; an empty list means there are no incoming routes.
    std::optional<std::vector<std::string>> named_incoming_trade_routes;
};

// Capture only owned, current BNW state. Pages without an authoritative source
// carry a reason instead of substituting a saved preview or a zero.
OverviewFrameState capture_overview_frame(const runtime::ResearchContext &game,
    const runtime::CityYieldContext &cities, const runtime::TreasuryContext &treasury,
    std::int64_t player_id, const runtime::MaintenanceContext *maintenance = nullptr,
    const content::RulesDatabase *database = nullptr,
    const content::InfoLocalizer &localize_info = {},
    const saves::SavedSessionSnapshot *saved = nullptr,
    const std::function<std::optional<std::string>(std::string_view)> &localize_city = {},
    const runtime::TreasuryNetQuote *gold_quote = nullptr,
    const runtime::ScienceQuote *science_quote = nullptr);

struct OverviewControl {
    std::string id, element, text_key, texture, style, anchor, offset, size;
};
struct OverviewSource {
    std::string context_id, source_key, selected_source_path;
    std::vector<OverviewControl> controls;
};

// U7's active single-player contexts are selected through the same mounted
// resources as U0; every source must have an authored XML Context root.
std::vector<OverviewSource> read_overview_sources(const assets::ResourceMountTable &resources,
    const UiContextRegistry &registry);

struct OverviewRect { std::int32_t x = 0, y = 0, width = 0, height = 0; };
enum class OverviewPaintKind { rectangle, texture, text };
struct OverviewPaint {
    OverviewPaintKind kind = OverviewPaintKind::text;
    OverviewRect bounds;
    std::string value, style;
};
struct OverviewHit {
    OverviewRect bounds;
    UiCommand command;
};
struct OverviewRenderPlan {
    std::string context_id;
    std::vector<OverviewPaint> paints;
    std::vector<OverviewHit> hits;
    std::vector<std::string> unresolved;
    const OverviewHit *hit(std::int32_t x, std::int32_t y) const;
};
using OverviewLocalizer = std::function<std::optional<std::string>(std::string_view)>;
enum class OverviewLanguage { english, russian };
OverviewRenderPlan plan_overview(const OverviewSource &source, const UiFrameSnapshot &frame,
    std::int32_t width, std::int32_t height, const OverviewLocalizer &localize = {},
    OverviewLanguage language = OverviewLanguage::english,
    const EconomicBoundFields *economic_fields = nullptr);

// Open/close and authored tab switches are navigation commands. Any eventual
// gameplay action must pass through the U0 backend before reporting success.
std::optional<UiCommand> overview_control_command(std::string_view context_id,
    std::string_view control_id);

} // namespace civ5::ui
