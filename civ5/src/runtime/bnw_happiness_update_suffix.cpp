#include "civ5/runtime/bnw_happiness_ui_pump.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;constexpr std::int32_t slot_mask=8191;
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"BNW happiness suffix exceeds native int");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)*b);}
auto &named(auto &rows,std::string_view name){using Row=std::remove_reference_t<decltype(rows.front())>;Row *found=nullptr;for(auto &r:rows)if(r.name&&name==r.name){need(!found,"Duplicate BNW happiness suffix field");found=&r;}need(found,"Missing BNW happiness suffix field");return *found;}
std::int32_t scalar(const auto &rows,std::string_view name){const auto &v=named(rows,name).values;need(v.size()==1,"Malformed BNW happiness suffix scalar");return integer(v[0]);}
std::int64_t &cache(LiveSessionState &s,std::int64_t p,const char *name){auto &f=named(s.world.all_players->players.at(p).core.fields,name);need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1,"BNW happiness suffix native cache shape unknown");integer(f.values[0]);return f.values[0];}
std::int64_t cache_read(const LiveSessionState &s,std::int64_t p,const char *name){need(p>=0&&p<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64,"BNW reached happiness source roster unavailable");const auto &raw=s.world.all_players->players.at(static_cast<std::size_t>(p));need(raw.saved_player_id==p&&raw.core.layout==bnw,"BNW reached happiness player source identity differs");const auto &f=named(raw.core.fields,name);need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1,"BNW happiness suffix native cache shape unknown");integer(f.values[0]);return f.values[0];}
void happiness(LiveSessionState &s,std::int64_t p,std::int32_t operand){auto &n=cache(s,p,"m_iHappiness");need(n==s.unhappiness.players.at(p).happiness,"BNW happiness suffix raw/owned cache stale");n=add(integer(n),operand);s.unhappiness.players.at(p).happiness=n;s.cities.players.at(p).excess_happiness.reset();}
void number(std::vector<std::uint8_t> &b,std::uint64_t n){for(unsigned i=0;i<8;++i)b.push_back(static_cast<std::uint8_t>(n>>(8*i)));}
void text(std::vector<std::uint8_t> &b,std::string_view s){number(b,s.size());b.insert(b.end(),s.begin(),s.end());}
void list(std::vector<std::uint8_t> &b,const auto &v) {number(b,v.size());for(auto n:v)number(b,n);}
void core(std::vector<std::uint8_t> &b,const auto &c) {
    number(b,static_cast<unsigned>(c.layout));number(b,c.start_offset);number(b,c.end_offset);number(b,c.version.has_value());if(c.version)number(b,*c.version);
    number(b,c.fields.size());for(const auto &f:c.fields) {
        text(b,f.name?f.name:"");number(b,static_cast<unsigned>(f.encoding));number(b,f.start_offset);number(b,f.end_offset);
        number(b,f.wire_count.has_value());if(f.wire_count)number(b,*f.wire_count);list(b,f.values);list(b,f.hashes);
        if constexpr(requires{f.row_counts;})list(b,f.row_counts);
        if constexpr(requires{f.row_keys;})list(b,f.row_keys);
        if constexpr(requires{f.bytes;})list(b,f.bytes);
        if constexpr(requires{f.rows;}) {number(b,f.rows.size());for(const auto &r:f.rows){number(b,r.key);number(b,r.wire_count);list(b,r.values);}}
    }
}
void production(std::vector<std::uint8_t> &b,const saves::CityProductionRecord &r) {
    text(b,r.name?r.name:"");number(b,r.index);number(b,r.start_offset);number(b,r.end_offset);number(b,r.fields.size());
    for(const auto &f:r.fields){text(b,f.name?f.name:"");number(b,f.value);}number(b,r.arrays.size());
    for(const auto &a:r.arrays){text(b,a.name?a.name:"");number(b,a.columns);list(b,a.values);}number(b,r.blobs.size());
    for(const auto &a:r.blobs){text(b,a.name?a.name:"");list(b,a.bytes);}number(b,r.children.size());for(const auto &c:r.children)production(b,c);
}
void tail(std::vector<std::uint8_t> &b,const saves::CityTailNode &r) {
    text(b,r.name?r.name:"");text(b,r.encoding?r.encoding:"");number(b,r.start_offset);number(b,r.end_offset);
    number(b,r.wire_count.has_value());if(r.wire_count)number(b,*r.wire_count);list(b,r.values);number(b,r.children.size());for(const auto &c:r.children)tail(b,c);
}
void sparse(std::vector<std::uint8_t> &b,const auto &v) {number(b,v.size());for(const auto &[id,n]:v){number(b,id);number(b,n);}}
std::string after_epoch(const LiveSessionState &s,const SessionCheckpointIdentity &identity,const content::RulesDatabase &db,std::int64_t player){
    std::vector<std::uint8_t> b;text(b,bnw_unit_order_world_revision(s,identity,db));number(b,player);
    const auto &raw=s.world.all_players->players.at(player);number(b,scalar(raw.core.fields,"m_iHappiness"));number(b,scalar(raw.core.fields,"m_iCityConnectionHappiness"));
    need(s.unhappiness.players.at(player).happiness==scalar(raw.core.fields,"m_iHappiness")&&s.cities.players.at(player).excess_happiness,"BNW GameData receiver happiness/excess owner incomplete");
    number(b,s.unhappiness.players.at(player).happiness);number(b,*s.cities.players.at(player).excess_happiness);
    const auto &connections=named(raw.planning.blocks,"city_connections");number(b,connections.fields.size());for(const auto &f:connections.fields){text(b,f.name?f.name:"");number(b,f.value);}number(b,connections.arrays.size());for(const auto &a:connections.arrays){text(b,a.name?a.name:"");number(b,a.columns);list(b,a.values);}
    for(const auto &player:s.world.all_players->players){core(b,player.core);core(b,player.traits);number(b,player.cities.cities.size());for(const auto &c:player.cities.cities){core(b,c.core);for(const auto &r:c.production.blocks)production(b,r);for(const auto &r:c.tail.blocks)tail(b,r);for(const auto &r:c.management.blocks){text(b,r.name?r.name:"");for(const auto &f:r.fields){text(b,f.name?f.name:"");number(b,f.value);}for(const auto &a:r.arrays){text(b,a.name?a.name:"");list(b,a.values);}for(const auto &h:r.hashes){text(b,h.name?h.name:"");number(b,h.wire_count);for(const auto &v:h.rows){number(b,v.type_hash);list(b,v.values);}}}}
        const auto &owned=s.cities.players.at(player.saved_player_id);number(b,owned.capital);number(b,owned.cities.size());for(const auto &[id,c]:owned.cities){number(b,id);number(b,c.owner);number(b,c.population);number(b,c.puppet);sparse(b,c.real_buildings);sparse(b,c.free_buildings);}
        const auto &u=s.unhappiness.players.at(player.saved_player_id);number(b,u.happiness);number(b,u.units);number(b,u.population_modifier);number(b,u.cities_modifier);number(b,u.public_opinion);for(const auto &[id,c]:u.cities){number(b,id);number(b,c.ignore);number(b,c.occupied);number(b,c.no_occupied_unhappiness);sparse(b,c.specialists);}
        if(player.saved_player_id>=22&&player.saved_player_id<63){const auto &ai=named(player.planning.blocks,"minor_civ_ai");number(b,named(ai.fields,"m_eAlly").value);list(b,named(ai.arrays,"m_aiFriendshipWithMajorTimes100").values);}
    }
    need(s.world.teams&&s.world.map_projection,"BNW GameData aggregate world seal incomplete");for(const auto &t:s.world.teams->teams){number(b,t.index);for(const auto &f:t.fields){text(b,f.name?f.name:"");number(b,f.value);}for(const auto &a:t.arrays){text(b,a.name?a.name:"");list(b,a.values);}}
    for(const auto &c:s.world.map_projection->cells){number(b,c.index);number(b,c.owner.has_value());if(c.owner)number(b,*c.owner);number(b,c.feature_type.has_value());if(c.feature_type)number(b,*c.feature_type);number(b,c.feature_type_hash.has_value());if(c.feature_type_hash)number(b,*c.feature_type_hash);number(b,c.is_city);}
    for(const auto &[name,values]:s.world.pregame->archive.string_arrays){text(b,name);number(b,values.size());for(const auto &v:values)text(b,v);}
    for(const auto *rng:{&s.random.map,&s.random.gameplay}){number(b,static_cast<unsigned>(rng->algorithm()));number(b,rng->state().seed);number(b,rng->state().calls);number(b,rng->state().resets);}
    return identify_bytes(b).sha256;
}
BnwUnhappinessQuote current_unhappiness(const LiveSessionState &s,const BnwHappinessSuffixRules &rules,std::int64_t p){
    detail::require_bnw_current_unhappiness_inputs(s,rules.unhappiness(),rules.types(),p);
    const auto q=bnw_unhappiness(s.unhappiness,s.setup,s.cities,rules.unhappiness(),rules.defines(),p);
    auto sum=add(integer(q.normal_cities_times100),integer(q.captured_cities_times100));sum=add(sum,integer(q.normal_population_times100));sum=add(sum,integer(q.occupied_population_times100));sum=add(sum,integer(q.units_times100));
    need(add(sum/100,integer(q.public_opinion))==q.before_ai,"BNW happiness native unhappiness aggregate differs");integer(q.total);integer(q.excess_happiness);return q;
}
}
BnwHappinessSuffixRules BnwHappinessSuffixRules::load(const content::RulesDatabase &db){BnwHappinessSuffixRules r;r.minors_=BnwMinorHappinessRules::load(db);r.wonders_=BnwNaturalWonderHappinessRules::load(db);r.unhappiness_=load_bnw_unhappiness_rules(db);r.defines_=db.cache_global_defines(content::GlobalDefineProfile::bnw);r.types_=db.cache_info_types();need(r.types_.report().all_succeeded()&&r.identity()==r.wonders_.rules_sha256(),"BNW suffix rule epochs differ");return r;}
BnwCityConnectionHappinessQuote quote_bnw_city_connection_happiness(const LiveSessionState &s,std::int64_t p){
    need(p>=0&&p<64&&s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64,"BNW happiness connection player registry incomplete");
    const auto &raw=s.world.all_players->players.at(p);const auto &cities=s.cities.players.at(p);need(raw.saved_player_id==p&&raw.core.layout==bnw&&scalar(raw.core.fields,"m_eID")==p,"BNW happiness connection player identity differs");BnwCityConnectionHappinessQuote q;q.rate=scalar(raw.core.fields,"m_iHappinessPerTradeRouteCount");if(q.rate==0)return q;
    need(raw.cities.layout==bnw&&raw.cities.header.layout==bnw&&raw.cities.header.wire_city_count==static_cast<std::int64_t>(raw.cities.cities.size())&&raw.cities.cities.size()==cities.cities.size(),"BNW happiness connection city roster incomplete");
    const auto capital=scalar(raw.core.fields,"m_iCapitalCityID");need(capital==cities.capital,"BNW happiness current capital cache differs");if(capital==-1)return q;
    const auto last=integer(named(raw.cities.header.fields,"last_index").value);std::map<std::int32_t,std::int64_t> slots;std::map<std::int64_t,std::uint64_t> plots;
    for(const auto &saved:raw.cities.cities){const auto id=scalar(saved.core.fields,"m_iID");const auto &city=cities.cities.at(id);const auto slot=id&slot_mask;
        need(id>=0&&slot<=last&&static_cast<std::uint64_t>(slot)<raw.cities.header.source_slots&&slots.emplace(slot,id).second&&saved.core.layout==bnw&&scalar(saved.core.fields,"m_eOwner")==p&&city.owner==p&&city.id==id&&
             city.source_core==std::optional{std::pair{saved.core.start_offset,saved.core.end_offset}}&&scalar(saved.core.fields,"m_iX")==city.x&&scalar(saved.core.fields,"m_iY")==city.y,"BNW happiness native city slot/binding differs");
        need(s.world.map_projection&&s.world.map_plots&&city.x>=0&&city.y>=0&&static_cast<std::uint64_t>(city.x)<s.world.map_header.grid.width&&static_cast<std::uint64_t>(city.y)<s.world.map_header.grid.height,"BNW happiness city plot unknown");const auto index=s.world.map_header.grid.index(city.x,city.y);const auto &cell=s.world.map_projection->cells.at(index);const auto &plot=s.world.map_plots->plots.at(index);
        need(cell.index==index&&cell.is_city&&cell.plot_city&&cell.plot_city->owner==p&&cell.plot_city->id==id&&plot.index==index&&named(plot.fields,"plot_city_owner").value==p&&named(plot.fields,"plot_city_id").value==id,"BNW happiness city/plot owner differs");plots.emplace(id,index);
    }
    const auto found=slots.find(capital&slot_mask);if(found==slots.end()||((capital&~slot_mask)!=0&&found->second!=capital))return q;q.capital=found->second;if(cities.cities.size()<=1)return q;
    need(raw.planning.layout==bnw,"BNW happiness connection cache layout unknown");const auto &connections=named(raw.planning.blocks,"city_connections");need(named(connections.fields,"version").value==1,"BNW happiness connection cache version unknown");
    const auto dimension=named(connections.fields,"route_infos_dimension").value;const auto &routes=named(connections.arrays,"route_infos"),&ids=named(connections.arrays,"city_plot_ids");const auto count=named(connections.fields,"city_ids_wire_count").value;
    need(dimension>=0&&dimension<=UINT32_MAX&&routes.columns==2&&routes.values.size()%2==0&&(dimension==0?routes.values.empty():(routes.values.size()/2)%dimension==0&&(routes.values.size()/2)/dimension==static_cast<std::uint64_t>(dimension))&&ids.columns==1&&count>=0&&static_cast<std::uint64_t>(count)==ids.values.size(),"BNW happiness connection cache shape differs");
    for(auto value:routes.values)need(value>=INT8_MIN&&value<=INT8_MAX,"BNW route info exceeds source char width");
    const auto connected=[&](std::uint64_t first,std::uint64_t second){std::int64_t a=-1,b=-1;for(std::size_t i=0;i<ids.values.size();++i){if(ids.values[i]==static_cast<std::int64_t>(first))a=i;if(ids.values[i]==static_cast<std::int64_t>(second))b=i;if(a>=0&&b>=0)break;}if(a<0||b<0||a>=dimension||b>=dimension)return false;return (routes.values.at(static_cast<std::size_t>((a*dimension+b)*2+1))&1)!=0;};
    for(const auto &saved:raw.cities.cities){const auto id=scalar(saved.core.fields,"m_iID");if(id==*q.capital)continue;if(connected(plots.at(*q.capital),plots.at(id))){q.connected_count=add(q.connected_count,1);q.connected_cities.push_back(id);}}
    q.total=mul(q.rate,q.connected_count)/100;return q;
}
BnwHappinessGameDataOwner start_bnw_happiness_game_data_client(const LiveSessionState &s,const BnwHappinessPrefixRules &rules,const SessionCheckpointIdentity &identity,std::int64_t p){const auto input=attest_bnw_happiness_prefix_input(s,rules,identity,p);return BnwHappinessGameDataOwner(identity,s.world.map_header.guid,input.epoch(),p,scalar(s.world.all_players->players.at(p).core.fields,"m_iHappiness"));}
void apply_bnw_happiness_game_data_batch(const LiveSessionState &s,BnwHappinessGameDataOwner &owner,const content::RulesDatabase &db,const BnwHappinessGameDataBatch &b){
    need(owner.identity_.payload_sha256==b.identity_.payload_sha256&&owner.identity_.rules_sha256==b.identity_.rules_sha256&&owner.guid_==b.guid_&&b.guid_==s.world.map_header.guid&&b.before_epoch_==owner.input_epoch_&&b.expected_revision_==owner.revision_&&owner.revision_<UINT64_MAX&&b.receipt_.player==owner.player_&&b.after_epoch_==after_epoch(s,b.identity_,db,owner.player_),"BNW GameData receipt missing/stale/source epoch differs");
    const auto &raw=s.world.all_players->players.at(owner.player_);need(b.receipt_.happiness==scalar(raw.core.fields,"m_iHappiness")&&b.receipt_.connections==scalar(raw.core.fields,"m_iCityConnectionHappiness")&&b.receipt_.excess==*s.cities.players.at(owner.player_).excess_happiness&&b.receipt_.achievement_xp2_45_requested==(s.world.pregame->archive.integers.at("active_player")==owner.player_&&b.receipt_.excess>=100),"BNW GameData actual view/cache payload differs");
    auto d=owner;d.dirty_=true;d.happiness_=b.receipt_.happiness;d.receipts_.push_back(b.receipt_);d.input_epoch_=b.after_epoch_;d.pending_fanout_=true;++d.revision_;owner=std::move(d);
}
namespace detail {
BnwHappinessPostReligionOperand bnw_happiness_post_religion_reached_read(
    const LiveSessionState &s,const BnwHappinessSuffixRules &rules,
    const SessionCheckpointIdentity &identity,std::int64_t p,BnwHappinessPostReligionRead stage){
    need(p>=0&&p<64,"BNW reached happiness player outside native domain");
    need(rules.identity()==identity.rules_sha256,"BNW reached happiness rules identity differs");
    BnwHappinessPostReligionOperand out;
    switch(stage){
    case BnwHappinessPostReligionRead::wonders:
        out.operand=quote_bnw_happiness_from_natural_wonders(s,rules.wonders(),identity,p).total;break;
    case BnwHappinessPostReligionRead::minors:
        out.operand=quote_bnw_happiness_from_minors(s,rules.minors(),identity,p).total;break;
    case BnwHappinessPostReligionRead::leagues:
        out.operand=integer(cache_read(s,p,"m_iHappinessFromLeagues"));break;
    case BnwHappinessPostReligionRead::connections_cache:
        out.connections=quote_bnw_city_connection_happiness(s,p);out.operand=out.connections->total;break;
    case BnwHappinessPostReligionRead::connections_add:
        out.operand=integer(cache_read(s,p,"m_iCityConnectionHappiness"));break;
    case BnwHappinessPostReligionRead::achievement_condition:{
        need(s.world.pregame&&s.world.pregame->layout==bnw,"BNW reached happiness active player source absent");
        const auto active=integer(s.world.pregame->archive.integers.at("active_player"));
        need(active>=-1&&active<64,"BNW reached happiness active player outside native domain");
        // CvPlayer::isLocalPlayer FIRST. No GetExcessHappiness/Unhappiness
        // operands, caches, cities or mirrored population on a false branch.
        if(active!=p)break;
        out.unhappiness=current_unhappiness(s,rules,p);
        out.operand=integer(out.unhappiness->excess_happiness);
        out.achievement_requested=out.operand>=100;break;
    }
    default:throw std::runtime_error("BNW reached happiness stage unavailable");
    }
    return out;
}
}
struct BnwHappinessSuffixAccess {
    static BnwHappinessSuffixExecution execute(const BnwHappinessPrefixDraft &prefix,const BnwHappinessPrefixRules &prefix_rules,const SavedContentProfile &selected,BnwGameplayLuaEpochOwner &lua,const BnwReligionHappinessRules &religion_rules,const BnwHappinessSuffixRules &rules,const BnwHappinessGameDataOwner &client,const content::RulesDatabase &db){
        BnwHappinessSuffixExecution out;try{
            const auto p=prefix.player();const auto &identity=prefix.identity();need(rules.identity()==identity.rules_sha256&&identify_bytes(db.gameplay_fingerprint_data()).sha256==identity.rules_sha256&&client.identity_.payload_sha256==identity.payload_sha256&&client.identity_.rules_sha256==identity.rules_sha256&&client.guid_==prefix.state().world.map_header.guid&&client.player_==p&&client.input_epoch_==prefix.input_epoch()&&client.happiness_==prefix.initial_happiness()&&!client.pending_fanout_,"BNW happiness suffix input/client epoch stale");
            out.continuation=continue_bnw_happiness_prefix_religion(prefix,prefix_rules,selected,lua,religion_rules);if(!out.continuation.store||!out.continuation.religion.query.epoch){out.frontier=out.continuation.frontier;return out;}
            auto &world=*out.continuation.religion.query.epoch->world;need(out.continuation.religion.query.epoch->identity.payload_sha256==identity.payload_sha256,"BNW suffix Lua epoch source identity differs");
            const auto apply=[&](BnwHappinessSuffixStage stage,std::int32_t operand){happiness(world,p,operand);out.stores.push_back({stage,operand,integer(cache(world,p,"m_iHappiness"))});};
            using Read=detail::BnwHappinessPostReligionRead;
            const auto read=[&](Read stage){return detail::bnw_happiness_post_religion_reached_read(world,rules,identity,p,stage);};
            apply(BnwHappinessSuffixStage::wonders,read(Read::wonders).operand);
            apply(BnwHappinessSuffixStage::minors,read(Read::minors).operand);
            apply(BnwHappinessSuffixStage::leagues,read(Read::leagues).operand);
            out.connections=read(Read::connections_cache).connections;cache(world,p,"m_iCityConnectionHappiness")=out.connections->total;
            out.stores.push_back({BnwHappinessSuffixStage::connections_cache,out.connections->total,integer(cache(world,p,"m_iHappiness"))});
            apply(BnwHappinessSuffixStage::connections_add,read(Read::connections_add).operand);
            out.unhappiness=current_unhappiness(world,rules,p);world.cities.players.at(p).excess_happiness=integer(out.unhappiness->excess_happiness);
            out.stores.push_back({BnwHappinessSuffixStage::excess,integer(out.unhappiness->excess_happiness),integer(cache(world,p,"m_iHappiness"))});
            const bool achievement=world.world.pregame->archive.integers.at("active_player")==p&&*world.cities.players.at(p).excess_happiness>=100;
            if(achievement)out.stores.push_back({BnwHappinessSuffixStage::achievement_request,1,integer(cache(world,p,"m_iHappiness"))});
            BnwHappinessGameDataBatch batch;batch.identity_=identity;batch.guid_=world.world.map_header.guid;batch.before_epoch_=prefix.input_epoch();batch.after_epoch_=after_epoch(world,identity,db,p);batch.expected_revision_=client.revision_;
            batch.receipt_={p,integer(cache(world,p,"m_iHappiness")),integer(cache(world,p,"m_iCityConnectionHappiness")),integer(*world.cities.players.at(p).excess_happiness),achievement};
            auto receiver=client;apply_bnw_happiness_game_data_batch(world,receiver,db,batch);out.batch=std::move(batch);out.receiver=std::move(receiver);
            out.stores.push_back({BnwHappinessSuffixStage::game_data_receiver,1,integer(cache(world,p,"m_iHappiness"))});out.native_cache_suffix_complete=true;
            out.frontier="Actual GameData dirty receiver consumed on SAME private epoch; mandatory native/UI/Lua fanout and atomic publication remain";
        }catch(const std::exception &e){out={};out.frontier=e.what();}return out;
    }
};
BnwHappinessSuffixExecution execute_bnw_happiness_remaining_suffix(const BnwHappinessPrefixDraft &p,const BnwHappinessPrefixRules &pr,const SavedContentProfile &profile,BnwGameplayLuaEpochOwner &lua,const BnwReligionHappinessRules &religion,const BnwHappinessSuffixRules &rules,const BnwHappinessGameDataOwner &client,const content::RulesDatabase &db){return BnwHappinessSuffixAccess::execute(p,pr,profile,lua,religion,rules,client,db);}
}

