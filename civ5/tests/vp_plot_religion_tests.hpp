#pragma once
void vp_plot_religion_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    constexpr auto vp=TechnologyProfile::vp542;constexpr auto layout=GameStateLayout::vp542_corpus;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);CREATE TABLE Beliefs(ID,Type,Pantheon,RequiresImprovement,CivilizationType);INSERT INTO Beliefs VALUES(0,'BELIEF',1,1,'CIV');"
        "CREATE TABLE Yields(ID,Type);INSERT INTO Yields VALUES(0,'FOOD');CREATE TABLE Terrains(ID,Type);INSERT INTO Terrains VALUES(0,'LAND');"
        "CREATE TABLE Plots(ID,Type);INSERT INTO Plots VALUES(2,'FLAT');CREATE TABLE Features(ID,Type,PseudoNaturalWonder);INSERT INTO Features VALUES(3,'WONDER',1);"
        "CREATE TABLE Resources(ID,Type);INSERT INTO Resources VALUES(4,'RESOURCE');CREATE TABLE Civilizations(ID,Type);INSERT INTO Civilizations VALUES(7,'CIV');"
        "CREATE TABLE Improvements(ID,Type,ImprovementResource,Cityside);INSERT INTO Improvements VALUES(0,'IMPROVEMENT','RESOURCE',1);");
    for(const auto &[suffix,key]:std::vector<std::pair<std::string,std::string>>{{"TerrainYieldChanges","TerrainType"},{"PlotYieldChanges","PlotType"},{"FeatureYieldChanges","FeatureType"},{"UnimprovedFeatureYieldChanges","FeatureType"},{"ResourceYieldChanges","ResourceType"},{"ImprovementYieldChanges","ImprovementType"}})
        db.apply_sql("CREATE TABLE Belief_"+suffix+"(BeliefType,"+key+",YieldType,Yield);");
    db.apply_sql("CREATE TABLE Belief_LakePlotYield(BeliefType,YieldType,Yield);CREATE TABLE Belief_YieldChangeNaturalWonder(BeliefType,YieldType,Yield);"
        "INSERT INTO Belief_TerrainYieldChanges VALUES('BELIEF','LAND','FOOD',9),('BELIEF','LAND','FOOD',0);INSERT INTO Belief_LakePlotYield VALUES('BELIEF','FOOD',7),('BELIEF','FOOD',11);"
        "CREATE TABLE Improvement_ResourceTypes(ImprovementType,ResourceType,ResourceTrade,ResourceMakesValid);INSERT INTO Improvement_ResourceTypes VALUES('IMPROVEMENT','RESOURCE',1,0),('IMPROVEMENT','RESOURCE',0,1);");
    const auto types=db.cache_info_types();const auto loaded=load_vp_plot_religion_rules(db,types);
    check(loaded.belief_slots==1&&loaded.resource_slots==5&&loaded.beliefs.at(0).civilization==7&&loaded.beliefs.at(0).terrain.at(0).empty()&&loaded.beliefs.at(0).lake.at(0)==11&&loaded.natural_wonders.contains(3),"Religion rules use active Info names, zero-overwrite and last duplicate lake value");
    check(loaded.improvements.at(0).trades.empty()&&loaded.improvements.at(0).valid_resources.contains(4)&&loaded.improvements.at(0).created_resource==4&&loaded.improvements.at(0).cityside,"Religious improvements distinguish trade, validity, created resource and Cityside");
    VpPlotReligionRules rules;rules.yields={0};rules.resource_slots=5;rules.belief_slots=10;rules.improvements[0]={};rules.natural_wonders={3};
    auto &b=rules.beliefs[0];b.terrain[0][0]=2;b.plot[2][0]=3;b.lake[0]=5;b.feature[3][0]=7;b.unimproved_feature[3][0]=11;b.wonder[0]=13;b.resources[4][0]=17;b.improvements[0][0]=19;
    auto &founder=rules.beliefs[1];founder.founder=true;founder.terrain[0][0]=23;founder.improvements[0][0]=67;
    auto &secondary=rules.beliefs[2];secondary.pantheon=true;secondary.terrain[0][0]=29;secondary.plot[2][0]=31;secondary.lake[0]=37;secondary.unimproved_feature[3][0]=41;secondary.feature[3][0]=43;secondary.wonder[0]=47;secondary.resources[4][0]=53;secondary.improvements[0][0]=59;
    rules.beliefs[9]={};
    CityYieldReligionBeliefs plain;
    check(!plain.extensions.find<VpReligionBeliefsProfileState>(),
          "Shared religion records must not require VP saved fields");
    rejects([&]{ensure_vp_religion_beliefs(plain,TechnologyProfile::bnw);});
    rejects([&]{vp_religion_beliefs(plain);});
    CityYieldState plain_city;
    check(!plain_city.extensions.find<VpCityReligionProfileState>() &&
          !vp_city_majority_religion(std::as_const(plain_city)),
          "Shared city records need no VP majority-religion cache");
    CityYieldPlayer plain_player;
    check(!plain_player.extensions.find<VpPlayerReligionProfileState>() &&
          !vp_player_state_religion(std::as_const(plain_player)),
          "Shared player records need no VP state-religion cache");
    CityYieldContext cities;cities.profile=vp;cities.yield_slots=1;cities.players[5].civilization=7;vp_player_state_religion(cities.players[5])=1;cities.players[5].second_pantheon_count=1;vp_player_state_religion(cities.players[6])=1;
    auto copied_player=cities.players[5];vp_player_state_religion(copied_player)=2;
    check(vp_player_state_religion(cities.players[5])==1 && vp_player_state_religion(copied_player)==2,
          "Copied VP player religion state is independent of its source");
    auto &city=cities.players[5].cities[44];city.owner=5;city.id=44;vp_city_majority_religion(city)=1;city.religions={{99,100,1000},{2,0,2},{1,0,9999}};
    CityYieldReligionBeliefs r;r.religion=1;r.founder=5;r.beliefs={0,0,1};ensure_vp_religion_beliefs(r,vp);vp_religion_beliefs(r).belief_religion=1;vp_religion_beliefs(r).holy_x=0;vp_religion_beliefs(r).holy_y=0;
    auto copied_religion=r;vp_religion_beliefs(copied_religion).lookup[9]=1;
    check(vp_religion_beliefs(r).lookup.empty()&&vp_religion_beliefs(copied_religion).lookup.at(9)==1,
          "Copied VP religion extension must be independent of its source");
    auto s=r;s.religion=2;s.beliefs={2,0};vp_religion_beliefs(s).belief_religion=2;auto pantheon=r;pantheon.religion=0;vp_religion_beliefs(pantheon).pantheon=true;vp_religion_beliefs(pantheon).holy_x=vp_religion_beliefs(pantheon).holy_y=-1;
    cities.religions={r,s,pantheon};auto &major=cities.religions[0];
    SavedMapProjection map{{2,1}};for(std::uint64_t id=0;id<2;++id){SavedMapCellProjection p;p.index=id;p.x=id;p.owner=5;p.plot_type=2;p.terrain_type=0;p.feature_type=3;p.water_cache.emplace();p.water_cache->lake=true;map.cells.push_back(p);}
    PlotWaterState ws;ws.layout=layout;PlotWaterRules wr;wr.profile=vp;PlotPassability pass;pass.layout=layout;pass.plots.resize(2);ResearchContext players;players.profile=vp;const auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);CustomModOptions options;
    PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,&options};VpPlotReligionContext context{rules,water};
    auto selection=vp_plot_religion_selection(cities,rules,city);
    check(selection.majority==&major&&selection.secondary==2,"VP selection uses saved majority and second saved pressure row, not follower totals or sorting");
    const auto query=[&](std::int64_t improvement=-1,std::int64_t feature=3,std::int64_t resource=4,std::int64_t player=5){return vp_religion_nature_yield(context,1,0,player,improvement,feature,resource,&city,selection);};
    check(query().total()==420&&query().terrain==56&&query().unimproved_feature==63,"Religion nature combines duplicated majority entries and direct secondary bonuses");
    check(query(0).total()==357&&query(-1,-1).total()==227,"Improvement removes unimproved feature; hypothetical NO_FEATURE suppresses feature/wonder only");
    map.cells[1].feature_type=-1;check(query().wonder==0&&query().total()==347,"Wonder test uses actual feature while yield feature is hypothetical");map.cells[1].feature_type=3;
    check(query(-1,3,4,-1).total()==0&&vp_religion_nature_yield(context,1,0,5,-1,3,4,&city,{nullptr,2}).total()==0,"NO_PLAYER or absent majority suppresses secondary too");
    b.improvement=true;check(query().terrain==29&&query(0,-1,-1).terrain==56,"Majority improvement-only terrain requirement does not require a connected resource");
    secondary.improvement=true;check(query(0,-1,-1).terrain==27,"Secondary improvement-only requirement additionally checks resource connection");
    rules.improvements[0].connects_all=true;check(query(0).terrain==56&&query(0,3,5).terrain==27,"ConnectsAllResources still requires an in-span resource identity");
    b.resource=true;rules.improvements[0].connects_all=false;check(query(0).terrain==0,"Both requirements need improvement resource trade or ConnectsAllResources");rules.improvements[0].trades.insert(4);check(query(0).terrain==56,"ResourceTrade connects resource independently of ResourceMakesValid");
    b.improvement=b.resource=secondary.improvement=false;b.no_feature=true;check(query().terrain==56,"No-feature-only source condition does not filter terrain bonus");b.no_feature=false;
    b.no_improvement=true;check(query(0).terrain==29,"No-improvement restriction applies to majority terrain aggregate");
    b.terrain[0][0]=-20;check(query(0).terrain==12,"Rejected negative majority aggregate survives source min cap");b.terrain[0][0]=2;b.no_improvement=false;
    auto &flags=rules.beliefs[9];flags.founder=true;
    check(vp_plot_belief_valid(context,9,1,5,&city)&&!vp_plot_belief_valid(context,9,1,6,&city),"Founder benefits follow holy plot owner and city owner");
    map.cells[0].owner=6;check(!vp_plot_belief_valid(context,9,1,5)&&vp_plot_belief_valid(context,9,1,6),"Conquered holy plot changes eligibility without changing founder");map.cells[0].owner=5;
    flags.founder=false;flags.enhancer=true;check(!vp_plot_belief_valid(context,9,1,6),"Enhancer also requires holy plot control");flags.enhancer=false;flags.follower=true;
    check(!vp_plot_belief_valid(context,9,2,5,&city)&&!vp_plot_belief_valid(context,9,1,6,&city)&&vp_plot_belief_valid(context,9,2,5),"Follower checks saved city majority and owner only when a city was supplied");flags.follower=false;flags.reformation=true;
    check(vp_plot_belief_valid(context,9,1,6,&city),"Reformation permits another owner's city when majority/state religion match");vp_player_state_religion(cities.players[6])=2;check(!vp_plot_belief_valid(context,9,1,6,&city),"Reformation without holy control requires saved state religion");flags.reformation=false;flags.pantheon=true;
    vp_city_majority_religion(city)=2;vp_religion_beliefs(major).lookup[9]=vp_religion_beliefs(cities.religions[2]).lookup[9]=1;
    check(vp_plot_belief_valid(context,9,1,5,&city),"Pantheon exception reads independent saved lookup, even if ordered list omits belief");vp_religion_beliefs(major).lookup[9]=2;check(!vp_plot_belief_valid(context,9,1,5,&city),"HasBelief accepts exactly one, not arbitrary nonzero");vp_religion_beliefs(major).lookup[9]=1;vp_city_majority_religion(city)=1;
    flags.civilization=8;options.entries["BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV"].value=1;check(!vp_plot_belief_valid(context,9,1,5,&city)&&vp_plot_belief_valid(context,999,1,-1),"Civilization restriction is active, NO_PLAYER exits before belief lookup");flags.civilization=7;
    check(vp_plot_belief_valid(context,9,1,5,&city),"Required civilization matches live player identity");
    check(vp_religion_improvement_yield(context,0,5,0,4,&city,selection)==164,"Religious improvement adds duplicated majority and secondary");
    founder.resource=secondary.resource=true;
    check(vp_religion_improvement_yield(context,0,6,0,4,&city,selection)==0,"Improvement uses city-owner requirements and ResourceMakesValid, not ResourceTrade");
    rules.improvements[0].ignore_ownership=true;check(vp_religion_improvement_yield(context,0,6,0,-1,&city,selection)==105,"IgnoreOwnership waives resource only for majority");
    rules.improvements[0].cityside=true;check(vp_religion_improvement_yield(context,0,6,0,-1,&city,selection)==164,"Cityside also waives secondary requirement");rules.improvements[0].cityside=rules.improvements[0].ignore_ownership=false;
    rules.improvements[0].created_resource=4;check(vp_religion_improvement_yield(context,0,5,0,4,&city,selection)==164,"Created resource satisfies both improvement branches");
    rules.improvements[0].created_resource=-1;rules.improvements[0].valid_resources.insert(4);check(vp_religion_improvement_yield(context,0,5,0,4,&city,selection)==164,"ResourceMakesValid satisfies religious improvement");
    b.improvements[0][0]=-100;check(vp_religion_improvement_yield(context,0,5,0,4,&city,selection)==-74,"Religious improvement preserves negative result");
    founder.resource=secondary.resource=false;b.terrain[0][0]=-1000;check(query().total()<0,"Religious nature is not clamped to zero");
    constexpr std::int64_t wide=4294967300LL;b.terrain[0][0]=wide;check(query().terrain==2*wide+52,"Belief yields and duplicate sums stay wider than int32");
    b.terrain[0][0]=INT64_MAX;rejects([&]{query();});b.terrain[0][0]=2;rules.beliefs[wide]=b;major.beliefs={wide};check(query().terrain==31,"Wide belief ID uses sparse rules");major.beliefs={0,0,1};
    auto copy=city;rejects([&]{vp_plot_religion_selection(cities,rules,copy);});vp_city_majority_religion(city).reset();
    check(vp_city_majority_religion(copy)==1 && !vp_city_majority_religion(city),
          "Copied VP city religion state is independent of its source");
    rejects([&]{vp_plot_religion_selection(cities,rules,city);});

    SavedSessionSnapshot snapshot;snapshot.all_players.emplace();snapshot.all_players->layout=snapshot.religion_culture.layout=layout;
    ReligionObservation saved_religion;saved_religion.religion=wide;saved_religion.founder=wide+1;saved_religion.holy_city_x=wide+2;saved_religion.holy_city_y=wide+3;saved_religion.pantheon=true;
    saved_religion.beliefs.religion=wide+4;saved_religion.beliefs.belief_ids={0,0};saved_religion.beliefs.belief_lookup={2,1};snapshot.religion_culture.religions={saved_religion};
    SavedPlayerObservation saved;saved.saved_player_id=wide+1;saved.core.layout=saved.knowledge.layout=saved.cities.layout=layout;
    PlayerCoreField counter{"m_iSecondReligionPantheonCount",PlayerCoreEncoding::i32};counter.values={wide+5};saved.core.fields={counter};
    PlayerKnowledgeBlock kb{"player_religions"};kb.fields={{"state_religion",wide+6}};saved.knowledge.blocks={kb};
    saved.cities.cities.emplace_back();auto &sc=saved.cities.cities[0];sc.core.layout=sc.tail.layout=layout;sc.core.start_offset=wide+10;sc.core.end_offset=wide+20;
    CityCoreField cid{"m_iID",CityCoreEncoding::i32},owner{"m_eOwner",CityCoreEncoding::i32};cid.values={wide+7};owner.values={wide+1};sc.core.fields={cid,owner};
    CityTailNode rel{"religions","fields"},majority{"m_eMajorityCityReligion","scalar"},status{"m_ReligionStatus","rows"},row{"religion","fields"};
    majority.values={wide+8};CityTailNode rid{"m_eReligion","scalar"},followers{"m_iFollowers","scalar"},pressure{"m_iPressure","scalar"};rid.values={wide};followers.values={wide+9};pressure.values={wide+10};row.children={rid,followers,pressure};status.children={row,row};rel.children={status,majority};sc.tail.blocks={rel};snapshot.all_players->players={saved};
    CityYieldContext imported;imported.profile=vp;auto &ip=imported.players[wide+1];auto &ic=ip.cities[wide+7];ic.id=wide+7;ic.owner=wide+1;ic.source_core={{wide+10,wide+20}};VpPlotReligionRules ir;ir.belief_slots=2;ir.beliefs[0]={};
    restore_vp_plot_religion(snapshot,ir,imported);
    check(vp_player_state_religion(ip)==wide+6&&ip.second_pantheon_count==wide+5&&vp_city_majority_religion(ic)==wide+8&&ic.religions.size()==2&&ic.religions[0].followers==wide+9,"VP religion restores wide player/city caches and ordered duplicate status rows without recomputing majority");
    const auto &rr=imported.religions[0];check(rr.religion==wide&&vp_religion_beliefs(rr).belief_religion==wide+4&&vp_religion_beliefs(rr).holy_x==wide+2&&rr.beliefs==std::vector<std::int64_t>{0,0}&&vp_religion_beliefs(rr).lookup.at(0)==2&&vp_religion_beliefs(rr).lookup.at(1)==1,"Religion import preserves distinct identity, lookup values and duplicate ordered beliefs");
    auto broken=snapshot;broken.religion_culture.religions[0].beliefs.belief_lookup.pop_back();rejects([&]{restore_vp_plot_religion(broken,ir,imported);});
    broken=snapshot;broken.religion_culture.religions[0].beliefs.belief_ids.push_back(99);rejects([&]{restore_vp_plot_religion(broken,ir,imported);});
    broken=snapshot;broken.all_players->players[0].cities.cities[0].core.start_offset++;rejects([&]{restore_vp_plot_religion(broken,ir,imported);});
    broken=snapshot;broken.all_players->players[0].knowledge.blocks[0].fields.clear();rejects([&]{restore_vp_plot_religion(broken,ir,imported);});
    broken=snapshot;broken.all_players->players.push_back(saved);rejects([&]{restore_vp_plot_religion(broken,ir,imported);});
}
