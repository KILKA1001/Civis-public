#pragma once
#include "civ5/runtime/vp_great_works.hpp"
#include "civ5/runtime/vp_corporation_profile.hpp"

namespace civ5::runtime {
struct VpCorporationRules {
    struct Corporation {
        std::int64_t headquarters=-1,office=-1,franchise=-1;
        YieldValues trade_city;
    };
    std::map<std::int64_t,Corporation> corporations;
    std::set<std::int64_t> yields;
};
VpCorporationRules load_vp_corporation_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_corporations(const saves::SavedSessionSnapshot&,CityYieldContext&);
struct VpCorporationContext {
    const VpCorporationRules &rules;
    const VpGreatWorkContext &buildings;
};
const saves::CorporationObservation *vp_active_corporation(const CityYieldContext&,std::int64_t corporation);
// Source getPlotCity over the shared map: city flag, owning-city handle, live lookup.
// Trade route city IDs and destination owner do not participate in this lookup.
const CityYieldState *vp_city_at(const CityYieldContext&,const saves::SavedMapProjection&,std::int64_t x,std::int64_t y);
bool vp_city_is_headquarters(const VpCorporationContext&,const CityYieldState&);
bool vp_city_has_office(const VpCorporationContext&,const CityYieldState&);
bool vp_city_has_franchise(const VpCorporationContext&,const CityYieldState&,std::int64_t corporation);
// Complete GetTradeRouteCityMod. Signed percentage; no final positivity clamp.
std::int64_t vp_city_corporation_trade_modifier(const VpCorporationContext&,const CityYieldState&,std::int64_t yield);
} // namespace civ5::runtime
