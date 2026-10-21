#include "civ5/runtime/bnw_game_deals_turn_prefix.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/rules_database.hpp"
#include <climits>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <thread>
#include <type_traits>
namespace {
using namespace civ5;using namespace civ5::runtime;
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class F>void refused(F&& f,BnwDealPrefixError code){bool caught=false;try{f();}catch(const BnwDealPrefixRefusal& e){need(e.code()==code,"Unexpected deal refusal");caught=true;}need(caught,"Expected deal refusal absent");}
auto& named(auto& rows,const char* key){for(auto& r:rows)if(r.name&&std::string_view(r.name)==key)return r;throw std::runtime_error("Test actual raw field absent");}
saves::DealObservation deal(std::int32_t final){
 saves::DealObservation d;d.version=3;d.from_player=0;d.to_player=1;d.final_turn=final;
 d.considering_renewal=false;d.checked_renewal=false;d.cancelled=false;return d;
}
saves::TradedItemObservation item(std::int32_t type,std::int32_t final,bool renewed=false){
 saves::TradedItemObservation i;i.version=2;i.type=type;i.final_turn=final;i.from_player=0;
 i.from_renewed=renewed;i.to_renewed=true;return i;
}
std::int64_t gold(const LiveSessionState& s,std::size_t p){return named(named(s.world.all_players->players.at(p).administration.blocks,"treasury").fields,"m_iGoldPerTurnFromDiplomacy").value;}
void set_gold(LiveSessionState& s,std::size_t p,std::int32_t n){named(named(s.world.all_players->players.at(p).administration.blocks,"treasury").fields,"m_iGoldPerTurnFromDiplomacy").value=n;s.maintenance.players.at(static_cast<std::int64_t>(p)).diplomacy_gold=n;}
void scenarios(const LiveSessionState& actual,const content::RulesDatabase& database,const content::InfoTypeCatalog& types){
 // These are explicitly detached variations of an ACTUAL decoded world,
 // not genuine triggers, issued Host scenarios or live turn publications.
 need(actual.setup.game_turn>=INT32_MIN&&actual.setup.game_turn<INT32_MAX,"Fixture turn native width unavailable");
 const auto turn=static_cast<std::int32_t>(actual.setup.game_turn);
 need(turn<INT32_MAX,"Fixture turn has no later native turn");
 const auto base=[&]{auto s=actual;s.world.history.deals.groups[1].clear();return s;};
 auto s=base();const auto proposed=s.world.history.deals.groups[0].size(),history=s.world.history.deals.groups[2].size();
 const auto random=s.random.gameplay.state();BnwDealPrefixObservation r;
 detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::empty_returned&&r.stores.empty()&&s.world.history.deals.groups[0].size()==proposed&&s.world.history.deals.groups[2].size()==history&&s.random.gameplay.state()==random,"Empty source branch has effects");
 auto ended=deal(turn),later=deal(turn+1);ended.items={item(0,-1)};later.items={item(3,-1)};
 s=base();s.world.history.deals.groups[1]={ended,later};r={};
 detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::list_migration_returned&&r.migrated==1&&s.world.history.deals.groups[1].size()==1&&s.world.history.deals.groups[1][0].final_turn==turn+1&&s.world.history.deals.groups[2].size()==history+1,"Ordered deal migration failed");
 // First scan MUST reach later expiry before ANY earlier validity resource.
 auto resource=deal(turn+1);resource.items={item(3,turn+1)};
 auto gpt=deal(turn+1);auto payment=item(1,turn);payment.data[0]=7;gpt.items={payment};
 s=base();set_gold(s,0,-9);set_gold(s,1,20);s.world.history.deals.groups[1]={resource,gpt};r={};
 detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::notifications_required&&r.deal_index==1&&r.stores.size()==3&&gold(s,0)==-2&&gold(s,1)==13&&s.maintenance.players.at(0).diplomacy_gold==-2&&s.maintenance.players.at(1).diplomacy_gold==13&&s.world.history.deals.groups[1][1].items[0].to_renewed==false&&s.world.history.deals.groups[2].size()==history,"GPT order/mirrors/mandatory notification prefix failed");
 refused([&]{detail::run_bnw_deals_detached_prefix(s,database,types,r);},BnwDealPrefixError::invalid_source);
 // Renewed flag clears AFTER teams but skips even an unavailable resource end.
 auto renewing=deal(turn);renewing.items={item(3,turn,true)};
 s=base();s.world.history.deals.groups[1]={renewing};r={};
 detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::active_player_ui_required&&r.stores.size()==1&&s.world.history.deals.groups[1][0].items[0].to_renewed==false&&s.world.history.deals.groups[1][0].items[0].from_renewed==true&&s.world.history.deals.groups[2].size()==history,"Renewal or pre-migration UI frontier failed");
 s=base();s.world.history.deals.groups[1]={renewing};s.setup.players.at(1).team=64;r={};
 refused([&]{detail::run_bnw_deals_detached_prefix(s,database,types,r);},BnwDealPrefixError::invalid_source);
 need(s.world.history.deals.groups[1][0].items[0].to_renewed==true&&r.stores.empty(),"TEAM read moved after renewal store");
 // TO unary - overflow is reached only AFTER actual FROM setter.
 payment.data[0]=INT32_MIN;gpt.items={payment};s=base();set_gold(s,0,0);set_gold(s,1,0);s.world.history.deals.groups[1]={gpt};r={};
 refused([&]{detail::run_bnw_deals_detached_prefix(s,database,types,r);},BnwDealPrefixError::native_overflow);
 need(gold(s,0)==INT32_MIN&&gold(s,1)==0&&s.maintenance.players.at(0).diplomacy_gold==INT32_MIN&&r.stores.size()==2,"Reached unary overflow discarded/moved prior setter");
 payment.data[0]=1;gpt.items={payment};s=base();set_gold(s,0,INT32_MAX);set_gold(s,1,0);s.world.history.deals.groups[1]={gpt};r={};
 refused([&]{detail::run_bnw_deals_detached_prefix(s,database,types,r);},BnwDealPrefixError::native_overflow);
 need(gold(s,0)==INT32_MAX&&r.stores.size()==1,"FROM overflow changed Treasury before Add completed");
 s=base();s.world.history.deals.groups[1]={resource};r={};detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::resource_total_required&&r.stores.empty(),"Resource validity guessed total/no mutation");
 auto peace=resource;peace.items.insert(peace.items.begin(),item(13,turn+1));s=base();s.world.history.deals.groups[1]={peace};r={};detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::list_migration_returned,"Peace treaty did not short-circuit later resource");
 // Nonpositive peace does not enter the native validity conditional.
 peace.items[0].final_turn=-1;s=base();s.world.history.deals.groups[1]={peace};r={};detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::resource_total_required,"Nonpositive peace wrongly short-circuited resource");
 // Today renewal ending succeeds; validity must then skip that peace item.
 peace.items[0].final_turn=turn;peace.items[0].from_renewed=true;s=base();s.world.history.deals.groups[1]={peace};r={};detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::resource_total_required&&r.stores.size()==1,"Expired renewed peace wrongly short-circuited resource");
 payment.data[0]=7;gpt.items={payment};s=base();set_gold(s,0,0);set_gold(s,1,0);s.maintenance.players.at(0).diplomacy_gold=8;s.world.history.deals.groups[1]={gpt};r={};
 refused([&]{detail::run_bnw_deals_detached_prefix(s,database,types,r);},BnwDealPrefixError::invalid_source);
 need(gold(s,0)==0&&r.stores.size()==1,"Invalid owned mirror changed raw Treasury");
 // Explicit raw hashed-slot witness, duplicate SET-last wins, unknown/null rows.
 const content::InfoTypeEntry* resource_type=nullptr;
 for(const auto& f:actual.world.all_players->players[0].core.fields)if(f.name&&std::string_view(f.name)=="m_paiResourceExport")for(auto h:f.hashes)if(h){const auto* t=types.find_hash(h);if(t&&t->table=="Resources"){resource_type=t;break;}}
 need(resource_type!=nullptr,"Actual Resources native hashed slot unavailable");
 const auto hash=content::saved_info_type_hash(resource_type->type);
 std::uint32_t unknown_hash=UINT32_MAX;while(!unknown_hash||types.find_hash(unknown_hash))--unknown_hash;
 auto exp=deal(turn+1);auto ri=item(3,turn);ri.data[0]=resource_type->id;ri.data[1]=3;exp.items={ri};
 s=base();auto& f=named(s.world.all_players->players[0].core.fields,"m_paiResourceExport");f.hashes={hash,0,unknown_hash,hash};f.values={100,-91,7};f.wire_count=4;s.world.history.deals.groups[1]={exp};r={};
 const auto to_before=named(s.world.all_players->players[1].core.fields,"m_paiResourceImport").values;
 detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::resource_happiness_required&&f.values==std::vector<std::int64_t>({100,-91,4})&&named(s.world.all_players->players[1].core.fields,"m_paiResourceImport").values==to_before&&r.stores.size()==2,"Export/DoUpdateHappiness/import chronology or duplicate semantics failed");
 ri.data[1]=0;exp.items={ri};s=base();s.world.history.deals.groups[1]={exp};r={};detail::run_bnw_deals_detached_prefix(s,database,types,r);
 need(r.boundary==BnwDealPrefixBoundary::resource_description_notifications_required&&r.stores.size()==1,"Zero-change resource setter fabricated happiness/store");
 ri.data[1]=INT32_MIN;exp.items={ri};s=base();s.world.history.deals.groups[1]={exp};r={};
 refused([&]{detail::run_bnw_deals_detached_prefix(s,database,types,r);},BnwDealPrefixError::native_overflow);
 need(r.stores.size()==1,"Resource unary overflow performed export/happiness");
 for(const auto type:{6,7,8,9,13,14,19}){auto effect=deal(turn+1);effect.items={item(type,turn)};s=base();s.world.history.deals.groups[1]={effect};r={};detail::run_bnw_deals_detached_prefix(s,database,types,r);need(r.boundary==(type==19?BnwDealPrefixBoundary::league_vote_effect_required:BnwDealPrefixBoundary::team_effect_required)&&r.stores.size()==1,"Mandatory ending effect skipped");}
 need(s.random.gameplay.state()==random,"Detached deals draw RNG");
}
}
int main(int argc,char** argv){
 need(argc==3,"Expected actual BNW save and selected installation");
 static_assert(!std::is_copy_constructible_v<BnwGameDealsTurnPrefix>);
 refused([]{(void)BnwGameDealsTurnPrefix::from_selected_host({});},BnwDealPrefixError::missing_parent);
 auto b=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);
 if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);
 need(bool(b.preview.host),"Actual selected Host unavailable");
 std::shared_ptr<ui::SelectedBnwPreviewHost> host=std::move(b.preview.host);
 const auto baseline=host->session().state();const auto rng=baseline.random.gameplay.state();
 scenarios(baseline,host->database(),host->types());
 auto owner=BnwGameDealsTurnPrefix::from_selected_host(host);auto alias=owner;
 need(!owner->observe().attempted,"Actual owner starts attempted");bool wrong_thread=false;
 std::thread foreign([&]{try{owner->advance();}catch(const BnwDealPrefixRefusal& e){wrong_thread=e.code()==BnwDealPrefixError::wrong_thread;}});foreign.join();need(wrong_thread&&!owner->observe().attempted,"Wrong thread consumed operation");
 owner->advance();const auto observed=owner->observe();const auto copy=owner->read_detached_state();
 need(observed.attempted&&host->session().state().random.gameplay.state()==rng&&host->session().state().world.history.deals.groups[1].size()==baseline.world.history.deals.groups[1].size(),"Actual detached owner modified live state");
 if(baseline.world.history.deals.groups[1].empty())need(observed.boundary==BnwDealPrefixBoundary::empty_returned&&copy->world.history.deals.groups[1].empty(),"Actual empty branch differs");
 else need(observed.boundary==BnwDealPrefixBoundary::list_migration_returned||!observed.required_effect.empty(),"Actual nonempty fixture needs precise admitted prefix/frontier");
 refused([&]{alias->advance();},BnwDealPrefixError::already_attempted);
 std::cout<<"Actual selected deals="<<baseline.world.history.deals.groups[1].size()<<" boundary="<<static_cast<int>(observed.boundary)<<" stores="<<observed.stores.size()<<" required="<<observed.required_effect<<'\n';
 // Exact genuine checkpoint restore retires held state/generation; no resurrection.
 const auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);
 refused([&]{owner->require_current();},BnwDealPrefixError::stale_source);alias.reset();owner.reset();
 auto retained=BnwGameDealsTurnPrefix::from_selected_host(host);std::weak_ptr<ui::SelectedBnwPreviewHost> weak=host;
 host.reset();retained->advance();need(!weak.expired(),"Actual deal owner borrows parent");retained.reset();need(weak.expired(),"Actual deal owner retains cycle");
}
