#include "civ5/runtime/bnw_city_current_project_production_difference.hpp"
#include "civ5/runtime/bnw_city_production_turn.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <utility>
#include <stdexcept>
namespace civ5::runtime {
struct BnwCityCurrentProjectProductionDifference::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<BnwCityProductionTurnPending> source;
};
BnwCityCurrentProjectProductionDifference::BnwCityCurrentProjectProductionDifference(std::shared_ptr<Impl> p):impl_(std::move(p)){}
std::int32_t BnwCityCurrentProjectProductionDifference::value_times100()const{
 if(!impl_||!impl_->source)throw std::runtime_error("Current ProjectDifference moved source");
 impl_->source->require_current();return impl_->source->private_current_difference();
}
BnwCityCurrentProjectProductionDifference BnwCityCurrentProjectProductionDifference::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){
 if(!host)throw std::runtime_error("Current ProjectDifference actual Host absent");
 auto p=std::make_shared<BnwCityCurrentProjectProductionDifference::Impl>();p->host=std::move(host);
 p->source=BnwCityProductionTurnPending::prepare_project_read(p->host->session(),p->host,owner,city);
 return BnwCityCurrentProjectProductionDifference(std::move(p));
}
BnwCityCurrentProjectProductionDifference inspect_bnw_city_current_project_production_difference(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return BnwCityCurrentProjectProductionDifference::bind(std::move(host),owner,city);}

}