namespace civ5::runtime {
struct BnwHappinessUiPumpAccess {
    static BnwHappinessUiPumpExecution consume(BnwHappinessSuffixExecution &&input,const content::RulesDatabase &db) {
        BnwHappinessUiPumpExecution out;
        auto staged=std::make_unique<BnwHappinessSuffixExecution>(std::move(input));
        bool retainable=false;
        try {
            need(staged->native_cache_suffix_complete&&!staged->mandatory_game_data_fanout_complete&&staged->batch&&staged->receiver,"BNW GameData pump has no pending native suffix receipt");
            retainable=true;
            auto &epoch=staged->continuation.religion.query.epoch;
            need(epoch&&epoch->world&&epoch->host&&epoch->getter_world==epoch->world.get()&&epoch->source_world&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"BNW GameData pump requires actual selected loaded UI registry/getters/escrow on SAME epoch");
            auto &client=*staged->receiver;const auto &batch=*staged->batch;const auto &world=*epoch->world;
            need(client.pending_fanout_&&client.dirty_&&client.revision_==batch.expected_revision_+1&&client.input_epoch_==batch.after_epoch_&&client.receipts_.size()>0&&client.receipts_.back().player==batch.receipt_.player&&client.happiness_==batch.receipt_.happiness&&epoch->identity.payload_sha256==batch.identity_.payload_sha256&&epoch->identity.rules_sha256==batch.identity_.rules_sha256&&after_epoch(world,batch.identity_,db,client.player_)==batch.after_epoch_,"BNW GameData pump receiver/current epoch stale");
            // Civis emits after the native prefix has returned its actual dirty
            // receipt. The selected stock UI source subscribes to this event;
            // SDK setDirty does not attest when the proprietary engine emits it.
            out.dispatch_attempted=true;
            const auto delivered=epoch->host->emit_ui_event("SerialEventGameDataDirty");
            need(delivered.errors.empty(),delivered.errors.empty()?"BNW GameData event failed":delivered.errors.front().c_str());
            need(delivered.listeners>0,"BNW selected GameData UI consumers not registered; empty registry cannot attest delivery");
            need(epoch->host->loading_complete()&&after_epoch(world,batch.identity_,db,client.player_)==batch.after_epoch_,"BNW GameData callback changed sealed happiness/unit/map operands or loading; reentrant dirty owner required");
            client.dirty_=false;client.pending_fanout_=false;
            staged->mandatory_game_data_fanout_complete=true;
            staged->frontier="Standalone deferred GameData UI event delivered on SAME private epoch; outer atomic world/VM/presentation publication remains";
            out.listeners=delivered.listeners;out.frontier=staged->frontier;out.staged=std::move(staged);
        }catch(const std::exception &e){out.frontier=e.what();if(retainable&&!out.dispatch_attempted)out.pending=std::move(staged);}
        return out;
    }
};
BnwHappinessUiPumpExecution pump_bnw_happiness_game_data(BnwHappinessSuffixExecution &&input,const content::RulesDatabase &db){return BnwHappinessUiPumpAccess::consume(std::move(input),db);}
}
