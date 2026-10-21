#include "civ5/runtime/bnw_player_happiness_prefix_owner.hpp"
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f,std::string_view expected){try{f();}catch(const std::exception& e){
 need(std::string_view(e.what())==expected,"Refusal reached a different source branch");return;}
 throw std::runtime_error("Unsupported native input succeeded");}
auto& field(auto& rows,std::string_view name){for(auto& f:rows)if(f.name&&std::string_view(f.name)==name)return f;
 throw std::runtime_error("Actual native field absent");}
void math(const LiveSessionState& actual,const BnwHappinessPrefixRules& rules,
 const SessionCheckpointIdentity& identity,std::int64_t player){
 using Stage=BnwHappinessNativeStore::Stage;
 const auto first=detail::bnw_happiness_prefix_stage_math(actual,rules,identity,player,Stage::handicap);
 need(first.happiness==first.operand&&first.operand==rules.default_happiness(actual.setup.players.at(player).handicap),
  "Source first assignment differs from reached Handicap getter");
 // These are detached arithmetic inputs only. They cannot issue the opaque owner.
 // Damage to future resource/building/policy data must not precede the first store.
 auto detached=actual;auto& raw=detached.world.all_players->players.at(static_cast<std::size_t>(player));
 raw.cities.cities.clear();raw.cities.header.wire_city_count=INT64_MAX;
 raw.strategy.blocks.clear();detached.cities.players.at(player).cities.clear();
 const auto skipped=detail::bnw_happiness_prefix_stage_math(detached,rules,identity,player,Stage::handicap);
 need(skipped.happiness==first.happiness,"Unreached numeric branch blocked Handicap assignment");
 refuses([&]{(void)detail::bnw_happiness_prefix_stage_math(detached,rules,identity,player,Stage::per_city);},
  "BNW happiness actual city count mirror stale");
 detached=actual;
 field(detached.world.all_players->players.at(static_cast<std::size_t>(player)).core.fields,"m_iHappinessPerCity").values={INT64_MAX};
 refuses([&]{(void)detail::bnw_happiness_prefix_stage_math(detached,rules,identity,player,Stage::per_city);},
  "BNW happiness cache exceeds native int");
 // Invalid stage must refuse before touching a world or loading future operands.
 auto absent=actual;absent.world={};
 refuses([&]{(void)detail::bnw_happiness_prefix_stage_math(absent,rules,identity,player,Stage::religion);},
  "BNW happiness native prefix stage unavailable");
 detached=actual;field(detached.world.all_players->players.at(static_cast<std::size_t>(player)).core.fields,"m_iHappiness").values.front()+=1;
 refuses([&]{(void)detail::bnw_happiness_prefix_stage_math(detached,rules,identity,player,Stage::handicap);},
  "BNW happiness native stage raw/owned echo stale");
 // Detached arithmetic variants only. Zero cities must not reach even policy
 // metadata, civilization mapping or garrison/religious city getters.
 detached=actual;auto& none=detached.world.all_players->players.at(static_cast<std::size_t>(player));
 none.cities.cities.clear();none.cities.header.wire_city_count=0;none.strategy.blocks.clear();
 detached.cities.players.at(player).cities.clear();
 detached.world.pregame->slots.integer_arrays.at("civilizations").at(static_cast<std::size_t>(player))=INT64_MAX;
 need(detail::bnw_happiness_buildings_reached_math(detached,rules.buildings(),identity,player,
  detail::BnwBuildingHappinessReachedStage::local_cities)==0,"Native zero cities queried a later operand");
 // A controlled current-Read arithmetic city reuses its actual identity/counts.
 // Clear religion/policies and garrison as declared pure-math inputs, never an issuer.
 detached=actual;auto& rawp=detached.world.all_players->players.at(static_cast<std::size_t>(player));
 if(rawp.cities.cities.empty()){std::cout<<"Nonzero local math source UNOBSERVED (no actual actor city)\n";return;}
 const auto first_city=rawp.cities.cities.front();rawp.cities.cities={first_city};rawp.cities.header.wire_city_count=1;
 const auto cid=field(rawp.cities.cities.front().core.fields,"m_iID").values.front();
 auto owned_city=detached.cities.players.at(player).cities.at(cid);
 detached.cities.players.at(player).cities.clear();detached.cities.players.at(player).cities.emplace(cid,std::move(owned_city));
 auto& city=rawp.cities.cities.front();auto& owned=detached.cities.players.at(player).cities.at(cid);
 auto& rel=field(city.tail.blocks,"religions");auto& rows=field(rel.children,"m_ReligionStatus");rows.children.clear();rows.wire_count=0;
 owned.religions.clear();field(rawp.core.fields,"m_iHappinessPerGarrisonedUnitCount").values={0};
 auto& policies=field(rawp.strategy.blocks,"policies");
 for(auto& array:policies.hashes)if(array.name&&std::string_view(array.name)=="has_policy")
  for(auto& row:array.rows)if(row.type_hash)row.values={0};
 detached.setup.players.at(player).policies.clear();detached.setup.players.at(player).policies_resolved=true;
 auto& cache=field(city.production.blocks,"before_strategy");
 field(cache.fields,"m_iBaseHappinessFromBuildings").value=1;
 field(cache.fields,"m_iUnmoddedHappinessFromBuildings").value=INT64_MAX;
 field(rawp.core.fields,"m_iHappinessPerXPolicies").values={INT64_MAX};
 field(rawp.core.fields,"m_iHappinessPerXPopulation").values={INT64_MAX};
 const auto local=detail::bnw_happiness_buildings_reached_math(detached,rules.buildings(),identity,player,
  detail::BnwBuildingHappinessReachedStage::local_cities);
 need(local>=0&&local<=1,"Local native result differs from actual population cap");
 refuses([&]{(void)quote_bnw_happiness_from_buildings(detached,rules.buildings(),identity,player);},
  "BNW policy happiness exceeds native int");
 // Only the global scope reaches this malformed unmodded cache. The actual
 // private owner cannot be issued from these supplied detached inputs.
 refuses([&]{(void)detail::bnw_happiness_buildings_reached_math(detached,rules.buildings(),identity,player,
  detail::BnwBuildingHappinessReachedStage::global_buildings);},"BNW building happiness exceeds native int");
 field(cache.fields,"m_iUnmoddedHappinessFromBuildings").value=2;
 field(rawp.core.fields,"m_iHappinessPerXPolicies").values={0};
 const auto global=detail::bnw_happiness_buildings_reached_math(detached,rules.buildings(),identity,player,
  detail::BnwBuildingHappinessReachedStage::global_buildings);
 rawp.strategy.blocks.clear();field(rawp.core.fields,"m_iHappinessPerGarrisonedUnitCount").values={INT64_MAX};
 need(detail::bnw_happiness_buildings_reached_math(detached,rules.buildings(),identity,player,
  detail::BnwBuildingHappinessReachedStage::global_buildings)==global,
  "Global stage evaluated unneeded local garrison/policy happiness operands");

}
}
static_assert(!std::is_default_constructible_v<BnwPlayerHappinessPrefixOwner>);
static_assert(!std::is_copy_constructible_v<BnwPlayerHappinessPrefixOwner>);
int main(int argc,char** argv){try{
 need(argc==3,"Expected SAVE and canonical shared installation");const auto save_sha=identify_file(argv[1]).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 need(bool(boot.preview.host),"Actual SAME Host absent");const auto& host=*boot.preview.host;
 const auto rules=BnwHappinessPrefixRules::load(host.database());
 math(host.session().state(),rules,host.identity(),host.active_player_id());
 need(!BnwCityEntryPending::full_city_turn_ready()&&!BnwCityEntryPending::growth_ready(),"Numeric test granted later phases");
 need(identify_file(argv[1]).sha256==save_sha,"Original save changed");
 std::cout<<"Player happiness reached-stage math passed; genuine Religion->Player writes are tested by Citizens; positive scene UNOBSERVED until that branch runs\n";
 return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
