#include "civ5/runtime/bnw_happiness_natural_wonders.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t value) {
    need(value>=INT32_MIN && value<=INT32_MAX,"BNW wonder happiness exceeds native int");
    return static_cast<std::int32_t>(value);
}
std::int32_t multiply(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)*b);}
std::int32_t add(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t boost(std::int32_t value,std::int32_t modifier) {
    return modifier>0?multiply(value,add(100,modifier))/100:value;
}
template<class T>const T &named(const std::vector<T> &rows,std::string_view name) {
    const T *found=nullptr;for(const auto &row:rows)if(row.name && std::string_view(row.name)==name) {
        need(!found,"Duplicate BNW wonder happiness field");found=&row;
    }
    need(found,"Missing BNW wonder happiness field");return *found;
}
template<class T>std::int32_t scalar(const std::vector<T> &rows,std::string_view name) {
    const auto &values=named(rows,name).values;need(values.size()==1,"Malformed BNW wonder happiness scalar");return integer(values[0]);
}
}
BnwNaturalWonderHappinessRules BnwNaturalWonderHappinessRules::load(const content::RulesDatabase &database) {
    BnwNaturalWonderHappinessRules result;result.sha256_=identify_bytes(database.gameplay_fingerprint_data()).sha256;
    result.types_=database.cache_info_types();need(result.types_.report().all_succeeded(),"BNW wonder InfoType cache failed");
    const auto report=database.prefetch_info("Features",[&](auto slot,auto,const auto &row) {
        need(slot<=INT32_MAX && result.features_.emplace(static_cast<std::int64_t>(slot),integer(row.integer("InBorderHappiness"))).second,
             "BNW wonder feature collection slot unavailable");return true;
    });
    need(report.all_succeeded(),"BNW wonder feature rules unavailable");
    const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto *entry=defines.find("HAPPINESS_PER_NATURAL_WONDER");
    need(entry && entry->lookup.found && entry->lookup.error.empty() && entry->kind==content::GlobalDefineKind::integer &&
         std::holds_alternative<std::int64_t>(entry->lookup.value),"BNW wonder happiness define unavailable");
    result.per_wonder_=integer(std::get<std::int64_t>(entry->lookup.value));return result;
}
std::int32_t BnwNaturalWonderHappinessRules::feature_happiness(const saves::SavedMapCellProjection &cell) const {
    need(!(cell.feature_type && cell.feature_type_hash),"BNW wonder feature identity is ambiguous");
    std::int64_t feature=-1;
    if(cell.feature_type)feature=*cell.feature_type;
    else if(cell.feature_type_hash) {
        if(!*cell.feature_type_hash)return 0;
        need(*cell.feature_type_hash<=UINT32_MAX,"BNW wonder feature hash exceeds source width");
        const auto *type=types_.find_hash(*cell.feature_type_hash);
        need(type && type->table=="Features","BNW wonder owned feature hash is unresolved");feature=type->id;
    } else throw std::runtime_error("BNW wonder owned feature is unknown");
    if(feature==-1)return 0;
    need(features_.contains(feature),"BNW wonder owned feature has no native Info slot");return features_.at(feature);
}
BnwNaturalWonderHappinessQuote quote_bnw_happiness_from_natural_wonders(const LiveSessionState &state,
    const BnwNaturalWonderHappinessRules &rules,const SessionCheckpointIdentity &identity,std::int64_t player) {
    need(identity.payload_sha256.size()==64 && identity.rules_sha256==rules.rules_sha256(),"BNW wonder selected rules identity differs");
    need(state.setup.profile==content::TechnologyProfile::bnw && state.world.all_players &&
         state.world.all_players->layout==bnw && state.world.all_players->players.size()==64 &&
         state.world.pregame && state.world.teams && state.world.teams->layout==bnw &&
         state.world.teams->teams.size()==64 && state.world.map_projection && player>=0 && player<64,
         "BNW wonder happiness live world is incomplete");
    const auto &raw=state.world.all_players->players.at(static_cast<std::size_t>(player));
    need(raw.saved_player_id==player && raw.core.layout==bnw && raw.traits.layout==bnw && scalar(raw.core.fields,"m_eID")==player,
         "BNW wonder happiness player identity differs");
    const auto team=state.setup.players.at(player).team;
    need(team>=0 && team<64 && state.world.pregame->archive.integer_arrays.at("team_types").at(player)==team,
         "BNW wonder happiness native team binding differs");
    const auto &saved_team=state.world.teams->teams.at(static_cast<std::size_t>(team));
    need(saved_team.index==static_cast<std::uint64_t>(team) && named(saved_team.fields,"id").value==team,"BNW wonder team identity differs");
    BnwNaturalWonderHappinessQuote quote;
    quote.discovered_count=integer(named(saved_team.fields,"natural_wonders").value);
    quote.discovered_base=multiply(quote.discovered_count,rules.per_discovered_wonder());
    quote.discovered_after_trait=boost(quote.discovered_base,scalar(raw.traits.fields,"m_iNaturalWonderHappinessModifier"));
    const auto yield_modifier=scalar(raw.traits.fields,"m_iNaturalWonderYieldModifier");
    const auto &map=*state.world.map_projection;
    need(map.grid.width==state.world.map_header.grid.width && map.grid.height==state.world.map_header.grid.height &&
         map.cells.size()==map.grid.size(),"BNW wonder map owner is incomplete");
    for(std::uint64_t index=0;index<map.cells.size();++index) {
        const auto &cell=map.cells.at(index);const auto xy=map.grid.coordinates(index);
        need(cell.index==index && cell.x==xy[0] && cell.y==xy[1] && cell.owner.has_value(),"BNW wonder plot identity/owner is unknown");
        if(*cell.owner!=player)continue;
        const auto base=rules.feature_happiness(cell);if(base<=0)continue;
        const auto after=boost(base,yield_modifier);quote.owned_positive_features.push_back({index,base,after});
        quote.owned_features=add(quote.owned_features,after);
    }
    quote.total=add(quote.discovered_after_trait,quote.owned_features);return quote;
}
}
