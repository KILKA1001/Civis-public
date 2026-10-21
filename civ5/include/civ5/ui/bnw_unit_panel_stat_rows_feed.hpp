#pragma once
#include "civ5/ui/bnw_unit_panel_stat_rows.hpp"
#include <memory>
#include <stdexcept>
#include <tuple>

namespace civ5::ui {
namespace detail {
struct BnwUnitPanelStatRowsFeedBinding {
    const runtime::LiveSession *session;
    const content::RulesDatabase *database;
    const content::InfoTypeCatalog *types;
    const runtime::SessionCheckpointIdentity *identity;
    std::int64_t active_player;
    std::shared_ptr<const TopPanelLocalizationCatalog> catalog;
    std::string locale;
    std::unique_ptr<BnwUnitPanelStatRowsBridge> bridge;
    std::uint64_t generation=0;
    std::optional<BnwUnitPanelReadRows> latest;

    std::optional<BnwUnitPanelReadRows> capture(const runtime::LiveSession &source,
        std::uint64_t frame,std::int64_t selected) {
        if (&source!=session || selected<0) return std::nullopt;
        const auto current_generation=source.state_generation();
        if (!bridge || generation!=current_generation) {
            // Only the capture path can bind a replacement generation. A
            // current check never repairs or replaces its borrowed owner.
            latest.reset();
            bridge=std::make_unique<BnwUnitPanelStatRowsBridge>(source,*database,*types,*identity,active_player);
            generation=current_generation;
        }
        latest.reset();
        auto read=bridge->capture(source,selected,frame,*catalog,locale);
        if (source.state_generation()!=generation) { latest.reset(); return std::nullopt; }
        latest=read;
        return read;
    }
    bool current(const runtime::LiveSession &source,const BnwUnitPanelReadRows &read,
        std::int64_t selected,std::uint64_t frame) const noexcept {
        try {
            if (&source!=session || !bridge || !latest || source.state_generation()!=generation ||
                std::tie(read.player_id,read.unit_id,read.turn,read.frame_number,read.source_world_revision,read.locale)!=
                std::tie(latest->player_id,latest->unit_id,latest->turn,latest->frame_number,latest->source_world_revision,latest->locale) ||
                read.rows!=latest->rows) return false;
            return bridge->matches(source,read,selected,frame,*catalog,locale) &&
                source.state_generation()==generation;
        } catch (...) { return false; }
    }
};
} // namespace detail

// The session and existing DB/types/identity owner must outlive the synchronous
// host and all copied sources. The selected catalog is retained by this binder.
// Temporary ABI limit: ReadRows has no generation field. Identical value-only
// rows from a previous generation cannot be distinguished after a fresh same-
// frame capture; production MUST retain its UiOwnerFences generation seal.
// This helper adds no readiness and does not infer or own selected-unit input.
inline void install_bnw_unit_panel_stat_rows_feed(LiveSessionUiSources &sources,
    const runtime::LiveSession &session,std::int64_t active_player,
    std::shared_ptr<const TopPanelLocalizationCatalog> catalog,std::string locale) {
    if (!sources.unit_stats_database || !sources.unit_stats_types || !sources.unit_stats_identity ||
        !catalog || locale.empty() || active_player<0)
        throw std::invalid_argument("Native UnitPanel rows require actual DB/types/identity/player/locale owners");
    auto binding=std::make_shared<detail::BnwUnitPanelStatRowsFeedBinding>();
    binding->session=&session;binding->database=sources.unit_stats_database;
    binding->types=sources.unit_stats_types;binding->identity=sources.unit_stats_identity;
    binding->active_player=active_player;binding->catalog=std::move(catalog);binding->locale=std::move(locale);
    auto capture=[binding](const runtime::LiveSession &source,std::uint64_t frame,std::int64_t selected) {
        try { return binding->capture(source,frame,selected); }
        catch (...) { return std::optional<BnwUnitPanelReadRows>{}; }
    };
    auto current=[binding](const runtime::LiveSession &source,const BnwUnitPanelReadRows &read,
        std::int64_t selected,std::uint64_t frame) { return binding->current(source,read,selected,frame); };
    sources.selected_unit_stat_rows=std::move(capture);
    sources.selected_unit_stat_rows_current=std::move(current);
}
} // namespace civ5::ui