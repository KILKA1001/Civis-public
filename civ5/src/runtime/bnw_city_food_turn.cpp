#include "civ5/runtime/bnw_city_food_turn.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include <climits>
#include <string_view>
namespace civ5::runtime {
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::int32_t native(std::int64_t n){
 need(n>=INT32_MIN&&n<=INT32_MAX,"BNW city food-turn native int width differs");
 return static_cast<std::int32_t>(n);
}
std::int32_t scalar(const saves::SavedCityObservation& city,std::string_view name){
 const saves::CityCoreField* found=nullptr;
 for(const auto& f:city.core.fields)if(f.name&&std::string_view(f.name)==name){
  need(!found,"BNW city food-turn duplicate raw field");found=&f;
 }
 need(found&&found->encoding==saves::CityCoreEncoding::i32&&
      found->values.size()==1&&!found->wire_count&&found->bytes.empty()&&
      found->hashes.empty()&&found->row_counts.empty()&&found->row_keys.empty(),
      "BNW city food-turn raw scalar shape unavailable");
 return native(found->values.front());
}
}
BnwCityFoodTurnUnavailable::BnwCityFoodTurnUnavailable():std::runtime_error(frontier()){}
BnwCityFoodTurnStep apply_bnw_city_food_turn(
 LiveSessionState& state,std::int64_t owner,std::int64_t city){
 // No effect/callback occurs here. Enclosing LiveSession owns thread/lifetime
 // and predecessor order; this detached component issues no turn capability.
 const auto* raw=resolve_bnw_current_city(state,native(owner),native(city));
 need(raw,"BNW city food-turn current city absent");
 const auto actual_id=scalar(*raw,"m_iID");
 const auto& owned=state.cities.players.at(owner).cities.at(actual_id);
 const auto turns=scalar(*raw,"m_iRazingTurns");
 need(owned.razing==(turns>0),"BNW city food-turn raw/owned razing predicate stale");
 // CvCity::IsRazing is GetRazingTurns()>0, not !=0. Return before reading food,
 // population, yields, order, GC, notifications or any later growth operand.
 if(turns>0)return BnwCityFoodTurnStep(owner,actual_id,turns);
 // Even a saved zero, population-one bottom clamp, or food-unit result does
 // not bypass the mandatory foodDifference reads that precede that clamp.
 throw BnwCityFoodTurnUnavailable();
}
}
