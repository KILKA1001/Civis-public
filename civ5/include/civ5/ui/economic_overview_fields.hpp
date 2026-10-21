#pragma once

#include "civ5/ui/economic_overview_active_tabs.hpp"
#include <set>

namespace civ5::ui {

struct EconomicFieldSourceProof {
    bool verified = false;
    std::string general_xml_path, general_xml_sha256;
    std::string happiness_xml_path, happiness_xml_sha256;
    std::string general_lua_path, general_lua_sha256;
    std::string happiness_lua_path, happiness_lua_sha256;
    std::vector<std::string> general_ids, happiness_ids;
    std::set<std::string> general_lua_bound, happiness_lua_bound;
    bool positive_happiness_binding = false;
    std::string blocker;
};

// Read selected Expansion2 child contexts and verify that each value control
// is actually assigned by its selected Lua. The parent popup's source SHA is
// checked against the active tab proof.
EconomicFieldSourceProof inspect_economic_field_source(
    const assets::ResourceMountTable &resources,
    const EconomicTabSourceProof &tab_source);

struct EconomicFieldValue {
    std::string control_id;
    std::optional<std::string> value;
    std::string unavailable_reason;
};

struct EconomicBoundFields {
    bool available = false;
    EconomicTab tab = EconomicTab::general;
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame = 0, owner_sequence = 0;
    std::vector<EconomicFieldValue> fields;
    std::string blocker;
};

// Read-only value projection for the one active popup tab. A row is published
// only when selected XML/Lua and this frame's live source agree. Unknown
// values stay individually unavailable; no saved row is substituted.
class EconomicOverviewFields {
  public:
    EconomicOverviewFields(EconomicOverviewLifecycle &lifecycle,
        EconomicOverviewActiveTabs &tabs, EconomicFieldSourceProof source);

    EconomicBoundFields capture(const runtime::LiveSession &session,
        const UiFrameSnapshot &frame, std::int32_t viewport_width,
        std::int32_t viewport_height) const;

  private:
    EconomicOverviewLifecycle *lifecycle_;
    EconomicOverviewActiveTabs *tabs_;
    EconomicFieldSourceProof source_;
};

} // namespace civ5::ui
