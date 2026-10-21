#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/content/info_types.hpp"
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
int main(int argc,char **argv) {
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
    const auto parent=owner.probe_parent_chunk(host);
    check(parent.selected_source_verified && !parent.full_parent_loaded &&
          parent.manager_bindings==33 &&
          parent.invalidation_listeners==2 &&
          parent.civilopedia_listeners==0 &&
          parent.first_missing_api=="UIManager.GetScreenSizeVal" &&
          parent.observed_include_order==std::vector<std::string>{
              "Assets/UI/InstanceManager.lua", "Assets/UI/IconSupport.lua",
              "Assets/Gameplay/Lua/FLuaVector.lua"} &&
          parent.blocker.find("UIManager")!=std::string::npos,
          "Selected Civilopedia parent must stop at first missing native UI API");
    check(host.session().state().player_research.players.at(0).queue==research_before &&
          host.session().state().random.gameplay.state().calls==gameplay_calls &&
          host.session().state().random.map.state().calls==map_calls,
          "Diagnostic parent chunk changed selected live state");
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
        const auto bridged=owner.probe_parent_chunk(host,style_resources,measured);
        check(bridged.selected_source_verified && !bridged.full_parent_loaded &&
              bridged.viewport_width==width && bridged.viewport_height==height &&
              bridged.manager_bindings==33 &&
              bridged.invalidation_listeners==2 &&
              bridged.civilopedia_listeners==0 &&
              bridged.layout_operations_completed==9 &&
              bridged.scrollbar_width==18 &&
              bridged.scrollbar_height==height-172 &&
              bridged.scroll_panel_height==height-126 &&
              bridged.down_button_offset_y==height-144 &&
              bridged.scroll_subtree_verified &&
              bridged.scroll_nodes==303 &&
              bridged.content_roots==4 &&
              bridged.explicit_hidden==1 &&
              bridged.first_missing_api=="ScrollPanel.CalculateInternalSize" &&
              bridged.blocker.find("CalculateInternalSize")!=std::string::npos,
              "Frame-owned Lua layout must execute nine native-order setters and stop at internal size");
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
        const auto progressed=owner.probe_parent_chunk(host,style_resources,measured,renderer);
        check(progressed.selected_source_verified &&
              !progressed.full_parent_loaded &&
              progressed.layout_operations_completed==11 &&
              progressed.scroll_subtree_verified &&
              progressed.invalidation_listeners==2 &&
              progressed.civilopedia_listeners==0 &&
              progressed.category_callback_controls==0 &&
              progressed.registered_category_callbacks==0 &&
              progressed.first_missing_api=="Controls.CategoryButton1.RegisterCallback" &&
              measured_panels==std::vector<std::string>{"ScrollPanel","LeftScrollPanel"},
              "Measured parent must execute both calculations then report the real category callback boundary");
        measured_panels.clear();
        const auto callbacks=owner.probe_parent_chunk(host,style_resources,measured,renderer,77);
        check(callbacks.selected_source_verified&&!callbacks.full_parent_loaded&&
              callbacks.layout_operations_completed==11&&callbacks.scroll_subtree_verified&&
              callbacks.invalidation_listeners==2&&callbacks.civilopedia_listeners==0&&
              callbacks.category_callback_controls>1&&
              callbacks.registered_category_callbacks==callbacks.category_callback_controls&&
              callbacks.first_missing_api=="Locale.ConvertTextKey"&&
              callbacks.blocker.find("Locale")!=std::string::npos&&
              measured_panels==std::vector<std::string>{"ScrollPanel","LeftScrollPanel"},
              "Explicit Civis click capability must retain actual installed category Lua closures then stop at real Locale service");
        const auto missing_measurement=owner.probe_parent_chunk(host,style_resources,measured,{},77);
        check(missing_measurement.selected_source_verified&&!missing_measurement.full_parent_loaded&&
              missing_measurement.layout_operations_completed==9&&
              missing_measurement.category_callback_controls>1&&missing_measurement.registered_category_callbacks==0&&
              missing_measurement.first_missing_api=="ScrollPanel.CalculateInternalSize",
              "Category capability must preserve the earlier missing renderer service boundary");
        const auto locale_plan=ui::plan_bnw_civilopedia_native_layout(layout_sources,host.identity(),
            0,host.session().state().setup.game_turn,measured.frame_number,measured);
        for(const auto *language:{"en_US","ru_RU"}){
            measured_panels.clear();
            auto locale=ui::SelectedCivilopediaLocale::open(root,text_resources,layout_sources,locale_plan.owner,language);
            const auto localized=owner.probe_parent_chunk(host,style_resources,measured,renderer,77,locale);
            std::cout<<"locale "<<language<<" "<<width<<"x"<<height<<" category="<<localized.registered_category_callbacks
                <<" controls="<<localized.registered_control_callbacks<<" declared="<<localized.top_level_callback_controls
                <<" labels="<<localized.resolved_locale_labels.size()<<" listeners="<<localized.civilopedia_listeners
                <<" next="<<localized.first_missing_api<<"\n";
            check(localized.selected_source_verified&&!localized.full_parent_loaded&&localized.layout_operations_completed==11&&
                  localized.registered_category_callbacks==localized.category_callback_controls&&
                  localized.registered_control_callbacks==localized.registered_category_callbacks+4&&
                  localized.top_level_callback_controls==5&&localized.civilopedia_listeners==2&&
                  localized.resolved_locale_labels.size()==16&&
                  localized.first_missing_api=="Controls.SearchEditBox.RegisterCallback(OnSearchTextEnter)"&&
                  measured_panels==std::vector<std::string>{"ScrollPanel","LeftScrollPanel"},
                  "Real selected language must resolve all16labels and register additional authored click Buttons before unsupported EditBox overload");
            const auto &labels=localized.resolved_locale_labels;
            check(labels.front().language==language&&labels.front().tag=="TXT_KEY_PEDIA_CATEGORY_1_LABEL"&&
                  labels.front().text==(std::string_view(language)=="en_US"?"Civilopedia Home Page":"Цивилопедия")&&
                  labels.back().tag=="TXT_KEY_PEDIA_CATEGORY_16_LABEL"&&
                  labels.back().text==(std::string_view(language)=="en_US"?"World Congress":"Всемирный конгресс")&&
                  std::all_of(labels.begin(),labels.end(),[](const auto &label){return !label.text.empty()&&label.text!=label.tag&&
                      !label.source_key.empty()&&label.source_sha256.size()==64;}),
                  "Actual EN/RU payloads and source pins must survive complete category text block");
            measured_panels.clear();
            auto input_locale=ui::SelectedCivilopediaLocale::open(root,text_resources,layout_sources,locale_plan.owner,language);
            const auto inputs=owner.probe_parent_chunk(host,style_resources,measured,renderer,77,input_locale,
                BnwCivilopediaInputCapability::captured_civis_input);
            std::cout<<"input "<<language<<" "<<width<<"x"<<height<<" category="<<inputs.registered_category_callbacks
                <<" controls="<<inputs.registered_control_callbacks<<" text="<<inputs.registered_text_callbacks
                <<" context="<<inputs.registered_context_handlers<<" listeners="<<inputs.civilopedia_listeners
                <<" next="<<inputs.first_missing_api<<"\n";
            check(inputs.selected_source_verified&&!inputs.full_parent_loaded&&inputs.layout_operations_completed==11&&
                  inputs.category_callback_controls==16&&inputs.registered_category_callbacks==16&&
                  inputs.top_level_callback_controls==5&&inputs.registered_control_callbacks==22&&
                  inputs.registered_text_callbacks==1&&inputs.registered_context_handlers==2&&
                  inputs.invalidation_listeners==2&&inputs.civilopedia_listeners==5&&inputs.resolved_locale_labels.size()==16&&
                  inputs.first_missing_api=="TTManager.GetTypeControlTable(TypeRoundImage)"&&
                  measured_panels==std::vector<std::string>{"ScrollPanel","LeftScrollPanel"},
                  "Explicit captured input must retain real EditBox/OK/context callbacks and five authored listeners before tooltip service");
            measured_panels.clear();
            auto tooltip_locale=ui::SelectedCivilopediaLocale::open(root,text_resources,layout_sources,locale_plan.owner,language);
            const auto tooltip=owner.probe_parent_chunk(host,style_resources,measured,renderer,77,tooltip_locale,
                BnwCivilopediaTooltipCapability::source_declared_state,&tooltip_plan.resources);
            std::cout<<"tooltip "<<language<<" "<<width<<"x"<<height<<" types="<<tooltip.tooltip_type_registrations
                <<" controls="<<tooltip.tooltip_control_tables<<" labels="<<tooltip.resolved_locale_labels.size()<<" next="<<tooltip.first_missing_api<<"\n";
            check(tooltip.selected_source_verified&&!tooltip.full_parent_loaded&&tooltip.layout_operations_completed==11&&
                  tooltip.registered_category_callbacks==16&&tooltip.registered_control_callbacks==22&&tooltip.registered_text_callbacks==1&&
                  tooltip.registered_context_handlers==2&&tooltip.civilopedia_listeners==5&&tooltip.tooltip_type_registrations==1&&
                  tooltip.tooltip_control_tables==2&&!tooltip.tooltip_render_receipt_verified&&tooltip.resolved_locale_labels.size()==17&&
                  tooltip.first_missing_api=="Locale.ToLower"&&measured_panels==std::vector<std::string>{"ScrollPanel","LeftScrollPanel"},
                  "Selected tooltip table must execute original home population until actual lowercasing service, with rendering still pending");
            auto text_locale=ui::SelectedCivilopediaLocale::open(root,text_resources,layout_sources,locale_plan.owner,language);
            const auto text_operations=owner.probe_parent_chunk(host,style_resources,measured,renderer,77,text_locale,
                BnwCivilopediaLocaleTextCapability::civis_windows_nls,&tooltip_plan.resources);
            std::cout<<"NLS "<<language<<" "<<width<<"x"<<height<<" labels="<<text_operations.resolved_locale_labels.size()
                     <<" categories="<<text_operations.registered_category_callbacks<<" controls="<<text_operations.registered_control_callbacks
                     <<" text="<<text_operations.registered_text_callbacks<<" contexts="<<text_operations.registered_context_handlers
                     <<" next="<<text_operations.first_missing_api<<" blocker="<<text_operations.blocker<<"\n";
            check(text_operations.selected_source_verified&&!text_operations.full_parent_loaded&&text_operations.layout_operations_completed==11&&
                  text_operations.registered_category_callbacks==16&&text_operations.registered_control_callbacks==22&&
                  text_operations.registered_text_callbacks==1&&text_operations.registered_context_handlers==2&&
                  text_operations.civilopedia_listeners==5&&text_operations.tooltip_type_registrations==1&&
                  text_operations.tooltip_control_tables==2&&!text_operations.tooltip_render_receipt_verified&&
                  text_operations.resolved_locale_labels.size()==453&&text_operations.first_missing_api=="Locale.ConvertTextKey"&&
                  text_operations.blocker.find("TXT_KEY_CONCEPT_ESPIONAGE_SPIES_DESCRIPTION")!=std::string::npos,
                  "Explicit NLS must preserve the exact observed453-row original frontier before opt-in reference grammar");
            auto referenced_locale=ui::SelectedCivilopediaLocale::open(root,text_resources,layout_sources,locale_plan.owner,language);
            referenced_locale->enable_plain_reference_expansion();
            const auto references=owner.probe_parent_chunk(host,style_resources,measured,renderer,77,referenced_locale,
                BnwCivilopediaLocaleTextCapability::civis_windows_nls,&tooltip_plan.resources);
            std::cout<<"references "<<language<<" "<<width<<"x"<<height<<" labels="<<references.resolved_locale_labels.size()
                     <<" next="<<references.first_missing_api<<" blocker="<<references.blocker<<std::endl;
            check(references.selected_source_verified&&!references.full_parent_loaded&&references.layout_operations_completed==11&&
                  references.registered_category_callbacks==16&&references.registered_control_callbacks==22&&
                  references.registered_text_callbacks==1&&references.registered_context_handlers==2&&
                  references.civilopedia_listeners==5&&references.tooltip_type_registrations==1&&
                  references.tooltip_control_tables==2&&!references.tooltip_render_receipt_verified&&
                  references.resolved_locale_labels.size()==expected_reference_labels&&
                  references.first_missing_api=="table.count"&&references.blocker.find(":353:")!=std::string::npos&&
                  references.blocker.find("attempt to call field 'count' (a nil value)")!=std::string::npos,
                  "Opt-in source references must reach exact535 translations then the original table.count dependency at line353");
            auto counted_locale=ui::SelectedCivilopediaLocale::open(root,text_resources,layout_sources,locale_plan.owner,language);
            counted_locale->enable_plain_reference_expansion();
            const auto counted=owner.probe_parent_chunk(host,style_resources,measured,renderer,77,counted_locale,
                BnwCivilopediaTableCountCapability::civis_raw_immediate_keys,&tooltip_plan.resources);
            std::cout<<"rawcount "<<language<<" "<<width<<"x"<<height<<" labels="<<counted.resolved_locale_labels.size()
                     <<" next="<<counted.first_missing_api<<" blocker="<<counted.blocker<<std::endl;
            check(counted.selected_source_verified&&!counted.full_parent_loaded&&counted.layout_operations_completed==11&&
                  counted.registered_category_callbacks==16&&counted.registered_control_callbacks==22&&
                  counted.registered_text_callbacks==1&&counted.registered_context_handlers==2&&
                  counted.civilopedia_listeners==5&&counted.tooltip_type_registrations==1&&counted.tooltip_control_tables==2&&
                  !counted.tooltip_render_receipt_verified&&counted.resolved_locale_labels.size()==expected_count_labels&&
                  counted.first_missing_api=="GameOptionTypes.GAMEOPTION_NO_RELIGION"&&
                  counted.blocker.find(":463:")!=std::string::npos&&
                  counted.blocker.find("attempt to index global 'GameOptionTypes' (a nil value)")!=std::string::npos,
                  "Explicit Civis count must reach exact616 translations then original missing GameOptionTypes at line463 without publishing parent");
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
    // A normal selected world has no technology post-trait transient. Its
    // genuine incomplete-context reason must also apply to general hooks.
    const auto generic_fork=owner.fork_at(host.session().state());
    check(!generic_fork.epoch && generic_fork.blocker==source.blocker &&
          !host.session().state().setup.extensions.find<BnwTeamTechLuaAttemptMarker>(),
          "General stock Lua fork incorrectly depends on a technology grant");
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
}
