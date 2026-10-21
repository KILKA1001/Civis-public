#include "civ5/runtime/bnw_happiness_update_prefix.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t n) {need(n>=INT32_MIN && n<=INT32_MAX,"BNW happiness cache exceeds native int");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t multiply(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)*b);}
auto &named(auto &rows,std::string_view name) {
    using Row=std::remove_reference_t<decltype(rows.front())>;Row *found=nullptr;
    for(auto &row:rows)if(row.name && name==row.name) {need(!found,"Duplicate BNW happiness prefix field");found=&row;}
    need(found,"Missing BNW happiness prefix field");return *found;
}
std::int32_t scalar(const auto &rows,std::string_view name) {const auto &v=named(rows,name).values;need(v.size()==1,"Malformed BNW happiness prefix scalar");return integer(v[0]);}
void number(std::vector<std::uint8_t> &b,std::uint64_t n) {for(unsigned i=0;i<8;++i)b.push_back(static_cast<std::uint8_t>(n>>(8*i)));}
void text(std::vector<std::uint8_t> &b,std::string_view s) {number(b,s.size());b.insert(b.end(),s.begin(),s.end());}
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
std::string epoch(const LiveSessionState &s,const BnwHappinessPrefixRules &rules,const SessionCheckpointIdentity &identity,std::int64_t player) {
    need(identity.payload_sha256.size()==64 && identity.rules_sha256==rules.rules_sha256(),"BNW happiness prefix rules identity differs");
    need(s.setup.profile==content::TechnologyProfile::bnw && s.world.game_state.layout==bnw && s.world.all_players && s.world.all_players->layout==bnw &&
         s.world.all_players->players.size()==64 && s.setup.players.size()==64 && s.unhappiness.players.size()==64 && s.cities.players.size()==64 &&
         s.world.pregame && s.world.pregame->layout==bnw && player>=0 && player<64 && s.setup.game_turn>=0 && s.setup.game_turn<=INT32_MAX &&
         s.world.pregame->archive.integers.at("game_turn")==s.setup.game_turn,"BNW happiness prefix current world/clock incomplete");
    const auto &raw=s.world.all_players->players.at(static_cast<std::size_t>(player));const auto &base=s.setup.players.at(player);const auto &cache=s.unhappiness.players.at(player);
    need(raw.saved_player_id==player && scalar(raw.core.fields,"m_eID")==player && cache.happiness==scalar(raw.core.fields,"m_iHappiness"),"BNW happiness prefix current cache owner stale");
    const auto handicap=s.world.pregame->archive.integer_arrays.at("handicaps").at(static_cast<std::size_t>(player));
    need(handicap==base.handicap,"BNW happiness prefix handicap binding differs");
    const auto resources=quote_bnw_happiness_from_resources(s,rules.resources(),identity,player);
    const auto buildings=quote_bnw_happiness_from_buildings(s,rules.buildings(),identity,player);
    const auto policies=quote_bnw_happiness_from_policies(s,rules.buildings().policies(),identity,player);
    std::vector<std::uint8_t> b;text(b,identity.payload_sha256);text(b,identity.rules_sha256);b.insert(b.end(),s.world.map_header.guid.begin(),s.world.map_header.guid.end());
    number(b,s.setup.game_turn);number(b,player);number(b,base.handicap);number(b,base.team);number(b,base.alive);number(b,base.policies_resolved);
    number(b,base.policies.size());for(auto p:base.policies)number(b,p);number(b,rules.default_happiness(handicap));core(b,raw.core);core(b,raw.traits);
    const auto &policy_block=named(raw.strategy.blocks,"policies");number(b,policy_block.fields.size());for(const auto &f:policy_block.fields){text(b,f.name?f.name:"");number(b,f.value);}
    number(b,policy_block.hashes.size());for(const auto &h:policy_block.hashes){text(b,h.name?h.name:"");number(b,h.wire_count);number(b,h.rows.size());for(const auto &r:h.rows){number(b,r.type_hash);list(b,r.values);}}
    number(b,resources.total);number(b,resources.luxuries.size());for(const auto &r:resources.luxuries){number(b,r.resource);number(b,r.available);number(b,r.banned);number(b,r.retained_export);number(b,r.from_minors);number(b,r.total);}
    number(b,buildings.local);number(b,buildings.global);number(b,policies.total);number(b,raw.cities.header.wire_city_count);number(b,raw.cities.cities.size());
    for(const auto &c:raw.cities.cities){core(b,c.core);number(b,c.production.blocks.size());for(const auto &r:c.production.blocks)production(b,r);number(b,c.tail.blocks.size());for(const auto &r:c.tail.blocks)tail(b,r);}
    const auto &cities=s.cities.players.at(player);number(b,cities.civilization);number(b,cities.capital);number(b,cities.excess_happiness.has_value());if(cities.excess_happiness)number(b,*cities.excess_happiness);
    number(b,cities.cities.size());for(const auto &[id,c]:cities.cities){number(b,id);number(b,c.owner);number(b,c.population);number(b,c.puppet);number(b,c.building_counts_resolved);sparse(b,c.real_buildings);sparse(b,c.free_buildings);}
    number(b,cache.happiness);return identify_bytes(b).sha256;
}
void store(LiveSessionState &s,std::int64_t player,std::int32_t n) {
    auto &raw=s.world.all_players->players.at(static_cast<std::size_t>(player));auto &field=named(raw.core.fields,"m_iHappiness");
    need(field.encoding==saves::PlayerCoreEncoding::i32 && field.values.size()==1 && s.unhappiness.players.at(player).happiness==field.values[0],"BNW happiness store cache owner differs");
    field.values[0]=n;s.unhappiness.players.at(player).happiness=n;
    // The source stores the positive cache now; complete GetExcessHappiness is
    // a later read. Invalidate this separate derived projection immediately.
    s.cities.players.at(player).excess_happiness.reset();
}
}
BnwHappinessPrefixRules BnwHappinessPrefixRules::load(const content::RulesDatabase &db) {
    BnwHappinessPrefixRules r;r.buildings_=BnwBuildingHappinessRules::load(db);r.resources_=BnwResourceHappinessRules::load(db);
    need(r.buildings_.rules_sha256()==r.resources_.rules_sha256(),"BNW happiness prefix rule snapshots differ");
    const auto report=db.prefetch_info("HandicapInfos",[&](auto slot,auto,const auto &row){need(slot<=INT32_MAX,"BNW handicap slot exceeds native int");r.handicaps_.emplace(static_cast<std::int64_t>(slot),integer(row.integer("HappinessDefault")));return true;});
    need(report.all_succeeded(),"BNW happiness default handicap Info unavailable");return r;
}
std::int32_t BnwHappinessPrefixRules::default_happiness(std::int64_t id) const {need(handicaps_.contains(id),"BNW happiness handicap has no native Info");return handicaps_.at(id);}
static BnwHappinessNativeStore stage_math(const LiveSessionState &s,
    const BnwHappinessPrefixRules &rules,const SessionCheckpointIdentity &id,std::int64_t p,
    BnwHappinessNativeStore::Stage stage,bool reached) {
    need(stage>=BnwHappinessNativeStore::Stage::handicap&&stage<=BnwHappinessNativeStore::Stage::per_city,
         "BNW happiness native prefix stage unavailable");
    need(p>=0&&p<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&
         s.setup.profile==content::TechnologyProfile::bnw&&id.rules_sha256==rules.rules_sha256(),
         "BNW happiness native stage source/rules owner unavailable");
    const auto &raw=s.world.all_players->players.at(static_cast<std::size_t>(p));
    need(raw.saved_player_id==p&&scalar(raw.core.fields,"m_eID")==p&&
         scalar(raw.core.fields,"m_iHappiness")==s.unhappiness.players.at(p).happiness,
         "BNW happiness native stage raw/owned echo stale");
    std::int32_t operand=0;
    switch(stage) {
      case BnwHappinessNativeStore::Stage::handicap: {
        need(s.world.pregame&&s.world.pregame->layout==bnw,"BNW happiness native handicap source absent");
        const auto &slots=s.world.pregame->archive.integer_arrays.at("handicaps");
        need(slots.size()==64&&slots.at(static_cast<std::size_t>(p))==s.setup.players.at(p).handicap,
             "BNW happiness native handicap mirror stale");
        operand=rules.default_happiness(s.setup.players.at(p).handicap);break;
      }
      case BnwHappinessNativeStore::Stage::resources:operand=quote_bnw_happiness_from_resources(s,rules.resources(),id,p).total;break;
      case BnwHappinessNativeStore::Stage::cities:operand=reached?
        detail::bnw_happiness_buildings_reached_math(s,rules.buildings(),id,p,detail::BnwBuildingHappinessReachedStage::local_cities):
        quote_bnw_happiness_from_buildings(s,rules.buildings(),id,p).local;break;
      case BnwHappinessNativeStore::Stage::buildings:operand=reached?
        detail::bnw_happiness_buildings_reached_math(s,rules.buildings(),id,p,detail::BnwBuildingHappinessReachedStage::global_buildings):
        quote_bnw_happiness_from_buildings(s,rules.buildings(),id,p).global;break;
      case BnwHappinessNativeStore::Stage::policies:operand=quote_bnw_happiness_from_policies(s,rules.buildings().policies(),id,p).total;break;
      case BnwHappinessNativeStore::Stage::per_city:
        need(raw.cities.header.wire_city_count>=0&&static_cast<std::uint64_t>(raw.cities.header.wire_city_count)==raw.cities.cities.size()&&
             raw.cities.cities.size()==s.cities.players.at(p).cities.size()&&raw.cities.cities.size()<=INT32_MAX,
             "BNW happiness actual city count mirror stale");
        operand=multiply(integer(static_cast<std::int64_t>(s.cities.players.at(p).cities.size())),scalar(raw.core.fields,"m_iHappinessPerCity"));break;
      default:throw std::runtime_error("BNW happiness native prefix stage unavailable");
    }
    return {stage,operand,stage==BnwHappinessNativeStore::Stage::handicap?operand:add(scalar(raw.core.fields,"m_iHappiness"),operand)};
}
BnwHappinessNativeStore detail::bnw_happiness_prefix_stage_math(const LiveSessionState& s,
 const BnwHappinessPrefixRules& r,const SessionCheckpointIdentity& id,std::int64_t p,
 BnwHappinessNativeStore::Stage stage){return stage_math(s,r,id,p,stage,false);}
