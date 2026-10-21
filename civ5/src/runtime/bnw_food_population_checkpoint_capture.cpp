#include "civ5/runtime/bnw_food_population_checkpoint_capture.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <utility>
namespace civ5::runtime {
BnwFoodPopulationCheckpointCapture::BnwFoodPopulationCheckpointCapture(
 std::shared_ptr<const BnwCityFoodGrowthPending> p,std::shared_ptr<const BnwCityPopulationPending> c,
 std::shared_ptr<const ui::SelectedBnwPreviewHost> h,BnwPopulationTotalsCheckpointBundle b,saves::CivisSaveSection clock,
 std::vector<BnwFoodPopulationStoreRecord> s,SessionCheckpointIdentity i,
 BnwCityFoodGrowthProgress f,BnwCityPopulationProgress cp,std::uint64_t g,std::uint64_t r,
 std::int64_t t,bool a,SimulationRandomState mr,SimulationRandomState gr):
 parent_(std::move(p)),child_(std::move(c)),host_(std::move(h)),components_(std::move(b)),clock_(std::move(clock)),
 stores_(std::move(s)),identity_(std::move(i)),food_(f),population_(cp),generation_(g),revision_(r),
 turn_(t),sent_auto_moves_(a),map_rng_(mr),game_rng_(gr){}
BnwFoodPopulationCheckpointCapture::~BnwFoodPopulationCheckpointCapture()=default;
void BnwFoodPopulationCheckpointCapture::require_current()const{
 if(thread_!=std::this_thread::get_id())throw BnwCityFoodGrowthUnavailable("Food checkpoint capture foreign creator thread");
 // Retained real parent/child/Host precede callbacks. Final canonical/once,
 // semantic history and companion checks are repeated after those callbacks.
 parent_->require_current();parent_->require_population_capture_current_without_callbacks(*this);
}
const BnwPopulationTotalsCheckpointBundle& BnwFoodPopulationCheckpointCapture::components()const{
 require_current();return components_;
}
std::span<const BnwFoodPopulationStoreRecord> BnwFoodPopulationCheckpointCapture::ordered_stores()const{
 require_current();return stores_;
}
}
