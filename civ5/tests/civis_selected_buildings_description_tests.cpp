#include "civ5/ui/selected_civilopedia_locale.hpp"
#include "civ5/ui/civis_selected_civilopedia_text_data_owner.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5;using namespace civ5::ui;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F call,const char *why){bool rejected=false;try{call();}catch(const std::exception &){rejected=true;}check(rejected,why);}
void write(const std::filesystem::path &file,const std::string &text){std::filesystem::create_directories(file.parent_path());std::ofstream out(file,std::ios::binary);out<<text;check(bool(out),"Authored fixture write failed");}
struct Fixture {
    std::filesystem::path root=std::filesystem::path("C:/CrazyProject/build/graphics-wave06/create-query/buildings-descriptions/fixtures")/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    content::Catalog catalog;runtime::SavedContentProfile profile;content::RulesDatabase rules;
    std::unique_ptr<assets::ResourceMountTable> selected_mounts,legacy_mounts;
    SelectedBnwCivilopediaLayoutSources sources;CivilopediaLayoutOwnerKey key;
    std::shared_ptr<assets::LooseResourceIndex> index;
    Fixture(){
        const std::string rows="<GameData><Language_en_US>"
            "<Row Tag=\"BASE\"><Text>Base</Text></Row><Row Tag=\"UNIT\"><Text>Warrior</Text></Row>"
            "<Row Tag=\"BUILD\"><Text>Granary</Text></Row><Row Tag=\"GENERAL\"><Text>A|B</Text></Row>"
            "<Row Tag=\"COMPOSITE\"><Text>{1_Name}</Text></Row></Language_en_US>"
            "<Language_RU_RU><Row Tag=\"BASE\"><Text>База</Text></Row><Row Tag=\"UNIT\"><Text>Воин|воина</Text></Row>"
            "<Row Tag=\"BUILD\"><Text>Амбар|амбара|амбару|амбар|амбаром|амбаре</Text></Row>"
            "<Row Tag=\"GENERAL\"><Text>А|Б</Text></Row><Row Tag=\"COMPOSITE\"><Text>{1_Name}</Text></Row></Language_RU_RU></GameData>";
        write(root/"Assets/DLC/Selected/Selected.Civ5Pkg","<Civ5Package><GUID>{b685d5de-7cca-4e75-81b4-2f60754e6330}</GUID><Version>1</Version><TextData>selected.xml</TextData><Gameplay><Directory>Gameplay</Directory></Gameplay></Civ5Package>");
        write(root/"Assets/DLC/Selected/Gameplay/XML/Text/selected.xml",rows);
        for(const auto prefix:{"Assets/Gameplay/XML/NewText/EN_US","Assets/Gameplay/XML/NewText/RU_RU","Assets/DLC/Expansion/Gameplay/XML/Text/en_US","Assets/DLC/Expansion/Gameplay/XML/Text/RU_RU","Assets/DLC/Expansion2/Gameplay/XML/Text/en_US","Assets/DLC/Expansion2/Gameplay/XML/Text/RU_RU"})write(root/prefix/"base.xml",rows);
        write(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua","x.labelString = Locale.Lookup(\"BASE\");");
        write(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml","<Context><Button ID=\"CategoryButton1\" ToolTip=\"BASE\"/></Context>");
        catalog=content::discover_packages({root/"Assets/DLC"});check(catalog.packages.size()==1,"Fixture selected catalog changed");
        profile.can_activate=profile.all_requirements_located=true;profile.enabled_dlc={0};profile.kind=runtime::ContentProfileKind::dlc;
        selected_mounts=std::make_unique<assets::ResourceMountTable>(content::build_selected_loose_resource_plan(root,profile.enabled_dlc,catalog).resources);
        index=std::make_shared<assets::LooseResourceIndex>(root);
        for(const auto prefix:{"Assets/Gameplay/XML/NewText/EN_US","Assets/Gameplay/XML/NewText/RU_RU","Assets/DLC/Expansion/Gameplay/XML/Text/en_US","Assets/DLC/Expansion/Gameplay/XML/Text/RU_RU","Assets/DLC/Expansion2/Gameplay/XML/Text/en_US","Assets/DLC/Expansion2/Gameplay/XML/Text/RU_RU"})index->scan({root/prefix,{},true});
        legacy_mounts=std::make_unique<assets::ResourceMountTable>(assets::resource_lookup(index),assets::resource_lookup_all(index),assets::resource_keys(index));
        content::RulesDatabase localization;localization.apply_sql("CREATE TABLE Languages(Type TEXT PRIMARY KEY);INSERT INTO Languages VALUES('en_US'),('ru_RU');CREATE TABLE LocalizedText(Language TEXT,Tag TEXT,Text TEXT,PRIMARY KEY(Language,Tag));");
        const std::array<std::pair<std::string,std::string>,5> en{{{"BASE","Base"},{"UNIT","Warrior"},{"BUILD","Granary"},{"GENERAL","A|B"},{"COMPOSITE","{1_Name}"}}};
        const std::array<std::pair<std::string,std::string>,5> ru{{{"BASE","База"},{"UNIT","Воин|воина"},{"BUILD","Амбар|амбара|амбару|амбар|амбаром|амбаре"},{"GENERAL","А|Б"},{"COMPOSITE","{1_Name}"}}};
        for(const auto &[tag,text]:en)localization.apply_statements(std::array<content::DatabaseStatement,1>{{{"INSERT INTO LocalizedText VALUES('en_US',?,?)",{tag,text}}}});
        for(const auto &[tag,text]:ru)localization.apply_statements(std::array<content::DatabaseStatement,1>{{{"INSERT INTO LocalizedText VALUES('ru_RU',?,?)",{tag,text}}}});
        rules.attach_localization(localization);rules.apply_sql("CREATE TABLE Units(Description TEXT);INSERT INTO Units VALUES('UNIT');CREATE TABLE Buildings(Description TEXT);INSERT INTO Buildings VALUES('BUILD'),('COMPOSITE');");
        sources.verified=sources.control_tree.verified=true;
        sources.civilopedia_sha256=runtime::identify_file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua").sha256;
        sources.xml_sha256=runtime::identify_file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml").sha256;sources.styles_sha256=std::string(64,'c');
        key.frame_number=11;key.owner_token="Civis:Buildings-description-fixture";key.player_id=0;key.game_turn=7;
        key.payload_sha256=std::string(64,'a');key.rules_sha256=std::string(64,'b');key.civilopedia_sha256=sources.civilopedia_sha256;key.xml_sha256=sources.xml_sha256;key.styles_sha256=sources.styles_sha256;
    }
    auto locale(std::string language){
        auto result=SelectedCivilopediaLocale::open(root,*legacy_mounts,sources,key,std::move(language));
        result->enable_selected_text_data(root,profile,catalog,*selected_mounts,rules);return result;
    }
    auto owner(CivisSelectedCivilopediaTextDataOwner::Current current=[](const auto &){return true;}){
        return CivisSelectedCivilopediaTextDataOwner::open(root,profile,catalog,*selected_mounts,rules,key,"ru_RU",std::move(current));
    }
};
int run(){
    Fixture f;
    auto legacy=f.locale("ru_RU");check(!legacy->buildings_description_first_form_enabled(),"Default TextData must not enable Buildings first form");
    check(legacy->provider().convert_text_key("BUILD",{})=="Амбар|амбара|амбару|амбар|амбаром|амбаре","Legacy selected TextData behavior must remain unchanged");
    rejects([&]{legacy->enable_buildings_description_first_form();},"Buildings capability cannot be enabled after text conversion");
    auto ru=f.locale("ru_RU"),en=f.locale("en_US");ru->enable_buildings_description_first_form();en->enable_buildings_description_first_form();
    check(ru->buildings_description_first_form_enabled()&&en->buildings_description_first_form_enabled(),"Explicit Buildings capability unavailable");
    const auto rp=ru->provider(),ep=en->provider();
    check(rp.convert_text_key("BUILD",{})=="Амбар"&&ep.convert_text_key("BUILD",{})=="Granary","Bounded selected Building literal names must use default form1 EN/RU");
    check(rp.convert_text_key("UNIT",{})=="Воин","Existing Units first form must stay intact");
    check(rp.convert_text_key("GENERAL",{})=="А|Б","Unrelated pipe payload must not receive Buildings formatting fallback");
    const std::array<runtime::ScriptValue,1> arg{std::string("Named")};
    check(rp.convert_text_key("BUILD",arg)=="Амбар|амбара|амбару|амбар|амбаром|амбаре","Building admission must apply only to actual zero-argument names");
    rejects([&]{rp.convert_text_key("COMPOSITE",{});},"Building literal first form must reject unowned composition");
    rejects([&]{ru->enable_buildings_description_first_form();},"Duplicate Buildings capability must reject");
    auto direct=f.owner();direct->enable_buildings_description_first_form();
    check(direct->building_description("BUILD")&&!direct->building_description("GENERAL")&&direct->unit_description("UNIT"),"Admission must derive separate selected Description domains");
    auto late=f.owner();late->raw("BUILD");rejects([&]{late->enable_buildings_description_first_form();},"Direct declaration after lookup must reject");
    auto snap=f.owner();f.rules.apply_sql("DELETE FROM Buildings;INSERT INTO Buildings VALUES('GENERAL');");snap->enable_buildings_description_first_form();
    check(snap->building_description("BUILD")&&!snap->building_description("GENERAL"),"Building admission must reuse SAME private clone, not reread mutated caller DB");
    f.rules.apply_sql("DELETE FROM Buildings;INSERT INTO Buildings VALUES('BUILD'),('COMPOSITE');");
    std::string borrowed="BUILD";bool armed=false,called=false;
    auto pinned=f.owner([&](const auto &){if(armed){borrowed=std::string(4096,'x');called=true;}return true;});
    pinned->enable_buildings_description_first_form();armed=true;check(pinned->building_description(borrowed)&&called,"Building admission must pin key before Current replacement");
    auto thread=f.owner();std::atomic<bool> rejected=false;std::thread foreign([&]{try{thread->enable_buildings_description_first_form();}catch(const runtime::ScriptError &){rejected=true;}});foreign.join();
    check(rejected,"Foreign thread cannot declare Buildings capability");thread->enable_buildings_description_first_form();
    bool valid=true;auto stale=f.owner([&](const auto &){return valid;});valid=false;
    rejects([&]{stale->enable_buildings_description_first_form();},"Stale owner cannot enable Buildings");valid=true;rejects([&]{stale->raw("BUILD");},"Stale failed owner cannot revive");
    std::shared_ptr<CivisSelectedCivilopediaTextDataOwner> closing;
    closing=f.owner([&](const auto &){if(closing)closing->close();return true;});
    rejects([&]{closing->enable_buildings_description_first_form();},"Current close must reject declaration before publish");
    std::shared_ptr<CivisSelectedCivilopediaTextDataOwner> reentrant;bool nested=false;
    reentrant=f.owner([&](const auto &){if(reentrant&&!nested){nested=true;try{reentrant->enable_buildings_description_first_form();}catch(...){}nested=false;}return true;});
    rejects([&]{reentrant->enable_buildings_description_first_form();},"Caught declaration reentry must retire outer owner");
    auto closed=f.locale("en_US");closed->enable_buildings_description_first_form();closed->close();rejects([&]{closed->buildings_description_first_form_enabled();},"Closed locale cannot report active Buildings admission");
    auto changed=f.locale("en_US");changed->enable_buildings_description_first_form();auto next=f.key;++next.frame_number;changed->observe(next);
    rejects([&]{changed->provider().convert_text_key("BUILD",{});},"Changed frame owner must reject Building names");
    std::cout<<"Selected Buildings Description first form: PASS (separate EN/RU scope, SAME clone/source, legacy preservation, no generic fallback, input pins and lifetime)\n";return 0;
}
}
int main(){try{return run();}catch(const std::exception &e){std::cerr<<"Buildings description test failed: "<<e.what()<<'\n';return 1;}catch(...){std::cerr<<"Buildings description unknown failure\n";return 1;}}
