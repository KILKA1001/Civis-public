#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/vp_plot_passability_profile.hpp"

namespace civ5::runtime {
namespace {
struct ImpassableInfo {
    std::map<std::uint64_t,bool> values;std::uint64_t span=0;
    bool get(std::int64_t id)const{
        if(id<0||static_cast<std::uint64_t>(id)>=span)throw ScriptError("Unresolved impassability Info ID");
        const auto found=values.find(id);return found!=values.end()&&found->second;
    }
};
ImpassableInfo infos(const content::RulesDatabase &db,const char *table){
    ImpassableInfo result;
    const auto report=db.prefetch_info(table,[&](std::uint64_t slot,std::int64_t,const content::RulesRowView &row){
        result.values.emplace(slot,row.boolean("Impassable"));return true;
    });
    if(!report.all_succeeded())throw ScriptError("Cannot load impassability Info: "+report.error);
    result.span=report.slots;return result;
}
MapGeometry geometry(const saves::SavedMapProjection &map,const PlotPassability &passability){
    return {map.grid,map.wrap_x,map.wrap_y,passability.layout};
}
}
PlotPassability restore_plot_passability(const saves::SavedSessionSnapshot &snapshot,
        const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    if(!snapshot.map_projection||!snapshot.map_plots||snapshot.map_header.layout!=snapshot.map_plots->layout)
        throw ScriptError("Plot passability requires one complete saved map");
    const auto &map=*snapshot.map_projection;const auto &plots=*snapshot.map_plots;
    PlotPassability result;result.layout=plots.layout;geometry(map,result).validate();
    if(map.grid.width!=snapshot.map_header.grid.width||map.grid.height!=snapshot.map_header.grid.height||
       map.wrap_x!=snapshot.map_header.wrap_x||map.wrap_y!=snapshot.map_header.wrap_y||
       map.cells.size()!=map.grid.size()||plots.plots.size()!=map.cells.size())throw ScriptError("Plot passability map mismatch");
    const bool vp=result.layout==saves::GameStateLayout::vp542_corpus;
    const auto terrain=vp?ImpassableInfo{}:infos(db,"Terrains"),features=vp?ImpassableInfo{}:infos(db,"Features");
    if(!vp&&!types.report().all_succeeded())throw ScriptError("Impassability requires a complete type cache");
    for(std::size_t i=0;i<map.cells.size();++i){
        const auto &cell=map.cells[i];const auto &saved=plots.plots[i];const auto xy=map.grid.coordinates(i);
        if(cell.index!=i||cell.x!=xy[0]||cell.y!=xy[1]||saved.index!=i||saved.x!=xy[0]||saved.y!=xy[1])
            throw ScriptError("Plot passability identity mismatch");
        result.plots.emplace_back();auto &value=result.plots.back();
        if(vp){
            restore_vp_plot_passability_flags(result,i,saved,plots.dimensions.teams);
        }else{
            if(!cell.terrain_type)throw ScriptError("Missing plot terrain");
            if(*cell.terrain_type!=-1){
                if(cell.feature_type&&cell.feature_type_hash)throw ScriptError("Ambiguous plot feature identity");
                auto feature=cell.feature_type;
                if(cell.feature_type_hash){
                    if(*cell.feature_type_hash==0)feature=-1;
                    else if(const auto *found=types.find_hash(*cell.feature_type_hash))feature=found->id;
                    else throw ScriptError("Unresolved plot feature hash");
                }
                if(!feature)throw ScriptError("Missing plot feature");
                value.impassable=*feature==-1?terrain.get(*cell.terrain_type):features.get(*feature);
            }
        }
    }
    return result;
}
ScriptMapProvider lua_map_services(const saves::SavedMapProjection &map,const PlotPassability &passability){
    geometry(map,passability).validate();
    if(map.cells.size()!=map.grid.size()||passability.plots.size()!=map.cells.size())throw ScriptError("Lua map state mismatch");
    return {
        [&map,&passability]{geometry(map,passability).validate();return std::pair{static_cast<std::int64_t>(map.grid.width),static_cast<std::int64_t>(map.grid.height)};},
        [&map]{return std::pair{map.wrap_x,map.wrap_y};},
        [&map,&passability](auto x,auto y){return geometry(map,passability).plot(x,y);},
        [&map,&passability](auto i){return geometry(map,passability).plot_by_index(i);},
        [&map,&passability](auto x,auto y,auto dir){return geometry(map,passability).direction(x,y,dir);},
        [&map,&passability](auto x,auto y,auto x2,auto y2){return geometry(map,passability).distance(x,y,x2,y2);},
        [&map,&passability](std::int64_t id,ScriptPlotQuery operation,std::int64_t team)->ScriptValue{
            if(id<0)throw ScriptError("Invalid plot reference");
            const auto &cell=map.cells.at(id);const auto xy=map.grid.coordinates(id);
            if(cell.index!=static_cast<std::uint64_t>(id)||cell.x!=xy[0]||cell.y!=xy[1])throw ScriptError("Shared plot identity changed");
            switch(operation){
                case ScriptPlotQuery::x:return static_cast<std::int64_t>(cell.x);
                case ScriptPlotQuery::y:return static_cast<std::int64_t>(cell.y);
                case ScriptPlotQuery::index:return id;
                case ScriptPlotQuery::water:case ScriptPlotQuery::mountain:
                    if(!cell.plot_type||*cell.plot_type<0||*cell.plot_type>3)throw ScriptError("Unresolved plot type");
                    if(operation==ScriptPlotQuery::mountain&&passability.layout==saves::GameStateLayout::vp542_corpus){
                        if(!cell.terrain_type)throw ScriptError("Unresolved mountain terrain");
                        // VP accepts either its fixed terrain enum or the plot
                        // category. BNW only examines the plot category.
                        return *cell.terrain_type==7||*cell.plot_type==0;
                    }
                    return *cell.plot_type==(operation==ScriptPlotQuery::water?3:0);
                case ScriptPlotQuery::impassable:{
                    const auto &p=passability.plots.at(id);
                    if(passability.layout==saves::GameStateLayout::expansion2_version1||team==-1)return p.impassable;
                    if(team<0)throw ScriptError("Invalid impassability team");
                    return vp_plot_team_impassable(passability,id,team);
                }
            }
            throw ScriptError("Unknown plot query");
        },{passability.layout==saves::GameStateLayout::vp542_corpus,
           passability.layout==saves::GameStateLayout::vp542_corpus},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}
    };
}
} // namespace civ5::runtime
