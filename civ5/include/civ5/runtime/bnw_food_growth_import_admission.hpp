#pragma once
#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
namespace civ5::saves {struct LoadedSave;struct SavedSession;}
namespace civ5::runtime {
struct LiveSessionState;
// A private actual import staging owner. Constructor access is exclusively
// LiveSession::restore_bnw; a generic DTO restore cannot create this owner.
class BnwFoodGrowthImportAdmission final {
 friend class LiveSession;friend class BnwCityFoodGrowthPending;
 explicit BnwFoodGrowthImportAdmission(const saves::LoadedSave&,const SessionCheckpointIdentity&);
 const std::thread::id creator_=std::this_thread::get_id();
 const SessionCheckpointIdentity identity_;
 saves::CivisSaveSection journal_,clock_,random_;
 BnwPopulationTotalsCheckpointBundle bundle_;
 std::weak_ptr<const saves::SavedSession> source_;
 std::weak_ptr<const LiveSessionState> state_;
 std::uint64_t generation_=0;
 bool attempted_=false,restored_=false,retired_=false;
};
}
