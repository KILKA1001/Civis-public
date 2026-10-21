#pragma once
#include "civ5/ui/live_session_frame.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"

namespace civ5::ui {
// A synchronous read-only bridge borrowed by the stable selected preview host.
// Restore replaces the state address: detach and construct a new bridge then.
class BnwTopPanelPreviewBridge {
  public:
    BnwTopPanelPreviewBridge(const runtime::LiveSession &session,
        const content::RulesDatabase &database,
        const runtime::SessionCheckpointIdentity &identity, std::int64_t active_player);
    LiveSessionUiQuotes capture(const runtime::LiveSession &session, std::uint64_t frame) const;
    // Repeats only this owner's native gold/science read; no mutable caches,
    // full-world revision or unknown extension comparison is implied.
    bool matches(const runtime::LiveSession &session, const LiveSessionUiQuotes &quotes,
        std::uint64_t frame) const noexcept;
    std::optional<std::string> blocker(const runtime::LiveSession &session) const;
  private:
    std::function<bool()> immutable_rules_current_;
    const runtime::LiveSession *session_;
    const runtime::LiveSessionState *state_;
    std::uint64_t state_generation_;
    std::int64_t player_;
    runtime::CityYieldRules city_rules_;
    runtime::MaintenanceRules maintenance_rules_;
    runtime::TreasuryRules treasury_rules_;
    content::GlobalDefineCache defines_;
};
} // namespace civ5::ui
