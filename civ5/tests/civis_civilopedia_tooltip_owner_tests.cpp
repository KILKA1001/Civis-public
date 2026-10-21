#include "civ5/ui/civis_civilopedia_tooltip_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace civ5;using namespace civ5::ui;using namespace civ5::runtime;
namespace {
void check(bool value,const char *reason){if(!value)throw std::runtime_error(reason);}
template<class F>void rejects(F call){bool rejected=false;try{call();}catch(const std::exception &){rejected=true;}check(rejected,"Expected explicit tooltip rejection");}
ScriptUiLayoutProvider layout(const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,
    const CivilopediaLayoutOwnerKey &identity,const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner,int width,int height){
    ScriptUiLayoutProvider result;result.context_id="CivilopediaScreen";
    result.screen_size=[=]{return std::pair<std::int64_t,std::int64_t>{width,height};};
    result.get_size=[](auto)->std::pair<std::int64_t,std::int64_t>{throw ScriptError("Unrequested diagnostic geometry");};
    result.set_size=[](auto,auto,auto){throw ScriptError("Unrequested diagnostic geometry");};
    result.set_offset=[](auto,auto,auto){throw ScriptError("Unrequested diagnostic geometry");};
    result.calculate_internal_size=[](auto){throw ScriptError("Unrequested diagnostic geometry");};
    bind_civis_civilopedia_category_callbacks(result,root,sources,identity,owner,77);return result;
}
std::unique_ptr<LuaScriptHost> vm(ScriptUiLayoutProvider layout,const std::shared_ptr<CivisCivilopediaTooltipOwner> &tooltip){
    ScriptEnumProvider enums;enums.tables={{"Mouse",{{"eLClick",77}}}};
    auto result=std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},
        ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},ScriptUiEventProvider{},ScriptContentProvider{},
        ScriptUserDataProvider{},std::move(enums),ScriptLocaleProvider{},std::move(layout));result->enable_ui_tooltip(tooltip->provider());return result;
}
}
int main(int argc,char **argv){
    check(argc==3,"Expected selected Gandhi save and installed game root");const std::filesystem::path root=argv[2];
    auto selected=bootstrap_selected_bnw_preview(std::filesystem::path(argv[1]),root,0);
    check(static_cast<bool>(selected.preview),"Actual selected bootstrap failed");
    const auto catalog=content::discover_packages({root/"Assets/DLC"});
    auto plan=content::build_selected_loose_resource_plan(root,selected.profile.enabled_dlc,catalog);
    // Same selected loose plan and ordered explicit FPK overlay mechanism as
    // production UI: base archive is nonpreferred when a plan is present.
    const assets::AssetEnvironment environment(root);std::uint64_t mount_id=UINT64_MAX;
    for(const auto *relative:{"Resource/DX9/UITextures.fpk","Resource/DX9/Expansion1UITextures.fpk","Resource/DX9/Expansion2UITextures.fpk"}){
        const auto opened=environment.open_fpk_archive(relative);const auto *archive=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
        check(archive!=nullptr,"Selected real UI FPK is missing");
        while(std::any_of(plan.resources.mounts().begin(),plan.resources.mounts().end(),[&](const auto &mount){return mount.id==mount_id;}))--mount_id;
        plan.resources.mount({mount_id--,0,0,assets::resource_lookup(*archive),true,std::string_view(relative)!="Resource/DX9/UITextures.fpk",
            assets::resource_lookup_all(*archive),assets::resource_keys(*archive)});
    }
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto begin=source.find("local tipControlTable = {};");const auto end=source.find("function TipHandler( control )",begin);
    check(begin!=source.npos&&end!=source.npos,"Selected tooltip source registration changed");const auto registration=source.substr(begin,end-begin);
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:selected-tooltip-diagnostic";
    identity.player_id=0;identity.game_turn=selected.preview.host->session().state().setup.game_turn;
    identity.payload_sha256=selected.preview.host->identity().payload_sha256;identity.rules_sha256=selected.preview.host->identity().rules_sha256;
    identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;
    for(const auto [width,height]:{std::pair{1280,960},std::pair{1920,1080}}){
        identity.frame_number=width==1280?31:32;
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto tooltip=CivisCivilopediaTooltipOwner::open(root,sources,identity,owner,&plan.resources);
        const auto authored=tooltip->snapshot();check(authored.size()==2&&tooltip->type_name()=="TypeRoundImage"&&!tooltip->render_receipt_verified(),
            "Selected XML must declare one type/two images while renderer receipt remains pending");
        check(authored[0].id=="ToolTipFrame"&&authored[0].texture=="64x64FrameButtons.dds"&&authored[1].id=="ToolTipImage"&&
              authored[1].texture=="UnitPortraitsAncientMedieval512.dds"&&!authored[0].selected_texture&&!authored[1].selected_texture,
              "Authored state must retain real XML texture names without inventing loaded handles");
        auto host=vm(layout(root,sources,identity,owner,width,height),tooltip);
        host->execute("CivilopediaScreen",registration+R"lua(
            local again={};TTManager:GetTypeControlTable("TypeRoundImage",again)
            assert(again.ToolTipFrame==tipControlTable.ToolTipFrame and again.ToolTipImage==tipControlTable.ToolTipImage)
            assert(tipControlTable.ToolTipFrame.kind=="Image" and tipControlTable.ToolTipImage.id=="ToolTipImage")
            assert(not pcall(function() TTManager:GetTypeControlTable("UnknownType",{}) end))
            Controls.CategoryButton1:RegisterCallback(Mouse.eLClick,function()
                tipControlTable.ToolTipFrame:SetTexture("64x64FrameButtons.dds")
                tipControlTable.ToolTipImage:SetTexture("UnitPortraitsAncientMedieval512.dds")
                tipControlTable.ToolTipImage:SetTextureOffset({x=0,y=0})
                tipControlTable.ToolTipFrame:SetHide(true)
            end)
        )lua","@installed-BNW-tooltip-registration-and-owned-setters");
        rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});host->complete_loading();
        const auto event=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
        check(event.listeners==1&&event.errors.empty(),"Real tooltip mutation must select original-game textures");
        const auto captured=tooltip->snapshot();
        check(tooltip->registrations()==2&&captured[0].hidden&&captured[0].selected_texture&&captured[1].selected_texture&&
              captured[0].retained_source&&captured[1].retained_source&&!tooltip->render_receipt_verified(),
              "Shared Lua tables must mutate captured state and retain genuine texture payloads without renderer readiness");
        for(const auto &state:captured){
            const auto &texture=*state.selected_texture;
            check(texture.source.sha256.size()==64&&texture.source.candidate.provenance.archive_relative_path&&
                  assets::normalize_asset_name(*texture.source.candidate.provenance.archive_relative_path).starts_with("resource/dx9/")&&
                  texture.dds.width()>0&&texture.dds.height()>0&&state.blocker.find("render receipt pending")!=std::string::npos,
                  "Resolved retained DDS must expose real FPK provenance and pending render receipt");
            std::cout<<state.id<<" "<<*texture.source.candidate.provenance.archive_relative_path<<" "<<texture.source.sha256<<"\n";
        }
        if(const auto selected_mount=captured[0].selected_texture->source.mount_id){
            plan.resources.set_enabled(*selected_mount,false);rejects([&]{tooltip->snapshot();});plan.resources.set_enabled(*selected_mount,true);
        }
        bool wrong_thread=false;std::thread other([&]{try{tooltip->snapshot();}catch(const ScriptError &){wrong_thread=true;}});other.join();
        check(wrong_thread,"Tooltip captured state requires its owner thread");
        auto changed=identity;++changed.game_turn;owner->observe(changed);rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
        rejects([&]{tooltip->snapshot();});tooltip->close();host->close_ui_context("CivilopediaScreen");
    }
    {
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto tooltip=CivisCivilopediaTooltipOwner::open(root,sources,identity,owner);
        const auto provider=tooltip->provider();rejects([&]{provider.set_texture("ToolTipImage","UnitPortraitsAncientMedieval512.dds");});
        check(tooltip->snapshot()[1].blocker.find("ResourceMountTable is absent")!=std::string::npos&&!tooltip->snapshot()[1].selected_texture,
              "Absent texture service must preserve explicit pending payload/blocker");
        rejects([&]{provider.set_texture_offset("ToolTipImage",0,0);});
        tooltip->close();rejects([&]{provider.set_hidden("ToolTipFrame",true);});
    }
    {
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto tooltip=CivisCivilopediaTooltipOwner::open(root,sources,identity,owner,&plan.resources);
        auto host=vm(layout(root,sources,identity,owner,1280,960),tooltip);
        host->execute("CivilopediaScreen",registration+"Controls.CategoryButton1:RegisterCallback(77,function() tipControlTable.ToolTipImage:SetTexture('CivisMissingTexture.dds') end)","=missing-tooltip-texture");host->complete_loading();
        const auto result=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
        check(result.listeners==1&&!result.errors.empty()&&!host->loading_complete()&&host->registered_ui_control_callbacks("CivilopediaScreen")==0,
              "Missing real texture must fail honestly and retire retained VM callbacks");
        check(!tooltip->snapshot()[1].selected_texture&&tooltip->snapshot()[1].blocker.find("CivisMissingTexture")!=std::string::npos,
              "Failed texture mutation must retain its exact blocker without fake payload");
    }
    {
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto tooltip=CivisCivilopediaTooltipOwner::open(root,sources,identity,owner);
        const auto provider=tooltip->provider();tooltip.reset();rejects([&]{provider.registered();});
    }
    std::cout<<"Selected tooltip tables/texture handles passed in both viewport fixtures; missing/changed/stale/closed/expired guards; renderer receipt pending.\n";
}
