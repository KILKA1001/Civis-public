#include "civ5/ui/bnw_unit_panel_stat_rows.hpp"
#include <algorithm>
#include <stdexcept>
#include <tuple>

namespace civ5::ui {
namespace {
void label(BnwUnitPanelStatRowRead &row,const TopPanelLocalizationCatalog &catalog,std::string key) {
    row.label_key=std::move(key);
    try {
        const auto &text=catalog.rows.at(row.label_key);
        if (text.find_first_of("{}")!=text.npos) throw std::runtime_error("Parameterized stat label");
        row.label=catalog.format(row.label_key);
    } catch (...) { row.label.reset(); }
}
void static_tooltip(BnwUnitPanelStatRowRead &row,const TopPanelLocalizationCatalog &catalog,std::string key) {
    row.tooltip_key=std::move(key);
    try {
        const auto &text=catalog.rows.at(row.tooltip_key);
        if (text.find_first_of("{}")!=text.npos) throw std::runtime_error("Parameterized stat tooltip");
        row.tooltip_text=catalog.format(row.tooltip_key);
    } catch (...) { row.tooltip_blocker="Selected no-argument combat tooltip is unavailable"; }
}
void numeric_tooltip(BnwUnitPanelStatRowRead &row,std::string key,
    std::vector<BnwUnitPanelNumericArgument> arguments,std::string_view locale) {
    row.tooltip_key=std::move(key); row.tooltip_arguments=std::move(arguments);
    row.tooltip_blocker=(locale=="ru_RU" || locale=="RU_RU" || locale=="Language_RU_RU") ?
        "Selected RU parameter/plural composition owner is unavailable" :
        "Selected native parameter composition owner is unavailable";
}
BnwUnitPanelStatRowsRead empty_rows(const TopPanelLocalizationCatalog &catalog) {
    BnwUnitPanelStatRowsRead rows;
    rows.movement.shown=true;
    label(rows.movement,catalog,"TXT_KEY_UPANEL_MOVEMENT");
    label(rows.strength,catalog,"TXT_KEY_UPANEL_STRENGTH");
    label(rows.ranged,catalog,"TXT_KEY_UPANEL_RANGED_ATTACK");
    rows.strength.visibility_blocker="Current religious conversion/tourism fallback owners are unavailable";
    rows.ranged.visibility_blocker="Current spreads/heresy/cargo fallback owners are unavailable";
    return rows;
}
void positive(BnwUnitPanelStatRowRead &row,std::string value,
    const TopPanelLocalizationCatalog &catalog,std::string tooltip) {
    row.shown=true; row.value=std::move(value); row.visibility_blocker.clear(); row.value_blocker.clear();
    static_tooltip(row,catalog,std::move(tooltip));
}
std::int64_t current_moves(const runtime::LiveSessionState &state,std::int64_t player,std::int64_t selected) {
    std::optional<std::int64_t> value;
    for (const auto &owner : state.world.all_players->players) if (owner.saved_player_id==player)
        for (const auto &unit : owner.units.units) {
            std::optional<std::int64_t> id,moves;
            for (const auto &field : unit.core.fields) if (field.name && field.values.size()==1) {
                if (std::string_view(field.name)=="m_iID") id=field.values.front();
                if (std::string_view(field.name)=="m_iMoves") moves=field.values.front();
            }
            if (id==selected) {
                if (value || !moves) throw std::runtime_error("Ambiguous current movement operand");
                value=std::max<std::int64_t>(0,*moves); // SDK movesLeft().
            }
        }
    if (!value) throw std::runtime_error("Missing current movement operand");
    return *value;
}
}
BnwUnitPanelStatRowsRead read_detached_bnw_air_stat_rows(const BnwUnitPanelSecondaryInfo &info,
    const TopPanelLocalizationCatalog &catalog,std::string_view locale) {
    auto rows=empty_rows(catalog);
    if (info.info_domain!=1 || info.unit_type<0) {
        rows.movement.value_blocker="Detached Info is not a bound native AIR branch";
        rows.strength.value_blocker="Detached Info is not a bound native AIR branch";
        return rows;
    }
    label(rows.movement,catalog,"TXT_KEY_UPANEL_RANGEMOVEMENT");
    if (info.range) rows.movement.value=std::to_string(*info.range)+" [ICON_MOVES]";
    else rows.movement.value_blocker=info.range_blocker;
    if (info.range && info.rebase_range_multiplier) numeric_tooltip(rows.movement,
        "TXT_KEY_UPANEL_UNIT_MAY_STRIKE_REBASE",{{*info.range,1},
            {std::int64_t(*info.range)*(*info.rebase_range_multiplier),100}},locale);
    else rows.movement.tooltip_blocker=info.rebase_blocker.empty() ? info.range_blocker : info.rebase_blocker;
    if (info.base_ranged_combat && *info.base_ranged_combat>0) positive(rows.strength,
        std::to_string(*info.base_ranged_combat)+" [ICON_STRENGTH]",catalog,"TXT_KEY_UPANEL_STRENGTH_TT");
    // Lua explicitly skips AIR's duplicate ranged-combat branch, but then
    // evaluates spreads, heresy and cargo. Their absence is not proven here.
    return rows;
}
BnwUnitPanelStatRowsBridge::BnwUnitPanelStatRowsBridge(const runtime::LiveSession &session,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const runtime::SessionCheckpointIdentity &identity,std::int64_t active_player)
    :primary_(session,database,types,identity,active_player),
     secondary_(session,database,types,identity,active_player),database_(&database),types_(&types),identity_(identity) {}
std::optional<BnwUnitPanelReadRows> BnwUnitPanelStatRowsBridge::capture(const runtime::LiveSession &session,
    std::int64_t selected,std::uint64_t frame,const TopPanelLocalizationCatalog &catalog,std::string_view locale) const {
    try {
        const auto primary=primary_.capture(session,selected,frame);
        const auto secondary=secondary_.capture(session,selected,frame);
        if (!primary || !secondary || locale.empty() ||
            std::tie(primary->player_id,primary->unit_id,primary->turn,primary->frame_number,primary->source_world_revision)!=
            std::tie(secondary->player_id,secondary->unit_id,secondary->turn,secondary->frame_number,secondary->source_world_revision))
            return std::nullopt;
        auto rows=secondary->values.info_domain==1 ? read_detached_bnw_air_stat_rows(secondary->values,catalog,locale) : empty_rows(catalog);
        if (secondary->values.info_domain!=1) {
            rows.movement.value=primary->movement; rows.movement.value_blocker=primary->movement_blocker;
            if (primary->movement) {
                const auto movement=runtime::quote_bnw_unit_movement(session.state(),*database_,*types_,identity_,
                    {primary->player_id,selected});
                if (movement.max_moves && movement.terms.denominator>0) numeric_tooltip(rows.movement,
                    "TXT_KEY_UPANEL_UNIT_MAY_MOVE",{{current_moves(session.state(),primary->player_id,selected),
                        movement.terms.denominator}},locale);
                else rows.movement.tooltip_blocker="Current native movement denominator is unavailable";
            } else rows.movement.tooltip_blocker=primary->movement_blocker;
            if (primary->strength) positive(rows.strength,*primary->strength,catalog,"TXT_KEY_UPANEL_STRENGTH_TT");
            else rows.strength.value_blocker=primary->strength_blocker;
            if (secondary->values.base_ranged_combat && *secondary->values.base_ranged_combat>0)
                positive(rows.ranged,std::to_string(*secondary->values.base_ranged_combat)+" [ICON_RANGE_STRENGTH]",
                    catalog,"TXT_KEY_UPANEL_RANGED_ATTACK_TT");
        }
        if (!primary_.matches(session,*primary,selected,frame) || !secondary_.matches(session,*secondary,selected,frame))
            return std::nullopt;
        return BnwUnitPanelReadRows{primary->player_id,selected,primary->turn,frame,
            primary->source_world_revision,std::string(locale),std::move(rows)};
    } catch (...) { return std::nullopt; }
}
bool BnwUnitPanelStatRowsBridge::matches(const runtime::LiveSession &session,const BnwUnitPanelReadRows &saved,
    std::int64_t selected,std::uint64_t frame,const TopPanelLocalizationCatalog &catalog,std::string_view locale) const noexcept {
    try {
        const auto current=capture(session,selected,frame,catalog,locale);
        return current && std::tie(saved.player_id,saved.unit_id,saved.turn,saved.frame_number,saved.source_world_revision,saved.locale)==
            std::tie(current->player_id,current->unit_id,current->turn,current->frame_number,current->source_world_revision,current->locale) &&
            saved.rows==current->rows;
    } catch (...) { return false; }
}
} // namespace civ5::ui
