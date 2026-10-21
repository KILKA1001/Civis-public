#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <thread>
namespace civ5::runtime {
class BnwCityFoodConsumptionDiagnostic;
class BnwCitySpecialistCounts;
namespace detail {
BnwCityFoodConsumptionDiagnostic diagnose_bnw_city_food_consumption_source(
 const saves::SavedSessionSnapshot&,const content::RulesDatabase&,
 const content::GlobalDefineCache&,std::int64_t,std::int64_t,std::int64_t,const BnwCitySpecialistCounts*);
}
// Read-only source component. SQL/define snapshots are explicitly diagnostic,
// not native GC cache lifecycle or refreshed city-yield authority.
BnwCityFoodConsumptionDiagnostic diagnose_bnw_city_food_consumption(
 const saves::SavedSessionSnapshot&,const content::RulesDatabase&,
 const content::GlobalDefineCache&,std::int64_t,std::int64_t,std::int64_t extra=0);
class BnwCityFoodConsumptionDiagnostic final {
 public:
  BnwCityFoodConsumptionDiagnostic(const BnwCityFoodConsumptionDiagnostic&)=delete;
  BnwCityFoodConsumptionDiagnostic& operator=(const BnwCityFoodConsumptionDiagnostic&)=delete;
  BnwCityFoodConsumptionDiagnostic(BnwCityFoodConsumptionDiagnostic&&)noexcept=default;
  std::int32_t consumption()const noexcept{return total_;}
  std::int32_t population_with_extra()const noexcept{return population_extra_;}
  std::int32_t specialist_total()const noexcept{return specialists_;}
  std::int32_t reduction()const noexcept{return reduction_;}
  bool reads_specialists()const noexcept{return half_>0;}
  void require_same_raw_inputs(const saves::SavedSessionSnapshot&)const;
  static constexpr bool native_gc_cache_owned()noexcept{return false;}
  static constexpr bool full_growth_ready()noexcept{return false;}
  static constexpr const char* frontier()noexcept{return "actual GC FOOD_CONSUMPTION_PER_POPULATION / DEFAULT_SPECIALIST / SpecialistInfos lifecycle and refreshed city food yields before foodDifference/doGrowth";}
 private:
  friend BnwCityFoodConsumptionDiagnostic detail::diagnose_bnw_city_food_consumption_source(
   const saves::SavedSessionSnapshot&,const content::RulesDatabase&,
   const content::GlobalDefineCache&,std::int64_t,std::int64_t,std::int64_t,const BnwCitySpecialistCounts*);
  friend BnwCityFoodConsumptionDiagnostic diagnose_bnw_city_food_consumption(
   const saves::SavedSessionSnapshot&,const content::RulesDatabase&,
   const content::GlobalDefineCache&,std::int64_t,std::int64_t,std::int64_t);
  BnwCityFoodConsumptionDiagnostic()=default;
  std::thread::id thread_=std::this_thread::get_id();
  std::int64_t owner_=-1,city_=-1;
  std::int32_t population_=0,half_=0,population_extra_=0,specialists_=0,reduction_=0,total_=0;
  std::pair<std::uint64_t,std::uint64_t> player_span_,city_span_,citizens_span_;
  std::int64_t specialist_wire_count_=0;
  std::vector<saves::TeamHashedValues> raw_specialists_;
};
}
