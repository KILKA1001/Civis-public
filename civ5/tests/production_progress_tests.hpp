#pragma once
void production_progress_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    namespace content=civ5::content;
    constexpr std::int64_t big=4294967300LL;
    RulesDatabase db;db.apply_sql("CREATE TABLE Units(ID,Type);INSERT INTO Units VALUES(7,'UNIT7');"
        "CREATE TABLE Buildings(ID,Type);INSERT INTO Buildings VALUES(3,'BUILDING3');"
        "CREATE TABLE Projects(ID,Type);INSERT INTO Projects VALUES(2,'PROJECT2');"
        "CREATE TABLE Processes(ID,Type);INSERT INTO Processes VALUES(1,'PROCESS1');"
        "CREATE TABLE Specialists(ID,Type);INSERT INTO Specialists VALUES(0,'SPECIALIST0');");
    const auto types=db.cache_info_types();
    const auto field=[](auto &fields,const char *name,std::vector<std::int64_t> values){
        typename std::decay_t<decltype(fields)>::value_type f{};f.name=name;f.values=std::move(values);fields.push_back(std::move(f));};
    const auto record=[](const char *name,std::uint32_t hash,std::int64_t n){
        CityProductionRecord block{name},row{"row"};row.fields={{"hash",hash}};row.arrays={{"values",1,{n}}};block.children.push_back(row);return block;};
    const auto fixture=[&](TechnologyProfile profile){
        const bool vp=profile==TechnologyProfile::vp542;
        const auto layout=vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;
        SavedSessionSnapshot snap;snap.all_players.emplace();snap.all_players->layout=layout;
        SavedPlayerObservation player;player.saved_player_id=big;player.cities.layout=layout;
        player.cities.cities.emplace_back();auto &saved=player.cities.cities.back();
        saved.core.layout=saved.production.layout=layout;saved.core.start_offset=100;saved.core.end_offset=200;
        field(saved.core.fields,"m_iID",{big+1});field(saved.core.fields,"m_eOwner",{big});
        field(saved.core.fields,"m_iOverflowProduction",{big+2});field(saved.core.fields,"m_iFeatureProduction",{-3});
        field(saved.core.fields,"m_paiProjectProduction",{0,0,big+3});
        CityProductionRecord buildings{"buildings"},queue{"queue"};
        CityProductionRecord order{"order"};order.fields={{"eOrderType",0},
            {vp?"iData1":"iData1_hash",vp?7:static_cast<std::int64_t>(content::saved_info_type_hash("UNIT7"))},
            {"iData2",-5},{"bSave",1},{"bRush",0}};queue.children.push_back(order);
        if(vp){
            field(saved.core.fields,"m_paiUnitProduction",{0,0,0,0,0,0,0,big+4});
            field(saved.core.fields,"m_paiUnitProductionTime",{0,0,0,0,0,0,0,12});
            buildings.arrays={{"m_paiBuildingProduction",1,{0,0,0,big+5}},
                {"m_paiBuildingProductionTime",1,{0,0,0,8}}};
        }else{
            saved.production.context.bnw_city_version=5;
            CityProductionRecord before{"before_queue"};
            before.children.push_back(record("m_paiUnitProduction",content::saved_info_type_hash("UNIT7"),big+4));
            before.children.push_back(record("m_paiUnitProductionTime",content::saved_info_type_hash("UNIT7"),12));
            before.children.push_back(record("m_paiUnitProduction",0xdeadbeefu,17));
            // Keep the same-name source rows under their one enclosing collection.
            auto unknown=before.children.back();before.children.pop_back();before.children[0].children.push_back(unknown.children.front());
            buildings.children.push_back(record("m_paiBuildingProduction",content::saved_info_type_hash("BUILDING3"),big+5));
            buildings.children.push_back(record("m_paiBuildingProductionTime",content::saved_info_type_hash("BUILDING3"),8));
            saved.production.blocks.push_back(before);
        }
        saved.production.blocks.push_back(buildings);saved.production.blocks.push_back(queue);
        snap.all_players->players.push_back(std::move(player));
        CityYieldContext world;world.profile=profile;auto &city=world.players[big].cities[big+1];
        city.owner=big;city.id=big+1;city.source_core={{100,200}};
        return std::pair{std::move(snap),std::move(world)};
    };
    for(auto profile:{TechnologyProfile::vp542,TechnologyProfile::bnw}){
        const auto rules=load_production_progress_rules(db,profile);
        check(rules.unit_slots==8&&rules.building_slots==4&&rules.project_slots==3,
            "Production progress rules use sparse active Info IDs rather than fixed host capacities");
        auto [snap,world]=fixture(profile);
        restore_production_progress(snap,rules,types,world);
        const auto &city=world.players.at(big).cities.at(big+1);const auto &state=*city.production_progress;
        check(state.overflow_times100==big+2&&state.feature_times100==-3&&state.units.at(7)==big+4&&
            state.unit_times.at(7)==12&&state.buildings.at(3)==big+5&&state.building_times.at(3)==8&&
            state.projects.at(2)==big+3,"VP and BNW import all saved production progress into wide common state");
        check(current_production_progress_times100(city,rules)==big+4&&city.production_orders->front().secondary==-5&&
            city.production_orders->front().save,"Current queued unit reads imported progress without advancing the turn");
        const auto checkpoint=make_production_checkpoint(world,rules,types);
        const auto original_world=world;
        auto wrong=rules;wrong.profile=profile==TechnologyProfile::vp542?TechnologyProfile::bnw:TechnologyProfile::vp542;
        rejects([&]{current_production_progress_times100(city,wrong);});
        auto copied=world;copied.players.at(big).cities.at(big+1).production_progress->units[7]=99;
        check(world.players.at(big).cities.at(big+1).production_progress->units.at(7)==big+4,
            "Production progress snapshots own independent wide state");
        if(profile==TechnologyProfile::bnw){
            check(!state.fully_resolved&&state.unresolved_units==CityYieldState::ProductionProgress::HashedRows{{0xdeadbeefu,17}}&&
                city.production_orders->front().source_hash==content::saved_info_type_hash("UNIT7"),
                "Missing BNW mod hash survives import without changing a resolved current order");
            auto broken=snap;auto &order=broken.all_players->players[0].cities.cities[0].production.blocks.back().children[0];
            order.fields[1].value=0xdeadbeefu;CityYieldContext unresolved=world;unresolved.players.at(big).cities.at(big+1).production_orders.reset();
            restore_production_progress(broken,rules,types,unresolved);
            rejects([&]{current_production_progress_times100(unresolved.players.at(big).cities.at(big+1),rules);});
        }else check(state.fully_resolved,"VP indexed progress resolves through active modded Info slots");
        auto invalid=snap;invalid.all_players->players[0].cities.cities[0].core.fields[4].values.pop_back();
        rejects([&]{restore_production_progress(invalid,rules,types,world);});
        auto &mutable_city=world.players.at(big).cities.at(big+1);
        mutable_city.production_orders->front().type=2;mutable_city.production_orders->front().item=2;
        check(current_production_progress_times100(mutable_city,rules)==big+3,"Current project uses its own saved progress");
        mutable_city.production_orders->front().type=1;mutable_city.production_orders->front().item=3;
        check(current_production_progress_times100(mutable_city,rules)==big+5,"Current building uses its own saved progress");
        mutable_city.production_orders->front().type=profile==TechnologyProfile::vp542?3:4;
        mutable_city.production_orders->front().item=1;
        check(current_production_progress_times100(mutable_city,rules)==0,"Process has no per-item production cache");
        check(checkpoint.id==static_cast<std::uint32_t>(CivisSaveSectionId::live_city_production)&&
            checkpoint.bytes.size()>100,"Production checkpoint uses a versioned Civis section");
        auto resumed=original_world;
        auto &resumed_city=resumed.players.at(big).cities.at(big+1);
        resumed_city.production_progress->units[7]=big+100;
        resumed_city.production_progress->overflow_times100=big+101;
        resumed_city.production_orders->clear();
        const auto changed=make_production_checkpoint(resumed,rules,types);
        restore_production_checkpoint(changed,rules,types,world);
        check(world.players.at(big).cities.at(big+1).production_progress->units.at(7)==big+100&&
            world.players.at(big).cities.at(big+1).production_progress->overflow_times100==big+101&&
            world.players.at(big).cities.at(big+1).production_orders->empty(),
            "Civis checkpoint restores changed wide city progress and queue over the original snapshot");
        restore_production_checkpoint(checkpoint,rules,types,world);
        auto damaged=checkpoint;damaged.bytes.pop_back();
        rejects([&]{restore_production_checkpoint(damaged,rules,types,world);});
        damaged=checkpoint;damaged.bytes[16]^=1;
        rejects([&]{restore_production_checkpoint(damaged,rules,types,world);});
        auto wrong_city=original_world;wrong_city.players.at(big).cities.at(big+1).source_core={{101,200}};
        rejects([&]{restore_production_checkpoint(checkpoint,rules,types,wrong_city);});
        check(wrong_city.players.at(big).cities.at(big+1).production_progress->units.at(7)==big+4,
            "Rejected production checkpoint leaves the live city unchanged");
        db.apply_sql("INSERT INTO Units VALUES(8,'EXTRA_MOD_UNIT');");
        const auto expanded=db.cache_info_types();
        rejects([&]{restore_production_checkpoint(checkpoint,rules,expanded,world);});
        db.apply_sql("DELETE FROM Units WHERE ID=8;");
    }
}
