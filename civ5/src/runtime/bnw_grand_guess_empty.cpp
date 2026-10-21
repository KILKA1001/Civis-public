#include "civ5/runtime/bnw_grand_guess_empty.hpp"
#include "civ5/runtime/bnw_player_do_turn_revolts.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
std::int64_t i32(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Grand guess native int width invalid");return v;}
auto &named(const auto &rows,std::string_view name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R *p=nullptr;for(const auto &r:rows)if(r.name&&name==r.name){need(!p,"Grand guess duplicate native field");p=&r;}if(!p)throw std::runtime_error("Grand guess required native field missing: "+std::string(name));return *p;}
std::int64_t scalar(const saves::SavedPlayerObservation &p,std::string_view name){const auto &f=named(p.core.fields,name);need(f.values.size()==1&&f.encoding==(name=="m_bAlive"?saves::PlayerCoreEncoding::boolean:saves::PlayerCoreEncoding::i32),"Grand guess native scalar shape/encoding unknown");return i32(f.values[0]);}
struct Digest {
 std::vector<std::uint8_t> bytes;
 void n(std::int64_t v){const auto u=static_cast<std::uint64_t>(v);for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(u>>(8*i)));}
 void t(std::string_view v){n(v.size());bytes.insert(bytes.end(),v.begin(),v.end());}
 void opt(const auto &v){n(v.has_value());if(v)n(*v);}
 void values(const auto &v){n(v.size());for(auto x:v)n(x);}
 void fields(const auto &v){n(v.size());for(const auto &x:v){t(x.name?x.name:"");n(x.value);}}
 void arrays(const auto &v){n(v.size());for(const auto &x:v){t(x.name?x.name:"");n(x.columns);values(x.values);}}
 void hashes(const auto &v){n(v.size());for(const auto &x:v){t(x.name?x.name:"");n(x.wire_count);n(x.rows.size());for(const auto &r:x.rows){n(r.type_hash);values(r.values);}}}
 void production(const saves::CityProductionRecord &r){t(r.name?r.name:"");n(r.index);n(r.start_offset);n(r.end_offset);fields(r.fields);arrays(r.arrays);n(r.blobs.size());for(const auto &b:r.blobs){t(b.name?b.name:"");values(b.bytes);}n(r.children.size());for(const auto &c:r.children)production(c);}
 void tail(const saves::CityTailNode &r){t(r.name?r.name:"");t(r.encoding?r.encoding:"");n(r.start_offset);n(r.end_offset);opt(r.wire_count);values(r.values);n(r.children.size());for(const auto &c:r.children)tail(c);}
};
const auto &grand(const LiveSessionState &s,std::int64_t player){need(s.world.all_players&&player>=0&&player<22&&s.world.all_players->players.size()==64,"Grand guess current native player owner unavailable");return named(s.world.all_players->players.at(player).strategy.blocks,"grand_strategy_ai");}
void validate_grand(const LiveSessionState &s,const BnwGrandWorldAverageRules &rules,std::int64_t player){
 const auto &g=grand(s,player);const auto &p=s.world.all_players->players.at(player);
 need(p.strategy.layout==bnw&&p.strategy.dimensions.grand==4&&p.strategy.dimensions.major==22&&p.strategy.dimensions.civ_players==63&&p.strategy.dimensions.profile_extensions.values.empty()&&p.strategy.start_offset<=g.start_offset&&g.start_offset<g.end_offset&&g.end_offset<=p.strategy.end_offset,"Grand guess current BNW Grand Read/schema owner unknown");
 need(g.fields.size()==3&&g.arrays.size()==3&&g.hashes.empty()&&named(g.fields,"version").value==1,"Grand guess actual Grand Read version/schema unknown");
 std::set<std::int64_t> entries;for(const auto &[name,type]:rules.types().entries())if(type.table=="AIGrandStrategies"){(void)name;need(type.id>=0&&type.id<4&&entries.insert(type.id).second,"Grand guess actual native Grand Info slots unknown");}
 need(entries.size()==4,"Grand guess current rules/native Grand Read slot binding unknown");
 const auto active=i32(named(g.fields,"active_strategy").value);need(active==-1||entries.contains(active),"Grand guess current active strategy native ID invalid");need(i32(named(g.fields,"turns_since_active_set").value)>=0,"Grand guess current native strategy age invalid");
 for(const auto name:{"priorities","other_player_strategy_guesses","guess_confidence"}){const auto &a=named(g.arrays,name);need(a.columns==1&&a.values.size()==(std::string_view(name)=="priorities"?4:22),"Grand guess current native Grand array cardinality/column unknown");for(auto v:a.values){i32(v);if(std::string_view(name)=="other_player_strategy_guesses")need(v==-1||entries.contains(v),"Grand guess retained native guess ID invalid");if(std::string_view(name)=="guess_confidence")need(v>=-1&&v<=2,"Grand guess retained native confidence enum invalid");}}
}
std::string first_strategy_type(const BnwGrandWorldAverageRules &rules){
 std::optional<std::string_view> first;for(const auto &[key,type]:rules.types().entries()){(void)key;if(type.table=="AIGrandStrategies"&&type.id==0){need(!first,"Grand guess duplicate first native Info type");first=type.type;}}
 need(first&&!first->empty(),"Grand guess actual first native Info type unavailable");return std::string(*first);
}
struct Inputs {std::string trade,military,military_average;};
Inputs current(const LiveSessionState &s,const BnwPlayerDoTurnTradeJournal &trade,const BnwPlayerDoTurnMilitaryJournal &military,const BnwGrandMilitaryAverageJournal &average,const BnwGrandWorldAverageRules &rules,const BnwHappinessSuffixRules &native,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t player){
 need(id.payload_sha256.size()==64&&id.rules_sha256==rules.identity()&&id.rules_sha256==native.identity(),"Grand guess selected rule/source identity differs");
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.setup.players.size()==64&&player>=0&&player<22,"Grand guess complete current BNW roster unavailable");
 Inputs r{bnw_player_do_turn_revolts_revision(s,native,db,id,player),bnw_player_military_registry_revision(s,player),bnw_grand_military_inputs_revision(s)};
 need(trade.native().player_id==player&&trade.after_world_revision()==r.trade,"Grand guess SAME accepted Trade full-world seal stale");
 need(military.native().player_id==player&&military.after_world_revision()==r.military&&military.grand_military_input_revision()==r.military_average,"Grand guess SAME accepted Military component/input seal stale");
 need(average.player()==player&&average.input_revision()==r.military_average,"Grand guess SAME accepted military average input seal stale");return r;
}
}
std::string bnw_grand_native_component_revision(const LiveSessionState &s,std::int64_t player){return bnw_grand_native_component_revision(s,player,grand(s,player));}
std::string bnw_grand_native_component_revision(const LiveSessionState &s,std::int64_t player,const saves::PlayerStrategyBlock &g){const auto &p=s.world.all_players->players.at(player);Digest h;h.n(player);h.n(p.saved_player_id);h.n(static_cast<unsigned>(p.strategy.layout));h.n(p.strategy.start_offset);h.n(p.strategy.end_offset);h.n(p.strategy.dimensions.major);h.n(p.strategy.dimensions.grand);h.n(p.strategy.dimensions.civ_players);h.n(p.strategy.dimensions.profile_extensions.values.size());h.n(p.strategy.blocks.size());h.t(g.name?g.name:"");h.n(g.start_offset);h.n(g.end_offset);h.fields(g.fields);h.arrays(g.arrays);h.hashes(g.hashes);return identify_bytes(h.bytes).sha256;}
std::string bnw_grand_world_average_inputs_revision(const LiveSessionState &s){
 Digest h;h.n(static_cast<unsigned>(s.cities.profile));h.n(s.world.payload_end_offset);
 // Founder read model consumes raw plot owners even when religions are empty.
 // Seal raw and live geometry/owner inputs, including absence and duplicates.
 h.n(static_cast<unsigned>(s.world.map_header.layout));h.n(s.world.map_header.grid.width);h.n(s.world.map_header.grid.height);h.n(s.world.map_header.wrap_x);h.n(s.world.map_header.wrap_y);
 h.n(s.world.map_projection.has_value());if(s.world.map_projection){const auto &m=*s.world.map_projection;h.n(m.grid.width);h.n(m.grid.height);h.n(m.wrap_x);h.n(m.wrap_y);h.n(m.cells.size());for(const auto &c:m.cells){h.n(c.index);h.n(c.x);h.n(c.y);h.opt(c.owner);}}
 h.n(s.world.map_plots.has_value());if(s.world.map_plots){const auto &m=*s.world.map_plots;h.n(static_cast<unsigned>(m.layout));h.n(m.plots.size());for(const auto &p:m.plots){h.n(p.index);h.n(p.x);h.n(p.y);h.fields(p.fields);}}

 h.n(s.world.pregame.has_value());if(s.world.pregame){const auto &pg=*s.world.pregame;h.n(static_cast<unsigned>(pg.layout));for(const auto name:{"civilizations"}){const auto it=pg.slots.integer_arrays.find(name);h.n(it!=pg.slots.integer_arrays.end());if(it!=pg.slots.integer_arrays.end())h.values(it->second);}const auto keys=pg.slots.string_arrays.find("civilization_keys");h.n(keys!=pg.slots.string_arrays.end());if(keys!=pg.slots.string_arrays.end()){h.n(keys->second.size());for(const auto &key:keys->second)h.t(key);}}
 h.n(s.world.all_players.has_value());if(s.world.all_players){h.n(static_cast<unsigned>(s.world.all_players->layout));h.n(s.world.all_players->players.size());for(const auto &p:s.world.all_players->players){h.n(p.index);h.n(p.saved_player_id);h.n(static_cast<unsigned>(p.cities.layout));const auto &r=p.cities.header;h.n(static_cast<unsigned>(r.layout));h.n(r.start_offset);h.n(r.end_offset);h.n(r.source_slots);h.fields(r.fields);h.values(r.next_free_indices);h.n(r.wire_city_count);h.n(p.cities.cities.size());for(const auto &c:p.cities.cities){h.n(c.index);h.n(static_cast<unsigned>(c.core.layout));h.n(c.core.start_offset);h.n(c.core.end_offset);h.opt(c.core.version);h.n(c.core.fields.size());for(const auto &f:c.core.fields){h.t(f.name?f.name:"");h.n(static_cast<unsigned>(f.encoding));h.opt(f.wire_count);h.values(f.values);h.values(f.row_keys);h.values(f.row_counts);h.values(f.hashes);h.values(f.bytes);}h.n(static_cast<unsigned>(c.production.layout));h.opt(c.production.context.bnw_city_version);h.n(c.production.blocks.size());for(const auto &b:c.production.blocks)h.production(b);h.n(static_cast<unsigned>(c.tail.layout));h.opt(c.tail.context.bnw_city_version);h.n(c.tail.blocks.size());for(const auto &b:c.tail.blocks)h.tail(b);}h.n(static_cast<unsigned>(p.strategy.layout));h.n(p.strategy.blocks.size());for(const auto &b:p.strategy.blocks)if(b.name&&std::string_view(b.name)=="policies"){h.t(b.name);h.fields(b.fields);h.arrays(b.arrays);h.hashes(b.hashes);}h.n(static_cast<unsigned>(p.traits.layout));h.n(p.traits.fields.size());for(const auto &f:p.traits.fields){h.t(f.name?f.name:"");h.n(static_cast<unsigned>(f.encoding));h.values(f.values);}}}
 h.n(s.setup.players.size());for(const auto &[id,p]:s.setup.players){h.n(id);h.n(p.policies_resolved);h.values(p.policies);}
 h.n(s.cities.players.size());for(const auto &[id,p]:s.cities.players){h.n(id);h.n(p.civilization);h.n(p.golden_age);h.n(p.cities.size());for(const auto &[key,c]:p.cities){h.n(key);h.n(c.owner);h.n(c.id);h.n(c.source_core.has_value());if(c.source_core){h.n(c.source_core->first);h.n(c.source_core->second);}h.n(c.resistance);h.n(c.razing);h.n(c.building_counts_resolved);for(const auto *m:{&c.real_buildings,&c.free_buildings}){h.n(m->size());for(const auto &[type,v]:*m){h.n(type);h.n(v);}}h.n(c.religions.size());for(const auto &r:c.religions){h.n(r.religion);h.n(r.followers);h.n(r.pressure);}}}
 h.n(s.technologies.size());for(const auto &t:s.technologies){h.n(static_cast<unsigned>(t.profile()));h.n(t.team());h.n(t.slots());h.n(t.last_tech());h.n(t.nondefault_values().size());for(const auto &[id,v]:t.nondefault_values()){h.n(id);h.n(v.has_tech);h.n(v.no_trade);h.n(v.progress_times100);h.n(v.count);}}
 const auto &rc=s.world.religion_culture;h.n(static_cast<unsigned>(rc.layout));h.n(rc.start_offset);h.n(rc.culture_offset);h.n(rc.end_offset);h.opt(rc.religions_version);h.n(rc.religions.size());for(const auto &r:rc.religions){h.opt(r.version);h.n(r.religion);h.n(r.founder);h.n(r.holy_city_x);h.n(r.holy_city_y);h.n(r.turn_founded);h.n(r.pantheon);h.n(r.enhanced);h.opt(r.reformed);}
 const auto &l=s.world.leagues;h.n(static_cast<unsigned>(l.layout));h.opt(l.version);h.n(l.start_offset);h.n(l.end_offset);h.n(l.leagues.size());
 return identify_bytes(h.bytes).sha256;
}
BnwGrandWorldAverageJournal stage_bnw_grand_world_average_journal(const LiveSessionState &s,const BnwPlayerDoTurnTradeJournal &trade,const BnwPlayerDoTurnMilitaryJournal &military,const BnwGrandMilitaryAverageJournal &military_average,const BnwGrandWorldAverageRules &rules,const BnwHappinessSuffixRules &native,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t player){
 const auto c=current(s,trade,military,military_average,rules,native,db,id,player);
 BnwGrandWorldAverageJournal j;j.player_=player;j.identity_=id;j.trade_=c.trade;j.military_=c.military;j.military_average_=c.military_average;j.query_=bnw_grand_world_average_inputs_revision(s);j.grand_=bnw_grand_native_component_revision(s,player);
 j.averages_=quote_bnw_grand_world_averages(s,rules,id);if(!j.averages_.calculated)throw std::runtime_error(j.averages_.blocker);
 static_assert(std::is_nothrow_move_constructible_v<BnwGrandWorldAverageJournal>);return j;
}
BnwGrandGuessEmptyJournal stage_bnw_grand_guess_empty(const LiveSessionState &s,const BnwPlayerDoTurnTradeJournal &trade,const BnwPlayerDoTurnMilitaryJournal &military,const BnwGrandMilitaryAverageJournal &military_average,const BnwGrandWorldAverageJournal &world_average,const BnwGrandWorldAverageRules &rules,const BnwHappinessSuffixRules &native,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t player){
 const auto c=current(s,trade,military,military_average,rules,native,db,id,player);
 need(world_average.player()==player&&world_average.identity().payload_sha256==id.payload_sha256&&world_average.identity().rules_sha256==id.rules_sha256&&world_average.trade_revision()==c.trade&&world_average.military_revision()==c.military&&world_average.military_average_revision()==c.military_average,"Grand guess SAME private world-average parent receipts stale");
 const auto query=bnw_grand_world_average_inputs_revision(s),component=bnw_grand_native_component_revision(s,player);
 need(world_average.query_revision()==query,"Grand guess accepted world-average query inputs stale");need(world_average.grand_revision()==component,"Grand guess native Grand component stale after accepted averages");validate_grand(s,rules,player);
 need(s.world.pregame&&s.world.pregame->layout==bnw&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64,"Grand guess actual team/pregame owner unavailable");
 const auto &mapping=s.world.pregame->archive.integer_arrays.at("team_types");need(mapping.size()==64,"Grand guess actual player-team inventory unknown");const auto &own=s.world.all_players->players.at(player);const auto own_id=scalar(own,"m_eID");need(own_id==player&&scalar(own,"m_bAlive")==1&&s.setup.players.at(player).alive,"Grand guess actual own player identity/alive differs");const auto team=mapping.at(player);need(team>=0&&team<64&&team==s.setup.players.at(player).team,"Grand guess actual own team mirror differs");const auto &t=s.world.teams->teams.at(team);need(t.index==static_cast<std::uint64_t>(team)&&named(t.fields,"id").value==team,"Grand guess actual native own team ID differs");const auto &met=named(t.arrays,"has_met");need(met.columns==1&&met.values.size()==64,"Grand guess actual hasMet64 cardinality unknown");for(auto flag:met.values)need(flag==0||flag==1,"Grand guess native hasMet boolean invalid");
 BnwGrandGuessEmptyJournal j;j.player_=player;j.query_=query;j.grand_=component;j.inspected_.reserve(22);
 for(std::int64_t p=0;p<22;++p){const auto &raw=s.world.all_players->players.at(p);need(raw.index==static_cast<std::uint64_t>(p)&&raw.saved_player_id==p&&raw.core.layout==bnw&&scalar(raw,"m_eID")==p,"Grand guess native major slot identity differs");const auto alive=scalar(raw,"m_bAlive");need((alive==0||alive==1)&&s.setup.players.at(p).alive==(alive==1),"Grand guess actual alive major mirror differs");BnwGrandForeignMajorCheck item{p,alive==1,p==own_id,{},{}};if(alive&&p!=own_id){const auto target=mapping.at(p);need(target>=0&&target<64&&target==s.setup.players.at(p).team,"Grand guess current foreign player-team mirror differs");const auto &foreign=s.world.teams->teams.at(target);need(foreign.index==static_cast<std::uint64_t>(target)&&named(foreign.fields,"id").value==target,"Grand guess current foreign team Read ID differs");item.team=target;item.met=met.values.at(target)!=0;if(*item.met)throw std::runtime_error("Grand guess first eligible major="+std::to_string(p)+" requires actual source strategy id=0 type="+first_strategy_type(rules)+" priority producer, then remaining strategy slots/sort/confidence/stores/logger; empty completion forbidden");}j.inspected_.push_back(item);}
 // Source has no else/reset for dead/self/not-met. No native stores, logging,
 // Lua, presentation or RNG operations are reachable in this supported branch.
 static_assert(std::is_nothrow_move_constructible_v<BnwGrandGuessEmptyJournal>);return j;
}
}
