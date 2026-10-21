#include "civ5/ui/civis_selected_civilopedia_text_data_owner.hpp"
#include "civ5/ui/bnw_info_default_form.hpp"
#include "civ5/ui/selected_civilopedia_locale.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

using namespace civ5;
namespace {
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
template<class F>void rejects(F f,const char *why){bool failed=false;try{f();}catch(...){failed=true;}check(failed,why);}
void write(const std::filesystem::path &path,const std::string &bytes){
    std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);out<<bytes;check(static_cast<bool>(out),"fixture write failed");
}
ui::CivilopediaLayoutOwnerKey identity(){
    ui::CivilopediaLayoutOwnerKey key;key.frame_number=7;key.owner_token="CivilopediaScreen:explicit-fixture";key.player_id=0;key.game_turn=3;
    key.payload_sha256=key.rules_sha256=key.civilopedia_sha256=key.xml_sha256=key.styles_sha256=std::string(64,'a');return key;
}
struct Fixture {
    const std::filesystem::path root=std::filesystem::path("C:/CrazyProject/build/graphics-wave06/create-query/selected-textdata/fixtures")/
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::path manifest,text;
    content::Catalog catalog;runtime::SavedContentProfile profile;
    std::unique_ptr<assets::ResourceMountTable> resources;
    content::RulesDatabase rules;
    bool valid=true;
    const std::shared_ptr<std::function<void()>> source_effect=std::make_shared<std::function<void()>>();
    const std::string rows="<GameData><Language_en_US><Row Tag=\"UNIT_SELECTED\"><Text>Selected EN</Text></Row>"
        "<Row Tag=\"ONLY_EN\"><Text>No language fallback</Text></Row>"
        "<Row Tag=\"FORMAT\"><Text>Owned {1_Name}</Text></Row></Language_en_US>"
        "<Language_RU_RU><Row Tag=\"UNIT_SELECTED\"><Text>Выбранный|выбранного</Text><Gender>masculine</Gender><Plurality>1</Plurality></Row></Language_RU_RU></GameData>";
    Fixture(){
        manifest=root/"Assets/DLC/Selected/Selected.Civ5Pkg";text=root/"Assets/DLC/Selected/Gameplay/XML/Text/selected.xml";
        write(root/"Assets/Gameplay/XML/NewText/EN_US/base.xml","<GameData><Language_en_US><Row Tag=\"BASE\"><Text>Base EN</Text></Row></Language_en_US><Language_RU_RU><Row Tag=\"BASE\"><Text>База</Text></Row></Language_RU_RU></GameData>");
        write(manifest,"<Civ5Package><GUID>{b685d5de-7cca-4e75-81b4-2f60754e6330}</GUID><Version>1</Version><TextData>selected.xml</TextData><Gameplay><Directory>Gameplay</Directory></Gameplay></Civ5Package>");
        write(text,rows);
        write(root/"Assets/DLC/Inactive/Inactive.Civ5Pkg","<Civ5Package><GUID>{293c1ee3-1176-44f6-ac1f-59663826de74}</GUID><Version>1</Version><TextData>inactive.xml</TextData><Gameplay><Directory>Gameplay</Directory></Gameplay></Civ5Package>");
        write(root/"Assets/DLC/Inactive/Gameplay/XML/Text/inactive.xml","<GameData><Language_en_US><Row Tag=\"INACTIVE\"><Text>Forbidden</Text></Row></Language_en_US></GameData>");
        refresh();
        content::RulesDatabase localization;
        localization.apply_sql("CREATE TABLE Languages(Type TEXT PRIMARY KEY,Name TEXT);CREATE TABLE LocalizedText(Language TEXT,Tag TEXT,Text TEXT,Gender TEXT,Plurality TEXT,PRIMARY KEY(Language,Tag));INSERT INTO Languages VALUES('en_US','English'),('ru_RU','Русский');");
        const std::array<content::DatabaseStatement,7> values{{
            {"INSERT INTO LocalizedText VALUES('en_US','BASE',?,NULL,NULL)",{std::string("Base EN")}},
            {"INSERT INTO LocalizedText VALUES('ru_RU','BASE',?,NULL,NULL)",{std::string("База")}},
            {"INSERT INTO LocalizedText VALUES('en_US','UNIT_SELECTED',?,NULL,NULL)",{std::string("Selected EN")}},
            {"INSERT INTO LocalizedText VALUES('ru_RU','UNIT_SELECTED',?,'masculine','1')",{std::string("Выбранный|выбранного")}},
            {"INSERT INTO LocalizedText VALUES('en_US','ONLY_EN',?,NULL,NULL)",{std::string("No language fallback")}},
            {"INSERT INTO LocalizedText VALUES('en_US','INACTIVE',?,NULL,NULL)",{std::string("Forbidden")}},
            {"INSERT INTO LocalizedText VALUES('en_US','FORMAT',?,NULL,NULL)",{std::string("Owned {1_Name}")}},
        }};
        localization.apply_statements(values);rules.attach_localization(localization);
        rules.apply_sql("CREATE TABLE Units(Description TEXT);INSERT INTO Units VALUES('UNIT_SELECTED');");
    }
    void refresh(){
        catalog=content::discover_packages({root/"Assets/DLC"});check(catalog.packages.size()==2,"fixture package discovery changed");
        profile={};profile.can_activate=profile.all_requirements_located=true;profile.kind=runtime::ContentProfileKind::dlc;
        for(std::size_t i=0;i<catalog.packages.size();++i)if(content::canonical_guid(catalog.packages[i].id)=="b685d5de-7cca-4e75-81b4-2f60754e6330")profile.enabled_dlc.push_back(i);
        check(profile.enabled_dlc.size()==1,"fixture selected activation ambiguous");
        resources=std::make_unique<assets::ResourceMountTable>(content::build_selected_loose_resource_plan(root,profile.enabled_dlc,catalog).resources);
        // This retained source callback observes resolution of the owner's
        // copied mount table without contributing candidates or fake assets.
        assets::ResourceMount observer;observer.id=777;
        observer.lookup=[](const auto &){return assets::ResourceMountHit{};};
        observer.all_lookup=[effect=source_effect](const auto &){
            if(*effect)(*effect)();return std::vector<assets::ResourceMountHit>{};
        };
        resources->mount(std::move(observer));
    }
    auto open(const std::string &language="en_US"){
        return ui::CivisSelectedCivilopediaTextDataOwner::open(root,profile,catalog,*resources,rules,identity(),language,
            [&](const auto &key){return valid&&key.owner_token==identity().owner_token&&key.frame_number==7;});
    }
};
void pin_checks(){
    {
        Fixture f;
        std::string borrowed="UNIT_SELECTED";
        bool armed=false,called=false;
        auto owner=ui::CivisSelectedCivilopediaTextDataOwner::open(f.root,f.profile,f.catalog,*f.resources,f.rules,identity(),"en_US",[&](const auto &){
            if(armed){borrowed=std::string(4096,'x');called=true;}return true;
        });
        armed=true;
        const auto row=owner->raw(borrowed);
        check(called&&row.tag=="UNIT_SELECTED"&&row.text=="Selected EN","Current replacement retargeted raw borrowed key");
        borrowed="UNIT_SELECTED";called=false;
        check(owner->unit_description(borrowed)&&called,"Current replacement retargeted Unit Description borrowed key");
    }
    {
        Fixture f;auto owner=f.open();std::string borrowed="UNIT_SELECTED";bool called=false;
        *f.source_effect=[&]{borrowed=std::string(4096,'x');called=true;};
        const auto row=owner->raw(borrowed);
        check(called&&row.tag=="UNIT_SELECTED"&&row.text=="Selected EN","source observer invalidated raw returned key");
    }
    {
        Fixture f;auto root=f.root;auto profile=f.profile;auto catalog=f.catalog;
        auto resources=*f.resources;bool called=false;
        auto owner=ui::CivisSelectedCivilopediaTextDataOwner::open(root,profile,catalog,resources,f.rules,identity(),"en_US",[&](const auto &){
            if(!called){
                called=true;root="C:/missing-after-selected-snapshot";profile.enabled_dlc.clear();
                profile.can_activate=false;catalog.packages.clear();
                resources.set_default_enabled(false);
                for(const auto &mount:std::vector<assets::ResourceMount>(resources.mounts()))resources.unmount(mount.id);
                f.rules.apply_sql("UPDATE Localization.LocalizedText SET Text='retargeted' WHERE Tag='UNIT_SELECTED';DELETE FROM Units;");
            }
            return true;
        });
        check(called&&owner->raw("UNIT_SELECTED").text=="Selected EN"&&owner->unit_description("UNIT_SELECTED"),
              "Current mutation retargeted activation configuration or attached DB before clone");
    }
    {
        Fixture f;auto root=f.root;auto profile=f.profile;auto catalog=f.catalog;auto resources=*f.resources;
        rejects([&]{ui::CivisSelectedCivilopediaTextDataOwner::open(root,profile,catalog,resources,f.rules,identity(),"en_US",[&](const auto &){
            root="C:/stale-replaced-root";profile.enabled_dlc.clear();catalog.packages.clear();return false;
        });},"mutated activation with stale Current must reject before publication");
    }
    {
        Fixture f;
        // Tiny authored sources exercise the real locale adapter without
        // replacing installed files or claiming a stock-source parent proof.
        const auto pedia=f.root/"Assets/DLC/Expansion2/UI/Civilopedia";
        write(pedia/"CivilopediaScreen.lua","x.labelString = Locale.Lookup(\"BASE\");");
        write(pedia/"CivilopediaScreen.xml","<Context><Button ID=\"CategoryButton1\" ToolTip=\"BASE\"/></Context>");
        const auto layer="<GameData><Language_en_US><Row Tag=\"BASE\"><Text>Base EN</Text></Row>"
            "<Row Tag=\"FORMAT\"><Text>Owned {1_Name}</Text></Row></Language_en_US></GameData>";
        write(f.root/"Assets/DLC/Expansion/Gameplay/XML/Text/en_US/base.xml",layer);
        write(f.root/"Assets/DLC/Expansion2/Gameplay/XML/Text/en_US/base.xml",layer);
        auto index=std::make_shared<assets::LooseResourceIndex>(f.root);
        for(const auto prefix:{"Assets/Gameplay/XML/NewText/EN_US","Assets/DLC/Expansion/Gameplay/XML/Text/en_US","Assets/DLC/Expansion2/Gameplay/XML/Text/en_US"})index->scan({f.root/prefix,{},true});
        assets::ResourceMountTable legacy(assets::resource_lookup(index),assets::resource_lookup_all(index),assets::resource_keys(index));
        ui::SelectedBnwCivilopediaLayoutSources sources;sources.verified=sources.control_tree.verified=true;
        sources.civilopedia_sha256=runtime::identify_file(pedia/"CivilopediaScreen.lua").sha256;
        sources.xml_sha256=runtime::identify_file(pedia/"CivilopediaScreen.xml").sha256;sources.styles_sha256=std::string(64,'a');
        auto key=identity();key.civilopedia_sha256=sources.civilopedia_sha256;key.xml_sha256=sources.xml_sha256;
        {
            auto legacy_locale=ui::SelectedCivilopediaLocale::open(f.root,legacy,sources,key,"en_US");
            auto legacy_provider=legacy_locale->provider();
            std::string borrowed_tag="FORMAT";
            const std::string original="External observer argument "+std::string(4096,'y');
            std::vector<runtime::ScriptValue> borrowed_args{original};bool notified=false;
            assets::ScopedResourceResolveObserver observer(legacy,[&](const auto &,auto,bool){
                notified=true;borrowed_tag=std::string(4096,'x');
                std::vector<runtime::ScriptValue>{std::string("Replaced observer argument")}.swap(borrowed_args);
            });
            check(legacy_provider.convert_text_key(borrowed_tag,borrowed_args)=="Owned "+original&&notified&&!observer.failed(),
                  "external source observer freed legacy format storage before key/argument snapshot");
        }
        auto locale=ui::SelectedCivilopediaLocale::open(f.root,legacy,sources,key,"en_US");
        locale->enable_selected_text_data(f.root,f.profile,f.catalog,*f.resources,f.rules);
        const auto provider=locale->provider();
        std::string tag="FORMAT";
        const std::string expected="Original argument "+std::string(4096,'z');
        std::vector<runtime::ScriptValue> args{expected};bool called=false;
        *f.source_effect=[&]{
            called=true;tag=std::string(4096,'x');
            std::vector<runtime::ScriptValue>{std::string("Retargeted")}.swap(args);
        };
        check(provider.convert_text_key(tag,args)=="Owned "+expected&&called,
              "source callback freed borrowed format key/ScriptValue storage or retargeted arguments");
        const auto &observed=locale->observations();
        check(observed.size()==1&&observed.front().tag=="FORMAT","format observation used callback-mutated original key");
        *f.source_effect=[&]{locale->close();};tag="FORMAT";args={expected};
        rejects([&]{provider.convert_text_key(tag,args);},"closing locale in source callback must reject pinned conversion");
    }
    std::cout<<"input pin checks: PASS (Current/source observer key+args replacement; configuration+attached DB mutation; stale/close rejection)\n";
}
}
int main(){
    try{
        pin_checks();
        Fixture f;auto en=f.open(),ru=f.open("ru_RU");
        const auto en_row=en->raw("UNIT_SELECTED"),ru_row=ru->raw("UNIT_SELECTED");
        check(en_row.text=="Selected EN"&&en_row.package==f.profile.enabled_dlc.front()&&en_row.mount_id.has_value()&&en_row.source_sha256.size()==64,"source/DB selected EN row and retained mount not exact");
        check(ru_row.text=="Выбранный|выбранного"&&ru_row.language=="ru_RU","exact RU authored forms must be retained before formatting");
        ui::TopPanelLocalizationCatalog catalog;catalog.rows.emplace(ru_row.tag,ru_row.text);
        check(ru->unit_description(ru_row.tag)&&ui::bnw_info_default_form(catalog,ru_row.tag)=="Выбранный","reuse default literal Info first form");
        check(en->raw("BASE").text=="Base EN"&&!en->raw("BASE").package,"base scope retained independently of DLC");
        rejects([&]{ru->raw("ONLY_EN");},"missing RU must not fall back to installed English");
        rejects([&]{en->raw("INACTIVE");},"unselected package cannot be authorized even if malicious DB has its tag");
        rejects([&]{en->raw("MISSING");},"missing source key must not echo or invent text");
        rejects([&]{f.open("fr_FR");},"unsupported language rejected");
        f.rules.apply_sql("UPDATE Localization.LocalizedText SET Text='changed' WHERE Tag='UNIT_SELECTED' AND Language='en_US'");
        check(en->raw("UNIT_SELECTED").text=="Selected EN","attached DB clone is immutable after caller mutation");
        auto mismatched=f.open();rejects([&]{mismatched->raw("UNIT_SELECTED");},"source versus selected snapshot mismatch cannot be hidden");
        f.rules.apply_sql("UPDATE Localization.LocalizedText SET Text='Selected EN' WHERE Tag='UNIT_SELECTED' AND Language='en_US'");
        bool wrong_thread=false;std::thread other([&]{try{en->raw("BASE");}catch(...){wrong_thread=true;}});other.join();check(wrong_thread,"source owner thread guard missing");
        f.valid=false;rejects([&]{en->raw("BASE");},"stale session/context owner must fail");f.valid=true;
        rejects([&]{en->raw("BASE");},"stale owner cannot revive");
        ru->close();rejects([&]{ru->raw("BASE");},"closed owner cannot resolve");
        auto changed_source=f.open();write(f.text,f.rows+"\n");rejects([&]{changed_source->raw("UNIT_SELECTED");},"changed source bytes cannot use cached text");write(f.text,f.rows);
        auto changed_manifest=f.open();write(f.manifest,"<Civ5Package><GUID>{b685d5de-7cca-4e75-81b4-2f60754e6330}</GUID><Version>2</Version><TextData>selected.xml</TextData><Gameplay><Directory>Gameplay</Directory></Gameplay></Civ5Package>");
        rejects([&]{changed_manifest->raw("BASE");},"selected manifest changed after opening");rejects([&]{f.open();},"activated Catalog does not match current selected manifest");
        write(f.manifest,"<Civ5Package><GUID>{b685d5de-7cca-4e75-81b4-2f60754e6330}</GUID><Version>1</Version><TextData>selected.xml</TextData><TextData>duplicate.xml</TextData><Gameplay><Directory>Gameplay</Directory></Gameplay></Civ5Package>");
        write(f.root/"Assets/DLC/Selected/Gameplay/XML/Text/duplicate.xml","<GameData><Language_en_US><Row Tag=\"UNIT_SELECTED\"><Text>Selected EN</Text></Row></Language_en_US></GameData>");f.refresh();
        auto ambiguous=f.open();rejects([&]{ambiguous->raw("UNIT_SELECTED");},"same-layer source duplicates must not guess a native winning insert");
        std::shared_ptr<ui::CivisSelectedCivilopediaTextDataOwner> closing;
        closing=ui::CivisSelectedCivilopediaTextDataOwner::open(f.root,f.profile,f.catalog,*f.resources,f.rules,identity(),"en_US",[&](const auto &){if(closing)closing->close();return true;});
        rejects([&]{closing->raw("BASE");},"current callback closing owner must fail after callback");
        std::shared_ptr<ui::CivisSelectedCivilopediaTextDataOwner> reentrant;
        bool nested=false;
        reentrant=ui::CivisSelectedCivilopediaTextDataOwner::open(f.root,f.profile,f.catalog,*f.resources,f.rules,identity(),"en_US",[&](const auto &){
            if(reentrant&&!nested){nested=true;try{reentrant->raw("BASE");}catch(...){}nested=false;}return true;
        });
        rejects([&]{reentrant->raw("BASE");},"caught nested provider failure must still retire outer resolution");
        auto bad_profile=f.profile;bad_profile.enabled_dlc.push_back(bad_profile.enabled_dlc.front());
        rejects([&]{ui::CivisSelectedCivilopediaTextDataOwner::open(f.root,bad_profile,f.catalog,*f.resources,f.rules,identity(),"en_US",[](const auto &){return true;});},"duplicate activation must not manufacture precedence");
        std::cout<<"selected Civilopedia TextData owner: PASS (explicit source/DB fixtures; no native readiness claim)\n";return 0;
    }catch(const std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
