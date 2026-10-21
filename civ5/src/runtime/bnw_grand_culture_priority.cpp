#include "civ5/runtime/bnw_grand_culture_priority.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool v,const char *why){if(!v)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"BNW Culture priority native int overflow/width");return static_cast<std::int32_t>(v);}
std::int32_t add(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)*b);}
std::int32_t define(const BnwHappinessSuffixRules &r,const char *key){const auto *v=r.defines().find(key);need(v&&v->lookup.found&&v->lookup.error.empty()&&!v->retained_previous&&std::holds_alternative<std::int64_t>(v->lookup.value),"BNW Culture priority actual define unavailable");return integer(std::get<std::int64_t>(v->lookup.value));}
struct Digest {
 std::vector<std::uint8_t> bytes;
 void n(std::uint64_t v){for(int k=0;k<8;++k)bytes.push_back(static_cast<std::uint8_t>(v>>(k*8)));}
 void t(std::string_view v){n(v.size());bytes.insert(bytes.end(),v.begin(),v.end());}
 template<class T>void opt(const std::optional<T> &v){n(v.has_value());if(v)n(*v);}
 template<class T>void nodes(const T &v){n(v.size());for(const auto &a:v)node(a);}
 template<class T>void values(const T &v){n(v.size());for(auto a:v)n(a);}
 template<class T>void node(const T &v){
  if constexpr(requires{v.name;})t(v.name?v.name:"");
  if constexpr(requires{v.layout;})n(static_cast<unsigned>(v.layout));
  if constexpr(requires{v.index;})n(v.index);
  if constexpr(requires{v.start_offset;})n(v.start_offset);
  if constexpr(requires{v.end_offset;})n(v.end_offset);
  if constexpr(requires{v.version;})opt(v.version);
  if constexpr(requires{v.yield_slots;})n(v.yield_slots);
  if constexpr(requires{v.row_counts;})values(v.row_counts);
  if constexpr(requires{v.value;})n(v.value);
  if constexpr(requires{v.columns;})n(v.columns);
  if constexpr(requires{v.type_hash;})n(v.type_hash);
  if constexpr(requires{v.wire_count;}){if constexpr(requires{v.wire_count.has_value();})opt(v.wire_count);else n(v.wire_count);}
  if constexpr(requires{v.encoding;})n(static_cast<unsigned>(v.encoding));
  if constexpr(requires{v.values;})values(v.values);
  if constexpr(requires{v.bytes;})values(v.bytes);
  if constexpr(requires{v.fields;})nodes(v.fields);
  if constexpr(requires{v.arrays;})nodes(v.arrays);
  if constexpr(requires{v.blocks;})nodes(v.blocks);
  if constexpr(requires{v.blobs;})nodes(v.blobs);
  if constexpr(requires{v.children;})nodes(v.children);
  if constexpr(requires{v.hashes;}){if constexpr(requires{v.hashes.front().name;})nodes(v.hashes);else values(v.hashes);}
  if constexpr(requires{v.rows;})nodes(v.rows);
 }
};
// Inventory seal only: no foreign total/city/minor/threshold calculators are
// executed eagerly. Absence/shape is hashed; each source getter validates its
// actual branch when visited. Owned caches and raw values are both sealed.
std::string revision(const LiveSessionState &s,const SessionCheckpointIdentity &id,const content::RulesDatabase &db){
 Digest h;h.t(bnw_unit_order_world_revision(s,id,db));h.t(bnw_grand_world_average_inputs_revision(s));
 for(const auto &raw:s.world.all_players->players){h.node(raw.core);h.node(raw.strategy);h.node(raw.traits);h.node(raw.planning);h.n(raw.planning.dimensions.major);h.node(raw.trade_culture);h.n(raw.trade_culture.dimensions.major);h.n(raw.trade_culture.dimensions.yields);for(const auto &c:raw.cities.cities)h.node(c.management);}
 h.n(s.world.teams.has_value());if(s.world.teams){h.n(static_cast<unsigned>(s.world.teams->layout));h.n(s.world.teams->dimensions.major);h.n(s.world.teams->dimensions.teams);h.nodes(s.world.teams->teams);}
 for(const auto &[k,v]:s.world.pregame->archive.integer_arrays){h.t(k);h.values(v);}for(const auto &[k,v]:s.world.pregame->archive.flag_arrays){h.t(k);h.values(v);}
 for(const auto &[k,v]:s.world.pregame->archive.string_arrays){h.t(k);h.n(v.size());for(const auto &a:v)h.t(a);}
 for(const auto *opts:{&s.setup.game_options,&s.world.pregame->game_options}){h.n(opts->size());for(const auto &v:*opts){h.t(v.name);h.n(v.value);}}
 for(const auto &[p,v]:s.setup.players){h.n(p);h.n(v.alive);h.n(v.team);h.n(v.handicap);h.n(v.slot_status);h.n(v.slot_claim);}
 for(const auto &[p,v]:s.player_research.players){h.n(p);h.n(v.pregame_minor);}for(const auto &[t,v]:s.player_research.teams){h.n(t);h.values(v.met);h.values(v.at_war);}for(const auto &[t,e]:s.maintenance.team_eras){h.n(t);h.n(e);}
 for(const auto &[p,v]:s.cities.players){h.n(p);h.n(v.minor);h.n(v.capital);h.opt(v.excess_happiness);for(const auto &[c,a]:v.cities){h.n(c);h.n(a.population);h.n(a.puppet);for(const auto &[y,b]:a.yields){h.n(y);h.n(b.terrain);h.n(b.buildings);h.n(b.specialists);h.n(b.misc);h.n(b.religion);h.n(b.per_population);h.n(b.per_religion);h.n(b.rate_modifier);h.n(b.resource_modifier);h.n(b.process_modifier);}}}
 for(const auto &rel:s.world.religion_culture.religions){h.opt(rel.beliefs.version);h.values(rel.beliefs.belief_hashes);}for(const auto &rel:s.cities.religions){h.n(rel.religion);h.n(rel.founder);h.values(rel.beliefs);}
 h.n(static_cast<unsigned>(s.world.trade.layout));h.opt(s.world.trade.version);h.n(s.world.trade.start_offset);h.n(s.world.trade.routes_end_offset);h.n(s.world.trade.matrices_offset);h.n(s.world.trade.end_offset);h.n(s.world.trade.dimensions.major_slots);h.n(s.world.trade.dimensions.yield_slots);h.n(s.world.trade.routes.size());h.n(s.cities.trade_routes.size());for(const auto &v:s.world.trade.routes){h.n(v.origin_x);h.n(v.origin_y);h.n(v.destination_x);h.n(v.destination_y);h.n(v.origin_owner);h.n(v.destination_owner);}for(const auto &v:s.cities.trade_routes){h.n(v.origin_x);h.n(v.origin_y);h.n(v.destination_x);h.n(v.destination_y);h.n(v.origin_owner);h.n(v.destination_owner);}
 h.n(s.unhappiness.world_cities_percent);for(const auto &[p,v]:s.unhappiness.players){h.n(p);h.n(v.happiness);h.n(v.units);h.n(v.units_modifier);h.n(v.population_modifier);h.n(v.cities_modifier);h.n(v.occupied_modifier);h.n(v.capital_modifier);h.n(v.trait_cities_modifier);h.n(v.trait_population_modifier);h.n(v.public_opinion);h.n(v.half_specialists);for(const auto &[c,a]:v.cities){h.n(c);h.n(a.ignore);h.n(a.occupied);h.n(a.no_occupied_unhappiness);for(const auto &[t,n]:a.specialists){h.n(t);h.n(n);}}}
 for(const auto &[p,v]:s.treasury.players){h.n(p);h.n(v.ally);}for(const auto *rng:{&s.random.gameplay,&s.random.map}){h.n(static_cast<unsigned>(rng->algorithm()));h.n(rng->state().seed);h.n(rng->state().calls);h.n(rng->state().resets);}return identify_bytes(h.bytes).sha256;
}
void binding(const BnwGrandCulturePriorityPending &p,const BnwTeamTechLuaEpoch &e,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){need(e.world&&e.host&&e.source_world&&e.source_world!=e.world.get()&&e.getter_world==e.world.get()&&e.identity.payload_sha256==p.identity().payload_sha256&&e.identity.rules_sha256==p.identity().rules_sha256&&e.selected_dlc==p.profile().enabled_dlc&&e.selected_mods==p.profile().enabled_mods&&p.identity().rules_sha256==r.identity()&&r.identity()==n.identity()&&identify_bytes(db.gameplay_fingerprint_data()).sha256==r.identity(),"BNW Culture priority SAME world/VM/source/rules binding invalid");}
bool foreign(const LiveSessionState &s,std::int64_t p,std::int64_t own){const auto &raw=s.world.all_players->players.at(p);need(raw.index==static_cast<std::uint64_t>(p)&&raw.saved_player_id==p&&raw.core.layout==saves::GameStateLayout::expansion2_version1,"BNW Culture priority native foreign identity unknown");const auto scalar=[&](const char *key,bool boolean=false){const saves::PlayerCoreField *f=nullptr;for(const auto &v:raw.core.fields)if(v.name&&std::string_view(v.name)==key){need(!f,"BNW Culture priority duplicate native field");f=&v;}need(f&&f->values.size()==1&&f->encoding==(boolean?saves::PlayerCoreEncoding::boolean:saves::PlayerCoreEncoding::i32),"BNW Culture priority native foreign field unknown");const auto v=integer(f->values[0]);if(boolean)need(v==0||v==1,"BNW Culture priority alive flag malformed");return v;};need(scalar("m_eID")==p,"BNW Culture priority native foreign ID differs");const auto alive=scalar("m_bAlive",true);need(s.setup.players.at(p).alive==static_cast<bool>(alive),"BNW Culture priority current alive mirror stale");if(!alive)return false;const auto &minor=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(minor.size()==64&&s.player_research.players.at(p).pregame_minor==minor.at(p)&&s.cities.players.at(p).minor==minor.at(p),"BNW Culture priority current minor mirror stale");if(minor.at(p))return false;return p!=63&&p!=own;}
}
std::unique_ptr<BnwGrandCulturePriorityPending> stage_bnw_grand_culture_priority(const BnwGameActivationHappinessPending &enclosing,std::unique_ptr<BnwTeamTechLuaEpoch> &epoch,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){
 need(enclosing.activation_&&enclosing.happiness_&&epoch&&enclosing.builder_complete_&&enclosing.diplomacy_begin_complete_&&enclosing.completed_prefix_.size()==14&&enclosing.completed_prefix_.back()==BnwGameActivationStep::diplomacy_requests_begin&&enclosing.happiness_->native_update_complete()&&&enclosing.state()==epoch->world.get(),"BNW Culture priority requires genuine SAME enclosing native prefix/epoch");
 const auto &owner=enclosing.activation_->owner;const auto player=enclosing.ticket_.player;const auto *first=enclosing.grand_first_prefix();const auto *total=enclosing.happiness_->total_culture();
 need(owner.player_do_turn_prefix&&owner.player_do_turn_revolts&&owner.player_do_turn_trade&&owner.player_do_turn_military&&owner.grand_military_average&&owner.grand_world_averages&&owner.grand_guess_empty&&first&&first->player==player&&first->strategy==0&&first->strategy_type=="AIGRANDSTRATEGY_CULTURE"&&first->base_quoted&&first->base&&first->culture_prefix_quoted&&first->culture_initial&&!first->culture_victory_disabled&&!first->category_quoted&&!first->category&&!first->before_random&&total&&total->player()==player&&!total->epoch(),"BNW Culture priority requires private actual first GetBase/enabled Culture/completed own total");
 const auto &checked=checked_bnw_total_jons_culture(*total,*epoch,r,n,db);
 need(epoch->identity.payload_sha256==owner.identity.payload_sha256&&epoch->identity.rules_sha256==owner.identity.rules_sha256&&owner.grand_guess_empty->player()==player,"BNW Culture priority enclosing receipt identity differs");
 auto p=std::unique_ptr<BnwGrandCulturePriorityPending>(new BnwGrandCulturePriorityPending);p->identity_=owner.identity;p->profile_=total->profile();p->player_=player;p->world_address_=epoch->world.get();p->host_address_=epoch->host.get();p->progress_.base=*first->base;p->progress_.initial=*first->culture_initial;p->progress_.own_culture=*checked.total;p->progress_.foreigners.reserve(63);binding(*p,*epoch,r,n,db);p->checkpoint_=revision(*epoch->world,p->identity_,db);p->epoch_=std::move(epoch);return p;
}
BnwGrandCulturePriorityAdvance continue_bnw_grand_culture_priority(BnwGrandCulturePriorityPending &p,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){
 BnwGrandCulturePriorityAdvance a;bool uncertain=false;try{
  need(p.status_!=BnwGrandCulturePriorityStatus::complete&&p.status_!=BnwGrandCulturePriorityStatus::discarded&&p.epoch(),"BNW Culture priority no replay of completed/discarded category");binding(p,*p.epoch(),r,n,db);need(p.epoch()->world.get()==p.world_address_&&p.epoch()->host.get()==p.host_address_,"BNW Culture priority SAME retained allocation changed");need(revision(*p.epoch()->world,p.identity_,db)==p.checkpoint_,"BNW Culture priority current checkpoint inputs stale");auto &q=p.progress_;
  while(p.status_!=BnwGrandCulturePriorityStatus::complete){
   switch(p.status_){
   case BnwGrandCulturePriorityStatus::own_tourism:{auto v=quote_bnw_player_tourism(*p.epoch_->world,r,p.identity_,p.player_);if(!v.calculated)throw std::runtime_error(v.blocker);need(v.total.has_value(),"BNW Culture priority own tourism missing source result");q.own_tourism=*v.total;p.status_=BnwGrandCulturePriorityStatus::foreign_scan;break;}
   case BnwGrandCulturePriorityStatus::foreign_scan:{if(p.next_slot_==63){p.status_=BnwGrandCulturePriorityStatus::weights;break;}if(!foreign(*p.epoch_->world,p.next_slot_,p.player_)){++p.next_slot_;++q.slots_visited;break;}auto nested=stage_bnw_total_jons_culture(p.epoch_,p.profile_,p.identity_,r,n,db,p.next_slot_);q.foreigners.push_back({p.next_slot_,{},{},0});p.nested_=std::move(nested);++q.slots_visited;p.status_=BnwGrandCulturePriorityStatus::foreign_total;break;}
   case BnwGrandCulturePriorityStatus::foreign_total:{need(p.nested_&&p.nested_->epoch(),"BNW Culture priority lost actual nested total owner");if(p.nested_->status()==BnwTotalJonsCultureStatus::awaiting_founder){const auto v=continue_bnw_total_jons_culture(*p.nested_,r,n,db);a.dispatched|=v.dispatched;if(!v.applied){if(v.nonrepeatable){p.status_=BnwGrandCulturePriorityStatus::discarded;p.nested_.reset();p.epoch_.reset();a.nonrepeatable=true;}a.blocker=v.blocker;return a;}}
    uncertain=true;const auto &value=checked_bnw_total_jons_culture(*p.nested_,*p.nested_->epoch(),r,n,db);auto checkpoint=revision(*p.nested_->epoch()->world,p.identity_,db);const auto own_ahead=q.own_culture>*value.total;auto &entry=q.foreigners.back();entry.culture=*value.total;entry.founder_calls=value.founder_calls;if(own_ahead)q.ahead_culture=add(q.ahead_culture,1);else q.behind_culture=add(q.behind_culture,1);p.epoch_=p.nested_->take_completed_epoch();p.nested_.reset();p.checkpoint_=std::move(checkpoint);p.status_=BnwGrandCulturePriorityStatus::foreign_tourism;uncertain=false;break;}
   case BnwGrandCulturePriorityStatus::foreign_tourism:{auto v=quote_bnw_player_tourism(*p.epoch_->world,r,p.identity_,p.next_slot_);if(!v.calculated)throw std::runtime_error(v.blocker);need(v.total.has_value(),"BNW Culture priority foreign tourism missing source result");q.foreigners.back().tourism=*v.total;if(*q.own_tourism>*v.total)q.ahead_tourism=add(q.ahead_tourism,1);else q.behind_tourism=add(q.behind_tourism,1);q.alive=add(q.alive,1);++p.next_slot_;p.status_=BnwGrandCulturePriorityStatus::foreign_scan;break;}
   case BnwGrandCulturePriorityStatus::weights:{auto culture=std::int32_t{0},tourism=std::int32_t{0};if(q.alive>0&&q.ahead_culture>q.behind_culture)culture=mul(define(n,"AI_GS_CULTURE_AHEAD_WEIGHT"),q.ahead_culture-q.behind_culture)/q.alive;if(q.alive>0&&q.ahead_tourism>q.behind_tourism)tourism=mul(define(n,"AI_GS_CULTURE_TOURISM_AHEAD_WEIGHT"),q.ahead_tourism-q.behind_tourism)/q.alive;q.culture_weight=culture;q.tourism_weight=tourism;p.status_=BnwGrandCulturePriorityStatus::influence;break;}
   case BnwGrandCulturePriorityStatus::influence:{const auto rules=BnwCultureInfluenceRules::load(db);auto count=quote_bnw_num_civs_influential_on(*p.epoch_->world,rules,p.identity_,p.player_);if(!count.calculated)throw std::runtime_error(count.blocker);need(count.count.has_value(),"BNW Culture priority influence missing source result");const auto weight=mul(*count.count,define(n,"AI_GS_CULTURE_INFLUENTIAL_CIV_MOD"));const auto category=add(add(add(q.initial,q.culture_weight),q.tourism_weight),weight);const auto before=add(q.base,category);auto checkpoint=revision(*p.epoch_->world,p.identity_,db);q.influential=std::move(count);q.influential_weight=weight;q.category=category;q.before_random=before;p.checkpoint_=std::move(checkpoint);p.status_=BnwGrandCulturePriorityStatus::complete;break;}
   default:throw std::runtime_error("BNW Culture priority unknown source continuation stage");
   }
  }a.applied=true;
 }catch(const std::exception &e){if(uncertain){p.status_=BnwGrandCulturePriorityStatus::discarded;p.nested_.reset();p.epoch_.reset();a.nonrepeatable=true;}a.blocker=e.what();}return a;
}
const BnwGrandCulturePriorityProgress &checked_bnw_grand_culture_priority(const BnwGrandCulturePriorityPending &p,const BnwTeamTechLuaEpoch &e,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){need(p.status_==BnwGrandCulturePriorityStatus::complete&&p.progress_.category&&p.progress_.before_random&&e.world.get()==p.world_address_&&e.host.get()==p.host_address_,"BNW Culture priority completed SAME allocation receipt invalid");binding(p,e,r,n,db);need(revision(*e.world,p.identity_,db)==p.checkpoint_,"BNW Culture priority completed post-callback inputs stale");return p.progress_;}
std::unique_ptr<BnwTeamTechLuaEpoch> BnwGrandCulturePriorityPending::take_completed_epoch(){need(status_==BnwGrandCulturePriorityStatus::complete&&progress_.category&&progress_.before_random&&epoch_,"BNW Culture priority cannot extract incomplete/consumed epoch");return std::move(epoch_);}
}
