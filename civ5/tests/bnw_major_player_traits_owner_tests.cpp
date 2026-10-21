#include "civ5/runtime/bnw_major_player_traits_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <climits>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <thread>
namespace {
using namespace civ5;
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const runtime::BnwMajorPlayerTraitsUnavailable&){rejected=true;}check(rejected,"Unsupported/stale BNW trait owner accepted");}

struct NaturalMirrors {
 std::int64_t cities=0,population=0,gold=0,land=0,naval=0;
 runtime::YieldValues combat;
 bool operator==(const NaturalMirrors&)const=default;
};
NaturalMirrors actual_mirrors(const runtime::LiveSessionState& state,std::int32_t actor){
 const auto& u=state.unhappiness.players.at(actor);const auto& m=state.maintenance.players.at(actor);
 return {u.trait_cities_modifier,u.trait_population_modifier,state.treasury.players.at(actor).gold_per_partner,
  m.land_modifier,m.naval_modifier,m.combat_modifiers};
}
void paired_mirrors(const runtime::LiveSessionState& state,std::int32_t actor){
 const auto& traits=state.world.all_players->players.at(static_cast<std::size_t>(actor)).traits;
 const auto scalar=[&](std::string_view key){for(const auto& f:traits.fields)if(f.name&&std::string_view(f.name)==key){
  check(f.values.size()==1,"Natural mirror scalar shape invalid");return f.values[0];}
  throw std::runtime_error("Natural mirror trait source missing");};
 NaturalMirrors expected;expected.cities=scalar("m_iCityUnhappinessModifier");expected.population=scalar("m_iPopulationUnhappinessModifier");
 expected.land=scalar("m_iLandUnitMaintenanceModifier");expected.naval=scalar("m_iNavalUnitMaintenanceModifier");
 bool trade=false,combat=false;
 for(const auto& f:traits.fields)if(f.name){const std::string_view key=f.name;
  if(key=="m_iYieldChangePerTradePartner"){check(f.values.size()==6,"Natural trade yield span invalid");expected.gold=f.values[2];trade=true;}
  if(key=="m_paiMaintenanceModifierUnitCombat"){for(std::size_t i=0;i<f.values.size();++i)if(f.values[i])expected.combat.emplace(static_cast<std::int64_t>(i),f.values[i]);combat=true;}}
 check(trade&&combat&&actual_mirrors(state,actor)==expected,"Natural paired raw/owned trait mirrors disagree");
}

