#include "civ5/runtime/bnw_city_current_production_rate.hpp"
#include "civ5/runtime/bnw_city_current_science_rate.hpp"
#include <utility>
namespace civ5::runtime {
namespace {
template<class F>auto typed_production(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityCurrentProductionRateUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityCurrentProductionRateUnavailable(e.what());}
}
}
struct BnwCityCurrentProductionRate::Impl {
 BnwCityCurrentScienceRate source;
 explicit Impl(BnwCityCurrentScienceRate s):source(std::move(s)){}
};
BnwCityCurrentProductionRate::BnwCityCurrentProductionRate(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
std::int32_t BnwCityCurrentProductionRate::value_times100()const{return typed_production([&]{
 const auto p=impl_;if(!p)throw BnwCityCurrentProductionRateUnavailable("Current PRODUCTION moved owner");return p->source.value_times100();
});}
std::vector<std::int64_t> BnwCityCurrentProductionRate::private_source_seal()const{return typed_production([&]{
 const auto p=impl_;if(!p)throw BnwCityCurrentProductionRateUnavailable("Current PRODUCTION moved owner");return p->source.current_source_seal();
});}
BnwCityCurrentProductionRate inspect_bnw_city_current_production_rate(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed_production([&]{
 // Fixed private mode; caller never supplies an index, rate, grant or second DB.
 auto source=BnwCityCurrentScienceRate::bind_current_source(std::move(host),owner,city,BnwCityCurrentScienceRate::SourceKind::production);
 return BnwCityCurrentProductionRate(std::make_shared<BnwCityCurrentProductionRate::Impl>(std::move(source)));
});}
}
