#include "civ5/saves/civis_save.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/treasury.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_plot_unit_live_services.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/runtime/bnw_processed_auto_moves_import_owner.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_player_turn_active_dirty_issuer.hpp"
#include "civ5/runtime/detail/bnw_dynamic_turn_mode.hpp"
#include "civ5/runtime/bnw_net_message_debug_manager_owner.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_barbarians_begin.hpp"
#include "civ5/runtime/bnw_world_religion_progress.hpp"
#include "civ5/runtime/bnw_game_score.hpp"
#include "civ5/runtime/bnw_game_deals.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <bit>
#include <string_view>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <variant>
namespace civ5::runtime {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::int64_t integer(const saves::PregameRecord& p,const char* name){
    const auto at=p.integers.find(name);need(at!=p.integers.end(),"Retained turn missing pregame identity");return at->second;
}
void preflight(const LiveSessionState& state){
    const auto& w=state.world;
    const auto* p=std::get_if<saves::Expansion2PrefixObservation>(&w.game_state.prefix);
    need(state.setup.profile==content::TechnologyProfile::bnw&&
         w.game_state.layout==saves::GameStateLayout::expansion2_version1&&p&&p->version==1&&
         w.pregame&&w.pregame->layout==saves::GameStateLayout::expansion2_version1&&
         w.pregame->slots_version==3&&w.pregame->archive_version==6,
         "Retained turn unsupported BNW source profile");
    need(p->scalars[21]==0,"Retained turn AutoSave/AI autoplay owner unavailable");
    need(state.setup.game_type==0&&integer(w.pregame->archive,"game_type")==0,
         "Retained turn requires ordinary single-player source branch");
    need(state.setup.game_turn>=0&&integer(w.pregame->archive,"game_turn")==state.setup.game_turn,
         "Retained turn setup/raw clock identity differs");
}
struct BarbarianStep {
    std::int64_t counter=0;
    bool counter_store=false,clear_spawned=false;
};
const saves::NamedSaveInteger& plot_field(const saves::PlotObservation& plot,const char* name){
    const saves::NamedSaveInteger* found=nullptr;
    for(const auto& value:plot.fields)if(value.name&&std::string_view(value.name)==name){
        need(!found,"Retained barbarian duplicate raw plot operand");found=&value;
    }
    need(found,"Retained barbarian missing raw plot operand");return *found;
}
bool improvement_id(const content::InfoTypeCatalog& types,std::int64_t id){
    return std::any_of(types.entries().begin(),types.entries().end(),[&](const auto& row){
        return row.second.table=="Improvements"&&row.second.id==id;
    });
}
std::vector<BarbarianStep> prepare_barbarians(const LiveSessionState& state,
    const ui::SelectedBnwPreviewHost& host,std::int64_t camp){
    const auto& w=state.world;constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(w.game_state.layout==bnw&&w.map_header.layout==bnw&&w.map_plots&&
         w.map_plots->layout==bnw&&w.map_projection&&w.supporting_classes&&
         w.supporting_classes->layout==bnw&&w.supporting_classes->game_version==1&&
         w.supporting_classes->barbarians_version==1,
         "Retained barbarian decoded BNW map/support owner unavailable");
    const auto count=w.map_header.grid.size();
    need(count&&w.map_projection->grid.width==w.map_header.grid.width&&
         w.map_projection->grid.height==w.map_header.grid.height&&
         w.map_projection->cells.size()==count&&w.map_plots->plots.size()==count&&
         w.supporting_classes->plot_count==count&&w.supporting_classes->barbarians.size()==count,
         "Retained barbarian map/counter shape mismatch");
    need(host.types().report().all_succeeded()&&improvement_id(host.types(),camp),
         "Retained barbarian actual camp define absent from SAME Improvements catalog");
    std::vector<BarbarianStep> plan;plan.reserve(static_cast<std::size_t>(count));
    for(std::size_t index=0;index<w.supporting_classes->barbarians.size();++index){
        const auto& raw=w.map_plots->plots[index];const auto& cell=w.map_projection->cells[index];
        const auto xy=w.map_header.grid.coordinates(index);
        need(raw.index==index&&raw.x==xy[0]&&raw.y==xy[1]&&cell.index==index&&
             cell.x==xy[0]&&cell.y==xy[1]&&raw.version&&*raw.version>=1&&*raw.version<=7,
             "Retained barbarian raw/projected plot identity mismatch");
        const bool hashed=*raw.version>=5;
        const auto value=plot_field(raw,hashed?"improvement_type_hash":"improvement_type").value;
        if(hashed)need(value>=0&&value<=UINT32_MAX&&cell.improvement_type_hash&&
             *cell.improvement_type_hash==static_cast<std::uint64_t>(value),
             "Retained barbarian raw/projected improvement hash mismatch");
        else need(value>=-1&&value<=INT32_MAX&&cell.improvement_type&&
             *cell.improvement_type==value&&!cell.improvement_type_hash,
             "Retained barbarian raw/projected improvement ID mismatch");
        const auto& barb=w.supporting_classes->barbarians[index];
        need(!barb.attacked&&barb.counter>=INT16_MIN&&barb.counter<=INT16_MAX&&
             barb.units_spawned>=INT16_MIN&&barb.units_spawned<=INT16_MAX,
             "Retained barbarian source counter width/profile mismatch");
        BarbarianStep step{barb.counter,false,false};
        if(barb.counter>0){
            auto id=value;
            if(hashed){
                if(value==0)id=-1;
                else{
                    const auto* info=host.types().find_hash(static_cast<std::uint64_t>(value));
                    need(info&&info->table=="Improvements"&&info->id>=0&&info->id<=INT32_MAX,
                         "Retained barbarian actual improvement hash unresolved");id=info->id;
                }
                need(!cell.improvement_type||*cell.improvement_type==id,
                     "Retained barbarian resolved improvement cache differs");
            }else need(id==-1||improvement_id(host.types(),id),
                       "Retained barbarian actual improvement ID unresolved");
            step.counter_store=true;
            if(id!=camp){step.counter=-1;step.clear_spawned=true;}
            else --step.counter;
        }else if(barb.counter<-1){++step.counter;step.counter_store=true;}
        plan.push_back(step);
    }
    return plan; // Only a private counter-store plan, never a detached world.
}
void apply_barbarians(std::vector<saves::BarbSpawnerObservation>& rows,
    const std::vector<BarbarianStep>& plan)noexcept{
    // All allocation, operands and row counts have been checked before attempt.
    // Preserve original per-plot store order. No callbacks/RNG/UI/Lua here.
    for(std::size_t index=0;index<plan.size();++index){
        if(plan[index].counter_store)rows[index].counter=plan[index].counter;
        if(plan[index].clear_spawned)rows[index].units_spawned=-1;
    }
}
}
namespace {
struct SmallAwardRule {
    std::string type;
    std::int32_t cities=0,population=0,points=0;
};
struct SmallAwardCollection {
    content::InfoCollectionReport report;
    std::vector<std::optional<SmallAwardRule>> slots;
};
std::unique_ptr<const SmallAwardCollection> small_award_collection(const ui::SelectedBnwPreviewHost& host){
    auto result=std::make_unique<SmallAwardCollection>();
    const auto narrow=[](std::int64_t value){need(value>=INT32_MIN&&value<=INT32_MAX,
         "Retained small-award native GetInt operand outside i32");return static_cast<std::int32_t>(value);};
    result->report=host.database().prefetch_info("SmallAwards",[&](std::uint64_t slot,std::int64_t id,const content::RulesRowView& row){
        // Bound allocation, and require the standard exact ID->GC slot map.
        // Source NULL holes remain NULL; they never become fabricated infos.
        need(slot<1024*1024&&id>=0&&static_cast<std::uint64_t>(id)==slot,
             "Retained small-award overlapping/native slot identity unavailable");
        const auto type=row.text("Type");need(type&& !type->empty(),"Retained small-award actual CvBaseInfo type unavailable");
        const auto* global=host.types().find(*type);
        const auto* hashed=global?host.types().find_hash(global->hash):nullptr;
        need(global&&global->table=="SmallAwards"&&global->id==id&&hashed&&hashed->table==global->table&&
             hashed->id==global->id&&hashed->type==global->type,"Retained small-award same actual global info occurrence unavailable");
        result->slots.resize(static_cast<std::size_t>(slot+1));
        result->slots[slot]=SmallAwardRule{*type,narrow(row.integer("NumCities")),
            narrow(row.integer("CityPopulation")),narrow(row.integer("NumVictoryPoints"))};
        return true;
    });
    need(result->report.all_succeeded()&&result->report.slots<=INT32_MAX&&
         result->report.overlapping_ids==0&&result->slots.size()==result->report.slots,
         "Retained small-award actual PrefetchCollection/CacheResults incomplete");
    return result;
}
}
namespace {
std::int32_t team_war_field(const saves::TeamObservation& team,std::string_view name){
    const saves::NamedSaveInteger* found=nullptr;
    for(const auto& row:team.fields)if(row.name&&std::string_view(row.name)==name){
        need(!found,"Retained team-war duplicate raw field");found=&row;
    }
    need(found&&found->value>=INT32_MIN&&found->value<=INT32_MAX,"Retained team-war raw source-i32 field unavailable");
    return static_cast<std::int32_t>(found->value);
}
struct TeamWarScalarPlan {
    struct Pair {std::int32_t war=0,locked=0;bool war_store=false,locked_store=false;};
    std::array<Pair,63> pairs{};
    const saves::PlotArrayObservation* war=nullptr;
    const saves::PlotArrayObservation* locked=nullptr;
    bool changed=false;
};
TeamWarScalarPlan prepare_team_war(const LiveSessionState& state,std::int32_t team_id){
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(state.setup.profile==content::TechnologyProfile::bnw&&state.world.teams&&
         state.world.teams->layout==bnw&&state.world.teams->dimensions.teams==64&&
         state.world.teams->dimensions.civ_teams==63&&state.world.teams->dimensions.major==22&&
         state.world.teams->dimensions.profile_extensions.values.empty()&&state.world.teams->teams.size()==64&&
         team_id>=0&&team_id<63,"Retained team-war complete actual nonbarbarian BNW roster unavailable");
    const auto& roster=state.world.teams->teams;
    const auto index=static_cast<std::size_t>(team_id);const auto& team=roster[index];
    need(team.index==index&&team_war_field(team,"id")==team_id&&team_war_field(team,"alive_count")>0,
         "Retained team-war actual slot/GetID/alive differs");
    const auto array=[](const saves::TeamObservation& value,std::string_view name)->const saves::PlotArrayObservation&{
        const saves::PlotArrayObservation* found=nullptr;
        for(const auto& row:value.arrays)if(row.name&&std::string_view(row.name)==name){
            need(!found,"Retained team-war duplicate raw array");found=&row;
        }
        need(found&&found->columns==1&&found->values.size()==64,"Retained team-war source64 relation array unavailable");return *found;
    };
    const auto& at_war=array(team,"at_war");const auto& war=array(team,"turns_at_war");
    const auto& locked=array(team,"turns_locked_at_war");TeamWarScalarPlan result;result.war=&war;result.locked=&locked;
    for(std::int32_t target=0;target<63;++target){
        const auto slot=static_cast<std::size_t>(target);
        need(roster[slot].index==slot&&team_war_field(roster[slot],"id")==target,
             "Retained team-war actual target slot/GetID differs");
        need(at_war.values[slot]==0||at_war.values[slot]==1,"Retained team-war at-war source boolean unavailable");
        const auto old_war=war.values[slot],old_locked=locked.values[slot];
        need(old_war>=INT32_MIN&&old_war<=INT32_MAX&&old_locked>=INT32_MIN&&old_locked<=INT32_MAX,
             "Retained team-war counter source-i32 unavailable");
        const auto next=at_war.values[slot]?old_war+1:0;
        // Native arithmetic precedes the nonzero-self no-store setter guard.
        need(next<=INT32_MAX,"Retained team-war Change source-i32 overflow BEFORE self setter guard");
        auto& pair=result.pairs[slot];pair.war=static_cast<std::int32_t>(next);
        pair.war_store=target!=team_id||pair.war==0;pair.locked=static_cast<std::int32_t>(old_locked);
        if(old_locked>0){pair.locked=static_cast<std::int32_t>(old_locked-1);pair.locked_store=target!=team_id||pair.locked==0;}
        result.changed=result.changed||(pair.war_store&&old_war!=pair.war)||(pair.locked_store&&old_locked!=pair.locked);
    }
    need(!result.changed||state.ui_source_revision<UINT64_MAX,"Retained team-war world revision exhausted");return result;
}
// Exact DoMinorCivTech no-acquisition branch on the SAME imported owners.
// A due setHasTech is mandatory before any no-trade attempt; no bit-only grant.
std::optional<BnwRetainedMinorTechnologyPlan> first_minor_tech_acquisition(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,std::int32_t recipient){
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(state.world.teams&&state.world.teams->layout==bnw&&state.world.teams->teams.size()==64&&
         state.world.pregame&&state.world.pregame->layout==bnw&&state.world.all_players&&
         state.world.all_players->layout==bnw&&state.world.all_players->context.player_count==64&&
         state.world.all_players->players.size()==64&&state.technologies.size()==64&&recipient>=0&&recipient<63,
         "Retained pending minor actual decoded owners unavailable BEFORE attempt");
    const auto& archive=state.world.pregame->archive;
    const auto teams=archive.integer_arrays.find("team_types");const auto flags=archive.flag_arrays.find("minor_nation_civs");
    need(teams!=archive.integer_arrays.end()&&teams->second.size()==64&&flags!=archive.flag_arrays.end()&&flags->second.size()==64,
         "Retained pending minor actual Pregame owners unavailable BEFORE attempt");
    const auto is_minor=[&](std::int32_t team)->bool{
        for(std::size_t slot=22;slot<64;++slot){const auto& player=state.world.all_players->players[slot];
            const saves::PlayerCoreField* id=nullptr;const saves::PlayerCoreField* alive=nullptr;
            for(const auto& row:player.core.fields)if(row.name){const auto name=std::string_view(row.name);
                if(name=="m_eID"){need(!id,"Duplicate minor technology actual player ID");id=&row;}
                if(name=="m_bAlive"){need(!alive,"Duplicate minor technology actual alive flag");alive=&row;}}
            need(player.index==slot&&player.saved_player_id==static_cast<std::int64_t>(slot)&&player.core.layout==bnw&&
                 id&&id->values.size()==1&&id->values[0]==static_cast<std::int64_t>(slot)&&
                 alive&&alive->values.size()==1&&(alive->values[0]==0||alive->values[0]==1),
                 "Retained pending minor source player identity/alive unavailable BEFORE attempt");
            if(alive->values[0]){need(teams->second[slot]>=0&&teams->second[slot]<64,"Retained pending minor living GetTeam unavailable");
                if(teams->second[slot]==team)return flags->second[slot];}
        }return false;
    };
    if(!is_minor(recipient))return std::nullopt;
    const auto& rules=host.technologies();need(rules.profile==content::TechnologyProfile::bnw&&rules.all_succeeded()&&rules.collection.slots<=INT32_MAX,
         "Retained pending minor actual native tech slot owner unavailable BEFORE attempt");
    std::array<bool,64> authenticated{};
    const auto validate_tech=[&](std::int32_t team)->const TeamTechnologies&{
        const auto index=static_cast<std::size_t>(team);const auto& raw=state.world.teams->teams[index];const auto& tech=state.technologies[index];
        need(raw.index==index&&team_war_field(raw,"id")==team&&team_war_field(raw,"tech_version")==1&&
             tech.team()==team&&tech.profile()==rules.profile&&tech.slots()==rules.collection.slots&&
             tech.profile_extensions().values.empty()&&tech.remap().all_mapped()&&tech.remap().duplicates==0&&tech.remap().holes==0,
             "Retained pending minor SAME current team technology ownership unavailable BEFORE attempt");
        if(!authenticated[index]){
            need(host.types().report().all_succeeded()&&host.types().report().hash_collisions==0,"Retained pending minor actual technology type mapping unavailable");
            const auto decoded=restore_team_technologies(raw,rules,host.types());
            need(decoded.profile()==tech.profile()&&decoded.team()==team&&decoded.slots()==tech.slots()&&
                 decoded.remap().all_mapped()&&decoded.remap().duplicates==0&&decoded.remap().holes==0&&
                 decoded.last_tech()==tech.last_tech(),"Retained pending minor raw/owned technology identity differs BEFORE attempt");
            for(std::uint64_t slot=0;slot<tech.slots();++slot)need(decoded.at(static_cast<std::int64_t>(slot))==tech.at(static_cast<std::int64_t>(slot)),
                 "Retained pending minor raw/owned HasTech/count/progress/no-trade differ BEFORE attempt");
            authenticated[index]=true;
        }
        return tech;
    };
    std::int32_t majors=0;
    for(std::int32_t team=0;team<63;++team){const auto& raw=state.world.teams->teams[static_cast<std::size_t>(team)];
        need(raw.index==static_cast<std::uint64_t>(team)&&team_war_field(raw,"id")==team,"Retained pending minor source team identity differs");
        if(team_war_field(raw,"alive_count")>0&&!is_minor(team))++majors;}
    // Native diagnostic expects >0 even though max(1,...) avoids division by
    // zero; do not guess an assertion service or fake a positive denominator.
    need(majors>0,"Retained pending minor zero-major native assertion requires diagnostic owner BEFORE attempt");
    const auto* define=host.defines().find("MINOR_CIV_TECH_PERCENT");
    need(host.defines().profile()==content::GlobalDefineProfile::bnw&&define&&define->lookup.found&&define->lookup.error.empty()&&
         std::holds_alternative<std::int64_t>(define->lookup.value),"Retained pending minor actual technology percentage unavailable BEFORE attempt");
    const auto percent=std::get<std::int64_t>(define->lookup.value);
    need(percent>=0&&percent<=INT32_MAX&&static_cast<std::int64_t>(majors)*percent<=INT32_MAX,
         "Retained pending minor native percentage multiplication unsupported/overflow BEFORE attempt");
    const auto threshold=std::max<std::int64_t>(1,static_cast<std::int64_t>(majors)*percent/100);
    const auto& target=validate_tech(recipient);
    for(std::uint64_t slot=0;slot<rules.collection.slots;++slot){need(rules.find(slot),"Retained pending minor native tech slot hole unavailable");
        if(target.has_tech(static_cast<std::int64_t>(slot)))continue;
        std::int32_t owners=0;
        for(std::int32_t team=0;team<63;++team){const auto& raw=state.world.teams->teams[static_cast<std::size_t>(team)];
            if(team_war_field(raw,"alive_count")>0&&!is_minor(team)&&validate_tech(team).has_tech(static_cast<std::int64_t>(slot)))++owners;}
        if(owners>=threshold){
            // Source getLeaderID is reached only for this first due slot.
            // Follow Pregame::teamType, not the mirrored saved m_eTeam field.
            std::array<bool,64> alive{};
            for(std::size_t player_slot=0;player_slot<64;++player_slot){
                const auto& player=state.world.all_players->players[player_slot];
                const saves::PlayerCoreField* id=nullptr;const saves::PlayerCoreField* flag=nullptr;
                for(const auto& row:player.core.fields)if(row.name){const auto name=std::string_view(row.name);
                    if(name=="m_eID"){need(!id,"Duplicate minor due-plan actual leader ID");id=&row;}
                    if(name=="m_bAlive"){need(!flag,"Duplicate minor due-plan actual leader alive");flag=&row;}}
                need(player.index==player_slot&&player.saved_player_id==static_cast<std::int64_t>(player_slot)&&
                     player.core.layout==bnw&&id&&id->values.size()==1&&id->values[0]==static_cast<std::int64_t>(player_slot)&&
                     flag&&flag->values.size()==1&&(flag->values[0]==0||flag->values[0]==1)&&
                     teams->second[player_slot]>=-1&&teams->second[player_slot]<64,
                     "Retained minor due-plan actual leader/Pregame owner unavailable BEFORE attempt");
                alive[player_slot]=flag->values[0]!=0;
            }
            std::int32_t leader=-1;
            for(std::size_t player_slot=0;player_slot<64;++player_slot)
                if(alive[player_slot]&&teams->second[player_slot]==recipient){leader=static_cast<std::int32_t>(player_slot);break;}
            if(leader==-1)for(std::size_t player_slot=0;player_slot<64;++player_slot)
                if(teams->second[player_slot]==recipient){leader=static_cast<std::int32_t>(player_slot);break;}
            return BnwRetainedMinorTechnologyPlan{recipient,leader,static_cast<std::int64_t>(slot),majors,owners,threshold};
        }
    }
    return std::nullopt;
}
void apply_team_war(std::vector<std::int64_t>& war,std::vector<std::int64_t>& locked,const TeamWarScalarPlan& plan)noexcept{
    for(std::size_t target=0;target<63;++target){
        if(plan.pairs[target].war_store)war[target]=plan.pairs[target].war;
        if(plan.pairs[target].locked_store)locked[target]=plan.pairs[target].locked;
    }
}
}
namespace {
std::optional<std::int64_t> retained_map_player_team(const LiveSessionState& state,std::int64_t owner){
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(owner>=0&&owner<64&&state.world.pregame&&state.world.pregame->layout==bnw&&
         state.world.all_players&&state.world.all_players->layout==bnw&&state.world.all_players->players.size()==64&&
         state.world.teams&&state.world.teams->layout==bnw&&state.world.teams->teams.size()==64,
         "Retained map actual player/team owner unavailable");
    const auto teams=state.world.pregame->archive.integer_arrays.find("team_types");
    need(teams!=state.world.pregame->archive.integer_arrays.end()&&teams->second.size()==64,
         "Retained map actual Pregame GetTeam source unavailable");
    const auto slot=static_cast<std::size_t>(owner);const auto id=teams->second[slot];
    const auto setup=state.setup.players.find(owner);
    const auto& player=state.world.all_players->players[slot];
    need(player.index==slot&&player.saved_player_id==owner&&setup!=state.setup.players.end()&&
         setup->second.team==id&&id>=0&&id<64,
         "Retained map raw/owned player GetTeam differs");
    const auto& team=state.world.teams->teams[static_cast<std::size_t>(id)];
    need(team.index==static_cast<std::uint64_t>(id)&&team_war_field(team,"id")==id,
         "Retained map actual team GetID differs");return id;
}
struct RetainedMapPlotPlan {
    saves::NamedSaveInteger *ownership=nullptr,*improvement=nullptr,*anchor=nullptr,*anchor_data=nullptr;
    saves::PlayerCoreField* land_score=nullptr;
    std::int64_t old_ownership=0,old_improvement=0,old_anchor_data=0,old_score=0;
    std::int64_t next_ownership=0,next_improvement=0,next_score=0;
    bool ownership_store=false,improvement_store=false,score_store=false,changed=false;
    bool operator==(const RetainedMapPlotPlan&)const=default;
};
RetainedMapPlotPlan prepare_retained_map_plot(LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,
    std::uint64_t index,std::int64_t threshold){
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    auto& world=state.world;const auto count=world.map_header.grid.size();
    need(world.map_header.layout==bnw&&world.map_plots&&world.map_plots->layout==bnw&&world.map_projection&&
         world.all_players&&world.all_players->layout==bnw&&world.map_plots->plots.size()==count&&
         world.map_projection->cells.size()==count&&index<count,
         "Retained map complete SAME decoded map/players unavailable");
    const auto slot=static_cast<std::size_t>(index);auto& raw=world.map_plots->plots[slot];
    const auto& cell=world.map_projection->cells[slot];const auto xy=world.map_header.grid.coordinates(index);
    need(raw.index==index&&raw.x==xy[0]&&raw.y==xy[1]&&cell.index==index&&cell.x==xy[0]&&cell.y==xy[1]&&
         raw.version&&*raw.version>=1&&*raw.version<=7&&cell.units&&cell.owner&&cell.plot_type,
         "Retained map raw/owned plot index/version/roster differs");
    const auto field=[&](const char* name)->saves::NamedSaveInteger*{
        saves::NamedSaveInteger* found=nullptr;
        for(auto& value:raw.fields)if(value.name&&std::string_view(value.name)==name){
            need(!found,"Retained map duplicate raw operand");found=&value;}
        need(found,"Retained map missing raw operand");return found;
    };
    const auto owner=field("owner")->value,plot_type=field("plot_type")->value;
    need(owner>=-1&&owner<64&&owner==*cell.owner&&plot_type>=0&&plot_type<=3&&plot_type==*cell.plot_type,
         "Retained map raw/owned owner or isWater differs");
    RetainedMapPlotPlan plan;plan.ownership=field("ownership_duration");plan.improvement=field("improvement_duration");
    plan.anchor=field("world_anchor");plan.anchor_data=field("world_anchor_data");
    plan.old_ownership=plan.ownership->value;plan.old_improvement=plan.improvement->value;
    plan.old_anchor_data=plan.anchor_data->value;
    need(plan.anchor->value==-1,"Retained map reached GameplayWorldAnchor removal receiver unavailable");
    need(plan.old_anchor_data>=INT8_MIN&&plan.old_anchor_data<=INT8_MAX&&
         plan.old_ownership>=0&&plan.old_ownership<=INT16_MAX&&plan.old_improvement>=0&&plan.old_improvement<=INT16_MAX,
         "Retained map serialized operand width unavailable");
    plan.next_ownership=plan.old_ownership;plan.next_improvement=plan.old_improvement;
    if(owner!=-1){need(plan.old_ownership<INT16_MAX,"Retained map ownership save-width overflow");
        plan.ownership_store=true;++plan.next_ownership;
        if(plan.old_ownership<threshold&&plan.next_ownership>=threshold&&plot_type!=3){
            auto& player=world.all_players->players.at(static_cast<std::size_t>(owner));
            need(player.index==static_cast<std::uint64_t>(owner)&&player.saved_player_id==owner,
                 "Retained map scored owner identity differs");
            for(auto& value:player.core.fields)if(value.name&&std::string_view(value.name)=="m_iTotalLandScored"){
                need(!plan.land_score,"Retained map duplicate player land score");plan.land_score=&value;}
            need(plan.land_score&&plan.land_score->encoding==saves::PlayerCoreEncoding::i32&&
                 plan.land_score->values.size()==1,"Retained map player land score unavailable");
            plan.old_score=plan.land_score->values.front();
            need(plan.old_score>=0&&plan.old_score<INT32_MAX,"Retained map land score source-i32 overflow");
            plan.next_score=plan.old_score+1;plan.score_store=true;
        }
    }
    const bool hashed=*raw.version>=5;const auto improvement=field(hashed?"improvement_type_hash":"improvement_type")->value;
    bool exists=false;
    if(hashed){need(improvement>=0&&improvement<=UINT32_MAX&&cell.improvement_type_hash&&
         *cell.improvement_type_hash==static_cast<std::uint64_t>(improvement),"Retained map improvement hash differs");
        if(improvement){const auto* info=host.types().find_hash(static_cast<std::uint64_t>(improvement));
            need(info&&info->table=="Improvements"&&info->id>=0&&info->id<=INT32_MAX&&
                 (!cell.improvement_type||*cell.improvement_type==info->id),"Retained map actual improvement hash unresolved");exists=true;
        }else need(!cell.improvement_type||*cell.improvement_type==-1,"Retained map no-improvement cache differs");
    }else{need(improvement>=-1&&improvement<=INT8_MAX&&cell.improvement_type&&*cell.improvement_type==improvement&&
         (improvement==-1||improvement_id(host.types(),improvement)),"Retained map legacy improvement identity differs");exists=improvement!=-1;}
    if(exists){need(plan.old_improvement<INT16_MAX,"Retained map improvement save-width overflow");
        plan.improvement_store=true;++plan.next_improvement;}
    plan.changed=plan.ownership_store||plan.improvement_store||plan.old_anchor_data!=-1;
    return plan;
}
}
namespace {
bool retained_options_equal(const std::vector<saves::PregameOption>& a,const std::vector<saves::PregameOption>& b){
    if(a.size()!=b.size())return false;
    for(std::size_t k=0;k<a.size();++k)if(a[k].name!=b[k].name||a[k].value!=b[k].value)return false;
    return true;
}
std::int32_t retained_barbarian_define(const ui::SelectedBnwPreviewHost& host,const char* name){
    const auto* entry=host.defines().find(name);
    need(entry&&entry->lookup.found&&entry->lookup.error.empty()&&
         std::holds_alternative<std::int64_t>(entry->lookup.value),"Retained barbarian active integer define missing");
    const auto value=std::get<std::int64_t>(entry->lookup.value);
    need(value>=INT32_MIN&&value<=INT32_MAX,"Retained barbarian native define width differs");
    return static_cast<std::int32_t>(value);
}
struct RetainedCampPlan {
    std::vector<saves::PregameOption> options;
    bool no_barbarians=false,spawn_branch=false,first_turn=false;
    std::int32_t camp=-1,not_visible=0,camps=0,fog=0,to_add=0,odds=0,coastal=0;
    SimulationRandomState rng;
    bool same(const RetainedCampPlan& p)const {
        return retained_options_equal(options,p.options)&&no_barbarians==p.no_barbarians&&
         spawn_branch==p.spawn_branch&&first_turn==p.first_turn&&camp==p.camp&&
         not_visible==p.not_visible&&camps==p.camps&&fog==p.fog&&to_add==p.to_add&&
         odds==p.odds&&coastal==p.coastal&&rng==p.rng;
    }
};
RetainedCampPlan prepare_retained_camps(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host){
    const auto& world=state.world;need(world.pregame&&
      retained_options_equal(state.setup.game_options,world.pregame->game_options),
      "Retained barbarian paired lazy GameOptions cache differs");
    auto setup=state.setup;RetainedCampPlan result;
    result.no_barbarians=setup.game_option_value(host.database(),"GAMEOPTION_NO_BARBARIANS")!=0;
    result.options=std::move(setup.game_options);result.rng=state.random.gameplay.state();
    if(result.no_barbarians)return result; // native before camp/map/rules operands
    result.camp=retained_barbarian_define(host,"BARBARIAN_CAMP_IMPROVEMENT");
    need(result.camp>=-1,"Retained camp improvement is not native NO_IMPROVEMENT or active ID");
    if(result.camp==-1)return result;
    // Reuse the genuine raw/hash/owned map and native counter identity proof.
    (void)prepare_barbarians(state,host,result.camp);
    const auto& visibility=bnw_live_plot_visibility_cache(state);
    const auto count=world.map_header.grid.size();need(count<=INT32_MAX,"Retained camps numPlots native width exceeded");
    std::array<bool,63> alive{};
    need(world.teams&&world.teams->teams.size()==64,"Retained camps active team source missing");
    for(std::size_t team=0;team<alive.size();++team){
        const auto& raw=world.teams->teams[team];
        need(raw.index==team&&team_war_field(raw,"id")==static_cast<std::int32_t>(team),
             "Retained camps native visibility team identity differs");
        alive[team]=team_war_field(raw,"alive_count")>0;
    }
    for(std::size_t k=0;k<static_cast<std::size_t>(count);++k){
        const auto& raw=world.map_plots->plots[k];const auto& cell=world.map_projection->cells[k];
        auto id=plot_field(raw,*raw.version>=5?"improvement_type_hash":"improvement_type").value;
        if(*raw.version>=5){if(id==0)id=-1;else{
            const auto* info=host.types().find_hash(static_cast<std::uint64_t>(id));
            need(info&&info->table=="Improvements"&&info->id>=0&&info->id<=INT32_MAX,
                 "Retained camp plot active Improvement hash unresolved");id=info->id;}}
        need(!cell.improvement_type||*cell.improvement_type==id,"Retained camp improvement projection differs");
        if(id==result.camp)++result.camps;
        const auto plot_type=plot_field(raw,"plot_type").value;
        need(cell.plot_type&&plot_type>=0&&plot_type<=3&&*cell.plot_type==plot_type,
             "Retained camps paired IsWater source unresolved");
        if(plot_type!=3){bool visible=false;
            for(std::size_t team=0;team<alive.size();++team)
                if(alive[team]&&visibility.counts.at(k*static_cast<std::size_t>(visibility.wire_slots)+team)>0){visible=true;break;}
            if(!visible)++result.not_visible;
        }
    }
    const auto size=integer(world.pregame->archive,"world_size");
    need(size>=0&&size<=INT32_MAX&&world.pregame->world.id==size,
         "Retained camps actual cached/raw world-size identity differs");
    bool found=false;std::string type;std::int64_t fog=0;
    const auto query=host.database().query_rows({"SELECT Type,FogTilesPerBarbarianCamp FROM Worlds WHERE ID=?",{size}},
      [&](const auto& row){need(!found,"Retained camps duplicate Worlds ID");const auto value=row.text("Type");
          need(bool(value),"Retained camps WorldInfo Type is NULL");found=true;type=*value;fog=row.integer_at(1);},2);
    need(query.all_succeeded()&&found&&fog>=INT32_MIN&&fog<=INT32_MAX&&
         type==world.pregame->world.text[5],"Retained camps SAME active WorldInfo fog input missing/different");
    const bool world_type=std::any_of(host.types().entries().begin(),host.types().entries().end(),[&](const auto& item){
        return item.second.table=="Worlds"&&item.second.id==size&&item.first==type;});
    need(world_type,"Retained camps WorldInfo missing actual active type catalog");result.fog=static_cast<std::int32_t>(fog);
    const auto target=result.fog!=0?result.not_visible/result.fog:0;
    const auto difference=static_cast<std::int64_t>(target)-result.camps;
    need(difference>=INT32_MIN&&difference<=INT32_MAX,"Retained camps native target subtraction overflows");
    result.to_add=static_cast<std::int32_t>(difference);
    if(result.to_add<=0)return result;
    result.odds=retained_barbarian_define(host,"BARBARIAN_CAMP_ODDS_OF_NEW_CAMP_SPAWNING");
    if(result.odds<=0)return result;
    const auto* game=std::get_if<saves::Expansion2PrefixObservation>(&world.game_state.prefix);
    need(game&&game->scalars[1]>=0&&game->scalars[1]<=INT32_MAX&&state.maintenance.elapsed_turns==game->scalars[1],
         "Retained camps actual elapsed clock unresolved");
    result.first_turn=game->scalars[1]==0;
    if(result.first_turn){
        const auto percent=retained_barbarian_define(host,"BARBARIAN_CAMP_FIRST_TURN_PERCENT_OF_TARGET_TO_ADD");
        const auto product=static_cast<std::int64_t>(result.to_add)*percent;
        need(product>=INT32_MIN&&product<=INT32_MAX,"Retained camps native first-turn multiplication overflows");
        result.to_add=static_cast<std::int32_t>(product)/100;
    }
    result.coastal=retained_barbarian_define(host,"BARBARIAN_CAMP_COASTAL_SPAWN_ROLL");
    need(result.odds<=UINT16_MAX&&result.coastal>0&&result.coastal<=UINT16_MAX&&
         state.random.gameplay.algorithm()==RandomAlgorithm::bnw&&result.rng.calls<=UINT64_MAX-2,
         "Retained camps native ushort RNG bounds/actual BNW stream unavailable");
    result.spawn_branch=true;return result;
}
struct RetainedBarbarianUnitPlan {
    std::vector<saves::PregameOption> options;
    bool no_barbarians=false,can_spawn=false;
    std::optional<std::uint64_t> due_plot;
    bool same(const RetainedBarbarianUnitPlan& p)const {
        return retained_options_equal(options,p.options)&&no_barbarians==p.no_barbarians&&
               can_spawn==p.can_spawn&&due_plot==p.due_plot;
    }
};
RetainedBarbarianUnitPlan prepare_retained_barbarian_units(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host){
    const auto& world=state.world;need(world.pregame&&
      retained_options_equal(state.setup.game_options,world.pregame->game_options),
      "Retained barbarian units paired lazy GameOptions differs");
    auto setup=state.setup;RetainedBarbarianUnitPlan result;
    result.no_barbarians=setup.game_option_value(host.database(),"GAMEOPTION_NO_BARBARIANS")!=0;
    result.options=std::move(setup.game_options);if(result.no_barbarians)return result;
    const auto* game=std::get_if<saves::Expansion2PrefixObservation>(&world.game_state.prefix);
    need(game&&game->scalars[1]>=0&&game->scalars[1]<=INT32_MAX&&state.maintenance.elapsed_turns==game->scalars[1],
         "Retained barbarian units elapsed source differs");
    result.can_spawn=game->scalars[1]>=10;if(!result.can_spawn)return result;
    const auto camp=retained_barbarian_define(host,"BARBARIAN_CAMP_IMPROVEMENT");
    need(camp>=-1,"Retained barbarian units native camp ID unresolved");
    // Unlike DoCamps there is no NO_IMPROVEMENT early return: native DoUnits
    // still compares each plot to -1. Refuse that unsupported spawn branch.
    need(camp!=-1,"Retained DoUnits NO_IMPROVEMENT scan requires genuine spawn handling");
    (void)prepare_barbarians(state,host,camp);
    for(std::size_t k=0;k<world.map_plots->plots.size();++k){
        const auto& raw=world.map_plots->plots[k];
        auto id=plot_field(raw,*raw.version>=5?"improvement_type_hash":"improvement_type").value;
        if(*raw.version>=5){if(id==0)id=-1;else{
            const auto* info=host.types().find_hash(static_cast<std::uint64_t>(id));
            need(info&&info->table=="Improvements","Retained DoUnits active Improvement unresolved");id=info->id;}}
        if(id==camp&&world.supporting_classes->barbarians[k].counter==0){result.due_plot=k;break;}
    }
    return result;
}
}
namespace {
auto& retained_religion_named(auto& rows,std::string_view name){
    using Row=std::remove_reference_t<decltype(rows.front())>;Row* found=nullptr;
    for(auto& row:rows)if(row.name&&name==row.name){need(!found,"Retained religion duplicate native field");found=&row;}
    need(found,"Retained religion native field missing");return *found;
}
std::int32_t retained_religion_i32(const auto& rows,std::string_view name){
    const auto& row=retained_religion_named(rows,name);
    need(row.values.size()==1&&row.values.front()>=INT32_MIN&&row.values.front()<=INT32_MAX,
         "Retained religion native scalar width differs");return static_cast<std::int32_t>(row.values.front());
}
struct RetainedReligionCityPlan {
    std::optional<std::pair<std::int32_t,std::int32_t>> city;
    std::vector<std::pair<saves::CityTailNode*,std::int64_t>> counters;
    bool holy=false,changed=false;
    bool operator==(const RetainedReligionCityPlan&)const=default;
};
RetainedReligionCityPlan prepare_retained_first_religion_city(LiveSessionState& state){
    auto& world=state.world;need(world.all_players&&world.all_players->players.size()==64,
        "Retained religion complete native player roster unavailable");
    RetainedReligionCityPlan result;
    for(std::int32_t owner=0;owner<64;++owner){
        auto& player=world.all_players->players[static_cast<std::size_t>(owner)];
        const auto live=state.setup.players.find(owner);
        const auto& alive_field=retained_religion_named(player.core.fields,"m_bAlive");
        const auto alive=retained_religion_i32(player.core.fields,"m_bAlive");
        need(player.index==static_cast<std::uint64_t>(owner)&&player.saved_player_id==owner&&
             player.core.layout==saves::GameStateLayout::expansion2_version1&&
             live!=state.setup.players.end()&&alive_field.encoding==saves::PlayerCoreEncoding::boolean&&
             (alive==0||alive==1)&&live->second.alive==(alive!=0),
             "Retained religion native/owned isAlive identity differs");
        if(!alive)continue;
        // Existing genuine registry proves the complete paired city roster,
        // unique native slots, capacity and GetCount. GetFirst scans occupied
        // FLTA slots ascending; archive vector order is not used as authority.
        (void)resolve_bnw_city_registry(world,state.cities,owner,0);
        saves::SavedCityObservation* first=nullptr;std::int32_t first_slot=INT32_MAX,id=-1;
        for(auto& city:player.cities.cities){
            const auto candidate=retained_religion_i32(city.core.fields,"m_iID"),slot=candidate&8191;
            if(slot<first_slot){first=&city;first_slot=slot;id=candidate;}
        }
        if(!first)continue;
        need(resolve_bnw_city_registry(world,state.cities,owner,id)==first,
             "Retained religion actual first city resolver differs");
        result.city=std::pair{owner,id};const auto& owned=state.cities.players.at(owner).cities.at(id);
        need(first->tail.layout==saves::GameStateLayout::expansion2_version1&&
             first->tail.context.profile_extensions.values.empty()&&owned.extensions.values.empty(),
             "Retained religion city profile extension requires its effects");
        auto& religion=retained_religion_named(first->tail.blocks,"religions");
        auto& status=retained_religion_named(religion.children,"m_ReligionStatus");
        need(status.encoding&&std::string_view(status.encoding)=="signed_records"&&status.wire_count&&
             *status.wire_count>=INT32_MIN&&*status.wire_count<=INT32_MAX&&
             static_cast<std::uint64_t>(std::max<std::int64_t>(0,*status.wire_count))==status.children.size()&&
             status.children.size()==owned.religions.size(),"Retained religion native/current status count differs");
        result.counters.reserve(status.children.size());
        for(std::size_t k=0;k<status.children.size();++k){
            auto& row=status.children[k];need(row.name&&std::string_view(row.name)=="religion",
                 "Retained religion native status record differs");
            const auto version=retained_religion_i32(row.children,"version");
            // v1 has an unserialized trade-route counter. Its real load owner
            // must restore that storage; missing is never supplied as zero.
            need(version==2,"Retained religion v1 unsaved pressure-count load owner required");
            const auto religion_id=retained_religion_i32(row.children,"m_eReligion");
            const auto followers=retained_religion_i32(row.children,"m_iFollowers");
            const auto pressure=retained_religion_i32(row.children,"m_iPressure");
            const auto founded=retained_religion_i32(row.children,"m_bFoundedHere");
            need(retained_religion_named(row.children,"m_bFoundedHere").encoding&&
                 std::string_view(retained_religion_named(row.children,"m_bFoundedHere").encoding)=="bool",
                 "Retained religion native holy flag storage differs");
            const auto& live_status=owned.religions[k];
            need((founded==0||founded==1)&&live_status.religion==religion_id&&
                 live_status.followers==followers&&live_status.pressure==pressure,
                 "Retained religion paired current status differs");
            auto& counter=retained_religion_named(row.children,"m_iNumTradeRoutesApplyingPressure");
            need(counter.encoding&&std::string_view(counter.encoding)=="i32"&&counter.children.empty(),
                 "Retained religion native trade-route counter storage differs");
            const auto old=retained_religion_i32(row.children,"m_iNumTradeRoutesApplyingPressure");
            result.counters.emplace_back(&counter,old);result.changed=result.changed||old!=0;
            result.holy=result.holy||founded!=0;
        }
        return result; // original outer player/city order, first actual city only
    }
    return result;
}
}
struct BnwRetainedTurnCursor::History {
    const std::thread::id thread=std::this_thread::get_id();
    std::atomic<bool> poisoned{false},retired{false};
    std::atomic<bool> trade_preparing{false};
    std::atomic<bool> league_preparing{false};
    std::atomic<bool> culture_preparing{false};
    std::atomic<bool> tail_dispatching{false};
    std::optional<std::int32_t> active_culture_player;
    std::optional<std::uint64_t> game_tail_earned_revision;
    BnwCulturePlatformStatus culture_platform_status=BnwCulturePlatformStatus::not_reached;
    BnwCultureDiagnosticStatus culture_diagnostic_status=BnwCultureDiagnosticStatus::not_reached;
    std::optional<std::int32_t> next_culture_player;
    std::uint64_t completed_culture_players=0;
    bool end_turn_ui_reset_attempted=false;
    bool ai_autoplay_tail_attempted=false;
    bool game_turn_net_log_attempted=false;
    std::optional<BnwNetDebugOwnDelivery> game_turn_net_log_delivery;
    std::optional<std::int32_t> logged_game_turn;
    std::shared_ptr<const BnwNetMessageDebugManagerOwner> game_turn_net_manager;
    struct GameTurnGameplayReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner> issuer;std::weak_ptr<const ui::BnwEngineUiTurnOwner> engine;
     std::weak_ptr<const LiveSessionState> world;
     std::uint64_t generation=0,before=0,after=0,serial=0;std::int64_t turn=0;
     bool stored=false,source_adopted=false;
    };
    struct GameTurnElapsedReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner> issuer;
     std::weak_ptr<const LiveSessionState> world;
     std::uint64_t generation=0,before=0,after=0,serial=0;
     std::int64_t old_elapsed=0,new_elapsed=0,turn=0;
     bool stored=false,source_adopted=false;
    };
    bool game_turn_elapsed_attempted=false;
    std::optional<GameTurnElapsedReturn> game_turn_elapsed_return;
    struct GameTurnDynamicModeReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner> issuer;
     std::weak_ptr<const LiveSessionState> world;
     std::uint64_t generation=0,before=0,after=0,serial=0;std::int64_t turn=0;
     std::uint64_t operation=0;BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_dynamic_turns_owner;
     SimulationRandomState map_rng,game_rng;
     std::vector<saves::PregameOption> raw_options,owned_options;
     std::int64_t dynamic=0,pitboss=0,simultaneous=0;
     std::optional<std::int32_t> sequential;
     std::optional<detail::DynamicModeComponentPlan> plan;
     std::optional<detail::DynamicModeOperands> read_operands;
     bool stored=false,source_adopted=false;
    };
    bool game_turn_dynamic_attempted=false;
    std::optional<GameTurnDynamicModeReturn> game_turn_dynamic_return;
    struct GameTurnShuffleReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner> issuer;std::weak_ptr<const LiveSessionState> world;
     std::uint64_t generation=0,before=0,after=0,serial=0,operation=0;std::int64_t turn=0,pitboss=0,simultaneous=0;
     std::vector<saves::PregameOption> raw_options,owned_options;
     std::optional<detail::TurnShuffleComponent> component;
     std::optional<std::int32_t> player;
     BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_simultaneous_shuffle_owner;
     bool rng_stored=false,source_adopted=false,pitboss_queried=false,scan_returned=false;
    };
    bool game_turn_shuffle_attempted=false;
    std::optional<GameTurnShuffleReturn> game_turn_shuffle_return;
    struct GameTurnPlayerActiveReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner> issuer;std::weak_ptr<const LiveSessionState> world;
     std::uint64_t generation=0,before=0,after=0,serial=0,operation=0;std::int64_t turn=0;
     std::int32_t player=-1;bool shuffled=false,stored=false,source_adopted=false;
     std::vector<saves::PregameOption> raw_options,owned_options;
     detail::DynamicModeOperands operands;
     std::array<std::int64_t,64> active{};
     std::array<const void*,64> active_fields{},active_values{};
     std::array<std::size_t,64> active_indexes{};
     SimulationRandomState map_rng,game_rng;
     void require_operands(const LiveSessionState& s,bool after_store)const{
      need(detail::capture_dynamic_mode_operands(s)==operands,"Player turn-active raw/owned roster changed");
      need(s.random.map.algorithm()==RandomAlgorithm::bnw&&s.random.gameplay.algorithm()==RandomAlgorithm::bnw&&
       s.random.map.state()==map_rng&&s.random.gameplay.state()==game_rng,
       "Player turn-active prefix changed native RNG algorithm/state without native draw");
      for(std::size_t n=0;n<64;++n){const auto& raw=s.world.all_players->players[n];
       const auto& f=detail::dynamic_core(raw,"m_bTurnActive");
       need(f.encoding==saves::PlayerCoreEncoding::boolean&& &f==active_fields[n]&&f.values.data()==active_values[n]&&
        active_indexes[n]<raw.core.fields.size()&& &raw.core.fields[active_indexes[n]]==&f&&
        f.values[0]==(after_store&&static_cast<std::int32_t>(n)==player?1:active[n]),
        "Player turn-active physical flag allocation/value changed");
      }
     }
    };
    bool game_turn_player_active_attempted=false;
    std::optional<GameTurnPlayerActiveReturn> game_turn_player_active_return;
    bool player_dirty_attempted=false,player_dirty_resumed=false;
    std::uint64_t player_dirty_operation=0,player_dirty_serial=0;
    std::shared_ptr<const BnwPlayerTurnActiveDirtyIssuer> player_dirty_issuer;
    struct GameTurnPlayerEndTurnReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner> issuer;std::weak_ptr<const LiveSessionState> world;
     std::uint64_t generation=0,before=0,after=0,serial=0,operation=0;
     std::int64_t turn=0,active_player=-1,pitboss=0;std::int32_t player=-1;
     bool human=false,old_end=false,old_auto=false,stored=false,source_adopted=false,setter_returned=false;
     bool pitboss_queried=false;std::optional<std::int64_t> pitboss_saved;
     bool busy_required=false,processed_auto_required=false;
     BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_player_end_turn_owner;
     std::array<std::int64_t,64> end{},automatic{};
     std::array<const void*,64> end_fields{},end_values{},auto_fields{},auto_values{};
     std::array<std::size_t,64> end_indexes{},auto_indexes{};
     void require_flags(const LiveSessionState& state,bool after_store,bool earned_busy_store=false,bool earned_auto_store=false)const{
      need(state.world.all_players&&state.world.all_players->players.size()==64,"EndTurn actual64 collection absent");
      for(std::size_t n=0;n<64;++n){const auto& raw=state.world.all_players->players[n];
       const auto& e=detail::dynamic_core(raw,"m_bEndTurn");const auto& a=detail::dynamic_core(raw,"m_bAutoMoves");
       need(e.encoding==saves::PlayerCoreEncoding::boolean&&a.encoding==saves::PlayerCoreEncoding::boolean&&
        &e==end_fields[n]&&e.values.data()==end_values[n]&&end_indexes[n]<raw.core.fields.size()&& &raw.core.fields[end_indexes[n]]==&e&&
        &a==auto_fields[n]&&a.values.data()==auto_values[n]&&auto_indexes[n]<raw.core.fields.size()&& &raw.core.fields[auto_indexes[n]]==&a&&
        e.values[0]==(after_store&&(stored||earned_busy_store)&&static_cast<std::int32_t>(n)==player?0:end[n])&&a.values[0]==(earned_auto_store&&static_cast<std::int32_t>(n)==player?0:automatic[n]),
        "EndTurn physical raw-owned flag operands changed");
      }
     }
    };
    bool game_turn_player_end_turn_attempted=false;
    std::optional<GameTurnPlayerEndTurnReturn> game_turn_player_end_turn_return;
    bool player_busy_attempted=false,player_busy_returned=false,player_busy_setter_returned=false;
    std::uint64_t player_busy_operation=0,player_busy_serial=0;
    BnwRetainedTurnFrontier player_busy_next=BnwRetainedTurnFrontier::game_turn_player_end_turn_busy_owner;
    std::shared_ptr<const BnwNativePlayerBusyUnitsReturn> player_busy_return;
    bool player_busy_city_attempted=false,player_busy_city_returned=false,player_busy_city_setter_returned=false;
    std::uint64_t player_busy_city_operation=0,player_busy_city_serial=0;
    BnwRetainedTurnFrontier player_busy_city_next=BnwRetainedTurnFrontier::game_turn_player_busy_city_owner;
    std::shared_ptr<const BnwNativePlayerBusyCitiesReturn>player_busy_city_return;
    struct GameTurnEndAfterBusyReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,before=0,after=0,serial=0,operation=0;std::int32_t player=-1;
     bool old_end=false,old_auto=false,stored=false,source_adopted=false,setter_returned=false;
     BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_player_end_turn_after_busy_owner;
    };
    struct ProcessedAutoReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,before=0,after=0,serial=0,operation=0,owner_before=0,owner_after=0;
     std::int32_t player=-1;const BnwProcessedAutoMovesImportOwner*owner=nullptr;
     bool old_processed=false,auto_stored=false,processed_stored=false,source_adopted=false,setter_returned=false;
    };
    struct AttritionFirstReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,operation=0,serial=0,before=0,after=0;std::int32_t player=-1;
     bool after_processed=false,source_adopted=false;BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner;
    };
    struct AttritionMountainReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,operation=0,revision=0;bool source_adopted=false;
     BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_player_attrition_death_owner;
    };
    bool attrition_mountain_attempted=false;std::optional<AttritionMountainReturn>attrition_mountain_return;
    struct AttritionDamageReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,before=0,after=0;bool source_adopted=false;
     BnwRetainedTurnFrontier entry=BnwRetainedTurnFrontier::game_turn_player_attrition_damage_owner;
     BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_player_set_damage_garrison_owner;
    };
    bool attrition_damage_attempted=false;std::optional<AttritionDamageReturn>attrition_damage_return;
    struct AttritionGarrisonReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,revision=0;bool source_adopted=false;
     BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner;
    };
    bool attrition_garrison_attempted=false;std::optional<AttritionGarrisonReturn>attrition_garrison_return;
    struct AttritionCityStrengthReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,before=0,after=0;bool source_adopted=false;
     BnwRetainedTurnFrontier next=BnwRetainedTurnFrontier::game_turn_player_set_damage_city_info_dirty_owner;
    };
    bool attrition_city_strength_attempted=false;std::optional<AttritionCityStrengthReturn>attrition_city_strength_return;
    struct AttritionCityInfoReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,revision=0,client_after=0,city_count_after=0;
     bool setter_returned=false,update_strength_returned=false,source_adopted=false;
    };
    bool attrition_city_info_attempted=false;std::optional<AttritionCityInfoReturn>attrition_city_info_return;
    std::shared_ptr<const BnwAttritionCityInfoDirtyLoan>attrition_city_info_loan;
    struct AttritionUnitInfoReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,revision=0,client_after=0,ordinal_after=0;
     bool setter_returned=false,info_bar_returned=false,source_adopted=false;
    };
    bool attrition_unit_info_attempted=false;std::optional<AttritionUnitInfoReturn>attrition_unit_info_return;
    std::shared_ptr<const BnwAttritionUnitInfoDirtyLoan>attrition_unit_info_loan;
    struct AttritionUnitSelectedReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,revision=0;
     bool query_returned=false,result=false,source_adopted=false;
    };
    bool attrition_unit_selected_attempted=false;std::optional<AttritionUnitSelectedReturn>attrition_unit_selected_return;
    std::shared_ptr<const BnwAttritionUnitSelectedRead>attrition_unit_selected_loan;
    struct AttritionSelectedUnitInfoReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,revision=0,client_after=0,ordinal_after=0;
     bool setter_returned=false,conditional_dirty_returned=false,source_adopted=false;
    };
    bool attrition_selected_unit_info_attempted=false;std::optional<AttritionSelectedUnitInfoReturn>attrition_selected_unit_info_return;
    std::shared_ptr<const BnwAttritionSelectedUnitInfoDirtyLoan>attrition_selected_unit_info_loan;
    struct AttritionSelectionPlotReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,revision=0;
     std::optional<std::int32_t>result;std::int32_t unit_plot=-1;
     bool query_returned=false,unit_plot_returned=false,plot_list_required=false,source_adopted=false;
    };
    bool attrition_selection_plot_attempted=false;std::optional<AttritionSelectionPlotReturn>attrition_selection_plot_return;
    std::shared_ptr<const BnwAttritionSelectionPlotRead>attrition_selection_plot_loan;
    struct AttritionPlotListReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,revision=0,client_after=0,ordinal_after=0;
     bool setter_returned=false,plot_list_returned=false,source_adopted=false;
    };
    bool attrition_plot_list_attempted=false;std::optional<AttritionPlotListReturn>attrition_plot_list_return;
    std::shared_ptr<const BnwAttritionPlotListDirtyLoan>attrition_plot_list_loan;

    struct AttritionDllWrapperReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner>issuer;std::weak_ptr<const LiveSessionState>world;
     std::uint64_t generation=0,serial=0,previous=0,operation=0,revision=0;
     bool final_initialized=false,call_returned=false,source_adopted=false;
    };
    std::optional<AttritionDllWrapperReturn>attrition_dll_wrapper_return;
    bool attrition_dll_wrapper_attempted=false;
    std::shared_ptr<const BnwAttritionDamageDllWrapperPrefix>attrition_dll_wrapper_loan;
    bool attrition_first_attempted=false;std::optional<AttritionFirstReturn>attrition_first_return;
    std::shared_ptr<const BnwNativeAttritionPrefix>attrition_first;
    bool processed_auto_attempted=false;std::optional<ProcessedAutoReturn>processed_auto_return;
    bool game_turn_end_after_busy_attempted=false;std::optional<GameTurnEndAfterBusyReturn>game_turn_end_after_busy_return;
    bool game_turn_end_timer_attempted=false;
    std::optional<BnwEndTurnTimerSample> game_turn_end_timer_return;
    bool game_turn_gameplay_attempted=false;
    std::optional<GameTurnGameplayReturn> game_turn_gameplay_return;
    bool game_turn_sent_auto_attempted=false;
    struct GameTurnSentAutoReturn {
     std::weak_ptr<const BnwSelectedStockLuaSourceOwner> issuer;
     std::weak_ptr<const LiveSessionState> world;
     std::uint64_t generation=0,before=0,after=0,serial=0;
     std::int64_t turn=0;bool old_value=false,stored=false,source_adopted=false;
    };
    std::optional<GameTurnSentAutoReturn> game_turn_sent_auto_return;
    bool game_turn_ui_dirty_attempted=false;
    std::optional<ui::BnwClientGameTurnDirtyPair> game_turn_ui_dirty_return;
    std::weak_ptr<const BnwSelectedStockLuaSourceOwner> game_turn_ui_dirty_issuer;
    bool game_turn_clock_attempted=false;
    struct GameTurnClockStoreReturn {
      std::weak_ptr<const BnwSelectedStockLuaSourceOwner> issuer;
      std::weak_ptr<const LiveSessionState> world;
      std::int64_t old_turn=0,new_turn=0;
      std::uint64_t generation=0,before=0,after=0,serial=0;
      bool score_store_returned=false,source_expectation_adopted=false;
    };
    std::optional<GameTurnClockStoreReturn> game_turn_clock_return;
    std::optional<std::uint64_t> end_turn_ui_reset_return;
    std::atomic<BnwRetainedTurnFrontier> frontier{BnwRetainedTurnFrontier::timer_reset};
    bool construction_attempted=false,timer_attempted=false,selection_cycle_attempted=false,engine_utility_attempted=false,barbarians_attempted=false,game_cache_attempted=false,religion_progress_attempted=false,score_attempted=false,deals_attempted=false,first_team_war_attempted=false;
    std::optional<std::int32_t> first_team;
    bool first_team_no_trade_attempted=false;
    std::optional<bool> first_team_is_minor;
    bool first_team_small_awards_attempted=false;
    std::optional<std::uint64_t> first_team_small_award_slots;
    bool first_team_circumnavigation_attempted=false;
    std::optional<BnwRetainedCircumnavigationReturn> first_team_circumnavigation_return;
    std::optional<std::uint64_t> first_team_circumnavigation_axis;
    bool first_team_return_attempted=false;
    std::optional<std::uint64_t> first_team_tail_assertions;
    std::optional<std::int32_t> team_loop_next_slot;
    std::optional<std::int32_t> next_alive_team;
    bool next_team_war_attempted=false;
    std::optional<std::int32_t> next_team;
    std::optional<std::int32_t> active_team;
    std::optional<bool> active_team_is_minor;
    std::optional<std::uint64_t> active_team_small_award_slots;
    std::optional<BnwRetainedCircumnavigationReturn> active_team_circumnavigation_return;
    std::optional<std::uint64_t> active_team_circumnavigation_axis;
    std::uint64_t completed_teams=0;
    std::optional<std::int32_t> last_completed_team;
    bool map_attempted=false;
    std::uint64_t completed_map_plots=0;
    std::optional<std::uint64_t> active_map_plot;
    bool camps_turn_attempted=false,units_turn_attempted=false;
    std::optional<std::int32_t> camps_native_to_add;
    std::optional<bool> camps_native_wants_coastal;
    bool religion_city_turn_attempted=false;
    std::optional<std::pair<std::int32_t,std::int32_t>> active_religion_city;
    bool engine_ui_attempted=false,engine_ui_returned=false;
    std::optional<std::int32_t> engine_ui_player;
    std::optional<std::uint64_t> engine_ui_earned_revision;

    std::uint64_t operation=0,generation=0;
    const LiveSession* facade=nullptr;
    std::shared_ptr<LiveSession::RetainedFacadeLifetime> lifetime;
    std::weak_ptr<BnwRetainedTurnCursor> cursor;
    std::weak_ptr<const LiveSessionState> state;
    const saves::SavedSession* source=nullptr;
    const std::uint8_t* payload=nullptr;
    std::size_t payload_size=0;
    std::string payload_sha256;
    std::int64_t turn=0;
    std::array<std::uint8_t,16> map_guid{},engine_guid{},gamecore_guid{};
    void thread_check() {
        if(std::this_thread::get_id()!=thread){poisoned.store(true);frontier.store(BnwRetainedTurnFrontier::quarantined);
            throw std::runtime_error("Retained turn foreign thread before live borrow");}
    }
    void first_check() {
        thread_check();need(!retired.load()&&!poisoned.load(),"Retained turn retired/quarantined");
        try{need(bool(lifetime),"Retained turn unissued facade identity");lifetime->require_current();}
        catch(...){quarantine();throw;}
    }
    void quarantine() noexcept {poisoned.store(true);frontier.store(BnwRetainedTurnFrontier::quarantined);}
    void check(const std::shared_ptr<const LiveSession>& session) {
        first_check();
        try {
            need(session&&session.get()==facade,"Retained turn foreign facade");
            need(session->retained_facade_lifetime_==lifetime&&lifetime->facade==facade,
                 "Retained turn genuine facade lifetime differs");
            const auto pinned=state.lock();need(static_cast<bool>(pinned),"Retained turn original allocation expired");
            const auto current=session->read_state_lease();
            need(current&&current.get()==pinned.get()&&session->state_generation()==generation,
                 "Retained turn allocation/generation changed");
            // A moved-from facade has no source: lease check is deliberately first.
            const auto& s=session->source();
            need(&s==source&&s.source.bytes.data()==payload&&s.source.bytes.size()==payload_size,
                 "Retained turn source allocation changed");
            preflight(*current);
            if(session->debug_){
                need(session->bnw_game_debug_owner_current(),"Retained turn actual Game debug owner retired");
                (void)session->bnw_game_debug_mode();
            }
            need(current->setup.game_turn==turn&&current->world.map_header.guid==map_guid&&
                 current->world.pregame->engine_guid==engine_guid&&
                 current->world.pregame->gamecore_guid==gamecore_guid,"Retained turn game identity changed");
        }catch(...){quarantine();throw;}
    }
};
struct BnwRetainedTurnCursor::Impl {
    std::shared_ptr<History> history;
    std::shared_ptr<const LiveSession> session;
    std::shared_ptr<BnwGameTurnTimerOwner> timer;
    std::shared_ptr<ui::BnwClientSelectionControl> cycle_owner;
    std::weak_ptr<const ui::LiveUiHost> cycle_ui;
    // Installed only by the actual SelectedBnwPreviewHost overload. A generic
    // facade/canonical UI observation never supplies this source context.
    struct EngineUtilityContext final {
        std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
        std::unique_ptr<const SmallAwardCollection> small_awards;
        explicit EngineUtilityContext(std::shared_ptr<const ui::SelectedBnwPreviewHost> h)
            :host(std::move(h)){}
        void do_turn() const noexcept {
            // Original utility4 QueryInterface returns SAME this, table+0x18
            // reaches EXE 0x532F60 = C3 RET. Exact raw binding proof192:
            // EXE SHA 7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686
            // BNW DLL SHA c5483b5857a6fb7ef107af93c5d34282eb713472e2a84d0eb9d3fdc104e155a5
            // This concrete body has no original internal calls or stores.
        }
        void do_update_cache_on_turn() const noexcept {
            // Pinned Expansion2 SDK CvGame.cpp9875-9878 has an empty body.
            // This is a concrete Civis-owned source implementation. No native
            // RET/body equivalence proof is inferred from the source evidence.
        }
    };
    std::unique_ptr<EngineUtilityContext> engine_utility;
    void check_engine(){
        check();
        need(engine_utility&&engine_utility->host,"Retained utility actual selected Host context unavailable");
        const auto& host=*engine_utility->host;
        need(&host.session()==session.get(),"Retained utility foreign actual Host session");
        const auto& measured=host.gamecore_implementation_identity_;
        need(measured&&measured->bytes==4061696&&measured->sha256==
             "c5483b5857a6fb7ef107af93c5d34282eb713472e2a84d0eb9d3fdc104e155a5",
             "Retained utility actual factory GameCore implementation is not the supported stock source");
        const auto& header=session->source().source.header;
        need(header.build=="1.0.3.279 (403694)"&&header.build_id=="403694"&&header.mods.empty(),
             "Retained utility supports only stock retail403694 BNW DX9 source implementation");
        need(host.database().immutable_preview_fingerprints_current(),
             "Retained utility immutable actual Host rules unavailable");
        need(session->bnw_game_debug_owner_current(),"Retained utility actual imported Game debug owner unavailable");
        (void)session->bnw_game_debug_mode();
        need(host.identity().payload_sha256==history->payload_sha256&&
             host.identity().payload_sha256==session->debug_->identity.payload_sha256&&
             host.identity().rules_sha256==session->debug_->identity.rules_sha256&&
             host.database().gameplay_fingerprint_sha256()==host.identity().rules_sha256,
             "Retained utility actual Host/debug rules or payload identity differs");
        need(history->selection_cycle_attempted&&cycle_owner,"Retained utility actual UI predecessor absent");
        cycle_owner->require_retained_cycle_session(*session);
        check();
    }
    void check(){
        history->check(session);timer->require_current();
        if(history->selection_cycle_attempted){
            need(cycle_owner&&cycle_ui.lock(),"Retained cycle actual UI lifetime retired");
            cycle_owner->require_retained_cycle_session(*session);
        }
    }
};
std::shared_ptr<BnwRetainedTurnCursor> BnwRetainedTurnCursor::bind(std::shared_ptr<const LiveSession> session){
    need(static_cast<bool>(session),"Retained turn absent owned session");
    // Tiny process tombstones retain actual private birth identities only.
    // The same live facade has one identity across legitimate alias owners;
    // actual destruction/address recycling creates a distinct private token.
    // No weak alias expiry is used as evidence of destruction.
    static std::mutex mutex;
    static std::vector<std::shared_ptr<History>> ledger;
    const std::lock_guard lock(mutex);
    // Foreign admission must never read the non-atomic lazy token slot.
    // Quarantine only already-issued LIVE history via its cached private
    // anchor; this auxiliary denial lookup cannot authorize an operation.
    if(session->plot_yield_refresh_thread_!=std::this_thread::get_id()){
        for(const auto& previous:ledger)
            if(previous->facade==session.get()&&previous->lifetime&&
               !previous->lifetime->destroyed.load())previous->thread_check();
        throw std::runtime_error("Retained turn foreign thread before live borrow");
    }
    const auto lifetime=session->retain_bnw_facade_lifetime();
    const auto found=std::find_if(ledger.begin(),ledger.end(),[&](const auto& h){
        return h->lifetime==lifetime&&!h->lifetime.owner_before(lifetime)&&!lifetime.owner_before(h->lifetime);
    });
    if(found!=ledger.end()){
        const auto h=*found;h->first_check();h->check(session);
        if(auto current=h->cursor.lock()){current->require_current();return current;}
        // Never reconstruct a destroyed/uncertain owner or replay its prefix.
        h->quarantine();throw std::runtime_error("Retained turn existing history has no current cursor");
    }
    need(session->plot_yield_refresh_thread_==std::this_thread::get_id(),
         "Retained turn foreign session constructor thread");
    const auto state=session->read_state_lease();need(static_cast<bool>(state),"Retained turn moved/absent state");
    if(session->bnw_game_debug_owner_current())(void)session->bnw_game_debug_mode();
    // A moved world is still the same retained operation, even in a new facade.
    for(const auto& previous:ledger)if(const auto old=previous->state.lock();old&&old.get()==state.get()){
        previous->thread_check();previous->quarantine();
        throw std::runtime_error("Retained turn original world moved to another facade");
    }
    preflight(*state);
    const auto& source=session->source();
    need(!source.source.bytes.empty()&&source.snapshot.pregame&&
         source.snapshot.map_header.guid==state->world.map_header.guid&&
         source.snapshot.pregame->engine_guid==state->world.pregame->engine_guid&&
         source.snapshot.pregame->gamecore_guid==state->world.pregame->gamecore_guid,
         "Retained turn retained source identity differs");
    auto h=std::make_shared<History>();h->facade=session.get();h->lifetime=lifetime;
    h->state=state;h->generation=session->state_generation();h->source=&source;
    h->payload=source.source.bytes.data();h->payload_size=source.source.bytes.size();
    h->payload_sha256=identify_bytes(source.source.bytes).sha256;
    h->turn=state->setup.game_turn;h->map_guid=state->world.map_header.guid;
    h->engine_guid=state->world.pregame->engine_guid;h->gamecore_guid=state->world.pregame->gamecore_guid;
    h->check(session);
    auto i=std::make_shared<Impl>();i->history=h;i->session=std::move(session);
    ledger.push_back(h); // History exists before the first FTimer constructor Start.
    // Sticky session guard covers even a failed constructor Start; partial save
    // cannot silently discard timer/cursor history, including after move/restore.
    i->session->mark_bnw_retained_timer_history();
    h->construction_attempted=true;
    try {
        i->timer=BnwGameTurnTimerOwner::create(i->session);
        i->check();
        auto cursor=std::shared_ptr<BnwRetainedTurnCursor>(new BnwRetainedTurnCursor(std::move(i)));
        h->cursor=cursor;return cursor;
    }catch(...){h->quarantine();throw;}
}
std::shared_ptr<BnwRetainedTurnCursor> BnwRetainedTurnCursor::bind(
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
    need(static_cast<bool>(host),"Retained turn absent selected host");
    const auto* session=&host->session();
    auto cursor=bind(std::shared_ptr<const LiveSession>(host,session));
    cursor->require_current();
    auto& context=cursor->impl_->engine_utility;
    if(context)need(context->host.get()==host.get(),"Retained utility context belongs to another actual Host");
    else {
        need(!cursor->impl_->history->engine_utility_attempted,"Retained utility cannot reconstruct attempted context");
        context=std::make_unique<Impl::EngineUtilityContext>(std::move(host));
    }
    cursor->require_current();return cursor;
}
BnwRetainedTurnCursor::~BnwRetainedTurnCursor(){retire();}
void BnwRetainedTurnCursor::retire() noexcept {
    const auto i=impl_;i->history->retired.store(true);
    i->history->frontier.store(BnwRetainedTurnFrontier::retired);i->timer->retire();
}
void BnwRetainedTurnCursor::require_current() const {
    const auto i=impl_;try{i->check();}catch(...){i->history->quarantine();throw;}
}
BnwRetainedTurnView BnwRetainedTurnCursor::view() const {
    const auto i=impl_;auto& h=*i->history;h.thread_check();
    if(!h.retired.load()&&!h.poisoned.load())try{i->check();}catch(...){h.quarantine();}
    // A stale/retired view reads only retained history, never grants execution.
    return {h.retired.load()?BnwRetainedTurnFrontier::retired:h.frontier.load(),h.operation,h.timer_attempted,h.selection_cycle_attempted,h.engine_utility_attempted,h.barbarians_attempted,h.game_cache_attempted,h.religion_progress_attempted,h.score_attempted,h.deals_attempted,h.first_team_war_attempted,h.first_team,h.first_team_no_trade_attempted,h.first_team_is_minor,h.first_team_small_awards_attempted,h.first_team_small_award_slots,h.first_team_circumnavigation_attempted,h.first_team_circumnavigation_return,h.first_team_circumnavigation_axis,h.first_team_return_attempted,h.first_team_tail_assertions,h.team_loop_next_slot,h.next_alive_team,h.next_team_war_attempted,h.next_team,h.active_team,h.active_team_is_minor,h.active_team_small_award_slots,h.active_team_circumnavigation_return,h.active_team_circumnavigation_axis,h.completed_teams,h.last_completed_team,h.map_attempted,h.completed_map_plots,h.active_map_plot,h.camps_turn_attempted,h.units_turn_attempted,h.camps_native_to_add,h.camps_native_wants_coastal,h.religion_city_turn_attempted,h.active_religion_city,h.engine_ui_attempted,h.engine_ui_returned,h.engine_ui_player,h.engine_ui_earned_revision};
}
std::shared_ptr<const BnwGameTurnTimerOwner> BnwRetainedTurnCursor::timer_owner() const {
    const auto i=impl_;require_current();return i->timer;
}
BnwGameTurnTimerSample BnwRetainedTurnCursor::execute_timer_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(!h.timer_attempted&&h.frontier.load()==BnwRetainedTurnFrontier::timer_reset,
         "Retained turn timer prefix already attempted");
    // Private history is stored BEFORE QPC writes; failures cannot replay.
    h.timer_attempted=true;h.operation=1;
    try {
        const auto use=i->timer->reset_for_engine_begin();
        i->check();const auto sample=i->timer->read(use);i->check();
        need(sample.reset==BnwGameTurnTimerReset::engine_begin&&sample.operation!=0,
             "Retained turn timer producer did not publish the actual reset");
        h.frontier.store(BnwRetainedTurnFrontier::selection_cycle_owner_unavailable);
        return sample;
    }catch(...){h.quarantine();throw;}
}
std::shared_ptr<ui::BnwClientSelectionControl> BnwRetainedTurnCursor::require_actual_ui(
    const ui::LiveUiHost& ui) const {
    require_current();
    const auto lifetime=ui.selected_owner_lifetime_.lock();
    need(lifetime&&lifetime.get()==&ui,"Retained cycle actual LiveUiHost lifetime absent");
    const auto control=ui.city_selection_control_;
    need(static_cast<bool>(control),"Retained cycle actual canonical UI receiver absent");
    control->require_retained_cycle_session(*impl_->session);
    if(impl_->history->selection_cycle_attempted)
        need(impl_->cycle_owner==control&&impl_->cycle_ui.lock().get()==&ui,
             "Retained cycle read belongs to another UI");
    require_current();return control;
}
void BnwRetainedTurnCursor::execute_selection_cycle_prefix(const ui::LiveUiHost& ui){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.timer_attempted&&!h.selection_cycle_attempted&&h.operation==1&&
         h.frontier.load()==BnwRetainedTurnFrontier::selection_cycle_owner_unavailable,
         "Retained cycle missing actual timer predecessor or already attempted");
    // Invalid/missing/foreign UI refuses before attempt or any actual UI store.
    const auto control=require_actual_ui(ui);
    const auto sample=i->timer->read(i->timer->current_use());
    need(sample.reset==BnwGameTurnTimerReset::engine_begin&&sample.operation==1,
         "Retained cycle timer predecessor has changed");
    require_current();control->require_retained_cycle_session(*i->session);
    i->cycle_owner=control;i->cycle_ui=ui.selected_owner_lifetime_;
    h.selection_cycle_attempted=true;h.operation=2; // BEFORE actual setter stores.
    try{
        control->reset_no_selection_list_cycle();i->check();
        need(!control->no_selection_list_cycle(),"Retained cycle actual UI setter did not reset flag");
        i->check();h.frontier.store(BnwRetainedTurnFrontier::engine_utility_do_turn_unavailable);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_engine_utility_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.timer_attempted&&h.selection_cycle_attempted&&!h.engine_utility_attempted&&h.operation==2&&
         h.frontier.load()==BnwRetainedTurnFrontier::engine_utility_do_turn_unavailable,
         "Retained utility missing actual operation 2 predecessor or already attempted");
    // Unsupported source/context refuses BEFORE attempt. SAME real UI is kept
    // alive for the entire synchronous invocation, never inferred from flags.
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained utility actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);
    const auto sample=i->timer->read(i->timer->current_use());
    need(sample.reset==BnwGameTurnTimerReset::engine_begin&&sample.operation==1,
         "Retained utility actual timer predecessor has changed");
    i->check_engine();
    h.engine_utility_attempted=true;h.operation=3; // BEFORE actual concrete call.
    try{
        i->engine_utility->do_turn(); // Synchronous source-backed empty body.
        i->check_engine();
        need(require_actual_ui(*actual_ui)==control,"Retained utility actual UI receiver changed on return");
        i->check_engine();h.frontier.store(BnwRetainedTurnFrontier::barbarians_begin);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_barbarians_begin_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.engine_utility_attempted&&!h.barbarians_attempted&&h.operation==3&&
         h.frontier.load()==BnwRetainedTurnFrontier::barbarians_begin,
         "Retained barbarian missing actual operation3 predecessor or already attempted");
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained barbarian actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);
    const auto sample=i->timer->read(i->timer->current_use());
    need(sample.reset==BnwGameTurnTimerReset::engine_begin&&sample.operation==1,
         "Retained barbarian actual timer predecessor has changed");
    const auto allocation=i->session->read_state_lease();
    const auto& host=*i->engine_utility->host;
    const auto camp=resolve_bnw_barbarian_camp_improvement(host.defines());
    const auto plan=prepare_barbarians(*allocation,host,camp);
    i->check_engine();
    need(i->session->read_state_lease()==allocation,"Retained barbarian state changed before stores");
    // No GAMEOPTION_NO_BARBARIANS shortcut: BeginTurn precedes and differs
    // from the later DoCamps/DoUnits option gates in CvBarbarians.cpp261/627.
    h.barbarians_attempted=true;h.operation=4; // BEFORE genuine in-place stores.
    try{
        apply_barbarians(i->session->state_->world.supporting_classes->barbarians,plan);
        i->check_engine();
        need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
             "Retained barbarian allocation or actual UI changed after stores");
        i->check_engine();h.frontier.store(BnwRetainedTurnFrontier::game_cache_update);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_game_cache_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.barbarians_attempted&&!h.game_cache_attempted&&h.operation==4&&
         h.frontier.load()==BnwRetainedTurnFrontier::game_cache_update,
         "Retained game cache missing actual operation4 predecessor or already attempted");
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained game cache actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);
    const auto sample=i->timer->read(i->timer->current_use());
    need(sample.reset==BnwGameTurnTimerReset::engine_begin&&sample.operation==1,
         "Retained game cache actual timer predecessor has changed");
    const auto allocation=i->session->read_state_lease();i->check_engine();
    h.game_cache_attempted=true;h.operation=5; // BEFORE the concrete SDK-backed call.
    try{
        i->engine_utility->do_update_cache_on_turn();
        i->check_engine();
        need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
             "Retained game cache allocation or actual UI changed on return");
        i->check_engine();h.frontier.store(BnwRetainedTurnFrontier::world_religion_tech_progress);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_world_religion_tech_progress_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.game_cache_attempted&&!h.religion_progress_attempted&&h.operation==5&&
         h.frontier.load()==BnwRetainedTurnFrontier::world_religion_tech_progress,
         "Retained religion cache missing actual operation5 predecessor or already attempted");
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained religion cache actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);
    const auto sample=i->timer->read(i->timer->current_use());
    need(sample.reset==BnwGameTurnTimerReset::engine_begin&&sample.operation==1,
         "Retained religion cache actual timer predecessor has changed");
    const auto allocation=i->session->read_state_lease();const auto& state=*allocation;
    const auto* prefix=std::get_if<saves::Expansion2PrefixObservation>(&state.world.game_state.prefix);
    need(prefix&&prefix->version==1&&prefix->scalars.size()==29,
         "Retained religion cache actual Expansion2 prefix shape unavailable");
    const auto& host=*i->engine_utility->host;
    const auto* entry=host.defines().find("NUM_AND_TECH_PREREQS");
    need(host.defines().profile()==content::GlobalDefineProfile::bnw&&entry&&
         entry->kind==content::GlobalDefineKind::integer&&entry->lookup.found&&
         entry->lookup.sqlite_code==0&&entry->lookup.error.empty()&&!entry->retained_previous,
         "Retained religion cache actual NUM_AND_TECH_PREREQS unavailable");
    const auto* slots=std::get_if<std::int64_t>(&entry->lookup.value);
    need(slots&&*slots>0&&*slots<=1024,"Retained religion cache unsupported actual AND slot count");
    need(state.setup.players.size()==64,"Retained religion cache actual player roster incomplete");
    for(std::int64_t player=0;player<64;++player){
        const auto found=state.setup.players.find(player);
        need(found!=state.setup.players.end()&&found->second.team>=-1&&found->second.team<64,
             "Retained religion cache actual player/team identity invalid");
    }
    std::array<bool,64> seen{};
    for(const auto& team:state.technologies){
        need(team.profile()==content::TechnologyProfile::bnw&&team.team()>=0&&team.team()<64,
             "Retained religion cache owned technology team identity invalid");
        const auto index=static_cast<std::size_t>(team.team());
        need(!seen[index],"Retained religion cache duplicate owned technology team");seen[index]=true;
    }
    // Allocation/recursion/overflow and unsupported operands can throw here,
    // before the operation6 attempt and before any owned scalar/revision store.
    const auto value=compute_bnw_world_religion_tech_progress(state,host.technologies(),*slots);
    const bool changed=prefix->scalars[23]!=value;
    need(!changed||state.ui_source_revision<UINT64_MAX,"Retained religion cache UI revision exhausted");
    i->check_engine();
    need(i->session->read_state_lease()==allocation,"Retained religion cache allocation changed before stores");
    h.religion_progress_attempted=true;h.operation=6; // BEFORE both actual nonthrowing stores.
    try{
        std::get<saves::Expansion2PrefixObservation>(i->session->state_->world.game_state.prefix).scalars[23]=value;
        // Client capture currentness depends on this owned world revision. It
        // invalidates snapshots without retiring SAME input/debug/selection.
        if(changed)++i->session->state_->ui_source_revision;
        i->check_engine();
        need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
             "Retained religion cache allocation or actual UI changed after stores");
        i->check_engine();h.frontier.store(BnwRetainedTurnFrontier::game_update_score);
    }catch(...){h.quarantine();throw;}
}
bool BnwRetainedTurnCursor::no_selection_list_cycle(const ui::LiveUiHost& ui)const{
    const auto control=require_actual_ui(ui);
    const bool value=control->no_selection_list_cycle();require_current();return value;
}
void BnwRetainedTurnCursor::execute_game_score_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.religion_progress_attempted&&!h.score_attempted&&h.operation==6&&
         h.frontier.load()==BnwRetainedTurnFrontier::game_update_score,
         "Retained score missing actual operation6 predecessor or already attempted");
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained score actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);
    const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,
         "Retained score actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();const auto& host=*i->engine_utility->host;
    // All SQL/numeric/replay allocation and shape validation precedes attempt.
    // A clean flag reaches the SDK1640 early return; no score values are assumed.
    auto plan=BnwGameScorePreparedWrites::prepare(*allocation,host.database(),host.types(),host.defines());
    plan->require_same_state(*allocation);
    control->preflight_score_dirty_setters(plan->changed_player_scores());
    need(!plan->dirty()||allocation->ui_source_revision<UINT64_MAX,
         "Retained score world UI revision exhausted");
    i->check_engine();
    need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
         "Retained score allocation or SAME actual UI changed before stores");
    plan->require_same_state(*allocation);
    h.score_attempted=true;h.operation=7; // BEFORE score flag/scalars/UI/replay.
    try{
        if(plan->dirty()){
            auto& state=*i->session->state_;
            plan->clear_dirty(state); // Source1643 precedes every score setter.
            for(std::size_t rank=0;rank<63;++rank){
                if(plan->store_player_score(state,rank))control->set_score_dirty(ui::BnwClientScoreDirtyKind::score);
                plan->store_player_replay(state,rank); // After that player's UI setter.
            }
            plan->store_team_scores(state);
            ++state.ui_source_revision;
        }
        i->check_engine();
        need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
             "Retained score allocation or actual UI changed after stores");
        i->check_engine();h.frontier.store(BnwRetainedTurnFrontier::game_deals_turn);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_game_deals_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.score_attempted&&!h.deals_attempted&&h.operation==7&&
         h.frontier.load()==BnwRetainedTurnFrontier::game_deals_turn,
         "Retained deals missing actual operation7 predecessor or already attempted");
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained deals actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);
    const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,
         "Retained deals actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();const auto& state=*allocation;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    const auto& deals=state.world.history.deals;const auto& current=deals.groups[1];
    need(state.setup.profile==content::TechnologyProfile::bnw&&state.world.history.layout==bnw&&
         deals.version==1&&!deals.next_deal_id,"Retained deals actual plain BNW history unavailable");
    // SDK empty returns BEFORE reading GameTurn, players, items or resources.
    std::int64_t turn=0,last_from=-1,last_to=-1;std::size_t migrated=0;
    bool ending=false,renewal_changed=false,dirty_ui=false;
    if(!current.empty()){
        need(state.world.pregame&&state.world.pregame->layout==bnw,
             "Retained deals actual Pregame clock source unavailable");
        const auto found=state.world.pregame->archive.integers.find("game_turn");
        need(found!=state.world.pregame->archive.integers.end()&&found->second>=0&&
             found->second<=INT32_MAX&&found->second==state.setup.game_turn,
             "Retained deals raw and owned GameTurn differ");turn=found->second;
        // Finish the ENTIRE expiration scan BEFORE any resource getter/validity scan. Missing
        // mandatory ending owners refuse, not successful skips or DTO effects.
        for(const auto& deal:current){
            need(deal.version&&*deal.version>=1&&*deal.version<=3&&!deal.id&&
                 deal.cancelled&&deal.from_player>=0&&deal.from_player<64&&
                 deal.to_player>=0&&deal.to_player<64&&deal.from_player!=deal.to_player&&
                 deal.final_turn>=INT32_MIN&&deal.final_turn<=INT32_MAX,
                 "Retained current deal source identity/layout unavailable");
            for(const auto& item:deal.items){
                need(item.final_turn>=-1&&item.final_turn<=INT32_MAX&&
                     item.type>=INT32_MIN&&item.type<=INT32_MAX&&
                     (item.from_player==deal.from_player||item.from_player==deal.to_player)&&
                     item.from_renewed&&item.to_renewed,
                     "Retained current deal item source layout unavailable");
                if(item.final_turn>-1&&item.final_turn==turn){
                    // Source DoEnd reads actual getTeam FROM/TO BEFORE renewal
                    // clear and early return; no saved m_eTeam/preview actor.
                    last_from=item.from_player;
                    last_to=last_from==deal.from_player?deal.to_player:deal.from_player;
                    (void)detail::bnw_player_team_read(state,static_cast<std::int32_t>(last_from));
                    (void)detail::bnw_player_team_read(state,static_cast<std::int32_t>(last_to));
                    need(*item.from_renewed,
                         "Retained non-renewed expiry requires complete DoEndTradedItem money/resource/team/notification effects");
                    ending=true;renewal_changed=renewal_changed||*item.to_renewed;
                }
            }
        }
        const auto& host=*i->engine_utility->host;
        for(const auto& deal:current){
            bool invalid=false,unbreakable=false;
            for(const auto& item:deal.items){
                if(item.final_turn<0||item.final_turn==turn)continue;
                last_from=item.from_player; // SDK EVERY reached item, including inert/peace.
                if(item.type==3){
                    // No externally supplied total/capability. THIS same private
                    // cursor binds the numerical read to actual Host/state/rules.
                    i->check_engine();
                    const auto total=compute_bnw_deal_resource_total_diagnostic(
                        state,host.database(),host.types(),host.defines(),item.from_player,item.data[0]);
                    i->check_engine();
                    need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                         "Retained resource getter SAME world/UI changed on return");
                    if(total<0)invalid=true;
                }
                else if(item.type==13){unbreakable=true;break;}
            }
            // A LATER reachable peace can suppress an earlier negative result.
            // All earlier reached resource getters still run in source order.
            need(unbreakable||!invalid,
                 "Retained negative resource validity requires actual deal cancellation and ALL DoEndTradedItem effects");
            if(deal.final_turn<=turn)++migrated;
        }
        if(ending){
            const auto active=detail::bnw_game_active_player_read(state);
            dirty_ui=last_from==active||last_to==active;
            if(dirty_ui)control->preflight_game_data_dirty_setter();
        }
        need(migrated<=deals.groups[2].max_size()-deals.groups[2].size(),
             "Retained historical deal list cannot append all source rows");
        need(!(migrated||renewal_changed)||state.ui_source_revision<UINT64_MAX,
             "Retained deal list migration UI revision exhausted");
    }
    // Pure preflight above resolves mandatory producers BEFORE an attempt.
    // Source207 no-ending copy remains before attempt. The renewed branch must
    // copy AFTER real renewal/UI effects, as actual DoUpdateCurrentDealsList.
    std::vector<saves::DealObservation> temp;
    if(!ending)temp=current;
    i->check_engine();
    need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
         "Retained deals SAME allocation/UI changed before stores");
    h.deals_attempted=true;h.operation=8; // BEFORE any actual renewal/UI/list store.
    try{
        auto& live=i->session->state_->world.history.deals;
        if(migrated||renewal_changed)++i->session->state_->ui_source_revision;
        if(ending){
            std::int64_t actual_from=-1,actual_to=-1;
            // Reached actual source expiry calls in original list/item order.
            for(auto& deal:live.groups[1])for(auto& item:deal.items){
                if(item.final_turn>-1&&item.final_turn==turn){
                    const auto to=item.from_player==deal.from_player?deal.to_player:deal.from_player;
                    actual_from=item.from_player;actual_to=to;
                    (void)detail::bnw_player_team_read(state,static_cast<std::int32_t>(item.from_player));
                    (void)detail::bnw_player_team_read(state,static_cast<std::int32_t>(to));
                    item.to_renewed=false; // Actual source store BEFORE from-renewed early return.
                    need(item.from_renewed&&*item.from_renewed,"Retained preflight renewal provenance changed");
                }
            }
            // Actual reached resource reads follow ALL actual expiry stores.
            // Preflight numerical checks do not supply their result authority.
            const auto& host=*i->engine_utility->host;
            for(const auto& deal:live.groups[1]){
                bool invalid=false,peace=false;
                for(const auto& item:deal.items){
                    if(item.final_turn<0||item.final_turn==turn)continue;
                    actual_from=item.from_player;
                    if(item.type==3){i->check_engine();
                        if(compute_bnw_deal_resource_total_diagnostic(state,host.database(),host.types(),host.defines(),item.from_player,item.data[0])<0)invalid=true;
                        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                            "Retained renewed resource getter SAME world/UI changed on return");
                    }else if(item.type==13){peace=true;break;}
                }
                need(peace||!invalid,"Retained preflight resource validity changed after renewal effects");
            }
            const auto actual_active=detail::bnw_game_active_player_read(state);
            const bool actual_dirty=actual_from==actual_active||actual_to==actual_active;
            need(actual_from==last_from&&actual_to==last_to&&actual_dirty==dirty_ui,
                 "Retained GameData LAST players/actual Game active-player provenance changed");
            if(actual_dirty)control->set_game_data_dirty(ui::BnwClientGameDataDirtyKind::game_data);
            // SDK migration temp copy AFTER ending, validity and actual UI.
            temp=live.groups[1]; // Allocation failure quarantines attempted prefix.
        }
        if(!temp.empty()){
            static_assert(std::is_nothrow_move_constructible_v<saves::DealObservation>);
            // Old frames invalid before observable renewal/list stores; actual
            // GameData setter additionally increments canonical UI revision.
            live.groups[1].clear();
            for(auto& deal:temp){
                if(deal.final_turn<=turn)live.groups[2].push_back(std::move(deal));
                else live.groups[1].push_back(std::move(deal));
            }
        }
        // Only genuine renewed early-return endings were admitted. Remaining
        // money/resource/team/league/notification effects were not skipped;
        // their non-renewed branches refused before an attempted operation.
        i->check_engine();
        need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
             "Retained deals SAME allocation/UI changed after stores");
        i->check_engine();h.frontier.store(BnwRetainedTurnFrontier::teams_turn);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_first_alive_team_war_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.deals_attempted&&!h.first_team_war_attempted&&h.operation==8&&
         h.frontier.load()==BnwRetainedTurnFrontier::teams_turn,
         "Retained first-team war missing actual operation8 predecessor or already attempted");
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained first-team actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);
    const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,
         "Retained first-team actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();const auto& state=*allocation;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(state.setup.profile==content::TechnologyProfile::bnw&&state.world.teams&&
         state.world.teams->layout==bnw&&state.world.teams->dimensions.teams==64&&
         state.world.teams->dimensions.civ_teams==63&&state.world.teams->dimensions.major==22&&
         state.world.teams->dimensions.profile_extensions.values.empty()&&state.world.teams->teams.size()==64,
         "Retained first-team complete actual BNW 64-team roster unavailable");
    const auto field=[](const saves::TeamObservation& team,std::string_view name){
        const saves::NamedSaveInteger* found=nullptr;
        for(const auto& row:team.fields)if(row.name&&std::string_view(row.name)==name){
            need(!found,"Retained first-team duplicate raw field");found=&row;
        }
        need(found&&found->value>=INT32_MIN&&found->value<=INT32_MAX,"Retained first-team raw source-i32 field unavailable");
        return static_cast<std::int32_t>(found->value);
    };
    const auto& roster=state.world.teams->teams;std::optional<std::int32_t> first;
    // Exact source outer loop: inspect slots in ascending order and stop at
    // the first alive-count >0. No player-derived count or supplied team ID.
    for(std::int32_t slot=0;slot<64;++slot){
        const auto& team=roster[static_cast<std::size_t>(slot)];
        need(team.index==static_cast<std::uint64_t>(slot)&&field(team,"id")==slot,
             "Retained first-team actual slot/GetID differs");
        if(field(team,"alive_count")>0){first=slot;break;}
    }
    need(first.has_value(),"Retained no-alive-team branch requires actual map-turn producer; war prefix unavailable");
    need(*first!=63,"Retained first alive barbarian team requires actual DoBarbarianTech BEFORE war/minor phase");
    const auto index=static_cast<std::size_t>(*first);const auto plan=prepare_team_war(state,*first);
    const auto& war=*plan.war;const auto& locked=*plan.locked;const bool changed=plan.changed;
    // Resolve private scalar destinations BEFORE attempt; never mutable_state()
    // or a clone/replacement of world/team/vector/array storage.
    auto& live=i->session->state_->world.teams->teams[index];
    std::vector<std::int64_t>* live_war=nullptr;std::vector<std::int64_t>* live_locked=nullptr;
    for(auto& row:live.arrays){if(&row==&war)live_war=&row.values;if(&row==&locked)live_locked=&row.values;}
    need(live_war&&live_locked&&live_war->data()==war.values.data()&&live_locked->data()==locked.values.data(),
         "Retained first-team SAME scalar-array destinations differ");
    i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                          "Retained first-team SAME world/UI changed before stores");
    h.first_team_war_attempted=true;h.first_team=*first;h.operation=9; // BEFORE any actual store.
    try{
        if(changed)++i->session->state_->ui_source_revision; // BEFORE first observable scalar mutation.
        apply_team_war(*live_war,*live_locked,plan);
        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                              "Retained first-team SAME world/UI changed after stores");
        h.frontier.store(BnwRetainedTurnFrontier::first_team_minor_technology);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_next_alive_team_war_prefix(){execute_team_war_prefix(true);}
void BnwRetainedTurnCursor::execute_team_war_prefix(bool first_next){
    const auto i=impl_;require_current();auto& h=*i->history;
    if(first_next){
    need(h.first_team_return_attempted&&h.first_team&&h.operation==13&&!h.next_team_war_attempted&&
         h.frontier.load()==BnwRetainedTurnFrontier::remaining_teams_turn&&h.team_loop_next_slot&&h.next_alive_team&&
         *h.team_loop_next_slot==*h.next_alive_team&&*h.next_alive_team>*h.first_team&&*h.next_alive_team<64,
         "Retained next-team war missing actual operation13 pending alive slot or already attempted");
    }else{
        need(h.first_team_return_attempted&&h.next_team_war_attempted&&!h.active_team&&h.last_completed_team&&
             h.completed_teams>=2&&h.operation>=18&&h.operation<UINT64_MAX&&h.team_loop_next_slot&&h.next_alive_team&&
             *h.team_loop_next_slot==*h.next_alive_team&&*h.next_alive_team>*h.last_completed_team&&*h.next_alive_team<64&&
             h.frontier.load()==BnwRetainedTurnFrontier::remaining_teams_turn,
             "Retained pending war missing ascending actual completed-team source boundary or already attempted");
    }
    const auto previous=first_next?h.first_team:h.last_completed_team;
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained next-team actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,"Retained next-team actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();const auto& state=*allocation;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(state.setup.profile==content::TechnologyProfile::bnw&&state.world.teams&&state.world.teams->layout==bnw&&
         state.world.teams->dimensions.teams==64&&state.world.teams->dimensions.civ_teams==63&&
         state.world.teams->dimensions.major==22&&state.world.teams->dimensions.profile_extensions.values.empty()&&
         state.world.teams->teams.size()==64,"Retained next-team complete actual BNW roster unavailable");
    // Re-observe the SAME outer source scan. No caller chosen team or skip of
    // an intervening alive slot; op13 recorded the actual pending loop boundary.
    std::optional<std::int32_t> next;
    for(std::int32_t slot=*previous+1;slot<=*h.team_loop_next_slot;++slot){
        const auto& team=state.world.teams->teams[static_cast<std::size_t>(slot)];
        need(team.index==static_cast<std::uint64_t>(slot)&&team_war_field(team,"id")==slot,
             "Retained next-team actual pending slot/GetID differs");
        if(team_war_field(team,"alive_count")>0){next=slot;break;}
    }
    need(next==h.next_alive_team,"Retained next-team actual ascending alive scan changed BEFORE operation14");
    // SDK isBarbarian precedes war counters; a due barbarian cannot receive
    // nonbarbarian stores or an acknowledged technology-call skip.
    need(*next!=63,"Retained next alive barbarian requires mandatory DoBarbarianTech BEFORE operation14; unavailable");
    const auto plan=prepare_team_war(state,*next);const auto index=static_cast<std::size_t>(*next);
    auto& live=i->session->state_->world.teams->teams[index];
    std::vector<std::int64_t>* war=nullptr;std::vector<std::int64_t>* locked=nullptr;
    for(auto& row:live.arrays){if(&row==plan.war)war=&row.values;if(&row==plan.locked)locked=&row.values;}
    need(war&&locked&&war->data()==plan.war->values.data()&&locked->data()==plan.locked->values.data(),
         "Retained next-team SAME scalar-array destinations differ");
    i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                          "Retained next-team SAME world/UI changed before stores");
    if(first_next){h.next_team_war_attempted=true;h.next_team=*next;h.operation=14;}else ++h.operation;
    h.active_team=*next; // BEFORE revision/first scalar store, source phase retained.
    h.active_team_is_minor.reset();h.active_team_small_award_slots.reset();
    h.active_team_circumnavigation_return.reset();h.active_team_circumnavigation_axis.reset();
    try{
        if(plan.changed)++i->session->state_->ui_source_revision;
        apply_team_war(*war,*locked,plan);
        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                              "Retained next-team SAME world/UI changed after stores");
        h.frontier.store(BnwRetainedTurnFrontier::next_team_minor_technology);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_first_team_nonminor_no_trade_prefix(){execute_team_no_trade_prefix(true);}
void BnwRetainedTurnCursor::execute_team_no_trade_prefix(bool first){
    const auto i=impl_;require_current();auto& h=*i->history;
    if(first){
    need(h.first_team_war_attempted&&h.first_team&&!h.first_team_no_trade_attempted&&h.operation==9&&
         h.frontier.load()==BnwRetainedTurnFrontier::first_team_minor_technology,
         "Retained first-team no-trade missing actual operation9 predecessor or already attempted");
    }else{
        need(h.first_team_return_attempted&&h.next_team_war_attempted&&h.active_team&&h.last_completed_team&&
             *h.active_team>*h.last_completed_team&&*h.active_team<63&&h.operation>=14&&h.operation<UINT64_MAX&&
             h.frontier.load()==BnwRetainedTurnFrontier::next_team_minor_technology,
             "Retained pending team phase missing SAME active team/predecessor or already attempted");
    }
    const auto source_team=first?h.first_team:h.active_team;
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained first-team no-trade actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,
         "Retained first-team no-trade actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();const auto& state=*allocation;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(*source_team>=0&&*source_team<63&&state.world.teams&&state.world.teams->layout==bnw&&
         state.world.teams->teams.size()==64&&state.world.pregame&&state.world.pregame->layout==bnw&&
         state.world.all_players&&state.world.all_players->layout==bnw&&
         state.world.all_players->context.player_count==64&&state.world.all_players->players.size()==64,
         "Retained first-team no-trade actual team/Pregame/player roster unavailable");
    const auto index=static_cast<std::size_t>(*source_team);const auto& team=state.world.teams->teams[index];
    const auto field=[](const saves::TeamObservation& team,std::string_view name){
        const saves::NamedSaveInteger* found=nullptr;
        for(const auto& row:team.fields)if(row.name&&std::string_view(row.name)==name){need(!found,"Duplicate no-trade team field");found=&row;}
        need(found&&found->value>=INT32_MIN&&found->value<=INT32_MAX,"No-trade source-i32 team field unavailable");
        return static_cast<std::int32_t>(found->value);
    };
    need(team.index==index&&field(team,"id")==*source_team&&field(team,"alive_count")>0,
         "Retained first-team no-trade raw GetID/alive source changed");
    const auto& archive=state.world.pregame->archive;
    const auto teams=archive.integer_arrays.find("team_types");const auto minors=archive.flag_arrays.find("minor_nation_civs");
    need(teams!=archive.integer_arrays.end()&&teams->second.size()==64&&
         minors!=archive.flag_arrays.end()&&minors->second.size()==64,
         "Retained first-team source Pregame team/minor slots unavailable");
    const auto scalar=[](const saves::SavedPlayerObservation& player,std::string_view name){
        const saves::PlayerCoreField* found=nullptr;
        for(const auto& row:player.core.fields)if(row.name&&std::string_view(row.name)==name){need(!found,"Duplicate source player scalar");found=&row;}
        need(found&&found->values.size()==1,"Retained first-team source player scalar unavailable");return found->values[0];
    };
    bool minor=false;
    // SDK isMinorCiv: first living player22..63 with Pregame team==GetID.
    // The saved m_eTeam and a caller-supplied/mirrored minor flag are not used.
    for(std::size_t slot=22;slot<64;++slot){
        const auto& player=state.world.all_players->players[slot];
        need(player.index==slot&&player.saved_player_id==static_cast<std::int64_t>(slot)&&
             scalar(player,"m_eID")==static_cast<std::int64_t>(slot),"Retained first-team actual player identity differs");
        const auto alive=scalar(player,"m_bAlive");need(alive==0||alive==1,"Retained first-team alive source boolean unavailable");
        if(alive){need(teams->second[slot]>=0&&teams->second[slot]<64,"Retained first-team alive player's source team unavailable");
            if(teams->second[slot]==*source_team){minor=minors->second[slot];break;}}
    }
    if(first)need(!minor,"Retained actual minor team requires mandatory DoMinorCivTech/setHasTech world/Lua effects BEFORE SetNoTrade; producer unavailable");
    else if(minor)need(!first_minor_tech_acquisition(state,*i->engine_utility->host,*source_team),
         "Retained pending minor reached DoMinorCivTech/setHasTech world/Lua effects mandatory BEFORE attempt; unavailable");
    const auto& host=*i->engine_utility->host;const auto& rules=host.technologies();const auto& types=host.types();
    need(rules.profile==content::TechnologyProfile::bnw&&rules.all_succeeded()&&types.report().all_succeeded()&&
         types.report().hash_collisions==0&&state.technologies.size()==64,
         "Retained first-team actual technology rules/catalog unavailable");
    const auto& owned=state.technologies[index];const auto slots=rules.collection.slots;
    need(owned.profile()==rules.profile&&owned.team()==*source_team&&owned.slots()==slots&&slots<=INT32_MAX&&
         owned.profile_extensions().values.empty()&&owned.remap().all_mapped()&&owned.remap().duplicates==0&&owned.remap().holes==0&&
         field(team,"tech_version")==1,"Retained first-team owned technology identity/remap unavailable");
    const auto saved=field(team,"saved_techs");need(saved>=0,"Retained first-team saved technology count invalid");
    const auto array=[&](std::string_view name)->const saves::PlotArrayObservation&{
        const saves::PlotArrayObservation* found=nullptr;
        for(const auto& row:team.arrays)if(row.name&&std::string_view(row.name)==name){need(!found,"Duplicate no-trade technology array");found=&row;}
        need(found&&found->columns==1&&found->values.size()==static_cast<std::size_t>(saved),"Retained first-team technology row span unavailable");return *found;
    };
    const auto& hashes=array("tech_hashes");const auto& raw=array("no_trade_tech");
    std::vector<std::size_t> rows(static_cast<std::size_t>(slots),SIZE_MAX);bool changed=false;
    // This operation's admission rejects collisions/unknown/duplicate hashes;
    // never infer reset-zero for an absent required actual slot or last-wins row.
    for(std::size_t row=0;row<hashes.values.size();++row){
        const auto hash=hashes.values[row];need(hash>=0&&hash<=UINT32_MAX&&(raw.values[row]==0||raw.values[row]==1),
                                                "Retained first-team technology hash/boolean outside source width");
        if(hash==0){need(raw.values[row]==0,"Retained first-team hashzero no-trade flag is not owned");continue;}
        const auto* type=types.find_hash(static_cast<std::uint64_t>(hash));
        need(type&&type->table=="Technologies"&&type->id>=0&&static_cast<std::uint64_t>(type->id)<slots,
             "Retained first-team unknown/foreign technology hash unavailable");
        const auto slot=static_cast<std::size_t>(type->id);const auto* rule=rules.find(slot);
        need(rule&&rule->base.type==type->type&&rows[slot]==SIZE_MAX,
             "Retained first-team technology rule/duplicate last-wins raw row is not admitted");
        need(owned.at(type->id).no_trade==(raw.values[row]!=0),"Retained first-team raw/owned no-trade disagree");
        rows[slot]=row;changed=changed||raw.values[row]!=0;
    }
    for(std::size_t slot=0;slot<rows.size();++slot)need(rows[slot]!=SIZE_MAX&&rules.find(slot),
         "Retained first-team actual technology slot missing; default zero cannot authorize SetNoTrade");
    need(!changed||state.ui_source_revision<UINT64_MAX,"Retained first-team no-trade revision exhausted");
    auto& live_owned=i->session->state_->technologies[index];std::vector<std::int64_t>* live_raw=nullptr;
    for(auto& row:i->session->state_->world.teams->teams[index].arrays)if(&row==&raw)live_raw=&row.values;
    need(&live_owned==&owned&&live_raw&&live_raw->data()==raw.values.data(),"Retained first-team SAME raw/owned destinations differ");
    i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                          "Retained first-team SAME no-trade world/UI changed before stores");
    if(first){h.first_team_no_trade_attempted=true;h.first_team_is_minor=false;h.operation=10;}
    else{h.active_team_is_minor=minor;++h.operation;}
    try{
        if(changed)++i->session->state_->ui_source_revision; // BEFORE first owned/raw observable store.
        for(std::size_t slot=0;slot<rows.size();++slot){
            if((*live_raw)[rows[slot]]!=0){
                // oldtrue necessarily has an existing owned map node. Clearing
                // updates/erases that node, never inserts a newly allocated node.
                live_owned.set_no_trade(static_cast<std::int64_t>(slot),false);
                (*live_raw)[rows[slot]]=0;
            }
        }
        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                              "Retained first-team SAME no-trade world/UI changed after stores");
        h.frontier.store(first?BnwRetainedTurnFrontier::first_team_small_awards:BnwRetainedTurnFrontier::pending_team_small_awards);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_first_team_small_awards_prefix(){execute_team_small_awards_prefix(true);}
void BnwRetainedTurnCursor::execute_team_small_awards_prefix(bool first){
    const auto i=impl_;require_current();auto& h=*i->history;
    if(first){
    need(h.first_team_no_trade_attempted&&h.first_team&&h.first_team_is_minor==false&&
         !h.first_team_small_awards_attempted&&h.operation==10&&
         h.frontier.load()==BnwRetainedTurnFrontier::first_team_small_awards,
         "Retained small-awards missing actual operation10 predecessor or already attempted");
    }else{
        need(h.first_team_return_attempted&&h.next_team_war_attempted&&h.active_team&&h.last_completed_team&&
             *h.active_team>*h.last_completed_team&&*h.active_team<63&&h.operation>=14&&h.operation<UINT64_MAX&&
             h.frontier.load()==BnwRetainedTurnFrontier::pending_team_small_awards,
             "Retained pending team phase missing SAME active team/predecessor or already attempted");
    }
    const auto source_team=first?h.first_team:h.active_team;
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained small-awards actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,
         "Retained small-awards actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();const auto& state=*allocation;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(*source_team>=0&&*source_team<63&&state.world.teams&&state.world.teams->layout==bnw&&
         state.world.teams->teams.size()==64&&state.world.pregame&&state.world.pregame->layout==bnw&&
         state.world.all_players&&state.world.all_players->layout==bnw&&
         state.world.all_players->context.player_count==64&&state.world.all_players->players.size()==64,
         "Retained small-awards actual team/Pregame/player roster unavailable");
    need(state.world.teams->dimensions.teams==64&&state.world.teams->dimensions.players==64&&
         state.world.teams->dimensions.civ_teams==63&&state.world.teams->dimensions.major==22&&
         state.world.teams->dimensions.profile_extensions.values.empty(),
         "Retained small-awards actual stock team source dimensions unavailable");
    const auto index=static_cast<std::size_t>(*source_team);const auto& team=state.world.teams->teams[index];
    const auto field=[](const auto& rows,std::string_view name){
        const saves::NamedSaveInteger* found=nullptr;
        for(const auto& row:rows)if(row.name&&std::string_view(row.name)==name){need(!found,"Duplicate small-award source field");found=&row;}
        need(found&&found->value>=INT32_MIN&&found->value<=INT32_MAX,"Small-award source-i32 field unavailable");
        return static_cast<std::int32_t>(found->value);
    };
    need(team.index==index&&field(team.fields,"id")==*source_team&&field(team.fields,"alive_count")>0,
         "Retained small-awards raw GetID/alive source changed");
    const auto& archive=state.world.pregame->archive;
    const auto teams=archive.integer_arrays.find("team_types");const auto minors=archive.flag_arrays.find("minor_nation_civs");
    need(teams!=archive.integer_arrays.end()&&teams->second.size()==64&&minors!=archive.flag_arrays.end()&&minors->second.size()==64,
         "Retained small-awards actual Pregame team/minor slots unavailable");
    const auto scalar=[](const saves::SavedPlayerObservation& player,std::string_view name){
        const saves::PlayerCoreField* found=nullptr;
        for(const auto& row:player.core.fields)if(row.name&&std::string_view(row.name)==name){need(!found,"Duplicate small-award player scalar");found=&row;}
        need(found&&found->values.size()==1&&found->values[0]>=INT32_MIN&&found->values[0]<=INT32_MAX,
             "Retained small-award player source-i32 unavailable");return static_cast<std::int32_t>(found->values[0]);
    };
    const auto player_at=[&](std::size_t slot)->const saves::SavedPlayerObservation&{
        const auto& player=state.world.all_players->players[slot];
        need(player.index==slot&&player.saved_player_id==static_cast<std::int64_t>(slot)&&scalar(player,"m_eID")==static_cast<std::int64_t>(slot),
             "Retained small-award actual player identity differs");return player;
    };
    bool minor=false;
    for(std::size_t slot=22;slot<64;++slot){const auto& player=player_at(slot);const auto alive=scalar(player,"m_bAlive");
        need(alive==0||alive==1,"Retained small-award alive source boolean unavailable");
        if(alive){need(teams->second[slot]>=0&&teams->second[slot]<64,"Retained small-award alive player's team unavailable");
            if(teams->second[slot]==*source_team){minor=minors->second[slot];break;}}}
    if(first)need(!minor,"Retained small-award actual isMinorCiv differs from operation10");
    else need(h.active_team_is_minor.has_value()&&minor==*h.active_team_is_minor,"Retained pending SmallAwards actual minor query differs from no-trade phase");
    if(minor){
        // Source4863-4865 is an actual early return BEFORE any award info lookup.
        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                              "Retained pending minor SmallAwards SAME world/UI changed before early return");
        ++h.operation; // BEFORE executing the admitted source no-effect return.
        try{
            i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
                                  "Retained pending minor SmallAwards SAME world/UI changed after early return");
            h.frontier.store(BnwRetainedTurnFrontier::pending_team_circumnavigation);
        }catch(...){h.quarantine();throw;}
        return;
    }
    const auto& host=*i->engine_utility->host;const auto& types=host.types();
    need(types.report().all_succeeded()&&types.report().hash_collisions==0,"Retained small-award actual type catalog incomplete");
    auto& retained=i->engine_utility->small_awards;
    if(!retained)retained=small_award_collection(host);
    const auto* occurrence=retained.get();const auto& rules=*retained;
    const saves::TeamHashedArray* awards=nullptr;
    for(const auto& row:team.hashes)if(row.name&&std::string_view(row.name)=="small_award_achieved"){
        need(!awards,"Retained small-award duplicate hashed array");awards=&row;}
    need(awards&&awards->wire_count>=0&&static_cast<std::uint64_t>(awards->wire_count)==awards->rows.size()&&
         static_cast<std::uint64_t>(awards->wire_count)==rules.report.slots,
         "Retained small-award actual hashed array/wire span unavailable");
    std::vector<std::optional<bool>> achieved(rules.slots.size());
    for(const auto& row:awards->rows){
        need(row.type_hash<=UINT32_MAX,"Retained small-award type hash outside source width");
        if(!row.type_hash){need(row.values.empty(),"Retained small-award hashzero has a fabricated value");continue;}
        const auto* type=types.find_hash(row.type_hash);
        need(type&&type->table=="SmallAwards"&&type->id>=0&&static_cast<std::uint64_t>(type->id)<rules.slots.size(),
             "Retained small-award unknown/foreign hash unavailable");
        const auto slot=static_cast<std::size_t>(type->id);
        need(rules.slots[slot]&&rules.slots[slot]->type==type->type&&!achieved[slot]&&row.values.size()==1&&
             (row.values[0]==0||row.values[0]==1),"Retained small-award raw slot duplicate/boolean/info occurrence differs");
        achieved[slot]=row.values[0]!=0;
    }
    for(std::size_t slot=0;slot<rules.slots.size();++slot)if(rules.slots[slot])need(achieved[slot].has_value(),
         "Retained small-award actual slot missing; reset-false cannot authorize phase");
    // SAME actual city observations are reconstructed only as borrowed slot
    // pointers. GetCount is last_index-free_list_count+1, NOT cities.size().
    const auto city_scalar=[](const saves::SavedCityObservation& city,std::string_view name){
        const saves::CityCoreField* found=nullptr;
        for(const auto& row:city.core.fields)if(row.name&&std::string_view(row.name)==name){need(!found,"Duplicate small-award city scalar");found=&row;}
        need(found&&found->encoding==saves::CityCoreEncoding::i32&&found->values.size()==1&&
             found->values[0]>=INT32_MIN&&found->values[0]<=INT32_MAX,"Retained small-award source-i32 city scalar unavailable");
        return static_cast<std::int32_t>(found->values[0]);
    };
    const auto cities_of=[&](std::size_t player_slot){
        const auto& cities=player_at(player_slot).cities;const auto& header=cities.header;
        need(cities.layout==bnw&&header.layout==bnw&&header.source_slots<=8192&&
             header.next_free_indices.size()==header.source_slots,"Retained small-award actual BNW city collection unavailable");
        const auto last=field(header.fields,"last_index"),free=field(header.fields,"free_list_count"),head=field(header.fields,"free_list_head"),current=field(header.fields,"current_id");
        const auto authored=field(header.fields,"num_slots");
        need(authored>=0&&static_cast<std::uint64_t>(authored)<=header.source_slots&&
             (header.source_slots==0?authored==0:header.source_slots==std::bit_ceil(static_cast<std::uint64_t>(authored)))&&
             last>=-1&&static_cast<std::int64_t>(last)<static_cast<std::int64_t>(header.source_slots)&&free>=0&&free<=last+1&&
             current>0&&(current&8191)==0,"Retained small-award city free-list source metadata unavailable");
        const auto count=static_cast<std::int64_t>(last)-free+1;
        need(count==header.wire_city_count&&count>=0&&static_cast<std::uint64_t>(count)==cities.cities.size(),
             "Retained small-award source GetCount and actual city collection disagree");
        std::vector<const saves::SavedCityObservation*> slots(static_cast<std::size_t>(last+1),nullptr);
        for(std::size_t row=0;row<cities.cities.size();++row){const auto& city=cities.cities[row];const auto id=city_scalar(city,"m_iID");const auto bucket=static_cast<std::size_t>(id&8191);
            need(city.index==row&&city.core.layout==bnw&&id>=0&&(id&~8191)>0&&(id&~8191)<current&&
                 bucket<slots.size()&&!slots[bucket]&&header.next_free_indices[bucket]==-1&&
                 city_scalar(city,"m_eOwner")==static_cast<std::int32_t>(player_slot),
                 "Retained small-award actual city Load identity/owner/bucket unavailable");slots[bucket]=&city;}
        std::vector<bool> visited(slots.size(),false);auto next=head;
        for(std::int32_t n=0;n<free;++n){need(next>=0&&next<=last&&!slots[next]&&!visited[next],
             "Retained small-award city free-list chain unavailable");visited[next]=true;
             const auto raw=header.next_free_indices[next];need(raw>=INT32_MIN&&raw<=INT32_MAX,"Retained small-award free link outside i32");next=static_cast<std::int32_t>(raw);}
        need(next==-1,"Retained small-award city free-list chain not terminated");
        for(std::size_t bucket=0;bucket<slots.size();++bucket)need(static_cast<bool>(slots[bucket])!=visited[bucket],
             "Retained small-award city source iteration membership incomplete");
        return std::pair{count,std::move(slots)};
    };
    // Source slot -> NumCities -> major players -> CityPopulation -> major
    // players -> source bucket order. Dead players on this team are NOT skipped.
    // No break/dedup after qualification: reached writes remain mandatory.
    for(std::size_t slot=0;slot<rules.slots.size();++slot){if(!rules.slots[slot]||*achieved[slot])continue;
        const auto& rule=*rules.slots[slot];
        if(rule.cities>0)for(std::size_t player=0;player<22;++player){(void)player_at(player);
            need(teams->second[player]>=-1&&teams->second[player]<64,"Retained small-award major source team unavailable");
            if(teams->second[player]==*source_team){const auto cities=cities_of(player);
                need(cities.first<rule.cities,"Retained reached small-award requires mandatory award/victory_points/notifications/checkpoint owner BEFORE operation11; unavailable");}}
        if(rule.population>0)for(std::size_t player=0;player<22;++player){(void)player_at(player);
            need(teams->second[player]>=-1&&teams->second[player]<64,"Retained small-award major source team unavailable");
            if(teams->second[player]==*source_team){const auto cities=cities_of(player);
                for(const auto* city:cities.second)if(city)need(city_scalar(*city,"m_iPopulation")<rule.population,
                    "Retained reached small-award requires mandatory award/victory_points/notifications/checkpoint owner BEFORE operation11; unavailable");}}
    }
    const auto revision=state.ui_source_revision;
    i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control&&
         retained.get()==occurrence,"Retained small-award SAME world/UI/info occurrence changed before phase");
    if(first){h.first_team_small_awards_attempted=true;h.first_team_small_award_slots=rules.report.slots;h.operation=11;}
    else{h.active_team_small_award_slots=rules.report.slots;++h.operation;}
    try{
        // All reached source branches are concretely evaluated and have no
        // stores/callbacks/RNG. Sticky operation precedes completion, never a
        // caller's receipt. No revision/store can be hidden by successful skip.
        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control&&
             retained.get()==occurrence&&state.ui_source_revision==revision,
             "Retained small-award SAME world/UI/info occurrence changed after phase");
        h.frontier.store(first?BnwRetainedTurnFrontier::first_team_circumnavigation:BnwRetainedTurnFrontier::pending_team_circumnavigation);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_first_team_circumnavigation_prefix(){execute_team_circumnavigation_prefix(true);}
void BnwRetainedTurnCursor::execute_team_circumnavigation_prefix(bool first){
    const auto i=impl_;require_current();auto& h=*i->history;
    if(first){
    need(h.first_team_small_awards_attempted&&h.first_team&&h.first_team_is_minor==false&&
         !h.first_team_circumnavigation_attempted&&h.operation==11&&
         h.frontier.load()==BnwRetainedTurnFrontier::first_team_circumnavigation,
         "Retained circumnavigation missing actual operation11 predecessor or already attempted");
    }else{
        need(h.first_team_return_attempted&&h.next_team_war_attempted&&h.active_team&&h.last_completed_team&&
             *h.active_team>*h.last_completed_team&&*h.active_team<63&&h.operation>=14&&h.operation<UINT64_MAX&&
             h.frontier.load()==BnwRetainedTurnFrontier::pending_team_circumnavigation,
             "Retained pending team phase missing SAME active team/predecessor or already attempted");
    }
    const auto source_team=first?h.first_team:h.active_team;
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained circumnavigation actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,
         "Retained circumnavigation actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();const auto& state=*allocation;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(source_team&&*source_team>=0&&*source_team<63&&state.world.teams&&state.world.teams->layout==bnw&&
         state.world.teams->teams.size()==64&&state.world.game_state.layout==bnw,
         "Retained circumnavigation actual team/game state unavailable");
    const auto index=static_cast<std::size_t>(*source_team);const auto& team=state.world.teams->teams[index];
    const saves::NamedSaveInteger* id=nullptr;
    for(const auto& row:team.fields)if(row.name&&std::string_view(row.name)=="id"){need(!id,"Duplicate circumnavigation team ID");id=&row;}
    need(team.index==index&&id&&id->value==*source_team,"Retained circumnavigation SAME actual GetID changed");
    // First retained alive team is already nonbarbarian (SDK GetID==63). Do
    // not invent a different team merely to reach the barbarian return branch.
    const saves::NamedSaveFlag* completed=nullptr;
    for(const auto& row:state.world.game_state.flags)if(row.name&&std::string_view(row.name)=="circumnavigated"){
        need(!completed,"Duplicate actual BNW circumnavigation flag");completed=&row;}
    need(completed,"Retained circumnavigation actual global BNW flag unavailable");
    std::optional<BnwRetainedCircumnavigationReturn> returned;
    std::optional<std::uint64_t> axis;
    if(completed->value)returned=BnwRetainedCircumnavigationReturn::already_circumnavigated;
    else{
        const auto& map=state.world.map_header;
        need(map.layout==bnw&&map.version==1,"Retained circumnavigation actual BNW map header unavailable");
        if(!map.wrap_x&&!map.wrap_y)returned=BnwRetainedCircumnavigationReturn::map_not_wrapped;
        else{
            need(map.grid.width>0&&map.grid.height>0&&map.grid.width<=INT32_MAX&&map.grid.height<=INT32_MAX,
                 "Retained circumnavigation native map dimensions unavailable");
            const auto total=map.grid.size();
            need(total<=static_cast<std::uint64_t>(INT32_MAX/2)&&map.land_plots>=0&&
                 static_cast<std::uint64_t>(map.land_plots)<=total,
                 "Retained circumnavigation source numPlots*2/count arithmetic unavailable");
            if(map.land_plots>static_cast<std::int64_t>((total*2)/3))returned=BnwRetainedCircumnavigationReturn::land_density;
            else{
                // BNW plot WIRE arrays are REALLY_MAX_PLAYERS/TEAMS80,
                // distinct from the logical Game/Team roster64. Authenticate
                // the complete fixed source profile; never accept >=64 or
                // infer a wire capacity from the selected logical team ID.
                const auto bnw_plot_wire=[](const saves::PlotWireDimensions& wire){
                    return wire.players==80&&wire.teams==80&&wire.major_civilizations==22&&
                           wire.yields==6&&wire.invisible_types==1;
                };
                const auto& source_plots=i->session->source().snapshot.map_plots;
                need(source_plots&&source_plots->layout==bnw&&bnw_plot_wire(source_plots->dimensions)&&
                     state.world.map_plots&&state.world.map_plots->layout==bnw&&
                     bnw_plot_wire(state.world.map_plots->dimensions)&&state.world.map_plots->plots.size()==total&&
                     state.world.map_projection&&state.world.map_projection->grid.width==map.grid.width&&
                     state.world.map_projection->grid.height==map.grid.height&&
                     state.world.map_projection->wrap_x==map.wrap_x&&state.world.map_projection->wrap_y==map.wrap_y&&
                     state.world.map_projection->cells.size()==total,
                     "Retained circumnavigation actual raw/owned map roster unavailable");
                const auto revealed=[&](std::uint64_t x,std::uint64_t y){
                    const auto position=map.grid.index(x,y);const auto& raw=state.world.map_plots->plots[position];
                    const auto& owned=state.world.map_projection->cells[position];
                    need(raw.index==position&&raw.x==x&&raw.y==y&&owned.index==position&&owned.x==x&&owned.y==y&&
                         owned.revealed_teams.has_value(),"Retained circumnavigation actual plot/reveal identity unavailable");
                    const saves::PlotArrayObservation* bits=nullptr;
                    for(const auto& row:raw.arrays)if(row.name&&std::string_view(row.name)=="revealed_words"){
                        need(!bits,"Duplicate circumnavigation revealed words");bits=&row;}
                    need(bits&&bits->columns==1&&bits->values.size()==4,"Retained circumnavigation raw BNW reveal words unavailable");
                    for(const auto word:bits->values)need(word>=0&&word<=UINT32_MAX,"Retained circumnavigation raw reveal word outside u32");
                    const auto word=static_cast<std::uint32_t>(bits->values[index/32]);
                    const bool value=(word&(std::uint32_t{1}<<(index%32)))!=0;
                    need(owned.revealed_teams->contains(*source_team)==value,
                         "Retained circumnavigation SAME raw/owned reveal bit disagrees");
                    return value; // Source one-argument isRevealed has no debug/visibility override.
                };
                if(map.wrap_x){
                    // Original for(iX=width;iX--), then height--. Both-wrap
                    // maps test ONLY X; the first missing high column returns.
                    for(auto x=map.grid.width;x-- >0;){bool found=false;
                        for(auto y=map.grid.height;y-- >0;)if(revealed(x,y)){found=true;break;}
                        if(!found){returned=BnwRetainedCircumnavigationReturn::unrevealed_x_column;axis=x;break;}}
                }else{
                    for(auto y=map.grid.height;y-- >0;){bool found=false;
                        for(auto x=map.grid.width;x-- >0;)if(revealed(x,y)){found=true;break;}
                        if(!found){returned=BnwRetainedCircumnavigationReturn::unrevealed_y_row;axis=y;break;}}
                }
            }
        }
    }
    need(returned.has_value(),"Retained reached circumnavigation requires mandatory makeCircumnavigated/moves/message/Lua/replay/checkpoint owners BEFORE operation12; unavailable");
    const auto revision=state.ui_source_revision;
    i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
         "Retained circumnavigation SAME world/UI changed before phase");
    if(first){h.first_team_circumnavigation_attempted=true;h.first_team_circumnavigation_return=returned;
        h.first_team_circumnavigation_axis=axis;h.operation=12;}
    else{h.active_team_circumnavigation_return=returned;h.active_team_circumnavigation_axis=axis;++h.operation;}
    try{
        // All admitted branches executed their actual source early return and
        // have no world stores. Once-history precedes completion and remains
        // sticky even though no revision, RNG or UI setter is needed here.
        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control&&
             state.ui_source_revision==revision,"Retained circumnavigation SAME world/UI changed after phase");
        h.frontier.store(first?BnwRetainedTurnFrontier::first_team_post_circumnavigation:BnwRetainedTurnFrontier::pending_team_return_tail);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_first_team_return_and_advance_prefix(){execute_team_return_and_advance_prefix(true);}
void BnwRetainedTurnCursor::execute_team_return_and_advance_prefix(bool first){
    const auto i=impl_;require_current();auto& h=*i->history;
    if(first){
    need(h.first_team_circumnavigation_attempted&&h.first_team&&h.first_team_is_minor==false&&
         !h.first_team_return_attempted&&h.operation==12&&
         h.frontier.load()==BnwRetainedTurnFrontier::first_team_post_circumnavigation,
         "Retained first-team return missing actual operation12 predecessor or already attempted");
    }else{
        need(h.first_team_return_attempted&&h.next_team_war_attempted&&h.active_team&&h.last_completed_team&&
             *h.active_team>*h.last_completed_team&&*h.active_team<63&&h.operation>=14&&h.operation<UINT64_MAX&&
             h.frontier.load()==BnwRetainedTurnFrontier::pending_team_return_tail,
             "Retained pending team phase missing SAME active team/predecessor or already attempted");
    }
    const auto source_team=first?h.first_team:h.active_team;
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained first-team return actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,
         "Retained first-team return actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();const auto& state=*allocation;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(*source_team>=0&&*source_team<63&&state.world.teams&&state.world.teams->layout==bnw&&
         state.world.teams->dimensions.teams==64&&state.world.teams->dimensions.players==64&&
         state.world.teams->dimensions.civ_teams==63&&state.world.teams->dimensions.major==22&&
         state.world.teams->dimensions.profile_extensions.values.empty()&&state.world.teams->teams.size()==64&&
         state.world.pregame&&state.world.pregame->layout==bnw&&state.world.all_players&&
         state.world.all_players->layout==bnw&&state.world.all_players->context.player_count==64&&
         state.world.all_players->context.profile_extensions.values.empty()&&state.world.all_players->players.size()==64,
         "Retained first-team return actual BNW team/Pregame/player roster unavailable");
    const auto field=[](const auto& fields,std::string_view name){
        const saves::NamedSaveInteger* found=nullptr;
        for(const auto& row:fields)if(row.name&&std::string_view(row.name)==name){need(!found,"Duplicate team-tail source field");found=&row;}
        need(found&&found->value>=INT32_MIN&&found->value<=INT32_MAX,"Retained team-tail source-i32 field unavailable");
        return static_cast<std::int32_t>(found->value);
    };
    const auto& roster=state.world.teams->teams;const auto index=static_cast<std::size_t>(*source_team);
    const auto& team=roster[index];need(team.index==index&&field(team.fields,"id")==*source_team&&
         field(team.fields,"alive_count")>0,"Retained first-team return SAME alive team/GetID changed");
    const auto teams=state.world.pregame->archive.integer_arrays.find("team_types");
    need(teams!=state.world.pregame->archive.integer_arrays.end()&&teams->second.size()==64,
         "Retained team-tail actual CvPreGame team slots unavailable");
    const saves::PlotArrayObservation* at_war=nullptr;
    for(const auto& row:team.arrays)if(row.name&&std::string_view(row.name)=="at_war"){
        need(!at_war,"Duplicate team-tail actual war array");at_war=&row;}
    need(at_war&&at_war->columns==1&&at_war->values.size()==64,"Retained team-tail actual war array unavailable");
    const auto player=[&](std::size_t slot)->const saves::SavedPlayerObservation&{
        const auto& value=state.world.all_players->players.at(slot);const saves::PlayerCoreField* id=nullptr;
        for(const auto& row:value.core.fields)if(row.name&&std::string_view(row.name)=="m_eID"){
            need(!id,"Duplicate team-tail actual player ID");id=&row;}
        need(value.core.layout==bnw&&value.index==slot&&value.saved_player_id==static_cast<std::int64_t>(slot)&&id&&
             id->values.size()==1&&id->values[0]==static_cast<std::int64_t>(slot),
             "Retained team-tail actual player identity unavailable");return value;
    };
    const auto alive=[](const saves::SavedPlayerObservation& value){
        const saves::PlayerCoreField* found=nullptr;
        for(const auto& row:value.core.fields)if(row.name&&std::string_view(row.name)=="m_bAlive"){
            need(!found,"Duplicate team-tail alive scalar");found=&row;}
        need(found&&found->values.size()==1&&(found->values[0]==0||found->values[0]==1),
             "Retained team-tail actual player alive flag unavailable");return found->values[0]!=0;
    };
    const auto player_team=[&](const saves::SavedPlayerObservation& value){
        const auto result=teams->second.at(static_cast<std::size_t>(value.saved_player_id));
        need(result>=-1&&result<64,"Retained team-tail actual GetTeam outside source TeamTypes BEFORE operation13");
        return static_cast<std::int32_t>(result);
    };
    const auto leader=[&](){
        // Exact getLeaderID: living matches first, then any member, then NO_PLAYER.
        // Do not substitute m_eTeam, alive_count, a cached leader or false default.
        for(std::size_t slot=0;slot<64;++slot){const auto& value=player(slot);
            if(alive(value)&&player_team(value)==*source_team)return static_cast<std::int32_t>(slot);}
        for(std::size_t slot=0;slot<64;++slot)if(player_team(player(slot))==*source_team)return static_cast<std::int32_t>(slot);
        return std::int32_t{-1}; // Actual final source NO_PLAYER, not unavailable data.
    };
    const auto tail=[&](){
        std::uint64_t assertions=0;
        // #ifndef FINAL_RELEASE cannot be guessed from SDK Mod configuration.
        // Admit only its concrete no-assertion-effect branch; also equivalent
        // when that diagnostic loop is excluded. No assert service is faked.
        for(std::size_t slot=22;slot<63;++slot){const auto& value=player(slot);const auto target=player_team(value);
            need(target>=0,"Retained team-tail actual isAtWar requires missing native index/assertion branch BEFORE operation13");
            const auto war=at_war->values[static_cast<std::size_t>(target)];need(war==0||war==1,"Retained team-tail actual war boolean unavailable");
            if(!war)continue;
            need(value.planning.layout==bnw&&value.planning.dimensions.major==22&&
                 value.planning.dimensions.civ_teams==63&&value.planning.dimensions.legacy_teams==80,
                 "Retained team-tail actual MinorCivAI source dimensions unavailable");
            const saves::PlayerPlanningRecord* minor=nullptr;
            for(const auto& block:value.planning.blocks)if(block.name&&std::string_view(block.name)=="minor_civ_ai"){
                need(!minor,"Duplicate team-tail MinorCivAI block");minor=&block;}
            need(minor&&field(minor->fields,"version")==1,"Retained team-tail actual MinorCivAI version unavailable");
            const auto ally=field(minor->fields,"m_eAlly");need(ally>=-1&&ally<64,"Retained team-tail source Ally outside PlayerTypes");
            // Source queries Ally BEFORE leader for EACH reached minor slot,
            // including duplicate teams and dead players; no alive filter/dedup.
            need(ally!=leader(),"Retained team-tail reached mandatory native alliance assertion requires diagnostic owner BEFORE operation13; unavailable");
            ++assertions;
        }
        return assertions;
    };
    const auto advance=[&](){
        std::pair<std::int32_t,std::optional<std::int32_t>> result{64,std::nullopt};
        for(std::int32_t slot=*source_team+1;slot<64;++slot){const auto& next=roster[static_cast<std::size_t>(slot)];
            need(next.index==static_cast<std::uint64_t>(slot)&&field(next.fields,"id")==slot,
                 "Retained remaining team loop actual slot/GetID differs");
            if(field(next.fields,"alive_count")>0){result={slot,slot};break;}}
        return result; // 64 means actual outer-loop exhaustion, not a fake team.
    };
    const auto expected_assertions=tail();const auto expected_next=advance(); // Mandatory refusal BEFORE attempt.
    const auto revision=state.ui_source_revision;
    i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
         "Retained team-tail SAME world/UI changed before phase");
    need(h.completed_teams<64&&(first?(h.completed_teams==0&&!h.last_completed_team):
         (h.last_completed_team&&*source_team>*h.last_completed_team)),"Retained team return order/history exhausted BEFORE attempt");
    if(first){h.first_team_return_attempted=true;h.operation=13;}else ++h.operation; // BEFORE actual source tail/loop.
    try{
        // Execute the admitted exact no-effect tail and the real source outer
        // loop. Its actual result, never the preflight result/caller receipt,
        // publishes the next boundary. No next team's doTurn is skipped/run.
        const auto assertions=tail();const auto next=advance();
        need(assertions==expected_assertions&&next==expected_next,"Retained team-tail source branch changed after attempt");
        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control&&
             state.ui_source_revision==revision,"Retained team-tail SAME world/UI/revision changed after phase");
        if(first)h.first_team_tail_assertions=assertions;
        ++h.completed_teams;h.last_completed_team=*source_team;
        h.active_team.reset();h.active_team_is_minor.reset();h.active_team_small_award_slots.reset();
        h.active_team_circumnavigation_return.reset();h.active_team_circumnavigation_axis.reset();
        h.team_loop_next_slot=next.first;h.next_alive_team=next.second;
        h.frontier.store(next.second?BnwRetainedTurnFrontier::remaining_teams_turn:BnwRetainedTurnFrontier::map_turn_unavailable);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_pending_team_war_prefix(){
    const auto i=impl_;require_current();execute_team_war_prefix(!i->history->next_team_war_attempted);
}
void BnwRetainedTurnCursor::execute_pending_team_no_trade_prefix(){execute_team_no_trade_prefix(false);}
std::optional<BnwRetainedMinorTechnologyPlan> BnwRetainedTurnCursor::pending_minor_technology_plan()const{
    const auto i=impl_;require_current();const auto& h=*i->history;
    const auto frontier=h.frontier.load();const bool first=frontier==BnwRetainedTurnFrontier::first_team_minor_technology;
    if(first)need(h.first_team_war_attempted&&h.first_team&&!h.first_team_no_trade_attempted&&h.operation==9,
                 "Retained minor due-plan missing SAME first-team operation9 boundary");
    else need(frontier==BnwRetainedTurnFrontier::next_team_minor_technology&&h.first_team_return_attempted&&
              h.next_team_war_attempted&&h.active_team&&h.last_completed_team&&
              *h.active_team>*h.last_completed_team&&*h.active_team<63&&h.operation>=14&&h.operation<UINT64_MAX,
              "Retained minor due-plan missing SAME pending technology boundary");
    const auto source_team=first?h.first_team:h.active_team;
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained minor due-plan actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);const auto timer=i->timer->read(i->timer->current_use());
    need(timer.reset==BnwGameTurnTimerReset::engine_begin&&timer.operation==1,
         "Retained minor due-plan actual timer predecessor changed");
    const auto allocation=i->session->read_state_lease();
    const auto revision=allocation->ui_source_revision;
    // Same helper supplies the mandatory execution refusal; no stored plan,
    // caller witness, once-history mutation or detached effect publication.
    const auto result=first_minor_tech_acquisition(*allocation,*i->engine_utility->host,*source_team);
    i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control&&
         allocation->ui_source_revision==revision,"Retained minor due-plan SAME world/UI changed during source queries");
    need(result==first_minor_tech_acquisition(*allocation,*i->engine_utility->host,*source_team),
         "Retained minor due-plan source values changed before publication");
    return result;
}

void BnwRetainedTurnCursor::execute_pending_team_small_awards_prefix(){execute_team_small_awards_prefix(false);}
void BnwRetainedTurnCursor::execute_pending_team_circumnavigation_prefix(){execute_team_circumnavigation_prefix(false);}
void BnwRetainedTurnCursor::execute_pending_team_return_and_advance_prefix(){execute_team_return_and_advance_prefix(false);}
BnwRetainedTeamsDiagnostic BnwRetainedTurnCursor::execute_pending_teams_until_boundary(){
    require_current();const auto i=impl_;need(i->history->first_team_return_attempted,"Retained queue diagnostic requires actual first-team return");
    BnwRetainedTeamsDiagnostic result;result.before=view();
    for(std::uint64_t calls=0;calls<64*5;++calls){
        const auto current=view();if(current.frontier==BnwRetainedTurnFrontier::map_turn_unavailable){
            result.after=current;result.stop=BnwRetainedTeamsStop::map_boundary;result.blocker="Actual CvMap::doTurn remains mandatory";return result;}
        try{
            switch(current.frontier){
            case BnwRetainedTurnFrontier::remaining_teams_turn:execute_pending_team_war_prefix();break;
            case BnwRetainedTurnFrontier::next_team_minor_technology:execute_pending_team_no_trade_prefix();break;
            case BnwRetainedTurnFrontier::pending_team_small_awards:execute_pending_team_small_awards_prefix();break;
            case BnwRetainedTurnFrontier::pending_team_circumnavigation:execute_pending_team_circumnavigation_prefix();break;
            case BnwRetainedTurnFrontier::pending_team_return_tail:execute_pending_team_return_and_advance_prefix();break;
            default:throw std::runtime_error("Retained queue diagnostic has no owned pending-team phase");
            }
            ++result.returned_operations;
        }catch(const std::exception& error){
            result.after=view();result.blocker=error.what();result.stop=result.after.frontier==BnwRetainedTurnFrontier::quarantined?
                BnwRetainedTeamsStop::quarantined:BnwRetainedTeamsStop::blocked;return result;
        }
    }
    result.after=view();result.stop=BnwRetainedTeamsStop::blocked;result.blocker="Retained ascending queue exceeded fixed source team bound";return result;
}
void BnwRetainedTurnCursor::execute_next_map_plot_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    const auto frontier=h.frontier.load();
    need((frontier==BnwRetainedTurnFrontier::map_turn_unavailable||frontier==BnwRetainedTurnFrontier::map_plots_turn)&&
         h.first_team_return_attempted&&h.team_loop_next_slot==64&&!h.next_alive_team&&!h.active_team&&
         h.completed_teams>0&&h.operation<UINT64_MAX,
         "Retained map missing actual completed ascending team loop");
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();
    need(static_cast<bool>(actual_ui),"Retained map actual UI lifetime retired");
    const auto control=require_actual_ui(*actual_ui);
    const auto allocation=i->session->read_state_lease();
    auto& state=*i->session->state_;const auto& host=*i->engine_utility->host;
    const auto count=state.world.map_header.grid.size();
    need(count>0&&count<=INT32_MAX&&h.completed_map_plots<count,
         "Retained map actual numPlots or next source index unavailable");
    const auto index=h.completed_map_plots; // No caller plot/order/completion input.
    const auto rules=resolve_bnw_map_turn_rules(host.defines());
    auto domains=load_bnw_plot_domain_rules(host.database(),host.types());
    auto reads=make_bnw_plot_unit_no_effect_services(std::move(domains),&host.database());
    reads.unit_team=[](const LiveSessionState& s,std::uint64_t,saves::SavedPlotUnitReference handle){
        return retained_map_player_team(s,handle.owner);
    };
    reads.plot_team=[](const LiveSessionState& s,std::uint64_t plot,saves::SavedPlotUnitReference){
        need(s.world.map_projection&&plot<s.world.map_projection->cells.size()&&
             s.world.map_projection->cells[static_cast<std::size_t>(plot)].owner.has_value(),
             "Retained map actual plot GetOwner unavailable");
        return retained_map_player_team(s,*s.world.map_projection->cells[static_cast<std::size_t>(plot)].owner);
    };
    const auto plan=prepare_retained_map_plot(state,host,index,rules.ownership_score_duration_threshold);
    const auto verify=[&](){
        const auto result=preflight_bnw_verify_unit_valid_plot(state,index,rules.plot_unit_limit,reads);
        need(result.source_valid&&result.complete_no_effect&&result.effects.empty(),
             result.reason.empty()?"Retained map unit validity source is unresolved":result.reason.c_str());
        // A possible native _DEBUG tail must also be effect-free. GetPlayerUnit
        // NULL skips, but an actual misplaced live unit requires removeUnit.
        const auto roster=preflight_bnw_map_occupied(state.world);
        need(roster.shape_valid,"Retained map actual raw/owned unit roster differs");
        for(const auto& unit:roster.units)if(unit.plot==index)
            need(!unit.saved_unit_found||unit.at_saved_plot,
                 "Retained map reached native debug removeUnit owner unavailable");
    };
    verify(); // Required jump/kill/anchor/debug effects refuse BEFORE this plot attempt.
    const auto revision=state.ui_source_revision;
    need(!plan.changed||revision<UINT64_MAX,"Retained map world revision exhausted");
    i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
         "Retained map SAME world/UI changed before plot");
    verify();
    const auto final=prepare_retained_map_plot(state,host,index,rules.ownership_score_duration_threshold);
    need(final==plan&&state.ui_source_revision==revision&&i->session->state_generation()==h.generation&&
         i->session->state_.get()==allocation.get(),"Retained map source operands changed before stores");
    h.map_attempted=true;h.active_map_plot=index;++h.operation;
    try{
        // Source CvPlot::doTurn order. All field locators/storage widths are
        // validated before this prefix; no cloning/swapping of the live world.
        if(plan.ownership_store){plan.ownership->value=plan.next_ownership;
            if(plan.score_store)plan.land_score->values.front()=plan.next_score;}
        if(plan.improvement_store)plan.improvement->value=plan.next_improvement;
        const bool duration_changed=plan.ownership_store||plan.improvement_store;
        if(duration_changed)++state.ui_source_revision;
        // Queries are executed again AFTER durations, as in the real source.
        // No caller callbacks or synthetic always-true predicates are installed.
        verify();
        // SetWorldAnchor(NO_WORLD_ANCHOR,-1): the old-anchor=-1 branch has
        // actual scalar stores and no engine removal/addition event.
        plan.anchor->value=-1;plan.anchor_data->value=-1;
        if(plan.changed&&!duration_changed)++state.ui_source_revision;
        i->check_engine();need(i->session->read_state_lease()==allocation&&require_actual_ui(*actual_ui)==control,
             "Retained map SAME world/UI changed after plot stores");
        ++h.completed_map_plots;h.active_map_plot.reset();
        h.frontier.store(h.completed_map_plots==count?BnwRetainedTurnFrontier::engine_ui_do_turn_unavailable:
             BnwRetainedTurnFrontier::map_plots_turn);
    }catch(...){h.quarantine();throw;} // Preserve already stored prefix, never replay.
}
BnwRetainedMapDiagnostic BnwRetainedTurnCursor::execute_map_plots_until_boundary(){
    require_current();BnwRetainedMapDiagnostic result;result.before=view();
    const auto count=impl_->session->state_->world.map_header.grid.size();
    for(std::uint64_t calls=0;calls<count;++calls){
        if(view().frontier==BnwRetainedTurnFrontier::engine_ui_do_turn_unavailable)break;
        try{execute_next_map_plot_prefix();++result.returned_plots;}
        catch(const std::exception& error){result.after=view();result.blocker=error.what();return result;}
    }
    result.after=view();result.blocker=result.after.frontier==BnwRetainedTurnFrontier::engine_ui_do_turn_unavailable?
        "Actual gDLL UI doTurn remains mandatory BEFORE barbarians/player activation":
        "Retained map did not reach actual source outer-loop return";
    return result;
}
std::uint64_t BnwRetainedTurnCursor::game_data_dirty_setter_count(const ui::LiveUiHost& ui)const{
    const auto control=require_actual_ui(ui);const auto value=control->game_data_dirty_setter_count();require_current();return value;
}
std::optional<bool> BnwRetainedTurnCursor::game_data_dirty(const ui::LiveUiHost& ui)const{
    const auto control=require_actual_ui(ui);const auto value=control->game_data_dirty();require_current();return value;
}
std::uint64_t BnwRetainedTurnCursor::score_dirty_setter_count(const ui::LiveUiHost& ui)const{
    const auto control=require_actual_ui(ui);const auto value=control->score_dirty_setter_count();require_current();return value;
}
std::optional<bool> BnwRetainedTurnCursor::score_dirty(const ui::LiveUiHost& ui)const{
    const auto control=require_actual_ui(ui);const auto value=control->score_dirty();
    require_current();return value;
}
} // namespace civ5::runtime

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_barbarian_camps_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.frontier.load()==BnwRetainedTurnFrontier::barbarian_camps_turn&&!h.camps_turn_attempted&&h.operation<UINT64_MAX,
         "Retained DoCamps requires actual engine UI doTurn return; no caller completion authority");
    i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Retained DoCamps actual UI retired");
    const auto control=require_actual_ui(*ui);const auto allocation=i->session->read_state_lease();
    auto& state=*i->session->state_;const auto& host=*i->engine_utility->host;
    const auto plan=prepare_retained_camps(state,host);
    auto raw_options=plan.options,owned_options=plan.options; // all allocating cache storage before effects
    const auto revision=state.ui_source_revision;
    const bool changed=!retained_options_equal(plan.options,state.setup.game_options);
    need((!changed&&!plan.spawn_branch)||revision<UINT64_MAX,"Retained DoCamps UI revision overflow");
    i->check_engine();const auto final=prepare_retained_camps(state,host);
    need(final.same(plan)&&state.ui_source_revision==revision&&i->session->read_state_lease()==allocation&&
         require_actual_ui(*ui)==control,"Retained DoCamps SAME operands changed before native effects");
    h.camps_turn_attempted=true;++h.operation;
    try{
        if(changed){state.setup.game_options.swap(owned_options);state.world.pregame->game_options.swap(raw_options);}
        if(!plan.spawn_branch){if(changed)++state.ui_source_revision;
            h.frontier.store(BnwRetainedTurnFrontier::barbarian_units_turn);return;}
        auto to_add=plan.to_add;
        if(!plan.first_turn)to_add=state.random.gameplay.draw(static_cast<std::uint64_t>(plan.odds))>0?1:0;
        // Native consumes this roll even if the preceding roll produced zero
        // camps. No detached RNG or guessed result is used.
        const bool coastal=state.random.gameplay.draw(static_cast<std::uint64_t>(plan.coastal))==0;
        ++state.ui_source_revision;h.camps_native_to_add=to_add;h.camps_native_wants_coastal=coastal;
        // Native do{} starts even with to_add==0. Full actual plot-finder,
        // SetImprovementType and unit Init are mandatory, never a success skip.
        h.frontier.store(BnwRetainedTurnFrontier::barbarian_camp_plot_finder_unavailable);
    }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_barbarian_units_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.frontier.load()==BnwRetainedTurnFrontier::barbarian_units_turn&&h.camps_turn_attempted&&
         !h.units_turn_attempted&&h.operation<UINT64_MAX,"Retained DoUnits requires actual completed DoCamps");
    i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Retained DoUnits actual UI retired");
    const auto control=require_actual_ui(*ui);const auto allocation=i->session->read_state_lease();
    auto& state=*i->session->state_;const auto& host=*i->engine_utility->host;
    const auto plan=prepare_retained_barbarian_units(state,host);
    need(!plan.due_plot,"Retained DoUnits reached real DoSpawnBarbarianUnit/DoCampActivationNotice; unit Init mandatory");
    auto raw_options=plan.options,owned_options=plan.options;
    const bool changed=!retained_options_equal(plan.options,state.setup.game_options);
    const auto revision=state.ui_source_revision;need(!changed||revision<UINT64_MAX,"Retained DoUnits revision overflow");
    i->check_engine();const auto final=prepare_retained_barbarian_units(state,host);
    need(final.same(plan)&&state.ui_source_revision==revision&&i->session->read_state_lease()==allocation&&
         require_actual_ui(*ui)==control,"Retained DoUnits SAME operands changed before native effects");
    h.units_turn_attempted=true;++h.operation;
    try{
        if(changed){state.setup.game_options.swap(owned_options);state.world.pregame->game_options.swap(raw_options);++state.ui_source_revision;}
        h.frontier.store(BnwRetainedTurnFrontier::game_religions_turn);
    }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_religion_first_city_prefix(){
    const auto i=impl_;require_current();auto& h=*i->history;
    need(h.frontier.load()==BnwRetainedTurnFrontier::game_religions_turn&&h.units_turn_attempted&&
         !h.religion_city_turn_attempted&&h.operation<UINT64_MAX,
         "Retained religion requires actual completed barbarian units phase");
    i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Retained religion actual UI retired");
    const auto control=require_actual_ui(*ui);const auto allocation=i->session->read_state_lease();
    auto& state=*i->session->state_;const auto plan=prepare_retained_first_religion_city(state);
    const auto revision=state.ui_source_revision;need(!plan.changed||revision<UINT64_MAX,"Retained religion revision exhausted");
    i->check_engine();const auto final=prepare_retained_first_religion_city(state);
    need(final==plan&&state.ui_source_revision==revision&&i->session->read_state_lease()==allocation&&
         require_actual_ui(*ui)==control,"Retained religion SAME city operands changed before reset");
    h.religion_city_turn_attempted=true;++h.operation;h.active_religion_city=plan.city;
    try{
        if(!plan.city){h.frontier.store(BnwRetainedTurnFrontier::game_trade_turn_unavailable);return;}
        // Actual CvCityReligions::ResetNumTradeRoutePressure source stores.
        // These counters have no projected CityYieldReligion member; no
        // pressure/follower/yield is invented or changed by the reset.
        for(const auto& [field,old]:plan.counters){(void)old;field->values.front()=0;}
        if(plan.changed)++state.ui_source_revision;
        h.frontier.store(plan.holy?BnwRetainedTurnFrontier::religion_holy_city_pressure_unavailable:
             BnwRetainedTurnFrontier::religion_spy_pressure_unavailable);
        // Holy-city pressure or the subsequent per-player spy/adjacent pressure
        // loop MUST execute before this city/outer religion phase may return.
    }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
namespace {auto& engine_named(auto& rows,std::string_view name){using Row=std::remove_reference_t<decltype(rows.front())>;Row*found=nullptr;for(auto& row:rows)if(row.name&&name==row.name){need(!found,"Engine UI duplicate native treasury field");found=&row;}need(found,"Engine UI native treasury field missing");return *found;}}
void BnwRetainedTurnCursor::execute_engine_ui_impl(BnwSelectedStockLuaBootstrap*vm,const BnwStockLuaAttemptLease*lease,const BnwSelectedStockLuaSourceOwner*stock){
    const auto i=impl_;require_current();auto&h=*i->history;
    need(h.frontier.load()==BnwRetainedTurnFrontier::engine_ui_do_turn_unavailable&&
        h.map_attempted&&i->session->state_->world.map_plots&&
        h.completed_map_plots==i->session->state_->world.map_plots->plots.size()&&
        !h.engine_ui_attempted&&h.operation<UINT64_MAX,
        "Engine UI turn requires actual completed map and once phase");
    i->check_engine();const auto actual_ui=i->cycle_ui.lock();need(bool(actual_ui),"Engine UI turn actual UiHost retired");
    const auto control=require_actual_ui(*actual_ui);const auto host=i->engine_utility->host;
    if(!actual_ui->engine_ui_turn_owner_)actual_ui->engine_ui_turn_owner_=ui::BnwEngineUiTurnOwner::bind(*actual_ui,host);
    const auto receiver=actual_ui->engine_ui_turn_owner_;receiver->require_current();
    need(require_actual_ui(*actual_ui)==control,"Engine UI turn actual receiver changed");
    h.engine_ui_earned_revision=i->session->state_->ui_source_revision;
    h.engine_ui_attempted=true;++h.operation; // before actual original setter stores
    try{
        receiver->reset_repetition_counters();i->check_engine();receiver->require_current();
        auto&state=*i->session->state_;const auto active=detail::bnw_game_active_player_read(state);
        need(state.world.all_players&&state.world.all_players->players.size()==64,
            "Engine UI turn native complete player roster unavailable");
        for(std::int32_t actor=0;actor<22;++actor){
            const auto&raw=state.world.all_players->players.at(static_cast<std::size_t>(actor));
            const auto&alive_field=retained_religion_named(raw.core.fields,"m_bAlive");
            const auto alive=retained_religion_i32(raw.core.fields,"m_bAlive");
            need(raw.index==static_cast<std::uint64_t>(actor)&&raw.saved_player_id==actor&&
                alive_field.encoding==saves::PlayerCoreEncoding::boolean&&(alive==0||alive==1)&&
                state.setup.players.contains(actor)&&state.setup.players.at(actor).alive==(alive!=0),
                "Engine UI turn actual native isAlive differs");
            h.engine_ui_player=actor;
            if(!alive)continue;
            (void)resolve_bnw_city_registry(state.world,state.cities,actor,0);
            if(raw.cities.cities.empty())continue;
            // Original active-player summary call is BEFORE gold gates.
            // Its human short circuit is genuine. Reached missing aggregate/
            // comparator/localization effects cannot be bypassed by gold>0.
            if(actor==active&&detail::bnw_player_human_read(state,actor,host->database())){
                if(receiver->has_selected_city_name_keys(actor)){need(vm&&lease,"Engine summary actual selected Locale Bootstrap supplier absent");vm->bind_engine_ui_localization_owned(*receiver,*lease);}
                receiver->publish_active_summary(actor);
            }
            const saves::PlayerAdministrationRecord*treasury=nullptr;
            for(const auto&block:raw.administration.blocks)if(block.name&&std::string_view(block.name)=="treasury"){
                need(!treasury,"Engine UI turn duplicate native treasury");treasury=&block;
            }
            need(treasury,"Engine UI turn native treasury missing");
            const saves::NamedSaveInteger*gold=nullptr;
            for(const auto&field:treasury->fields)if(field.name&&std::string_view(field.name)=="m_iGold"){
                need(!gold,"Engine UI turn duplicate native gold");gold=&field;
            }
            need(gold&&gold->value>=INT32_MIN&&gold->value<=INT32_MAX&&
                state.treasury.players.contains(actor)&&state.treasury.players.at(actor).balance_times100==gold->value,
                "Engine UI turn raw/owned gold differs");
            if(static_cast<std::int32_t>(gold->value)/100<=0){
                if(!vm||!lease||!stock){h.frontier.store(BnwRetainedTurnFrontier::engine_ui_net_gold_unavailable);return;}
                // Only genuine SourceOwner creates this current SAME VM operation.
                // Actor is read from the original cursor loop, not caller input.
                const auto serial=lease->operation_serial();lease->require_operation(serial);
                const auto returned=vm->accumulate_engine_founder_owned(actor,*lease);
                need(returned.errors.empty(),"Engine UI actual founder callback failed");
                lease->require_operation(serial);i->check_engine();receiver->require_current();
                const auto rules=load_treasury_rules(host->database(),content::TechnologyProfile::bnw,host->types());
                const auto city_rules=load_city_yield_rules(host->database(),content::TechnologyProfile::bnw);
                const auto maintenance_rules=load_maintenance_rules(host->database(),content::TechnologyProfile::bnw,host->types());
                TreasuryServices services;
                services.founder_benefits_religion=[actor,value=returned.value](std::int64_t asked){
                    need(asked==actor,"Engine UI treasury requested another founder");return value;
                };
                const MaintenanceWorld world{state.setup,state.player_research,state.cities,state.technologies};
                auto scratch=state.maintenance;
                const auto quote=net_gold_times100(state.treasury,world,scratch,rules,city_rules,maintenance_rules,host->defines(),nullptr,actor,services);
                need(quote.expenses.has_value(),"Engine UI actual BNW expense query absent");
                const auto native_int=[](std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Engine UI native treasury intermediate overflow");return static_cast<std::int32_t>(n);};
                std::int64_t gross=0;
                for(auto part:{quote.income.cities,quote.income.diplomacy,quote.income.connections,quote.income.religion,quote.income.traits}){(void)native_int(part);gross+=part;(void)native_int(gross);}
                (void)native_int(quote.expenses->units.total);(void)native_int(quote.expenses->supply);
                (void)native_int(quote.expenses->total);(void)native_int(quote.expenses->total*100);
                const auto net=native_int(quote.net_times100);
                need(gross==quote.income.total_times100,"Engine UI source gross chain differs");
                auto& player=state.world.all_players->players.at(static_cast<std::size_t>(actor));
                auto& target=engine_named(player.administration.blocks,"treasury");
                auto& raw_units=engine_named(target.fields,"m_iExpensePerTurnUnitMaintenance");
                auto& raw_supply=engine_named(target.fields,"m_iExpensePerTurnUnitSupply");
                auto& owned=state.maintenance.players.at(actor);
                need(raw_units.value==owned.cached_unit_expense&&raw_supply.value==owned.cached_supply_expense,
                     "Engine UI native/owned expense caches differ");
                const auto before=state.ui_source_revision;
                const bool changed=raw_units.value!=quote.expenses->units.total||raw_supply.value!=quote.expenses->supply;
                need(!changed||before<UINT64_MAX,"Engine UI actual cache revision exhausted");
                auto staged=receiver->prepare_net_gold_observation(actor,net,before+static_cast<std::uint64_t>(changed));
                lease->require_operation(serial);i->check_engine();receiver->require_current();
                // No Lua/provider/callback after this recomputation/bookend. Native
                // writes are in CalculatePreInflatedCosts unit-then-supply order.
                auto verified=state.maintenance;
                const auto final=net_gold_times100(state.treasury,world,verified,rules,city_rules,maintenance_rules,host->defines(),nullptr,actor,services);
                need(final.net_times100==quote.net_times100&&final.expenses&&final.expenses->units.total==quote.expenses->units.total&&
                    final.expenses->supply==quote.expenses->supply&&state.ui_source_revision==before&&
                    raw_units.value==owned.cached_unit_expense&&raw_supply.value==owned.cached_supply_expense,
                    "Engine UI treasury operands changed before stores");
                raw_units.value=quote.expenses->units.total;owned.cached_unit_expense=quote.expenses->units.total;
                raw_supply.value=quote.expenses->supply;owned.cached_supply_expense=quote.expenses->supply;
                if(changed)++state.ui_source_revision;
                receiver->commit_net_gold_observation(staged);h.engine_ui_earned_revision=state.ui_source_revision;
                stock->adopt_engine_cache_revision(*this,*lease);
                if(net/100<=0){
                    receiver->record_pending_deficit(actor,net,state.setup.game_turn,state.ui_source_revision);
                    h.frontier.store(BnwRetainedTurnFrontier::engine_ui_deficit_localization_unavailable);
                    vm->bind_engine_ui_localization_owned(*receiver,*lease);
                    receiver->format_pending_deficit();
                    lease->require_operation(serial);i->check_engine();receiver->require_current();
                    const bool notification_changed=receiver->add_formatted_deficit(state);
                    if(notification_changed){
                        h.engine_ui_earned_revision=state.ui_source_revision;
                        stock->adopt_engine_cache_revision(*this,*lease);
                    }
                    // Actual Add return continues the original ascending loop.

                }
            }
        }
        receiver->require_current();i->check_engine();
        need(require_actual_ui(*actual_ui)==control&&actual_ui->engine_ui_turn_owner_.get()==receiver.get(),
            "Engine UI turn receiver replaced after native gates");
        h.engine_ui_returned=true;h.engine_ui_player.reset();
        h.frontier.store(BnwRetainedTurnFrontier::barbarian_camps_turn);
    }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
std::shared_ptr<BnwRetainedTurnCursor> BnwRetainedTurnCursor::retain_engine_factory_owner()const{
 const auto h=impl_->history;h->thread_check();const auto actual=h->cursor.lock();
 need(actual&&actual.get()==this,"Engine UI cursor factory retired");actual->require_current();return actual;
}
void BnwRetainedTurnCursor::require_engine_host(const ui::SelectedBnwPreviewHost& host)const{
 const auto p=impl_;p->check_engine();need(p->engine_utility->host.get()==&host&&&host.session()==p->session.get(),"Engine UI actual Host/session differs");
}
std::uint64_t BnwRetainedTurnCursor::engine_actual_revision()const{require_current();need(impl_->history->engine_ui_earned_revision&&*impl_->history->engine_ui_earned_revision==impl_->session->state_->ui_source_revision,"Engine UI actual earned revision absent/changed");return *impl_->history->engine_ui_earned_revision;}
void BnwRetainedTurnCursor::execute_engine_ui_with_actual_stock(BnwSelectedStockLuaBootstrap& vm,const BnwStockLuaAttemptLease& lease,const BnwSelectedStockLuaSourceOwner& source){
 require_current();const auto state=impl_->session->read_state_lease();const auto actual=lease.world_lease();
 need(actual.get()==state.get()&&!actual.owner_before(state)&&!state.owner_before(actual),"Engine UI genuine stock world differs");
 execute_engine_ui_impl(&vm,&lease,&source);
}
void BnwRetainedTurnCursor::execute_engine_ui_prefix(){execute_engine_ui_impl(nullptr,nullptr,nullptr);}
}

namespace civ5::runtime {
namespace {
struct RetainedReligionNoPressurePlan {
 std::vector<std::pair<std::int32_t,std::int32_t>> cities;
 std::vector<std::pair<saves::CityTailNode*,std::int64_t>> counters;
 std::vector<std::int64_t> operands;
 bool changed=false;
 bool operator==(const RetainedReligionNoPressurePlan&)const=default;
};
RetainedReligionNoPressurePlan prepare_retained_religion_no_pressure(LiveSessionState& state){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 const auto& religions=state.world.religion_culture;
 need(state.setup.profile==content::TechnologyProfile::bnw&&state.cities.profile==content::TechnologyProfile::bnw&&
      state.cities.extensions.values.empty()&&religions.layout==bnw&&religions.religions_version&&
      *religions.religions_version>=2&&*religions.religions_version<=4&&
      religions.start_offset<religions.culture_offset&&religions.culture_offset<=religions.end_offset&&
      religions.end_offset<=state.world.payload_end_offset,
      "Retained SpreadReligion genuine raw/owned religion source absent");
 // GetReligionCreatedByPlayer scans the actual current list; with BOTH
 // authoritative serialized and owned lists empty it returns NO_RELIGION
 // for every actor. GetSpyPressure then returns its initialized zero, before
 // any espionage or belief call. This is an actual branch, not a spy default.
 need(religions.religions.empty()&&state.cities.religions.empty(),
      "Retained SpreadReligion reached founded/pantheon source pressure owner required");
 need(state.world.all_players&&state.world.all_players->layout==bnw&&state.world.all_players->players.size()==64&&
      state.setup.players.size()==64,"Retained SpreadReligion complete source player roster unavailable");
 RetainedReligionNoPressurePlan plan;
 const auto emit=[&](std::int64_t value){plan.operands.push_back(value);};
 for(std::int32_t actor=0;actor<64;++actor){
  auto& player=state.world.all_players->players.at(static_cast<std::size_t>(actor));
  const auto& field=retained_religion_named(player.core.fields,"m_bAlive");
  const auto alive=retained_religion_i32(player.core.fields,"m_bAlive");
  need(player.index==static_cast<std::uint64_t>(actor)&&player.saved_player_id==actor&&player.core.layout==bnw&&
       field.encoding==saves::PlayerCoreEncoding::boolean&&(alive==0||alive==1)&&
       state.setup.players.contains(actor)&&state.setup.players.at(actor).alive==(alive!=0),
       "Retained SpreadReligion native alive/actor binding differs");
  emit(actor);emit(alive);if(!alive)continue;
  (void)resolve_bnw_city_registry(state.world,state.cities,actor,0);
  std::vector<std::pair<std::int32_t,saves::SavedCityObservation*>> order;
  for(auto& city:player.cities.cities){
   const auto id=retained_religion_i32(city.core.fields,"m_iID");
   order.emplace_back(id,&city);
  }
  std::sort(order.begin(),order.end(),[](const auto&a,const auto&b){return (a.first&8191)<(b.first&8191);});
  emit(static_cast<std::int64_t>(order.size()));
  for(const auto& [id,raw]:order){
   need(resolve_bnw_city_registry(state.world,state.cities,actor,id)==raw,
        "Retained SpreadReligion actual forward FLTA city differs");
   const auto& owned=state.cities.players.at(actor).cities.at(id);
   need(raw->tail.layout==bnw&&raw->tail.context.profile_extensions.values.empty()&&owned.extensions.values.empty(),
        "Retained SpreadReligion city source extension requires effects");
   auto& block=retained_religion_named(raw->tail.blocks,"religions");
   auto& status=retained_religion_named(block.children,"m_ReligionStatus");
   need(status.encoding&&std::string_view(status.encoding)=="signed_records"&&status.wire_count&&
        *status.wire_count>=INT32_MIN&&*status.wire_count<=INT32_MAX&&
        static_cast<std::uint64_t>(std::max<std::int64_t>(0,*status.wire_count))==status.children.size()&&
        status.children.size()==owned.religions.size(),"Retained SpreadReligion native status/owned count differs");
   plan.cities.emplace_back(actor,id);emit(id);emit(*status.wire_count);
   std::int32_t total=0,most=-1,most_pressure=0,majority=-1;
   for(std::size_t n=0;n<status.children.size();++n){
    auto& row=status.children[n];need(row.name&&std::string_view(row.name)=="religion","Retained SpreadReligion native status entry differs");
    const auto version=retained_religion_i32(row.children,"version");
    need(version==2,"Retained SpreadReligion v1 unsaved trade-pressure count owner absent");
    const auto religion=retained_religion_i32(row.children,"m_eReligion");
    const auto followers=retained_religion_i32(row.children,"m_iFollowers");
    const auto pressure=retained_religion_i32(row.children,"m_iPressure");
    const auto founded=retained_religion_i32(row.children,"m_bFoundedHere");
    const auto& flag=retained_religion_named(row.children,"m_bFoundedHere");
    need(flag.encoding&&std::string_view(flag.encoding)=="bool"&&(founded==0||founded==1),"Retained SpreadReligion native holy flag differs");
    const auto& live=owned.religions[n];
    need(live.religion==religion&&live.followers==followers&&live.pressure==pressure,
         "Retained SpreadReligion paired current status differs");
    need(!founded,"Retained SpreadReligion reached actual AddHolyCityPressure");
    // Exact CvCityReligions::GetReligiousMajority: running native i32 sum,
    // followers then pressure tie-break, followed by most*2 >= total.
    const auto next=std::int64_t(total)+followers;
    need(next>=INT32_MIN&&next<=INT32_MAX,"Retained SpreadReligion majority sum native overflow");total=static_cast<std::int32_t>(next);
    if(followers>most||(followers==most&&pressure>most_pressure)){most=followers;most_pressure=pressure;majority=religion;}
    auto& counter=retained_religion_named(row.children,"m_iNumTradeRoutesApplyingPressure");
    need(counter.encoding&&std::string_view(counter.encoding)=="i32"&&counter.children.empty(),
         "Retained SpreadReligion native trade counter storage differs");
    const auto old=retained_religion_i32(row.children,"m_iNumTradeRoutesApplyingPressure");
    plan.counters.emplace_back(&counter,old);plan.changed|=old!=0;
    for(const auto value:{version,religion,followers,pressure,founded,old})emit(value);
   }
   const auto twice=std::int64_t(most)*2;
   need(twice>=INT32_MIN&&twice<=INT32_MAX,"Retained SpreadReligion majority multiply native overflow");
   if(twice<total)majority=-1;
   need(majority==-1,"Retained SpreadReligion reached real adjacent/trade religious pressure");
  }
 }
 return plan;
}
}
void BnwRetainedTurnCursor::execute_game_religion_no_pressure_prefix(){
 const auto i=impl_;require_current();auto&h=*i->history;
 need(h.frontier.load()==BnwRetainedTurnFrontier::religion_spy_pressure_unavailable&&
      h.religion_city_turn_attempted&&h.active_religion_city&&h.units_turn_attempted&&h.operation<UINT64_MAX,
      "Retained SpreadReligion requires actual first city reset/holy no-effect predecessor");
 i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Retained SpreadReligion actual UiHost retired");
 const auto control=require_actual_ui(*ui);const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;
 const auto plan=prepare_retained_religion_no_pressure(state);
 need(!plan.cities.empty()&&plan.cities.front()==*h.active_religion_city,
      "Retained SpreadReligion first actual source city differs");
 const auto first=prepare_retained_first_religion_city(state);
 need(first.city==h.active_religion_city&&!first.holy&&!first.changed,
      "Retained SpreadReligion first native trade-pressure reset was changed");
 const auto revision=state.ui_source_revision;need(!plan.changed||revision<UINT64_MAX,"Retained SpreadReligion revision overflow");
 i->check_engine();const auto final=prepare_retained_religion_no_pressure(state);
 need(final==plan&&state.ui_source_revision==revision&&i->session->read_state_lease()==allocation&&
      require_actual_ui(*ui)==control,"Retained SpreadReligion relevant source operands changed before stores");
 ++h.operation;
 try{
  // Original ascending alive-player/forward city order. First counters are
  // already zero from the true predecessor. Every remaining reset is native
  // storage, followed by proven false holy, zero spy and NO_RELIGION sources.
  // No belief, spy, follower, pressure, yield, RNG or clock is fabricated.
  for(const auto& [field,old]:final.counters){(void)old;field->values.front()=0;}
  if(plan.changed)++state.ui_source_revision;
  h.active_religion_city.reset();h.frontier.store(BnwRetainedTurnFrontier::game_trade_turn_unavailable);
 }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
namespace {
struct RetainedTradeTechPlan {
 std::vector<saves::PregameOption> old_options,next_options;
 std::vector<std::int64_t> operands;
 std::array<std::int64_t,22*22> old_matrix{},next_matrix{};
 std::array<bool,64> reached_teams{};
 bool no_science=false;
 const std::int64_t* storage=nullptr;
};
const saves::PlotArrayObservation& retained_trade_tech_array(const saves::TeamObservation& raw,const char* name){
 const saves::PlotArrayObservation* found=nullptr;
 for(const auto& a:raw.arrays)if(a.name&&std::string_view(a.name)==name){need(!found,"Trade tech duplicate raw array");found=&a;}
 need(found&&found->columns==1,"Trade tech raw array owner absent");return *found;
}
template<class Emit> void visit_retained_trade_tech_operands(const LiveSessionState& state,
 const ui::SelectedBnwPreviewHost& host,const RetainedTradeTechPlan& plan,Emit emit){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 const auto& trade=state.world.trade;
 need(trade.layout==bnw&&trade.version&&*trade.version==3&&trade.dimensions.major_slots==22&&
  trade.dimensions.yield_slots==6&&trade.profile_extensions.values.empty()&&trade.start_offset<trade.routes_end_offset&&
  trade.routes_end_offset==trade.matrices_offset&&trade.matrices_offset<trade.end_offset&&
  trade.technology_difference&&trade.technology_difference->size()==22*22,
  "Trade native technology matrix version3 owner unavailable");
 need(trade.technology_difference->data()==plan.storage&&state.world.pregame&&
  retained_options_equal(state.setup.game_options,plan.old_options)&&
  retained_options_equal(state.world.pregame->game_options,plan.old_options),"Trade matrix/options storage changed");
 for(const auto n:{trade.start_offset,trade.routes_end_offset,trade.matrices_offset,trade.end_offset}){
  need(n<=INT64_MAX,"Trade source boundary width differs");emit(static_cast<std::int64_t>(n));
 }
 emit(state.setup.game_turn);emit(integer(state.world.pregame->archive,"game_turn"));
 need(state.setup.game_turn==integer(state.world.pregame->archive,"game_turn"),"Trade raw/owned current turn differs");
 for(const auto n:*trade.technology_difference){need(n>=INT32_MIN&&n<=INT32_MAX,"Trade current matrix non-native integer");emit(n);}
 // isOption(NO_SCIENCE) returns i==1; no player/team reads when true.
 if(plan.no_science)return;
 need(state.world.all_players&&state.world.all_players->players.size()==64&&
  state.world.teams&&state.world.teams->layout==bnw&&state.world.teams->teams.size()==64&&
  state.technologies.size()==64,"Trade actual complete player/team owners unavailable");
 for(std::int32_t player=0;player<22;++player){
  const auto team=detail::bnw_player_team_read(state,player);emit(team);
  const auto& raw=state.world.all_players->players[static_cast<std::size_t>(player)];
  const saves::PlayerCoreField* alive=nullptr;
  for(const auto& field:raw.core.fields)if(field.name&&std::string_view(field.name)=="m_bAlive"){
   need(!alive,"Trade duplicate actual IsAlive");alive=&field;
  }
  need(alive&&alive->encoding==saves::PlayerCoreEncoding::boolean&&alive->values.size()==1&&
   (alive->values[0]==0||alive->values[0]==1)&&state.setup.players.at(player).alive==(alive->values[0]!=0),
   "Trade actual raw/owned IsAlive differs");emit(alive->values[0]);
 }
 const auto& rules=host.technologies();
 need(rules.profile==content::TechnologyProfile::bnw&&rules.all_succeeded()&&rules.collection.slots<=INT32_MAX,
  "Trade actual native NumTechs cache unavailable");emit(static_cast<std::int64_t>(rules.collection.slots));
 for(std::size_t team=0;team<plan.reached_teams.size();++team)if(plan.reached_teams[team]){
  const auto& raw=state.world.teams->teams[team];const auto& owned=state.technologies[team];
  need(raw.index==team&&owned.team()==static_cast<std::int64_t>(team)&&owned.profile()==rules.profile&&
   owned.slots()==rules.collection.slots&&owned.profile_extensions().values.empty()&&
   owned.remap().all_mapped()&&owned.remap().duplicates==0&&owned.remap().holes==0,"Trade SAME team tech owner changed");
  for(const auto* name:{"id","tech_version","last_tech","saved_techs"})emit(team_war_field(raw,name));
  for(const auto* name:{"tech_hashes","has_tech","no_trade_tech","research_progress","tech_count"}){
   const auto& array=retained_trade_tech_array(raw,name);emit(static_cast<std::int64_t>(array.values.size()));
   for(const auto value:array.values)emit(value);
  }
  emit(owned.last_tech());const auto& remap=owned.remap();
  for(const auto value:{remap.saved,remap.mapped,remap.sentinels,remap.unknown,remap.outside,remap.duplicates,remap.holes}){
   need(value<=INT64_MAX,"Trade tech remap width differs");emit(static_cast<std::int64_t>(value));
  }
  emit(remap.last_tech_outside?1:0);
  for(std::int64_t slot=0;slot<static_cast<std::int64_t>(owned.slots());++slot){const auto v=owned.at(slot);
   emit(v.has_tech?1:0);emit(v.no_trade?1:0);emit(v.progress_times100);emit(v.count);
  }
 }
}
RetainedTradeTechPlan prepare_retained_trade_tech(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host){
 RetainedTradeTechPlan p;
 need(state.world.pregame&&retained_options_equal(state.setup.game_options,state.world.pregame->game_options),
  "Trade paired current GameOptions cache differs");
 p.old_options=state.setup.game_options;
 auto setup=state.setup;p.no_science=setup.game_option_value(host.database(),"GAMEOPTION_NO_SCIENCE")==1;
 p.next_options=std::move(setup.game_options);
 const auto& trade=state.world.trade;
 need(trade.technology_difference&&trade.technology_difference->size()==p.old_matrix.size(),"Trade serialized/live native matrix owner absent");
 p.storage=trade.technology_difference->data();std::copy(trade.technology_difference->begin(),trade.technology_difference->end(),p.old_matrix.begin());
 p.next_matrix.fill(-1); // Actual ResetTechDifference precedes Build.
 if(!p.no_science){
  need(state.world.all_players&&state.world.all_players->players.size()==64&&state.world.teams&&
   state.world.teams->teams.size()==64&&state.technologies.size()==64,"Trade native roster unavailable");
  const auto& rules=host.technologies();need(rules.profile==content::TechnologyProfile::bnw&&rules.all_succeeded()&&
   rules.collection.slots<=INT32_MAX&&host.types().report().all_succeeded()&&host.types().report().hash_collisions==0,
   "Trade actual technology/type caches unavailable");
  const auto authenticate=[&](std::int32_t team)->const TeamTechnologies&{
   need(team>=0&&team<64,"Trade alive native team is NO_TEAM/outside roster");const auto n=static_cast<std::size_t>(team);
   const auto& raw=state.world.teams->teams[n];const auto& owned=state.technologies[n];
   if(!p.reached_teams[n]){
    need(raw.index==n&&team_war_field(raw,"id")==team&&team_war_field(raw,"tech_version")==1,
     "Trade actual reached team identity/version differs");
    const auto decoded=restore_team_technologies(raw,rules,host.types());
    need(decoded.profile()==owned.profile()&&decoded.team()==owned.team()&&decoded.slots()==owned.slots()&&
     decoded.remap().all_mapped()&&decoded.remap().duplicates==0&&decoded.remap().holes==0&&
     decoded.last_tech()==owned.last_tech(),"Trade raw/owned reached technology identity differs");
    for(std::int64_t slot=0;slot<static_cast<std::int64_t>(decoded.slots());++slot)
     need(decoded.at(slot)==owned.at(slot),"Trade raw/owned HasTech/count/progress/noTrade differs");
    p.reached_teams[n]=true;
   }return owned;
  };
  // Exact source getTeam order and alive short circuit. No route movement,
  // minor filter, SQL row count substitute, or guessed knowledge.
  for(std::int32_t source=0;source<22;++source){const auto a=detail::bnw_player_team_read(state,source);
   for(std::int32_t target=0;target<22;++target){const auto b=detail::bnw_player_team_read(state,target);
    auto& value=p.next_matrix[static_cast<std::size_t>(source*22+target)];
    if(a==b||!state.setup.players.at(source).alive||!state.setup.players.at(target).alive){value=0;continue;}
    const auto& from=authenticate(a);const auto& to=authenticate(b);std::int32_t difference=0;
    for(std::int64_t slot=0;slot<static_cast<std::int64_t>(rules.collection.slots);++slot){
     const auto from_knows=from.has_tech(slot),to_knows=to.has_tech(slot);
     if(to_knows&&!from_knows){need(difference<INT32_MAX,"Trade native tech difference overflow");++difference;}
    }value=difference;
   }
  }
 }
 visit_retained_trade_tech_operands(state,host,p,[&](std::int64_t n){p.operands.push_back(n);});return p;
}
bool retained_trade_tech_matches(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,const RetainedTradeTechPlan& p){
 std::size_t index=0;bool matches=true;
 visit_retained_trade_tech_operands(state,host,p,[&](std::int64_t n){
  if(index>=p.operands.size()||p.operands[index]!=n)matches=false;++index;
 });return matches&&index==p.operands.size();
}
}
void BnwRetainedTurnCursor::execute_game_trade_technology_difference_prefix(){
 const auto i=impl_;require_current();auto& h=*i->history;
 need(h.frontier.load()==BnwRetainedTurnFrontier::game_trade_turn_unavailable&&h.religion_city_turn_attempted&&
  !h.active_religion_city&&h.units_turn_attempted&&h.operation<UINT64_MAX,"Trade doTurn requires real religion normal return");
 if(h.trade_preparing.exchange(true)){h.quarantine();throw std::runtime_error("Trade doTurn reentry denied");}
 struct Release{std::atomic<bool>& flag;~Release(){flag.store(false);}} release{h.trade_preparing};
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Trade doTurn actual UiHost retired");
  const auto control=require_actual_ui(*ui);const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;
  const auto& host=*i->engine_utility->host;const auto p=prepare_retained_trade_tech(state,host);
  auto raw_options=p.next_options,owned_options=p.next_options;
  const bool options_changed=!retained_options_equal(p.old_options,p.next_options),changed=options_changed||p.old_matrix!=p.next_matrix;
  const auto revision=state.ui_source_revision;need(!changed||revision<UINT64_MAX,"Trade doTurn revision overflow");
  // All callbacks/allocating rules work finish before the shared pure visitor.
  i->check_engine();const auto current_control=require_actual_ui(*ui);
  need(current_control==control&&retained_trade_tech_matches(state,host,p)&&state.ui_source_revision==revision&&
   i->session->state_.get()==allocation.get()&&i->session->state_generation_==h.generation,
   "Trade doTurn relevant actual operands changed before stores");
  h.first_check();need(h.frontier.load()==BnwRetainedTurnFrontier::game_trade_turn_unavailable,"Trade doTurn predecessor changed");
  auto& matrix=*state.world.trade.technology_difference;
  std::fill(matrix.begin(),matrix.end(),-1); // genuine Reset native stores
  if(options_changed){state.setup.game_options.swap(owned_options);state.world.pregame->game_options.swap(raw_options);}
  if(!p.no_science)for(std::size_t n=0;n<p.next_matrix.size();++n)matrix[n]=p.next_matrix[n];
  if(changed)++state.ui_source_revision;
  ++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_leagues_turn_unavailable);
 }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
namespace {
struct RetainedLeaguePlan{
 std::vector<saves::PregameOption> old_options,next_options;
 std::vector<std::int64_t> operands;
 std::int32_t host_votes=1,civ_votes=1,minor_votes=1,votes=0;
 std::array<float,4> defines{};
 const saves::Expansion2PrefixObservation* game=nullptr;
};
const saves::PlayerCoreField& retained_league_player_flag(const saves::SavedPlayerObservation& raw,const char* name){
 const saves::PlayerCoreField* found=nullptr;
 for(const auto& f:raw.core.fields)if(f.name&&std::string_view(f.name)==name){need(!found,"League duplicate actual player flag");found=&f;}
 need(found&&found->encoding==saves::PlayerCoreEncoding::boolean&&found->values.size()==1&&
  (found->values[0]==0||found->values[0]==1),"League current native player boolean unavailable");return *found;
}
std::int64_t retained_league_integer(const std::vector<saves::NamedSaveInteger>& rows,const char* name){
 const saves::NamedSaveInteger* found=nullptr;
 for(const auto& row:rows)if(row.name&&std::string_view(row.name)==name){need(!found,"League duplicate native scalar");found=&row;}
 need(found&&found->value>=INT32_MIN&&found->value<=INT32_MAX,"League native scalar unavailable");return found->value;
}
template<class Emit> std::pair<float,float> visit_retained_league_operands(const LiveSessionState& state,const RetainedLeaguePlan& p,Emit emit){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 const auto* game=std::get_if<saves::Expansion2PrefixObservation>(&state.world.game_state.prefix);
 need(game&&game==p.game&&game->version==1&&state.world.game_state.layout==bnw&&
  state.world.pregame&&state.world.pregame->layout==bnw&&state.world.all_players&&state.world.all_players->layout==bnw&&
  state.world.all_players->players.size()==64&&state.world.all_players->context.player_count==64&&
  state.world.leagues.layout==bnw&&state.world.leagues.version&&*state.world.leagues.version>=1&&
  *state.world.leagues.version<=5&&state.world.leagues.leagues.empty()&&state.cities.leagues.empty(),
  "League genuine unfounded raw/owned Game/league owners unavailable");
 need(retained_options_equal(state.setup.game_options,p.old_options)&&retained_options_equal(state.world.pregame->game_options,p.old_options),
  "League actual GameOptions changed");
 const auto flags=state.world.pregame->archive.flag_arrays.find("minor_nation_civs");
 need(flags!=state.world.pregame->archive.flag_arrays.end()&&flags->second.size()==64,"League native CvPreGame isMinorCiv slots absent");
 emit(static_cast<std::int64_t>(*state.world.leagues.version));emit(state.world.leagues.generated_id_count);
 emit(state.world.leagues.leagues_ever_founded);emit(state.world.leagues.diplomatic_victor);emit(state.world.leagues.last_era_trigger);
 emit(game->scalars[27]);emit(state.setup.game_turn);emit(integer(state.world.pregame->archive,"game_turn"));
 need(state.setup.game_turn==integer(state.world.pregame->archive,"game_turn"),"League source game turn differs");
 float civs=0.0f,minors=0.0f;
 for(std::int32_t id=0;id<63;++id){
  const auto& raw=state.world.all_players->players[static_cast<std::size_t>(id)];
  const auto team=detail::bnw_player_team_read(state,id);emit(team);
  need(raw.core.layout==bnw&&raw.saved_player_id==id&&raw.index==static_cast<std::size_t>(id),"League native player identity differs");
  const auto ever=retained_league_player_flag(raw,"m_bEverAlive").values[0];emit(ever);
  need(state.setup.players.at(id).ever_alive==(ever!=0),"League paired isEverAlive differs");
  const auto alive=retained_league_player_flag(raw,"m_bAlive").values[0];emit(alive);
  need(state.setup.players.at(id).alive==(alive!=0),"League paired IsAlive differs");
  const bool minor=flags->second[static_cast<std::size_t>(id)];emit(minor?1:0);
  if(ever){
   if(minor){
    need(raw.planning.layout==bnw&&raw.planning.dimensions.major==22,"League minor AI source dimensions unavailable");
    const saves::PlayerPlanningRecord* ai=nullptr;
    for(const auto& block:raw.planning.blocks)if(block.name&&std::string_view(block.name)=="minor_civ_ai"){
     need(!ai,"League duplicate actual MinorCivAI");ai=&block;
    }
    need(ai&&retained_league_integer(ai->fields,"version")==1,
     "League actual MinorCivAI Read version absent");
    const saves::NamedSaveInteger* bought=nullptr;
    for(const auto& f:ai->fields)if(f.name&&std::string_view(f.name)=="m_eMajorBoughtOutBy"){need(!bought,"League duplicate bought-out owner");bought=&f;}
    need(bought&&bought->value>=-1&&bought->value<22,"League actual bought-out player domain unavailable");emit(bought->value);
    if(bought->value==-1)minors+=alive?1.0f:0.5f;
   }else civs+=alive?1.0f:0.5f;
  }
  // Source founding scan reaches only living major slots. All those teams
  // must prove the actual saved HasTechForWorldCongress false branch.
  if(id<22&&alive){
   need(team>=0&&team<64&&state.world.teams&&state.world.teams->teams.size()==64,"League living congress team unavailable");
   const auto& actual=state.world.teams->teams[static_cast<std::size_t>(team)];
   need(actual.index==static_cast<std::size_t>(team)&&team_war_field(actual,"id")==team,"League congress team identity differs");
   const auto congress=team_war_field(actual,"has_world_congress_tech");emit(congress);
   need(congress==0,"League reached actual founding/met-everyone supplier");
  }
 }
 return {civs,minors};
}
RetainedLeaguePlan prepare_retained_league(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host){
 RetainedLeaguePlan p;p.game=std::get_if<saves::Expansion2PrefixObservation>(&state.world.game_state.prefix);
 need(state.world.pregame&&retained_options_equal(state.setup.game_options,state.world.pregame->game_options),"League paired GameOptions unavailable");
 p.old_options=state.setup.game_options;auto setup=state.setup;
 need(setup.game_option_value(host.database(),"GAMEOPTION_NO_LEAGUES")!=1,"League disabled diplomacy-victory assertion owner required");
 p.next_options=std::move(setup.game_options);
 const auto report=host.database().prefetch_info("LeagueSpecialSessions",[&](std::uint64_t slot,std::int64_t id,const content::RulesRowView& row){
  need(id>=0&&slot==static_cast<std::uint64_t>(id),"League native special-session cache identity unavailable");
  const auto type=row.text("Type");const auto* identity=type?host.types().find(*type):nullptr;
  need(identity&&identity->table=="LeagueSpecialSessions"&&identity->id==id,"League actual selected session Type/ID differs");
  const auto native=[](std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"League session native delegate integer overflow");return static_cast<std::int32_t>(n);};
  need(row.has_column("UnitedNations"),"League actual UN cache column absent");const bool united=row.boolean("UnitedNations");
  if(united){p.host_votes=native(row.integer("HostDelegates"));p.civ_votes=native(row.integer("CivDelegates"));p.minor_votes=native(row.integer("CityStateDelegates"));}
  return true;
 });
 need(report.all_succeeded()&&report.holes==0&&report.overlapping_ids==0&&report.slots<=INT32_MAX,"League genuine special-session Info cache incomplete");
 const std::array<const char*,4> names{"DIPLO_VICTORY_CIV_DELEGATES_COEFFICIENT","DIPLO_VICTORY_CIV_DELEGATES_CONSTANT",
  "DIPLO_VICTORY_CS_DELEGATES_COEFFICIENT","DIPLO_VICTORY_CS_DELEGATES_CONSTANT"};
 for(std::size_t n=0;n<names.size();++n){const auto value=host.database().global_define(names[n],content::GlobalDefineKind::source_float);
  need(value.found&&value.error.empty()&&std::holds_alternative<double>(value.value),"League genuine selected native float Define unavailable");
  p.defines[n]=static_cast<float>(std::get<double>(value.value));need(std::isfinite(p.defines[n]),"League selected float Define nonfinite");
 }
 const auto [civs,minors]=visit_retained_league_operands(state,p,[&](std::int64_t n){p.operands.push_back(n);});
 const auto portion=[](float count,float coefficient,float constant){
  const float logarithm=static_cast<float>(std::log(static_cast<double>(count)));
  float term=coefficient*logarithm;term=term+constant;if(term<0.0f)term=0.0f;
  need(std::isfinite(term),"League native delegate FP conversion undefined");return term;
 };
 const float a=portion(civs,p.defines[0],p.defines[1]),b=portion(minors,p.defines[2],p.defines[3]);
 const float sum=a+b;const double floored=std::floor(static_cast<double>(sum));
 need(std::isfinite(floored)&&floored>=INT32_MIN&&floored<=INT32_MAX,"League native votes floor/cast overflow");
 const auto native=[](std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"League native delegate chain overflow");return static_cast<std::int32_t>(n);};
 const auto minimum=native(native(std::int64_t(p.host_votes)+p.civ_votes)+1);
 const auto civ_part=native(std::int64_t(p.civ_votes)*static_cast<std::int32_t>(civs));
 const auto minor_part=native(std::int64_t(p.minor_votes)*static_cast<std::int32_t>(minors));
 const auto maximum=native(std::int64_t(native(std::int64_t(p.host_votes)+civ_part))+minor_part);
 p.votes=std::min(maximum,std::max(minimum,static_cast<std::int32_t>(floored)));return p;
}
}
void BnwRetainedTurnCursor::execute_game_leagues_unfounded_prefix(){
 const auto i=impl_;require_current();auto&h=*i->history;
 need(h.frontier.load()==BnwRetainedTurnFrontier::game_leagues_turn_unavailable&&h.operation<UINT64_MAX,"League doTurn requires actual trade return");
 if(h.league_preparing.exchange(true)){h.quarantine();throw std::runtime_error("League doTurn reentry denied");}
 struct Release{std::atomic<bool>&flag;~Release(){flag.store(false);}} release{h.league_preparing};
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"League actual UiHost retired");
  const auto control=require_actual_ui(*ui);const auto allocation=i->session->read_state_lease();auto&state=*i->session->state_;
  const auto p=prepare_retained_league(state,*i->engine_utility->host);
  auto raw_options=p.next_options,owned_options=p.next_options;const bool option_changed=!retained_options_equal(p.old_options,p.next_options);
  const bool changed=option_changed||p.game->scalars[27]!=p.votes;const auto revision=state.ui_source_revision;
  need(!changed||revision<UINT64_MAX,"League actual UI source revision exhausted");control->preflight_league_screen_dirty_setter();
  i->check_engine();need(require_actual_ui(*ui)==control,"League canonical UI changed");
  std::size_t index=0;bool matches=true;
  (void)visit_retained_league_operands(state,p,[&](std::int64_t n){if(index>=p.operands.size()||p.operands[index]!=n)matches=false;++index;});
  need(matches&&index==p.operands.size()&&state.ui_source_revision==revision&&i->session->state_.get()==allocation.get()&&
   i->session->state_generation_==h.generation,"League actual relevant operands changed before stores");h.first_check();
  auto&game=std::get<saves::Expansion2PrefixObservation>(state.world.game_state.prefix);
  if(option_changed){state.setup.game_options.swap(owned_options);state.world.pregame->game_options.swap(raw_options);}
  if(game.scalars[27]!=p.votes)game.scalars[27]=p.votes; // exact SetVotesNeeded conditional store
  control->set_league_screen_dirty(ui::BnwClientLeagueDirtyKind::league_screen);
  if(changed)++state.ui_source_revision;
  ++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_culture_turn_unavailable);
 }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
namespace {
struct RetainedCultureResetPlan{
 std::int32_t actor=-1,needed=0;
 std::vector<std::int64_t> operands;
 saves::PlayerTradeCultureRecord* culture=nullptr;
 std::array<saves::NamedSaveInteger*,4> scalar{};
 std::array<saves::NamedSaveInteger*,2> lengths{};
 std::array<saves::PlayerTradeCultureBlob*,2> tooltip{};
 saves::PlotArrayObservation* current=nullptr; saves::PlotArrayObservation* last=nullptr;
 std::int64_t* owned_opinion=nullptr;
 bool changed=false;
};
template<class Emit> RetainedCultureResetPlan visit_retained_culture_reset(LiveSessionState& state,Emit emit,std::optional<std::int32_t> actual_actor={}){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;RetainedCultureResetPlan p;
 need(state.world.pregame&&state.world.pregame->layout==bnw&&state.world.all_players&&state.world.all_players->layout==bnw&&
  state.world.all_players->players.size()==64&&state.world.teams&&state.world.teams->layout==bnw&&state.world.teams->teams.size()==64,
  "Culture actual complete native roster absent");
 const auto minors=state.world.pregame->archive.flag_arrays.find("minor_nation_civs");
 need(minors!=state.world.pregame->archive.flag_arrays.end()&&minors->second.size()==64,"Culture actual isMinorCiv slots unavailable");
 std::array<bool,64> alive{};std::array<std::int32_t,64> teams{};std::int32_t majors=0;
 for(std::int32_t id=0;id<64;++id){
  teams[static_cast<std::size_t>(id)]=detail::bnw_player_team_read(state,id);emit(teams[static_cast<std::size_t>(id)]);
  const auto& raw=state.world.all_players->players[static_cast<std::size_t>(id)];
  const auto value=retained_league_player_flag(raw,"m_bAlive").values[0];emit(value);
  alive[static_cast<std::size_t>(id)]=value!=0;need(state.setup.players.at(id).alive==alive[static_cast<std::size_t>(id)],"Culture paired actual IsAlive differs");
  const bool minor=minors->second[static_cast<std::size_t>(id)];emit(minor?1:0);
  // CvPlayer::isBarbarian reads GetID()==BARBARIAN_PLAYER, native63.
  if(value&&!minor&&id!=63)++majors;
  if(id<22&&value&&p.actor<0)p.actor=id;
 }
 if(actual_actor){need(*actual_actor>=0&&*actual_actor<22&&alive[static_cast<std::size_t>(*actual_actor)],"Culture actual private loop actor is not an alive native slot");p.actor=*actual_actor;}
 p.needed=majors-1;emit(p.actor);emit(p.needed);
 if(p.actor<0)return p; // Native outer GameCulture loop has no calls.
 const auto owner_team=teams[static_cast<std::size_t>(p.actor)];need(owner_team>=0,"Culture actual living owner NO_TEAM");
 // Both GetNumCivsInfluentialOn and the influence-update loop ask the OTHER
 // team hasMet(ownerTeam). This interval admits their actual unmet branch,
 // and refuses genuine tourism/level suppliers before any speculative write.
 for(std::int32_t id=0;id<22;++id){
  const auto team=teams[static_cast<std::size_t>(id)];need(team>=0,"Culture native GET_TEAM before IsAlive unavailable");
  const auto& raw=state.world.teams->teams[static_cast<std::size_t>(team)];
  need(raw.index==static_cast<std::size_t>(team)&&team_war_field(raw,"id")==team,"Culture actual other team identity differs");
  if(id==p.actor||!alive[static_cast<std::size_t>(id)]||minors->second[static_cast<std::size_t>(id)])continue;
  const saves::PlotArrayObservation* met=nullptr;
  for(const auto& a:raw.arrays)if(a.name&&std::string_view(a.name)=="has_met"){need(!met,"Culture duplicate native hasMet");met=&a;}
  need(met&&met->columns==1&&met->values.size()==64,"Culture native met roster unavailable");
  const auto value=met->values[static_cast<std::size_t>(owner_team)];need(value==0||value==1,"Culture native hasMet boolean malformed");emit(value);
  need(value==0,"Culture reached genuine influence-level/tourism computation");
 }
 auto& player=state.world.all_players->players[static_cast<std::size_t>(p.actor)];
 need(player.trade_culture.layout==bnw&&player.trade_culture.dimensions.major==22,"Culture actual saved player context unavailable");
 for(auto& record:player.trade_culture.blocks)if(record.name&&std::string_view(record.name)=="player_culture"){
  need(!p.culture,"Culture duplicate actual player storage");p.culture=&record;
 }
 need(p.culture&&retained_league_integer(p.culture->fields,"version")==6,"Culture exact six-version native current storage unavailable");
 const auto count=retained_league_integer(p.culture->fields,"cultural_influence_wire_count");
 need(count==22,"Culture saved native influence array count differs");emit(count);
 for(auto& array:p.culture->arrays)if(array.name){
  if(std::string_view(array.name)=="m_aiCulturalInfluence"){need(!p.current,"Culture duplicate current array");p.current=&array;}
  if(std::string_view(array.name)=="m_aiLastTurnCulturalInfluence"){need(!p.last,"Culture duplicate last array");p.last=&array;}
 }
 need(p.current&&p.last&&p.current->columns==1&&p.last->columns==1&&p.current->values.size()==22&&p.last->values.size()==22,
  "Culture genuine native22 influence storage unavailable");
 for(std::size_t n=0;n<22;++n){const auto current=p.current->values[n],last=p.last->values[n];
  need(current>=INT32_MIN&&current<=INT32_MAX&&last>=INT32_MIN&&last<=INT32_MAX,"Culture influence native width exceeded");
  emit(current);emit(last);p.changed|=current!=last;
 }
 const std::array<const char*,4> names{"m_eOpinion","m_ePreferredIdeology","m_iOpinionUnhappiness","m_eOpinionBiggestInfluence"};
 for(std::size_t n=0;n<names.size();++n){
  for(auto& f:p.culture->fields)if(f.name&&std::string_view(f.name)==names[n]){need(!p.scalar[n],"Culture duplicate actual opinion scalar");p.scalar[n]=&f;}
  need(p.scalar[n],"Culture actual opinion scalar absent");const auto value=retained_league_integer(p.culture->fields,names[n]);emit(value);p.changed|=value!=(n==2?0:-1);
 }
 const std::array<const char*,2> blob_names{"m_strOpinionTooltip","m_strOpinionUnhappinessTooltip"};
 const std::array<const char*,2> length_names{"m_strOpinionTooltip_wire_bytes","m_strOpinionUnhappinessTooltip_wire_bytes"};
 for(std::size_t n=0;n<blob_names.size();++n){
  for(auto& b:p.culture->blobs)if(b.name&&std::string_view(b.name)==blob_names[n]){need(!p.tooltip[n],"Culture duplicate tooltip storage");p.tooltip[n]=&b;}
  for(auto& f:p.culture->fields)if(f.name&&std::string_view(f.name)==length_names[n]){need(!p.lengths[n],"Culture duplicate tooltip wire length");p.lengths[n]=&f;}
  need(p.tooltip[n]&&p.lengths[n]&&p.lengths[n]->value>=0&&static_cast<std::uint64_t>(p.lengths[n]->value)==p.tooltip[n]->bytes.size(),"Culture raw tooltip/length owner differs");
  emit(p.lengths[n]->value);for(const auto byte:p.tooltip[n]->bytes)emit(byte);p.changed|=!p.tooltip[n]->bytes.empty();
 }
 const auto owned=state.unhappiness.players.find(p.actor);need(owned!=state.unhappiness.players.end()&&owned->second.public_opinion==p.scalar[2]->value,
  "Culture actual paired public opinion cache differs");p.owned_opinion=&owned->second.public_opinion;emit(*p.owned_opinion);
 return p;
}
bool retained_culture_storage_same(const RetainedCultureResetPlan&a,const RetainedCultureResetPlan&b){
 return a.actor==b.actor&&a.needed==b.needed&&a.culture==b.culture&&a.scalar==b.scalar&&a.lengths==b.lengths&&
  a.tooltip==b.tooltip&&a.current==b.current&&a.last==b.last&&a.owned_opinion==b.owned_opinion&&a.changed==b.changed;
}
}
void BnwRetainedTurnCursor::execute_game_culture_first_unmet_prefix(){
 const auto i=impl_;require_current();auto&h=*i->history;
 need(h.frontier.load()==BnwRetainedTurnFrontier::game_culture_turn_unavailable&&h.operation<UINT64_MAX,"Culture requires actual league normal return");
 if(h.culture_preparing.exchange(true)){h.quarantine();throw std::runtime_error("Culture doTurn reentry denied");}
 struct Release{std::atomic<bool>&flag;~Release(){flag.store(false);}} release{h.culture_preparing};
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Culture actual UiHost retired");const auto control=require_actual_ui(*ui);
  const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;
  std::vector<std::int64_t> operands;const auto p=visit_retained_culture_reset(state,[&](std::int64_t n){operands.push_back(n);});
  const auto revision=state.ui_source_revision;need(!p.changed||revision<UINT64_MAX,"Culture actual UI revision exhausted");
  i->check_engine();need(require_actual_ui(*ui)==control,"Culture canonical UI changed");
  std::size_t index=0;bool matches=true;
  const auto final=visit_retained_culture_reset(state,[&](std::int64_t n){if(index>=operands.size()||operands[index]!=n)matches=false;++index;});
  need(matches&&index==operands.size()&&retained_culture_storage_same(p,final)&&state.ui_source_revision==revision&&
   i->session->state_.get()==allocation.get()&&i->session->state_generation_==h.generation,"Culture actual relevant operands changed before stores");h.first_check();
  if(p.actor>=0){
   // Native forward-loop copy; proven no reached GetInfluencePerTurn call.
   for(std::size_t n=0;n<22;++n)p.last->values[n]=p.current->values[n];
   // Actual initial DoPublicOpinion body, before branch Defines/Locale.
   p.scalar[0]->value=-1;p.scalar[1]->value=-1;p.scalar[2]->value=0;*p.owned_opinion=0;
   for(std::size_t n=0;n<p.tooltip.size();++n){p.tooltip[n]->bytes.clear();p.lengths[n]->value=0;}
   p.scalar[3]->value=-1;h.active_culture_player=p.actor;
  }
  if(p.changed)++state.ui_source_revision;
  ++h.operation;
  h.frontier.store(p.actor<0?BnwRetainedTurnFrontier::engine_ui_end_turn_reset_unavailable:BnwRetainedTurnFrontier::culture_public_opinion_source_owner);
 }catch(...){h.quarantine();throw;}
}
std::optional<std::int32_t> BnwRetainedTurnCursor::active_game_culture_player()const{require_current();return impl_->history->active_culture_player;}
}

namespace civ5::runtime {
BnwRetainedTurnView BnwRetainedTurnCursor::advance_game_tail_to_next_required_owner(){
 const auto i=impl_;require_current();auto&h=*i->history;
 need(h.engine_ui_returned&&h.selection_cycle_attempted,"Retained game tail requires actual EngineUI return");
 if(h.tail_dispatching.exchange(true)){h.quarantine();throw std::runtime_error("Retained game tail reentry denied");}
 struct Release{std::atomic<bool>&flag;~Release(){flag.store(false);}} release{h.tail_dispatching};
 try{
  // Each concrete executor owns its own actual predecessor/once checks and
  // in-place stores. No supplied actor, progress, clock or completion flags.
  // Unsupported reached effects return their genuine frontier immediately.
  for(std::size_t calls=0;calls<8;++calls){
   require_current();const auto before=h.frontier.load();
   switch(before){
   case BnwRetainedTurnFrontier::barbarian_camps_turn:execute_barbarian_camps_prefix();break;
   case BnwRetainedTurnFrontier::barbarian_units_turn:execute_barbarian_units_prefix();break;
   case BnwRetainedTurnFrontier::game_religions_turn:execute_game_religion_first_city_prefix();break;
   case BnwRetainedTurnFrontier::religion_spy_pressure_unavailable:execute_game_religion_no_pressure_prefix();break;
   case BnwRetainedTurnFrontier::game_trade_turn_unavailable:execute_game_trade_technology_difference_prefix();break;
   case BnwRetainedTurnFrontier::game_leagues_turn_unavailable:execute_game_leagues_unfounded_prefix();break;
   case BnwRetainedTurnFrontier::game_culture_turn_unavailable:execute_game_culture_first_unmet_prefix();break;
   default:h.game_tail_earned_revision=i->session->state_->ui_source_revision;return view();
   }
   require_current();need(h.frontier.load()!=before,"Retained game tail real executor did not advance");
  }
  h.game_tail_earned_revision=i->session->state_->ui_source_revision;return view();
 }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
namespace {
struct RetainedOpinionBranch {std::int32_t id;std::uint64_t hash;bool level;};
template<class Emit> std::int32_t retained_current_ideology(const LiveSessionState& state,std::int32_t actor,
 const std::vector<RetainedOpinionBranch>& rules,Emit emit){
 const auto& raw=state.world.all_players->players.at(static_cast<std::size_t>(actor));
 need(raw.saved_player_id==actor&&raw.strategy.layout==saves::GameStateLayout::expansion2_version1,"Culture actual Policy owner identity unavailable");
 const saves::TeamHashedArray* unlocked=nullptr;
 for(const auto& block:raw.strategy.blocks)if(block.name&&std::string_view(block.name)=="policies")
  for(const auto& value:block.hashes)if(value.name&&std::string_view(value.name)=="branch_unlocked"){
   need(!unlocked,"Culture duplicate native branch-unlocked owner");unlocked=&value;
  }
 need(unlocked&&unlocked->wire_count>=0&&static_cast<std::uint64_t>(unlocked->wire_count)==unlocked->rows.size(),"Culture actual unlocked-branch native wire count differs");
 emit(unlocked->wire_count);
 for(const auto& row:unlocked->rows){
  emit(static_cast<std::int64_t>(row.type_hash));emit(static_cast<std::int64_t>(row.values.size()));
  if(!row.type_hash){need(row.values.empty(),"Culture null branch hash has payload");continue;}
  need(std::find_if(rules.begin(),rules.end(),[&](const auto& rule){return rule.hash==row.type_hash;})!=rules.end()&&
   row.values.size()==1&&(row.values[0]==0||row.values[0]==1),"Culture unknown/malformed actual Policy branch flag");emit(row.values[0]);
 }
 // CvPlayerPolicies::GetLateGamePolicyTree: forward native slots, first
 // PurchaseByLevel && IsPolicyBranchUnlocked. Native Read last hash wins.
 for(const auto& rule:rules){
  bool value=false;for(const auto& row:unlocked->rows)if(row.type_hash==rule.hash)value=row.values[0]!=0;
  if(rule.level&&value)return rule.id;
 }
 return -1;
}
}
void BnwRetainedTurnCursor::execute_game_culture_public_opinion_prefix(){execute_game_culture_public_opinion_impl(nullptr,nullptr);}
void BnwRetainedTurnCursor::execute_game_culture_public_opinion_with_actual_stock(BnwSelectedStockLuaBootstrap& stock,const BnwStockLuaAttemptLease& lease){execute_game_culture_public_opinion_impl(&stock,&lease);}
void BnwRetainedTurnCursor::execute_game_culture_public_opinion_impl(BnwSelectedStockLuaBootstrap* stock,const BnwStockLuaAttemptLease* lease){
 const auto i=impl_;require_current();auto& h=*i->history;
 need(h.frontier.load()==BnwRetainedTurnFrontier::culture_public_opinion_source_owner&&h.active_culture_player&&h.operation<UINT64_MAX,"Culture opinion requires actual source reset/predecessor");
 if(h.culture_preparing.exchange(true)){h.quarantine();throw std::runtime_error("Culture opinion reentry denied");}
 struct Release{std::atomic<bool>& value;~Release(){value.store(false);}} release{h.culture_preparing};
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Culture selected UI source retired");const auto control=require_actual_ui(*ui);
  const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;const auto revision=state.ui_source_revision;
  const auto& host=*i->engine_utility->host;const auto actor=*h.active_culture_player;
  std::array<std::int32_t,3> definitions{};bool has_branches=true;
  constexpr std::array<const char*,3> names{"POLICY_BRANCH_FREEDOM","POLICY_BRANCH_AUTOCRACY","POLICY_BRANCH_ORDER"};
  for(std::size_t n=0;n<names.size();++n){
   const auto value=host.database().global_define(names[n],content::GlobalDefineKind::integer);
   const auto* integer_value=std::get_if<std::int64_t>(&value.value);
   need(value.found&&value.sqlite_code==0&&value.error.empty()&&integer_value&&*integer_value>=-1&&*integer_value<=INT32_MAX,"Culture actual postprocessed ideology Define unavailable");
   definitions[n]=static_cast<std::int32_t>(*integer_value);has_branches&=definitions[n]!=-1;
  }
  std::vector<RetainedOpinionBranch> rules;std::string overview;
  if(has_branches){
   const auto collection=host.database().prefetch_info("PolicyBranchTypes",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
    const auto type=row.text("Type").value_or("");const auto* actual=host.types().find(type);const auto* hashed=actual?host.types().find_hash(actual->hash):nullptr;
    need(slot<=INT32_MAX&&actual&&hashed&&actual->table=="PolicyBranchTypes"&&actual->id==static_cast<std::int64_t>(slot)&&actual->hash&&
     hashed->id==actual->id&&hashed->table==actual->table&&hashed->type==actual->type,"Culture actual branch Info/hash/remap differs");
    rules.push_back({static_cast<std::int32_t>(slot),actual->hash,row.boolean("PurchaseByLevel")});return true;
   });
   need(collection.all_succeeded()&&collection.overlapping_ids==0&&collection.rows==rules.size(),"Culture complete actual native branch cache unavailable");
   std::sort(rules.begin(),rules.end(),[](const auto&a,const auto&b){return a.id<b.id;});
   for(auto id:definitions)need(std::find_if(rules.begin(),rules.end(),[&](const auto& branch){return branch.id==id;})!=rules.end(),"Culture actual ideology Define points to null native branch");
   const auto receiver=ui->engine_ui_turn_owner_;need(bool(receiver),"Culture actual selected Engine Locale owner absent");
   if(stock){need(lease,"Culture actual stock lease absent");stock->bind_engine_ui_localization_owned(*receiver,*lease);}
   // Source-backed selected TextData translation, not a manufactured native
   // Locale registry or caller language. No arguments in this actual key.
   overview=receiver->selected_localization_text("TXT_KEY_CO_OPINION_TT_OVERVIEW");
   need(overview.find('\0')==std::string::npos&&overview.size()<=UINT32_MAX,"Culture actual overview CvString unsupported");
  }
  std::vector<std::int64_t> operands;
  const auto before=visit_retained_culture_reset(state,[&](std::int64_t value){operands.push_back(value);},h.active_culture_player);
  need(before.actor==actor&&!before.changed,"Culture native initial reset/influence copy changed before opinion return");
  std::int32_t ideology=-1;
  if(has_branches)ideology=retained_current_ideology(state,actor,rules,[&](std::int64_t value){operands.push_back(value);});
  std::vector<std::uint8_t> text(overview.begin(),overview.end());const bool changed=has_branches&&!text.empty();
  need(!changed||revision<UINT64_MAX,"Culture opinion actual UI revision exhausted");
  i->check_engine();need(require_actual_ui(*ui)==control,"Culture opinion actual canonical UI changed");
  if(has_branches){const auto receiver=ui->engine_ui_turn_owner_;need(bool(receiver),"Culture Locale retired");receiver->require_selected_localization_current();}
  for(std::size_t n=0;n<names.size();++n){const auto value=host.database().global_define(names[n],content::GlobalDefineKind::integer);const auto* actual=std::get_if<std::int64_t>(&value.value);need(value.found&&value.sqlite_code==0&&value.error.empty()&&actual&&*actual==definitions[n],"Culture actual ideology Defines changed");}
  // All DB/localization/storage allocations and callbacks are over. Rewalk
  // raw source operands and stable current addresses without temporary data.
  std::size_t index=0;bool matches=true;const auto compare=[&](std::int64_t value){if(index>=operands.size()||operands[index]!=value)matches=false;++index;};
  const auto final=visit_retained_culture_reset(state,compare,h.active_culture_player);
  if(has_branches)need(retained_current_ideology(state,actor,rules,compare)==ideology,"Culture current actual ideology changed");
  need(matches&&index==operands.size()&&retained_culture_storage_same(before,final)&&state.ui_source_revision==revision&&
   i->session->state_.get()==allocation.get()&&i->session->state_generation_==h.generation,"Culture actual opinion operands changed before stores");h.first_check();
  if(has_branches){before.tooltip[0]->bytes.swap(text);before.lengths[0]->value=static_cast<std::int64_t>(before.tooltip[0]->bytes.size());}
  if(changed)++state.ui_source_revision;
  h.game_tail_earned_revision=state.ui_source_revision;
  ++h.operation;h.frontier.store(ideology>=0?BnwRetainedTurnFrontier::culture_ideology_pressure_owner:BnwRetainedTurnFrontier::culture_notification_source_owner);
 }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::game_tail_actual_revision()const{
 require_current();const auto i=impl_;const auto&h=*i->history;
 need(h.engine_ui_returned&&h.game_tail_earned_revision&&*h.game_tail_earned_revision==i->session->state_->ui_source_revision,"Retained game tail actual earned revision absent/changed");
 return *h.game_tail_earned_revision;
}
}

namespace civ5::runtime {
namespace {
struct RetainedCultureNotificationPlan {
 RetainedCultureResetPlan culture;
 saves::PlayerAdministrationRecord* notifications=nullptr;
 saves::NamedSaveInteger* reported_one=nullptr;
 bool one_condition=false,changed=false;
};
template<class Emit> RetainedCultureNotificationPlan visit_retained_culture_notification(
 LiveSessionState& state,std::optional<std::int32_t> victory,Emit emit,std::optional<std::int32_t> actual_actor){
 RetainedCultureNotificationPlan p;p.culture=visit_retained_culture_reset(state,emit,actual_actor);
 need(p.culture.actor>=0,"Culture notification actual native first player absent");
 auto& player=state.world.all_players->players.at(static_cast<std::size_t>(p.culture.actor));
 // Both calls to GetNumCivsInfluentialOn use the same proven native unmet
 // branch from GetInfluenceLevel (returns NO_INFLUENCE_LEVEL, not guessed0).
 for(std::size_t n=0;n<22;++n)need(p.culture.current->values[n]==p.culture.last->values[n],"Culture no-influence notification predecessor changed");
 need(p.culture.scalar[0]->value==-1&&p.culture.scalar[1]->value==-1&&p.culture.scalar[2]->value==0&&p.culture.scalar[3]->value==-1,
  "Culture notification requires genuine no-ideology opinion return");
 // CvPlayer::GetNotifications is evaluated even when no Add is reached.
 need(player.administration.layout==saves::GameStateLayout::expansion2_version1&&player.administration.blocks.size()==3,
  "Culture nullable notification native owner unavailable");
 p.notifications=&player.administration.blocks.front();
 need(p.notifications->name&&std::string_view(p.notifications->name)=="notifications","Culture actual GetNotifications owner identity differs");
 const auto present=retained_league_integer(p.notifications->fields,"has_notifications");
 need(present==0||present==1,"Culture actual nullable notification pointer malformed");emit(present);
 bool valid=true;emit(victory.value_or(-1));
 if(victory){
  const auto flags=state.world.pregame->archive.flag_arrays.find("victories");
  need(flags!=state.world.pregame->archive.flag_arrays.end()&&*victory>=0&&static_cast<std::size_t>(*victory)<flags->second.size(),
   "Culture actual native IsVictoryValid storage unavailable");
  valid=flags->second[static_cast<std::size_t>(*victory)];
 }
 emit(valid?1:0);
 const auto two=retained_league_integer(p.culture.culture->fields,"m_bReportedTwoCivsAway");
 const auto one=retained_league_integer(p.culture.culture->fields,"m_bReportedOneCivAway");
 need((two==0||two==1)&&(one==0||one==1),"Culture native milestone flags malformed");emit(two);emit(one);
 for(auto& f:p.culture.culture->fields)if(f.name&&std::string_view(f.name)=="m_bReportedOneCivAway"){
  need(!p.reported_one,"Culture duplicate actual one-civ flag");p.reported_one=&f;
 }
 need(p.reported_one,"Culture actual one-civ flag destination absent");
 // The first three conditions are false for exact previous/current count0.
 // The fourth can still be true after eliminations: never silently skip it.
 if(!one&&0==p.culture.needed-1){
  const auto minors=state.world.pregame->archive.flag_arrays.find("minor_nation_civs");
  need(minors!=state.world.pregame->archive.flag_arrays.end()&&minors->second.size()==64,"Culture native minor roster unavailable");
  std::int32_t ever=0;
  for(std::int32_t id=0;id<63;++id){
   const auto& raw=state.world.all_players->players.at(static_cast<std::size_t>(id));
   const auto value=retained_league_player_flag(raw,"m_bEverAlive").values[0];emit(value);
   need(state.setup.players.at(id).ever_alive==(value!=0),"Culture paired actual ever-alive differs");
   const bool minor=minors->second[static_cast<std::size_t>(id)];emit(minor?1:0);if(value&&!minor)++ever;
  }
  emit(ever);p.one_condition=ever>=3;
  need(!p.one_condition||!valid,"Culture reached actual within-one notification Add/selected Locale owner");
 }
 p.changed=p.one_condition;return p;
}
bool retained_culture_notification_same(const RetainedCultureNotificationPlan&a,const RetainedCultureNotificationPlan&b){
 return retained_culture_storage_same(a.culture,b.culture)&&a.notifications==b.notifications&&a.reported_one==b.reported_one&&
  a.one_condition==b.one_condition&&a.changed==b.changed;
}
}
void BnwRetainedTurnCursor::execute_game_culture_unmet_notifications_standalone_prefix(){
 const auto i=impl_;require_current();auto&h=*i->history;
 need(h.frontier.load()==BnwRetainedTurnFrontier::culture_notification_source_owner&&h.active_culture_player&&h.operation<UINT64_MAX,
  "Culture notifications require actual public-opinion normal return");
 if(h.culture_preparing.exchange(true)){h.quarantine();throw std::runtime_error("Culture notification reentry denied");}
 struct Release{std::atomic<bool>&value;~Release(){value.store(false);}}release{h.culture_preparing};
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Culture selected UI source retired");const auto control=require_actual_ui(*ui);
  const auto allocation=i->session->read_state_lease();auto&state=*i->session->state_;const auto revision=state.ui_source_revision;
  saves::require_supported_standalone_services(state.standalone_services);
  need(h.game_tail_earned_revision&&*h.game_tail_earned_revision==revision,"Culture notification actual earned predecessor absent");
  const auto&host=*i->engine_utility->host;std::optional<std::int32_t> victory;
  if(const auto*info=host.types().find("VICTORY_CULTURAL")){
   const auto*hash=host.types().find_hash(info->hash);
   need(info->table=="Victories"&&info->id>=0&&info->id<=INT32_MAX&&info->hash&&hash&&hash->type==info->type&&hash->id==info->id&&hash->table==info->table,
    "Culture actual VICTORY_CULTURAL Info/hash/remap differs");victory=static_cast<std::int32_t>(info->id);
  }
  std::vector<std::int64_t> operands;const auto before=visit_retained_culture_notification(state,victory,[&](std::int64_t n){operands.push_back(n);},h.active_culture_player);
  need(before.culture.actor==*h.active_culture_player,"Culture notification actual retained player changed");
  need(!before.changed||revision<UINT64_MAX,"Culture milestone UI revision exhausted");
  i->check_engine();need(require_actual_ui(*ui)==control,"Culture notification canonical UI changed");
  std::size_t index=0;bool matches=true;const auto final=visit_retained_culture_notification(state,victory,[&](std::int64_t n){if(index>=operands.size()||operands[index]!=n)matches=false;++index;},h.active_culture_player);
  need(matches&&index==operands.size()&&retained_culture_notification_same(before,final)&&state.ui_source_revision==revision&&
   i->session->state_.get()==allocation.get()&&i->session->state_generation_==h.generation,"Culture notification actual operands changed before stores");h.first_check();
  saves::require_supported_standalone_services(state.standalone_services);
  if(before.changed){before.reported_one->value=1;++state.ui_source_revision;}
  // Civis' accepted first-BNW platform backend has no Steamworks. This is
  // explicitly disabled, never UnlockAchievement success. The source subtree
  // only changes local variables and calls XP2_35/38; game effects are above.
  h.culture_platform_status=BnwCulturePlatformStatus::disabled_standalone;
  h.game_tail_earned_revision=state.ui_source_revision;++h.operation;
  h.frontier.store(BnwRetainedTurnFrontier::culture_log_source_owner);
 }catch(...){h.quarantine();throw;}
}
BnwCulturePlatformStatus BnwRetainedTurnCursor::game_culture_platform_status()const{
 require_current();return impl_->history->culture_platform_status;
}
}

namespace civ5::runtime {
namespace {
template<class Emit> std::optional<std::int32_t> retained_culture_next_alive(
 LiveSessionState& state,std::int32_t actor,Emit emit){
 need(actor>=0&&actor<22&&state.world.all_players&&state.world.all_players->players.size()==64,"Culture native outer-loop predecessor unavailable");
 for(std::int32_t id=actor+1;id<22;++id){
  const auto& raw=state.world.all_players->players.at(static_cast<std::size_t>(id));
  need(raw.index==static_cast<std::uint64_t>(id)&&raw.saved_player_id==id,"Culture next native player slot identity differs");
  const auto alive=retained_league_player_flag(raw,"m_bAlive").values[0];emit(id);emit(alive);
  need(state.setup.players.at(id).alive==(alive!=0),"Culture next paired native IsAlive differs");
  if(alive)return id; // CvGameCulture tests IsAlive only, not IsMinorCiv.
 }
 return {};
}
}
void BnwRetainedTurnCursor::execute_game_culture_standalone_diagnostics_return(){
 const auto i=impl_;require_current();auto&h=*i->history;
 need(h.frontier.load()==BnwRetainedTurnFrontier::culture_log_source_owner&&h.active_culture_player&&
  h.culture_platform_status==BnwCulturePlatformStatus::disabled_standalone&&h.completed_culture_players<22&&h.operation<UINT64_MAX,
  "Culture diagnostic return requires real notifications/platform predecessor");
 if(h.culture_preparing.exchange(true)){h.quarantine();throw std::runtime_error("Culture diagnostic return reentry denied");}
 struct Release{std::atomic<bool>&value;~Release(){value.store(false);}}release{h.culture_preparing};
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Culture diagnostic selected UI source retired");const auto control=require_actual_ui(*ui);
  const auto allocation=i->session->read_state_lease();auto&state=*i->session->state_;const auto revision=state.ui_source_revision;
  saves::require_supported_standalone_services(state.standalone_services);
  need(h.game_tail_earned_revision&&*h.game_tail_earned_revision==revision,"Culture diagnostic earned revision absent");
  const auto actor=*h.active_culture_player;std::vector<std::int64_t> operands;
  const auto next=retained_culture_next_alive(state,actor,[&](std::int64_t n){operands.push_back(n);});
  i->check_engine();need(require_actual_ui(*ui)==control,"Culture diagnostic actual UI changed");
  saves::require_supported_standalone_services(state.standalone_services);
  std::size_t index=0;bool matches=true;const auto final=retained_culture_next_alive(state,actor,[&](std::int64_t n){if(index>=operands.size()||operands[index]!=n)matches=false;++index;});
  need(next==final&&matches&&index==operands.size()&&state.ui_source_revision==revision&&i->session->state_.get()==allocation.get()&&
   i->session->state_generation_==h.generation,"Culture native next-player operands changed");h.first_check();
  // Session schema1 intentionally has no native diagnostic backend. This is
  // not a claim that original GC.getLogging/getAILogging bytes are false, nor
  // a log-delivered acknowledgement. SDK LogCultureData contains only local
  // values and diagnostic log calls; gameplay and notifications already ran.
  h.culture_diagnostic_status=BnwCultureDiagnosticStatus::backend_disabled;
  ++h.completed_culture_players;h.next_culture_player=next;++h.operation;
  h.game_tail_earned_revision=revision;
  h.frontier.store(next?BnwRetainedTurnFrontier::culture_next_player_source_owner:BnwRetainedTurnFrontier::engine_ui_end_turn_reset_unavailable);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::execute_game_culture_next_player_unmet_prefix(){
 const auto i=impl_;require_current();auto&h=*i->history;
 need(h.frontier.load()==BnwRetainedTurnFrontier::culture_next_player_source_owner&&h.active_culture_player&&h.next_culture_player&&h.operation<UINT64_MAX,
  "Culture next reset requires actual preceding player normal return");
 if(h.culture_preparing.exchange(true)){h.quarantine();throw std::runtime_error("Culture next-player reset reentry denied");}
 struct Release{std::atomic<bool>&value;~Release(){value.store(false);}}release{h.culture_preparing};
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"Culture next reset selected UI source retired");const auto control=require_actual_ui(*ui);
  const auto allocation=i->session->read_state_lease();auto&state=*i->session->state_;const auto revision=state.ui_source_revision;
  saves::require_supported_standalone_services(state.standalone_services);
  const auto next=*h.next_culture_player,previous=*h.active_culture_player;
  std::vector<std::int64_t> operands;
  need(retained_culture_next_alive(state,previous,[&](std::int64_t n){operands.push_back(n);})==h.next_culture_player,"Culture native next source slot changed");
  const auto before=visit_retained_culture_reset(state,[&](std::int64_t n){operands.push_back(n);},next);
  need(before.actor==next&&(!before.changed||revision<UINT64_MAX),"Culture next source reset/UiRev unsupported");
  i->check_engine();need(require_actual_ui(*ui)==control,"Culture next reset actual UI changed");
  saves::require_supported_standalone_services(state.standalone_services);
  std::size_t index=0;bool matches=true;const auto compare=[&](std::int64_t n){if(index>=operands.size()||operands[index]!=n)matches=false;++index;};
  need(retained_culture_next_alive(state,previous,compare)==h.next_culture_player,"Culture final native next source changed");
  const auto final=visit_retained_culture_reset(state,compare,next);
  need(matches&&index==operands.size()&&retained_culture_storage_same(before,final)&&state.ui_source_revision==revision&&
   i->session->state_.get()==allocation.get()&&i->session->state_generation_==h.generation,"Culture next reset actual operands changed before stores");h.first_check();
  for(std::size_t n=0;n<22;++n)before.last->values[n]=before.current->values[n];
  before.scalar[0]->value=-1;before.scalar[1]->value=-1;before.scalar[2]->value=0;*before.owned_opinion=0;
  for(std::size_t n=0;n<before.tooltip.size();++n){before.tooltip[n]->bytes.clear();before.lengths[n]->value=0;}
  before.scalar[3]->value=-1;if(before.changed)++state.ui_source_revision;
  h.active_culture_player=next;h.next_culture_player.reset();h.culture_platform_status=BnwCulturePlatformStatus::not_reached;
  h.culture_diagnostic_status=BnwCultureDiagnosticStatus::not_reached;h.game_tail_earned_revision=state.ui_source_revision;
  ++h.operation;h.frontier.store(BnwRetainedTurnFrontier::culture_public_opinion_source_owner);
 }catch(...){h.quarantine();throw;}
}
BnwCultureDiagnosticStatus BnwRetainedTurnCursor::game_culture_diagnostic_status()const{require_current();return impl_->history->culture_diagnostic_status;}
std::uint64_t BnwRetainedTurnCursor::completed_game_culture_players()const{require_current();return impl_->history->completed_culture_players;}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_fresh_engine_ui_end_turn_reset(){execute_engine_ui_end_turn_reset_impl(nullptr);}
void BnwRetainedTurnCursor::execute_engine_ui_end_turn_reset_with_actual_stock(const BnwStockLuaAttemptLease& source){
 execute_engine_ui_end_turn_reset_impl(&source);
}
void BnwRetainedTurnCursor::execute_engine_ui_end_turn_reset_impl(const BnwStockLuaAttemptLease* source){
 const auto i=impl_;auto& h=*i->history;h.first_check();
 need(h.frontier.load()==BnwRetainedTurnFrontier::engine_ui_end_turn_reset_unavailable&&!h.end_turn_ui_reset_attempted&&h.operation<UINT64_MAX,
  "End-turn UI reset requires genuine completed gameCulture outer loop");
 h.end_turn_ui_reset_attempted=true; // Once reserve before callbackful providers.
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"End-turn UI actual SourceUI retired");
  const auto control=require_actual_ui(*ui);const auto allocation=i->session->read_state_lease();
  const auto revision=allocation->ui_source_revision,generation=i->session->state_generation();
  const auto engine=ui->engine_ui_turn_owner_;need(bool(engine),"End-turn UI actual canonical Engine owner absent");
  // Exclude exactly these genuine retained Cursor locals from outside counts.
  // This private constructor is available only to the actual Cursor owner.
  ::civ5::ui::BnwEngineUiTurnOwner::EndTurnCursorPin borrowed(*engine,*ui,*allocation);
  const auto serial=source?source->operation_serial():0;
  if(source){
   source->require_operation(serial);
   {const auto actual=source->world_lease();
    need(actual.get()==allocation.get()&&!actual.owner_before(allocation)&&!allocation.owner_before(actual),
     "End-turn Cursor Lease differs from genuine SAME session world control");}
   source->require_operation(serial);
  }
  i->check_engine();need(require_actual_ui(*ui)==control,"End-turn UI canonical Selection changed before setters");
  const auto returned=source?engine->reset_end_turn_flags_owned(*source):engine->reset_fresh_end_turn_flags();
  if(source)source->require_operation(serial);
  i->check_engine();engine->require_fresh_end_turn_flags_return(returned);
  need(ui->engine_ui_turn_owner_==engine&&require_actual_ui(*ui)==control&&i->session->read_state_lease()==allocation&&
   i->session->state_generation()==generation&&generation==h.generation&&allocation->ui_source_revision==revision,
   "End-turn UI actual source/world/generation changed on return");
  engine->require_fresh_end_turn_flags_return(returned);
  if(source)source->require_operation(serial);
  h.end_turn_ui_reset_return=returned;
  h.game_tail_earned_revision=revision;++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_ai_autoplay_source_owner);
 }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_ai_autoplay_zero_return(){
 const auto i=impl_;auto& h=*i->history;h.first_check();
 need(h.frontier.load()==BnwRetainedTurnFrontier::game_ai_autoplay_source_owner&&h.end_turn_ui_reset_return&&
  !h.ai_autoplay_tail_attempted&&h.operation<UINT64_MAX,"AI autoplay tail requires genuine UI reset predecessor");
 h.ai_autoplay_tail_attempted=true; // Once before any callbackful source provider.
 try{
  i->check_engine();const auto ui=i->cycle_ui.lock();need(bool(ui),"AI autoplay tail actual UI retired");
  const auto control=require_actual_ui(*ui);const auto engine=ui->engine_ui_turn_owner_;
  need(bool(engine),"AI autoplay tail actual Engine owner absent");engine->require_fresh_end_turn_flags_return(*h.end_turn_ui_reset_return);
  const auto allocation=i->session->read_state_lease();const auto revision=allocation->ui_source_revision;
  const auto* prefix=std::get_if<saves::Expansion2PrefixObservation>(&allocation->world.game_state.prefix);
  need(prefix&&prefix->version==1&&std::string_view(saves::Expansion2PrefixObservation::scalar_names[21])=="ai_auto_play"&&prefix->scalars[21]==0,
   "AI autoplay positive/unknown scalar requires actual decrement/Revive owner");
  // SDK getAIAutoPlay reads this native scalar. 0>0 is false; neither
  // changeAIAutoPlay nor ReviveActivePlayer executes. No empty guess or ACK.
  i->check_engine();engine->require_fresh_end_turn_flags_return(*h.end_turn_ui_reset_return);
  const auto* after=std::get_if<saves::Expansion2PrefixObservation>(&i->session->state_->world.game_state.prefix);
  need(i->session->read_state_lease()==allocation&&require_actual_ui(*ui)==control&&ui->engine_ui_turn_owner_==engine&&
   i->session->state_generation()==h.generation&&allocation->ui_source_revision==revision&&after&&after->version==1&&after->scalars[21]==0,
   "AI autoplay actual zero read/source/world changed before branch return");
  h.game_tail_earned_revision=revision;++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_increment_source_owner);
 }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_net_log_prefix(
 std::shared_ptr<const BnwNetMessageDebugManagerOwner> supplied){
 const auto i=impl_;auto& h=*i->history;h.first_check();
 need(h.frontier.load()==BnwRetainedTurnFrontier::game_turn_increment_source_owner&&h.ai_autoplay_tail_attempted&&h.end_turn_ui_reset_return&&
  !h.game_turn_net_log_attempted&&!h.game_turn_net_log_delivery&&h.operation<UINT64_MAX,
  "setGameTurn net log requires genuine completed Culture/UI/autoplay predecessors");
 h.game_turn_net_log_attempted=true; // Once reserve BEFORE source callbacks or OS effects.
 try{
  i->check_engine();need(bool(supplied),"setGameTurn actual Boot net manager absent");
  const auto manager=supplied->retain_factory_owner_without_callbacks();
  need(manager.get()==supplied.get()&&!manager.owner_before(supplied)&&!supplied.owner_before(manager),"setGameTurn supplied manager control differs");
  const auto& host=*i->engine_utility->host;manager->require_same_host(host);
  const auto allocation=i->session->read_state_lease();const auto generation=i->session->state_generation();
  const auto revision=allocation->ui_source_revision;const auto old_turn=allocation->setup.game_turn;
  need(old_turn>=0&&old_turn<INT32_MAX&&old_turn==h.turn&&allocation->world.pregame&&
   integer(allocation->world.pregame->archive,"game_turn")==old_turn,
   "setGameTurn native old/paired clock unavailable or increment would overflow");
  const auto next_turn=static_cast<std::int32_t>(old_turn+1);
  const auto* prefix=std::get_if<saves::Expansion2PrefixObservation>(&allocation->world.game_state.prefix);
  need(prefix&&prefix->version==1&&!allocation->game_clock.pending,"setGameTurn existing pending clock/source requires real recovery");
  const auto prefix_scalars=prefix->scalars;const auto elapsed=allocation->maintenance.elapsed_turns;
  const auto sent_auto=allocation->game_clock.sent_auto_moves;
  const saves::NamedSaveFlag* score=nullptr;
  for(const auto& flag:allocation->world.game_state.flags)if(flag.name&&std::string_view(flag.name)=="score_dirty"){
   need(!score,"setGameTurn duplicate native ScoreDirty storage");score=&flag;
  }
  need(score,"setGameTurn native ScoreDirty storage absent");const auto score_before=score->value;
  const auto unchanged=[&]{
   i->check_engine();manager->require_same_host(host);i->check_engine();
   const auto current=i->session->read_state_lease();const auto* current_prefix=std::get_if<saves::Expansion2PrefixObservation>(&current->world.game_state.prefix);
   const saves::NamedSaveFlag* current_score=nullptr;
   for(const auto& flag:current->world.game_state.flags)if(flag.name&&std::string_view(flag.name)=="score_dirty"){
    need(!current_score,"setGameTurn duplicate native ScoreDirty on source return");current_score=&flag;
   }
   need(current==allocation&&i->session->state_generation()==generation&&generation==h.generation&&
    current->ui_source_revision==revision&&current->setup.game_turn==old_turn&&h.turn==old_turn&&current->world.pregame&&
    integer(current->world.pregame->archive,"game_turn")==old_turn&&current_prefix&&current_prefix->version==1&&current_prefix->scalars==prefix_scalars&&
    current->maintenance.elapsed_turns==elapsed&&!current->game_clock.pending&&current->game_clock.sent_auto_moves==sent_auto&&current_score&&current_score==score&&current_score->value==score_before,
    "setGameTurn source or any clock storage changed during pre-store log prefix");
  };
  unchanged();h.game_turn_net_manager=manager;h.logged_game_turn=next_turn;
  // Concrete enabled write+flush or actual selected disabled branch only.
  // This private actual function cannot receive a DTO, caller bool or callback.
  h.game_turn_net_log_delivery=manager->submit_game_turn_message(next_turn);
  // Preserve the earned OS history even if a later source bookend fails.
  unchanged();h.game_tail_earned_revision=revision;++h.operation;
  h.frontier.store(BnwRetainedTurnFrontier::game_turn_paired_clock_store_owner);
 }catch(...){h.quarantine();throw;}
}
std::optional<BnwNetDebugOwnDelivery> BnwRetainedTurnCursor::game_turn_net_log_delivery()const {
 require_current();return impl_->history->game_turn_net_log_delivery;
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_paired_with_actual_stock(
 const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_paired_clock_store_owner&&
  h.game_turn_net_log_attempted&&h.game_turn_net_log_delivery&&h.game_turn_net_manager&&h.logged_game_turn&&
  !h.game_turn_clock_attempted&&!h.game_turn_clock_return&&h.operation<UINT64_MAX,
  "setGameTurn paired/native ScoreDirty stores require actual returned NET prefix");
 h.game_turn_clock_attempted=true; // Before callbacks/effects; never replay an uncertain prefix.
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  const auto canonical=source.retain_action_factory_owner();
  need(canonical.get()==&source,"setGameTurn actual source factory differs");
  const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;
  need(allocation.get()==&state&&h.generation==i->session->state_generation()&&state.world.pregame,
   "setGameTurn paired store actual allocation/Pregame absent");
  const auto old_turn=state.setup.game_turn;const auto revision=state.ui_source_revision;
  need(old_turn>=0&&old_turn<INT32_MAX&&h.turn==old_turn&&*h.logged_game_turn==old_turn+1&&revision<UINT64_MAX&&
   integer(state.world.pregame->archive,"game_turn")==old_turn&&!state.game_clock.pending,
   "setGameTurn actual old/native logged new turn differs");
  const auto* prefix=std::get_if<saves::Expansion2PrefixObservation>(&state.world.game_state.prefix);
  need(prefix&&prefix->version==1,"setGameTurn actual BNW prefix unavailable");
  const auto prefix_before=prefix->scalars;const auto elapsed=state.maintenance.elapsed_turns;const auto sent_auto=state.game_clock.sent_auto_moves;
  const auto score_value=[&]{
   saves::NamedSaveFlag* value=nullptr;
   for(auto& field:state.world.game_state.flags)if(field.name&&std::string_view(field.name)=="score_dirty"){
    need(!value,"setGameTurn native ScoreDirty duplicated");value=&field;
   }
   need(value,"setGameTurn actual native ScoreDirty absent");return value;
  };
  const auto score_before=score_value()->value;
  const auto unchanged=[&]{
   i->check_engine();h.game_turn_net_manager->require_same_host(*i->engine_utility->host);
   lease.require_operation(serial);i->check_engine();
   const auto current=i->session->read_state_lease();const auto* now=std::get_if<saves::Expansion2PrefixObservation>(&state.world.game_state.prefix);
   need(current.get()==&state&&current==allocation&&i->session->state_generation()==h.generation&&
    state.ui_source_revision==revision&&state.setup.game_turn==old_turn&&h.turn==old_turn&&state.world.pregame&&
    integer(state.world.pregame->archive,"game_turn")==old_turn&&now&&now->version==1&&now->scalars==prefix_before&&
    state.maintenance.elapsed_turns==elapsed&&state.game_clock.sent_auto_moves==sent_auto&&!state.game_clock.pending&&
    score_value()->value==score_before,"setGameTurn source/clock changed before paired stores");
  };
  unchanged();History::GameTurnClockStoreReturn receipt;
  static_assert(std::is_nothrow_move_constructible_v<History::GameTurnClockStoreReturn>);
  receipt.issuer=canonical;receipt.world=allocation;receipt.old_turn=old_turn;receipt.new_turn=old_turn+1;
  receipt.generation=h.generation;receipt.before=revision;receipt.after=revision+1;receipt.serial=serial;
  unchanged();
  // Reacquire physical native destinations after ALL provider bookends. The
  // SDK CvPreGame::setGameTurn assigns s_gameTurn, then CvGame::setScoreDirty
  // assigns true even if it was already true. Setup is Civis's paired mirror.
  auto raw=state.world.pregame->archive.integers.find("game_turn");auto* score=score_value();
  need(raw!=state.world.pregame->archive.integers.end()&&raw->second==old_turn,"setGameTurn actual Pregame destination retired");
  raw->second=receipt.new_turn;state.setup.game_turn=receipt.new_turn;score->value=true;
  state.ui_source_revision=receipt.after;h.turn=receipt.new_turn;receipt.score_store_returned=true;
  h.game_turn_clock_return=std::move(receipt);h.game_tail_earned_revision=state.ui_source_revision;
  // Private SAME-source expectation adoption is pure. Calling a normal
  // source/lease provider before this would incorrectly retire genuine stores.
  source.adopt_actual_game_turn_clock_return(*this,lease);
  h.game_turn_clock_return->source_expectation_adopted=true;
  i->timer->adopt_actual_game_turn_clock_return(*this); // Genuine SAME clock identity before currentness.
  ++h.operation;
  h.frontier.store(BnwRetainedTurnFrontier::game_turn_ui_dirty_owner);
  i->check_engine();lease.require_operation(serial);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_clock_store_return(
 const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,
 std::uint64_t generation,std::int64_t old_turn,std::uint64_t before,std::uint64_t serial,
 const BnwSelectedStockLuaSourceOwner& source)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.game_turn_clock_attempted&&h.game_turn_clock_return&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_paired_clock_store_owner&&i->engine_utility&&
  i->engine_utility->host.get()==&host,"setGameTurn actual private return owner absent");
 const auto& r=*h.game_turn_clock_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 const auto allocation=h.state.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
  world&&allocation&&world.get()==&state&&allocation.get()==&state&&
  !world.owner_before(allocation)&&!allocation.owner_before(world)&&r.generation==generation&&generation==h.generation&&
  r.old_turn==old_turn&&old_turn>=0&&old_turn<INT32_MAX&&r.new_turn==old_turn+1&&r.before==before&&before<UINT64_MAX&&
  r.after==before+1&&r.serial==serial&&serial!=0&&r.score_store_returned&&!r.source_expectation_adopted&&
  h.turn==r.new_turn&&state.setup.game_turn==r.new_turn&&state.world.pregame&&
  integer(state.world.pregame->archive,"game_turn")==r.new_turn&&state.ui_source_revision==r.after&&
  h.game_tail_earned_revision&&*h.game_tail_earned_revision==r.after&&h.logged_game_turn&&*h.logged_game_turn==r.new_turn&&
  h.game_turn_net_log_delivery&&!state.game_clock.pending,
  "setGameTurn private paired/native return seal differs");
 const saves::NamedSaveFlag* score=nullptr;
 for(const auto& value:state.world.game_state.flags)if(value.name&&std::string_view(value.name)=="score_dirty"){
  need(!score,"setGameTurn returned native ScoreDirty duplicated");score=&value;
 }
 need(score&&score->value,"setGameTurn actual native ScoreDirty store did not return");
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_ui_dirty_with_actual_stock(
 const ui::LiveUiHost& ui,const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_ui_dirty_owner&&
  h.game_turn_clock_return&&h.game_turn_clock_return->source_expectation_adopted&&
  !h.game_turn_ui_dirty_attempted&&!h.game_turn_ui_dirty_return&&h.operation<UINT64_MAX,
  "setGameTurn UI13/11 requires genuine adopted native clock return");
 h.game_turn_ui_dirty_attempted=true; // Reserve before all source/UI providers.
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  const auto canonical=source.retain_action_factory_owner();const auto allocation=i->session->read_state_lease();
  const auto control=require_actual_ui(ui);const auto generation=h.generation;const auto turn=h.turn;
  const auto revision=allocation->ui_source_revision;const auto& clock=*h.game_turn_clock_return;
  need(canonical.get()==&source&&clock.issuer.lock()==canonical&&clock.world.lock()==allocation&&
   clock.new_turn==turn&&clock.after==revision&&clock.generation==generation,
   "setGameTurn UI SAME actual clock issuer/world differs");
  const auto sent_auto=allocation->game_clock.sent_auto_moves;const auto elapsed=allocation->maintenance.elapsed_turns;
  const auto current=[&]{
   i->check_engine();lease.require_operation(serial);
   need(require_actual_ui(ui)==control&&i->session->read_state_lease()==allocation&&i->session->state_generation()==generation&&
    allocation->ui_source_revision==revision&&allocation->setup.game_turn==turn&&h.turn==turn&&
    !allocation->game_clock.pending&&allocation->game_clock.sent_auto_moves==sent_auto&&allocation->maintenance.elapsed_turns==elapsed&&
    h.game_tail_earned_revision&&*h.game_tail_earned_revision==revision,
    "setGameTurn UI actual source/world/next clock operands changed");
  };
  current();h.game_turn_ui_dirty_issuer=canonical;
  // The retained canonical receiver performs physical 13 then 11. Preserve
  // its actual returned pair before any further callbackful bookend.
  h.game_turn_ui_dirty_return=control->set_game_turn_dirty_pair();
  current();
  need(control->game_turn_dirty_pair_matches_without_callbacks(*h.game_turn_ui_dirty_return)&&
   h.game_turn_ui_dirty_issuer.lock()==canonical,"setGameTurn concrete UI13/11 return differs");
  ++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_sent_auto_moves_owner);
 }catch(...){h.quarantine();throw;}
}
std::uint64_t BnwRetainedTurnCursor::turn_timer_dirty_setter_count(const ui::LiveUiHost& ui)const{
 const auto control=require_actual_ui(ui);const auto result=control->turn_timer_dirty_setter_count();require_current();return result;
}
std::optional<bool> BnwRetainedTurnCursor::turn_timer_dirty(const ui::LiveUiHost& ui)const{
 const auto control=require_actual_ui(ui);const auto result=control->turn_timer_dirty();require_current();return result;
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_sent_auto_with_actual_stock(
 const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_sent_auto_moves_owner&&
  h.game_turn_ui_dirty_return&&!h.game_turn_sent_auto_attempted&&!h.game_turn_sent_auto_return&&h.operation<UINT64_MAX,
  "setGameTurn sentAutoMoves requires returned actual UI13/11 pair");
 h.game_turn_sent_auto_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  const auto canonical=source.retain_action_factory_owner();const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;
  const auto ui=i->cycle_ui.lock();need(bool(ui),"setGameTurn sentAutoMoves actual UI lifetime absent");const auto control=require_actual_ui(*ui);
  const auto before=state.ui_source_revision;const auto old_value=state.game_clock.sent_auto_moves;const auto elapsed=state.maintenance.elapsed_turns;
  need(canonical.get()==&source&&allocation.get()==&state&&before<UINT64_MAX&&h.game_turn_clock_return&&
   h.game_turn_clock_return->source_expectation_adopted&&h.game_turn_clock_return->new_turn==h.turn&&
   h.game_turn_clock_return->after==before,"setGameTurn sentAutoMoves actual clock identity differs");
  const auto current=[&]{
   i->check_engine();lease.require_operation(serial);need(require_actual_ui(*ui)==control&&i->session->read_state_lease()==allocation&&
    i->session->state_generation()==h.generation&&state.ui_source_revision==before&&state.setup.game_turn==h.turn&&
    state.world.pregame&&integer(state.world.pregame->archive,"game_turn")==h.turn&&
    !state.game_clock.pending&&state.game_clock.sent_auto_moves==old_value&&state.maintenance.elapsed_turns==elapsed,
    "setGameTurn sentAutoMoves actual operands changed before store");
   need(control->game_turn_dirty_pair_matches_without_callbacks(*h.game_turn_ui_dirty_return),"setGameTurn UI pair changed before sentAutoMoves");
  };
  current();History::GameTurnSentAutoReturn r;r.issuer=canonical;r.world=allocation;r.generation=h.generation;
  r.before=before;r.after=before+1;r.serial=serial;r.turn=h.turn;r.old_value=old_value;
  static_assert(std::is_nothrow_move_constructible_v<History::GameTurnSentAutoReturn>);
  current();
  // Genuine CvGame runtime scalar (constructor false, not serialized in
  // Civ5Save). Always assign, including equal false, after the UI13/11 pair.
  state.game_clock.sent_auto_moves=false;state.ui_source_revision=r.after;r.stored=true;
  h.game_tail_earned_revision=r.after;h.game_turn_sent_auto_return=std::move(r);
  source.adopt_actual_game_turn_sent_auto_return(*this,lease); // Pure, before callbackful currentness.
  h.game_turn_sent_auto_return->source_adopted=true;++h.operation;
  h.frontier.store(BnwRetainedTurnFrontier::game_turn_gameplay_changed_owner);
  i->check_engine();lease.require_operation(serial);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_sent_auto_store_return(
 const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,std::uint64_t generation,
 std::uint64_t before,std::uint64_t serial,const BnwSelectedStockLuaSourceOwner& source)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.game_turn_sent_auto_attempted&&h.game_turn_sent_auto_return&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_sent_auto_moves_owner&&i->engine_utility&&
  i->engine_utility->host.get()==&host,"setGameTurn sentAutoMoves actual private return absent");
 const auto& r=*h.game_turn_sent_auto_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();const auto allocation=h.state.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
  world&&allocation&&world.get()==&state&&allocation.get()==&state&&!world.owner_before(allocation)&&!allocation.owner_before(world)&&
  generation==h.generation&&r.generation==generation&&r.before==before&&before<UINT64_MAX&&r.after==before+1&&
  r.serial==serial&&serial!=0&&r.turn==h.turn&&state.setup.game_turn==r.turn&&r.stored&&!r.source_adopted&&
  !state.game_clock.sent_auto_moves&&!state.game_clock.pending&&state.ui_source_revision==r.after&&h.game_tail_earned_revision&&
  *h.game_tail_earned_revision==r.after,"setGameTurn genuine sentAutoMoves store/issuer/world seal differs");
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_gameplay_with_actual_stock(const ui::LiveUiHost& ui,
 const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_gameplay_changed_owner&&
  h.game_turn_sent_auto_return&&h.game_turn_sent_auto_return->source_adopted&&!h.game_turn_gameplay_attempted&&
  !h.game_turn_gameplay_return&&h.operation<UINT64_MAX,"GameplayTurnChanged requires actual returned sentAutoMoves prefix");
 h.game_turn_gameplay_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);const auto canonical=source.retain_action_factory_owner();
  (void)require_actual_ui(ui);const std::weak_ptr<ui::BnwEngineUiTurnOwner> engine=ui.engine_ui_turn_owner_;const auto allocation=i->session->read_state_lease();
  need(!engine.expired()&&canonical.get()==&source&&allocation->ui_source_revision<UINT64_MAX,"GameplayTurnChanged actual source/UI/world absent");
  History::GameTurnGameplayReturn r;r.issuer=canonical;r.engine=engine;r.world=allocation;r.generation=h.generation;
  r.before=allocation->ui_source_revision;r.after=r.before+1;r.serial=serial;r.turn=h.turn;h.game_turn_gameplay_return=std::move(r);
  ui::BnwEngineUiTurnOwner::publish_game_turn_changed_from_owned_weak(engine,*this,*i->session->state_,*canonical,lease);
  need(h.game_turn_gameplay_return->stored&&h.game_turn_gameplay_return->source_adopted,
   "GameplayTurnChanged actual queued return/source adoption absent");
  i->check_engine();lease.require_operation(serial);++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_timer_reset_owner);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_gameplay_source(const ui::LiveUiHost& ui,const ui::BnwEngineUiTurnOwner& engine,
 const LiveSessionState& state,const BnwStockLuaAttemptLease& lease)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();i->check_engine();const auto control=require_actual_ui(ui);
 need(h.game_turn_gameplay_attempted&&h.game_turn_gameplay_return&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_gameplay_changed_owner,
  "GameplayTurnChanged actual pending dispatch absent");const auto& r=*h.game_turn_gameplay_return;const auto world=r.world.lock();
 need(world&&world.get()==&state&&world==i->session->read_state_lease()&&r.engine.lock().get()==&engine&&ui.engine_ui_turn_owner_.get()==&engine&&
  r.generation==h.generation&&r.before==state.ui_source_revision&&r.turn==h.turn&&r.turn==state.setup.game_turn&&r.serial==lease.operation_serial()&&
  !r.stored&&!r.source_adopted&&!state.game_clock.sent_auto_moves&&!state.game_clock.pending&&h.game_tail_earned_revision&&
  *h.game_tail_earned_revision==r.before,"GameplayTurnChanged pending SAME runtime source/world differs");
 need(control->game_turn_dirty_pair_matches_without_callbacks(*h.game_turn_ui_dirty_return),"GameplayTurnChanged actual UI13/11 pair changed");
 lease.require_operation(r.serial);
}
void BnwRetainedTurnCursor::accept_actual_game_turn_gameplay_return(const ui::BnwEngineUiTurnOwner& engine,
 const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();need(bool(h.game_turn_gameplay_return),"GameplayTurnChanged concrete pending receipt absent");
 auto& r=*h.game_turn_gameplay_return;const auto world=r.world.lock();const auto issuer=r.issuer.lock();
 need(!h.retired.load()&&!h.poisoned.load()&&world&&issuer&&issuer.get()==&source&&r.engine.lock().get()==&engine&&
  !r.stored&&!r.source_adopted&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_gameplay_changed_owner,
  "GameplayTurnChanged real queue store issuer differs");
 engine.require_game_turn_changed_return(*this,*world,r.before,r.serial,r.generation);
 r.stored=true;h.game_tail_earned_revision=r.after;source.adopt_actual_game_turn_gameplay_return(*this,lease);r.source_adopted=true;
}
void BnwRetainedTurnCursor::require_game_turn_gameplay_store_return(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,
 std::uint64_t generation,std::uint64_t before,std::uint64_t serial,const BnwSelectedStockLuaSourceOwner& source)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();need(h.game_turn_gameplay_return&&!h.retired.load()&&!h.poisoned.load()&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_gameplay_changed_owner&&i->engine_utility->host.get()==&host,
  "GameplayTurnChanged actual private source return absent");const auto& r=*h.game_turn_gameplay_return;
 const auto world=r.world.lock();const auto issuer=r.issuer.lock();const auto engine=r.engine.lock();const auto allocation=h.state.lock();
 need(world&&world.get()==&state&&allocation&&world==allocation&&issuer&&issuer.get()==&source&&
  !r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&engine&&r.generation==generation&&generation==h.generation&&
  r.before==before&&before<UINT64_MAX&&r.after==before+1&&r.serial==serial&&serial!=0&&r.stored&&!r.source_adopted&&
  state.ui_source_revision==r.after&&state.setup.game_turn==r.turn&&r.turn==h.turn&&!state.game_clock.sent_auto_moves&&
  h.game_tail_earned_revision&&*h.game_tail_earned_revision==r.after,"GameplayTurnChanged pure earned source/world seal differs");
 engine->require_game_turn_changed_return(*this,state,before,serial,generation);
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::require_actual_timer_clock_adoption(const BnwGameTurnTimerOwner& timer,
 const LiveSession& session,const LiveSessionState& state,std::uint64_t generation,std::int64_t old_turn)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&i->timer.get()==&timer&&i->session.get()==&session&&
  h.facade==&session&&h.game_turn_clock_attempted&&h.game_turn_clock_return&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_paired_clock_store_owner,
  "BNW timer clock adoption requires actual SAME retained source cursor");
 const auto& r=*h.game_turn_clock_return;const auto world=r.world.lock();const auto allocation=h.state.lock();
 need(world&&allocation&&world.get()==&state&&world==allocation&&session.state_.get()==&state&&
  !world.owner_before(allocation)&&!allocation.owner_before(world)&&generation==h.generation&&r.generation==generation&&
  r.old_turn==old_turn&&old_turn>=0&&old_turn<INT32_MAX&&r.new_turn==old_turn+1&&r.new_turn==h.turn&&
  state.setup.game_turn==r.new_turn&&state.world.pregame&&integer(state.world.pregame->archive,"game_turn")==r.new_turn&&
  r.before<UINT64_MAX&&r.after==r.before+1&&state.ui_source_revision==r.after&&r.serial!=0&&
  r.score_store_returned&&r.source_expectation_adopted&&h.game_tail_earned_revision&&*h.game_tail_earned_revision==r.after&&
  r.issuer.lock()&&!state.game_clock.pending,"BNW timer genuine stored paired-clock return differs");
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::require_actual_end_turn_timer_reset_source(const BnwGameTurnTimerOwner& timer,
 const LiveSession& session,const LiveSessionState& state,std::uint64_t generation,std::int64_t turn)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_timer_reset_owner&&
  h.game_turn_end_timer_attempted&&h.game_turn_gameplay_attempted&&h.game_turn_gameplay_return&&
  h.game_turn_gameplay_return->stored&&h.game_turn_gameplay_return->source_adopted&&i->timer.get()==&timer&&i->session.get()==&session&&h.facade==&session&&
  generation==h.generation&&turn==h.turn&&state.setup.game_turn==turn,"EndTurnTimer requires actual SAME returned gameplay/clock prefix");
 const auto allocation=h.state.lock();const auto& r=*h.game_turn_gameplay_return;const auto world=r.world.lock();const auto source=r.issuer.lock();
 need(allocation&&world&&allocation.get()==&state&&world.get()==&state&&r.generation==generation&&r.turn==turn&&r.after==state.ui_source_revision&&source&&
  !r.issuer.owner_before(source)&&!source.owner_before(r.issuer)&&session.state_generation()==generation&&session.read_state_lease().get()==&state,
  "EndTurnTimer actual source/world/generation changed");
}
void BnwRetainedTurnCursor::execute_game_turn_end_timer_with_actual_stock(const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_timer_reset_owner&&
  !h.game_turn_end_timer_attempted&&!h.game_turn_end_timer_return&&h.operation<UINT64_MAX,
  "EndTurnTimer actual prefix unreached/repeated");h.game_turn_end_timer_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);const auto canonical=source.retain_action_factory_owner();
  need(h.game_turn_gameplay_return&&h.game_turn_gameplay_return->issuer.lock()==canonical,"EndTurnTimer actual gameplay source issuer differs");
  const auto allocation=i->session->read_state_lease();const auto revision=allocation->ui_source_revision;
  const auto use=i->timer->current_use();const auto before=i->timer->read(use);
  const auto actual=i->timer->reset_actual_end_turn_timer(*this);
  h.game_turn_end_timer_return=actual; // Actual OS receipt retained before post providers.
  i->timer->require_actual_end_turn_timer_return(*this,actual);
  const auto after=i->timer->read(use);
  need(before.current_start==after.current_start&&before.game_turn_start==after.game_turn_start&&before.frequency==after.frequency&&
   before.operation==after.operation&&before.reset==after.reset&&before.pause_delta==after.pause_delta&&
   before.current_stopped_elapsed==after.current_stopped_elapsed&&before.game_turn_stopped_elapsed==after.game_turn_stopped_elapsed&&
   after.current_elapsed>=before.current_elapsed&&after.game_turn_elapsed>=before.game_turn_elapsed&&allocation->ui_source_revision==revision,
   "EndTurnTimer reset changed unrelated timer/world state");
  i->check_engine();lease.require_operation(serial);++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_elapsed_increment_owner);
 }catch(...){h.quarantine();throw;}
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_elapsed_with_actual_stock(const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_elapsed_increment_owner&&
  h.game_turn_end_timer_attempted&&h.game_turn_end_timer_return&&!h.game_turn_elapsed_attempted&&!h.game_turn_elapsed_return&&h.operation<UINT64_MAX,
  "incrementElapsedGameTurns requires actual complete setGameTurn timer prefix");
 h.game_turn_elapsed_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);const auto canonical=source.retain_action_factory_owner();
  const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;
  const auto* initial=std::get_if<saves::Expansion2PrefixObservation>(&state.world.game_state.prefix);
  need(canonical.get()==&source&&allocation.get()==&state&&state.world.game_state.layout==saves::GameStateLayout::expansion2_version1&&initial&&initial->version==1&&
   state.maintenance.profile==content::TechnologyProfile::bnw&&initial->scalars[1]==state.maintenance.elapsed_turns&&initial->scalars[1]>=0&&initial->scalars[1]<INT32_MAX&&
   state.ui_source_revision<UINT64_MAX&&state.setup.game_turn==h.turn&&state.world.pregame&&!state.game_clock.pending,
   "incrementElapsedGameTurns actual raw/owned native operands invalid");
  const auto prior=initial->scalars;const auto old=prior[1];const auto revision=state.ui_source_revision;const auto sent=state.game_clock.sent_auto_moves;
  History::GameTurnElapsedReturn r;r.issuer=canonical;r.world=allocation;r.generation=h.generation;r.before=revision;r.after=revision+1;r.serial=serial;r.turn=h.turn;r.old_elapsed=old;r.new_elapsed=old+1;
  // All callbackful checks are completed before reacquiring raw destinations.
  // This native scalar increment has no logger, dirty setter or Lua hook.
  i->check_engine();lease.require_operation(serial);i->check_engine();
  auto* raw=std::get_if<saves::Expansion2PrefixObservation>(&state.world.game_state.prefix);
  need(i->session->read_state_lease()==allocation&&i->session->state_generation()==h.generation&&state.ui_source_revision==revision&&raw&&raw->version==1&&raw->scalars==prior&&
   state.maintenance.profile==content::TechnologyProfile::bnw&&state.maintenance.elapsed_turns==old&&state.setup.game_turn==r.turn&&h.turn==r.turn&&state.world.pregame&&
   integer(state.world.pregame->archive,"game_turn")==r.turn&&state.game_clock.sent_auto_moves==sent&&!state.game_clock.pending,
   "incrementElapsedGameTurns native destination changed before stores");
  static_assert(std::is_nothrow_move_constructible_v<History::GameTurnElapsedReturn>);
  raw->scalars[1]=r.new_elapsed;state.maintenance.elapsed_turns=r.new_elapsed;state.ui_source_revision=r.after;r.stored=true;
  h.game_turn_elapsed_return=std::move(r);h.game_tail_earned_revision=state.ui_source_revision;
  source.adopt_actual_game_turn_elapsed_return(*this,lease);h.game_turn_elapsed_return->source_adopted=true;
  ++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_dynamic_turns_owner);
  i->check_engine();lease.require_operation(serial);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_elapsed_store_return(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,
 std::uint64_t generation,std::uint64_t before,std::uint64_t serial,const BnwSelectedStockLuaSourceOwner& source)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_elapsed_increment_owner&&
  h.game_turn_elapsed_attempted&&h.game_turn_elapsed_return&&h.game_turn_end_timer_return&&i->engine_utility&&i->engine_utility->host.get()==&host,
  "incrementElapsedGameTurns actual private source return absent");
 const auto& r=*h.game_turn_elapsed_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();const auto allocation=h.state.lock();
 const auto* raw=std::get_if<saves::Expansion2PrefixObservation>(&state.world.game_state.prefix);
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&allocation&&world.get()==&state&&allocation.get()==&state&&
  !world.owner_before(allocation)&&!allocation.owner_before(world)&&r.generation==generation&&generation==h.generation&&r.before==before&&before<UINT64_MAX&&r.after==before+1&&r.serial==serial&&serial!=0&&
  r.stored&&!r.source_adopted&&r.old_elapsed>=0&&r.old_elapsed<INT32_MAX&&r.new_elapsed==r.old_elapsed+1&&r.turn==h.turn&&state.setup.game_turn==r.turn&&
  raw&&raw->version==1&&raw->scalars[1]==r.new_elapsed&&state.maintenance.profile==content::TechnologyProfile::bnw&&state.maintenance.elapsed_turns==r.new_elapsed&&
  state.ui_source_revision==r.after&&h.game_tail_earned_revision==std::optional{r.after}&&state.world.pregame&&integer(state.world.pregame->archive,"game_turn")==r.turn&&!state.game_clock.pending,
  "incrementElapsedGameTurns actual raw/owned paired return seal differs");
}
}

namespace civ5::runtime {
namespace {
bool dynamic_options_same(const std::vector<saves::PregameOption>& a,const std::vector<saves::PregameOption>& b)noexcept{
 if(a.size()!=b.size())return false;
 for(std::size_t n=0;n<a.size();++n)if(a[n].name!=b[n].name||a[n].value!=b[n].value)return false;
 return true;
}
std::optional<std::int64_t> dynamic_named_option(const std::vector<saves::PregameOption>& options,std::string_view requested){
 // CvPreGame string overload: first saved-name-length strncmp match, not exact key search.
 for(const auto& option:options){
  const auto saved=std::string_view(option.name).substr(0,option.name.find('\0'));
  need(saved.size()<64,"Dynamic turn native option name exceeds fixed64");
  if(saved.size()<=requested.size()&&requested.substr(0,saved.size())==saved){
   need(option.value>=INT32_MIN&&option.value<=INT32_MAX,"Dynamic turn native option outside i32");return option.value;
  }
 }
 return std::nullopt;
}
}
void BnwRetainedTurnCursor::execute_game_turn_activation_prefix_with_actual_stock(const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_dynamic_turns_owner&&
  h.game_turn_elapsed_return&&h.game_turn_elapsed_return->stored&&h.game_turn_elapsed_return->source_adopted&&
  !h.game_turn_dynamic_attempted&&!h.game_turn_dynamic_return&&h.operation<UINT64_MAX,
  "Dynamic mode requires genuine elapsed return and once current cursor");h.game_turn_dynamic_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);const auto canonical=source.retain_action_factory_owner();
  const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;
  need(canonical.get()==&source&&h.game_turn_elapsed_return->issuer.lock()==canonical&&allocation.get()==&state&&
   state.world.pregame&&state.setup.game_type==0&&integer(state.world.pregame->archive,"game_type")==0&&
   state.setup.game_turn==h.turn&&h.game_tail_earned_revision==std::optional{state.ui_source_revision}&&
   h.game_turn_elapsed_return->after==state.ui_source_revision&&!state.game_clock.pending,
   "Dynamic mode SAME ordinary world/elapsed source changed");
  const auto raw_options=state.world.pregame->game_options;const auto owned_options=state.setup.game_options;
  need(dynamic_options_same(raw_options,owned_options),"Dynamic mode actual paired Pregame options differ");
  const auto option=[&](std::string_view name){
   need(dynamic_options_same(state.world.pregame->game_options,raw_options)&&dynamic_options_same(state.setup.game_options,owned_options),"Dynamic turn options changed before query");
   const auto raw=dynamic_named_option(raw_options,name),owned=dynamic_named_option(owned_options,name);
   std::int64_t value=0;if(raw)value=*raw;else{
    std::optional<std::int64_t> selected;const auto report=i->engine_utility->host->database().query_rows(
     {"SELECT [Default] FROM GameOptions WHERE Type = ? LIMIT 1",{std::string(name)}},[&](const auto& row){selected=row.integer_at(0);},1);
    need(report.all_succeeded()&&report.sqlite_code==0,"Dynamic turn actual selected default query failed");value=selected.value_or(0);
   }
   need(value>=INT32_MIN&&value<=INT32_MAX&&(!owned||*owned==value)&&(!raw||owned),"Dynamic turn actual raw/owned option differs");
   return value;
  };
  History::GameTurnDynamicModeReturn r;r.issuer=canonical;r.world=allocation;r.generation=h.generation;
  r.before=state.ui_source_revision;r.after=r.before;r.serial=serial;r.turn=h.turn;r.raw_options=raw_options;r.owned_options=owned_options;
  r.operation=h.operation;r.map_rng=state.random.map.state();r.game_rng=state.random.gameplay.state();
  r.dynamic=option("GAMEOPTION_DYNAMIC_TURNS");auto next=BnwRetainedTurnFrontier::game_turn_simultaneous_shuffle_owner;
  if(r.dynamic==1){
   // Source loop/setter owns exact current native operands; no notification result is invented.
   // Stage the source-order loop without effects. A first OPEN getter cannot
   // depend on its unknown result: it is reached before that result branches.
   // Do not query later teams after an earlier mandatory notification, or
   // OPEN members hidden by an earlier TAKEN/assigned OBSERVER short circuit.
   auto preliminary=detail::DynamicModeComponentPlan::prepare(state,0);
   if(preliminary.pitboss_read()){
    r.pitboss=option("GAMEOPTION_PITBOSS");preliminary.require_current(state,false);
    if(r.pitboss!=0)r.plan.emplace(detail::DynamicModeComponentPlan::prepare(state,r.pitboss));
    else r.plan.emplace(std::move(preliminary));
   }else r.plan.emplace(std::move(preliminary));
   if(r.plan->blocked())next=BnwRetainedTurnFrontier::game_turn_dynamic_notification_owner;
  }else{
   need(option("GAMEOPTION_DYNAMIC_TURNS")==r.dynamic,"Dynamic turn repeated getter changed");
   r.simultaneous=option("GAMEOPTION_SIMULTANEOUS_TURNS");
   if(r.simultaneous!=1){
    // Ordinary single-player isSimultaneousTeamTurns is false before option access.
    need(option("GAMEOPTION_SIMULTANEOUS_TURNS")==r.simultaneous,"Sequential turn repeated getter changed");
    r.read_operands.emplace(detail::capture_dynamic_mode_operands(state));next=BnwRetainedTurnFrontier::game_turn_victory_owner;
    for(std::size_t n=0;n<64;++n)if(r.read_operands->players[n].alive){r.sequential=static_cast<std::int32_t>(n);next=BnwRetainedTurnFrontier::game_turn_player_activation_owner;break;}
   }
  }
  r.next=next;const auto stores=r.plan?r.plan->stores():0;
  need(stores==0||r.before<UINT64_MAX,"Dynamic mode native store world revision exhausted");r.after=r.before+(stores?1u:0u);
  i->check_engine();lease.require_operation(serial);i->check_engine();
  // No provider or allocator remains between this exact operand seal and primitive stores.
  need(i->session->read_state_lease()==allocation&&i->session->state_generation()==r.generation&&state.ui_source_revision==r.before&&
   state.world.pregame&&integer(state.world.pregame->archive,"game_type")==0&&state.setup.game_type==0&&
   integer(state.world.pregame->archive,"game_turn")==r.turn&&state.setup.game_turn==r.turn&&!state.game_clock.pending&&
   dynamic_options_same(state.world.pregame->game_options,r.raw_options)&&dynamic_options_same(state.setup.game_options,r.owned_options)&&
   state.random.map.state()==r.map_rng&&state.random.gameplay.state()==r.game_rng,
   "Dynamic mode final canonical clock/options changed");
  if(r.plan)r.plan->require_current(state,false);
  if(r.read_operands)need(detail::capture_dynamic_mode_operands(state)==*r.read_operands,"Sequential actual alive/slot source changed");
  static_assert(std::is_nothrow_move_constructible_v<History::GameTurnDynamicModeReturn>);
  if(r.plan)need(r.plan->apply_prepared_without_callbacks(state),"Dynamic mode prepared component repeated");
  state.ui_source_revision=r.after;r.stored=true;
  h.game_turn_dynamic_return=std::move(r);h.game_tail_earned_revision=state.ui_source_revision;
  // Receipt/history exists before pure source adoption or any late callback.
  source.adopt_actual_game_turn_activation_return(*this,lease);h.game_turn_dynamic_return->source_adopted=true;
  ++h.operation;h.frontier.store(next);i->check_engine();lease.require_operation(serial);
  require_game_turn_activation_store_return(*i->engine_utility->host,state,h.generation,h.game_turn_dynamic_return->before,serial,source,true);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_activation_store_return(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,
 std::uint64_t generation,std::uint64_t before,std::uint64_t serial,const BnwSelectedStockLuaSourceOwner& source,bool after_adoption)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 try{
 need(!h.retired.load()&&!h.poisoned.load()&&
  h.game_turn_dynamic_attempted&&h.game_turn_dynamic_return&&h.game_turn_elapsed_return&&h.game_turn_elapsed_return->stored&&
  h.game_turn_elapsed_return->source_adopted&&i->engine_utility&&i->engine_utility->host.get()==&host,
  "Dynamic mode actual elapsed/controlled return absent");
 const auto& r=*h.game_turn_dynamic_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();const auto allocation=h.state.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&allocation&&
  world.get()==&state&&allocation.get()==&state&&!world.owner_before(allocation)&&!allocation.owner_before(world)&&
  r.generation==generation&&generation==h.generation&&r.before==before&&r.serial==serial&&serial!=0&&r.stored&&r.source_adopted==after_adoption&&
  h.frontier.load()==(after_adoption?r.next:BnwRetainedTurnFrontier::game_turn_dynamic_turns_owner)&&h.operation==r.operation+(after_adoption?1u:0u)&&
  r.turn==h.turn&&state.setup.game_turn==r.turn&&state.ui_source_revision==r.after&&h.game_tail_earned_revision==std::optional{r.after}&&
  h.game_turn_elapsed_return->after==r.before&&state.world.pregame&&!state.game_clock.pending&&
  integer(state.world.pregame->archive,"game_turn")==r.turn&&integer(state.world.pregame->archive,"game_type")==0&&state.setup.game_type==0&&
  dynamic_options_same(state.world.pregame->game_options,r.raw_options)&&dynamic_options_same(state.setup.game_options,r.owned_options)&&
  state.random.map.state()==r.map_rng&&state.random.gameplay.state()==r.game_rng,
  "Dynamic mode sealed SAME source/world/elapsed lineage differs");
 need(r.after==r.before+((r.plan&&r.plan->stores())?1u:0u),"Dynamic mode actual revision/store relation differs");
 if(r.plan)r.plan->require_current(state,true);
 if(r.read_operands)need(detail::capture_dynamic_mode_operands(state)==*r.read_operands,"Sequential actual return operands changed");
 }catch(...){h.quarantine();throw;} // Keep actual stored/adopted history on late source failure.
}
BnwDynamicTurnModeProgress BnwRetainedTurnCursor::dynamic_turn_mode_progress()const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();BnwDynamicTurnModeProgress out;out.attempted=h.game_turn_dynamic_attempted;
 if(h.game_turn_dynamic_return){const auto& r=*h.game_turn_dynamic_return;out.native_prefix_returned=r.stored;out.source_adopted=r.source_adopted;
  out.dynamic_option=r.dynamic;out.simultaneous_option=r.simultaneous;out.sequential_player=r.sequential;
  if(r.plan){out.teams_visited=r.plan->teams_visited();out.returned_setters=r.plan->returned_setters();out.native_scalar_stores=r.plan->stores();out.notification_frontier=r.plan->blocked();}}
 return out; // Pure retained history, not currentness/continuation permission.
}
} // namespace civ5::runtime


namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_shuffle_with_actual_stock(const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_simultaneous_shuffle_owner&&
  h.game_turn_dynamic_return&&h.game_turn_dynamic_return->stored&&h.game_turn_dynamic_return->source_adopted&&
  !h.game_turn_shuffle_attempted&&!h.game_turn_shuffle_return&&h.operation<UINT64_MAX,
  "Turn shuffle requires genuine elapsed/dynamic source return and once cursor");h.game_turn_shuffle_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);const auto canonical=source.retain_action_factory_owner();
  const auto allocation=i->session->read_state_lease();auto& state=*i->session->state_;const auto& parent=*h.game_turn_dynamic_return;
  require_game_turn_activation_store_return(*i->engine_utility->host,state,h.generation,parent.before,parent.serial,source,true);
  need(canonical.get()==&source&&parent.issuer.lock()==canonical&&allocation.get()==&state&&
   (parent.dynamic==1||parent.simultaneous==1)&&parent.next==BnwRetainedTurnFrontier::game_turn_simultaneous_shuffle_owner&&
   parent.after==state.ui_source_revision&&state.ui_source_revision<UINT64_MAX&&!state.game_clock.pending,
   "Turn shuffle actual ordinary dynamic/simultaneous predecessor differs");
  History::GameTurnShuffleReturn r;r.issuer=canonical;r.world=allocation;r.generation=h.generation;r.before=state.ui_source_revision;
  r.after=r.before+1;r.serial=serial;r.operation=h.operation;r.turn=h.turn;r.raw_options=parent.raw_options;r.owned_options=parent.owned_options;
  r.component.emplace(detail::TurnShuffleComponent::prepare(state));
  i->check_engine();lease.require_operation(serial);i->check_engine();
  need(i->session->read_state_lease()==allocation&&i->session->state_generation()==r.generation&&state.ui_source_revision==r.before&&
   h.game_tail_earned_revision==std::optional{r.before}&&state.setup.game_turn==r.turn&&state.world.pregame&&
   integer(state.world.pregame->archive,"game_type")==0&&state.setup.game_type==0&&integer(state.world.pregame->archive,"game_turn")==r.turn&&
   dynamic_options_same(state.world.pregame->game_options,r.raw_options)&&dynamic_options_same(state.setup.game_options,r.owned_options)&&!state.game_clock.pending,
   "Turn shuffle canonical clock/options changed before RNG");r.component->require_current(state,false);
  static_assert(std::is_nothrow_move_constructible_v<History::GameTurnShuffleReturn>);
  // Own attempted receipt exists before the first real RNG mutation. Each draw
  // records its actual state immediately, including any partial failure history.
  h.game_turn_shuffle_return.emplace(std::move(r));auto& earned=*h.game_turn_shuffle_return;
  earned.component->draw_without_callbacks(state);state.ui_source_revision=earned.after;earned.rng_stored=true;
  h.game_tail_earned_revision=state.ui_source_revision;
  source.adopt_actual_game_turn_shuffle_return(*this,lease);earned.source_adopted=true;
  const auto option=[&](std::string_view name){
   need(dynamic_options_same(state.world.pregame->game_options,earned.raw_options)&&dynamic_options_same(state.setup.game_options,earned.owned_options),"Turn shuffle options changed before reached query");
   const auto raw=dynamic_named_option(earned.raw_options,name),owned=dynamic_named_option(earned.owned_options,name);std::int64_t value=0;
   if(raw)value=*raw;else{std::optional<std::int64_t> selected;const auto report=i->engine_utility->host->database().query_rows(
    {"SELECT [Default] FROM GameOptions WHERE Type = ? LIMIT 1",{std::string(name)}},[&](const auto& row){selected=row.integer_at(0);},1);
    need(report.all_succeeded()&&report.sqlite_code==0,"Turn shuffle selected option default query failed");value=selected.value_or(0);}
   need(value>=INT32_MIN&&value<=INT32_MAX&&(!owned||*owned==value)&&(!raw||owned),"Turn shuffle raw/owned native option differs");
   earned.component->require_current(state,true);return value;
  };
  while(earned.component->scanned()<64&&!earned.component->pending()){
   i->check_engine();lease.require_operation(serial);earned.component->require_current(state,true);
   // isAlive short-circuits isHuman. OPEN alone reaches actual PitBoss config.
   if(earned.component->next_requires_pitboss()&&!earned.pitboss_queried){earned.pitboss=option("GAMEOPTION_PITBOSS");earned.pitboss_queried=true;}
   (void)earned.component->scan_one_without_callbacks(state,earned.pitboss);
  }
  auto next=BnwRetainedTurnFrontier::game_turn_player_activation_owner;
  if(earned.component->pending())earned.player=earned.component->pending();
  else{
   need(earned.component->loop_returned(),"Turn shuffle original AI loop not returned");
   // Ordinary type0 isSimultaneousTeamTurns=false. Original second independent
   // branch still queries SIMULTANEOUS after the returned AI activation loop.
   earned.simultaneous=option("GAMEOPTION_SIMULTANEOUS_TURNS");next=BnwRetainedTurnFrontier::game_turn_victory_owner;
   if(earned.simultaneous!=1)for(const auto& player:earned.component->operands().players)
    if(player.alive&&(parent.dynamic!=1||player.mode==0)){earned.player=static_cast<std::int32_t>(player.id);next=BnwRetainedTurnFrontier::game_turn_player_activation_owner;break;}
  }
  i->check_engine();lease.require_operation(serial);earned.component->require_current(state,true);
  need(state.ui_source_revision==earned.after&&h.game_tail_earned_revision==std::optional{earned.after}&&
   state.world.pregame&&dynamic_options_same(state.world.pregame->game_options,earned.raw_options)&&dynamic_options_same(state.setup.game_options,earned.owned_options)&&
   state.setup.game_turn==earned.turn&&integer(state.world.pregame->archive,"game_turn")==earned.turn&&!state.game_clock.pending,
   "Turn shuffle final source/scan changed");
  earned.next=next;earned.scan_returned=true;++h.operation;h.frontier.store(next);
  i->check_engine();lease.require_operation(serial);
  require_game_turn_shuffle_store_return(*i->engine_utility->host,state,h.generation,earned.before,serial,source,true);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_shuffle_store_return(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,
 std::uint64_t generation,std::uint64_t before,std::uint64_t serial,const BnwSelectedStockLuaSourceOwner& source,bool after_adoption)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();try{
 need(!h.retired.load()&&!h.poisoned.load()&&h.game_turn_shuffle_attempted&&h.game_turn_shuffle_return&&h.game_turn_dynamic_return&&
  h.game_turn_dynamic_return->source_adopted&&i->engine_utility&&i->engine_utility->host.get()==&host,"Turn shuffle private canonical return absent");
 const auto& r=*h.game_turn_shuffle_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();const auto allocation=h.state.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&allocation&&world.get()==&state&&
  allocation.get()==&state&&!world.owner_before(allocation)&&!allocation.owner_before(world)&&generation==h.generation&&r.generation==generation&&r.before==before&&
  before<UINT64_MAX&&r.after==before+1&&r.serial==serial&&serial!=0&&r.rng_stored&&r.component&&r.component->draws()==64&&r.source_adopted==after_adoption&&
  r.turn==h.turn&&state.setup.game_turn==r.turn&&state.ui_source_revision==r.after&&h.game_tail_earned_revision==std::optional{r.after}&&
  h.game_turn_dynamic_return->after==r.before&&state.world.pregame&&integer(state.world.pregame->archive,"game_type")==0&&state.setup.game_type==0&&
  integer(state.world.pregame->archive,"game_turn")==r.turn&&!state.game_clock.pending&&
  dynamic_options_same(state.world.pregame->game_options,r.raw_options)&&dynamic_options_same(state.setup.game_options,r.owned_options),"Turn shuffle sealed source/world/RNG lineage differs");
 need(after_adoption?(r.scan_returned&&h.frontier.load()==r.next&&h.operation==r.operation+1):
  (!r.scan_returned&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_simultaneous_shuffle_owner&&h.operation==r.operation),"Turn shuffle actual control phase differs");
 r.component->require_current(state,true);
 }catch(...){h.quarantine();throw;}
}
BnwTurnShuffleProgress BnwRetainedTurnCursor::turn_shuffle_progress()const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();BnwTurnShuffleProgress out;out.attempted=h.game_turn_shuffle_attempted;
 if(h.game_turn_shuffle_return){const auto& r=*h.game_turn_shuffle_return;out.rng_stored=r.rng_stored;out.source_adopted=r.source_adopted;out.scan_returned=r.scan_returned;out.pending_player=r.player;
  if(r.component){out.draws=r.component->draws();out.scanned=r.component->scanned();out.equal_active_returns=r.component->equal_returns();out.ai_loop_returned=r.component->loop_returned();
   out.permutation=r.component->permutation();out.seed_before=r.component->before_rng().seed;out.seed_after=r.component->after_rng().seed;out.calls_before=r.component->before_rng().calls;out.calls_after=r.component->after_rng().calls;}}
 return out; // Creator-only historical observation; no continuation authority.
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_player_active_with_actual_stock(
 const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_owner&&
  h.game_turn_dynamic_return&&h.game_turn_dynamic_return->stored&&h.game_turn_dynamic_return->source_adopted&&
  !h.game_turn_player_active_attempted&&!h.game_turn_player_active_return&&h.operation<UINT64_MAX,
  "Changed Player turn-active needs genuine current predecessor and once cursor");
 h.game_turn_player_active_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  const auto canonical=source.retain_action_factory_owner();const auto allocation=i->session->read_state_lease();
  auto& state=*i->session->state_;const auto& dynamic=*h.game_turn_dynamic_return;
  History::GameTurnPlayerActiveReturn r;r.issuer=canonical;r.world=allocation;r.generation=h.generation;
  r.before=state.ui_source_revision;r.after=r.before+1;r.serial=serial;r.operation=h.operation;r.turn=h.turn;
  if(h.game_turn_shuffle_return){const auto& shuffle=*h.game_turn_shuffle_return;
   require_game_turn_shuffle_store_return(*i->engine_utility->host,state,h.generation,shuffle.before,shuffle.serial,source,true);
   need(shuffle.player&&shuffle.next==BnwRetainedTurnFrontier::game_turn_player_activation_owner,
    "Changed Player setter lacks actual shuffled/sequential selection");
   r.player=*shuffle.player;r.shuffled=true;r.raw_options=shuffle.raw_options;r.owned_options=shuffle.owned_options;
  }else{
   require_game_turn_activation_store_return(*i->engine_utility->host,state,h.generation,dynamic.before,dynamic.serial,source,true);
   need(dynamic.sequential&&dynamic.next==BnwRetainedTurnFrontier::game_turn_player_activation_owner,
    "Changed Player setter lacks actual ascending alive selection");
   r.player=*dynamic.sequential;r.raw_options=dynamic.raw_options;r.owned_options=dynamic.owned_options;
  }
  need(canonical.get()==&source&&allocation.get()==&state&&r.player>=0&&r.player<64&&r.before<UINT64_MAX&&
   h.game_tail_earned_revision==std::optional{r.before}&&!state.game_clock.pending,
   "Changed Player setter canonical source/revision differs");
  r.operands=detail::capture_dynamic_mode_operands(state);r.map_rng=state.random.map.state();r.game_rng=state.random.gameplay.state();
  for(std::size_t n=0;n<64;++n){const auto& raw=state.world.all_players->players[n];const auto& f=detail::dynamic_core(raw,"m_bTurnActive");
   need(f.encoding==saves::PlayerCoreEncoding::boolean&&(f.values[0]==0||f.values[0]==1),"Actual Player turn-active native bool absent");
   r.active[n]=f.values[0];r.active_fields[n]=&f;r.active_values[n]=f.values.data();
   r.active_indexes[n]=static_cast<std::size_t>(&f-raw.core.fields.data());
  }
  const auto actor=static_cast<std::size_t>(r.player);
  need(r.operands.players[actor].alive&&r.operands.players[actor].id==r.player&&r.active[actor]==0,
   "Changed Player setter requires actual living inactive player; equal path not admitted here");
  // Finish all callbacks/providers before sealing the actual scalar target.
  i->check_engine();lease.require_operation(serial);i->check_engine();
  need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_owner&&
   h.operation==r.operation&&i->session->read_state_lease()==allocation&&i->session->state_generation()==r.generation&&state.ui_source_revision==r.before&&
   state.setup.game_turn==r.turn&&state.world.pregame&&integer(state.world.pregame->archive,"game_type")==0&&
   integer(state.world.pregame->archive,"game_turn")==r.turn&&
   dynamic_options_same(state.world.pregame->game_options,r.raw_options)&&dynamic_options_same(state.setup.game_options,r.owned_options)&&
   h.game_tail_earned_revision==std::optional{r.before}&&!state.game_clock.pending,
   "Changed Player setter source/clock/options changed before native store");r.require_operands(state,false);
  static_assert(std::is_nothrow_move_constructible_v<History::GameTurnPlayerActiveReturn>);
  h.game_turn_player_active_return.emplace(std::move(r));auto& earned=*h.game_turn_player_active_return;
  // CvPlayer.cpp15789. No allocation/provider/publication in this interval.
  state.world.all_players->players[actor].core.fields[earned.active_indexes[actor]].values[0]=1;
  state.ui_source_revision=earned.after;earned.stored=true;h.game_tail_earned_revision=earned.after;
  source.adopt_actual_game_turn_player_active_return(*this,lease);earned.source_adopted=true;
  ++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_player_activation_dirty_owner);
  i->check_engine();lease.require_operation(serial);
  require_game_turn_player_active_store_return(*i->engine_utility->host,state,h.generation,earned.before,serial,source,true);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_player_active_store_return(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,std::uint64_t generation,std::uint64_t before,std::uint64_t serial,
 const BnwSelectedStockLuaSourceOwner& source,bool after_adoption)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();try{
  need(!h.retired.load()&&!h.poisoned.load()&&h.game_turn_player_active_attempted&&h.game_turn_player_active_return&&
   i->engine_utility&&i->engine_utility->host.get()==&host,"Actual changed Player return absent/retired");
  const auto& r=*h.game_turn_player_active_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();const auto held=h.state.lock();
  need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
   world&&held&&world.get()==&state&&held.get()==&state&&!world.owner_before(held)&&!held.owner_before(world)&&
   generation==h.generation&&r.generation==generation&&r.before==before&&before<UINT64_MAX&&r.after==before+1&&
   r.serial==serial&&serial!=0&&r.stored&&r.source_adopted==after_adoption&&r.turn==h.turn&&state.setup.game_turn==r.turn&&
   state.ui_source_revision==r.after&&h.game_tail_earned_revision==std::optional{r.after}&&state.world.pregame&&
   integer(state.world.pregame->archive,"game_type")==0&&integer(state.world.pregame->archive,"game_turn")==r.turn&&!state.game_clock.pending&&
   dynamic_options_same(state.world.pregame->game_options,r.raw_options)&&dynamic_options_same(state.setup.game_options,r.owned_options),
   "Actual changed Player return source/world/clock identity differs");
  need(after_adoption?(h.operation==r.operation+1&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_dirty_owner):
   (h.operation==r.operation&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_owner),
   "Actual changed Player return phase differs");
  // Historical predecessor only: its pre-store TurnActive snapshot is no
  // longer current. The exact new receipt proves just one selected mutation.
  need(h.game_turn_dynamic_return&&h.game_turn_dynamic_return->stored&&h.game_turn_dynamic_return->source_adopted&&
   (r.shuffled?(h.game_turn_shuffle_return&&h.game_turn_shuffle_return->rng_stored&&h.game_turn_shuffle_return->source_adopted&&
    h.game_turn_shuffle_return->scan_returned&&h.game_turn_shuffle_return->player==std::optional{r.player}&&h.game_turn_shuffle_return->after==r.before):
    (!h.game_turn_shuffle_return&&h.game_turn_dynamic_return->sequential==std::optional{r.player}&&h.game_turn_dynamic_return->after==r.before)),
   "Changed Player return has no authentic selected predecessor");
  r.require_operands(state,true);
 }catch(...){h.quarantine();throw;}
}
BnwPlayerTurnActiveProgress BnwRetainedTurnCursor::player_turn_active_progress()const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();BnwPlayerTurnActiveProgress out;out.attempted=h.game_turn_player_active_attempted;
 if(h.game_turn_player_active_return){const auto& r=*h.game_turn_player_active_return;out.player=r.player;out.stored=r.stored;
  out.source_adopted=r.source_adopted;out.before_revision=r.before;out.after_revision=r.after;}
 out.publication_attempted=h.player_dirty_attempted;
 if(h.player_dirty_issuer){out.queued=h.player_dirty_issuer->queued_without_callbacks();out.publication_returned=h.player_dirty_issuer->publisher_returned_without_callbacks();
  out.delivery_attempted=h.player_dirty_issuer->delivery_attempted_without_callbacks();out.delivered=h.player_dirty_issuer->delivered_without_callbacks();}
 return out; // Creator-only history, never UI/render/full-turn acceptance.
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_player_dirty_with_actual_stock(const BnwSelectedStockLuaSourceOwner& source,
 const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_dirty_owner&&
  h.game_turn_player_active_return&&h.game_turn_player_active_return->stored&&h.game_turn_player_active_return->source_adopted&&
  !h.player_dirty_attempted&&!h.player_dirty_issuer&&h.operation<UINT64_MAX,"Player dirty needs actual changed Player return and once phase");
 h.player_dirty_attempted=true;h.player_dirty_operation=h.operation;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);h.player_dirty_serial=serial;
  const auto cursor=h.cursor.lock();need(cursor&&cursor.get()==this,"Player dirty factory cursor retired");
  h.player_dirty_issuer=source.issue_player_turn_dirty_owned(cursor,lease); // retained before real Engine/providers
  const auto actual_ui=i->cycle_ui.lock();need(bool(actual_ui),"Player dirty actual UI expired");const auto control=require_actual_ui(*actual_ui);
  const auto receiver=actual_ui->engine_ui_turn_owner_;need(bool(receiver),"Player dirty canonical Engine absent");
  const auto allocation=i->session->read_state_lease();ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*receiver,*actual_ui,*allocation);
  receiver->require_current();i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*actual_ui)==control&&actual_ui->engine_ui_turn_owner_.get()==receiver.get(),"Player dirty Engine/physical UI substituted");
  receiver->consume_player_turn_dirty(h.player_dirty_issuer,lease);
  i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*actual_ui)==control&&actual_ui->engine_ui_turn_owner_.get()==receiver.get(),"Player dirty receiver changed after publisher");
  h.player_dirty_issuer->require_before_delivery_without_callbacks(*receiver,lease);
  need(h.player_dirty_issuer->queued_without_callbacks()&&h.player_dirty_issuer->publisher_returned_without_callbacks(),"Player dirty real publisher missing");
  ++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_player_activation_dirty_delivery_owner);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_player_turn_dirty_operation_without_callbacks(const BnwPlayerTurnActiveDirtyIssuer& issuer,
 const BnwSelectedStockLuaSourceOwner&,std::uint64_t serial,bool publisher)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.player_dirty_attempted&&h.player_dirty_issuer.get()==&issuer&&h.player_dirty_serial&&
  (publisher?serial==h.player_dirty_serial:serial>=h.player_dirty_serial)&&
  ((h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_dirty_owner&&h.operation==h.player_dirty_operation)||
   (h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_dirty_delivery_owner&&h.operation==h.player_dirty_operation+1)||
   (h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_owner&&h.operation==h.player_dirty_operation+2&&h.player_dirty_resumed)),
  "Player dirty actual Cursor/operation/once lineage changed");
}
void BnwRetainedTurnCursor::require_player_turn_dirty_operands_without_callbacks(const BnwPlayerTurnActiveDirtyIssuer& issuer,
 const ui::BnwEngineUiTurnOwner& engine,const BnwSelectedStockLuaSourceOwner& source)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();const auto ui=i->cycle_ui.lock();const auto allocation=h.state.lock();
 need(!h.retired.load()&&!h.poisoned.load()&&h.player_dirty_issuer.get()==&issuer&&h.game_turn_player_active_return&&ui&&allocation&&
  ui->engine_ui_turn_owner_.get()==&engine&&allocation.get()==i->session->state_.get()&&i->session->state_generation()==h.generation,
  "Player dirty canonical Engine/UI/world changed");
 const auto& r=*h.game_turn_player_active_return;const auto s=r.issuer.lock();const auto world=r.world.lock();const auto& state=*allocation;
 need(s&&s.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&world.get()==&state&&
  r.stored&&r.source_adopted&&r.generation==h.generation&&r.after==state.ui_source_revision&&h.game_tail_earned_revision==std::optional{r.after}&&
  state.setup.game_turn==r.turn&&state.world.pregame&&integer(state.world.pregame->archive,"game_turn")==r.turn&&
  integer(state.world.pregame->archive,"game_type")==0&&!state.game_clock.pending&&
  dynamic_options_same(state.world.pregame->game_options,r.raw_options)&&dynamic_options_same(state.setup.game_options,r.owned_options),
  "Player dirty actual scalar/clock/source return changed");r.require_operands(state,true);
}
void BnwRetainedTurnCursor::require_player_turn_dirty_return_without_callbacks(const BnwSelectedStockLuaSourceOwner& source,
 const BnwStockLuaAttemptLease& lease,bool resumed)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();need(h.player_dirty_issuer!=nullptr,"Player dirty issued history absent");
 source.require_player_turn_dirty_source_without_callbacks(*h.player_dirty_issuer,*this,lease,false);
 const auto ui=i->cycle_ui.lock();need(ui&&ui->engine_ui_turn_owner_,"Player dirty actual Engine absent after return");
 require_player_turn_dirty_operands_without_callbacks(*h.player_dirty_issuer,*ui->engine_ui_turn_owner_,source);
 need(h.player_dirty_issuer->queued_without_callbacks()&&h.player_dirty_issuer->publisher_returned_without_callbacks()&&
  (resumed?(h.player_dirty_resumed&&h.player_dirty_issuer->delivered_without_callbacks()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_owner):
   (!h.player_dirty_resumed&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_dirty_delivery_owner)),
  "Player dirty actual normal return/delivery phase differs");
}
void BnwRetainedTurnCursor::resume_game_turn_player_after_dirty_delivery_with_actual_stock(const BnwSelectedStockLuaSourceOwner& source,
 const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.player_dirty_issuer&&!h.player_dirty_resumed&&h.operation<UINT64_MAX&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_dirty_delivery_owner&&h.player_dirty_issuer->delivered_without_callbacks(),
  "Player continuation needs actual SAME69 protected VM delivery; queue append alone insufficient");
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  need(serial>h.player_dirty_serial,"Player delivery continuation cannot reuse publisher scope");
  require_player_turn_dirty_return_without_callbacks(source,lease,false);
  h.player_dirty_resumed=true;++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_player_end_turn_owner);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::fail_player_turn_dirty_without_callbacks()noexcept{const auto h=impl_->history;if(h->thread!=std::this_thread::get_id())std::terminate();h->quarantine();}
void BnwRetainedTurnCursor::require_player_turn_dirty_outer_return_without_callbacks(const BnwSelectedStockLuaSourceOwner& source,bool resumed)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();need(h.player_dirty_issuer!=nullptr,"Player dirty outer receipt absent");
 source.require_player_turn_dirty_idle_source_without_callbacks(*h.player_dirty_issuer,*this);
 const auto ui=i->cycle_ui.lock();need(ui&&ui->engine_ui_turn_owner_,"Player dirty outer actual Engine expired");
 require_player_turn_dirty_operands_without_callbacks(*h.player_dirty_issuer,*ui->engine_ui_turn_owner_,source);
 need(h.player_dirty_issuer->queued_without_callbacks()&&h.player_dirty_issuer->publisher_returned_without_callbacks()&&
  (resumed?(h.player_dirty_resumed&&h.player_dirty_issuer->delivered_without_callbacks()&&h.operation==h.player_dirty_operation+2&&
    h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_owner):
   (!h.player_dirty_resumed&&h.operation==h.player_dirty_operation+1&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_activation_dirty_delivery_owner)),
  "Player dirty outer real publication/delivery phase changed");
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::execute_game_turn_player_end_turn_with_actual_stock(const BnwSelectedStockLuaSourceOwner& source,
 const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_owner&&
  h.player_dirty_resumed&&h.player_dirty_issuer&&h.player_dirty_issuer->delivered_without_callbacks()&&
  !h.game_turn_player_end_turn_attempted&&!h.game_turn_player_end_turn_return&&h.operation<UINT64_MAX,
  "setEndTurn requires actual protected Event69 delivery/current Cursor/once issuer");
 h.game_turn_player_end_turn_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  // The actual delivery return stays a strict predecessor BEFORE any new stores.
  require_player_turn_dirty_return_without_callbacks(source,lease,true);
  const auto allocation=i->session->state_;auto& state=*allocation;const auto self=source.action_self_.lock();
  History::GameTurnPlayerEndTurnReturn r;r.issuer=self;r.world=allocation;r.generation=h.generation;
  r.before=state.ui_source_revision;r.after=r.before;r.turn=state.setup.game_turn;r.serial=serial;r.operation=h.operation;
  r.player=h.game_turn_player_active_return->player;r.active_player=integer(state.world.pregame->archive,"active_player");
  need(self&&self.get()==&source&&r.active_player>=-1&&r.active_player<64&&r.player>=0&&r.player<64,
   "setEndTurn canonical Source/Game/actor absent");
  const auto actor=static_cast<std::size_t>(r.player);
  const auto& player=h.game_turn_player_active_return->operands.players[actor];
  // Original isHuman only reaches PITBOSS for a valid OPEN player. Retain the
  // actual selected Rules default if the native saved-name getter misses.
  if(player.id!=-1&&player.status==0){
   r.pitboss_queried=true;r.pitboss_saved=dynamic_named_option(state.world.pregame->game_options,"GAMEOPTION_PITBOSS");
   if(r.pitboss_saved)r.pitboss=*r.pitboss_saved;
   else{std::optional<std::int64_t> selected;const auto report=i->engine_utility->host->database().query_rows(
    {"SELECT [Default] FROM GameOptions WHERE Type = ? LIMIT 1",{std::string("GAMEOPTION_PITBOSS")}},
    [&](const auto& row){selected=row.integer_at(0);},1);
    need(report.all_succeeded()&&report.sqlite_code==0,"setEndTurn actual selected PITBOSS default query failed");r.pitboss=selected.value_or(0);}
   need(r.pitboss>=INT32_MIN&&r.pitboss<=INT32_MAX,"setEndTurn PITBOSS native i32 invalid");
  }
  r.human=detail::dynamic_player_human(player,r.pitboss);
  for(std::size_t n=0;n<64;++n){const auto& raw=state.world.all_players->players[n];
   const auto& e=detail::dynamic_core(raw,"m_bEndTurn");const auto& a=detail::dynamic_core(raw,"m_bAutoMoves");
   need(e.encoding==saves::PlayerCoreEncoding::boolean&&a.encoding==saves::PlayerCoreEncoding::boolean&&
    (e.values[0]==0||e.values[0]==1)&&(a.values[0]==0||a.values[0]==1),"setEndTurn actual archive boolean invalid");
   r.end[n]=e.values[0];r.automatic[n]=a.values[0];r.end_fields[n]=&e;r.end_values[n]=e.values.data();
   r.auto_fields[n]=&a;r.auto_values[n]=a.values.data();r.end_indexes[n]=static_cast<std::size_t>(&e-raw.core.fields.data());
   r.auto_indexes[n]=static_cast<std::size_t>(&a-raw.core.fields.data());
  }
  r.old_end=r.end[actor]!=0;r.old_auto=r.automatic[actor]!=0;
  // CvPlayer16041 calls isSimultaneousTurns first. Its actual current options
  // and player-mode inputs are already sealed; false argument short-circuits
  // before the network DLL query. No network acknowledgement is fabricated.
  r.busy_required=!r.human||(!r.old_end&&r.player!=r.active_player); // 16053/16060 BEFORE EndTurn equality
  if(r.busy_required){r.next=BnwRetainedTurnFrontier::game_turn_player_end_turn_busy_owner;}
  else{
   if(r.old_end){need(r.before<UINT64_MAX,"setEndTurn world revision exhausted");r.after=r.before+1;}
   // 16100/16106 -> setAutoMoves16008. Equal false has no store and does NOT
   // reset ProcessedAutoMoves. Changed true needs its genuine unsaved owner.
   r.processed_auto_required=r.old_auto;
   r.next=r.old_auto?BnwRetainedTurnFrontier::game_turn_player_end_turn_processed_auto_owner:
    BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner;
  }
  const auto actual_ui=i->cycle_ui.lock();need(actual_ui&&actual_ui->engine_ui_turn_owner_,"setEndTurn actual retained UI/Engine expired");
  const auto control=require_actual_ui(*actual_ui);const auto engine=actual_ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*actual_ui,*allocation);
  engine->require_current();i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*actual_ui)==control&&actual_ui->engine_ui_turn_owner_==engine,"setEndTurn actual UI/Engine substituted");
  i->check_engine();lease.require_operation(serial);
  // Final physical/source/control/64-row/RNG seals AFTER every provider and
  // directly BEFORE the native write. No provider/callback in the store interval.
  require_player_turn_dirty_return_without_callbacks(source,lease,true);
  source.require_player_end_turn_source_without_callbacks(*this,&lease,false,r.before);
  need(h.operation==r.operation&&i->session->state_==allocation&&state.ui_source_revision==r.before&&
   integer(state.world.pregame->archive,"active_player")==r.active_player&&
   (!r.pitboss_queried||dynamic_named_option(state.world.pregame->game_options,"GAMEOPTION_PITBOSS")==r.pitboss_saved),
   "setEndTurn Game/actor/source inputs changed before store");r.require_flags(state,false);
  static_assert(std::is_nothrow_move_constructible_v<History::GameTurnPlayerEndTurnReturn>);
  h.game_turn_player_end_turn_return.emplace(std::move(r));auto& earned=*h.game_turn_player_end_turn_return;
  if(!earned.busy_required&&earned.old_end){
   // Canonical owned world.core IS the live raw archive mirror. There is no
   // detached EndTurn projection. Original SavedSession bytes stay immutable.
   state.world.all_players->players[actor].core.fields[earned.end_indexes[actor]].values[0]=0; // CvPlayer16086
   state.ui_source_revision=earned.after;earned.stored=true;h.game_tail_earned_revision=earned.after;
  }
  earned.setter_returned=!earned.busy_required&&!earned.processed_auto_required;
  source.adopt_actual_game_turn_player_end_turn_return(*this,lease);earned.source_adopted=true;
  ++h.operation;h.frontier.store(earned.next);
  i->check_engine();lease.require_operation(serial);
  require_game_turn_player_end_turn_return_without_callbacks(*i->engine_utility->host,state,h.generation,earned.before,serial,source,true);
 }catch(...){h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_player_end_turn_return_without_callbacks(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,std::uint64_t generation,std::uint64_t before,std::uint64_t serial,
 const BnwSelectedStockLuaSourceOwner& source,bool adopted,bool busy_continuation,bool city_continuation)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.game_turn_player_end_turn_attempted&&h.game_turn_player_end_turn_return&&
  i->engine_utility&&i->engine_utility->host.get()==&host,"setEndTurn actual issued return retired/absent");
 const auto& r=*h.game_turn_player_end_turn_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();const auto held=h.state.lock();
 const auto ui=i->cycle_ui.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
  world&&held&&world.get()==&state&&held.get()==&state&&!world.owner_before(held)&&!held.owner_before(world)&&
  i->session->state_.get()==&state&&i->session->state_generation()==generation&&generation==h.generation&&r.generation==generation&&
  r.before==before&&r.after==before+(r.stored?1U:0U)&&r.serial==serial&&serial&&r.source_adopted==adopted&&
  ui&&ui->engine_ui_turn_owner_&&h.game_turn_player_active_return&&h.player_dirty_resumed&&h.player_dirty_issuer&&h.player_dirty_issuer->queued_without_callbacks()&&
  h.player_dirty_issuer->publisher_returned_without_callbacks()&&h.player_dirty_issuer->delivered_without_callbacks()&&
  r.turn==h.turn&&state.setup.game_turn==r.turn&&state.world.pregame&&integer(state.world.pregame->archive,"game_turn")==r.turn&&
  integer(state.world.pregame->archive,"game_type")==0&&integer(state.world.pregame->archive,"active_player")==r.active_player&&
  (!r.pitboss_queried||dynamic_named_option(state.world.pregame->game_options,"GAMEOPTION_PITBOSS")==r.pitboss_saved)&&
  state.ui_source_revision==r.after&&h.game_tail_earned_revision==std::optional{r.after}&&!state.game_clock.pending,
  "setEndTurn actual source/clock/delivered predecessor changed");
   need(city_continuation?(adopted&&r.busy_required&&!r.stored&&h.player_busy_attempted&&h.player_busy_returned&&h.player_busy_return&&
    h.player_busy_city_attempted&&h.player_busy_city_returned&&h.player_busy_city_return&&h.operation==r.operation+3&&h.frontier.load()==h.player_busy_city_next):
    busy_continuation?(adopted&&r.busy_required&&!r.stored&&h.player_busy_attempted&&h.player_busy_returned&&h.player_busy_return&&
   h.operation==r.operation+2&&h.frontier.load()==h.player_busy_next):adopted?(h.operation==r.operation+1&&h.frontier.load()==r.next):
  (h.operation==r.operation&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_owner),"setEndTurn once return phase changed");
 const auto& parent=*h.game_turn_player_active_return;
 need(parent.stored&&parent.source_adopted&&parent.player==r.player&&parent.after==r.before&&
  dynamic_options_same(state.world.pregame->game_options,parent.raw_options)&&dynamic_options_same(state.setup.game_options,parent.owned_options),
  "setEndTurn history lacks actual changed activation/unchanged options");
 parent.require_operands(state,true);r.require_flags(state,true);
 need(r.pitboss_queried==(parent.operands.players[static_cast<std::size_t>(r.player)].id!=-1&&parent.operands.players[static_cast<std::size_t>(r.player)].status==0)&&
  r.human==detail::dynamic_player_human(parent.operands.players[static_cast<std::size_t>(r.player)],r.pitboss)&&
  r.busy_required==(!r.human||(!r.old_end&&r.player!=r.active_player))&&r.stored==(!r.busy_required&&r.old_end)&&
  r.processed_auto_required==(!r.busy_required&&r.old_auto)&&r.setter_returned==(!r.busy_required&&!r.processed_auto_required)&&
  r.next==(r.busy_required?BnwRetainedTurnFrontier::game_turn_player_end_turn_busy_owner:
   r.processed_auto_required?BnwRetainedTurnFrontier::game_turn_player_end_turn_processed_auto_owner:BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner),
  "setEndTurn original guards/store/mandatory tail relation changed");
}
BnwPlayerEndTurnProgress BnwRetainedTurnCursor::player_end_turn_progress()const{
 const auto h=impl_->history;h->thread_check();BnwPlayerEndTurnProgress out;out.attempted=h->game_turn_player_end_turn_attempted;
 if(h->game_turn_player_end_turn_return){const auto& r=*h->game_turn_player_end_turn_return;
  out.player=r.player;out.before_end_turn=r.old_end;out.before_auto_moves=r.old_auto;out.end_flag_stored=r.stored;
  out.source_adopted=r.source_adopted;out.setter_returned=r.setter_returned;out.busy_guard_required=r.busy_required;
  out.processed_auto_owner_required=r.processed_auto_required;out.before_revision=r.before;out.after_revision=r.after;
 }
 return out;
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::require_player_busy_entry_without_callbacks(const BnwNativeUnitAllocatorCore& core,
 const BnwSelectedStockLuaSourceOwner& source)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&!h.player_busy_attempted&&!h.player_busy_return&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_busy_owner&&h.game_turn_player_end_turn_return&&
  h.game_turn_player_end_turn_return->busy_required,"Busy source entry requires genuine current busy predecessor");
 const auto& parent=*h.game_turn_player_end_turn_return;const auto& state=*i->session->state_;
 source.require_player_end_turn_source_without_callbacks(*this,nullptr,true,parent.after);
 require_game_turn_player_end_turn_return_without_callbacks(*i->engine_utility->host,state,h.generation,parent.before,parent.serial,source,true);
 core.require_live_gold_roster_source_without_callbacks(*i->engine_utility->host,state,parent.player);
}
std::int32_t BnwRetainedTurnCursor::player_busy_actor_without_callbacks(const BnwSelectedStockLuaSourceOwner& source,
 const BnwStockLuaAttemptLease& lease)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(h.game_turn_player_end_turn_return&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_busy_owner&&
  h.game_turn_player_end_turn_return->busy_required,"Busy query lacks original setEndTurn busy predecessor");
 const auto& parent=*h.game_turn_player_end_turn_return;const auto& state=*i->session->state_;
 source.require_player_end_turn_source_without_callbacks(*this,&lease,false,parent.after);
 require_game_turn_player_end_turn_return_without_callbacks(*i->engine_utility->host,state,h.generation,parent.before,parent.serial,source,true);
 return parent.player;
}
void BnwRetainedTurnCursor::execute_game_turn_player_busy_with_actual_stock(const BnwNativeUnitAllocatorCore& core,
 const BnwSelectedStockLuaSourceOwner& source,const BnwStockLuaAttemptLease& lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&!h.player_busy_attempted&&!h.player_busy_return&&h.operation<UINT64_MAX,
  "Busy query Cursor retired/reentered/already issued");
 const auto actor=player_busy_actor_without_callbacks(source,lease);const auto allocation=i->session->state_;
 core.require_live_gold_roster_source_without_callbacks(*i->engine_utility->host,*allocation,actor);
 h.player_busy_attempted=true;h.player_busy_operation=h.operation;h.player_busy_serial=lease.operation_serial();
 try{
  i->check_engine();lease.require_operation(h.player_busy_serial);
  (void)player_busy_actor_without_callbacks(source,lease);
  source.issue_player_busy_units_return_owned(*this,core,lease,h.player_busy_return);
  // The sink is installed by Core before any provider/native predicate read.
  // No numeric value returned by a public observer can mint this history.
  i->check_engine();lease.require_operation(h.player_busy_serial);
  (void)player_busy_actor_without_callbacks(source,lease);
  source.seal_player_busy_units_return_without_callbacks(core,*h.player_busy_return,&lease,false);
  h.player_busy_next=!h.player_busy_return->completed_without_callbacks()?BnwRetainedTurnFrontier::game_turn_player_busy_runtime_owner:
   h.player_busy_return->busy_without_callbacks()?BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner:
   BnwRetainedTurnFrontier::game_turn_player_busy_city_owner;
  // True hasBusyUnit short-circuits hasBusyCity and the remote-human OR.
  // This earns the original setEndTurn early void return, not a flag store.
  h.player_busy_setter_returned=h.player_busy_return->completed_without_callbacks()&&h.player_busy_return->busy_without_callbacks();
  h.player_busy_returned=true;++h.operation;h.frontier.store(h.player_busy_next);
  i->check_engine();lease.require_operation(h.player_busy_serial);
  source.require_player_busy_source_without_callbacks(*this,core,&lease,false);
 }catch(...){if(h.player_busy_return)h.player_busy_return->quarantine_without_callbacks();h.quarantine();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_player_busy_return_without_callbacks(const BnwNativeUnitAllocatorCore& core,
 const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,const BnwSelectedStockLuaSourceOwner& source,
 const void* control,std::uint64_t generation,std::uint64_t serial,bool city_continuation)const{
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(h.player_busy_attempted&&h.player_busy_returned&&h.player_busy_return&&h.player_busy_serial==serial&&
  h.game_turn_player_end_turn_return&&h.player_busy_operation==h.game_turn_player_end_turn_return->operation+1&&
   (city_continuation?(h.player_busy_city_attempted&&h.player_busy_city_returned&&h.player_busy_city_return&&
    h.operation==h.player_busy_operation+2&&h.frontier.load()==h.player_busy_city_next):
    (h.operation==h.player_busy_operation+1&&h.frontier.load()==h.player_busy_next)),
  "Busy native return/current once Cursor lineage absent");
 const auto& parent=*h.game_turn_player_end_turn_return;
 require_game_turn_player_end_turn_return_without_callbacks(host,state,generation,parent.before,parent.serial,source,true,true,city_continuation);
 h.player_busy_return->require_current_without_callbacks(core,host,state,source,control,generation,serial);
 need(h.player_busy_setter_returned==(h.player_busy_return->completed_without_callbacks()&&h.player_busy_return->busy_without_callbacks())&&
  h.player_busy_next==(!h.player_busy_return->completed_without_callbacks()?BnwRetainedTurnFrontier::game_turn_player_busy_runtime_owner:
   h.player_busy_return->busy_without_callbacks()?BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner:
   BnwRetainedTurnFrontier::game_turn_player_busy_city_owner),"Busy native short-circuit/mandatory tail changed");
}
void BnwRetainedTurnCursor::fail_player_busy_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->player_busy_return)h->player_busy_return->quarantine_without_callbacks();h->quarantine();
}
BnwPlayerBusyUnitsProgress BnwRetainedTurnCursor::player_busy_units_progress()const{
 const auto h=impl_->history;h->thread_check();BnwPlayerBusyUnitsProgress out;out.attempted=h->player_busy_attempted;
 out.issued=bool(h->player_busy_return);out.native_prefix_returned=h->player_busy_returned;out.setter_early_returned=h->player_busy_setter_returned;
 if(h->player_busy_return){out.queried_units=h->player_busy_return->queried_without_callbacks();
  out.units_query_completed=h->player_busy_return->completed_without_callbacks();
  if(out.units_query_completed)out.has_busy_unit=h->player_busy_return->busy_without_callbacks();
  const auto id=h->player_busy_return->required_id_without_callbacks();if(id!=-1)out.required_unit=id;
 }
 return out;
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::require_player_busy_units_parent_for_city_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation)const{
 const auto h=impl_->history;h->thread_check();require_game_turn_player_busy_return_without_callbacks(core,host,state,source,control,generation,h->player_busy_serial);
}
void BnwRetainedTurnCursor::require_player_busy_city_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 if(h.player_busy_city_attempted&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_busy_city_owner){
  // Same genuine Source/realm only. Foreign references fail before poison.
  source.require_player_busy_city_reentry_without_callbacks(*this,core);
  if(h.player_busy_city_return)h.player_busy_city_return->quarantine_without_callbacks();h.quarantine();need(false,"City busy SAME actual source reentry quarantined");
 }
 need(!h.retired.load()&&!h.poisoned.load()&&!h.player_busy_city_attempted&&!h.player_busy_city_return&&
  h.player_busy_returned&&h.player_busy_return&&h.player_busy_return->completed_without_callbacks()&&!h.player_busy_return->busy_without_callbacks()&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_busy_city_owner,"City busy requires real unit-false predecessor");
 source.require_player_end_turn_source_without_callbacks(*this,nullptr,true,i->session->state_->ui_source_revision);
 source.require_player_busy_units_parent_for_city_without_callbacks(*this,core,nullptr,true);
}
std::int32_t BnwRetainedTurnCursor::player_busy_city_actor_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease)const{
 const auto i=impl_;auto&h=*i->history;h.thread_check();need(h.player_busy_return&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_busy_city_owner&&
  h.player_busy_return->completed_without_callbacks()&&!h.player_busy_return->busy_without_callbacks(),"City busy actual unit-false query absent");
 source.require_player_end_turn_source_without_callbacks(*this,&lease,false,i->session->state_->ui_source_revision);
 source.require_player_busy_units_parent_for_city_without_callbacks(*this,core,&lease,false);
 return h.game_turn_player_end_turn_return->player;
}
void BnwRetainedTurnCursor::execute_game_turn_player_busy_city_with_actual_stock(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 if(h.player_busy_city_attempted&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_busy_city_owner){
  // Same genuine Source/realm only. Foreign references fail before poison.
  source.require_player_busy_city_reentry_without_callbacks(*this,core);
  if(h.player_busy_city_return)h.player_busy_city_return->quarantine_without_callbacks();h.quarantine();need(false,"City busy SAME actual source reentry quarantined");
 }
 need(!h.retired.load()&&!h.poisoned.load()&&!h.player_busy_city_attempted&&!h.player_busy_city_return&&h.operation<UINT64_MAX,"City busy Cursor replay/reentry");
 (void)player_busy_city_actor_without_callbacks(core,source,lease);h.player_busy_city_attempted=true;h.player_busy_city_operation=h.operation;h.player_busy_city_serial=lease.operation_serial();
 try{i->check_engine();lease.require_operation(h.player_busy_city_serial);(void)player_busy_city_actor_without_callbacks(core,source,lease);
  source.issue_player_busy_cities_return_owned(*this,core,lease,h.player_busy_return,h.player_busy_city_return);
  i->check_engine();lease.require_operation(h.player_busy_city_serial);(void)player_busy_city_actor_without_callbacks(core,source,lease);
  source.seal_player_busy_cities_return_without_callbacks(core,*h.player_busy_city_return,&lease,false);
  const auto&r=*h.player_busy_city_return;const auto&parent=*h.game_turn_player_end_turn_return;
  h.player_busy_city_next=!r.completed_without_callbacks()?BnwRetainedTurnFrontier::game_turn_player_busy_city_runtime_owner:
   r.busy_without_callbacks()?BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner:
   parent.human?BnwRetainedTurnFrontier::game_turn_player_remote_turn_complete_owner:BnwRetainedTurnFrontier::game_turn_player_end_turn_after_busy_owner;
  h.player_busy_city_setter_returned=r.completed_without_callbacks()&&r.busy_without_callbacks();
  h.player_busy_city_returned=true;++h.operation;h.frontier.store(h.player_busy_city_next);
  i->check_engine();lease.require_operation(h.player_busy_city_serial);source.require_player_busy_city_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_player_busy_city_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_game_turn_player_busy_city_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial)const{
 const auto i=impl_;auto&h=*i->history;h.thread_check();need(h.player_busy_city_attempted&&h.player_busy_city_returned&&h.player_busy_city_return&&
  h.player_busy_city_serial==serial&&h.player_busy_city_operation==h.player_busy_operation+1&&h.operation==h.player_busy_city_operation+1&&h.frontier.load()==h.player_busy_city_next,
  "City busy actual issued native return lineage absent");
 require_game_turn_player_busy_return_without_callbacks(core,host,state,source,control,generation,h.player_busy_serial,true);
 h.player_busy_city_return->require_current_without_callbacks(core,host,state,source,control,generation,serial);
 const auto&r=*h.player_busy_city_return;need(h.player_busy_city_setter_returned==(r.completed_without_callbacks()&&r.busy_without_callbacks())&&
  h.player_busy_city_next==(!r.completed_without_callbacks()?BnwRetainedTurnFrontier::game_turn_player_busy_city_runtime_owner:
   r.busy_without_callbacks()?BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner:
   h.game_turn_player_end_turn_return->human?BnwRetainedTurnFrontier::game_turn_player_remote_turn_complete_owner:BnwRetainedTurnFrontier::game_turn_player_end_turn_after_busy_owner),
  "City busy original OR/remote-human/AI continuation changed");
}
void BnwRetainedTurnCursor::fail_player_busy_city_without_callbacks()noexcept{const auto h=impl_->history;if(h->player_busy_city_return)h->player_busy_city_return->quarantine_without_callbacks();h->quarantine();}
BnwPlayerBusyCitiesProgress BnwRetainedTurnCursor::player_busy_cities_progress()const{
 const auto h=impl_->history;h->thread_check();BnwPlayerBusyCitiesProgress out;out.attempted=h->player_busy_city_attempted;out.issued=bool(h->player_busy_city_return);
 out.native_prefix_returned=h->player_busy_city_returned;out.setter_early_returned=h->player_busy_city_setter_returned;
 if(h->player_busy_city_return){out.queried_cities=h->player_busy_city_return->queried_without_callbacks();out.cities_query_completed=h->player_busy_city_return->completed_without_callbacks();
  if(out.cities_query_completed)out.has_busy_city=h->player_busy_city_return->busy_without_callbacks();const auto id=h->player_busy_city_return->required_id_without_callbacks();if(id!=-1)out.required_city=id;}
 return out;
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::require_player_end_after_busy_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 if(h.game_turn_end_after_busy_attempted){
  source.require_player_end_after_busy_reentry_without_callbacks(*this,core);h.quarantine();need(false,"AI EndTurn SAME actual Source reentry quarantined");}
 need(!h.retired.load()&&!h.poisoned.load()&&!h.game_turn_end_after_busy_attempted&&!h.game_turn_end_after_busy_return&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_after_busy_owner&&h.player_busy_city_returned&&h.player_busy_city_return&&
  h.player_busy_city_return->completed_without_callbacks()&&!h.player_busy_city_return->busy_without_callbacks()&&h.game_turn_player_end_turn_return&&
  !h.game_turn_player_end_turn_return->human,"AI EndTurn requires actual units/cities-false native return");
 source.require_player_busy_city_source_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::require_player_end_after_busy_transition_without_callbacks(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,
 const BnwSelectedStockLuaSourceOwner&source,std::uint64_t query_revision,std::int32_t actor)const{
 require_player_end_after_busy_bound_without_callbacks(host,state,source,query_revision,actor,false);
}
void BnwRetainedTurnCursor::require_player_end_after_busy_bound_without_callbacks(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,
 const BnwSelectedStockLuaSourceOwner&source,std::uint64_t query_revision,std::int32_t actor,bool after_auto)const{
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 need(!h.retired.load()&&!h.poisoned.load()&&h.game_turn_end_after_busy_attempted&&h.game_turn_end_after_busy_return&&h.game_turn_player_end_turn_return&&
  h.game_turn_player_active_return&&i->engine_utility&&i->engine_utility->host.get()==&host,"AI EndTurn exact transition absent/retired");
 const auto&r=*h.game_turn_end_after_busy_return;const auto&old=*h.game_turn_player_end_turn_return;const auto&active=*h.game_turn_player_active_return;
 const auto self=r.issuer.lock();const auto world=r.world.lock();const auto held=h.state.lock();const auto ui=i->cycle_ui.lock();
 need(self&&self.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&held&&world.get()==&state&&
  held.get()==&state&&i->session->state_.get()==&state&&i->session->state_generation()==h.generation&&r.generation==h.generation&&
  r.player==actor&&actor==old.player&&r.before==query_revision&&r.before==old.after&&r.after==r.before+(r.stored?1U:0U)&&
  r.stored==r.old_end&&r.old_end==old.old_end&&r.old_auto==old.old_auto&&r.setter_returned==(!r.old_auto)&&!old.human&&old.busy_required&&!old.stored&&
  state.ui_source_revision==(after_auto?h.processed_auto_return->after:r.after)&&
  h.game_tail_earned_revision==std::optional{after_auto?h.processed_auto_return->after:r.after}&&r.serial>h.player_busy_city_serial&&
  r.operation==h.player_busy_city_operation+1&&h.player_busy_city_returned&&h.player_busy_city_return&&h.player_busy_returned&&h.player_busy_return&&
  h.player_busy_city_return->completed_without_callbacks()&&!h.player_busy_city_return->busy_without_callbacks()&&
  h.player_busy_return->completed_without_callbacks()&&!h.player_busy_return->busy_without_callbacks()&&
  h.player_dirty_resumed&&h.player_dirty_issuer&&h.player_dirty_issuer->queued_without_callbacks()&&h.player_dirty_issuer->publisher_returned_without_callbacks()&&
  h.player_dirty_issuer->delivered_without_callbacks()&&ui&&ui->engine_ui_turn_owner_&&
  state.setup.game_turn==old.turn&&state.world.pregame&&integer(state.world.pregame->archive,"game_turn")==old.turn&&
  integer(state.world.pregame->archive,"game_type")==0&&integer(state.world.pregame->archive,"active_player")==old.active_player&&!state.game_clock.pending&&
  (!old.pitboss_queried||dynamic_named_option(state.world.pregame->game_options,"GAMEOPTION_PITBOSS")==old.pitboss_saved)&&
  dynamic_options_same(state.world.pregame->game_options,active.raw_options)&&dynamic_options_same(state.setup.game_options,active.owned_options),
  "AI EndTurn actual native store/query/69/source/clock lineage changed");
 need(after_auto?(r.source_adopted&&h.processed_auto_return&&h.operation==h.processed_auto_return->operation+(h.processed_auto_return->source_adopted?1U:0U)):
  ((h.operation==r.operation&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_after_busy_owner)||
  (r.source_adopted&&h.operation==r.operation+1&&h.frontier.load()==r.next)),"AI EndTurn transition outside own exact once phase");
 active.require_operands(state,true);old.require_flags(state,true,r.stored,after_auto);
 need(r.next==(r.old_auto?BnwRetainedTurnFrontier::game_turn_player_end_turn_processed_auto_owner:BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner),
  "AI EndTurn auto equality/native mandatory branch changed");
}
void BnwRetainedTurnCursor::execute_game_turn_player_end_after_busy_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto&h=*i->history;h.thread_check();need(!h.retired.load()&&!h.poisoned.load()&&!h.game_turn_end_after_busy_attempted&&
  !h.game_turn_end_after_busy_return&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_after_busy_owner&&h.operation<UINT64_MAX,
  "AI EndTurn current once native predecessor absent");
 h.game_turn_end_after_busy_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  source.require_player_busy_city_source_without_callbacks(*this,core,&lease,false);
  const auto allocation=i->session->state_;auto&state=*allocation;const auto&old=*h.game_turn_player_end_turn_return;
  need(!old.human&&h.player_busy_city_return->completed_without_callbacks()&&!h.player_busy_city_return->busy_without_callbacks(),"AI EndTurn cannot bypass city/native remote check");
  History::GameTurnEndAfterBusyReturn staged;staged.issuer=source.action_self_;staged.world=allocation;staged.generation=h.generation;
  staged.before=state.ui_source_revision;staged.after=staged.before;staged.serial=serial;staged.operation=h.operation;staged.player=old.player;
  staged.old_end=old.old_end;staged.old_auto=old.old_auto;staged.setter_returned=!staged.old_auto;
  if(staged.old_end){need(staged.before<UINT64_MAX,"AI EndTurn revision exhausted before any flag store");staged.after=staged.before+1;}
  staged.next=staged.old_auto?BnwRetainedTurnFrontier::game_turn_player_end_turn_processed_auto_owner:BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner;
  const auto ui=i->cycle_ui.lock();need(ui&&ui->engine_ui_turn_owner_,"AI EndTurn actual UI/Engine lifetime missing");const auto control=require_actual_ui(*ui);const auto engine=ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*ui,*allocation);
  engine->require_current();i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*ui)==control&&ui->engine_ui_turn_owner_==engine,"AI EndTurn canonical UI/Engine substituted");
  i->check_engine();lease.require_operation(serial);
  // LAST pure parent physical64/RNG/clock/Source seals after providers. No
  // callback, allocation, UI publication or ProcessedAutoMoves write below.
  source.require_player_busy_city_source_without_callbacks(*this,core,&lease,false);
  need(i->session->state_==allocation&&state.ui_source_revision==staged.before&&h.operation==staged.operation&&serial>h.player_busy_city_serial,
   "AI EndTurn allocation/source/serial changed before native store");old.require_flags(state,true);
  static_assert(std::is_nothrow_move_constructible_v<History::GameTurnEndAfterBusyReturn>);
  h.game_turn_end_after_busy_return.emplace(std::move(staged));auto&earned=*h.game_turn_end_after_busy_return;
  if(earned.old_end){state.world.all_players->players[static_cast<std::size_t>(earned.player)].core.fields[old.end_indexes[static_cast<std::size_t>(earned.player)]].values[0]=0;
   state.ui_source_revision=earned.after;earned.stored=true;h.game_tail_earned_revision=earned.after;}
  // setAutoMoves(false): equal false performs no stores and does NOT reset
  // unsaved ProcessedAutoMoves. True is mandatory BEFORE both changed stores.
  source.adopt_actual_player_end_after_busy_return(*this,core,lease);earned.source_adopted=true;
  ++h.operation;h.frontier.store(earned.next);
  i->check_engine();lease.require_operation(serial);source.require_player_end_after_busy_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_player_end_after_busy_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_player_end_after_busy_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t before,std::uint64_t serial,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(h->game_turn_end_after_busy_return&&h->player_busy_city_return,"AI EndTurn retained physical return absent");
 const auto&r=*h->game_turn_end_after_busy_return;
 need(r.generation==generation&&r.before==before&&r.serial==serial&&r.source_adopted==adopted&&serial&&
  (adopted?h->operation==r.operation+1:h->operation==r.operation),"AI EndTurn exact issued/adopted return scope changed");
 require_player_end_after_busy_transition_without_callbacks(host,state,source,before,r.player);
 h->player_busy_city_return->require_after_busy_end_turn_without_callbacks(core,host,state,source,control,generation,h->player_busy_city_serial,*this);
}
std::uint64_t BnwRetainedTurnCursor::player_end_after_busy_before_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(h->game_turn_end_after_busy_return.has_value(),"AI EndTurn actual receipt absent");return h->game_turn_end_after_busy_return->before;
}
void BnwRetainedTurnCursor::fail_player_end_after_busy_without_callbacks()noexcept{impl_->history->quarantine();}
BnwPlayerEndAfterBusyProgress BnwRetainedTurnCursor::player_end_after_busy_progress()const{
 const auto h=impl_->history;h->thread_check();BnwPlayerEndAfterBusyProgress out;out.attempted=h->game_turn_end_after_busy_attempted;out.issued=bool(h->game_turn_end_after_busy_return);
 if(h->game_turn_end_after_busy_return){const auto&r=*h->game_turn_end_after_busy_return;out.player=r.player;out.before_end_turn=r.old_end;out.before_auto_moves=r.old_auto;
  out.end_flag_stored=r.stored;out.source_adopted=r.source_adopted;out.setter_returned=r.setter_returned;out.processed_auto_owner_required=r.old_auto;
  out.before_revision=r.before;out.after_revision=r.after;}
 return out;
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::require_processed_auto_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 if(h.processed_auto_attempted){source.require_processed_auto_reentry_without_callbacks(*this,core);h.quarantine();need(false,"ProcessedAutoMoves SAME canonical source reentry");}
 need(!h.retired.load()&&!h.poisoned.load()&&!h.processed_auto_return&&h.game_turn_end_after_busy_return&&
  h.game_turn_end_after_busy_return->source_adopted&&h.game_turn_end_after_busy_return->old_auto&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_processed_auto_owner,
  "ProcessedAutoMoves requires actual false-AI EndTurn changed-AutoMoves predecessor");
 source.require_player_end_after_busy_source_without_callbacks(*this,core,nullptr,true);
 const auto&host=*i->engine_utility->host;need(host.bnw_processed_auto_moves_import_!=nullptr,"ProcessedAutoMoves genuine original-Read seal missing; Civis continuation unsupported");
 host.bnw_processed_auto_moves_import_->require_host_control_without_callbacks(i->engine_utility->host);
 host.bnw_processed_auto_moves_import_->require_current_without_callbacks(host,*i->session->state_,h.generation);
}
void BnwRetainedTurnCursor::require_processed_auto_transition_without_callbacks(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,
 const BnwSelectedStockLuaSourceOwner&source,std::uint64_t query_revision,std::int32_t actor)const{
 const auto i=impl_;auto&h=*i->history;h.thread_check();need(h.processed_auto_attempted&&h.processed_auto_return&&h.game_turn_end_after_busy_return,
  "ProcessedAutoMoves typed actual receipt absent");
 const auto&r=*h.processed_auto_return;const auto&old=*h.game_turn_end_after_busy_return;
 const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&world.get()==&state&&
  r.generation==h.generation&&r.owner&&r.owner==host.bnw_processed_auto_moves_import_.get()&&r.player==actor&&actor==old.player&&
  r.before==old.after&&r.before<UINT64_MAX&&r.after==r.before+1&&r.operation==old.operation+1&&r.serial>old.serial&&
  old.old_auto&&old.source_adopted&&!old.setter_returned&&r.owner_before<UINT64_MAX&&r.owner_after==r.owner_before+1&&
  r.auto_stored&&r.processed_stored&&r.setter_returned&&state.ui_source_revision==r.after&&
  h.game_tail_earned_revision==std::optional{r.after}&&r.owner->sequence_without_callbacks()==r.owner_after&&
  !r.owner->value_without_callbacks(actor),"ProcessedAutoMoves actual paired store/once issuer/owner history differs");
 need((!r.source_adopted&&h.operation==r.operation&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_processed_auto_owner)||
  (r.source_adopted&&h.operation==r.operation+1&&h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner),
  "ProcessedAutoMoves outside exact own native continuation");
 r.owner->require_current_without_callbacks(host,state,h.generation);
 r.owner->require_host_control_without_callbacks(i->engine_utility->host);
 require_player_end_after_busy_bound_without_callbacks(host,state,source,query_revision,actor,true);
}
void BnwRetainedTurnCursor::execute_processed_auto_with_actual_stock(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source,
 const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto&h=*i->history;h.thread_check();need(!h.processed_auto_attempted&&!h.processed_auto_return&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_end_turn_processed_auto_owner&&h.operation<UINT64_MAX,
  "ProcessedAutoMoves once native predecessor absent");h.processed_auto_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  source.require_processed_auto_parent_source_without_callbacks(*this,core,&lease,false);
  const auto allocation=i->session->state_;auto&state=*allocation;const auto&host=*i->engine_utility->host;
  const auto&old=*h.game_turn_end_after_busy_return;auto*owner=host.bnw_processed_auto_moves_import_.get();
  need(owner&&old.old_auto&&old.source_adopted&&!old.setter_returned,"ProcessedAutoMoves cannot bypass genuine paired owner or native changed branch");
  owner->require_current_without_callbacks(host,state,h.generation);
  owner->require_host_control_without_callbacks(i->engine_utility->host);
  History::ProcessedAutoReturn staged;staged.issuer=source.action_self_;staged.world=allocation;staged.generation=h.generation;
  staged.before=state.ui_source_revision;need(staged.before<UINT64_MAX,"ProcessedAutoMoves revision exhausted before either store");staged.after=staged.before+1;
  staged.serial=serial;staged.operation=h.operation;staged.player=old.player;staged.owner=owner;
  staged.old_processed=owner->value_without_callbacks(staged.player);staged.owner_before=owner->sequence_without_callbacks();
  need(staged.owner_before<UINT64_MAX,"ProcessedAutoMoves owner history exhausted before native stores");staged.owner_after=staged.owner_before+1;
  const auto ui=i->cycle_ui.lock();need(ui&&ui->engine_ui_turn_owner_,"ProcessedAutoMoves actual Engine UI lifetime missing");
  const auto control=require_actual_ui(*ui);const auto engine=ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*ui,*allocation);
  engine->require_current();i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*ui)==control&&ui->engine_ui_turn_owner_==engine,"ProcessedAutoMoves actual UI/control substituted");
  i->check_engine();lease.require_operation(serial);
  // LAST pure seal after all callbackful providers. The issued receipt is
  // installed before the raw AutoMoves write and native unsaved false store.
  source.require_processed_auto_parent_source_without_callbacks(*this,core,&lease,false);
  owner->require_current_without_callbacks(host,state,h.generation);
  owner->require_host_control_without_callbacks(i->engine_utility->host);
  need(i->session->state_==allocation&&state.ui_source_revision==staged.before&&h.operation==staged.operation&&serial>old.serial&&
   host.bnw_processed_auto_moves_import_.get()==owner&&owner->sequence_without_callbacks()==staged.owner_before,
   "ProcessedAutoMoves actual allocation/owner/source changed before store");
  const auto&flags=*h.game_turn_player_end_turn_return;flags.require_flags(state,true,old.stored);
  static_assert(std::is_nothrow_move_constructible_v<History::ProcessedAutoReturn>);
  h.processed_auto_return.emplace(std::move(staged));auto&earned=*h.processed_auto_return;
  state.world.all_players->players[static_cast<std::size_t>(earned.player)].core.fields[flags.auto_indexes[static_cast<std::size_t>(earned.player)]].values[0]=0;
  earned.auto_stored=true;
  owner->store_false_without_callbacks(earned.player);earned.processed_stored=true;
  state.ui_source_revision=earned.after;h.game_tail_earned_revision=earned.after;earned.setter_returned=true;
  source.adopt_actual_processed_auto_return(*this,core,lease);earned.source_adopted=true;
  ++h.operation;h.frontier.store(BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner);
  i->check_engine();lease.require_operation(serial);source.require_processed_auto_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_processed_auto_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_processed_auto_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(h->processed_auto_return.has_value(),"ProcessedAutoMoves actual physical return absent");
 const auto&r=*h->processed_auto_return;need(r.generation==generation&&r.serial==serial&&serial&&r.source_adopted==adopted,
  "ProcessedAutoMoves exact issued Source/control scope changed");
 h->player_busy_city_return->require_after_processed_auto_without_callbacks(core,host,state,source,control,generation,h->player_busy_city_serial,*this);
}
std::uint64_t BnwRetainedTurnCursor::processed_auto_before_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(h->processed_auto_return.has_value(),"ProcessedAutoMoves issued revision absent");return h->processed_auto_return->before;
}
void BnwRetainedTurnCursor::fail_processed_auto_without_callbacks()noexcept{impl_->history->quarantine();}
BnwProcessedAutoMovesProgress BnwRetainedTurnCursor::processed_auto_moves_progress()const{
 const auto h=impl_->history;h->thread_check();BnwProcessedAutoMovesProgress out;out.attempted=h->processed_auto_attempted;out.issued=bool(h->processed_auto_return);
 if(h->processed_auto_return){const auto&r=*h->processed_auto_return;out.player=r.player;out.before_processed=r.old_processed;
  out.auto_stored=r.auto_stored;out.processed_stored=r.processed_stored;out.source_adopted=r.source_adopted;out.setter_returned=r.setter_returned;
  out.before_revision=r.before;out.after_revision=r.after;out.owner_before=r.owner_before;out.owner_after=r.owner_after;}return out;
}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::require_attrition_first_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();if(h->attrition_first_attempted){source.require_attrition_first_reentry_without_callbacks(*this,core);h->quarantine();need(false,"Attrition SAME active source reentry");}
 need(!h->retired.load()&&!h->poisoned.load()&&!h->attrition_first&&h->game_turn_end_after_busy_return&&
  h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner,
  "Attrition first entry requires actual false-AI EndTurn setter return");
 source.require_attrition_first_parent_source_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_first_with_actual_stock(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source,
 const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto&h=*i->history;h.thread_check();need(!h.attrition_first_attempted&&!h.attrition_first&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner&&h.operation<UINT64_MAX,"Attrition once native predecessor absent");h.attrition_first_attempted=true;
 try{i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  source.require_attrition_first_parent_source_without_callbacks(*this,core,&lease,false);
  const auto allocation=i->session->state_;auto&state=*allocation;const auto&host=*i->engine_utility->host;
  const auto&old=*h.game_turn_end_after_busy_return;History::AttritionFirstReturn staged;
  staged.issuer=source.action_self_;staged.world=allocation;staged.generation=h.generation;staged.player=old.player;
  staged.operation=h.operation;staged.serial=serial;staged.before=state.ui_source_revision;staged.after=staged.before;
  staged.after_processed=h.processed_auto_return.has_value();
  const auto ui=i->cycle_ui.lock();need(ui&&ui->engine_ui_turn_owner_,"Attrition real Engine UI missing");const auto control=require_actual_ui(*ui);const auto engine=ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*ui,*allocation);
  source.issue_attrition_first_owned(*this,core,lease,h.attrition_first);
  engine->require_current();i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*ui)==control&&ui->engine_ui_turn_owner_==engine,"Attrition canonical UI/control substituted");
  source.require_attrition_first_parent_source_without_callbacks(*this,core,&lease,false);
  const auto before=h.attrition_first->progress_without_callbacks();staged.after=before.after_revision;
  need(state.ui_source_revision==staged.before&&i->session->state_==allocation&&h.operation==staged.operation&&serial>old.serial,
   "Attrition actual source/allocation/native predecessor changed before effect");
  source.seal_attrition_first_without_callbacks(core,*h.attrition_first,lease,false);
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionFirstReturn>);h.attrition_first_return.emplace(std::move(staged));
  auto&earned=*h.attrition_first_return;h.attrition_first->execute_native_without_callbacks(state);
  h.game_tail_earned_revision=earned.after;
  const auto actual=h.attrition_first->progress_without_callbacks();
  switch(actual.boundary){
   case BnwAttritionPrefixBoundary::minor_open_borders_owner:earned.next=BnwRetainedTurnFrontier::game_turn_player_attrition_minor_ob_owner;break;
   case BnwAttritionPrefixBoundary::localized_damage_owner:earned.next=BnwRetainedTurnFrontier::game_turn_player_attrition_damage_owner;break;
   case BnwAttritionPrefixBoundary::mountain_domain_owner:earned.next=BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_owner;break;
   case BnwAttritionPrefixBoundary::active_deferred_fog_owner:earned.next=BnwRetainedTurnFrontier::game_turn_player_deferred_fog_owner;break;
   case BnwAttritionPrefixBoundary::active_count_owner:earned.next=BnwRetainedTurnFrontier::game_turn_player_active_count_owner;break;}
  source.adopt_actual_attrition_first_return(*this,core,lease);earned.source_adopted=true;
  ++h.operation;h.frontier.store(earned.next);
  i->check_engine();lease.require_operation(serial);source.require_attrition_first_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_attrition_first_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_first_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial,bool adopted)const{
 require_attrition_first_bound_without_callbacks(core,host,state,source,control,generation,serial,adopted,false);
}
void BnwRetainedTurnCursor::require_attrition_first_bound_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial,bool adopted,bool after_mountain)const{
 const auto i=impl_;auto&h=*i->history;h.thread_check();need(!h.retired.load()&&!h.poisoned.load()&&h.attrition_first_attempted&&h.attrition_first_return&&
  h.attrition_first&&h.game_turn_end_after_busy_return&&h.game_turn_player_end_turn_return&&h.game_turn_player_active_return,
  "Attrition physical actual prefix absent/retired");
 const auto&r=*h.attrition_first_return;const auto&end=*h.game_turn_end_after_busy_return;const auto&flags=*h.game_turn_player_end_turn_return;
 const auto&active=*h.game_turn_player_active_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 const auto ui=i->cycle_ui.lock();need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
  world&&world.get()==&state&&i->session->state_.get()==&state&&i->session->state_generation()==generation&&h.generation==generation&&r.generation==generation&&
  r.serial==serial&&serial&&r.source_adopted==adopted&&r.player==end.player&&!flags.human&&end.source_adopted&&
  r.after_processed==h.processed_auto_return.has_value()&&r.operation==(r.after_processed?h.processed_auto_return->operation+1:end.operation+1)&&
  r.before==(r.after_processed?h.processed_auto_return->after:end.after)&&
  (r.after_processed?(h.processed_auto_return->source_adopted&&h.processed_auto_return->setter_returned&&h.processed_auto_return->player==r.player):end.setter_returned)&&
  (after_mountain?(adopted&&h.attrition_mountain_return&&h.attrition_mountain_return->operation==r.operation+1&&
   h.operation==h.attrition_mountain_return->operation+(h.attrition_mountain_return->source_adopted?1U:0U)&&
   h.frontier.load()==(h.attrition_mountain_return->source_adopted?h.attrition_mountain_return->next:BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_owner)):
   (h.operation==r.operation+(adopted?1U:0U)&&h.frontier.load()==(adopted?r.next:BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner)))&&
  state.ui_source_revision==r.after&&h.game_tail_earned_revision==std::optional{r.after}&&ui&&ui->engine_ui_turn_owner_&&
  h.player_dirty_resumed&&h.player_dirty_issuer&&h.player_dirty_issuer->delivered_without_callbacks()&&
  state.setup.game_turn==flags.turn&&state.world.pregame&&integer(state.world.pregame->archive,"game_turn")==flags.turn&&
  integer(state.world.pregame->archive,"game_type")==0&&integer(state.world.pregame->archive,"active_player")==flags.active_player&&!state.game_clock.pending&&
  detail::capture_dynamic_mode_operands(state)==active.operands&&dynamic_options_same(state.world.pregame->game_options,active.raw_options)&&
  dynamic_options_same(state.setup.game_options,active.owned_options),"Attrition actual Source/69/native setter/clock/once history differs");
 flags.require_flags(state,true,end.stored,r.after_processed);
 for(std::size_t n=0;n<64;++n){const auto&f=detail::dynamic_core(state.world.all_players->players[n],"m_bTurnActive");
  need(f.encoding==saves::PlayerCoreEncoding::boolean&&&f==active.active_fields[n]&&f.values.data()==active.active_values[n]&&
   active.active_indexes[n]<state.world.all_players->players[n].core.fields.size()&&
   &state.world.all_players->players[n].core.fields[active.active_indexes[n]]==&f&&f.values[0]==(static_cast<std::int32_t>(n)==r.player?1:active.active[n]),
   "Attrition actual TurnActive64 allocation/value differs");}
 if(r.after_processed){const auto owner=host.bnw_processed_auto_moves_import_.get();need(owner&&owner==h.processed_auto_return->owner,
   "Attrition same processed owner missing");owner->require_current_without_callbacks(host,state,generation);owner->require_host_control_without_callbacks(i->engine_utility->host);
  need(owner->sequence_without_callbacks()==h.processed_auto_return->owner_after&&!owner->value_without_callbacks(r.player),"Attrition processed owner history changed");}
 h.attrition_first->require_current_without_callbacks(core,host,state,source,control,generation,serial,true);
 const auto q=h.attrition_first->progress_without_callbacks();need(q.executed&&q.before_revision==r.before&&q.after_revision==r.after&&
  r.after==r.before+(q.rng_stored?1U:0U),"Attrition actual bounded draw/revision does not match physical return");
}
void BnwRetainedTurnCursor::fail_attrition_first_without_callbacks()noexcept{impl_->history->quarantine();}
std::uint64_t BnwRetainedTurnCursor::attrition_first_before_without_callbacks()const{const auto h=impl_->history;h->thread_check();
 need(h->attrition_first_return.has_value(),"Attrition issued revision absent");return h->attrition_first_return->before;}
BnwAttritionPrefixProgress BnwRetainedTurnCursor::attrition_first_progress()const{const auto h=impl_->history;h->thread_check();return h->attrition_first?h->attrition_first->progress_without_callbacks():BnwAttritionPrefixProgress{};}
}

namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::end_after_busy_serial_without_callbacks()const{const auto h=impl_->history;h->thread_check();
 need(h->game_turn_end_after_busy_return.has_value(),"Actual native EndAfterBusy serial absent");return h->game_turn_end_after_busy_return->serial;}
std::uint64_t BnwRetainedTurnCursor::processed_auto_serial_without_callbacks()const{const auto h=impl_->history;h->thread_check();
 need(h->processed_auto_return.has_value(),"Actual native ProcessedAuto serial absent");return h->processed_auto_return->serial;}
std::uint64_t BnwRetainedTurnCursor::attrition_first_serial_without_callbacks()const{const auto h=impl_->history;h->thread_check();
 need(h->attrition_first_return.has_value(),"Actual native AttritionFirst serial absent");return h->attrition_first_return->serial;}
}

namespace civ5::runtime {
void BnwRetainedTurnCursor::require_attrition_mountain_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();if(h->attrition_mountain_attempted){h->quarantine();need(false,"Attrition mountain actual Source once reentry");}
 need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_first&&h->attrition_first_return&&h->attrition_first_return->source_adopted&&
  h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_owner,"Attrition mountain genuine native predecessor absent");
 source.require_attrition_mountain_parent_source_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_mountain_with_actual_stock(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source,
 const BnwStockLuaAttemptLease&lease){const auto i=impl_;auto&h=*i->history;h.thread_check();
 need(!h.attrition_mountain_attempted&&h.attrition_first&&!h.attrition_mountain_return&&h.operation<UINT64_MAX&&
  h.frontier.load()==BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_owner,"Attrition mountain once actual entry absent");h.attrition_mountain_attempted=true;
 try{i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);source.require_attrition_mountain_parent_source_without_callbacks(*this,core,&lease,false);
  const auto allocation=i->session->state_;const auto&state=*allocation;History::AttritionMountainReturn staged;
  staged.issuer=source.action_self_;staged.world=allocation;staged.generation=h.generation;staged.serial=serial;staged.operation=h.operation;staged.revision=state.ui_source_revision;
  const auto ui=i->cycle_ui.lock();need(ui&&ui->engine_ui_turn_owner_,"Attrition mountain actual Engine UI absent");const auto control=require_actual_ui(*ui);const auto engine=ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*ui,state);source.issue_attrition_mountain_owned(*this,core,lease,*h.attrition_first);
  engine->require_current();i->check_engine();lease.require_operation(serial);need(require_actual_ui(*ui)==control&&ui->engine_ui_turn_owner_==engine,"Attrition mountain actual UI/control substituted");
  source.require_attrition_mountain_parent_source_without_callbacks(*this,core,&lease,false);
  source.seal_attrition_mountain_without_callbacks(core,*h.attrition_first,lease,false);
  need(i->session->state_==allocation&&state.ui_source_revision==staged.revision&&h.operation==staged.operation,"Attrition mountain source replaced before native return");
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionMountainReturn>);h.attrition_mountain_return.emplace(std::move(staged));auto&earned=*h.attrition_mountain_return;
  h.attrition_first->execute_mountain_without_callbacks(state);const auto q=h.attrition_first->mountain_progress_without_callbacks();
  earned.next=q.damage_required?BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_damage_owner:BnwRetainedTurnFrontier::game_turn_player_attrition_death_owner;
  source.adopt_actual_attrition_mountain_return(*this,core,lease);earned.source_adopted=true;++h.operation;h.frontier.store(earned.next);
  i->check_engine();lease.require_operation(serial);source.require_attrition_mountain_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_attrition_mountain_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_mountain_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_mountain_attempted&&h->attrition_mountain_return&&
  h->attrition_first&&h->attrition_first_return,"Attrition mountain actual once return absent");const auto&r=*h->attrition_mountain_return;
 const auto issuer=r.issuer.lock();const auto world=r.world.lock();const auto&parent=*h->attrition_first_return;
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&world.get()==&state&&
  r.generation==generation&&r.serial==serial&&serial&&parent.serial<UINT64_MAX&&r.serial==parent.serial+1&&r.source_adopted==adopted&&
  r.operation==parent.operation+1&&r.revision==parent.after&&state.ui_source_revision==r.revision&&
  h->operation==r.operation+(adopted?1U:0U)&&h->frontier.load()==(adopted?r.next:BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_owner),
  "Attrition mountain exact Source/world/operation/revision differs");
 h->attrition_first->require_mountain_without_callbacks(core,host,state,source,control,generation,serial,true);
 const auto q=h->attrition_first->mountain_progress_without_callbacks();need(q.executed&&q.revision==r.revision&&
  r.next==(q.damage_required?BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_damage_owner:BnwRetainedTurnFrontier::game_turn_player_attrition_death_owner),
  "Attrition mountain physical condition/mandatory suffix differs");
 require_attrition_first_bound_without_callbacks(core,host,state,source,control,generation,parent.serial,true,true);
}
void BnwRetainedTurnCursor::fail_attrition_mountain_without_callbacks()noexcept{impl_->history->quarantine();}
BnwAttritionMountainProgress BnwRetainedTurnCursor::attrition_mountain_progress()const{const auto h=impl_->history;h->thread_check();
 return h->attrition_first?h->attrition_first->mountain_progress_without_callbacks():BnwAttritionMountainProgress{};}
}

#include "bnw_attrition_damage_cursor.inc"
#include "bnw_attrition_garrison_cursor.inc"
#include "bnw_attrition_city_strength_cursor.inc"
#include "bnw_attrition_city_info_cursor.inc"
#include "bnw_attrition_unit_info_cursor.inc"
#include "bnw_attrition_unit_selected_cursor.inc"
#include "bnw_attrition_selected_unit_info_cursor.inc"
#include "bnw_attrition_selection_plot_cursor.inc"
#include "bnw_attrition_plot_list_cursor.inc"
#include "bnw_attrition_damage_dll_wrapper_cursor.inc"
