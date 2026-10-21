#include "civ5/runtime/bnw_unit_sight_exploration.hpp"
#include <climits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
std::int64_t plot_field(const saves::PlotObservation &plot,
                        std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &item:plot.fields)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW exploration plot field");found=&item;
        }
    require(found,"BNW exploration plot field is unavailable");
    return found->value;
}
std::int64_t alive(const saves::PlayerCoreObservation &core) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &item:core.fields)
        if(name(item.name)=="m_bAlive") {
            require(!found,"Duplicate BNW exploration player alive field");
            found=&item;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::boolean &&
            found->values.size()==1 &&
            (found->values.front()==0 || found->values.front()==1),
            "BNW exploration player alive state is unavailable");
    return found->values.front();
}
std::vector<saves::NamedSaveInteger> &economic_fields(
    saves::PlayerStrategyObservation &strategy) {
    require(strategy.layout==saves::GameStateLayout::expansion2_version1,
            "BNW exploration strategy profile differs");
    saves::PlayerStrategyBlock *found=nullptr;
    for(auto &block:strategy.blocks)
        if(name(block.name)=="economic_ai") {
            require(!found,"Duplicate BNW economic AI block");found=&block;
        }
    require(found,"BNW economic AI block is unavailable");
    return found->fields;
}
void set_dirty(saves::PlayerStrategyObservation &strategy) {
    auto &fields=economic_fields(strategy);
    saves::NamedSaveInteger *found=nullptr;
    for(auto &field:fields)
        if(name(field.name)=="exploration_plots_dirty") {
            require(!found,"Duplicate BNW exploration dirty field");
            found=&field;
        }
    require(found && (found->value==0 || found->value==1),
            "BNW exploration dirty source is not boolean");
    found->value=1;
}
std::map<std::uint32_t,bool> feature_rules(
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types) {
    require(types.report().all_succeeded(),
            "BNW exploration feature catalog has errors");
    std::map<std::uint32_t,bool> result;
    const auto report=database.query_rows(
        {"SELECT ID,Type,NaturalWonder FROM Features",{}},
        [&](const auto &row) {
            const auto type=row.text("Type");
            require(type && row.has_column("ID") &&
                    row.has_column("NaturalWonder"),
                    "BNW exploration feature rule is incomplete");
            const auto id=row.integer("ID");
            require(id>=0 && id<=INT32_MAX,
                    "BNW exploration feature ID exceeds SDK range");
            const auto *entry=types.find(*type);
            require(entry && entry->table=="Features" && entry->id==id &&
                    entry->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(entry->hash) &&
                    types.find_hash(entry->hash)->type==*type,
                    "BNW exploration feature rule ID/hash disagrees");
            const auto wonder=row.integer("NaturalWonder");
            require(wonder==0 || wonder==1,
                    "BNW NaturalWonder rule flag is not boolean");
            require(result.emplace(entry->hash,wonder==1).second,
                    "Duplicate BNW exploration feature hash");
        });
    require(report.all_succeeded() && report.rows==result.size(),
            "BNW exploration feature rules unavailable");
    return result;
}
}

BnwSightExplorationResult apply_bnw_unit_sight_exploration_detached(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightExplorationResult result;
    try {
        result.sight=apply_bnw_unit_sight_detached(
            state,request,database,types,defines,identity);
        require(result.sight.calculated && result.sight.detached,
                "BNW exploration needs detached setRevealed counter/bit state");
        auto &copy=*result.sight.detached;
        const auto team=result.sight.plan.team;
        require(team>=0 && team<64 && copy.world.map_plots &&
                copy.world.map_projection && copy.world.pregame &&
                copy.world.all_players &&
                copy.world.all_players->layout==
                    saves::GameStateLayout::expansion2_version1,
                "BNW exploration owned team/map/player profile is incomplete");
        const auto features=feature_rules(database,types);
        bool first_reveal=false;
        for(const auto &edge:result.sight.plan.newly_visible) {
            if(!edge.first_reveal) continue;
            first_reveal=true;
            const auto &plot=copy.world.map_plots->plots.at(edge.plot);
            const auto &cell=copy.world.map_projection->cells.at(edge.plot);
            const auto raw=plot_field(plot,"feature_type_hash");
            require(raw>=0 && raw<=UINT32_MAX &&
                    cell.feature_type_hash && *cell.feature_type_hash==
                        static_cast<std::uint64_t>(raw),
                    "BNW exploration feature raw/projected hash disagrees");
            if(raw) {
                const auto found=features.find(static_cast<std::uint32_t>(raw));
                require(found!=features.end(),
                        "BNW exploration feature absent from active rules");
                require(!found->second,
                        "BNW natural wonder requires earlier Lua/reward/notification effects");
            }
        }
        if(first_reveal) {
            // CvPlot::changeVisibilityCount is called by changeAdjacentSight
            // with bInformExplorationTracking=true. SDK sets this after the
            // central setRevealed call and before adjacent owner/meet effects.
            const auto mapping=copy.world.pregame->archive.integer_arrays.find(
                "team_types");
            require(mapping!=copy.world.pregame->archive.integer_arrays.end() &&
                    mapping->second.size()>=64 &&
                    copy.world.all_players->players.size()==64,
                    "BNW exploration owner/team roster is unavailable");
            std::set<std::int64_t> seen;
            for(auto &player:copy.world.all_players->players) {
                const auto id=player.saved_player_id;
                require(id>=0 && id<64 && seen.insert(id).second &&
                        player.core.layout==
                            saves::GameStateLayout::expansion2_version1,
                        "BNW exploration player roster identity changed");
                if(!alive(player.core)) continue;
                const auto player_team=mapping->second.at(
                    static_cast<std::size_t>(id));
                require(player_team>=0 && player_team<64,
                        "BNW exploration living player team is invalid");
                if(player_team!=team) continue;
                set_dirty(player.strategy);
                result.dirty_players.push_back(id);
            }
            require(seen.size()==64,
                    "BNW exploration player roster is incomplete");
        }
        result.calculated=true;
        result.blockers={
            BnwSightExplorationBlocker::natural_wonder_lua_rewards_and_notifications,
            BnwSightExplorationBlocker::sdk_ordered_meet_and_team_diplomacy,
            BnwSightExplorationBlocker::remaining_ui_and_set_xy_effects,
            BnwSightExplorationBlocker::complete_world_checkpoint};
        result.reason="Detached BNW EconomicAI discovery dirty flag; remaining SDK effects block setXY";
    } catch(const std::exception &error) {
        result.calculated=false;
        result.sight.detached.reset();
        result.dirty_players.clear();
        result.blockers={
            BnwSightExplorationBlocker::natural_wonder_lua_rewards_and_notifications};
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
