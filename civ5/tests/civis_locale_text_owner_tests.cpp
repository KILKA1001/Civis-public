#include "civ5/ui/civis_locale_text_owner.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5;using namespace civ5::ui;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F call){bool rejected=false;try{call();}catch(const ScriptError &){rejected=true;}check(rejected,"Expected explicit text operation rejection");}
std::vector<std::string> selected_tags(const std::filesystem::path &root){
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    check(source.find("return Locale.Compare(a.entryName, b.entryName) == -1;")!=source.npos&&
          source.find("searchableList[Locale.ToLower(name)] = article;")!=source.npos,"Selected text operation call contracts changed");
    const auto xml=content::read_xml(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml");
    std::vector<std::string> tags;
    const std::function<void(const content::XmlNode &)> collect=[&](const auto &node){
        if(node.attribute("ID").starts_with("CategoryButton"))tags.emplace_back(node.attribute("ToolTip"));
        for(const auto &child:node.children)collect(child);
    };collect(xml);return tags;
}
void lua_checks(const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,
    const CivilopediaLayoutOwnerKey &identity,const std::shared_ptr<SelectedCivilopediaLocale> &locale,
    const std::shared_ptr<CivisLocaleTextOwner> &text_owner,int width,int height){
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto alphabet=source.find("function Alphabetically(a, b)");
    const auto alphabet_end=source.find("\nend",alphabet);
    const auto home=source.find("CivilopediaCategory[CategoryHomePage].PopulateList = function()");
    const auto home_end=source.find("CivilopediaCategory[CategoryGameConcepts].PopulateList",home);
    check(alphabet!=source.npos&&alphabet_end!=source.npos&&home!=source.npos&&home_end!=source.npos,
          "Selected original sort/home population blocks changed");
    auto callback_owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    const auto vm=[&]{
        ScriptUiLayoutProvider layout;layout.context_id="CivilopediaScreen";
        layout.screen_size=[=]{return std::pair<std::int64_t,std::int64_t>{width,height};};
        layout.get_size=[](auto)->std::pair<std::int64_t,std::int64_t>{throw ScriptError("Unrequested diagnostic geometry");};
        layout.set_size=[](auto,auto,auto){throw ScriptError("Unrequested diagnostic geometry");};
        layout.set_offset=[](auto,auto,auto){throw ScriptError("Unrequested diagnostic geometry");};
        layout.calculate_internal_size=[](auto){throw ScriptError("Unrequested diagnostic geometry");};
        bind_civis_civilopedia_category_callbacks(layout,root,sources,identity,callback_owner,77);
        ScriptEnumProvider enums;enums.tables={{"Mouse",{{"eLClick",77}}}};
        return std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},
            ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},ScriptUiEventProvider{},ScriptContentProvider{},
            ScriptUserDataProvider{},std::move(enums),locale->provider(),std::move(layout));
    };
    auto host=vm();std::size_t lower_calls=0,compare_calls=0;
    const auto native=text_owner->provider();
    host->enable_locale_text_operations("CivilopediaScreen",{
        [&](auto text){++lower_calls;return native.lower(text);},
        [&](auto a,auto b){++compare_calls;return native.compare(a,b);}});
    rejects([&]{host->enable_locale_text_operations("CivilopediaScreen",native);});
    host->execute("OtherContext","assert(Locale.ToLower==nil and Locale.Compare==nil)","=unrequested-locale-text-context");
    // These globals are an explicit diagnostic harness for the unmodified
    // source function. Every article name still comes from installed text.
    host->execute("CivilopediaScreen",R"lua(
        CivilopediaCategory={{}}; CategoryHomePage=1; numCategories=16
        sortedList={}; searchableList={}; searchableTextKeyList={}; categorizedList={}
        absurdlyLargeNumTopicsInCategory=10000
    )lua"+source.substr(alphabet,alphabet_end+4-alphabet)+"\n"+source.substr(home,home_end-home)+R"lua(
        CivilopediaCategory[CategoryHomePage].PopulateList()
        assert(#sortedList[CategoryHomePage][1]==16)
        for i,article in ipairs(sortedList[CategoryHomePage][1]) do
            assert(article.entryID==i and article.entryCategory==CategoryHomePage)
            assert(searchableTextKeyList['TXT_KEY_PEDIA_CATEGORY_'..i..'_LABEL']==article)
        end
    )lua","@installed-Civilopedia-original-home-population");
    check(lower_calls==16&&compare_calls==0,"Original home block must lowercase all sixteen actual selected names");
    host->execute("CivilopediaScreen",R"lua(
        table.sort(sortedList[CategoryHomePage][1],Alphabetically)
        for i=2,#sortedList[CategoryHomePage][1] do
            assert(Locale.Compare(sortedList[CategoryHomePage][1][i-1].entryName,sortedList[CategoryHomePage][1][i].entryName)~=1)
        end
        assert(not pcall(Locale.ToLower,123) and not pcall(Locale.ToLower,'a','b'))
        assert(not pcall(Locale.Compare,'a') and not pcall(Locale.Compare,'a','b','c'))
        Controls.CategoryButton1:RegisterCallback(Mouse.eLClick,function()
            assert(Locale.ToLower('ЦИВИЛОПЕДИЯ ЁЖ')=='цивилопедия ёж')
            assert(Locale.Compare('А','Б')==-1 and Locale.Compare('Б','А')==1 and Locale.Compare('А','А')==0)
        end)
    )lua","@installed-Civilopedia-actual-Alphabetically-predicate");
    check(compare_calls>0,"Original Alphabetically function must call the real selected comparator");
    rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    host->complete_loading();const auto event=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
    check(event.listeners==1&&event.errors.empty()&&lower_calls==17,"Ready owner callback must invoke actual UTF-8 text operations");
    host->close_ui_context("CivilopediaScreen");rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    {
        auto unrequested=vm();unrequested->execute("CivilopediaScreen","assert(Locale.ToLower==nil and Locale.Compare==nil)","=old-locale-frontier");
        rejects([&]{unrequested->enable_locale_text_operations("CivilopediaScreen",native);});
    }
    {
        auto invalid=vm();invalid->enable_locale_text_operations("CivilopediaScreen",native);
        invalid->execute("CivilopediaScreen","assert(not pcall(Locale.ToLower,string.char(0xC0,0xAF)))","=invalid-locale-UTF8");
        check(!invalid->loading_complete(),"Caught provider error must never grant VM readiness");rejects([&]{invalid->complete_loading();});
    }
    {
        auto invalid=vm();invalid->enable_locale_text_operations("CivilopediaScreen",{
            native.lower,[](auto,auto){return std::int64_t{2};}});
        invalid->execute("CivilopediaScreen","assert(not pcall(Locale.Compare,'A','B'))","=invalid-comparator-contract");
        rejects([&]{invalid->complete_loading();});
    }
    {
        auto stale=vm();stale->enable_locale_text_operations("CivilopediaScreen",native);
        auto changed=identity;++changed.game_turn;callback_owner->observe(changed);
        stale->execute("CivilopediaScreen","assert(not pcall(Locale.ToLower,'A'))","=stale-locale-context-owner");
        rejects([&]{stale->complete_loading();});
    }
}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed game root");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);const auto tags=selected_tags(root);
    check(tags.size()==16,"Actual selected XML category labels changed");
    auto index=std::make_shared<assets::LooseResourceIndex>(root);
    for(const auto *directory:{"Assets/Gameplay/XML/NewText/EN_US","Assets/Gameplay/XML/NewText/RU_RU",
        "Assets/DLC/Expansion/Gameplay/XML/Text/en_US","Assets/DLC/Expansion/Gameplay/XML/Text/ru_RU",
        "Assets/DLC/Expansion2/Gameplay/XML/Text/en_US","Assets/DLC/Expansion2/Gameplay/XML/Text/ru_RU"})index->scan({root/directory,{},true});
    assets::ResourceMountTable resources(assets::resource_lookup(index),assets::resource_lookup_all(index),assets::resource_keys(index));
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:locale-text-diagnostic";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;
    for(const auto *language:{"en_US","ru_RU"})for(const auto frame:{31,32}){
        identity.frame_number=frame;auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,language);
        auto owner=CivisLocaleTextOwner::open(locale,identity,language);const auto provider=owner->provider();
        const auto &policy=owner->policy();
        check(policy.backend=="Windows NLS linguistic lowercase / word collation"&&policy.language==language&&
              policy.locale_name==(std::string_view(language)=="en_US"?"en-US":"ru-RU")&&policy.casing_flags!=0&&
              policy.comparison_flags==0&&policy.nls_version!=0&&policy.defined_version!=0,
              "Explicit selected language and actual NLS version/flags must be recorded");
        check(provider.lower("CIVILOPEDIA HOME PAGE 123")=="civilopedia home page 123"&&
              provider.lower("ЦИВИЛОПЕДИЯ ЁЖ 123")=="цивилопедия ёж 123"&&
              provider.lower("АБВ abc 😃")=="абв abc 😃"&&provider.lower("").empty(),
              "Real UTF-8 Latin/Cyrillic/supplementary text must map linguistically without ASCII fallback");
        check(provider.compare("Alpha","Beta")==-1&&provider.compare("Beta","Alpha")==1&&provider.compare("Alpha","Alpha")==0&&
              provider.compare("А","Б")==-1&&provider.compare("Б","А")==1&&provider.compare("я","я")==0&&
              provider.compare("","")==0&&provider.compare("","А")==-1&&provider.compare("А","")==1,
              "Comparator must supply the authored -1/0/1 contract for real EN/RU and empty text");
        std::vector<std::string> labels;
        const auto selected_locale=locale->provider();
        for(const auto &tag:tags){
            const auto label=selected_locale.convert_text_key(tag,{});labels.push_back(label);
            check(!label.empty()&&label!=tag&&provider.lower(provider.lower(label))==provider.lower(label),
                  "Actual source-resolved category labels must lowercase consistently");
        }
        check(labels.front()==(std::string_view(language)=="en_US"?"Civilopedia Home Page":"Цивилопедия")&&
              labels.back()==(std::string_view(language)=="en_US"?"World Congress":"Всемирный конгресс"),
              "Installed selected EN/RU payloads must be tested rather than key echoes");
        for(const auto &a:labels)for(const auto &b:labels){
            const auto forward=provider.compare(a,b),reverse=provider.compare(b,a);
            check((forward==-1||forward==0||forward==1)&&forward==-reverse,"Selected label collation must be antisymmetric with authored comparator range");
        }
        // Exact predicate used by original Alphabetically(a,b), applied to
        // real selected names. Original engine ordering remains to be compared.
        std::sort(labels.begin(),labels.end(),[&](const auto &a,const auto &b){return provider.compare(a,b)==-1;});
        for(std::size_t i=1;i<labels.size();++i)check(provider.compare(labels[i-1],labels[i])!=1,"Original Alphabetically predicate must form a consistent selected collation");
        for(const auto &malformed:{std::string("\xc0\xaf",2),std::string("\xed\xa0\x80",3),std::string("\xf4\x90\x80\x80",4),std::string("\xe2\x82",2),std::string("x\0y",3)}){
            rejects([&]{provider.lower(malformed);});rejects([&]{provider.compare(malformed,"valid");});
        }
        lua_checks(root,sources,identity,locale,owner,frame==31?1280:1920,frame==31?960:1080);
        bool wrong_thread=false;std::thread other([&]{try{provider.lower("ABC");}catch(const ScriptError &){wrong_thread=true;}});other.join();
        check(wrong_thread,"Locale text operations require their owner thread");
        rejects([&]{CivisLocaleTextOwner::open(locale,identity,"xx_XX");});
        rejects([&]{CivisLocaleTextOwner::open(locale,identity,std::string_view(language)=="en_US"?"ru_RU":"en_US");});
        std::cout<<language<<" frame="<<frame<<" actualLabels="<<labels.size()<<" NLS="<<policy.nls_version<<" defined="<<policy.defined_version
            <<" firstSorted="<<labels.front()<<" lastSorted="<<labels.back()<<" (Civis policy, native ordering unverified)\n";
        auto changed=identity;++changed.game_turn;locale->observe(changed);
        rejects([&]{provider.lower("ABC");});rejects([&]{provider.compare("A","B");});owner->close();
    }
    {
        auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,"en_US");
        auto owner=CivisLocaleTextOwner::open(locale,identity,"en_US");const auto provider=owner->provider();owner->close();rejects([&]{provider.lower("ABC");});
    }
    {
        auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,"en_US");
        auto owner=CivisLocaleTextOwner::open(locale,identity,"en_US");const auto provider=owner->provider();locale.reset();rejects([&]{provider.compare("A","B");});
    }
    {
        auto locale=SelectedCivilopediaLocale::open(root,resources,sources,identity,"en_US");
        auto owner=CivisLocaleTextOwner::open(locale,identity,"en_US");const auto provider=owner->provider();owner.reset();rejects([&]{provider.lower("ABC");});
    }
    std::cout<<"Selected EN/RU Windows NLS text provider and explicit Lua capability passed: original home/sort source, real UTF-8/category payloads, invalid input/language and lifecycle guards. Native collation parity remains unverified.\n";
}
