#include "civ5/runtime/bnw_player_happiness_prefix_owner.hpp"
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_food_growth_prefix.hpp"
#include "civ5/runtime/bnw_city_religion_yield_owner.hpp"
#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
#include "civ5/runtime/bnw_city_tail_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw BnwPlayerHappinessPrefixUnavailable(why);}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* found=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!found,"Player happiness duplicate source field");found=&r;}
 need(found,"Player happiness required source field absent");return *found;
}
std::int32_t current(const LiveSessionState& s,std::int32_t player){
 need(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&player>=0&&player<64,
  "Player happiness native source roster unavailable");
 const auto& raw=s.world.all_players->players.at(static_cast<std::size_t>(player));
 const auto& f=named(raw.core.fields,"m_iHappiness");
 need(raw.saved_player_id==player&&raw.index==static_cast<std::uint64_t>(player)&&raw.core.layout==bnw&&
  f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.hashes.empty()&&f.bytes.empty()&&f.row_counts.empty()&&
  f.values.front()>=INT32_MIN&&f.values.front()<=INT32_MAX,"Player happiness native current scalar unavailable");
 need(s.unhappiness.players.at(player).happiness==f.values.front(),"Player happiness raw/owned cache mirror stale");
 return static_cast<std::int32_t>(f.values.front());
}
void number(std::vector<std::uint8_t>& bytes,std::uint64_t n){for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(n>>(8*i)));}
void text(std::vector<std::uint8_t>& bytes,std::string_view s){number(bytes,s.size());bytes.insert(bytes.end(),s.begin(),s.end());}
// Metadata comparison, not an export/restore or an operand result. Its private
// issuer is a genuine actual owner and it never invokes future numeric getters.
std::string raw_seal(const LiveSessionState& s,const SessionCheckpointIdentity& id){
 std::vector<std::uint8_t> bytes;
 const auto put=[&](const saves::CivisSaveSection& section){number(bytes,section.id);number(bytes,section.flags);
  number(bytes,section.bytes.size());bytes.insert(bytes.end(),section.bytes.begin(),section.bytes.end());};
 put(make_bnw_player_core_checkpoint(s.world,s.world,id,true));
 const auto city=make_bnw_city_checkpoint_quadruple(s.world,s.world,id,true);
 put(city.core);put(city.production);put(city.management);put(city.tail);
 need(s.world.pregame&&s.world.pregame->layout==bnw&&s.world.leagues.layout==bnw,
  "Player happiness native metadata profile unavailable");
 const auto& handicaps=s.world.pregame->archive.integer_arrays.at("handicaps");
 number(bytes,handicaps.size());for(auto n:handicaps)number(bytes,static_cast<std::uint64_t>(n));
 number(bytes,static_cast<std::uint64_t>(s.world.pregame->archive.integers.at("game_turn")));
 // Resource-banning reaches membership, active resolution flag and proposer
 // choice only. Retain their source order without performing that future query.
 number(bytes,s.world.leagues.leagues.size());
 for(const auto& league:s.world.leagues.leagues){
  number(bytes,league.members.size());for(const auto& member:league.members)number(bytes,static_cast<std::uint64_t>(member.player));
  number(bytes,league.active_resolutions.size());
  for(const auto& resolution:league.active_resolutions){number(bytes,resolution.effects.flags.size());
   for(const auto& f:resolution.effects.flags){text(bytes,f.name?f.name:"");number(bytes,f.value);}
   number(bytes,static_cast<std::uint64_t>(resolution.proposer_decision.vote.choice));}
 }
 return identify_bytes(bytes).sha256;
}
}
struct BnwPlayerHappinessPrefixOwner::Impl {
 std::thread::id thread=std::this_thread::get_id();std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 const BnwCityEntryPending* parent=nullptr;const BnwCityFoodGrowthPending* population_parent=nullptr;const BnwCityReligionYieldOwner* religion=nullptr;
 std::shared_ptr<const LiveSessionState> state;std::shared_ptr<const saves::SavedSession> source;
 SessionCheckpointIdentity identity;std::function<bool()> rules_current;BnwHappinessPrefixRules rules;
 std::int32_t player=-1,expected=0;std::optional<std::int64_t> excess;
 std::uint64_t generation=0,revision=0;std::uint8_t next=0;std::string seal;
 std::atomic<bool> retired{false};
};
BnwPlayerHappinessPrefixOwner::BnwPlayerHappinessPrefixOwner(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwPlayerHappinessPrefixOwner::~BnwPlayerHappinessPrefixOwner()=default;
void BnwPlayerHappinessPrefixOwner::retire()noexcept{if(impl_)impl_->retired.store(true);}
template<class Parent>void BnwPlayerHappinessPrefixOwner::require_current_bound(const Parent& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{
 need(impl_&&impl_->thread==std::this_thread::get_id(),"Player happiness foreign owner thread/moved handle");const auto& p=*impl_;
 need(!p.retired.load()&&host&&p.host.lock()==host&&((std::is_same_v<Parent,BnwCityEntryPending>&&static_cast<const void*>(p.parent)==&parent)||
  (std::is_same_v<Parent,BnwCityFoodGrowthPending>&&static_cast<const void*>(p.population_parent)==&parent))&&p.rules_current&&p.rules_current(),
  "Player happiness actual parent/RulesImpl retired");
 (void)host->session().bnw_game_debug_mode();
 need(host->identity().payload_sha256==p.identity.payload_sha256&&host->identity().rules_sha256==p.identity.rules_sha256&&
  host->database().gameplay_fingerprint_sha256()==p.identity.rules_sha256,"Player happiness SAME rules/source differs");
 need(host->session().read_state_lease()==p.state&&host->session().state_generation()==p.generation&&
  &host->session().source()==p.source.get(),"Player happiness actual allocation/generation retired");
 parent.require_current();p.religion->require_player_happiness_tail(parent,host);
 need(p.state->ui_source_revision==p.revision&&current(*p.state,p.player)==p.expected&&
  p.state->cities.players.at(p.player).excess_happiness==p.excess&&raw_seal(*p.state,p.identity)==p.seal,
  "Player happiness current source/cache/epoch changed outside actual prefix");
 need(p.rules_current(),"Player happiness RulesImpl retired after source capture");
}
template<class Parent>std::unique_ptr<BnwPlayerHappinessPrefixOwner> BnwPlayerHappinessPrefixOwner::from_religion_tail_bound(
 const Parent& parent,const BnwCityReligionYieldOwner& religion,
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,const std::shared_ptr<const LiveSessionState>& state,
 const std::shared_ptr<const saves::SavedSession>& source){
 need(bool(host),"Player happiness actual Host absent");(void)host->session().bnw_game_debug_mode();
 auto p=std::make_unique<Impl>();p->rules_current=host->database().immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"Player happiness actual immutable rules required before borrow");
 religion.require_player_happiness_tail(parent,host);parent.require_current();const auto progress=parent.progress();
 need(state&&source&&host->session().read_state_lease()==state&&&host->session().source()==source.get(),
  "Player happiness actual parent allocation differs");
 p->host=host;
 if constexpr(std::is_same_v<Parent,BnwCityEntryPending>)p->parent=&parent;
 else p->population_parent=&parent;
 p->religion=&religion;p->state=state;p->source=source;
 p->identity=host->identity();p->player=progress.owner;p->generation=host->session().state_generation();
 p->rules=BnwHappinessPrefixRules::load(host->database());
 need(p->rules.rules_sha256()==p->identity.rules_sha256,"Player happiness actual Rules snapshots differ");
 // Prove reached native Handicap slot and global Type/hash identity. This is
 // metadata, not a future stage result or a caller-provided native Info table.
 need(state->world.pregame&&state->world.pregame->layout==bnw,"Player happiness pregame unavailable");
 const auto& handicaps=state->world.pregame->archive.integer_arrays.at("handicaps");
 need(handicaps.size()==64&&p->player>=0&&p->player<64,"Player happiness actual handicap slots unavailable");
 const auto selected=handicaps.at(static_cast<std::size_t>(p->player));bool located=false;
 need(selected>=0&&selected<=INT32_MAX&&selected==state->setup.players.at(p->player).handicap,
  "Player happiness selected native HandicapInfo/mirror unavailable");
 const auto report=host->database().prefetch_info("HandicapInfos",[&](auto slot,auto authored,const auto& row){
  need(p->rules_current(),"Player happiness rules retired during Info capture");
  need(slot<=INT32_MAX,"Player happiness native HandicapInfo span exceeds int32");
  if(static_cast<std::int64_t>(slot)!=selected)return true;
  need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot),"Player happiness actual native HandicapInfo slot differs");
  const auto type=row.text("Type");const auto* named_type=type?host->types().find(*type):nullptr;
  const auto hash=type?content::saved_info_type_hash(*type):0;const auto* hashed=host->types().find_hash(hash);
  need(type&&named_type&&hashed&&named_type->table=="HandicapInfos"&&hashed->table==named_type->table&&
   named_type->id==selected&&hashed->id==selected&&named_type->type==*type&&hashed->type==*type&&
   named_type->hash==hash&&hashed->hash==hash&&!located,"Player happiness native HandicapInfo Type/hash identity differs");
  located=true;return true;
 });
 need(report.all_succeeded()&&report.slots<=INT32_MAX&&!report.overlapping_ids&&located,
  "Player happiness selected HandicapInfo absent/incomplete");
 p->expected=current(*state,p->player);p->excess=state->cities.players.at(p->player).excess_happiness;
 p->revision=state->ui_source_revision;p->seal=raw_seal(*state,p->identity);
 auto out=std::unique_ptr<BnwPlayerHappinessPrefixOwner>(new BnwPlayerHappinessPrefixOwner(std::move(p)));
 out->require_current(parent,host);return out;
}
template<class Parent>BnwPlayerHappinessPrefixOwner::Step BnwPlayerHappinessPrefixOwner::prepare_next_bound(const Parent& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{
 require_current(parent,host);const auto& p=*impl_;
 if(p.next==6)throw BnwPlayerHappinessPrefixUnavailable(
  "Player happiness mandatory genuine founder-Lua/SAME script-world epoch unavailable; native six stores retained, GameData fanout pending");
 need(p.next<6,"Player happiness actual native stage invalid");
 BnwHappinessNativeStore store;
 try{store=detail::bnw_happiness_prefix_reached_stage_math(*p.state,p.rules,p.identity,p.player,
  static_cast<BnwHappinessNativeStore::Stage>(p.next));}
 catch(const std::exception& e){throw BnwPlayerHappinessPrefixUnavailable(std::string("Player happiness reached native operand unavailable: ")+e.what());}
 auto staged=*p.state;auto& raw=staged.world.all_players->players.at(static_cast<std::size_t>(p.player));
 for(auto& f:raw.core.fields)if(f.name&&std::string_view(f.name)=="m_iHappiness")f.values.front()=store.happiness;
 staged.unhappiness.players.at(p.player).happiness=store.happiness;staged.cities.players.at(p.player).excess_happiness.reset();
 Step result{impl_.get(),p.next,p.expected,store.happiness,store.operand,raw_seal(staged,p.identity)};
 require_current(parent,host);return result;
}
template<class Parent>void BnwPlayerHappinessPrefixOwner::validate_before_publication_bound(const Step& step,const Parent& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{
 require_current(parent,host);need(step.owner==impl_.get()&&step.stage==impl_->next&&step.before==impl_->expected,
  "Player happiness prepared stage replay/cross-owner refusal");
}
std::unique_ptr<BnwPlayerHappinessPrefixOwner> BnwPlayerHappinessPrefixOwner::from_religion_tail(
 const BnwCityEntryPending& parent,const BnwCityReligionYieldOwner& religion,
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,const std::shared_ptr<const LiveSessionState>& state,
 const std::shared_ptr<const saves::SavedSession>& source){
 return from_religion_tail_bound(parent,religion,std::move(host),state,source);
}
void BnwPlayerHappinessPrefixOwner::require_current(const BnwCityEntryPending& parent,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{require_current_bound(parent,host);}
BnwPlayerHappinessPrefixOwner::Step BnwPlayerHappinessPrefixOwner::prepare_next(const BnwCityEntryPending& parent,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{return prepare_next_bound(parent,host);}
void BnwPlayerHappinessPrefixOwner::validate_before_publication(const Step& step,const BnwCityEntryPending& parent,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{validate_before_publication_bound(step,parent,host);}
std::unique_ptr<BnwPlayerHappinessPrefixOwner> BnwPlayerHappinessPrefixOwner::from_population_religion_tail(
 const BnwCityFoodGrowthPending& parent,const BnwCityReligionYieldOwner& religion,
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,const std::shared_ptr<const LiveSessionState>& state,
 const std::shared_ptr<const saves::SavedSession>& source){
 parent.require_population_player_return();
 return from_religion_tail_bound(parent,religion,std::move(host),state,source);
}
void BnwPlayerHappinessPrefixOwner::require_current(const BnwCityFoodGrowthPending& parent,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{require_current_bound(parent,host);}
BnwPlayerHappinessPrefixOwner::Step BnwPlayerHappinessPrefixOwner::prepare_next(const BnwCityFoodGrowthPending& parent,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{return prepare_next_bound(parent,host);}
void BnwPlayerHappinessPrefixOwner::validate_before_publication(const Step& step,const BnwCityFoodGrowthPending& parent,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{validate_before_publication_bound(step,parent,host);}
void BnwPlayerHappinessPrefixOwner::commit_published(Step&& step,std::uint64_t revision)noexcept{
 impl_->expected=step.after;impl_->seal.swap(step.next_raw_seal);impl_->excess.reset();impl_->revision=revision;++impl_->next;
}
}
