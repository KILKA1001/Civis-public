#include "civ5/runtime/bnw_food_growth_import_admission.hpp"
#include "civ5/saves/loaded_save.hpp"
namespace civ5::runtime {
BnwFoodGrowthImportAdmission::BnwFoodGrowthImportAdmission(const saves::LoadedSave& in,const SessionCheckpointIdentity& identity):identity_(identity){
 const auto part=[](std::uint32_t id,const auto& bytes){if(!bytes)throw BnwCityFoodGrowthUnavailable("Food growth47 missing mandatory imported companion");return saves::CivisSaveSection{id,0,*bytes};};
 if(in.container!=saves::SaveContainerKind::civis||!in.source_sha256||!in.source_prefix||in.source_prefix->empty())throw BnwCityFoodGrowthUnavailable("Food growth47 requires genuine OriginalPayload framing");
 journal_=part(47,in.live_bnw_food_growth_journal);clock_=part(16,in.live_bnw_game_clock);random_=part(13,in.live_simulation_random);
 bundle_.totals=part(46,in.live_bnw_population_totals);bundle_.players=part(24,in.live_bnw_player_core);
 bundle_.cities={part(25,in.live_bnw_city_core),part(26,in.live_bnw_city_production),part(27,in.live_bnw_city_management),part(28,in.live_bnw_city_tail)};
 validate_bnw_food_growth_journal_binding(journal_,identity);
 const auto doc=saves::make_civis_import_document(in,*in.source_sha256);validate_bnw_food_growth_journal_companions(doc);
 for(const auto& s:doc.sections){const auto id=s.id;if(!(id==1||id==4||id==5||id==6||id==42||id==13||id==16||(id>=24&&id<=28)||id==46||id==47))throw BnwCityFoodGrowthUnavailable("Food growth47 cannot stage another mutable/history section");}
}
}
