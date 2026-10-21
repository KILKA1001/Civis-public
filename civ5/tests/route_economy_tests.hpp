#pragma once
void route_economy_tests() {
    using namespace civ5::runtime;using namespace civ5::saves;
    struct Services final : RouteEconomyServices {
        std::vector<std::pair<std::int64_t,std::int64_t>> calls;
        std::function<void()> observe;
        bool fail=false;
        void game_data_dirty(std::int64_t player) override {calls.emplace_back(player,-1);if(observe)observe();if(fail)throw std::runtime_error("Host unavailable");}
        void test_over_resource_notification(std::int64_t player,std::int64_t resource) override {calls.emplace_back(player,resource);if(fail)throw std::runtime_error("Notification unavailable");}
    } service;
    constexpr std::int64_t large=4294967300LL;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Resources(ID,Type);INSERT INTO Resources(rowid,ID,Type) VALUES(1,0,'IRON'),(4294967301,4294967300,'COAL');"
        "CREATE TABLE Routes(ID,Type,GoldMaintenance);INSERT INTO Routes VALUES(0,'ROAD',3),(4294967300,'RAIL',4294967300);"
        "CREATE TABLE Route_ResourceQuantityRequirements(RouteType,ResourceType,Cost);"
        "INSERT INTO Route_ResourceQuantityRequirements VALUES('ROAD','IRON',99),('ROAD','IRON',2),('ROAD','COAL',7),"
        "('ROAD','MISSING',9),('RAIL','COAL',1),('RAIL','COAL',0);");
    const auto types=db.cache_info_types();
    auto rules=load_route_economy_rules(db,TechnologyProfile::vp542);
    check(rules.resource_slots==large+1&&rules.route_slots==large+1&&rules.routes.size()==2&&
        rules.routes.at(0).resources==std::map<std::int64_t,std::int64_t>{{0,2},{large,7}}&&
        rules.routes.at(large).gold_maintenance==large&&rules.routes.at(large).resources.empty(),
        "Route rules must widen sparse IDs, overwrite duplicate costs and resolve authored joins");
    CustomModOptions options;options.entries["BALANCE_VP"].value=1;options.entries["IMPROVEMENTS_EXTENSIONS"].value=1;
    RouteEconomyState state;state.profile=TechnologyProfile::vp542;state.players[0]={};state.players[large]={};
    auto &vp_counts=ensure_vp_route_economy_profile(state).responsible_routes;
    vp_counts[0]={};vp_counts[large]={};
    MaintenanceContext maintenance;maintenance.profile=state.profile;maintenance.players[0].base_improvement=12;maintenance.players[large].base_improvement=20;
    state.players[0].resources_used={{0,9},{large,15}};state.players[large].resources_used={{0,11},{large,12}};
    vp_counts[0][0]=1;
    RouteEconomyContext c{state,maintenance,rules,&options,service,0};
    SavedMapCellProjection plot;plot.index=large;plot.plot_type=2;plot.terrain_type=0;plot.route=SavedRoutePlot{0,0,0,true};
    service.observe=[&]{check(plot.route->responsible==0,"Route responsibility must change AFTER source side effects");};
    set_route_responsible(c,plot,large);
    service.observe={};
    check(plot.route->responsible==large&&plot.route->builder==0&&plot.route->pillaged&&
        maintenance.players[0].base_improvement==9&&maintenance.players[large].base_improvement==23&&
        vp_route_counts(state,0).empty()&&vp_route_counts(state,large).at(0)==1&&
        state.players[0].resources_used.at(0)==7&&state.players[0].resources_used.at(large)==8&&
        state.players[large].resources_used.at(0)==13&&state.players[large].resources_used.at(large)==19,
        "VP transfer must change shared maintenance and resources, retaining builder/pillage");
    check(service.calls==std::vector<std::pair<std::int64_t,std::int64_t>>{{0,-1},{0,-1},{large,-1},{0,-1},{large,-1}},
        "VP transfer source order is old/new maintenance then old/new usage by resource ID");
    service.calls.clear();set_route_responsible(c,plot,large);
    check(service.calls.empty(),"Unchanged responsibility must have no effects");
    for(const auto profile:{TechnologyProfile::bnw,TechnologyProfile::vp542}){
        state.profile=maintenance.profile=rules.profile=profile;c.options=profile==TechnologyProfile::vp542?&options:nullptr;
        for(int balance=0;balance<2;++balance){options.entries["BALANCE_VP"].value=balance;
            for(const auto category:{0,1,2,3})for(const auto terrain:{0,7})for(const bool trait:{false,true}){
                plot.plot_type=category;plot.terrain_type=terrain;state.players[0].no_hills_maintenance=trait;
                const bool expected=!(trait&&(category==1||(profile==TechnologyProfile::vp542&&balance&&(category==0||terrain==7))));
                check(must_pay_plot_maintenance(c,plot,0)==expected,"BNW/VP maintenance exemption differs");
            }
        }
    }
    plot.plot_type=2;plot.terrain_type=0;state.players[0].no_hills_maintenance=false;
    options.entries["IMPROVEMENTS_EXTENSIONS"].value=0;plot.route->responsible=0;service.calls.clear();
    const auto used=state.players[0].resources_used,counts=vp_route_counts(state,0);
    set_route_responsible(c,plot,-1);
    check(maintenance.players[0].base_improvement==6&&state.players[0].resources_used==used&&
        vp_route_counts(state,0)==counts&&service.calls.size()==1,"Disabled VP extensions must still transfer gold only");
    for(const auto type:{-1,123}){
        plot.route->type=type;service.calls.clear();set_route_responsible(c,plot,0);set_route_responsible(c,plot,-1);
        check(service.calls.empty(),"No route or missing Info changes responsibility without costs");
    }
    state.profile=maintenance.profile=rules.profile=TechnologyProfile::bnw;c.options=nullptr;
    rejects([&]{(void)vp_route_counts(state,0);});
    plot.route->type=0;plot.route->responsible=0;service.calls.clear();set_route_responsible(c,plot,large);
    check(service.calls.empty()&&maintenance.players[0].base_improvement==6&&plot.route->responsible==large,
        "BNW responsibility setter only assigns; costs belong to its route builder");
    service.calls.clear();change_resource_used(c,large,large,large);
    check(state.players[large].resources_used.at(large)==large+19&&
        service.calls==std::vector<std::pair<std::int64_t,std::int64_t>>{{large,large},{large,-1}},
        "BNW resource increment must notify before UI and retain int64 totals");
    service.calls.clear();change_resource_used(c,large,large,0);
    check(service.calls==std::vector<std::pair<std::int64_t,std::int64_t>>{{large,-1}},"Zero resource change still dirties game data");
    maintenance.players[0].base_improvement=0;service.calls.clear();change_base_improvement_maintenance(c,0,-3);
    check(maintenance.players[0].base_improvement==0&&service.calls.size()==1,"Maintenance compares requested value before clamping");
    service.calls.clear();change_base_improvement_maintenance(c,0,0);
    check(service.calls.empty(),"Unchanged maintenance must not dirty UI");
    change_base_improvement_maintenance(c,0,large);
    check(maintenance.players[0].base_improvement==large,"Shared maintenance must exceed UINT32_MAX");
    rejects([&]{change_base_improvement_maintenance(c,0,INT64_MAX);});
    rejects([&]{change_resource_used(c,large,large,INT64_MAX);});
    rejects([&]{change_resource_used(c,0,large+1,1);});
    rejects([&]{change_resource_used(c,123,0,1);});
    service.fail=true;const auto prior=state.players[0].resources_used.at(0);
    rejects([&]{change_resource_used(c,0,0,2);});
    check(state.players[0].resources_used.at(0)==prior+2,"Failed mandatory notification retains prior source mutation");
    state.profile=maintenance.profile=rules.profile=TechnologyProfile::vp542;c.options=&options;
    plot.route->responsible=0;const auto old_maintenance=maintenance.players[0].base_improvement;
    rejects([&]{set_route_responsible(c,plot,large);});
    check(plot.route->responsible==0&&maintenance.players[0].base_improvement==old_maintenance-3,
        "Failed host side effect must not advance responsibility or roll back source-order mutations");
    service.fail=false;c.options=nullptr;rejects([&]{set_route_responsible(c,plot,-1);});c.options=&options;
    // Source restoration: VP numeric vectors and first-wins map; BNW hashes
    // remap to a wide ID and duplicate saved hashes overwrite the previous value.
    for(const bool vp:{false,true}){
        const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        const auto layout=vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;
        RouteEconomyRules imported;imported.profile=profile;imported.resource_slots=vp?2:large+1;imported.route_slots=2;
        MaintenanceContext money;money.profile=profile;money.players[large]={};
        SavedSessionSnapshot snap{};snap.game_state.layout=layout;snap.all_players.emplace();snap.all_players->layout=layout;
        SavedPlayerObservation player{};player.saved_player_id=large;player.core.layout=player.traits.layout=player.tail.layout=layout;
        PlayerTraitsField trait{};trait.name="m_bNoHillsImprovementMaintenance";trait.values={1};player.traits.fields.push_back(trait);
        PlayerCoreField field{};field.name="m_paiNumResourceUsed";field.encoding=vp?PlayerCoreEncoding::vector_i32:PlayerCoreEncoding::hashed_i32;
        field.values=vp?std::vector<std::int64_t>{5,large}:std::vector<std::int64_t>{7,large};
        if(!vp)field.hashes={0,saved_info_type_hash("COAL"),saved_info_type_hash("COAL")};
        player.core.fields.push_back(field);
        PlayerTailNode count{};count.name="m_piResponsibleForRouteCount";count.encoding="stl_pairs";count.values={0,large,0,1,1,0};player.tail.fields.push_back(count);
        snap.all_players->players.push_back(player);
        const auto restored=restore_route_economy(snap,imported,money,types);const auto &p=restored.players.at(large);
        check(p.no_hills_maintenance&&p.resources_used.at(vp?1:large)==large&&
            (vp?(vp_route_counts(restored,large).at(0)==large&&vp_route_counts(restored,large).contains(1)):
                !restored.profile_extensions.find<VpRouteEconomyProfileState>()),
            "Route economy restore loses hash remaps, duplicate map semantics or wide state");
        snap.all_players->players[0].core.fields[0].values.push_back(9);
        rejects([&]{restore_route_economy(snap,imported,money,types);});
        // Project saved routes before optional terrain/graphics resolution.
        MapHeaderObservation header{};header.layout=layout;header.grid={1,1};MapPlotsObservation plots{};plots.layout=layout;
        PlotObservation cell{};cell.fields={{"route_type",large},{"route_responsible",large},{"route_builder",3},
            {vp?"packed_flags":"route_pillaged",vp?8:1}};plots.plots.push_back(cell);
        const auto projected=project_saved_map(header,plots);
        check(projected.cells[0].route==SavedRoutePlot{large,large,vp?std::optional<std::int64_t>(3):std::nullopt,true},
            "Shared map route projection must preserve profile-specific route fields with wide IDs");
        plots.plots[0].fields.erase(plots.plots[0].fields.begin());
        check(!project_saved_map(header,plots).cells[0].route,"Missing route wire fields must stay unresolved");
    }
}
