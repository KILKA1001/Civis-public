#include "civ5/runtime/normalised_bnw_state.hpp"
#include "civ5/runtime/live_session.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace civ5::runtime {
namespace {
std::string json_quote(std::string_view value){
    constexpr char hex[]="0123456789abcdef";
    std::string out="\"";
    for(const char source_byte:value){
        const auto byte=static_cast<unsigned char>(source_byte);
        if(byte=='"'||byte=='\\'){out+='\\';out+=static_cast<char>(byte);}
        else if(byte<32){out+="\\u00";out+=hex[byte>>4];out+=hex[byte&15];}
        else out+=static_cast<char>(byte);
    }
    out+='"';return out;
}
std::string key(std::string_view value){
    std::string out;
    for(const char c:value){if(c=='~')out+="~0";else if(c=='/')out+="~1";else out+=c;}
    return out;
}
std::string named(const char* value){if(!value)throw std::invalid_argument("Null source field name");return key(value);}
template<class T>std::string scalar(T value){
    static_assert(std::is_integral_v<T>);
    if constexpr(std::is_same_v<T,bool>)return value?"true":"false";
    else return std::to_string(value);
}
template<class Range>std::string list(const Range& values){
    std::string out="[";bool first=true;
    for(const auto& value:values){if(!first)out+=',';first=false;out+=scalar(value);}
    out+=']';return out;
}
std::string bytes(const std::vector<std::uint8_t>& values){
    constexpr char hex[]="0123456789abcdef";std::string out="\"";
    for(const auto value:values){out+=hex[value>>4];out+=hex[value&15];}
    out+='"';return out;
}
struct Writer {
    NormalisedBnwState state;
    void token(const std::string& path,std::string value){
        if(!state.values.emplace(path,std::move(value)).second)
            throw std::invalid_argument("Duplicate normalised path: "+path);
    }
    template<class T>void put(const std::string& path,T value){token(path,scalar(value));}
    template<class Range>void seq(const std::string& path,const Range& values){token(path,list(values));}
    void blob(const std::string& path,const std::vector<std::uint8_t>& value){token(path,bytes(value));}
    void gap(const std::string& path,const std::string& reason){state.unresolved.emplace(path,reason);}
    template<class T>void optional(const std::string& path,const std::optional<T>& value){
        if(value)put(path,*value);else gap(path,"Absent decoded/owned value; no default inferred");
    }
    template<class Range>void fields(const std::string& path,const Range& rows){
        for(const auto& row:rows)put(path+"/"+named(row.name),row.value);
    }
    void arrays(const std::string& path,const std::vector<saves::PlotArrayObservation>& rows){
        for(const auto& row:rows){const auto p=path+"/"+named(row.name);put(p+"/columns",row.columns);seq(p+"/values",row.values);}
    }
    void hashes(const std::string& path,const std::vector<saves::TeamHashedArray>& rows){
        for(const auto& row:rows){
            const auto p=path+"/"+named(row.name);
            // Hash holes and duplicate source entries can affect remapping.
            // Their order is retained until a resolved semantic owner exists.
            for(std::size_t i=0;i<row.rows.size();++i){const auto q=p+"/"+std::to_string(i);put(q+"/hash",row.rows[i].type_hash);seq(q+"/values",row.rows[i].values);}
            put(p+"/count",row.rows.size());
        }
    }
};
void core(Writer& w,const std::string& path,const saves::PlayerCoreObservation& source){
    for(const auto& field:source.fields){
        const auto p=path+"/"+named(field.name);auto values=field.values;
        if(field.encoding==saves::PlayerCoreEncoding::set_i32){std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());}
        w.seq(p+"/values",values);w.seq(p+"/row_counts",field.row_counts);w.seq(p+"/hashes",field.hashes);w.blob(p+"/bytes_hex",field.bytes);
    }
}
void core(Writer& w,const std::string& path,const saves::CityCoreObservation& source){
    for(const auto& field:source.fields){
        const auto p=path+"/"+named(field.name);auto values=field.values;
        if(field.encoding==saves::CityCoreEncoding::set_i32){std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());}
        w.seq(p+"/values",values);w.seq(p+"/row_keys",field.row_keys);w.seq(p+"/row_counts",field.row_counts);w.seq(p+"/hashes",field.hashes);w.blob(p+"/bytes_hex",field.bytes);
    }
}
void core(Writer& w,const std::string& path,const saves::UnitCoreObservation& source){
    for(const auto& field:source.fields){
        const auto p=path+"/"+named(field.name);auto values=field.values;
        if(field.encoding==saves::UnitCoreEncoding::set_i32){std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());}
        w.seq(p+"/values",values);w.blob(p+"/bytes_hex",field.bytes);
    }
}
template<class Node>void nodes(Writer& w,const std::string& path,const std::vector<Node>& source){
    w.put(path+"/count",source.size());
    for(std::size_t i=0;i<source.size();++i){
        const auto& node=source[i];const auto p=path+"/"+std::to_string(i)+"/"+named(node.name);
        w.seq(p+"/values",node.values);
        if constexpr(requires{node.bytes;})w.blob(p+"/bytes_hex",node.bytes);
        nodes(w,p+"/children",node.children);
    }
}
template<class Record>void records(Writer& w,const std::string& path,const std::vector<Record>& source){
    w.put(path+"/count",source.size());
    for(std::size_t i=0;i<source.size();++i){
        const auto& row=source[i];const auto p=path+"/"+std::to_string(i)+"/"+named(row.name);
        w.fields(p+"/fields",row.fields);w.arrays(p+"/arrays",row.arrays);
        for(const auto& blob:row.blobs)w.blob(p+"/bytes_hex/"+named(blob.name),blob.bytes);
        records(w,p+"/children",row.children);
    }
}
template<class Range>void integer_map(Writer& w,const std::string& path,const Range& rows){
    for(const auto& [id,value]:rows)w.put(path+"/"+std::to_string(id),value);
    w.put(path+"/count",rows.size());
}
void route(Writer& w,const std::string& p,const saves::TradeRouteObservation& r){
    w.optional(p+"/id",r.id);w.put(p+"/origin_x",r.origin_x);w.put(p+"/origin_y",r.origin_y);
    w.put(p+"/destination_x",r.destination_x);w.put(p+"/destination_y",r.destination_y);
    w.put(p+"/origin_owner",r.origin_owner);w.put(p+"/destination_owner",r.destination_owner);
    w.put(p+"/domain",r.domain);w.put(p+"/connection_type",r.connection_type);
    w.put(p+"/unit_location_index",r.unit_location_index);w.put(p+"/unit_id",r.unit_id);
    w.put(p+"/moving_forward",r.moving_forward);w.put(p+"/circuits_completed",r.circuits_completed);
    w.put(p+"/circuits_to_complete",r.circuits_to_complete);w.put(p+"/turn_route_complete",r.turn_route_complete);
    w.seq(p+"/origin_yields",r.origin_yields);w.seq(p+"/destination_yields",r.destination_yields);
    w.put(p+"/plots/count",r.plots.size());
    for(std::size_t i=0;i<r.plots.size();++i){const auto q=p+"/plots/"+std::to_string(i);w.put(q+"/x",r.plots[i].x);w.put(q+"/y",r.plots[i].y);}
}
void owned(Writer& w,const LiveSessionState& s){
    const auto& setup=s.setup;
    w.put("/owned/clock/game_turn",setup.game_turn);w.put("/owned/clock/sent_auto_moves",s.game_clock.sent_auto_moves);
    w.put("/owned/setup/game_type",setup.game_type);w.put("/owned/setup/speed",setup.speed);
    w.put("/owned/setup/start_era",setup.start_era);w.put("/owned/setup/current_era",setup.current_era);
    w.put("/owned/setup/game_handicap",setup.game_handicap);w.put("/owned/setup/map_random_seed",setup.map_random_seed);
    if(s.game_clock.pending){const auto& p=*s.game_clock.pending;w.put("/owned/clock/pending/turn",p.turn);w.put("/owned/clock/pending/next_step",p.next_step);w.put("/owned/clock/pending/uncertain",p.uncertain);}
    else w.token("/owned/clock/pending","null");
    for(const auto& [id,p]:setup.players){
        const auto base="/owned/players/"+std::to_string(id);
        w.put(base+"/team",p.team);w.put(base+"/alive",p.alive);w.put(base+"/ever_alive",p.ever_alive);
        w.put(base+"/handicap",p.handicap);w.put(base+"/slot_status",p.slot_status);w.put(base+"/slot_claim",p.slot_claim);
        if(p.policies_resolved)w.seq(base+"/policies",p.policies);else w.gap(base+"/policies","Unresolved active policy IDs");
    }
    w.put("/owned/players/count",setup.players.size());
    integer_map(w,"/owned/setup/team_members",setup.team_members);
    for(const auto& [id,p]:s.player_research.players){const auto base="/owned/research/players/"+std::to_string(id);w.seq(base+"/queue",p.queue);w.put(base+"/overflow_times100",p.overflow_times100);w.put(base+"/max_effective_cities",p.max_effective_cities);w.put(base+"/capital",p.capital);w.put(base+"/pregame_minor",p.pregame_minor);}
    for(const auto& [id,t]:s.player_research.teams){
        const auto p="/owned/research/teams/"+std::to_string(id);w.put(p+"/alive_count",t.alive_count);w.seq(p+"/members",t.members);
        w.seq(p+"/met",t.met);w.seq(p+"/embassy",t.embassy);w.seq(p+"/at_war",t.at_war);
        if(t.war_peace){integer_map(w,p+"/locked_turns",t.war_peace->locked_turns);w.seq(p+"/permanent",t.war_peace->permanent);}
        else w.gap(p+"/war_peace","Missing owned war/peace extension");
    }
    for(const auto& t:s.technologies){
        if(t.slots()>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))throw std::invalid_argument("Technology slots exceed owned accessor ID domain");
        const auto p="/owned/technologies/"+std::to_string(t.team());w.put(p+"/slots",t.slots());w.put(p+"/last_tech",t.last_tech());
        for(std::uint64_t i=0;i<t.slots();++i){const auto v=t.at(static_cast<std::int64_t>(i));const auto q=p+"/"+std::to_string(i);w.put(q+"/has_tech",v.has_tech);w.put(q+"/no_trade",v.no_trade);w.put(q+"/progress_times100",v.progress_times100);w.put(q+"/count",v.count);}
        if(!t.remap().all_mapped())w.gap(p+"/type_remap","Unknown/outside saved technology identity");
    }
    for(const auto& [owner,player]:s.cities.players){
        const auto base="/owned/cities/"+std::to_string(owner);w.put(base+"/total_population",player.total_population);
        w.optional(base+"/excess_happiness",player.excess_happiness);
        for(const auto& [id,c]:player.cities){
            const auto p=base+"/"+std::to_string(id);w.put(p+"/owner",c.owner);w.put(p+"/id",c.id);
            w.put(p+"/x",c.x);w.put(p+"/y",c.y);w.put(p+"/area",c.area);w.put(p+"/population",c.population);
            w.put(p+"/resistance",c.resistance);w.put(p+"/razing",c.razing);w.put(p+"/puppet",c.puppet);
            integer_map(w,p+"/real_buildings",c.real_buildings);integer_map(w,p+"/free_buildings",c.free_buildings);
            if(!c.building_counts_resolved)w.gap(p+"/building_identity","Building counts not resolved");
            for(const auto& [yield,v]:c.yields){const auto q=p+"/yields/"+std::to_string(yield);w.put(q+"/terrain",v.terrain);w.put(q+"/buildings",v.buildings);w.put(q+"/specialists",v.specialists);w.put(q+"/misc",v.misc);w.put(q+"/religion",v.religion);w.put(q+"/per_population",v.per_population);w.put(q+"/per_religion",v.per_religion);w.put(q+"/rate_modifier",v.rate_modifier);w.put(q+"/resource_modifier",v.resource_modifier);w.put(q+"/process_modifier",v.process_modifier);}
            if(c.production_orders){w.put(p+"/orders/count",c.production_orders->size());for(std::size_t i=0;i<c.production_orders->size();++i){const auto& o=(*c.production_orders)[i];const auto q=p+"/orders/"+std::to_string(i);w.put(q+"/type",o.type);w.put(q+"/item",o.item);w.put(q+"/secondary",o.secondary);w.put(q+"/save",o.save);w.put(q+"/rush",o.rush);w.optional(q+"/source_hash",o.source_hash);}}
            else w.gap(p+"/orders","Production queue absent");
            if(c.production_progress){const auto& v=*c.production_progress;w.put(p+"/production/overflow_times100",v.overflow_times100);w.put(p+"/production/feature_times100",v.feature_times100);integer_map(w,p+"/production/units",v.units);integer_map(w,p+"/production/unit_times",v.unit_times);integer_map(w,p+"/production/buildings",v.buildings);integer_map(w,p+"/production/building_times",v.building_times);integer_map(w,p+"/production/projects",v.projects);if(!v.fully_resolved)w.gap(p+"/production/type_remap","Unresolved source orders/progress hashes retained elsewhere");}
            else w.gap(p+"/production","Production progress absent");
        }
    }
    for(const auto& [id,p]:s.treasury.players){const auto q="/owned/treasury/"+std::to_string(id);w.put(q+"/balance_times100",p.balance_times100);w.put(q+"/connections_times100",p.connections_times100);w.put(q+"/lifetime_gross_gold",p.lifetime_gross_gold);w.seq(q+"/balance_history_times100",p.balance_history_times100);w.seq(q+"/income_history_times100",p.income_history_times100);}
    for(const auto& [name,rng]:std::vector<std::pair<std::string,const SimulationRandom*>>{{"map",&s.random.map},{"gameplay",&s.random.gameplay}}){const auto p="/owned/rng/"+name;const auto& v=rng->state();w.put(p+"/algorithm",static_cast<unsigned>(rng->algorithm()));w.put(p+"/seed",v.seed);w.put(p+"/calls",v.calls);w.put(p+"/resets",v.resets);}
    if(s.plot_visibility.current){w.put("/owned/fog/team_slots",s.plot_visibility.team_slots);w.seq("/owned/fog/visibility_counts",s.plot_visibility.counts);}
    else w.gap("/owned/fog/visibility_counts","Live post-load visibility owner not current; saved raw arrays remain separate");
    w.put("/owned/events/city_dirty/count",s.city_dirty_events.size());
    for(std::size_t i=0;i<s.city_dirty_events.size();++i){const auto& e=s.city_dirty_events[i];const auto p="/owned/events/city_dirty/"+std::to_string(i);w.put(p+"/owner",e.owner);w.put(p+"/city",e.city);w.put(p+"/plot",e.plot);w.put(p+"/yield",e.yield);w.put(p+"/kind",static_cast<std::int32_t>(e.kind));w.put(p+"/value",e.value);}
    w.put("/owned/events/plot_gameplay/count",s.plot_gameplay_events.size());
    for(std::size_t i=0;i<s.plot_gameplay_events.size();++i){const auto& e=s.plot_gameplay_events[i];const auto p="/owned/events/plot_gameplay/"+std::to_string(i);w.put(p+"/plot",e.plot);w.put(p+"/x",e.x);w.put(p+"/y",e.y);w.seq(p+"/yields",e.yields);w.put(p+"/kind",static_cast<unsigned>(e.kind));}
}
void raw(Writer& w,const saves::SavedSessionSnapshot& s){
    const auto* prefix=std::get_if<saves::Expansion2PrefixObservation>(&s.game_state.prefix);
    if(prefix){for(std::size_t i=0;i<prefix->scalars.size();++i)w.put("/raw/clock/"+std::string(prefix->scalar_names[i]),prefix->scalars[i]);}
    else w.gap("/raw/clock","Missing BNW Expansion2 prefix");
    w.fields("/raw/game/controls",s.game_state.controls);w.fields("/raw/game/flags",s.game_state.flags);
    if(s.game_state.roster){
        const auto& roster=*s.game_state.roster;w.blob("/raw/game/script_hex",roster.script_data);
        for(std::size_t i=0;i<roster.arrays.size();++i)w.seq("/raw/game/roster/"+std::string(roster.array_names[i]),roster.arrays[i]);
    }else w.gap("/raw/game/roster","Roster not decoded");
    for(std::size_t i=0;i<s.progression.random.size();++i){const auto& r=s.progression.random[i];const auto p=std::string("/raw/rng/")+(i==0?"map":"gameplay");w.put(p+"/seed",r.seed);w.put(p+"/calls",r.calls);w.put(p+"/resets",r.resets);}
    for(const auto& array:s.progression.arrays){const auto p="/raw/progression/"+named(array.name);w.put(p+"/columns",array.columns);w.put(p+"/count",array.rows.size());for(std::size_t i=0;i<array.rows.size();++i){const auto q=p+"/"+std::to_string(i);w.optional(q+"/hash",array.rows[i].type_hash);w.seq(q+"/values",array.rows[i].values);}}
    const auto& m=s.map_header;w.put("/raw/map/width",m.grid.width);w.put("/raw/map/height",m.grid.height);
    w.put("/raw/map/wrap_x",m.wrap_x);w.put("/raw/map/wrap_y",m.wrap_y);w.put("/raw/map/land_plots",m.land_plots);w.put("/raw/map/owned_plots",m.owned_plots);
    if(s.map_plots){
        w.put("/raw/map/plots/count",s.map_plots->plots.size());
        for(const auto& plot:s.map_plots->plots){const auto p="/raw/map/plots/"+std::to_string(plot.index);w.put(p+"/x",plot.x);w.put(p+"/y",plot.y);w.fields(p+"/fields",plot.fields);w.arrays(p+"/arrays",plot.arrays);if(plot.script_data)w.blob(p+"/script_hex",*plot.script_data);else w.gap(p+"/script_hex","Plot script field absent in selected wire version");}
    }else w.gap("/raw/map/plots","Map plots not decoded");
    if(s.teams){for(const auto& team:s.teams->teams){const auto p="/raw/teams/"+std::to_string(team.index);w.fields(p+"/fields",team.fields);w.arrays(p+"/arrays",team.arrays);w.hashes(p+"/hashes",team.hashes);}}
    else w.gap("/raw/teams","Teams not decoded");
    if(s.all_players){
        w.put("/raw/players/count",s.all_players->players.size());
        for(const auto& player:s.all_players->players){
            const auto p="/raw/players/"+std::to_string(player.saved_player_id);core(w,p+"/core",player.core);
            for(const auto& f:player.diplomacy.fields){const auto q=p+"/diplomacy/"+named(f.name);w.put(q+"/rows",f.rows);w.put(q+"/columns",f.columns);w.seq(q+"/values",f.values);}
            for(const auto& b:player.knowledge.blocks){const auto q=p+"/knowledge/"+named(b.name);w.fields(q+"/fields",b.fields);w.arrays(q+"/arrays",b.arrays);w.hashes(q+"/hashes",b.hashes);}
            records(w,p+"/administration",player.administration.blocks);
            // Free-list metadata influences future entity IDs; retain it.
            w.fields(p+"/cities/allocator",player.cities.header.fields);w.seq(p+"/cities/next_free",player.cities.header.next_free_indices);
            for(const auto& c:player.cities.cities){const auto q=p+"/cities/slot/"+std::to_string(c.index);core(w,q+"/core",c.core);records(w,q+"/production",c.production.blocks);nodes(w,q+"/tail",c.tail.blocks);}
            w.fields(p+"/units/allocator",player.units.header.fields);w.seq(p+"/units/next_free",player.units.header.next_free_indices);
            for(const auto& u:player.units.units){const auto q=p+"/units/slot/"+std::to_string(u.index);core(w,q+"/core",u.core);nodes(w,q+"/tail",u.tail.blocks);}
        }
    }else w.gap("/raw/players","Full players not decoded");
    w.put("/raw/trade/next_id",s.trade.next_id);w.put("/raw/trade/routes/count",s.trade.routes.size());
    for(std::size_t i=0;i<s.trade.routes.size();++i)route(w,"/raw/trade/routes/"+std::to_string(i),s.trade.routes[i]);
    if(s.trade.technology_difference)w.seq("/raw/trade/technology_difference",*s.trade.technology_difference);
    else w.gap("/raw/trade/technology_difference","Version-specific absent matrix; native reset semantics not projected");
    const auto& religion=s.religion_culture;w.put("/raw/religion/minimum_faith_next_pantheon",religion.minimum_faith_next_pantheon);
    w.put("/raw/religion/religions/count",religion.religions.size());
    for(std::size_t i=0;i<religion.religions.size();++i){const auto& r=religion.religions[i];const auto p="/raw/religion/religions/"+std::to_string(i);w.put(p+"/id",r.religion);w.put(p+"/founder",r.founder);w.put(p+"/holy_city_x",r.holy_city_x);w.put(p+"/holy_city_y",r.holy_city_y);w.put(p+"/turn_founded",r.turn_founded);w.put(p+"/pantheon",r.pantheon);w.put(p+"/enhanced",r.enhanced);w.optional(p+"/reformed",r.reformed);w.blob(p+"/custom_name_hex",r.custom_name);w.seq(p+"/belief_hashes",r.beliefs.belief_hashes);w.seq(p+"/belief_ids",r.beliefs.belief_ids);w.seq(p+"/belief_lookup",r.beliefs.belief_lookup);w.fields(p+"/belief_effects",r.beliefs.cached_values);}
    const auto& leagues=s.leagues;w.put("/raw/leagues/generated_id_count",leagues.generated_id_count);w.put("/raw/leagues/leagues_ever_founded",leagues.leagues_ever_founded);w.put("/raw/leagues/diplomatic_victor",leagues.diplomatic_victor);w.put("/raw/leagues/last_era_trigger",leagues.last_era_trigger);
    for(const auto& l:leagues.leagues){const auto p="/raw/leagues/"+std::to_string(l.id);w.put(p+"/host",l.host);w.put(p+"/turns_until_session",l.turns_until_session);w.put(p+"/in_session",l.in_session);w.put(p+"/united_nations",l.united_nations);for(const auto& member:l.members){const auto q=p+"/members/"+std::to_string(member.player);w.put(q+"/votes",member.votes);w.put(q+"/extra_votes",member.extra_votes);w.put(q+"/proposals",member.proposals);w.put(q+"/abstained_votes",member.abstained_votes);w.put(q+"/may_propose",member.may_propose);w.blob(q+"/vote_sources_hex",member.vote_sources);}}
}
template<class Map>std::vector<NormalisedBnwChange> changes(const Map& before,const Map& after){
    std::vector<NormalisedBnwChange> out;auto a=before.begin(),b=after.begin();
    while(a!=before.end()||b!=after.end()){
        if(b==after.end()||(a!=before.end()&&a->first<b->first)){out.push_back({a->first,a->second,std::nullopt});++a;}
        else if(a==before.end()||b->first<a->first){out.push_back({b->first,std::nullopt,b->second});++b;}
        else{if(a->second!=b->second)out.push_back({a->first,a->second,b->second});++a;++b;}
    }return out;
}
std::string object(const std::map<std::string,std::string,std::less<>>& values,bool quote_values){
    std::string out="{";bool first=true;
    for(const auto& [path,value]:values){if(!first)out+=',';first=false;out+=json_quote(path)+":"+(quote_values?json_quote(value):value);}
    out+='}';return out;
}
std::string change_list(const std::vector<NormalisedBnwChange>& values,bool strings){
    std::string out="[";bool first=true;
    for(const auto& v:values){if(!first)out+=',';first=false;out+="{\"path\":"+json_quote(v.path)+",\"before\":"+(v.before?(strings?json_quote(*v.before):*v.before):"null")+",\"after\":"+(v.after?(strings?json_quote(*v.after):*v.after):"null")+",\"before_present\":"+scalar(v.before.has_value())+",\"after_present\":"+scalar(v.after.has_value())+"}";}
    out+=']';return out;
}
} // namespace
NormalisedBnwState normalise_bnw_state(const LiveSession& session){
    const auto lease=session.read_state_lease();if(!lease)throw std::invalid_argument("Missing genuine session state");
    const auto generation=session.state_generation(),revision=lease->ui_source_revision;
    if(lease->setup.profile!=content::TechnologyProfile::bnw||lease->world.game_state.layout!=saves::GameStateLayout::expansion2_version1)
        throw std::invalid_argument("Normalised schema1 accepts BNW only");
    Writer w;owned(w,*lease);raw(w,lease->world);
    // Explicit coverage frontier: partial equality MUST NOT masquerade as
    // equivalence of everything affecting the next turn or original BNW.
    w.gap("/coverage/rules_identity","No canonical rule/type fingerprint in this projection; compare only same activated rules");
    w.gap("/coverage/map_semantics","Terrain projection, resource/build/remap, invisible visibility, region state and raw map collection semantics not fully projected");
    w.gap("/coverage/player_subsystems","Strategy/planning/traits/espionage/trade-culture/tail and unsaved AI owners not fully projected");
    w.gap("/coverage/city_subsystems","Management/citizen/strategy and semantic ID remapping not fully projected");
    w.gap("/coverage/owned_contexts","Maintenance/science/research-selection/research-turn and all modifier contexts not fully projected");
    w.gap("/coverage/diplomacy","Deals, scratchpad and diplomacy requests/AI lifecycle not fully projected");
    w.gap("/coverage/religion_culture","Great works/tourism, religion belief/class remapping and live effects not fully projected");
    w.gap("/coverage/league","Resolutions/decisions/projects/held proposals and full league lifecycle not projected");
    w.gap("/coverage/events_scripts","Game supporting/final systems, pending native-operation histories, Lua VM and all event delivery cursors not projected");
    w.gap("/coverage/cache_semantics","Known raw named fields retained conservatively; observational irrelevance of every cache/slot representation is not proved");
    const auto after=session.read_state_lease();
    if(!after||after.get()!=lease.get()||after.owner_before(lease)||lease.owner_before(after)||session.state_generation()!=generation||lease->ui_source_revision!=revision)
        throw std::runtime_error("Live state changed during synchronous normalisation");
    w.state.complete=w.state.unresolved.empty();return std::move(w.state);
}
std::string NormalisedBnwState::json()const{
    return "{\"schema\":\"normalised-bnw-state1\",\"complete\":"+scalar(complete&&unresolved.empty())+",\"values\":"+object(values,false)+",\"unresolved\":"+object(unresolved,true)+"}\n";
}
NormalisedBnwDiff diff_normalised_bnw_state(const NormalisedBnwState& before,const NormalisedBnwState& after){
    NormalisedBnwDiff out;out.values=changes(before.values,after.values);out.unresolved=changes(before.unresolved,after.unresolved);
    out.known_values_equal=out.values.empty();out.complete_comparison=before.complete&&after.complete&&before.unresolved.empty()&&after.unresolved.empty();return out;
}
std::string NormalisedBnwDiff::json()const{
    return "{\"schema\":\"normalised-bnw-diff1\",\"known_values_equal\":"+scalar(known_values_equal)+",\"complete_comparison\":"+scalar(complete_comparison)+",\"original_parity_proven\":false,\"values\":"+change_list(values,false)+",\"unresolved\":"+change_list(unresolved,true)+"}\n";
}
} // namespace civ5::runtime
