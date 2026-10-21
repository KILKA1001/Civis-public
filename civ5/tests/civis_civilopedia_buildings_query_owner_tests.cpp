#include "civ5/ui/civis_civilopedia_units_query_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <atomic>
#include <fstream>
#include <iostream>
#include <set>
#include <thread>
using namespace civ5;using namespace civ5::ui;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F call,const char *why){bool rejected=false;try{call();}catch(const std::exception &){rejected=true;}check(rejected,why);}
std::string read(const std::filesystem::path &path){std::ifstream file(path,std::ios::binary);check(bool(file),"Original Civilopedia unavailable");return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};}
void fixture(content::RulesDatabase &db){
    db.apply_sql("CREATE TABLE Technologies(Type TEXT,Era TEXT);INSERT INTO Technologies VALUES('A','ERA_ANCIENT'),('M','ERA_MEDIEVAL');"
        "CREATE TABLE BuildingClasses(Type TEXT,MaxGlobalInstances INTEGER,MaxPlayerInstances INTEGER,MaxTeamInstances INTEGER);"
        "INSERT INTO BuildingClasses VALUES('NORMAL',-1,-1,-1),('WORLD',1,-1,-1),('ZERO_GLOBAL',0,-1,-1),('NATIONAL',-1,1,-1),('TEAM',-1,-1,1);"
        "CREATE TABLE Buildings(ID INTEGER,Description TEXT,PortraitIndex INTEGER,IconAtlas TEXT,BuildingClass TEXT,PrereqTech TEXT,FaithCost INTEGER,Cost INTEGER,SpecialistCount INTEGER);"
        "INSERT INTO Buildings VALUES(5000000101,'TXT_NORMAL',3,'ATLAS_A','NORMAL','A',0,40,0),"
        "(102,'TXT_PAID_FAITH',4,'ATLAS_B','NORMAL','A',100,60,0),(103,'TXT_FAITH_ONLY',5,'ATLAS_C','NORMAL','A',100,-1,0),"
        "(104,'TXT_WORLD',6,'ATLAS_D','WORLD','A',0,80,0),(105,'TXT_ZERO_GLOBAL',7,'ATLAS_E','ZERO_GLOBAL','A',0,80,0),"
        "(106,'TXT_NATIONAL',8,'ATLAS_F','NATIONAL','A',0,70,0),(107,'TXT_NATIONAL_SPECIALIST',9,'ATLAS_G','NATIONAL','A',0,70,1),"
        "(108,'TXT_TEAM',10,'ATLAS_H','TEAM','A',0,90,0),(109,'TXT_MEDIEVAL',11,'ATLAS_I','NORMAL','M',0,100,0),"
        "(110,'TXT_NO_TECH',12,'ATLAS_J','NORMAL',NULL,0,50,0),(111,'TXT_MISSING_TECH',13,'ATLAS_K','NORMAL','MISSING',0,50,0),"
        "(112,'TXT_ZERO_FAITH_NEGATIVE_COST',14,'ATLAS_L','NORMAL','A',0,-1,0);"
        "CREATE TABLE Units(ID INTEGER,Description TEXT,PortraitIndex INTEGER,IconAtlas TEXT,PreReqTech TEXT,FaithCost INTEGER,RequiresFaithPurchaseEnabled boolean,Cost INTEGER,ShowInPedia boolean);"
        "INSERT INTO Units VALUES(1,'TXT_UNIT_A',0,'UNITS','A',0,0,30,1),(2,'TXT_UNIT_M',1,'UNITS','M',0,0,40,1);");
}
auto era(std::string value){return std::array<content::DatabaseScriptValue,1>{std::move(value)};}
std::int64_t id(const content::DatabaseScriptRow &row){
    check(row.size()==4&&row[0].first=="ID"&&row[1].first=="Description"&&row[2].first=="PortraitIndex"&&row[3].first=="IconAtlas","Actual Buildings projection must retain four owned fields in source order");
    return std::get<std::int64_t>(row[0].second);
}
std::set<std::int64_t> ids(const std::shared_ptr<CivisCivilopediaUnitsIterator> &iter){
    std::set<std::int64_t> result;while(const auto row=iter->next())result.insert(id(*row));return result;
}
void edit_reject(const std::string &source,const std::string &before,const std::string &after){
    auto next=source;const auto start=next.find("CivilopediaCategory[CategoryBuildings].PopulateList = function()");
    const auto at=next.find(before,start);check(at!=next.npos,"Installed mutation anchor changed");next.replace(at,before.size(),after);
    rejects([&]{CivisCivilopediaUnitsQueryOwner::declared_buildings_sql_from_source(next);},"Changed Buildings declaration/call must reject");
}
int run(int argc,char **argv){
    check(argc==2,"Expected original gameRoot");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);check(sources.verified,"Original selected sources unavailable");
    const auto source=read(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const auto units=CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(source);
    const auto buildings=CivisCivilopediaUnitsQueryOwner::declared_buildings_sql_from_source(source);
    check(runtime::identify_bytes({reinterpret_cast<const std::uint8_t*>(buildings.data()),buildings.size()}).sha256=="38a0744c32d5ea946906ee7120c3e87eeaaa14a853ea5537f13b2dc35ee4ae76","Decoded installed Buildings SQL identity changed");
    std::string lf;for(const auto c:source)if(c!='\r')lf+=c;
    std::string lfcr;for(const auto c:lf){lfcr+=c;if(c=='\n')lfcr+='\r';}
    check(CivisCivilopediaUnitsQueryOwner::declared_buildings_sql_from_source(lf)==buildings&&CivisCivilopediaUnitsQueryOwner::declared_buildings_sql_from_source(lfcr)==buildings,"Shared Lua long-string newline decoder must preserve Buildings identity");
    rejects([&]{CivisCivilopediaUnitsQueryOwner::declared_buildings_sql_from_source("");},"Missing block must reject");
    rejects([&]{CivisCivilopediaUnitsQueryOwner::declared_buildings_sql_from_source(source+source);},"Duplicate source block must reject");
    edit_reject(source,"local BuildingsByEra = DB.CreateQuery(sql);","local BuildingsByEra = DB.Query(sql);");
    edit_reject(source,"local BuildingsByEra = DB.CreateQuery(sql);","local BuildingsByEra = DB.CreateQuery(sql); DB.CreateQuery(sql);");
    edit_reject(source,"local BuildingsByEra = DB.CreateQuery(sql);","local sql = [[other]]; local BuildingsByEra = DB.CreateQuery(sql);");
    edit_reject(source,"for building in BuildingsByEra(era.Type) do","for building in BuildingsByEra(era.ID) do");
    edit_reject(source,"Technologies.Era = ?;","Technologies.Era = ?; DELETE FROM Buildings;");
    edit_reject(source,"INNER JOIN BuildingClasses ON","INNER  JOIN BuildingClasses ON");
    const auto begin=source.find("CivilopediaCategory[CategoryBuildings].PopulateList = function()");
    const auto second=source.find("local sql = [[",source.find("local BuildingsByEra = DB.CreateQuery(sql);",begin));
    check(second!=source.npos,"Original second no-tech DB.Query declaration missing");
    auto changed_no_tech=source;changed_no_tech.replace(second+std::string_view("local sql = [[").size(),1," ");
    check(CivisCivilopediaUnitsQueryOwner::declared_buildings_sql_from_source(changed_no_tech)==buildings,"Unrelated later DB.Query must not be mistaken for admitted prepared statement");
    CivilopediaLayoutOwnerKey key;key.frame_number=31;key.owner_token="Civis:buildings-query-fixture";key.player_id=0;key.game_turn=123;
    key.payload_sha256=std::string(64,'a');key.rules_sha256=std::string(64,'b');key.civilopedia_sha256=sources.civilopedia_sha256;key.xml_sha256=sources.xml_sha256;key.styles_sha256=sources.styles_sha256;
    auto context=std::make_shared<CivisCivilopediaCallbackOwner>(key);
    content::RulesDatabase rules;fixture(rules);const auto source_fingerprint=rules.gameplay_fingerprint_data();
    auto legacy=CivisCivilopediaUnitsQueryOwner::open(root,sources,rules,key,context);
    rejects([&]{legacy->declared_buildings_sql();},"Buildings declaration cannot be enabled by legacy open");
    bool exact_legacy=false;try{legacy->prepare(buildings);}catch(const runtime::ScriptError &e){exact_legacy=std::string(e.what())=="Civis CreateQuery only allows the exact selected first Units statement";}
    check(exact_legacy&&legacy->declared_sql()==units,"Old Units-only boundary/error must remain intact");
    auto owner=CivisCivilopediaUnitsQueryOwner::open_with_buildings(root,sources,rules,key,context);
    check(owner->declared_sql()==units&&owner->declared_buildings_sql()==buildings,"Opt-in owner must retain exactly the two installed statements");
    for(const auto &bad:{buildings+" ",buildings+"DELETE FROM Buildings;",std::string("SELECT * FROM Buildings"),std::string("SELECT ID FROM Buildings WHERE PrereqTech IS NULL"),buildings+std::string(1,'\0')})
        rejects([&]{owner->prepare(bad);},"No normalization, third query, no-tech query or NUL may broaden allowlist");
    auto query=owner->prepare(buildings),units_query=owner->prepare(units);
    auto ancient=query->iterate(era("ERA_ANCIENT"));
    auto first=ancient->next();check(first.has_value(),"Admitted Ancient row missing");const auto owned_first=*first;
    auto ancient_ids=ids(ancient);ancient_ids.insert(id(owned_first));
    check(ancient_ids==std::set<std::int64_t>{5000000101LL,102,107,112},"Actual Buildings faith/cost/global/player/specialist/team/tech predicates differ");
    check(!ancient->next()&&!ancient->next(),"Buildings EOF must be sticky");
    auto medieval=query->iterate(era("ERA_MEDIEVAL"));rejects([&]{ancient->next();},"Old era iterator must invalidate even after EOF");
    check(ids(medieval)==std::set<std::int64_t>{109},"Buildings era binding must reset/rebind actual cursor");
    auto empty=query->iterate(era("ERA_EMPTY"));check(!empty->next()&&!empty->next(),"Unknown era must produce real EOF");
    check(!query->iterate(era(""))->next(),"Empty era is a string parameter producing EOF");
    auto old=query->iterate(era("ERA_ANCIENT"));check(old->next().has_value(),"Partial iteration fixture missing");
    auto replacement=query->iterate(era("ERA_MEDIEVAL"));rejects([&]{old->next();},"Partially consumed old era must invalidate");
    for(const auto &bad:std::array<content::DatabaseScriptValue,5>{std::monostate{},true,std::int64_t{1},1.0,std::string("ERA\0A",5)}){
        const std::array<content::DatabaseScriptValue,1> args{bad};rejects([&]{query->iterate(args);},"Buildings binding must reject undeclared type/NUL");
    }
    rejects([&]{query->iterate({});},"Zero argument Buildings call must reject");
    const std::array<content::DatabaseScriptValue,2> extra{std::string("ERA_ANCIENT"),std::string("ERA_MEDIEVAL")};
    rejects([&]{query->iterate(extra);},"Extra arguments must reject");check(id(*replacement->next())==109,"Rejected binding must not reset valid generation");
    auto independent=owner->prepare(buildings);
    auto a=query->iterate(era("ERA_ANCIENT")),b=independent->iterate(era("ERA_MEDIEVAL")),u=units_query->iterate(era("ERA_ANCIENT"));
    check(a->next().has_value()&&id(*b->next())==109&&id(*u->next())==1&&a->next().has_value(),"Units and independent Buildings factories must share clone but not cursor binding");
    auto only_units=CivisCivilopediaUnitsQueryOwner::open(root,sources,rules,key,context);
    rejects([&]{only_units->prepare(buildings);},"Expanded owner must not globally enable Buildings in another owner");
    check(!query->iterate(era("ERA_ANCIENT'; DELETE FROM Buildings;--"))->next(),"Copied string era must bind, never execute SQL");
    check(rules.gameplay_fingerprint_data()==source_fingerprint,"Diagnostic cursors must not mutate caller rules");
    rules.apply_sql("DELETE FROM Buildings;DELETE FROM Units;");
    check(ids(query->iterate(era("ERA_ANCIENT")))==ancient_ids&&id(*units_query->iterate(era("ERA_ANCIENT"))->next())==1,"Same private clone must preserve both statements after source DB mutation");
    check(id(owned_first)>0&&std::get<std::string>(owned_first[1].second).starts_with("TXT_"),"Returned Buildings row must own values after reset and source mutation");
    std::atomic<int> thread_rejections=0;auto threaded=query->iterate(era("ERA_ANCIENT"));
    std::thread other([&]{for(const auto &call:std::array<std::function<void()>,7>{[&]{owner->prepare(buildings);},[&]{owner->declared_sql();},[&]{owner->declared_buildings_sql();},[&]{query->iterate(era("ERA_ANCIENT"));},[&]{threaded->next();},[&]{query->close();},[&]{owner->close();}})
        try{call();}catch(const runtime::ScriptError &){++thread_rejections;}});other.join();check(thread_rejections==7&&threaded->next().has_value(),"Foreign thread must reject before mutation");
    auto changed=key;++changed.frame_number;context->observe(changed);
    rejects([&]{threaded->next();},"Stale iterator must reject");rejects([&]{owner->prepare(buildings);},"Stale Buildings factory must reject");
    rejects([&]{owner->declared_buildings_sql();},"Stale declared policy must reject");owner->close();owner->close();
    auto fresh=std::make_shared<CivisCivilopediaCallbackOwner>(key);content::RulesDatabase fresh_rules;fixture(fresh_rules);
    auto closed=CivisCivilopediaUnitsQueryOwner::open_with_buildings(root,sources,fresh_rules,key,fresh);
    auto closed_q=closed->prepare(buildings),closed_u=closed->prepare(units);auto ci=closed_q->iterate(era("ERA_ANCIENT")),ui=closed_u->iterate(era("ERA_ANCIENT"));closed->close();
    rejects([&]{ci->next();},"Owner close must release Buildings cursor");rejects([&]{ui->next();},"Same owner close must release Units cursor");
    auto keep=CivisCivilopediaUnitsQueryOwner::open_with_buildings(root,sources,fresh_rules,key,fresh);auto drop=keep->prepare(buildings);auto retained=drop->iterate(era("ERA_ANCIENT"));drop.reset();
    check(retained->next().has_value(),"Iterator must retain prepared cursor after factory drop");keep.reset();rejects([&]{retained->next();},"Dropped owner cannot leave active Buildings iterator");
    auto bad=sources;bad.civilopedia_sha256=std::string(64,'0');rejects([&]{CivisCivilopediaUnitsQueryOwner::open_with_buildings(root,bad,fresh_rules,key,fresh);},"Mismatched source identity must reject expanded policy");
    content::RulesDatabase missing;auto schema=CivisCivilopediaUnitsQueryOwner::open_with_buildings(root,sources,missing,key,fresh);rejects([&]{schema->prepare(buildings);},"Missing schema must be honest prepare failure");
    content::RulesDatabase overflow;fixture(overflow);overflow.apply_sql("DROP TABLE Buildings;CREATE VIEW Buildings AS SELECT abs(-9223372036854775808) AS ID,'TXT' AS Description,0 AS PortraitIndex,'ATLAS' AS IconAtlas,'NORMAL' AS BuildingClass,'A' AS PrereqTech,0 AS FaithCost,1 AS Cost,0 AS SpecialistCount;");
    auto errors=CivisCivilopediaUnitsQueryOwner::open_with_buildings(root,sources,overflow,key,fresh);auto lazy=errors->prepare(buildings);auto step=lazy->iterate(era("ERA_ANCIENT"));
    rejects([&]{step->next();},"Original projection overflow must fail only when lazy Step runs");rejects([&]{step->next();},"Failed Buildings cursor must remain failed");rejects([&]{lazy->iterate(era("ERA_ANCIENT"));},"Failed Buildings factory must not revive");
    std::cout<<"Civis exact Units+Buildings query policy: PASS (installed SQL738, shared clone, predicates, isolation, lazy cursor, binding and lifetime; legacy Units-only preserved)\n";return 0;
}
}
int main(int argc,char **argv){try{return run(argc,argv);}catch(const std::exception &e){std::cerr<<"Buildings query test failed: "<<e.what()<<'\n';return 1;}catch(...){std::cerr<<"Buildings query test unknown failure\n";return 1;}}