void natural(const std::filesystem::path& input,const std::filesystem::path& root){
 auto first=ui::bootstrap_selected_bnw_preview(input,root,0);
 check(first.preview.host&&!first.preview.blocker,"Natural selected BNW traits fixture unavailable");
 ui::SelectedBnwReadOnlyPreview preview(std::move(first.preview));
 std::vector<std::int64_t> saved_wound;
 for(const auto& field:preview.session().state().world.all_players->players.at(0).core.fields)
  if(field.name&&std::string_view(field.name)=="m_iWoundedUnitDamageMod")saved_wound=field.values;
 check(saved_wound.size()==1,"Natural serialized wounded player operand unavailable");
 const auto owner=runtime::BnwMajorPlayerTraitsOwner::from_selected_preview(preview,0);
 for(const auto& field:preview.session().state().world.all_players->players.at(0).core.fields)
  if(field.name&&std::string_view(field.name)=="m_iWoundedUnitDamageMod")
   check(field.values==saved_wound,"Import replayed the already saved wounded modifier");
 owner->require_current();paired_mirrors(preview.session().state(),0);const auto snapshot=owner->snapshot();
 std::optional<std::int64_t> final_improvement=-1;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  for(const auto id:snapshot.active_traits){bool row_found=false;
   const auto q=host->database().query_rows({"SELECT CombatBonusImprovement FROM Traits WHERE ID=?",{id}},[&](const auto&row){
    check(!row_found,"Duplicate native trait row in provenance oracle");row_found=true;
    if(const auto type=row.text("CombatBonusImprovement")){const auto*info=host->types().find(*type);
     check(info&&info->table=="Improvements","Defined native combat-improvement Info missing");final_improvement=info->id;
    }else final_improvement.reset();
   });check(q.all_succeeded()&&row_found,"Actual Traits provenance query failed");
  }
 });
 bool complete_null_zero=true;std::size_t nullable_rows=0,total_rows=0;
 preview.with_owned_host_camera_input([&](auto h,const auto*){
  const auto table=h->database().prefetch_info("Traits");
  const auto q=h->database().query_rows({"SELECT CombatBonusImprovement, NearbyImprovementCombatBonus FROM Traits ORDER BY ID",{}},[&](const auto&row){
   ++total_rows;if(!row.text("CombatBonusImprovement")){++nullable_rows;if(row.integer("NearbyImprovementCombatBonus")!=0)complete_null_zero=false;}
  });check(table.all_succeeded()&&q.all_succeeded()&&table.rows==total_rows,"Natural complete Traits zero-effect oracle unavailable");
 });
 const auto provenance=final_improvement?runtime::BnwTraitCombatImprovementProvenance::native_info_value:
  (complete_null_zero&&owner->cache_value("m_iNearbyImprovementCombatBonus")==0&&owner->cache_value("m_eCombatBonusImprovement")==-1?
   runtime::BnwTraitCombatImprovementProvenance::canonical_unobserved_cache:runtime::BnwTraitCombatImprovementProvenance::imported_opaque_cache);
 if(complete_null_zero&&nullable_rows){
  check(owner->cache_value("m_iNearbyImprovementCombatBonus")==0,"Natural NULL-enum zero-effect aggregate differs from genuine native cache");
  // Genuine rules+aggregate prove the installed native/Lua zero-return path;
  // changing this diagnostic enum would not affect that return. This does not
  // execute a HasTech transition: the real major-team supplier is still absent.
  check(nullable_rows<=total_rows,"Natural nullable table accounting differs");
 }else std::cout<<"UNAVAILABLE: natural complete NULL-enum zero-effect rules; observable Reset remains refused\n";
 check(snapshot.combat_improvement_provenance==provenance,"Unknown NULL trait enum mislabeled as native rule seed");
 std::optional<std::int64_t> saved_improvement;
 for(const auto&field:preview.session().state().world.all_players->players.at(0).traits.fields)
  if(field.name&&std::string_view(field.name)=="m_eCombatBonusImprovement"){
   check(field.values.size()==1,"Actual imported combat-improvement scalar malformed");saved_improvement=field.values[0];}
 check(saved_improvement&&owner->cache_value("m_eCombatBonusImprovement")==*saved_improvement,
  "Imported native opaque enum was replaced with guessed SQL NULL default");
 if(final_improvement)check(*saved_improvement==*final_improvement,"Defined native enum differs from source oracle");
 check(snapshot.player==0&&!snapshot.active_traits.empty(),"Natural major player has no active trait");
 std::vector<runtime::BnwTraitFreeResourceXCities> expected(snapshot.free_resource_x_cities.size());
 for(std::size_t i=0;i<expected.size();++i)expected[i].resource=static_cast<std::int32_t>(i);
 std::int64_t population=0;std::string rule_hash;
 preview.with_owned_host_camera_input([&](auto h,const auto*){
  rule_hash=h->database().gameplay_fingerprint_sha256();
  // Independent source oracle checks actual active rows and the positive-only
  // resource replacement rule. No edited HasTech/Pregame/synthetic traits.
  for(const auto id:snapshot.active_traits){std::string type;
   const auto scalar=h->database().query_rows({"SELECT Type, PopulationUnhappinessModifier FROM Traits WHERE ID=?",{id}},[&](const auto& row){const auto key=row.text("Type");check(bool(key),"Actual trait type missing");type=*key;population+=row.integer_at(1);});
   check(scalar.all_succeeded()&&!type.empty(),"Natural trait scalar oracle source unavailable");
   const auto resources=h->database().query_rows({"SELECT Resources.ID, ResourceQuantity, NumCities FROM Trait_FreeResourceFirstXCities JOIN Resources ON Resources.Type=ResourceType WHERE TraitType=?",{type}},[&](const auto& row){
    const auto resource=row.integer_at(0),quantity=row.integer_at(1),cities=row.integer_at(2);
    check(resource>=0&&static_cast<std::uint64_t>(resource)<expected.size(),"Natural resource oracle target invalid");
    if(quantity>0)expected[static_cast<std::size_t>(resource)]={static_cast<std::int32_t>(resource),static_cast<std::int32_t>(cities),static_cast<std::int32_t>(quantity)};
   });check(resources.all_succeeded(),"Natural free resource oracle query failed");
  }
  check(h->database().gameplay_fingerprint_sha256()==rule_hash,"Oracle changed actual rules");
 });
 check(population!=0&&owner->cache_value("m_iPopulationUnhappinessModifier")==population,
       "Natural Gandhi actual nonzero trait scalar differs from native cache loop");
 check(snapshot.free_resource_x_cities==expected&&!expected.empty(),
       "Native Reset/Init free-resource vector differs from full actual relation oracle");
 // Every known derived domain was checked; the opaque enum is the authentic
 // saved cache with explicit provenance, not a completed native Init seed;
 // public snapshot preserves the actual saved Maya/luxury histories too.
 const auto& original=preview.session().state().world.all_players->players.at(0).traits;
 check(snapshot.cache.size()==original.fields.size(),"Native trait cache omitted saved operands");
 for(std::size_t i=0;i<snapshot.cache.size();++i)check(snapshot.cache[i].values==original.fields[i].values&&
  snapshot.cache[i].hashes==original.fields[i].hashes,"Native trait snapshot changed serialized cache/history");
 bool foreign=false;std::thread other([&]{try{owner->require_current();}catch(const runtime::BnwMajorPlayerTraitsUnavailable&){foreign=true;}});other.join();
 check(foreign,"Foreign first trait borrow accepted");owner->require_current();
 // Positive SAME-session HasTech→Reset execution is pending the real major-team
 // transition supplier; no raw-flag edit is used to manufacture this coverage.
 // An unchanged natural HasTech state never authorizes a technology effect.
 // No production setter, raw-bit edit or fabricated predecessor receipt.
 {
  auto denied=runtime::BnwMajorPlayerTraitsOwner::from_selected_preview(preview,0);
  std::int64_t tech=-1;bool current=false;
  preview.with_owned_host_camera_input([&](auto h,const auto*){
   for(const auto& team:h->session().state().technologies)if(team.team()==snapshot.team){
    for(std::uint64_t i=0;i<team.slots();++i)if(h->technologies().find(i)){tech=static_cast<std::int64_t>(i);current=team.has_tech(tech);break;}}
  });check(tech>=0,"Natural technology catalog missing");
  const auto old_mirrors=actual_mirrors(preview.session().state(),0);
  const auto old_traits=preview.session().state().world.all_players->players.at(0).traits;
  const auto old_revision=preview.session().state().ui_source_revision;
  refuses([&]{(void)denied->reset_init_after_technology_flag_transition(tech,current?-1:1);});
  check(actual_mirrors(preview.session().state(),0)==old_mirrors,"Rejected native trait effect changed persistent mirrors");
  check(preview.session().state().ui_source_revision==old_revision,"Rejected trait transition changed live revision");
  const auto& unchanged=preview.session().state().world.all_players->players.at(0).traits;
  for(std::size_t i=0;i<old_traits.fields.size();++i)check(old_traits.fields[i].values==unchanged.fields[i].values,
   "Rejected genuine unchanged-flag transition partially wrote native traits");
  owner->require_current();
 }

 // Natural cached GetHappiness minus fresh GetUnhappiness oracle. A saved
 // excess value is not the expected result and is never used as the oracle.
 {
  const auto allocation=preview.session().read_state_lease();const auto generation=preview.session().state_generation();
  const auto old_positive=allocation->unhappiness.players.at(0).happiness;
  std::map<std::int64_t,runtime::BnwUnhappinessQuote> expected;
  preview.with_owned_host_camera_input([&](auto h,const auto*){
   const auto rules=runtime::load_bnw_unhappiness_rules(h->database());
   for(const auto& [id,p]:allocation->cities.players){
    // The test has no issuer authority for the private dense specialist owner.
    // Still execute production for these players; never count absent oracle
    // as production coverage. Other natural rows retain the existing oracle.
    if(!allocation->unhappiness.players.at(id).half_specialists||p.cities.empty())
     expected.emplace(id,runtime::detail::bnw_native_unhappiness_math(allocation->unhappiness,allocation->setup,allocation->cities,rules,h->defines(),id));
   }
  });
  const auto result=owner->refresh_cached_happiness_getter_projection();
  check(preview.session().read_state_lease()==allocation&&preview.session().state_generation()==generation,"Happiness getter refresh replaced actual live world");
  check(!expected.empty()&&result.size()==allocation->cities.players.size()&&preview.session().bnw_unhappiness_current(),"Happiness getter cache not current for complete roster");
  for(const auto& [id,q]:expected){check(result.at(id).total==q.total&&result.at(id).excess_happiness==q.excess_happiness,
   "Actual native happiness getter differs from existing native arithmetic oracle");
   check(allocation->cities.players.at(id).excess_happiness==q.excess_happiness,"SAMEstate cached excess store missing");
   check(q.excess_happiness==allocation->unhappiness.players.at(id).happiness-q.total,"Getter incorrectly used saved excess happiness");}
  check(allocation->unhappiness.players.at(0).happiness==old_positive,"Getter refresh falsely recalculated positive happiness");
  owner->require_current();
 }

 // Two actual factory Hosts never share a mutable world. An actual mutation of
 // B does not retire A; mutating A invalidates A even at unchanged allocation.
 {
  auto second=ui::bootstrap_selected_bnw_preview(input,root,0);
  check(second.preview.host&&!second.preview.blocker,"Second genuine BNW Host unavailable");
  ui::SelectedBnwReadOnlyPreview sibling(std::move(second.preview));
  auto child=runtime::BnwMajorPlayerTraitsOwner::from_selected_preview(sibling,0);
  sibling.with_owned_host_camera_input([&](auto h,const auto*){
   auto& state=const_cast<ui::SelectedBnwPreviewHost&>(*h).session().mutable_state();
   const auto current=state.unhappiness.players.at(0).trait_population_modifier;
   check(current<INT64_MAX,"Natural stale mirror test operand overflow");
   state.unhappiness.players.at(0).trait_population_modifier=current+1;
  });
  const auto excess=sibling.session().state().cities.players.at(0).excess_happiness;
  refuses([&]{child->refresh_cached_happiness_getter_projection();});
  check(sibling.session().state().cities.players.at(0).excess_happiness==excess,"Rejected stale happiness getter partially stored output");
  refuses([&]{child->require_current();});owner->require_current();
 }
 preview.with_owned_host_camera_input([&](auto h,const auto*){(void)const_cast<ui::SelectedBnwPreviewHost&>(*h).session().mutable_state();});
 refuses([&]{(void)owner->snapshot();});
 check(!preview.session().readiness().can_continue(),"Trait owner falsely closed full BNW readiness");
 std::cout<<"PASS natural BNW known Traits domains+resource owner and imported opaque enum; isolated live Reset/Init positive tech transition requires genuine supplier; Lua/full turn unavailable\n";
}
}
int main(int argc,char** argv){try{check(argc==3,"Supply natural Gandhi BNW save and game root");natural(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL BNW Traits owner: "<<e.what()<<'\n';return 1;}}
