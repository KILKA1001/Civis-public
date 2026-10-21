#pragma once
#include "civ5/runtime/vp_city_construction_intrigue.hpp"

void vp_city_construction_intrigue_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    constexpr auto vp=content::TechnologyProfile::vp542;
    constexpr auto layout=saves::GameStateLayout::vp542_corpus;
    constexpr std::int64_t first=5000000002LL,second=5000000001LL;
    saves::SavedSessionSnapshot snapshot;
    snapshot.all_players.emplace();snapshot.all_players->layout=layout;
    saves::SavedPlayerObservation saved{};
    saved.saved_player_id=0;saved.cities.layout=layout;
    for(const auto [id,offset]:{std::pair{first,std::uint64_t{20}},
                                std::pair{second,std::uint64_t{40}}}) {
        saves::SavedCityObservation city{};
        city.core.layout=layout;city.core.start_offset=offset;city.core.end_offset=offset+10;
        saves::CityCoreField field{};field.name="m_iID";field.values={id};
        city.core.fields.push_back(field);
        saved.cities.cities.push_back(city);
    }
    snapshot.all_players->players.push_back(saved);
    CityYieldContext world;world.profile=vp;
    for(const auto [id,offset]:{std::pair{first,std::uint64_t{20}},
                                std::pair{second,std::uint64_t{40}}}) {
        CityYieldState city{};
        city.id=id;city.owner=0;city.x=id==first?3:5;city.y=7;
        city.source_core=std::pair{offset,offset+10};
        city.production_orders=std::vector<CityYieldState::ProductionOrder>{};
        world.players[0].cities.emplace(id,city);
    }
    restore_vp_city_intrigue_order_profile(snapshot,world);
    check(find_vp_city_intrigue_order_profile(world)->players.at(0)==
              std::vector<std::int64_t>{first,second},
          "VP city iteration must preserve saved source order, including wide IDs");
    content::RulesDatabase db;
    db.apply_sql("CREATE TABLE Projects(ID INTEGER PRIMARY KEY,Type TEXT,IsRepeatable INTEGER);"
        "INSERT INTO Projects VALUES(7,'PROJECT_SPACE',0),(8,'PROJECT_REPEAT',1);"
        "CREATE TABLE BuildingClasses(Type TEXT PRIMARY KEY,MaxGlobalInstances INTEGER);"
        "INSERT INTO BuildingClasses VALUES('WORLD',1),('ORDINARY',-1);"
        "CREATE TABLE Buildings(ID INTEGER PRIMARY KEY,BuildingClass TEXT);"
        "INSERT INTO Buildings VALUES(9,'WORLD'),(10,'ORDINARY');"
        "CREATE TABLE Units(ID INTEGER PRIMARY KEY,SpaceshipProject TEXT);"
        "INSERT INTO Units VALUES(11,'PROJECT_SPACE'),(12,NULL);");
    auto &a=world.players.at(0).cities.at(first);
    auto &b=world.players.at(0).cities.at(second);
    a.production_orders=std::vector<CityYieldState::ProductionOrder>{{2,7}};
    b.production_orders=std::vector<CityYieldState::ProductionOrder>{{1,9}};
    auto scan=vp_first_city_construction_intrigue(world,db,0);
    check(scan.scanned_cities==1&&scan.candidate&&scan.candidate->city==first&&
          scan.candidate->order_type==2&&scan.candidate->item==7,
          "VP intrigue must choose the first eligible city in source order");
    db.apply_sql("UPDATE Projects SET IsRepeatable=1 WHERE ID=7");
    scan=vp_first_city_construction_intrigue(world,db,0);
    check(scan.scanned_cities==2&&scan.candidate&&scan.candidate->city==second&&
          scan.candidate->order_type==1,
          "A mod SQL update must change project eligibility without a C++ port");
    db.apply_sql("UPDATE BuildingClasses SET MaxGlobalInstances=-1 WHERE Type='WORLD'");
    a.production_orders=std::vector<CityYieldState::ProductionOrder>{{0,11}};
    scan=vp_first_city_construction_intrigue(world,db,0);
    check(scan.scanned_cities==1&&scan.candidate&&scan.candidate->city==first&&
          scan.candidate->order_type==0,
          "VP intrigue must recognize spaceship-project units through the live rules table");
    db.apply_sql("UPDATE Units SET SpaceshipProject=NULL WHERE ID=11");
    scan=vp_first_city_construction_intrigue(world,db,0);
    check(scan.scanned_cities==2&&!scan.candidate,
          "Removing a modded spaceship-project link must remove the intrigue candidate");
    a.production_orders=std::vector<CityYieldState::ProductionOrder>{{2,999}};
    rejects([&]{(void)vp_first_city_construction_intrigue(world,db,0);});
    a.production_orders.reset();
    rejects([&]{(void)vp_first_city_construction_intrigue(world,db,0);});
    a.production_orders=std::vector<CityYieldState::ProductionOrder>{};
    auto bad=snapshot;
    bad.all_players->players[0].cities.cities[1].core.fields[0].values={first};
    rejects([&]{restore_vp_city_intrigue_order_profile(bad,world);});
    check(find_vp_city_intrigue_order_profile(world)->players.at(0)==
              std::vector<std::int64_t>{first,second},
          "Failed VP city order import must leave the previous profile intact");
    CityYieldContext vanilla;
    rejects([&]{(void)ensure_vp_city_intrigue_order_profile(vanilla);});
    check(!find_vp_city_intrigue_order_profile(vanilla),
          "Vanilla city runtime must not require VP intrigue ordering");
}
