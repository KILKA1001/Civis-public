#include "civ5/runtime/improvement_yields.hpp"
#include <stdexcept>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Improvement yield ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t index(std::int64_t id){if(id<0)throw std::runtime_error("Negative improvement yield relation index");return id;}
content::DatabaseValue parameter(const content::RulesRowView &row){
    const auto type=row.text("Type");return type?content::DatabaseValue(std::string(type->c_str())):content::DatabaseValue{};
}
void assign(YieldValues &values,std::int64_t key,std::int64_t n){
    index(key);if(n)values[key]=n;else values.erase(key);
}
}
ImprovementYieldRules load_improvement_yield_rules(const content::RulesDatabase &db,TechnologyProfile profile){
    if(profile!=TechnologyProfile::bnw&&profile!=TechnologyProfile::vp542)throw std::runtime_error("Unknown improvement yield profile");
    ImprovementYieldRules result;result.profile=profile;const bool vp=profile==TechnologyProfile::vp542;
    const auto collection=[&](const std::string &table,const auto &visit){
        const auto r=db.prefetch_info(table,visit);if(!r.all_succeeded())throw std::runtime_error("Improvement yield Info: "+r.error);
        i64(r.slots);return r.slots;
    };
    const auto read=[&](content::DatabaseStatement statement,const auto &visit){
        const auto r=db.query_rows(statement,[&](const auto &row){++result.relation_rows;visit(row);});
        if(!r.all_succeeded())throw std::runtime_error("Improvement yield relation: "+r.error);
    };
    collection("Yields",[&](auto slot,auto,const auto &){result.yields.push_back(i64(slot));return true;});
    result.technology_slots=collection("Technologies",[](auto,auto,const auto &){return true;});
    const auto simple=[&](const std::string &table,const std::string &filter,const auto &param,YieldValues &values){
        read({"select Yields.ID, Yield from "+table+" inner join Yields on YieldType = Yields.Type where "+filter+" = ?",{param}},
            [&](const auto &row){assign(values,row.integer_at(0),row.integer_at(1));});
    };
    result.route_slots=collection("Routes",[&](auto slot,auto,const auto &row){
        auto &route=result.routes[i64(slot)];if(vp)simple("Route_Yields","RouteType",parameter(row),route);return true;
    });
    result.policy_slots=collection("Policies",[&](auto slot,auto,const auto &row){
        auto &policy=result.policies[i64(slot)];
        read({"select Improvements.ID, Yields.ID, Yield from Policy_ImprovementYieldChanges inner join Improvements on Improvements.Type = ImprovementType inner join Yields on Yields.Type = YieldType where PolicyType = ?",{parameter(row)}},
            [&](const auto &linked){assign(policy[index(linked.integer_at(0))],linked.integer_at(1),linked.integer_at(2));});
        return true;
    });
    result.improvement_slots=collection("Improvements",[&](auto slot,auto authored,const auto &row){
        ImprovementYieldInfo info;info.authored_id=authored;const auto param=parameter(row);
        constexpr const char *tables[]={"Improvement_Yields","Improvement_YieldPerEra","Improvement_AdjacentCityYields",
            "Improvement_AdjacentMountainYieldChanges","Improvement_CoastalLandYields","Improvement_FreshWaterYields",
            "Improvement_HillsYields","Improvement_RiverSideYields","Improvement_WLTKDYields","Improvement_GoldenAgeYields"};
        for(std::size_t i=0;i<(vp?10u:8u);++i)simple(tables[i],"ImprovementType",param,info.yields[i]);
        const auto matrix=[&](const char *table,const std::string &types,const std::string &column,ImprovementYieldMatrix &values){
            read({"select Yields.ID, "+types+".ID, Yield from "+table+" inner join Yields on YieldType = Yields.Type inner join "+types+
                " on "+column+" = "+types+".Type where ImprovementType = ?",{param}},
                [&](const auto &linked){assign(values[index(linked.integer_at(1))],linked.integer_at(0),linked.integer_at(2));});
        };
        matrix("Improvement_TechYieldChanges","Technologies","TechType",info.technologies);
        matrix("Improvement_TechFreshWaterYieldChanges","Technologies","TechType",info.fresh_technologies);
        matrix("Improvement_TechNoFreshWaterYieldChanges","Technologies","TechType",info.dry_technologies);
        matrix("Improvement_RouteYieldChanges","Routes","RouteType",info.routes);
        // Resource yields are loaded ONLY for matching Improvement_ResourceTypes
        // rows. An orphan yield relation must not create a resource bonus.
        read({"select Resources.ID, ResourceType from Improvement_ResourceTypes inner join Resources on ResourceType = Resources.Type where ImprovementType = ?",{param}},
            [&](const auto &resource){
                auto &values=info.resources[index(resource.integer_at(0))];values.clear();
                const auto name=resource.text("ResourceType");
                const content::DatabaseValue resource_param=name?content::DatabaseValue(std::string(name->c_str())):content::DatabaseValue{};
                read({"select Yields.ID, Yield from Improvement_ResourceType_Yields inner join Yields on YieldType = Yields.Type where ImprovementType = ? and ResourceType = ?",{param,resource_param}},
                    [&](const auto &linked){assign(values,linked.integer_at(0),linked.integer_at(1));});
            });
        if(vp){
            matrix("Improvement_FeatureYieldChanges","Features","FeatureType",info.features);
            matrix("Improvement_AdjacentFeatureYieldChanges","Features","FeatureType",info.adjacent_features);
            matrix("Improvement_AdjacentResourceYieldChanges","Resources","ResourceType",info.adjacent_resources);
            matrix("Improvement_AdjacentTerrainYieldChanges","Terrains","TerrainType",info.adjacent_terrains);
            const auto culture=row.integer("CultureAdjacentSameType");
            if(culture>0)info.fractional_improvements[4][authored]+=YieldFraction(culture);
            const auto fractions=[&](const char *table,const std::string &types,const std::string &column,ImprovementFractionMatrix &values){
                read({"select Yields.ID, "+types+".ID, Yield, NumRequired from "+table+" inner join Yields on YieldType = Yields.Type inner join "+types+
                    " on "+column+" = "+types+".Type where ImprovementType = ?",{param}},
                    [&](const auto &linked){values[index(linked.integer_at(0))][index(linked.integer_at(1))]+=YieldFraction(linked.integer_at(2),linked.integer_at(3));});
            };
            fractions("Improvement_YieldPerXAdjacentImprovement","Improvements","OtherImprovementType",info.fractional_improvements);
            fractions("Improvement_YieldPerXAdjacentTerrain","Terrains","TerrainType",info.fractional_terrains);
        }
        result.improvements.emplace(i64(slot),std::move(info));return true;
    });
    return result;
}
std::int64_t ImprovementAdjacencyYield::combined()const{auto total=improvements;total+=terrains;return total.truncate();}
ImprovementAdjacencyYield improvement_fractional_adjacency(const saves::SavedMapProjection &map,
    const ImprovementYieldRules &rules,const content::InfoTypeCatalog &types,std::int64_t id,
    std::int64_t improvement,std::int64_t yield){
    if(rules.profile!=TechnologyProfile::vp542)throw std::runtime_error("Fractional improvement adjacency is a VP rule");
    const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,saves::GameStateLayout::vp542_corpus};geometry.validate();
    if(id<0||static_cast<std::uint64_t>(id)>=map.cells.size()||map.cells.size()!=map.grid.size()||!types.report().all_succeeded())
        throw std::runtime_error("Invalid improvement adjacency map or types");
    const auto &info=rules.improvements.at(improvement);const auto xy=map.grid.coordinates(id);
    const auto improvements=info.fractional_improvements.find(yield),terrains=info.fractional_terrains.find(yield);
    ImprovementAdjacencyYield result;
    for(int d=0;d<6;++d)if(const auto n=geometry.direction(i64(xy[0]),i64(xy[1]),d)){
        const auto &p=map.cells.at(*n);const auto expected=map.grid.coordinates(*n);
        if(p.index!=static_cast<std::uint64_t>(*n)||p.x!=expected[0]||p.y!=expected[1])throw std::runtime_error("Improvement adjacency cell identity mismatch");
        if(improvements!=info.fractional_improvements.end()){
            if(!p.improvement_pillaged)throw std::runtime_error("Unresolved adjacent improvement pillage");
            if(!*p.improvement_pillaged){
                std::int64_t adjacent=-1;
                if(p.improvement_type&&p.improvement_type_hash)throw std::runtime_error("Ambiguous adjacent improvement");
                if(p.improvement_type)adjacent=*p.improvement_type;
                else if(p.improvement_type_hash){if(*p.improvement_type_hash){
                    const auto *type=types.find_hash(*p.improvement_type_hash);if(!type)throw std::runtime_error("Unknown adjacent improvement hash");adjacent=type->id;
                }}else throw std::runtime_error("Unresolved adjacent improvement");
                if(adjacent!=-1){const auto bonus=improvements->second.find(adjacent);if(bonus!=improvements->second.end())result.improvements+=bonus->second;}
            }
        }
        if(terrains!=info.fractional_terrains.end()){
            if(!p.terrain_type)throw std::runtime_error("Unresolved adjacent terrain");
            const auto bonus=terrains->second.find(*p.terrain_type);if(bonus!=terrains->second.end())result.terrains+=bonus->second;
        }
    }
    return result;
}
} // namespace civ5::runtime
