#include "civ5/runtime/bnw_new_game_player_setup.hpp"
#include "civ5/content/global_defines.hpp"
#include "civ5/runtime/bnw_fresh_game_builder.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>
namespace civ5::runtime {
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::int32_t index32(std::int64_t value){need(value>=0&&value<=std::numeric_limits<std::int32_t>::max(),"BNW player setup Info index outside source range");return static_cast<std::int32_t>(value);}
struct Info {
    BnwCreationInfoBinding identity;
    std::optional<std::string> default_color;
    bool playable=false;
};
struct Collection {
    std::vector<std::optional<Info>> slots;
    std::map<std::string,std::int32_t,std::less<>> types;
};
}
struct BnwNewGamePlayerSetup::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    const BnwFreshGame* const game;
    const std::shared_ptr<const BnwNewGameSetupRules> rules;
    const content::RulesDatabase* const database;
    const BnwNewGamePlayerSetupRequest request;
    bool attempted=false,first_player_attempted=false;
    std::shared_ptr<const BnwFreshFirstPlayerState> first_player;
    BnwNewGamePlayerSetupObservation state;
    std::map<std::string,Collection,std::less<>> collections;
    struct Team {
        BnwFreshTeamInfoArrays data;
        // Actual native constructor leaves TeamTechs source pointers unwritten.
        // Completed Init binds the SAME actual retained technology collection
        // and THIS owned Team; no supplied counts or observation issues links.
        const Collection* tech_source=nullptr;
        Team* tech_team=nullptr;
        bool info_initialized=false;
        Team(){data.fixed.id=0;data.fixed.relations.resize(64);data.fixed.found_players_territory.assign(64,false);}
    };
    std::vector<std::unique_ptr<Team>> teams;
    Impl(const BnwFreshGame& parent,std::shared_ptr<const BnwNewGameSetupRules> source,
         const content::RulesDatabase& db,BnwNewGamePlayerSetupRequest selected)
        :game(&parent),rules(std::move(source)),database(&db),request(std::move(selected)){}
    void current(const BnwFreshGame& parent)const {
        need(thread==std::this_thread::get_id(),"BNW team/player setup used from another thread");
        need(game==&parent&&rules&&database,"BNW team/player setup requires SAME owning Game");rules->require_current();
    }
    const Collection& collect(const char* table){
        current(*game);if(const auto found=collections.find(table);found!=collections.end())return found->second;
        Collection value;const bool colored=std::string_view(table)=="Civilizations"||std::string_view(table)=="MinorCivilizations";
        const auto report=database->prefetch_info(table,[&](std::uint64_t slot,std::int64_t authored,const content::RulesRowView& row){
            current(*game);need(slot<65536&&authored==static_cast<std::int64_t>(slot)&&row.has_column("ID")&&row.integer("ID")==authored&&row.has_column("Type"),"BNW selected collection Info span/schema unavailable");
            const auto type=row.text("Type");need(type&&!type->empty()&&type->find('\0')==std::string::npos,"BNW selected Info Type missing");
            Info info;info.identity={table,*type,index32(authored),content::saved_info_type_hash(*type)};
            if(colored){need(row.has_column("DefaultPlayerColor"),"BNW civilization color operand missing");info.default_color=row.text("DefaultPlayerColor");}
            if(std::string_view(table)=="Civilizations"){need(row.has_column("Playable"),"BNW civilization Playable source missing");info.playable=row.boolean("Playable");}
            if(value.slots.size()<=slot)value.slots.resize(static_cast<std::size_t>(slot)+1);
            need(value.types.emplace(*type,index32(authored)).second,"BNW selected Info duplicate Type unsupported");value.slots[static_cast<std::size_t>(slot)]=std::move(info);return true;
        },content::InfoCollectionBudget{65536});
        need(report.all_succeeded()&&report.slots==value.slots.size(),"BNW selected actual Info collection incomplete");
        return collections.emplace(table,std::move(value)).first->second;
    }
    const Info& info(const char* table,std::int32_t id){
        const auto& catalog=collect(table);need(id>=0&&static_cast<std::size_t>(id)<catalog.slots.size()&&catalog.slots[static_cast<std::size_t>(id)].has_value(),"BNW selected Info row is NULL/out of range");return *catalog.slots[static_cast<std::size_t>(id)];
    }
    std::int32_t type(const char* table,const std::string& selected){
        const auto& catalog=collect(table);const auto found=catalog.types.find(selected);need(found!=catalog.types.end(),"BNW selected roster Type not in activated Rules");return found->second;
    }
    std::int32_t define(const char* name,const char* table){
        current(*game);const auto result=database->global_define(name,content::GlobalDefineKind::integer);
        need(result.found&&result.error.empty()&&std::holds_alternative<std::int64_t>(result.value),"BNW required actual PostDefine index absent");
        const auto id=index32(std::get<std::int64_t>(result.value));(void)info(table,id);return id;
    }
    std::int32_t color(const char* table,std::int32_t id){
        const auto& row=info(table,id);need(row.default_color&&!row.default_color->empty(),"BNW selected default player color absent");return type("PlayerColors",*row.default_color);
    }
    std::size_t count(const char* table){return collect(table).slots.size();}
    void init_team(Team& owned,std::int32_t id){
        current(*game);auto& team=owned.data;
        // Actual CvTeam::init consists only of reset(eID). Fresh constructor
        // storage has no prior objects/effects to dispose of.
        team=BnwFreshTeamInfoArrays{};team.fixed.relations.resize(64);team.fixed.found_players_territory.assign(64,false);team.fixed.id=id;
        state.order.emplace_back("Team["+std::to_string(id)+"].uninit/scalars/fixed/resetID");
        const auto vote=count("VoteSources"),victory=count("Victories"),small=count("SmallAwards"),route=count("Routes"),build=count("Builds"),project=count("Projects"),unit=count("UnitClasses"),building_class=count("BuildingClasses"),building=count("Buildings"),terrain=count("Terrains"),improvement=count("Improvements");
        team.vote_eligibility.assign(vote,0);team.can_launch.assign(victory,false);team.victory_achieved.assign(victory,false);team.small_award_achieved.assign(small,false);
        team.route_change.assign(route,0);team.build_time_change.assign(build,0);team.project_count.assign(project,0);team.project_default_art.assign(project,0);team.project_making.assign(project,0);
        team.unit_class_count.assign(unit,0);team.building_class_count.assign(building_class,0);team.obsolete_building_count.assign(building,0);team.terrain_trade_count.assign(terrain,0);team.victory_countdown.assign(victory,-1);
        team.turn_team_met.assign(64,-1);team.turns_at_war.assign(64,0);team.turns_locked_into_war.assign(64,0);
        team.improvement_yield.assign(improvement,std::vector<std::int32_t>(6,0));team.improvement_no_fresh_water_yield.assign(improvement,std::vector<std::int32_t>(6,0));team.improvement_fresh_water_yield.assign(improvement,std::vector<std::int32_t>(6,0));
        state.order.emplace_back("Team["+std::to_string(id)+"].actual selected batch/relations/improvement yields");
        // Genuine selected tech source object and actual THIS pointer are bound
        // at original CvTeamTechs::Init, before its four allocated Reset arrays.
        owned.tech_source=&collect("Technologies");owned.tech_team=&owned;
        const auto techs=owned.tech_source->slots.size();team.has_tech.assign(techs,false);team.no_trade_tech.assign(techs,false);team.research_progress.assign(techs,0);team.tech_count.assign(techs,0);team.last_tech_acquired=-1;
        team.project_art_types.assign(project,{});team.revealed_resources.clear();team.fixed.dynamic_info_arrays_allocated=true;team.fixed.constructor_tech_arrays_empty=false;
        owned.info_initialized=true;++state.initialized_teams;state.order.emplace_back("Team["+std::to_string(id)+"].TeamTechs Init/Reset/project art/revealed clear return");
    }
};
BnwNewGamePlayerSetup::BnwNewGamePlayerSetup(const BnwFreshGame& game,std::shared_ptr<const BnwNewGameSetupRules> rules,
    const content::RulesDatabase& db,BnwNewGamePlayerSetupRequest request):impl_(std::make_unique<Impl>(game,std::move(rules),db,std::move(request))){impl_->current(game);}
