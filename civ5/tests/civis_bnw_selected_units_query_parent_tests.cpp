#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/civis_civilopedia_units_query_owner.hpp"
#include "civ5/ui/civis_civilopedia_geometry.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <iostream>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string_view>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
LiveSessionState after_post_traits(ui::SelectedBnwPreviewHost &host,
                                  const content::InfoTypeCatalog &types) {
    auto state=host.session().state();
    constexpr std::int64_t agriculture=0;
    state.technologies[63].set_has_tech(agriculture,true);
    std::vector<std::int64_t> *hashes=nullptr,*flags=nullptr;
    for(auto &array:state.world.teams->teams[63].arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array.values;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array.values;
    }
    check(hashes && flags,"Missing raw technology arrays");
    std::size_t found=0;
    for(std::size_t i=0;i<hashes->size();++i)
        if((*hashes)[i]==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            (*flags)[i]=1;++found;
        }
    check(found==1,"Ambiguous Agriculture raw technology row");
    const auto traits=run_bnw_barbarian_trait_reset(state,
        host.technologies(),host.database(),types,host.defines(),agriculture);
    check(traits.applied && !traits.grant_complete,"BNW trait prefix unavailable");
    const auto post=run_bnw_barbarian_post_traits(state,
        host.technologies(),host.database(),types,agriculture);
    check(post.applied && !post.grant_complete,"BNW post-trait prefix unavailable");
    return state;
}
void verify_selected_click_block(const std::filesystem::path &root){
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto blocker=source.find("Controls.SearchEditBox:RegisterCallback( OnSearchTextEnter );");
    check(blocker!=source.npos,"Selected EditBox callback source changed");
    for(const auto *id:{"CloseButton","BackButton","ForwardButton","SearchButton"}){
        const auto call=source.find("Controls."+std::string(id)+":RegisterCallback( Mouse.eLClick,");
        check(call!=source.npos&&call<blocker,"Selected click Button block before EditBox changed");
    }
    const auto ok=source.find("Controls.OK:RegisterCallback(Mouse.eLClick,");
    check(ok!=source.npos&&ok>blocker,"Selected later OK click callback order changed");
    for(const auto *name:{"SearchForPediaEntry","GoToPediaHomePage"}){
        const auto call=source.find("Events."+std::string(name)+".Add(");
        check(call!=source.npos&&call<blocker,"Selected Civilopedia listener block before EditBox changed");
    }
    const auto tooltip=source.find("TTManager:GetTypeControlTable( \"TypeRoundImage\", tipControlTable );");
    check(tooltip!=source.npos&&tooltip>ok,"Selected tooltip service order changed");
    for(const auto *name:{"GameplaySetActivePlayer","MultiplayerGameLobbyInvite","MultiplayerGameServerInvite"}){
        const auto call=source.find("Events."+std::string(name)+".Add(");
        check(call!=source.npos&&call>blocker&&call<tooltip,"Selected later Civilopedia listener source changed");
    }
}
std::size_t reference_frontier(const std::filesystem::path &root,
                               const content::RulesDatabase &database) {
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto begin=source.find("local conceptSections = {");
    check(begin!=source.npos,"Selected concept section declaration missing");
    const auto end=source.find('}',begin);
    check(end!=source.npos,"Selected concept section declaration incomplete");
    const auto declaration=source.substr(begin,end-begin);
    std::set<std::string> headers;
    for(auto offset=declaration.find("HEADER_");offset!=declaration.npos;
        offset=declaration.find("HEADER_",offset)) {
        const auto finish=declaration.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789",offset);
        headers.insert(declaration.substr(offset,finish-offset));
        offset=finish;
    }
    check(headers.size()==25,"Selected original concept section map changed");
    const auto xml=content::read_xml(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml");
    std::set<std::string> categories;
    const auto collect=[&](const auto &self,const content::XmlNode &node)->void {
        const auto id=node.attribute("ID");
        if(id.starts_with("CategoryButton"))categories.insert(id);
        for(const auto &child:node.children)self(self,child);
    };
    collect(collect,xml);
    check(categories.size()==16,"Selected authored category button count changed");
    std::size_t concepts=0;
    database.query({"SELECT CivilopediaHeaderType FROM Concepts",{}},
        [&](std::span<const content::DatabaseValue> row) {
            check(row.size()==1,"Selected concept header projection changed");
            const auto *header=std::get_if<std::string>(&row.front());
            if(header&&headers.contains(*header))++concepts;
        });
    check(concepts==503,"Selected source-filtered concept row count changed");
    // Original category setup and home population translate each category
    // once; Concepts then translates each row admitted by its authored map.
    return 2*categories.size()+concepts;
}
std::size_t selected_technology_translations(const content::RulesDatabase &database){
    std::size_t count=0;
    // The installed source iterates each selected Era and then Technologies
    // whose Era matches that row. Each admitted technology translates once.
    database.query({"SELECT Technologies.Description FROM Technologies JOIN Eras ON Technologies.Era=Eras.Type",{}},
        [&](std::span<const content::DatabaseValue> row){
            check(row.size()==1&&std::get_if<std::string>(&row.front()),"Selected technology description projection changed");
            ++count;
        });
    check(count==81,"Selected original source technology population changed");
    return count;
}
std::size_t selected_faith_unit_translations(const content::RulesDatabase &database){
    std::size_t count=0;
    // Exact installed first Units population filter, before DB.CreateQuery.
    database.query({"SELECT Description FROM Units WHERE FaithCost>0 AND Cost=-1 AND NOT RequiresFaithPurchaseEnabled AND ShowInPedia=1",{}},
        [&](std::span<const content::DatabaseValue> row){
            check(row.size()==1&&std::get_if<std::string>(&row.front()),"Selected faith-unit description projection changed");++count;
        });
    return count;
}
}
int run(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    const auto root=std::filesystem::path(argv[2]);
    verify_selected_click_block(root);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]),root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.profile.can_activate &&
          bootstrap.profile.enabled_dlc.size()==14 &&
          bootstrap.profile.enabled_mods.empty(),
          "Selected Gandhi stock profile changed");
    const auto catalog=content::discover_packages({root/"Assets/DLC"});
    auto tooltip_plan=content::build_selected_loose_resource_plan(root,bootstrap.profile.enabled_dlc,catalog);
    const assets::AssetEnvironment texture_environment(root);std::uint64_t texture_mount=UINT64_MAX;
    for(const auto *relative:{"Resource/DX9/UITextures.fpk","Resource/DX9/Expansion1UITextures.fpk","Resource/DX9/Expansion2UITextures.fpk"}){
        const auto opened=texture_environment.open_fpk_archive(relative);const auto *source=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
        check(source!=nullptr,"Selected actual tooltip FPK unavailable");
        while(std::any_of(tooltip_plan.resources.mounts().begin(),tooltip_plan.resources.mounts().end(),[&](const auto &mount){return mount.id==texture_mount;}))--texture_mount;
        tooltip_plan.resources.mount({texture_mount--,0,0,assets::resource_lookup(*source),true,std::string_view(relative)!="Resource/DX9/UITextures.fpk",
            assets::resource_lookup_all(*source),assets::resource_keys(*source)});
    }
    auto &host=*bootstrap.preview.host;
    std::size_t selected_slinger_languages=0;
    host.database().query({"SELECT Language,Text,Gender,Plurality FROM Localization.LocalizedText WHERE Tag='TXT_KEY_UNIT_INCAN_SLINGER' AND Language IN ('en_US','ru_RU') ORDER BY Language",{}},[&](auto row){
        check(row.size()==4&&std::get_if<std::string>(&row[0])&&std::get_if<std::string>(&row[1]),"Selected Slinger localization DB row is malformed");
        ++selected_slinger_languages;
        std::cout<<"selected-db Slinger "<<std::get<std::string>(row[0])<<" text="<<std::get<std::string>(row[1])<<std::endl;
    });
    check(selected_slinger_languages==2,"Selected attached Localization must contain exact EN/RU Slinger rows");
    bool selected_inca=false;
    for(const auto index:bootstrap.profile.enabled_dlc){
        const auto &package=catalog.packages.at(index);
        std::cout<<"selected-dlc "<<content::canonical_guid(package.id)<<" manifest="<<package.path.string()<<std::endl;
        if(content::canonical_guid(package.id)=="b685d5de-7cca-4e75-81b4-2f60754e6330"){
            content::PackageResourceQuery query;query.selector="TextData";query.enabled_categories={true,true,true};
            for(const auto &entry:content::package_resources(package,query))
                if(entry.value=="CIV5GameTextInfos_Units_DLC_02.xml")selected_inca=true;
        }
    }
    check(selected_inca,"Selected Inca manifest must declare actual Units TextData source");
    std::string first_era;
    host.database().query_rows({"SELECT * FROM Eras",{}},[&](const content::RulesRowView &row){if(first_era.empty())first_era=row.text("Type").value_or("");});
    check(first_era=="ERA_ANCIENT","Original first Units population era changed");
    std::ifstream units_source(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string units_lua{std::istreambuf_iterator<char>(units_source),std::istreambuf_iterator<char>()};
    const auto declared_sql=ui::CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(units_lua);
    std::size_t before_missing=0,missing_prefix=0;
    host.database().query({declared_sql,{first_era}},[&](auto row){
        if(missing_prefix)return;
        check(row.size()==4&&std::get_if<std::string>(&row[1]),"Source first Units SQL projection changed");
        ++before_missing;
        if(std::get<std::string>(row[1])=="TXT_KEY_UNIT_INCAN_SLINGER")missing_prefix=before_missing;
    });
    check(missing_prefix==10,"Source-selected Ancient Units prefix to Slinger changed");
    const auto expected_reference_labels=reference_frontier(root,host.database());
    check(expected_reference_labels==535,"Selected independently derived reference frontier changed");
    const auto expected_count_labels=expected_reference_labels+selected_technology_translations(host.database());
    check(expected_count_labels==616,"Selected independently derived count frontier changed");
    const auto selected_faith_units=selected_faith_unit_translations(host.database());
    check(selected_faith_units==2,"Selected source-filtered faith-unit population changed");
    BnwSelectedStockLuaSourceOwner owner(root,catalog,bootstrap.profile,
                                         host.identity());
    const auto &source=owner.evidence();
    check(source.selected_profile && source.first_include_executed &&
          source.declared_in_game_contexts>1 &&
          source.first_context_id=="CivilopediaScreen" &&
          source.declaration_sha256.size()==64 &&
          source.first_source_sha256.size()==64 &&
          source.first_include_sha256.size()==64 &&
          source.blocker.find("parent Lua chunk")!=std::string::npos,
          "Selected stock first include/source order evidence unavailable");
    const auto research_before=host.session().state().player_research.players.at(0).queue;
    const auto gameplay_calls=host.session().state().random.gameplay.state().calls;
    const auto map_calls=host.session().state().random.map.state().calls;
    const auto layout_sources=ui::inspect_bnw_civilopedia_layout_sources(root);
    check(layout_sources.verified,"Selected Civilopedia layout source unavailable");
    auto base_styles=std::make_shared<assets::LooseResourceIndex>(root);
    base_styles->add_file(root/"Assets/UI/Styles.xml");
    assets::ResourceMountTable style_resources(
        assets::resource_lookup(base_styles),
        assets::resource_lookup_all(base_styles));
    auto text_index=std::make_shared<assets::LooseResourceIndex>(root);
    for(const auto &directory:{"Assets/Gameplay/XML/NewText/EN_US","Assets/Gameplay/XML/NewText/RU_RU",
        "Assets/DLC/Expansion/Gameplay/XML/Text/en_US","Assets/DLC/Expansion/Gameplay/XML/Text/ru_RU",
        "Assets/DLC/Expansion2/Gameplay/XML/Text/en_US","Assets/DLC/Expansion2/Gameplay/XML/Text/ru_RU"})
        text_index->scan({root/directory,{},true});
    assets::ResourceMountTable text_resources(assets::resource_lookup(text_index),assets::resource_lookup_all(text_index),assets::resource_keys(text_index));
    for(const auto [width,height]:std::array<std::pair<std::int32_t,std::int32_t>,2>{
            std::pair{1280,960},std::pair{1920,1080}}) {
        ui::CivilopediaMeasuredLayout measured;
        measured.frame_number=width==1280?31:32;
        measured.owner_token="CivilopediaScreen:selected-frame-"+
            std::to_string(measured.frame_number);
        measured.player_id=0;
        measured.game_turn=host.session().state().setup.game_turn;
        measured.payload_sha256=host.identity().payload_sha256;
        measured.rules_sha256=host.identity().rules_sha256;
        measured.resolved_styles_sha256=layout_sources.styles_sha256;
        measured.viewport_width=width;
        measured.viewport_height=height;
        measured.scrollbar_width=18;
        measured.scroll_panel_width=1002;
        measured.scroll_panel_height=980;
        measured.left_panel_width=294;
        measured.left_panel_height=980;
        std::vector<std::string> measured_panels;
        const ui::CivisCivilopediaScrollMeasurementProvider renderer=
            [&](const auto &current,std::string_view id,const auto &viewport) {
                measured_panels.emplace_back(id);
                ui::CivisMeasuredScrollSnapshot snapshot;
                snapshot.owner=current;
                snapshot.panel_id=std::string(id);
                snapshot.geometry_revision=1;
                snapshot.viewport_width=viewport.width;
                snapshot.viewport_height=viewport.height;
                snapshot.participants_complete=true;
                // Explicit resolved fixture checks the adapter; it is not an
                // original renderer measurement or a native visual proof.
                snapshot.children={{"ResolvedContent",true,
                    ui::CivisMeasuredRect{0,0,200,1200}}};
                return snapshot;
            };
        const auto locale_plan=ui::plan_bnw_civilopedia_native_layout(layout_sources,host.identity(),
            0,host.session().state().setup.game_turn,measured.frame_number,measured);
        for(const auto *language:{"en_US","ru_RU"}){
            auto option_locale=ui::SelectedCivilopediaLocale::open(root,text_resources,layout_sources,locale_plan.owner,language);
            option_locale->enable_plain_reference_expansion();
            const auto option_probe=owner.probe_parent_chunk(host,style_resources,measured,renderer,77,option_locale,
                BnwCivilopediaGameOptionCapability::civis_selected_no_religion,&tooltip_plan.resources);
            check(option_probe.option.has_value(),"Selected Gandhi option proof missing");
            const auto &option=*option_probe.option;const auto &option_parent=option_probe.parent;
            std::cout<<"gameoption "<<language<<" "<<width<<"x"<<height<<" value="<<option.value
                <<" origin="<<static_cast<int>(option.origin)<<" ordinal="<<(option.saved_ordinal?std::to_string(*option.saved_ordinal):"none")
                <<" labels="<<option_parent.resolved_locale_labels.size()<<" faithUnits="<<selected_faith_units
                <<" next="<<option_parent.first_missing_api<<" blocker="<<option_parent.blocker<<std::endl;
            const auto expected_option_labels=expected_count_labels+(option.value==1?0:selected_faith_units);
            check(option.token==21&&option.key=="GAMEOPTION_NO_RELIGION"&&option.rules_sha256==host.identity().rules_sha256&&
                  option.value==0&&option.origin==ui::CivisGameOptionResolutionOrigin::selected_default&&!option.saved_ordinal&&
                  option_parent.selected_source_verified&&!option_parent.full_parent_loaded&&option_parent.layout_operations_completed==11&&
                  option_parent.registered_category_callbacks==16&&option_parent.registered_control_callbacks==22&&
                  option_parent.registered_text_callbacks==1&&option_parent.registered_context_handlers==2&&
                  option_parent.civilopedia_listeners==5&&option_parent.tooltip_type_registrations==1&&option_parent.tooltip_control_tables==2&&
                  !option_parent.tooltip_render_receipt_verified&&option_parent.resolved_locale_labels.size()==expected_option_labels&&
                  expected_option_labels==618&&
                  option_parent.first_missing_api=="DB.CreateQuery"&&option_parent.blocker.find(":480:")!=std::string::npos&&
                  option_parent.blocker.find("attempt to call field 'CreateQuery' (a nil value)")!=std::string::npos,
                  "Selected diagnostic Game option must execute the original faith-unit branch then stop at real CreateQuery480 without publication");
            auto units_locale=ui::SelectedCivilopediaLocale::open(root,text_resources,layout_sources,locale_plan.owner,language);
            units_locale->enable_plain_reference_expansion();
            const auto units_probe=owner.probe_parent_chunk(host,style_resources,measured,renderer,77,units_locale,
                BnwCivilopediaUnitsQueryCapability::civis_first_units_statement,&tooltip_plan.resources);
            const auto &units_parent=units_probe.parent;
            std::cout<<"unitsquery "<<language<<" "<<width<<"x"<<height
                <<" prepared="<<units_probe.query.prepared<<" invocations="<<units_probe.query.invocations
                <<" rows="<<units_probe.query.rows<<" eof="<<units_probe.query.eof
                <<" labels="<<units_parent.resolved_locale_labels.size()<<" next="<<units_parent.first_missing_api
                <<" blocker="<<units_parent.blocker<<std::endl;
            check(units_probe.option&&units_probe.option->value==0&&
                units_probe.option->origin==ui::CivisGameOptionResolutionOrigin::selected_default&&
                units_parent.selected_source_verified&&!units_parent.full_parent_loaded&&
                units_parent.layout_operations_completed==11&&units_parent.registered_category_callbacks==16&&
                units_parent.registered_control_callbacks==22&&units_parent.registered_text_callbacks==1&&
                units_parent.registered_context_handlers==2&&units_parent.civilopedia_listeners==5&&
                !units_parent.tooltip_render_receipt_verified&&units_probe.query.prepared==1&&
                units_probe.query.invocations==1&&units_probe.query.rows==missing_prefix&&units_probe.query.eof==0&&
                units_parent.resolved_locale_labels.size()==expected_option_labels+missing_prefix-1&&
                units_parent.resolved_locale_labels.size()==627&&
                units_parent.first_missing_api=="Locale.ConvertTextKey"&&
                units_parent.blocker.find(":442:")!=std::string::npos&&
                units_parent.blocker.find("Civilopedia localized key is absent: TXT_KEY_UNIT_INCAN_SLINGER")!=std::string::npos,
                "First Units cursor must stop at selected three-layer locale Slinger442 after exact observed prefix, without publication");


        }
        measured.scrollbar_width=19;
        const auto wrong=owner.probe_parent_chunk(host,style_resources,measured);
        check(!wrong.selected_source_verified &&
              wrong.layout_operations_completed==0 &&
              !wrong.full_parent_loaded,
              "Mismatched measured style width must fail before Lua side effects");
    }
    check(host.session().state().player_research.players.at(0).queue==research_before &&
          host.session().state().random.gameplay.state().calls==gameplay_calls &&
          host.session().state().random.map.state().calls==map_calls,
          "Frame-owned Lua layout changed selected live world");
    auto state=after_post_traits(host,host.types());
    const auto fork=owner.fork_at(state);
    check(!fork.epoch && fork.blocker==source.blocker &&
          quote_bnw_team_tech_researched(state,bootstrap.profile,
              host.identity(),fork.epoch.get(),0).status==
              BnwTeamTechLuaStatus::blocked &&
          !state.setup.extensions.find<BnwTeamTechLuaAttemptMarker>(),
          "Incomplete stock Lua context was treated as an active listener epoch");
    const auto published=owner.publish({},state);
    check(!published.committed && !published.uncertain &&
          !state.setup.extensions.find<BnwTeamTechLuaAttemptMarker>(),
          "Incomplete stock Lua owner published a world or VM");

    auto incomplete=bootstrap.profile;
    const auto before=incomplete.enabled_dlc.size();
    std::erase_if(incomplete.enabled_dlc,[&](std::size_t index) {
        return content::canonical_guid(catalog.packages[index].id)==
               "6da07636-4123-4018-b643-6575b4ec336b";
    });
    check(incomplete.enabled_dlc.size()+1==before,
          "Selected Expansion2 manifest was not unique");
    BnwSelectedStockLuaSourceOwner incomplete_owner(root,catalog,incomplete,
                                                    host.identity());
    check(!incomplete_owner.evidence().first_include_executed &&
          !incomplete_owner.fork_at(state).epoch &&
          !incomplete_owner.probe_parent_chunk(host).selected_source_verified,
          "Incomplete selected DLC composition produced a Lua epoch");
    return 0;
}

int main(int argc,char **argv) {
    try { return run(argc,argv); }
    catch(const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n';return 1; }
    catch(...) { std::cerr<<"FAIL unexpected selected Units parent error\n";return 1; }
}