BnwHappinessNativeStore detail::bnw_happiness_prefix_reached_stage_math(const LiveSessionState& s,
 const BnwHappinessPrefixRules& r,const SessionCheckpointIdentity& id,std::int64_t p,
 BnwHappinessNativeStore::Stage stage){return stage_math(s,r,id,p,stage,true);}
struct BnwHappinessPrefixAccess {
    static BnwHappinessPrefixInput attest(const LiveSessionState &s,const BnwHappinessPrefixRules &rules,const SessionCheckpointIdentity &identity,std::int64_t p) {
        return BnwHappinessPrefixInput(identity,p,epoch(s,rules,identity,p));
    }
    static BnwHappinessPrefixExecution execute(const LiveSessionState &s,const BnwHappinessPrefixInput &input,const BnwHappinessPrefixRules &rules) {
        BnwHappinessPrefixExecution out;
        try {
            need(input.epoch_==epoch(s,rules,input.identity_,input.player_),"BNW happiness prefix input/cache epoch stale");
            auto d=std::unique_ptr<BnwHappinessPrefixDraft>(new BnwHappinessPrefixDraft(s,input.identity_,input.player_));const auto p=input.player_;d->input_epoch_=input.epoch_;d->initial_happiness_=scalar(s.world.all_players->players.at(p).core.fields,"m_iHappiness");
            for(const auto stage:{BnwHappinessNativeStore::Stage::handicap,BnwHappinessNativeStore::Stage::resources,
                BnwHappinessNativeStore::Stage::cities,BnwHappinessNativeStore::Stage::buildings,
                BnwHappinessNativeStore::Stage::policies,BnwHappinessNativeStore::Stage::per_city}) {
                const auto item=detail::bnw_happiness_prefix_stage_math(d->state_,rules,input.identity_,p,stage);
                store(d->state_,p,item.happiness);d->stores_.push_back(item);
            }
            d->epoch_=epoch(d->state_,rules,input.identity_,p);out.prefix=std::move(d);
            out.frontier="Mandatory GetHappinessFromReligion founder Lua epoch; native prefix remains private";
        }catch(const std::exception &e){out.frontier=e.what();}
        return out;
    }
    static BnwHappinessReligionContinuation religion(const BnwHappinessPrefixDraft &d,const BnwHappinessPrefixRules &rules,const SavedContentProfile &profile,
        BnwGameplayLuaEpochOwner &owner,const BnwReligionHappinessRules &religion_rules) {
        BnwHappinessReligionContinuation out;
        try {
            need(d.stores_.size()==6 && d.epoch_==epoch(d.state_,rules,d.identity_,d.player_) && religion_rules.rules_sha256()==rules.rules_sha256(),"BNW happiness religion continuation prefix/rules stale");
            out.religion=quote_bnw_happiness_from_religion_terms(d.state_,profile,d.identity_,owner,religion_rules,d.player_);
            auto &q=out.religion.query;
            if(!q.happiness_quoted || !q.happiness || !q.epoch || !q.epoch->world) {out.frontier=q.blocker;return out;}
            auto &world=*q.epoch->world;const auto &raw=world.world.all_players->players.at(d.player_);
            need(raw.saved_player_id==d.player_ && scalar(raw.core.fields,"m_eID")==d.player_,"BNW happiness callback replaced actor identity");
            const auto n=add(scalar(raw.core.fields,"m_iHappiness"),*q.happiness);store(world,d.player_,n);
            out.store=BnwHappinessNativeStore{BnwHappinessNativeStore::Stage::religion,*q.happiness,n};
            out.frontier="Religion cached in SAME detached Lua epoch; natural wonders/minors/leagues/connections/UI suffix remains mandatory";
        }catch(const std::exception &e){out.religion={};out.store.reset();out.frontier=e.what();}
        return out;
    }
};
BnwHappinessPrefixInput attest_bnw_happiness_prefix_input(const LiveSessionState &s,const BnwHappinessPrefixRules &r,const SessionCheckpointIdentity &i,std::int64_t p) {return BnwHappinessPrefixAccess::attest(s,r,i,p);}
BnwHappinessPrefixExecution execute_bnw_happiness_native_prefix(const LiveSessionState &s,const BnwHappinessPrefixInput &i,const BnwHappinessPrefixRules &r) {return BnwHappinessPrefixAccess::execute(s,i,r);}
BnwHappinessReligionContinuation continue_bnw_happiness_prefix_religion(const BnwHappinessPrefixDraft &d,const BnwHappinessPrefixRules &r,const SavedContentProfile &p,BnwGameplayLuaEpochOwner &o,const BnwReligionHappinessRules &b) {return BnwHappinessPrefixAccess::religion(d,r,p,o,b);}
}
