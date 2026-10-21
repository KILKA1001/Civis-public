#include "civ5/ui/bnw_unit_panel_secondary_stats.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <stdexcept>

namespace civ5::ui {
namespace {
std::int64_t scalar(const saves::UnitCoreObservation &core, std::string_view name) {
    std::optional<std::int64_t> result;
    for (const auto &field : core.fields) if (field.name && std::string_view(field.name)==name) {
        if (result || field.values.size()!=1) throw std::runtime_error("Ambiguous selected unit scalar");
        result=field.values.front();
    }
    if (!result) throw std::runtime_error("Missing selected unit scalar");
    return *result;
}
std::int32_t narrow(std::int64_t value) {
    if (value<INT32_MIN || value>INT32_MAX) throw std::runtime_error("Native UnitPanel read exceeds int32");
    return static_cast<std::int32_t>(value);
}
const saves::UnitCoreObservation &selected(const runtime::LiveSessionState &state,
    std::int64_t player, std::int64_t id) {
    (void)runtime::bnw_unit_order_plot(state,{player,id});
    const saves::UnitCoreObservation *result=nullptr;
    for (const auto &owner : state.world.all_players->players) if (owner.saved_player_id==player)
        for (const auto &unit : owner.units.units) if (scalar(unit.core,"m_iID")==id) {
            if (result) throw std::runtime_error("Ambiguous selected unit owner");
            result=&unit.core;
        }
    if (!result || scalar(*result,"m_eOwner")!=player || scalar(*result,"m_bDeathDelay")!=0)
        throw std::runtime_error("Selected owned unit is absent/dying");
    return *result;
}
}
BnwUnitPanelSecondaryInfo read_bnw_unit_panel_secondary_info(
    const content::RulesDatabase &database, const content::InfoTypeCatalog &types,
    std::uint32_t hash, std::int64_t extra_range) {
    BnwUnitPanelSecondaryInfo result;
    result.range_blocker="Selected native Range Info/current operand is unavailable";
    result.base_ranged_blocker="Selected native base-ranged Info is unavailable";
    result.rebase_blocker="Selected AIR_UNIT_REBASE_RANGE_MULTIPLIER is unavailable";
    const auto *info=types.find_hash(hash);
    if (!types.report().all_succeeded() || !info || info->table!="Units") return result;
    try {
        bool found=false;
        const auto query=database.query_rows({"SELECT Type,Domain,Range,RangedCombat FROM Units WHERE ID=?",{info->id}},
            [&](const content::RulesRowView &row) {
                if (found || row.text("Type")!=info->type) throw std::runtime_error("Selected Unit Info differs");
                found=true;
                const auto name=row.text("Domain");
                const auto *entry=name ? types.find(*name) : nullptr;
                constexpr std::array<std::string_view,5> domains{
                    "DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
                if (!entry || entry->table!="Domains" || entry->id<0 || entry->id>=5 ||
                    entry->type!=domains.at(entry->id)) throw std::runtime_error("Selected native info_domain Info is unresolved");
                result.unit_type=info->id; result.info_domain=entry->id;
                // SDK CvUnit.cpp:9669 GetRange = UnitInfo.Range + current extra range.
                // CvUnit.cpp:10689 GetBaseRangedCombatStrength reads Info.RangedCombat;
                // the serialized m_iBaseRangedCombat is not this getter's source.
                const auto range=row.integer("Range"), combat=row.integer("RangedCombat");
                if (row.has_column("Range") && extra_range>=INT32_MIN && extra_range<=INT32_MAX &&
                    range>=INT32_MIN && range<=INT32_MAX &&
                    range+extra_range>=INT32_MIN && range+extra_range<=INT32_MAX) {
                    result.range=narrow(range+extra_range); result.range_blocker.clear();
                }
                if (row.has_column("RangedCombat") && combat>=0 && combat<=INT32_MAX) {
                    result.base_ranged_combat=narrow(combat); result.base_ranged_blocker.clear();
                }
            },2);
        if (!query.all_succeeded() || !found) return BnwUnitPanelSecondaryInfo{
            .unit_type=-1, .info_domain=-1, .range={}, .base_ranged_combat={}, .rebase_range_multiplier={},
            .range_blocker="Selected native Range query failed",
            .base_ranged_blocker="Selected native base-ranged query failed",
            .rebase_blocker="Selected native Info query failed"};
        // Original UnitPanel.lua:631-632 uses this selected define only for
        // the rebase tooltip; its absence does not erase a proven range read.
        const auto define=database.query_rows({"SELECT Value FROM Defines WHERE Name=?",
            {std::string("AIR_UNIT_REBASE_RANGE_MULTIPLIER")}},[&](const content::RulesRowView &row) {
                if (result.rebase_range_multiplier) throw std::runtime_error("Duplicate selected rebase define");
                const auto value=row.integer("Value");
                if (row.has_column("Value") && value>=0 && value<=INT32_MAX) {
                    result.rebase_range_multiplier=narrow(value); result.rebase_blocker.clear();
                }
            },2);
        if (!define.all_succeeded()) { result.rebase_range_multiplier.reset();
            result.rebase_blocker="Selected rebase define query failed"; }
    } catch (const std::exception &) {
        return BnwUnitPanelSecondaryInfo{
            .unit_type=-1, .info_domain=-1, .range={}, .base_ranged_combat={}, .rebase_range_multiplier={},
            .range_blocker="Selected native Range query failed",
            .base_ranged_blocker="Selected native base-ranged query failed",
            .rebase_blocker="Selected native Info query failed"};
    }
    return result;
}
BnwUnitPanelSecondaryStatsBridge::BnwUnitPanelSecondaryStatsBridge(const runtime::LiveSession &session,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const runtime::SessionCheckpointIdentity &identity,std::int64_t active_player)
    : session_(&session),state_(&session.state()),state_generation_(session.state_generation()),database_(&database),types_(&types),identity_(identity),player_(active_player) {
    if (!types.report().all_succeeded() ||
        identity.payload_sha256!=runtime::identify_bytes(session.source().source.bytes).sha256 ||
        identity.rules_sha256!=database.gameplay_fingerprint_sha256())
        throw std::invalid_argument("UnitPanel secondary selected source/rules binding differs");
}
std::optional<BnwUnitPanelSecondaryStats> BnwUnitPanelSecondaryStatsBridge::capture(
    const runtime::LiveSession &session,std::int64_t id,std::uint64_t frame) const {
    if (&session!=session_ || &session.state()!=state_ || session.state_generation()!=state_generation_ || !frame || id<0) return std::nullopt;
    const auto &state=session.state();
    const auto owner=state.setup.players.find(player_);
    if (owner==state.setup.players.end() || !owner->second.alive ||
        state.setup.profile!=content::TechnologyProfile::bnw) return std::nullopt;
    try {
        const auto &core=selected(state,player_,id);
        const auto hash=scalar(core,"idUnitType");
        if (hash<0 || hash>UINT32_MAX) return std::nullopt;
        const auto values=read_bnw_unit_panel_secondary_info(*database_,*types_,
            static_cast<std::uint32_t>(hash),scalar(core,"m_iExtraRange"));
        if (values.unit_type<0 || values.info_domain<0 || scalar(core,"m_eUnitType")!=values.unit_type ||
            state.maintenance.players.at(player_).units.at(id).type!=values.unit_type) return std::nullopt;
        BnwUnitPanelSecondaryStats result;
        result.player_id=player_; result.unit_id=id; result.turn=narrow(state.setup.game_turn);
        result.frame_number=frame;
        result.source_world_revision=runtime::bnw_unit_order_world_revision(state,identity_,*database_);
        result.values=values;
        return result;
    } catch (const std::exception &) { return std::nullopt; }
}
bool BnwUnitPanelSecondaryStatsBridge::matches(const runtime::LiveSession &session,
    const BnwUnitPanelSecondaryStats &saved,std::int64_t id,std::uint64_t frame) const {
    if (saved.player_id!=player_ || saved.unit_id!=id || saved.frame_number!=frame ||
        saved.turn!=session.state().setup.game_turn) return false;
    const auto current=capture(session,id,frame);
    return current && current->source_world_revision==saved.source_world_revision &&
        current->values.unit_type==saved.values.unit_type && current->values.info_domain==saved.values.info_domain &&
        current->values.range==saved.values.range && current->values.base_ranged_combat==saved.values.base_ranged_combat &&
        current->values.rebase_range_multiplier==saved.values.rebase_range_multiplier &&
        current->values.range_blocker==saved.values.range_blocker &&
        current->values.base_ranged_blocker==saved.values.base_ranged_blocker &&
        current->values.rebase_blocker==saved.values.rebase_blocker;
}
} // namespace civ5::ui
