#include "civ5/ui/civis_civilopedia_units_query_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <atomic>
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5;
using namespace civ5::ui;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F call){
    bool rejected=false;try{call();}catch(const std::exception &){rejected=true;}
    check(rejected,"Expected explicit Civis Units capability rejection");
}
std::string read(const std::filesystem::path &path){
    std::ifstream file(path,std::ios::binary);check(bool(file),"Installed source unavailable");
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
void fixture(content::RulesDatabase &db){
    db.apply_sql("CREATE TABLE Technologies(Type TEXT,Era TEXT);"
        "CREATE TABLE Units(ID INTEGER,Description TEXT,PortraitIndex INTEGER,IconAtlas TEXT,PreReqTech TEXT,FaithCost INTEGER,RequiresFaithPurchaseEnabled boolean,Cost INTEGER,ShowInPedia boolean);"
        "INSERT INTO Technologies VALUES('TECH_A','ERA_ANCIENT'),('TECH_B','ERA_MEDIEVAL');"
        "INSERT INTO Units VALUES(5000000001,'TXT_A',3,'ATLAS_A','TECH_A',0,0,30,1),"
        "(2,'TXT_B',4,'ATLAS_B','TECH_A',100,1,-1,1),(3,'TXT_C',5,'ATLAS_C','TECH_B',80,0,60,1),"
        "(4,'TXT_FAITH',6,'ATLAS_D','TECH_A',90,0,-1,1),(5,'TXT_HIDDEN',7,'ATLAS_E','TECH_A',0,0,30,0),"
        "(6,'TXT_NO_TECH',8,'ATLAS_F',NULL,0,0,30,1),(7,'TXT_UNKNOWN',9,'ATLAS_G','MISSING',0,0,30,1);");
}
std::array<content::DatabaseScriptValue,1> era(std::string value){return {std::move(value)};}
std::int64_t id(const content::DatabaseScriptRow &row){
    check(row.size()==4&&row[0].first=="ID"&&row[1].first=="Description"&&row[2].first=="PortraitIndex"&&row[3].first=="IconAtlas",
        "Original SELECT must return owned four named fields in declared order");
    return std::get<std::int64_t>(row[0].second);
}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed game root");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);
    const auto source=read(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const auto sql=CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(source);
    check(sql.size()==280&&runtime::identify_bytes({reinterpret_cast<const std::uint8_t*>(sql.data()),sql.size()}).sha256==
        "6975d9c0ba85e25c436c7bfcc96474dacab401cbe4636a894c7846f79182bff9","Decoded installed Lua long-string SQL identity changed");
    std::string unix_source;for(const auto c:source)if(c!='\r')unix_source+=c;
    std::string alternate;for(const auto c:unix_source){alternate+=c;if(c=='\n')alternate+='\r';}
    check(CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(unix_source)==sql&&
        CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(alternate)==sql,"Lua long strings must decode LF/CRLF/LFCR consistently");
    rejects([&]{CivisCivilopediaUnitsQueryOwner::declared_sql_from_source("");});
    rejects([&]{CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(source+source);});
    auto edited=source;const auto predicate=edited.find("Technologies.Era = ?;");
    check(predicate!=edited.npos,"Installed predicate missing");edited.replace(predicate,21,"Technologies.Era = ?; DELETE FROM Units;");
    rejects([&]{CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(edited);});
    edited=source;edited.replace(edited.find("local UnitsByEra = DB.CreateQuery(sql);"),38,"local UnitsByEra = DB.Query(sql);");
    rejects([&]{CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(edited);});
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:units-query-diagnostic";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;
    auto context=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    content::RulesDatabase rules;fixture(rules);const auto source_before=rules.gameplay_fingerprint_data();
    const auto owner=CivisCivilopediaUnitsQueryOwner::open(root,sources,rules,identity,context);
    check(owner->declared_sql()==sql,"Owner must retain decoded installed SQL");
    for(const auto &bad:{sql+" ",sql+"DELETE FROM Units;",std::string("DELETE FROM Units"),sql+std::string(1,'\0')})
        rejects([&]{owner->prepare(bad);});
    auto query=owner->prepare(sql);const auto ancient=era("ERA_ANCIENT"),medieval=era("ERA_MEDIEVAL");
    auto first=query->iterate(ancient);auto first_row=first->next(),second_row=first->next();
    check(first_row&&second_row,"Actual Units SQL must return both admitted Ancient rows");
    const auto first_id=id(*first_row),second_id=id(*second_row);
    check((first_id==5000000001LL&&second_id==2)||(first_id==2&&second_id==5000000001LL),
        "Original SQL has no ORDER BY; both exact admitted IDs including wide ID must survive");
    const auto owned_wide=first_id==5000000001LL?*first_row:*second_row;
    check(std::get<std::string>(owned_wide.at(1).second)=="TXT_A"&&std::get<std::int64_t>(owned_wide.at(2).second)==3&&
        std::get<std::string>(owned_wide.at(3).second)=="ATLAS_A","Real cursor must return declared text/portrait/atlas");
    check(!first->next()&&!first->next(),"Faith-enabled unit included; excluded faith/hidden/no-tech units absent and EOF sticky");
    auto second=query->iterate(medieval);rejects([&]{first->next();});
    check(id(*second->next())==3&&!second->next(),"Prepared statement must reset and rebind era after exhaustion");
    auto empty=query->iterate(era("ERA_EMPTY"));check(!empty->next()&&!empty->next(),"Unknown era must have real sticky EOF");
    empty=query->iterate(era(""));check(!empty->next(),"Empty string is a bound value, not an absent parameter");
    auto interrupted=query->iterate(ancient);check(id(*interrupted->next())==first_id,"Early abort fixture absent");
    auto replacement=query->iterate(medieval);rejects([&]{interrupted->next();});check(id(*replacement->next())==3,"New generation replaces partially consumed era");
    // Invalid invocation does not mutate cursor/generation; the later host
    // adapter must also make a caught binding exception reject VM readiness.
    rejects([&]{query->iterate({});});
    for(const auto &value:std::array<content::DatabaseScriptValue,5>{std::monostate{},true,std::int64_t{1},1.0,std::string("ERA\0A",5)}){
        const std::array<content::DatabaseScriptValue,1> args{value};rejects([&]{query->iterate(args);});
    }
    const std::array<content::DatabaseScriptValue,2> extra{std::string("ERA_ANCIENT"),std::string("ERA_MEDIEVAL")};
    rejects([&]{query->iterate(extra);});check(!replacement->next(),"Rejected bind must not invalidate current iterator");
    auto independent=owner->prepare(sql);auto a=query->iterate(ancient),b=independent->iterate(medieval);
    check(id(*a->next())==first_id&&id(*b->next())==3&&id(*a->next())==second_id,"Independent factories must not share cursor bindings");
    auto injection=query->iterate(era("ERA_ANCIENT'; DELETE FROM Units;--"));check(!injection->next(),"Era must bind as copied literal, never SQL");
    check(rules.gameplay_fingerprint_data()==source_before,"Diagnostic SELECT and binds must not mutate source rules");
    rules.apply_sql("DELETE FROM Units");check(id(*query->iterate(ancient)->next())==first_id,"Owned clone must survive later source DB mutation");
    check(std::get<std::string>(owned_wide.at(1).second)=="TXT_A","Returned rows must retain owned data after reset/rebind/source mutation");
    std::atomic<int> thread_rejections=0;
    auto threaded=query->iterate(ancient);
    std::thread foreign([&]{
        for(const auto &call:std::array<std::function<void()>,6>{[&]{owner->prepare(sql);},[&]{owner->declared_sql();},
            [&]{query->iterate(ancient);},[&]{threaded->next();},[&]{query->close();},[&]{owner->close();}})
            try{call();}catch(const runtime::ScriptError &){++thread_rejections;}
    });foreign.join();check(thread_rejections==6&&threaded->next().has_value(),"Foreign-thread operations must reject before changing state");
    auto changed=identity;++changed.frame_number;context->observe(changed);
    rejects([&]{threaded->next();});rejects([&]{owner->prepare(sql);});rejects([&]{query->iterate(ancient);});
    query->close();owner->close();owner->close();rejects([&]{owner->declared_sql();});
    auto fresh_context=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    content::RulesDatabase fresh_rules;fixture(fresh_rules);
    auto closed_owner=CivisCivilopediaUnitsQueryOwner::open(root,sources,fresh_rules,identity,fresh_context);
    auto closed_query=closed_owner->prepare(sql);auto closed_iter=closed_query->iterate(ancient);closed_owner->close();
    rejects([&]{closed_iter->next();});rejects([&]{closed_query->iterate(ancient);});
    auto query_owner=CivisCivilopediaUnitsQueryOwner::open(root,sources,fresh_rules,identity,fresh_context);
    auto explicit_query=query_owner->prepare(sql);auto explicit_iter=explicit_query->iterate(ancient);explicit_query->close();
    rejects([&]{explicit_iter->next();});check(query_owner->prepare(sql)->iterate(ancient)->next().has_value(),"Closing one factory must not close owner");
    auto expired_owner=CivisCivilopediaUnitsQueryOwner::open(root,sources,fresh_rules,identity,fresh_context);
    auto expired_query=expired_owner->prepare(sql);auto expired_iter=expired_query->iterate(ancient);expired_owner.reset();
    rejects([&]{expired_iter->next();});rejects([&]{expired_query->iterate(ancient);});
    std::shared_ptr<CivisCivilopediaUnitsQueryOwner> retained;
    {content::RulesDatabase temporary;fixture(temporary);retained=CivisCivilopediaUnitsQueryOwner::open(root,sources,temporary,identity,fresh_context);}
    check(retained->prepare(sql)->iterate(ancient)->next().has_value(),"Owned clone must survive source DB destruction");
    auto expired_context=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    auto context_owner=CivisCivilopediaUnitsQueryOwner::open(root,sources,fresh_rules,identity,expired_context);
    auto context_query=context_owner->prepare(sql);auto context_iter=context_query->iterate(ancient);expired_context.reset();
    rejects([&]{context_iter->next();});rejects([&]{context_query->iterate(ancient);});
    auto bad_sources=sources;bad_sources.civilopedia_sha256=std::string(64,'0');
    rejects([&]{CivisCivilopediaUnitsQueryOwner::open(root,bad_sources,fresh_rules,identity,fresh_context);});
    auto bad_identity=identity;bad_identity.rules_sha256.clear();
    rejects([&]{CivisCivilopediaUnitsQueryOwner::open(root,sources,fresh_rules,bad_identity,fresh_context);});
    rejects([&]{CivisCivilopediaUnitsQueryOwner::open(root/"missing",sources,fresh_rules,identity,fresh_context);});
    content::RulesDatabase missing_schema;auto invalid=CivisCivilopediaUnitsQueryOwner::open(root,sources,missing_schema,identity,fresh_context);
    rejects([&]{invalid->prepare(sql);});
    content::RulesDatabase step_error;
    step_error.apply_sql("CREATE TABLE Technologies(Type TEXT,Era TEXT);INSERT INTO Technologies VALUES('TECH_A','ERA_ANCIENT');"
        "CREATE VIEW Units AS SELECT abs(-9223372036854775808) AS ID,'TXT' AS Description,0 AS PortraitIndex,'ATLAS' AS IconAtlas,"
        "'TECH_A' AS PreReqTech,0 AS FaithCost,0 AS RequiresFaithPurchaseEnabled,1 AS Cost,1 AS ShowInPedia;");
    auto error_owner=CivisCivilopediaUnitsQueryOwner::open(root,sources,step_error,identity,fresh_context);
    auto lazy_error=error_owner->prepare(sql);auto error_iter=lazy_error->iterate(ancient);
    // SQLite integer overflow occurs only when the projection is stepped.
    // Prepare/invoke succeeded: an eager Rows substitute would fail sooner.
    rejects([&]{error_iter->next();});rejects([&]{error_iter->next();});rejects([&]{lazy_error->iterate(ancient);});
    auto factory_owner=CivisCivilopediaUnitsQueryOwner::open(root,sources,fresh_rules,identity,fresh_context);
    auto dropped_factory=factory_owner->prepare(sql);auto retained_iter=dropped_factory->iterate(ancient);dropped_factory.reset();
    check(retained_iter->next().has_value(),"Iterator must retain prepared state after factory reference is dropped");
    factory_owner->close();rejects([&]{retained_iter->next();});
    fresh_context->close();rejects([&]{query_owner->prepare(sql);});
    std::cout<<"Civis first Units CreateQuery owner: installed SQL, clone, reset/rebind, generation, sticky EOF, binding and lifetime PASS\n";
}
