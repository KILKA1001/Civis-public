#pragma once
#include "civ5/graphics/bnw_client_capture.hpp"
#include "civ5/graphics/bnw_ui_source_observation.hpp"
#include "civ5/ui/bnw_live_preview_launch.hpp"
#include "material_preview_settings.hpp"
#include <map>
#include <windows.h>

// Validation producer: one actual composed readback per run, with immutable
// selected-source and applied-setting evidence. Never asserts original parity.
namespace bnw_capture_evidence {
inline std::string utc_now() {
    SYSTEMTIME t{}; GetSystemTime(&t); char b[40]{};
    std::snprintf(b,sizeof(b),"%04u-%02u-%02uT%02u:%02u:%02u.%03u+00:00",
        unsigned(t.wYear),unsigned(t.wMonth),unsigned(t.wDay),unsigned(t.wHour),
        unsigned(t.wMinute),unsigned(t.wSecond),unsigned(t.wMilliseconds));
    return b;
}
inline std::string utf8(const std::filesystem::path &p) {
    const auto s=p.generic_u8string(); return {s.begin(),s.end()};
}
inline void write(const std::filesystem::path &p,std::string_view s) {
    civ5::graphics::capture_detail::exclusive_write(p,{reinterpret_cast<const std::uint8_t*>(s.data()),s.size()});
}
inline std::string spec(const std::filesystem::path &p) {
    return "{\"path\":"+civ5::graphics::capture_detail::quote(utf8(std::filesystem::absolute(p)))+
        ",\"sha256\":"+civ5::graphics::capture_detail::quote(civ5::runtime::identify_file(p).sha256)+"}";
}
inline std::string normalized_settings(const material_preview_settings::Profile &p) {
    const auto &s=p.configured.material; const auto &q=s.quality;
    std::map<std::string,std::string> v;
    const auto number=[&](std::string k,auto n){v.emplace(std::move(k),std::to_string(n));};
    number("adapterRule",s.adapter_rule_index); number("baseSize",q.base_size);
    number("blockOnLoad",p.configured.block_on_load); number("cacheMode",q.cache_mode);
    number("cacheSize",q.cache_size); number("compression",q.compression); number("detail",s.terrain_detail);
    number("deviceId",p.adapter.device_id); number("flags",s.flags); number("gameLoaded",p.game_loaded);
    number("graphicsLoaded",p.graphics_loaded); number("minimalBuildOnLoad",p.loading.minimal_build_on_load);
    number("minimizeGrayTiles",p.configured.minimize_gray_tiles); number("movingBudget",s.moving_budget);
    number("os32",p.adapter.os_32bit); number("pixelShaderVersion",p.capabilities.pixel_shader_version);
    number("preset",s.preset); number("ps20Caps",p.capabilities.ps20_caps);
    number("ps20NumTemps",p.capabilities.ps20_num_temps); number("rendererCode",p.adapter.renderer_code);
    number("stillBudget",s.still_budget); number("temporarySize",q.temporary_size);
    number("tessellation",q.tessellation); number("vendorId",p.adapter.vendor_id);
    v["textureQualityOverride"]=p.texture_quality_override?std::to_string(*p.texture_quality_override):"null";
    v["pools"]="["; for(std::size_t i=0;i<q.pool_counts.size();++i) v["pools"]+=(i?",":"")+std::to_string(q.pool_counts[i]); v["pools"]+=']';
    std::string out="{"; for(const auto &[k,value]:v) {if(out.size()>1)out+=','; out+='"'+k+"\":"+value;} return out+'}';
}
class Producer {
    std::filesystem::path prefix_,payload_,rules_,gamecore_,activation_,settings_,report_,scene_,save_;
    std::string language_; bool source_paths_allowed_=false,observed_=false,captured_=false;
    std::uint32_t cache_mode_=0,compression_=0;
    std::shared_ptr<civ5::graphics::BnwUiSourceObservation> ui_sources_=
        std::make_shared<civ5::graphics::BnwUiSourceObservation>();
public:
    Producer(const std::filesystem::path &out,const civ5::ui::BnwLivePreviewRequest &request,
        std::filesystem::path scene,const material_preview_settings::Profile &profile,
        std::string language,bool stock_ui):scene_(std::move(scene)),save_(request.save_path),language_(std::move(language)) {
        gamecore_=request.game_root/"Assets/DLC/Expansion2/CvGameCore_Expansion2.dll";
        auto tag=utc_now(); for(auto &c:tag)if(c==':'||c=='+')c='-';
        prefix_=out/("client-frame-"+tag+"-"+std::to_string(GetCurrentProcessId()));
        payload_=prefix_; payload_+=".payload.bin"; rules_=prefix_; rules_+=".rules.bin"; activation_=prefix_; activation_+=".activation.json";
        settings_=prefix_;settings_+=".settings.json"; report_=prefix_;report_+=".settings-report.json";
        write(settings_,normalized_settings(profile)); write(report_,material_preview_settings::report(profile));
        source_paths_allowed_=stock_ui;
        cache_mode_=profile.configured.material.quality.cache_mode;
        compression_=profile.configured.material.quality.compression;
    }
    const std::shared_ptr<civ5::graphics::BnwUiSourceObservation> &ui_sources() const { return ui_sources_; }
    void observe(const civ5::ui::SelectedBnwBootstrapResult &b) {
        if(observed_ || !b.preview.host || b.preview.blocker || !b.activation || !b.postprocess ||
           !b.activation->rules_database_ready || !b.postprocess->all_succeeded ||
           !b.postprocess->transaction_released || !b.profile.enabled_mods.empty())
            throw std::runtime_error("Actual selected stock activation unavailable for capture");
        const auto &host=*b.preview.host; const auto &bytes=host.session().source().source.bytes;
        if(civ5::runtime::identify_bytes(bytes).sha256!=host.identity().payload_sha256)
            throw std::runtime_error("Capture selected payload differs from its owner");
        civ5::graphics::capture_detail::exclusive_write(payload_,bytes);
        const auto rules=host.database().gameplay_fingerprint_data();
        civ5::graphics::capture_detail::exclusive_write(rules_,rules);
        if(civ5::runtime::identify_file(gamecore_).sha256!=b.gamecore_sha256)
            throw std::runtime_error("Capture selected GameCore source changed");
        const auto &a=*b.activation;
        write(activation_,"{\"schema\":\"civis-selected-capture-activation-v1\",\"rulesDatabaseReady\":true,"
            "\"backend\":\"OpenGL compatibility material preview\",\"postprocessSucceeded\":true,\"transactionReleased\":true,\"mods\":0,\"indexedResources\":"+
            std::to_string(b.indexed_resources)+",\"baseAppliedSources\":"+std::to_string(a.base.applied)+
            ",\"gamecoreSha256\":"+civ5::graphics::capture_detail::quote(b.gamecore_sha256)+
            ",\"rulesSha256\":"+civ5::graphics::capture_detail::quote(host.identity().rules_sha256)+"}");
        observed_=true;
    }
    civ5::graphics::BnwClientCaptureHook hook(const civ5::ui::LiveBnwPreviewInput *input) {
        return [this,input](const auto &owner,const auto &frame,const auto &scene,std::int32_t w,std::int32_t h,
            std::string_view context,float material_time,const civ5::graphics::BnwFogRenderedFrameReceipt *rendered_fog,const auto &readback) {
            if(captured_)return;
            if(!observed_||!input)throw std::runtime_error("Capture selected owner observation missing");
            civ5::graphics::BnwClientCaptureOptions options;
            options.output_prefix=prefix_;options.utc_now=utc_now;options.context=std::string(context);
            options.material_time=material_time;options.live_session=&input->session;options.rendered_fog=rendered_fog;
            const auto ui_before=ui_sources_->snapshot_json();
            const auto files=civ5::graphics::capture_bnw_client_rgb(owner,frame,scene,w,h,readback,options);
            const auto ui_after=ui_sources_->snapshot_json();
            auto ui_receipt=prefix_;ui_receipt+=".ui-sources.json";
            write(ui_receipt,"{\"schema\":\"civis-capture-ui-sources-v1\",\"native\":"+spec(files.observation)+
                ",\"before\":"+ui_before+",\"after\":"+ui_after+"}");
            ui_sources_->stop_recording();
            captured_=true;
            // Source paths alone cannot prove EUI exclusion: the loose source
            // can resolve an overridden stock filename. Retain this actual
            // observation, but the strict packager must reject unknown EUI.
            auto pending=prefix_;pending+=".metadata-pending.json";
            write(pending,"{\"schema\":\"civis-capture-pending-v1\",\"reason\":\"Independent stock UI selection and EUI exclusion proof unavailable\",\"sourcePathsAllowed\":"+
                std::string(source_paths_allowed_?"true":"false")+",\"native\":"+spec(files.observation)+"}");
            auto applied=prefix_;applied+=".applied-profile.json";
            write(applied,"{\"schema\":\"civis-applied-capture-profile-v1\",\"applied\":true,"
                "\"application\":\"Selected BNW activation and ScheduledMaterialPreview applied; backend OpenGL compatibility path\","
                "\"profile\":{\"game\":\"BNW\",\"renderer\":\"DX9\",\"eui\":null,\"cacheMode\":"+std::to_string(cache_mode_)+",\"compression\":"+std::to_string(compression_)+",\"settings_sha256\":"+
                civ5::graphics::capture_detail::quote(civ5::runtime::identify_file(settings_).sha256)+
                "},\"settings\":"+spec(settings_)+",\"sourceArtifacts\":["+spec(activation_)+","+spec(report_)+","+spec(rules_)+","+spec(gamecore_)+","+spec(ui_receipt)+"]}");
            std::array<wchar_t,32768> exe{};const auto n=GetModuleFileNameW(nullptr,exe.data(),static_cast<DWORD>(exe.size()));
            if(!n||n>=exe.size())throw std::runtime_error("Capture executable path unavailable");
            std::ostringstream animation;animation.imbue(std::locale::classic());animation<<std::setprecision(std::numeric_limits<float>::max_digits10)<<material_time;
            auto metadata=prefix_;metadata+=".metadata.json";
            write(metadata,"{\"schema\":\"civis-capture-metadata-v1\",\"executable\":"+spec(std::filesystem::path(exe.data()))+
                ",\"save\":"+spec(save_)+",\"ui_sources\":"+spec(ui_receipt)+",\"profile_evidence\":"+spec(applied)+",\"language\":"+
                civ5::graphics::capture_detail::quote(language_)+",\"context\":"+civ5::graphics::capture_detail::quote(context)+
                ",\"animation_phase\":{\"material_time\":"+animation.str()+"},\"action_sequence\":[{\"action\":\"load-selected-save\"},{\"action\":\"readback-initial-composed-frame\"}],\"sources\":["+spec(scene_)+","+spec(payload_)+"]}");
        };
    }
};
} // namespace bnw_capture_evidence