BnwNewGamePlayerSetup::~BnwNewGamePlayerSetup()=default;
void BnwNewGamePlayerSetup::require_same_game(const BnwFreshGame& game)const{impl_->current(game);}
void BnwNewGamePlayerSetup::advance_team_roster_prefix(const BnwFreshGame& game){
    auto& p=*impl_;p.current(game);need(!p.attempted,"SAME Game InitPlayers prefix cannot replay");p.attempted=true;
    const auto selected=p.rules->observe().selected;p.state.selected_rules_sha256=selected.rules_sha256;
    need(p.request.minors.size()<=41&&p.request.active_player>=0&&p.request.active_player<22,"BNW selected roster capacity/active player invalid");
    need(p.request.majors[static_cast<std::size_t>(p.request.active_player)].status==BnwFreshSlotStatus::taken,"Single player roster requires an actual selected human slot");
    const auto standard=p.define("STANDARD_HANDICAP","HandicapInfos");
    std::size_t humans=0;
    for(std::size_t i=0;i<64;++i){auto& slot=p.state.roster[i];slot.team=static_cast<std::int32_t>(i);slot.handicap=standard;}
    // SDK resetPlayer sets team=(TeamTypes)p, status CLOSED, NONE identities,
    // standard handicap, NO_PLAYERCOLOR, netID=-1. Real menu choices overwrite it.
    for(std::size_t i=0;i<22;++i){const auto& input=p.request.majors[i];auto& slot=p.state.roster[i];
        need(input.status<=BnwFreshSlotStatus::observer,"BNW chosen major slot status invalid");slot.status=input.status;
        if(input.status==BnwFreshSlotStatus::taken)++humans;
        if(input.team){need(*input.team>=0&&*input.team<63,"BNW major team choice invalid");slot.team=*input.team;}
        if(input.status==BnwFreshSlotStatus::taken||input.status==BnwFreshSlotStatus::computer||input.status==BnwFreshSlotStatus::observer){
            slot.civilization=p.type("Civilizations",input.civilization);need(p.info("Civilizations",slot.civilization).playable,"Chosen major civilization is not playable");slot.leader=p.type("Leaders",input.leader);
            const auto link=p.database->query_rows({"SELECT LeaderheadType FROM Civilization_Leaders WHERE CivilizationType = ? AND LeaderheadType = ?",{input.civilization,input.leader}},[](const content::RulesRowView&){},2);
            need(link.all_succeeded()&&link.rows==1,"Selected leader is not assigned to actual Civilization rules");slot.handicap=selected.info[5].id;
        }else need(input.civilization.empty()&&input.leader.empty(),"Closed/open slot contains unresolved active civilization choice");
        if(input.player_color)slot.color=p.type("PlayerColors",*input.player_color);
    }
    need(humans==1,"BNW own single-player setup requires exactly one chosen human");
    for(std::size_t i=0;i<p.request.minors.size();++i){auto& slot=p.state.roster[22+i];const auto& input=p.request.minors[i];slot.minor_type=p.type("MinorCivilizations",input.minor_civilization);
        if(input.team){need(*input.team>=0&&*input.team<63,"BNW minor team choice invalid");slot.team=*input.team;}
    }
    // Qualify all source counts before allocation. Arrays use genuine native
    // sparse spans; no SQL count/MAX(ID) or supplied collection readiness.
    std::uint64_t cells=0;for(const auto* table:{"VoteSources","Victories","SmallAwards","Routes","Builds","Projects","UnitClasses","BuildingClasses","Buildings","Terrains","Improvements","Technologies"}){
        const auto n=p.count(table);const std::uint64_t multiplier=std::string_view(table)=="Improvements"?18u:std::string_view(table)=="Technologies"?4u:std::string_view(table)=="Projects"?4u:std::string_view(table)=="Victories"?3u:1u;cells+=n*multiplier;
    }
    need(cells+3u*64u<=32ULL*1024*1024/64,"Actual BNW Team init allocation exceeds owned budget");
    p.teams.reserve(80);for(std::size_t i=0;i<80;++i){p.teams.push_back(std::make_unique<Impl::Team>());p.state.order.emplace_back("Team constructor reset(0,true)");}
    // Exact initStatics tail assigns all80 IDs AFTER all constructor calls.
    for(std::size_t i=0;i<80;++i)p.teams[i]->data.fixed.id=static_cast<std::int32_t>(i);
    p.state.order.emplace_back("Team initStatics actual80 ID writes");
    for(std::int32_t i=0;i<64;++i)p.init_team(*p.teams[static_cast<std::size_t>(i)],i);
    p.state.color_workspace.fill(-1);
    for(std::size_t i=0;i<22;++i){const auto& slot=p.state.roster[i];p.state.color_workspace[i]=slot.color;
        if(slot.color==-1&&(slot.status==BnwFreshSlotStatus::taken||slot.status==BnwFreshSlotStatus::computer||slot.status==BnwFreshSlotStatus::observer))p.state.color_workspace[i]=p.color("Civilizations",slot.civilization);
    }
    const auto barb_civ=p.define("BARBARIAN_CIVILIZATION","Civilizations"),minor_civ=p.define("MINOR_CIVILIZATION","Civilizations");
    const auto barb_color=p.color("Civilizations",barb_civ);const auto color_count=p.count("PlayerColors");
    // Preserve native duplicate algorithm: it scans all candidates without
    // breaking when a free replacement was found; the last available wins.
    for(std::size_t i=0;i<22;++i){if(p.state.color_workspace[i]==-1)continue;
        for(std::size_t j=0;j<i;++j){if(p.state.color_workspace[i]!=p.state.color_workspace[j])continue;
            for(std::size_t k=0;k<color_count;++k){const auto candidate=static_cast<std::int32_t>(k);if(candidate==barb_color)continue;
                bool valid=true;for(std::size_t l=0;l<22;++l)if(p.state.color_workspace[l]==candidate){valid=false;break;}
                if(valid)p.state.color_workspace[i]=candidate;
            }break;
        }
    }
    p.state.order.emplace_back("InitPlayers actual80 color workspace/major22/duplicate loops");
    const auto barb_handicap=p.define("BARBARIAN_HANDICAP","HandicapInfos"),barb_leader=p.define("BARBARIAN_LEADER","Leaders"),ai_handicap=p.define("AI_HANDICAP","HandicapInfos"),minor_handicap=p.define("MINOR_CIV_HANDICAP","HandicapInfos");
    for(std::size_t i=0;i<64;++i){auto& slot=p.state.roster[i];
        if(i==63){slot.team=63;slot.status=BnwFreshSlotStatus::computer;slot.net_id=-1;slot.handicap=barb_handicap;slot.civilization=barb_civ;slot.leader=barb_leader;slot.color=barb_color;slot.minor=false;}
        else if(i<22){slot.color=p.state.color_workspace[i];if(slot.status==BnwFreshSlotStatus::computer)slot.handicap=ai_handicap;else if(slot.status==BnwFreshSlotStatus::observer)slot.team=21;}
        else if(i<22+p.request.minors.size()){slot.status=BnwFreshSlotStatus::computer;slot.net_id=-1;slot.handicap=minor_handicap;slot.civilization=minor_civ;slot.leader=barb_leader;slot.color=p.color("MinorCivilizations",slot.minor_type);slot.minor=true;}
        p.state.order.emplace_back("InitPlayers roster slot["+std::to_string(i)+"] original-order writes");
    }
    p.current(game);p.state.boundary=BnwNewGamePlayerSetupBoundary::player_reset_required;
    p.state.mandatory={"Actual first CvPlayer::init notifications/diplomacy allocation and full Reset on SAME Game", "Actual player containers/setupGraphical/team membership/setAlive/traits/personality and AI Init", "Remaining64 player initialization before publishing IsEverAlive/starting plot API to selected map script"};
}
void BnwNewGamePlayerSetup::advance_first_player_reset_prefix(const BnwFreshGame& game,
    std::shared_ptr<const BnwFreshGameContentResult> selected,const BnwFreshTacticalMap& actual_map,std::int32_t actual_game_handicap){
    auto& p=*impl_;p.current(game);
    need(p.state.boundary==BnwNewGamePlayerSetupBoundary::player_reset_required&&p.state.initialized_teams==64&&!p.first_player_attempted,
         "First actual player prefix requires SAME completed real team/roster source");
    p.first_player_attempted=true;p.state.boundary=BnwNewGamePlayerSetupBoundary::failed_prefix;
    need(selected&&selected->rules.get()==p.database&&selected->content_ready(),"First Player requires SAME genuine activated Content supplied by owning Game");
    // Compile-time BNW dimensions are qualified from the SDK; indexed Define
    // operands are the actual retained immutable PostDefines. There is no
    // imported header, caller-ready flag, new RNG or separate supplied realm.
    std::vector<content::GameplayDefine> constants{{"MAX_TEAMS",64},{"MAX_PLAYERS",64},{"MAX_CIV_PLAYERS",63},{"MAX_MAJOR_CIVS",22},{"BARBARIAN_PLAYER",63},{"BARBARIAN_TEAM",63},{"NUM_DOMAIN_TYPES",5},{"NUM_YIELD_TYPES",6}};
    for(const auto* name:{"BARBARIAN_CIVILIZATION","BARBARIAN_HANDICAP","BARBARIAN_LEADER","MINOR_CIVILIZATION","MINOR_CIV_HANDICAP","AI_HANDICAP"}){
        const auto value=p.database->global_define(name,content::GlobalDefineKind::integer);
        need(value.found&&value.error.empty()&&std::holds_alternative<std::int64_t>(value.value),"First Player actual indexed Define missing");
        constants.push_back({name,std::get<std::int64_t>(value.value)});
    }
    std::vector<BnwFreshPregameSlot> slots;std::vector<BnwFreshMajorColorSlot> colors;std::vector<std::int32_t> leaders,minors;
    slots.reserve(64);leaders.reserve(64);colors.reserve(22);minors.reserve(p.request.minors.size());
    for(std::size_t i=0;i<64;++i){const auto& slot=p.state.roster[i];slots.push_back({slot.status,slot.team});leaders.push_back(slot.leader);
        if(i<22)colors.push_back({slot.status,slot.civilization,slot.color});
        else if(i<22+p.request.minors.size())minors.push_back(slot.minor_type);
    }
    const BnwFreshGameBuilderInputs inputs{constants,slots,colors,leaders,minors,static_cast<std::int32_t>(minors.size()),21,p.state.roster[63].civilization};
    // Invoke the existing actual executable owned Reset/service producer.
    // Its detached projections are validation/planning only and are NEVER
    // installed as Game Team/Player/alive state or a completion receipt.
    auto produced=draft_bnw_fresh_gamecore(*selected,inputs);p.current(game);
    if(!produced.first_player_state || !produced.frontier ||
       produced.frontier->code!=BnwFreshGameBuilderBlockerCode::tactical_game_analysis_map_source_pending){
        p.state.mandatory={produced.frontier?produced.frontier->message:"Existing actual first Player owner was not issued"};
        throw std::runtime_error(p.state.mandatory.front());
    }
    p.first_player=std::move(produced.first_player_state);
    need(p.first_player->selected_rules_sha256()==p.state.selected_rules_sha256,"Actual first player belongs to another selected Rules source");
    p.first_player->complete_tactical_init_from_game(actual_map,*p.database,actual_game_handicap);
    p.state.first_player_tactical_game_map_bound=p.first_player->tactical_init_map_is_bound();
    for(const auto* name:{"recruit_range","land_barbarian_range","sea_barbarian_range","reposition_range","deploy_radius","random_range"})
        p.state.first_player_tactical_integers.emplace(name,p.first_player->tactical_init_integer(name));
    p.state.first_player_tactical_flavor_dampening=p.first_player->tactical_init_flavor_dampening();
    for(const auto& [key,value]:p.first_player->tactical_init_cached_types())p.state.first_player_tactical_cached_types.emplace_back(key,value);
    p.state.first_player_id=std::get<std::int32_t>(p.first_player->registered_member("m_eID").value);
    need(*p.state.first_player_id==0,"Actual CvGame::InitPlayers first player must be slot0");
    p.state.first_player_tactical_priority_turn=p.first_player->tactical_reset_value("move_priority_turn");
    p.state.first_player_tactical_reset_map_null=p.first_player->tactical_reset_map_is_null();
    for(const auto source:p.first_player->source_order())p.state.first_player_reset_order.emplace_back(source);
    p.state.order.emplace_back("Actual first-player owned Reset/services and SAME Game Tactical Init return retained");
    p.state.boundary=BnwNewGamePlayerSetupBoundary::first_player_homeland_required;
    p.state.mandatory={"Actual next HomelandAI Init and remaining first Player Reset", "First player containers/setupGraphical/setAlive/traits/personality/AI and remaining native64 players before map getters"};
    p.current(game);
}
BnwNewGamePlayerSetupObservation BnwNewGamePlayerSetup::observe_from_game(const BnwFreshGame& game)const{
    impl_->current(game);auto result=impl_->state;result.teams.reserve(impl_->teams.size());
    for(const auto& team:impl_->teams){if(team->info_initialized){need(team->tech_team==team.get()&&team->tech_source==&impl_->collections.at("Technologies"),"Actual initialized TeamTechs binding changed");}result.teams.push_back(team->data);}
    impl_->current(game);return result;
}
}
