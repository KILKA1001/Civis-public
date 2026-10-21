#pragma once
void improvement_yield_tests(){
    using namespace civ5::runtime;
    check(YieldFraction(0,17)==YieldFraction()&&YieldFraction(2,-4)==YieldFraction(-1,2),"Yield fractions normalize zero and denominator signs");
    rejects([]{YieldFraction(1,0);});
    auto fraction=YieldFraction(INT64_MAX,2);fraction+=YieldFraction(-INT64_MAX,3);
    check(fraction==YieldFraction(INT64_MAX,6),"Wide fractional products may cancel to a representable rational");
    fraction=YieldFraction(INT64_MIN,2);fraction+=YieldFraction(INT64_MIN,2);
    check(fraction==YieldFraction(INT64_MIN),"Exact negative INT64 boundary must remain representable");
    rejects([&]{fraction+=YieldFraction(-1);});
    check(fraction==YieldFraction(INT64_MIN),"Fraction overflow must leave the previous value intact");
    check(YieldFraction(INT64_MIN,INT64_MIN)==YieldFraction(1),"Normalize INT64_MIN without signed negation overflow");
    rejects([]{YieldFraction(1,INT64_MIN);});
    for(std::int64_t a=-9;a<=9;++a)for(std::int64_t b=-9;b<=9;++b)
        for(std::int64_t d=1;d<=9;++d)for(std::int64_t e=1;e<=9;++e){
            auto value=YieldFraction(a,d);value+=YieldFraction(b,e);
            check(value.truncate()==(a*e+b*d)/(d*e),"Fraction sum must truncate once toward zero");
        }
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Yields(ID,Type);INSERT INTO Yields VALUES(0,'FOOD'),(4,'CULTURE'),(4294967300,'WIDE_YIELD');"
        "CREATE TABLE Improvements(ID,Type,CultureAdjacentSameType);INSERT INTO Improvements VALUES(0,'FARM',2),(1,'OTHER',0),(4294967300,'WIDE',0),(4294967300,'DUP',3);"
        "CREATE TABLE Resources(ID,Type);INSERT INTO Resources VALUES(0,'WHEAT'),(1,'ORPHAN');"
        "CREATE TABLE Technologies(ID,Type);INSERT INTO Technologies VALUES(4294967300,'TECH');"
        "CREATE TABLE Routes(ID,Type);INSERT INTO Routes VALUES(4294967300,'ROAD');"
        "CREATE TABLE Policies(ID,Type);INSERT INTO Policies VALUES(4294967300,'POLICY');"
        "CREATE TABLE Terrains(ID,Type);INSERT INTO Terrains VALUES(0,'GRASS'),(4294967300,'WIDE_TERRAIN');"
        "CREATE TABLE Features(ID,Type);INSERT INTO Features VALUES(0,'FOREST');"
        "CREATE TABLE Improvement_ResourceTypes(ImprovementType,ResourceType);INSERT INTO Improvement_ResourceTypes VALUES('FARM','WHEAT');"
        "CREATE TABLE Improvement_ResourceType_Yields(ImprovementType,ResourceType,YieldType,Yield);"
        "INSERT INTO Improvement_ResourceType_Yields VALUES('FARM','WHEAT','FOOD',3),('FARM','WHEAT','FOOD',-7),('FARM','ORPHAN','FOOD',20);"
        "CREATE TABLE Policy_ImprovementYieldChanges(PolicyType,ImprovementType,YieldType,Yield);"
        "INSERT INTO Policy_ImprovementYieldChanges VALUES('POLICY','FARM','FOOD',9),('POLICY','FARM','FOOD',12);"
        "CREATE TABLE Route_Yields(RouteType,YieldType,Yield);INSERT INTO Route_Yields VALUES('ROAD','WIDE_YIELD',4294967300);");
    for(const auto *table:{"Improvement_Yields","Improvement_YieldPerEra","Improvement_AdjacentCityYields","Improvement_AdjacentMountainYieldChanges",
        "Improvement_CoastalLandYields","Improvement_FreshWaterYields","Improvement_HillsYields","Improvement_RiverSideYields","Improvement_WLTKDYields","Improvement_GoldenAgeYields"})
        db.apply_sql(std::string("CREATE TABLE ")+table+"(ImprovementType,YieldType,Yield)");
    for(const auto &[table,column]:std::vector<std::pair<std::string,std::string>>{
        {"Improvement_TechYieldChanges","TechType"},{"Improvement_TechFreshWaterYieldChanges","TechType"},{"Improvement_TechNoFreshWaterYieldChanges","TechType"},
        {"Improvement_RouteYieldChanges","RouteType"},{"Improvement_FeatureYieldChanges","FeatureType"},{"Improvement_AdjacentFeatureYieldChanges","FeatureType"},
        {"Improvement_AdjacentResourceYieldChanges","ResourceType"},{"Improvement_AdjacentTerrainYieldChanges","TerrainType"}})
        db.apply_sql("CREATE TABLE "+table+"(ImprovementType,YieldType,Yield,"+column+")");
    db.apply_sql("INSERT INTO Improvement_Yields VALUES('FARM','FOOD',3),('FARM','FOOD',0),('WIDE','WIDE_YIELD',4294967300);"
        "INSERT INTO Improvement_TechYieldChanges VALUES('FARM','FOOD',4,'TECH');"
        "INSERT INTO Improvement_TechFreshWaterYieldChanges VALUES('FARM','FOOD',5,'TECH');"
        "INSERT INTO Improvement_TechNoFreshWaterYieldChanges VALUES('FARM','FOOD',6,'TECH');"
        "INSERT INTO Improvement_RouteYieldChanges VALUES('FARM','FOOD',7,'ROAD');"
        "INSERT INTO Improvement_AdjacentFeatureYieldChanges VALUES('FARM','FOOD',8,'FOREST');"
        "INSERT INTO Improvement_FeatureYieldChanges VALUES('FARM','FOOD',9,'FOREST');"
        "INSERT INTO Improvement_AdjacentResourceYieldChanges VALUES('FARM','FOOD',10,'WHEAT');"
        "INSERT INTO Improvement_AdjacentTerrainYieldChanges VALUES('FARM','FOOD',11,'GRASS');"
        "CREATE TABLE Improvement_YieldPerXAdjacentImprovement(ImprovementType,OtherImprovementType,YieldType,Yield,NumRequired);"
        "INSERT INTO Improvement_YieldPerXAdjacentImprovement VALUES('FARM','OTHER','FOOD',1,3),('FARM','OTHER','FOOD',1,6),('FARM','FARM','CULTURE',1,2),('FARM','WIDE','FOOD',-1,3);"
        "CREATE TABLE Improvement_YieldPerXAdjacentTerrain(ImprovementType,TerrainType,YieldType,Yield,NumRequired);"
        "INSERT INTO Improvement_YieldPerXAdjacentTerrain VALUES('FARM','GRASS','FOOD',1,12),('FARM','WIDE_TERRAIN','WIDE_YIELD',4294967300,2);");
    const auto types=db.cache_info_types();auto rules=load_improvement_yield_rules(db,TechnologyProfile::vp542);
    const auto &farm=rules.improvements.at(0);
    check(rules.improvement_slots==4294967302ULL&&rules.improvements.size()==4&&rules.improvements.at(4294967301LL).authored_id==4294967300LL,
        "Improvement yields must preserve sparse wide slots and duplicate authored IDs");
    check(rules.improvements.at(4294967301LL).fractional_improvements.at(4).at(4294967300LL)==YieldFraction(3),
        "Legacy culture adjacency uses authored ID, not the appended Info slot");
    check(farm.yields[0].empty()&&farm.resources.at(0).at(0)==-7&&!farm.resources.contains(1),
        "Integer relation rows overwrite including zero; orphan resource yields stay absent");
    check(farm.technologies.at(4294967300LL).at(0)==4&&farm.fresh_technologies.at(4294967300LL).at(0)==5&&farm.dry_technologies.at(4294967300LL).at(0)==6&&
        farm.routes.at(4294967300LL).at(0)==7&&rules.routes.at(4294967300LL).at(4294967300LL)==4294967300LL&&rules.policies.at(4294967300LL).at(0).at(0)==12,
        "Technology, route and policy yield relations use wide identities and values");
    check(farm.features.at(0).at(0)==9&&farm.adjacent_features.at(0).at(0)==8&&farm.adjacent_resources.at(0).at(0)==10&&farm.adjacent_terrains.at(0).at(0)==11,
        "VP feature and adjacent relation orientation must match source");
    check(farm.fractional_improvements.at(0).at(1)==YieldFraction(1,2)&&farm.fractional_improvements.at(4).at(0)==YieldFraction(5,2),
        "Fraction rows accumulate, including the legacy CultureAdjacentSameType contribution");
    civ5::saves::SavedMapProjection map{{5,5}};
    for(std::uint64_t id=0;id<25;++id){civ5::saves::SavedMapCellProjection p;p.index=id;p.x=id%5;p.y=id/5;
        p.terrain_type=0;p.improvement_type=-1;p.improvement_pillaged=false;map.cells.push_back(p);}
    map.cells[13].improvement_type=1;
    auto adjacent=improvement_fractional_adjacency(map,rules,types,12,0,0);
    check(adjacent.improvements==YieldFraction(1,2)&&adjacent.terrains==YieldFraction(1,2)&&adjacent.combined()==1,
        "Fractional improvements and terrain combine before truncating; separate truncation would lose a yield");
    map.cells[13].improvement_pillaged=true;
    check(improvement_fractional_adjacency(map,rules,types,12,0,0).combined()==0,"Live pillaging removes only improvement adjacency");
    map.cells[13].improvement_pillaged=false;map.cells[13].improvement_type.reset();map.cells[13].improvement_type_hash=saved_info_type_hash("OTHER");
    check(improvement_fractional_adjacency(map,rules,types,12,0,0).combined()==1,"Shared projection hash identities may supply an adjacent improvement");
    map.cells[13].improvement_type_hash.reset();map.cells[13].improvement_type=0;
    check(improvement_fractional_adjacency(map,rules,types,12,0,4).combined()==2,"Fractional legacy culture truncates only after summing");
    map.cells[13].terrain_type=4294967300LL;
    check(improvement_fractional_adjacency(map,rules,types,12,0,4294967300LL).combined()==2147483650LL,"Wide terrain/yield IDs must work on the common map");
    map.cells[13].improvement_pillaged.reset();rejects([&]{improvement_fractional_adjacency(map,rules,types,12,0,0);});
    // No VP-only schema should be needed by BNW.
    for(const auto *table:{"Route_Yields","Improvement_WLTKDYields","Improvement_GoldenAgeYields","Improvement_FeatureYieldChanges",
        "Improvement_AdjacentFeatureYieldChanges","Improvement_AdjacentResourceYieldChanges","Improvement_AdjacentTerrainYieldChanges",
        "Improvement_YieldPerXAdjacentImprovement","Improvement_YieldPerXAdjacentTerrain"})db.apply_sql(std::string("DROP TABLE ")+table);
    rules=load_improvement_yield_rules(db,TechnologyProfile::bnw);
    check(rules.improvements.at(0).fractional_improvements.empty()&&rules.improvements.at(0).resources.at(0).at(0)==-7,
        "BNW rules load without VP tables or VP legacy culture migration");
    rejects([&]{improvement_fractional_adjacency(map,rules,types,12,0,0);});
}
