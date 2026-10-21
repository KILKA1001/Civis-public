#pragma once
#include "civ5/runtime/improvement_yields.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string improvement_yield_report(const saves::SavedMapProjection &map,const content::RulesDatabase &db,
        const content::InfoTypeCatalog &types,content::TechnologyProfile profile){
    const auto rules=runtime::load_improvement_yield_rules(db,profile);
    const bool vp=profile==content::TechnologyProfile::vp542;
    std::ostringstream out;
    const auto pairs=[&](const auto &values){out<<'[';bool comma=false;for(const auto &[id,n]:values){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto matrix=[&](const auto &values){out<<'[';bool comma=false;for(const auto &[id,v]:values){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(v);out<<']';}out<<']';};
    const auto fractions=[&](const auto &values){out<<'[';bool comma=false;for(const auto &[yield,row]:values)for(const auto &[id,f]:row){
        if(comma)out<<',';comma=true;out<<'['<<yield<<','<<id<<','<<f.numerator()<<','<<f.denominator()<<']';}out<<']';};
    out<<"{\"full_improvement_yield\":false,\"full_plot_yield\":false,\"grid\":["<<map.grid.width<<','<<map.grid.height<<','<<map.wrap_x<<','<<map.wrap_y<<']'
       <<",\"improvement_slots\":"<<rules.improvement_slots<<",\"technology_slots\":"<<rules.technology_slots<<",\"route_slots\":"<<rules.route_slots
       <<",\"policy_slots\":"<<rules.policy_slots<<",\"relation_rows\":"<<rules.relation_rows<<",\"rules\":[";
    bool comma=false;
    for(const auto &[id,info]:rules.improvements){
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<id<<",\"authored\":"<<info.authored_id<<",\"yields\":[";
        for(std::size_t i=0;i<info.yields.size();++i){if(i)out<<',';pairs(info.yields[i]);}
        out<<"],\"resources\":";matrix(info.resources);out<<",\"routes\":";matrix(info.routes);
        out<<",\"technologies\":";matrix(info.technologies);out<<",\"fresh_technologies\":";matrix(info.fresh_technologies);
        out<<",\"dry_technologies\":";matrix(info.dry_technologies);out<<",\"features\":";matrix(info.features);
        out<<",\"adjacent_features\":";matrix(info.adjacent_features);out<<",\"adjacent_resources\":";matrix(info.adjacent_resources);
        out<<",\"adjacent_terrains\":";matrix(info.adjacent_terrains);out<<",\"fractional_improvements\":";fractions(info.fractional_improvements);
        out<<",\"fractional_terrains\":";fractions(info.fractional_terrains);out<<'}';
    }
    out<<"],\"route_yields\":";matrix(rules.routes);out<<",\"policies\":[";comma=false;
    for(const auto &[id,p]:rules.policies){if(comma)out<<',';comma=true;out<<'['<<id<<',';matrix(p);out<<']';}
    out<<"],\"source_fractions\":[";comma=false;
    // Separate database rows let the Python verifier reconstruct the fraction
    // cache independently of the runtime loader's stored values.
    if(vp){
        const auto collection=db.prefetch_info("Improvements",[&](auto slot,auto authored,const auto &row){
            const auto emit=[&](int kind,std::int64_t y,std::int64_t other,std::int64_t n,std::int64_t d){
                if(comma)out<<',';comma=true;out<<'['<<slot<<','<<kind<<','<<y<<','<<other<<','<<n<<','<<d<<']';};
            if(const auto culture=row.integer("CultureAdjacentSameType");culture>0)emit(0,4,authored,culture,1);
            const auto type=row.text("Type");
            const content::DatabaseValue param=type?content::DatabaseValue(std::string(type->c_str())):content::DatabaseValue{};
            for(int kind=0;kind<2;++kind){
                const std::string other=kind?"Terrains":"Improvements",field=kind?"TerrainType":"OtherImprovementType";
                const std::string table=kind?"Improvement_YieldPerXAdjacentTerrain":"Improvement_YieldPerXAdjacentImprovement";
                const auto r=db.query_rows({"select Yields.ID, "+other+".ID, Yield, NumRequired from "+table+
                    " inner join Yields on YieldType = Yields.Type inner join "+other+" on "+field+" = "+other+".Type where ImprovementType = ?",{param}},
                    [&](const auto &linked){emit(kind,linked.integer_at(0),linked.integer_at(1),linked.integer_at(2),linked.integer_at(3));});
                if(!r.all_succeeded())throw std::runtime_error("Source fractional diagnostic: "+r.error);
            }
            return true;
        });
        if(!collection.all_succeeded())throw std::runtime_error("Source improvement diagnostic: "+collection.error);
    }
    const auto resolve=[&](const auto &p){
        if(p.improvement_type&&p.improvement_type_hash)throw std::runtime_error("Ambiguous improvement diagnostic ID");
        if(p.improvement_type)return *p.improvement_type;
        if(p.improvement_type_hash){if(!*p.improvement_type_hash)return std::int64_t{-1};if(const auto *t=types.find_hash(*p.improvement_type_hash))return t->id;}
        throw std::runtime_error("Unresolved improvement diagnostic ID");
    };
    out<<"],\"cells\":[";comma=false;
    for(const auto &p:map.cells){
        if(!p.terrain_type||!p.improvement_pillaged)throw std::runtime_error("Unresolved improvement diagnostic cell");
        if(comma)out<<',';comma=true;out<<'['<<*p.terrain_type<<','<<resolve(p)<<','<<*p.improvement_pillaged<<']';
    }
    out<<"],\"yield_ids\":[";comma=false;for(const auto y:rules.yields){if(comma)out<<',';comma=true;out<<y;}
    out<<"],\"cases\":[";comma=false;
    if(vp)for(const auto &p:map.cells){
        const auto improvement=resolve(p);if(improvement==-1)continue;
        for(const auto y:rules.yields){
            const auto r=runtime::improvement_fractional_adjacency(map,rules,types,static_cast<std::int64_t>(p.index),improvement,y);
            if(comma)out<<',';comma=true;
            out<<'['<<p.index<<','<<improvement<<','<<y<<','<<r.improvements.numerator()<<','<<r.improvements.denominator()<<','
               <<r.terrains.numerator()<<','<<r.terrains.denominator()<<','<<r.combined()<<']';
        }
    }
    out<<"]}";return out.str();
}
} // namespace civ5::validation
