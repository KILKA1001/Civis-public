#pragma once
#include "civ5/runtime/bnw_food_population_checkpoint_capture.hpp"
#include <array>
#include <string>
#include <string_view>
namespace civ5::runtime {
// Reserved by Root. This journal covers only an earned growth(+1) return
// before the first Plot.UpdateYield call; it does not cover a city/player turn.
inline constexpr std::uint32_t bnw_food_growth_journal_section_id=47;
class BnwFoodPopulationGrowthCheckpoint final {
 public:
 static saves::CivisSaveSection make(const BnwFoodPopulationCheckpointCapture&);
 static saves::CivisSaveDocument document(const BnwFoodPopulationCheckpointCapture&,
     std::string_view original_source_sha256);
 private:
 friend class ui::SelectedBnwPreviewHost;friend class BnwCityFoodGrowthPending;
 struct Record {
  SessionCheckpointIdentity identity;
  std::int32_t actor=-1,city=-1,difference=0,threshold=0;
  std::uint64_t first_revision=0,revision=0;
  std::int64_t turn=0;
  SimulationRandomState map_rng,game_rng;
  std::array<std::string,8> companions;
  std::vector<BnwFoodPopulationStoreRecord> stores;
 };
 static Record decode(const saves::CivisSaveSection&);
 static saves::CivisSaveSection encode(const Record&);
 static void validate_growth(const Record&,const saves::SavedSessionSnapshot&,
     const BnwPopulationTotalsCheckpointBundle&,std::int32_t,std::int32_t);
 friend void validate_bnw_food_growth_journal_shape(const saves::CivisSaveSection&);
 friend void validate_bnw_food_growth_journal_binding(const saves::CivisSaveSection&,
     const SessionCheckpointIdentity&);
 friend void validate_bnw_food_growth_journal_companions(const saves::CivisSaveDocument&);
};
void validate_bnw_food_growth_journal_shape(const saves::CivisSaveSection&);
void validate_bnw_food_growth_journal_binding(const saves::CivisSaveSection&,
    const SessionCheckpointIdentity&);
void validate_bnw_food_growth_journal_companions(const saves::CivisSaveDocument&);
}
