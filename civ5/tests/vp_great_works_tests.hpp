#pragma once
void vp_great_works_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    constexpr auto vp=TechnologyProfile::vp542;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('CITY_MAX_NUM_BUILDINGS',1),('BASE_CULTURE_PER_GREAT_WORK',3),('BASE_TOURISM_PER_GREAT_WORK',2);CREATE TABLE Yields(ID,Type);INSERT INTO Yields VALUES(4,'YIELD_CULTURE'),(6,'YIELD_TOURISM'),(12,'YIELD_JFD');CREATE TABLE BuildingClasses(ID,Type);INSERT INTO BuildingClasses VALUES(1,'CLASS');CREATE TABLE GreatWorkSlots(ID,Type);INSERT INTO GreatWorkSlots VALUES(7,'GREAT_WORK_SLOT_ART_ARTIFACT'),(8,'GREAT_WORK_SLOT_LITERATURE'),(9,'GREAT_WORK_SLOT_MUSIC'),(10,'GREAT_WORK_SLOT_FILM'),(11,'GREAT_WORK_SLOT_RELIC');CREATE TABLE GreatWorkClasses(ID,Type);INSERT INTO GreatWorkClasses VALUES(2,'GREAT_WORK_ART'),(3,'GREAT_WORK_ARTIFACT');CREATE TABLE Buildings(ID,Type,BuildingClass,GreatWorkSlotType,GreatWorkYieldType,GreatWorkCount);INSERT INTO Buildings VALUES(10,'MUSEUM','CLASS','GREAT_WORK_SLOT_ART_ARTIFACT','YIELD_CULTURE',2);CREATE TABLE Building_ThemingYieldBonus(BuildingType,YieldType,Yield);INSERT INTO Building_ThemingYieldBonus VALUES('MUSEUM','YIELD_TOURISM',99),('MUSEUM','YIELD_TOURISM',3);CREATE TABLE Building_ThemingBonuses(BuildingType,Bonus,Description,SameEra,UniqueEras,ConsecutiveEras,MustBeArt,MustBeArtifact,MustBeEqualArtArtifact,RequiresOwner,RequiresAnyButOwner,RequiresSamePlayer,RequiresUniquePlayers,AIPriority);INSERT INTO Building_ThemingBonuses(BuildingType,Bonus) VALUES('MUSEUM',2),('MUSEUM',3);CREATE TABLE Beliefs(ID,Type,MinPopulation);INSERT INTO Beliefs VALUES(1,'BELIEF',5);CREATE TABLE Belief_GreatWorkYieldChanges(BeliefType,YieldType,Yield);INSERT INTO Belief_GreatWorkYieldChanges VALUES('BELIEF','YIELD_CULTURE',2);");
    auto types=db.cache_info_types();auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);auto rules=load_vp_great_work_rules(db,types);
    check(rules.buildings.at(10).themes==std::vector<std::int64_t>{2,3}&&rules.buildings.at(10).theme_yields.at(6)==3&&rules.slots==std::array<std::int64_t,5>{7,8,9,10,11}&&rules.beliefs.at(1).minimum_population==5,"Great-work rules bind types, preserve theme order and last yield assignment");
    CustomModOptions options;options.entries["BUILDINGS_THOROUGH_PREREQUISITES"].value=1;
    ResearchContext setup;setup.profile=vp;setup.players[5].slot_status=3;setup.players[5].team=9;
    CityYieldContext cities;cities.profile=vp;cities.yield_slots=17;auto &p=cities.players[5];p.civilization=0;p.capital=42;vp_player_state_religion(p)=-1;p.second_pantheon_count=0;
    auto &city=p.cities[42];city.id=42;city.owner=5;city.population=8;city.building_counts_resolved=true;city.real_buildings[10]=1;vp_city_majority_religion(city)=-1;
    auto &great_work_profile=ensure_vp_great_work_profile(cities);auto &s=great_work_profile.cities[{5,42}];auto &pg=great_work_profile.players[5];s.existing_buildings={10};s.theming_indices[10]=0;s.works={{1,0,0},{1,1,1}};
    cities.great_works.emplace();(*cities.great_works)[0].class_type=2;(*cities.great_works)[1].class_type=3;
    SavedMapProjection map{{1,1}};PlotPassability pass;PlotWaterRules wr;wr.profile=vp;PlotWaterState ws;ws.layout=GameStateLayout::vp542_corpus;
    PlotWaterContext water{map,ws,wr,types,pass,setup,cities,defines,&options};VpPlotReligionRules rr;rr.beliefs[1]={};VpPlotReligionContext religion{rr,water};
    CityYieldRules cr;cr.profile=vp;cr.civilization_buildings[0][1]=10;
    struct Services:VpGreatWorkServices {std::vector<std::int64_t> awards;void unlock_capital_theming_achievement(std::int64_t id)override{awards.push_back(id);}} services;
    VpGreatWorkContext c{cities,cr,rules,religion,services};
    rejects([&]{vp_building_from_class(c,city,-1);});
    check(!s.building_by_class,"Rejected NO_BUILDINGCLASS leaves the lazy class cache cold");
    auto q=vp_great_work_yield(c,city,4);check(q.works==2&&q.matching_works==2&&q.base_times100==600&&q.themes==2&&q.total_times100==800,"Great works add building-specific base culture and theming");
    pg.per_work[6]=1;pg.trait_per_work[6]=2;s.per_work[6]=3;s.tourism_modifier=25;pg.tourism_modifier=10;
    pg.type_bonus[0][6]=4;pg.trait_type_bonus[0][6]=1;pg.type_bonus[1][6]=6;pg.trait_type_bonus[1][6]=2;s.per_local_theme[6]=7;
    q=vp_great_work_yield(c,city,6);check(q.per_work==8&&q.base_times100==1600&&q.tourism_times100==560&&q.themes==2&&q.extra_themes==10&&q.type_bonus==13&&q.total_times100==4660,"Tourism multiplier excludes themes, local theme yields and art/artifact bonuses");
    check(vp_cached_great_work_yield(c,city,6)==4660,"Lazy cache computes fresh great-work yields");pg.per_work[6]=10;check(vp_cached_great_work_yield(c,city,6)==4660,"Saved city yield cache remains valid until explicit invalidation");
    invalidate_vp_great_work_yields(cities,city);check(vp_cached_great_work_yield(c,city,6)==7090,"Invalidation observes live per-work changes");pg.per_work[6]=1;
    s.yield_cache[4]=-1;check(vp_cached_great_work_yield(c,city,4)==800,"Native minus-one sentinel causes cache recomputation");
    {
        const auto saved=s;const auto player_saved=pg;s.works.resize(1);s.theming_indices.clear();s.per_work.clear();s.per_local_theme.clear();s.tourism_modifier=-101;pg={};pg.per_work[6]=-1;
        invalidate_vp_great_work_yields(cities,city);check(vp_cached_great_work_yield(c,city,6)==-1,"Negative great-work yields are not clamped");
        pg.per_work[6]=0;check(vp_cached_great_work_yield(c,city,6)==-2,"Computed minus-one is a miss on the next query, as in the original");s=saved;pg=player_saved;
    }
    s.works.push_back({1,0,0});check(vp_great_works_in_building(c,city,1)==3&&vp_great_works_in_slot(c,city,7,false,true)==2,"Duplicate slots and work identities are preserved in source counts");s.works.pop_back();
    rules.buildings[10].slot=8;check(vp_great_works_in_slot(c,city,7,false,true)==1&&vp_great_works_in_slot(c,city,8)==2,"Art class count ignores requested slot identity after the NO_SLOT guard");
    check(vp_great_works_in_slot(c,city,-1,true)==0&&vp_great_works_in_slot(c,city,7,true,true)==1,"NO_SLOT returns zero and artifact takes precedence over art");
    rules.buildings[10].slot=7;
    for(std::size_t i=2;i<6;++i){rules.buildings[10].slot=rules.slots[i-1];pg.type_bonus[i][12]=static_cast<std::int64_t>(i+1);}
    options.entries["BALANCE_CORE_JFD"].value=1;q=vp_great_work_yield(c,city,12);check(q.type_bonus==12&&q.total_times100==1200,"Relic slot counts use player bonuses without trait film/relic fields");
    for(std::size_t i=2;i<5;++i){rules.buildings[10].slot=rules.slots[i-1];if(i<4)pg.trait_type_bonus[i][12]=1;q=vp_great_work_yield(c,city,12);check(q.type_bonus==2*(static_cast<std::int64_t>(i+1)+(i<4?1:0)),"Literature, music and film select their own yield effects");}
    rules.buildings[10].slot=7;options.entries["BALANCE_CORE_JFD"].value=0;check(vp_great_work_yield(c,city,12).suppressed,"JFD option gates errata yields");
    // Different buildings of the same class count their works separately, but
    // the first eligible entry determines the class lookup and its theme.
    rules.buildings[11]=rules.buildings[10];rules.building_slots=12;city.real_buildings[11]=1;s.existing_buildings={11,10};s.theming_indices[11]=1;
    check(vp_building_from_class(c,city,1)==10,"Class lookup cache retains its original selection");invalidate_vp_building_class_cache(cities,city);
    check(vp_building_from_class(c,city,1)==11&&vp_theming_bonus(c,city,1)==3,"Dirty class cache selects first eligible live building in list order");
    pg.theming_policy_modifier=25;pg.capital_theming_modifier=50;
    check(vp_theming_multiplier(c,city)==18750&&vp_current_theming_bonuses(c,city,4)==8,"Theme multiplier rounds each building separately before summing");
    p.capital=-1;check(vp_theming_multiplier(c,city)==12500,"Capital trait multiplier applies only to current capital");p.capital=42;
    pg.theming_policy_modifier=0;pg.capital_theming_modifier=700;options.entries["ENABLE_ACHIEVEMENTS"].value=1;
    check(vp_theming_bonus(c,city,1)==24&&services.awards==std::vector<std::int64_t>{5},"Eligible capital theme requests the original achievement through a service");
    for(auto game_type:{1,2,3}){setup.game_type=game_type;vp_theming_bonus(c,city,1);}setup.game_type=0;setup.players[5].slot_status=1;vp_theming_bonus(c,city,1);
    check(services.awards.size()==1,"Multiplayer and AI players do not request theming achievements");setup.players[5].slot_status=3;options.entries["ENABLE_ACHIEVEMENTS"].value=0;
    options.entries["BUILDINGS_THOROUGH_PREREQUISITES"].value=0;check(vp_building_from_class(c,city,1)==10,"Legacy class lookup uses civilization replacement");options.entries["BUILDINGS_THOROUGH_PREREQUISITES"].value=1;
    s.existing_buildings.clear();invalidate_vp_building_class_cache(cities,city);check(vp_building_from_class(c,city,1)==-1&&vp_building_from_class(c,city,1,true)==10,"Fallback applies only when explicitly requested");
    s.existing_buildings={10};s.theming_indices.clear();pg.capital_theming_modifier=0;invalidate_vp_building_class_cache(cities,city);
    CityYieldReligionBeliefs r;r.religion=2;r.founder=5;r.beliefs={1,1};ensure_vp_religion_beliefs(r,vp);vp_religion_beliefs(r).belief_religion=2;vp_religion_beliefs(r).holy_x=-1;vp_religion_beliefs(r).holy_y=-1;cities.religions={r};vp_city_majority_religion(city)=2;
    q=vp_great_work_yield(c,city,4);check(q.beliefs==4&&q.total_times100==1400,"Majority great-work yields preserve ordered duplicate beliefs");
    city.population=4;q=vp_great_work_yield(c,city,4);check(q.beliefs==0&&q.total_times100==600,"Population threshold gates majority great-work belief yields");
    city.population=8;rr.beliefs[1].founder=true;check(vp_great_work_yield(c,city,4).beliefs==0,"Majority beliefs use live ownership validity");rr.beliefs[1].founder=false;
    auto pantheon=r;pantheon.religion=0;vp_religion_beliefs(pantheon).belief_religion=0;vp_religion_beliefs(pantheon).pantheon=true;pantheon.beliefs={1};cities.religions.push_back(pantheon);rr.beliefs[1].pantheon=true;
    city.population=1;p.second_pantheon_count=1;city.religions={{2,1,10},{0,1,1}};
    check(vp_great_work_yield(c,city,4).secondary==2,"Secondary pantheon adds raw great-work bonus below minimum population");
    options.entries["BALANCE_PERMANENT_PANTHEONS"].value=1;vp_player_state_religion(p)=2;
    check(vp_great_work_yield(c,city,4).pantheon==0,"Permanent pantheon does not double count selected secondary belief");
    p.second_pantheon_count=0;vp_city_majority_religion(city)=-1;check(vp_great_work_yield(c,city,4).pantheon==2,"Permanent pantheon adds raw selected belief when majority is absent");
    options.entries["BALANCE_PERMANENT_PANTHEONS"].value=0;vp_city_majority_religion(city)=-1;
    s.theming_indices[10]=15;rules.buildings[10].themes.resize(16,1);rules.buildings[10].themes[15]=9;
    check(vp_theming_bonus(c,city,1)==9,"Owned theme rules can exceed original twelve-entry capacity");
    s.theming_indices[10]=17;check(vp_theming_bonus(c,city,1)==0&&vp_themed_buildings(c,city)==1,"Unused theme entry returns zero while themed count depends on saved index");
    rules.buildings[10].themes[0]=0;rejects([&]{vp_theming_bonus(c,city,1);});rules.buildings[10].themes[0]=2;s.theming_indices.clear();
    city.free_buildings[10]=1;city.real_buildings[10]=-1;invalidate_vp_building_class_cache(cities,city);check(vp_building_from_class(c,city,1)==10,"Single-building mode uses max of real and free counts");
    db.apply_sql("UPDATE Defines SET Value=2 WHERE Name='CITY_MAX_NUM_BUILDINGS'");defines=db.cache_global_defines(GlobalDefineProfile::vp542);invalidate_vp_building_class_cache(cities,city);
    check(vp_building_from_class(c,city,1)==-1,"Multiple-building mode sums real and free counts before class eligibility");city.real_buildings[10]=1;
    constexpr std::int64_t wide=4294967300LL;pg.per_work[4]=wide;invalidate_vp_building_class_cache(cities,city);
    check(vp_great_work_yield(c,city,4).total_times100==wide*200+600,"Great-work yield arithmetic exceeds uint32 without wrapping");
    auto node=rules.buildings.extract(10);node.key()=wide;node.mapped().building_class=wide+1;rules.buildings.insert(std::move(node));rules.class_slots=static_cast<std::uint64_t>(wide+2);
    s.existing_buildings={wide};city.real_buildings={{wide,1}};city.free_buildings.clear();s.works={{wide+1,wide+2,wide+3}};(*cities.great_works)[wide+3].class_type=2;invalidate_vp_building_class_cache(cities,city);
    check(vp_building_from_class(c,city,wide+1)==wide&&vp_great_works_in_slot(c,city,7,false,true)==1,"Work, slot, building and class identities support sparse 64-bit IDs");
    pg.per_work[4]=INT64_MAX;rejects([&]{vp_great_work_yield(c,city,4);});
    auto detached=cities;VpGreatWorkContext wrong{detached,cr,rules,religion,services};rejects([&]{vp_great_work_yield(wrong,detached.players.at(5).cities.at(42),4);});
    rejects([&]{vp_great_work_yield(c,city,999);});s.works.clear();s.theming_indices[wide]=0;
    check(vp_great_work_yield(c,city,4).total_times100==0,"No works returns zero before per-work arithmetic despite active theme");
}
