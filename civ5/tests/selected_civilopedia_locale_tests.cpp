#include "civ5/ui/selected_civilopedia_locale.hpp"
#include "civ5/ui/civis_locale_reference_text.hpp"
#include <fstream>
#include <iostream>
using namespace civ5::ui;using namespace civ5::runtime;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F call){bool ok=false;try{call();}catch(const std::exception &){ok=true;}check(ok,"Expected explicit rejection");}
std::string installed_labels(const std::filesystem::path &root){
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto declarations=source.find("local CategoryHomePage = 1;");const auto end=source.find("-- the instance managers",declarations);
    const auto labels=source.find("CivilopediaCategory[CategoryHomePage].labelString = Locale.ConvertTextKey(");
    const auto finish=source.find("CivilopediaCategory[CategoryHomePage].PopulateList = function()",labels);
    check(declarations!=source.npos&&end!=source.npos&&labels!=source.npos&&finish!=source.npos,"Installed category label source block changed");
    // Category table allocation is a diagnostic prerequisite, not a fake UI
    // registration method. The16translation calls below are exact installed Lua.
    return source.substr(declarations,end-declarations)+
        "\nfor i=1,numCategories do CivilopediaCategory[i]={} end\n"+source.substr(labels,finish-labels);
}
LuaScriptHost host(ScriptLocaleProvider provider){return LuaScriptHost({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},std::move(provider));}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed game root");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);check(sources.verified,"Selected Civilopedia source unavailable");
    auto index=std::make_shared<civ5::assets::LooseResourceIndex>(root);
    for(const auto &prefix:{"Assets/Gameplay/XML/NewText/EN_US","Assets/Gameplay/XML/NewText/RU_RU",
        "Assets/DLC/Expansion/Gameplay/XML/Text/en_US","Assets/DLC/Expansion/Gameplay/XML/Text/ru_RU",
        "Assets/DLC/Expansion2/Gameplay/XML/Text/en_US","Assets/DLC/Expansion2/Gameplay/XML/Text/ru_RU"})index->scan({root/prefix,{},true});
    civ5::assets::ResourceMountTable resources(civ5::assets::resource_lookup(index),civ5::assets::resource_lookup_all(index),civ5::assets::resource_keys(index));
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:locale-diagnostic";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;
    const auto block=installed_labels(root);
    {
        // Explicit grammar fixtures exercise bounds/error handling; these
        // synthetic rows are never installed or presented as translations.
        const std::map<std::string,std::string> rows={
            {"ROOT","Prefix {TXT_KEY_A} / {TXT_KEY_A} / {1_Name}"},
            {"TXT_KEY_A","{TXT_KEY_B}"},{"TXT_KEY_B","Source"},
            {"CYCLE","{TXT_KEY_CYCLE}"},{"TXT_KEY_CYCLE","{TXT_KEY_CYCLE}"},
            {"MISSING","{TXT_KEY_ABSENT}"},{"MODIFIER","{TXT_KEY_A:upper}"},
            {"MALFORMED","{TXT_KEY_A"},{"OVERSIZE",std::string(1024*1024+1,'x')}};
        const auto resolve=[&](std::string_view tag){const auto found=rows.find(std::string(tag));
            if(found==rows.end())throw ScriptError("Diagnostic selected source is absent");
            return found->second;};
        check(expand_civis_locale_references("ROOT",resolve)=="Prefix Source / Source / {1_Name}",
              "Plain recursive source references must preserve numeric argument placeholders");
        for(const auto *tag:{"CYCLE","MISSING","MODIFIER","MALFORMED","OVERSIZE"})
            rejects([&]{expand_civis_locale_references(tag,resolve);});
        rejects([&]{expand_civis_locale_references("ROOT",{});});
        rejects([&]{expand_civis_locale_references("TXT_KEY_CHAIN_0",[](std::string_view tag){
            const auto depth=std::stoul(std::string(tag.substr(std::string_view("TXT_KEY_CHAIN_").size())));
            return "{TXT_KEY_CHAIN_"+std::to_string(depth+1)+"}";});});
    }
    for(const auto *language:{"en_US","ru_RU"})for(const auto frame:{31,32}){
        identity.frame_number=frame;
        auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,language);
        locale->enable_plain_reference_expansion();rejects([&]{locale->enable_plain_reference_expansion();});
        check(locale->plain_lookup_tags()==std::vector<std::string>{"TXT_KEY_PEDIA_CATEGORY_15_LABEL","TXT_KEY_PEDIA_CATEGORY_16_LABEL"},"Plain Lookup capability must come from exact source/XML tags");
        auto vm=host(locale->provider());vm.enable_plain_locale_lookup("CivilopediaScreen",locale->plain_lookup_tags());
        vm.execute("OtherContext","assert(Locale.Lookup==nil)","=unrequested-context");
        const auto first=std::string(language)=="en_US"?"Civilopedia Home Page":"Цивилопедия";
        const auto last=std::string(language)=="en_US"?"World Congress":"Всемирный конгресс";
        vm.execute("CivilopediaScreen",block+"\nassert(CivilopediaCategory[1].labelString=='"+first+"' and CivilopediaCategory[16].labelString=='"+last+"')","@installed-BNW-category-labels");
        check(locale->observations().size()==16&&locale->observations().front().language==language&&
              locale->observations().front().text==first&&locale->observations().back().text==last&&
              locale->observations().back().source_sha256.size()==64,"All16actual EN/RU labels must retain payload/source identity");
        const auto referenced=locale->provider().convert_text_key("TXT_KEY_CONCEPT_ESPIONAGE_SPIES_DESCRIPTION",{});
        check(referenced==(std::string_view(language)=="en_US"?"Spies and Diplomats":"Шпионы и дипломаты")&&
              locale->observations().size()==17&&civ5::assets::normalize_asset_name(locale->observations().back().source_key).ends_with("civ5gametextinfos2_inherited_expansion2.xml"),
              "Actual inherited EN/RU plain-key reference must resolve selected source text and retain the calling payload pin");
        vm.complete_loading();
        vm.execute("CivilopediaScreen",R"lua(
            assert(not pcall(function() return Locale.Lookup('TXT_KEY_PEDIA_CATEGORY_1_LABEL') end))
            assert(not pcall(function() return Locale.Lookup('TXT_KEY_PEDIA_CATEGORY_16_LABEL',1) end))
        )lua","=bounded-lookup-surface");
        rejects([&]{locale->provider().convert_text_key("TXT_KEY_CIVIS_MISSING_TRANSLATION",{});});
        auto changed=identity;++changed.game_turn;locale->observe(changed);
        rejects([&]{locale->provider().convert_text_key("TXT_KEY_PEDIA_CATEGORY_1_LABEL",{});});
        rejects([&]{locale->provider().current();});
    }
    {
        auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,"en_US");auto vm=host(locale->provider());
        rejects([&]{vm.execute("CivilopediaScreen",block,"=old-unrequested-lookup-boundary");});
        check(locale->observations().size()==14,"Old locale capability must not silently enable Lookup");
        rejects([&]{locale->enable_plain_reference_expansion();});
    }
    {
        auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,"en_US");
        rejects([&]{locale->provider().convert_text_key("TXT_KEY_CONCEPT_ESPIONAGE_SPIES_DESCRIPTION",{});});
        check(locale->observations().empty(),"Old locale grammar must reject reference tokens rather than return unresolved key text");
    }
    {
        auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,"en_US");const auto provider=locale->provider();locale.reset();
        rejects([&]{provider.convert_text_key("TXT_KEY_PEDIA_CATEGORY_1_LABEL",{});});
    }
    {
        auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,"en_US");const auto provider=locale->provider();resources.set_default_enabled(false);
        rejects([&]{provider.convert_text_key("TXT_KEY_PEDIA_CATEGORY_1_LABEL",{});});resources.set_default_enabled(true);
    }
    {
        civ5::assets::ResourceMountTable missing(civ5::assets::resource_lookup(index),civ5::assets::resource_lookup_all(index));
        rejects([&]{SelectedCivilopediaLocale::open(root,missing,sources,identity,"en_US");});
    }
    {
        auto all=civ5::assets::resource_lookup_all(index);
        civ5::assets::ResourceMountTable ambiguous(civ5::assets::resource_lookup(index),[all](const auto &key){auto hits=all(key);auto duplicate=hits;hits.insert(hits.end(),duplicate.begin(),duplicate.end());return hits;},civ5::assets::resource_keys(index));
        rejects([&]{SelectedCivilopediaLocale::open(root,ambiguous,sources,identity,"en_US");});
    }
    rejects([&]{SelectedCivilopediaLocale::open(root,resources,sources,identity,"xx_XX");});
    {
        auto changed=sources;changed.xml_sha256=std::string(64,'f');rejects([&]{SelectedCivilopediaLocale::open(root,resources,changed,identity,"en_US");});
    }
    std::cout<<"Actual selected EN/RU category labels passed:14ConvertTextKey+2boundedLookup; four source owners, payload/provenance pins, missing/ambiguous/changed/stale/expired handling. Diagnostic source subset; general Lookup grammar and native parity unverified.\n";
}
