#pragma once
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/graphics/bnw_rendered_fog_frame.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <locale>
#include <limits>
#include <sstream>

namespace civ5::graphics {
using BnwClientCaptureHook = std::function<void(
    const ui::BnwClientFrameOwner &, const std::shared_ptr<const ui::BnwClientFrame> &,
    const BnwRendererSceneReceipt &, std::int32_t, std::int32_t, std::string_view, float, const BnwFogRenderedFrameReceipt *,
    const std::function<std::vector<std::uint8_t>()> &)>;
struct BnwClientCaptureOptions {
    std::filesystem::path output_prefix;
    // Offset-aware UTC ISO8601; sampled immediately around actual readback.
    std::function<std::string()> utc_now;
    // Optional additional camera/compositor validation. Semantic live state is
    // checked directly through the required borrowed session below.
    std::function<void()> validate_live_scene;
    std::string context;
    float material_time = 0;
    const runtime::LiveSession *live_session = nullptr;
    const BnwFogRenderedFrameReceipt *rendered_fog = nullptr;
};
struct BnwClientCaptureFiles { std::filesystem::path image, observation; };
namespace capture_detail {
inline std::string quote(std::string_view text) {
    std::ostringstream out; out << '"';
    for(unsigned char c:text) {
        if(c=='"' || c=='\\') out << '\\' << c;
        else if(c<32) out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << unsigned(c) << std::dec;
        else out << c;
    }
    out << '"'; return out.str();
}
template<class T> inline void optional(std::ostream &out,const std::optional<T> &v) {
    if(v) out << *v; else out << "null";
}
inline std::string stamp(const ui::BnwClientFrame &frame) {
    const auto &id=frame.stamp.identity;
    return "{\"owner\":"+std::to_string(id.owner)+",\"revision\":"+std::to_string(id.revision)+
        ",\"frame\":"+std::to_string(id.frame)+",\"session\":"+quote(id.session)+"}";
}
inline std::string state(const ui::BnwClientFrame &frame,const BnwRendererSceneReceipt &scene) {
    std::ostringstream out; out.imbue(std::locale::classic()); out << std::boolalpha;
    out << "{\"schema\":\"bnw-loaded-map-projection-v1\",\"player\":" << frame.stamp.identity.player
        << ",\"team\":" << frame.stamp.identity.team << ",\"turn\":" << frame.stamp.identity.turn
        << ",\"width\":" << scene.width << ",\"height\":" << scene.height
        << ",\"wrap\":[" << scene.wrap_x << ',' << scene.wrap_y << "],\"map_guid\":[";
    for(std::size_t i=0;i<scene.map_guid.size();++i) { if(i) out << ','; out << unsigned(scene.map_guid[i]); }
    out << "],\"selection\":["; optional(out,frame.stamp.selection.unit); out << ',';
    optional(out,frame.stamp.selection.city); out << "],\"cells\":[";
    for(std::size_t i=0;i<scene.cells.size();++i) {
        if(i) out << ',';
        const auto &c=scene.cells[i];
        out << '[' << c.index << ',' << c.x << ',' << c.y << ',';
        for(const auto &v:{c.owner,c.plot_type,c.terrain_type,c.feature_type,c.resource_type,c.improvement_type}) { optional(out,v); out << ','; }
        out << c.is_city << ',';
        if(c.rivers) { const auto &r=*c.rivers; out << '[' << r.is_river << ',' << r.west << ',' << r.northwest << ',' << r.northeast << ',' << r.flow_e << ',' << r.flow_se << ',' << r.flow_sw << ']'; } else out << "null";
        out << ','; if(c.route) { out << '[' << c.route->type << ',' << c.route->responsible << ','; optional(out,c.route->builder); out << ',' << c.route->pillaged << ']'; } else out << "null";
        out << ','; if(c.units) { out << '['; for(std::size_t j=0;j<c.units->size();++j) { if(j) out << ','; out << '[' << (*c.units)[j].owner << ',' << (*c.units)[j].id << ']'; } out << ']'; } else out << "null";
        out << ','; if(c.revealed_teams) { out << '['; bool comma=false; for(auto t:*c.revealed_teams) { if(comma) out << ','; comma=true; out << t; } out << ']'; } else out << "null"; out << ']';
    }
    out << "],\"saved_team_art\":[";
    for(std::size_t i=0;i<scene.active_team_art.size();++i) {
        if(i) out << ',';
        const auto &a=scene.active_team_art[i]; out << '[' << a.plot_index << ',' << a.visibility_count << ',';
        optional(out,a.revealed); out << ','; optional(out,a.revealed_improvement); out << ','; optional(out,a.revealed_improvement_hash); out << ',' << a.revealed_route << ']';
    }
    out << "],\"live_fog\":[";
    for(std::size_t i=0;i<frame.visibility.plots.size();++i) { if(i) out << ','; out << unsigned(frame.visibility.plots[i]); }
    out << "]}"; return out.str();
}
inline std::string camera(const ui::BnwClientFrame &frame) {
    const auto &c=frame.camera.camera; std::ostringstream out; out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10) << "{\"world\":[";
    for(std::size_t i=0;i<c.world.size();++i) { if(!std::isfinite(c.world[i])) throw std::runtime_error("Nonfinite capture camera"); if(i) out << ','; out << c.world[i]; }
    for(float v:{c.field_of_view,c.aspect_ratio,c.near_plane,c.far_plane}) if(!std::isfinite(v)) throw std::runtime_error("Nonfinite capture lens");
    out << "],\"lens\":[" << c.field_of_view << ',' << c.aspect_ratio << "],\"clipping\":[" << c.near_plane << ',' << c.far_plane << "]}"; return out.str();
}
inline void exclusive_write(const std::filesystem::path &path,std::span<const std::uint8_t> data) {
#ifdef _WIN32
    auto *file=_wfopen(path.c_str(),L"wbx");
#else
    auto *file=std::fopen(path.c_str(),"wbx");
#endif
    if(!file) throw std::runtime_error("Capture output exists or cannot be created");
    const auto size=std::fwrite(data.data(),1,data.size(),file);
    const auto closed=std::fclose(file);
    if(size!=data.size() || closed) { std::filesystem::remove(path); throw std::runtime_error("Capture output write failed"); }
}
} // namespace capture_detail
// Readback must return tightly packed RGB8, OpenGL bottom row first. This
// producer publishes no parity verdict and asserts no original-game provenance.
inline BnwClientCaptureFiles capture_bnw_client_rgb(
    const ui::BnwClientFrameOwner &owner,const std::shared_ptr<const ui::BnwClientFrame> &frame,
    const BnwRendererSceneReceipt &scene,std::int32_t width,std::int32_t height,
    const std::function<std::vector<std::uint8_t>()> &readback,const BnwClientCaptureOptions &options) {
    if(!frame || !options.utc_now || !options.live_session || !readback || options.output_prefix.empty() || options.context.empty() || !std::isfinite(options.material_time) || width<=0 || height<=0 ||
       width!=frame->stamp.viewport.framebuffer_width || height!=frame->stamp.viewport.framebuffer_height)
        throw std::runtime_error("Missing or mismatched capture inputs");
    const auto validate=[&] {
        if(!owner.is_current(frame)) throw std::runtime_error("Capture frame is stale");
        if(auto why=bnw_renderer_scene_revision_mismatch(scene,frame->world,frame->stamp.identity.turn,frame->stamp.identity.player)) throw std::runtime_error("Capture renderer mismatch: "+*why);
        if(scene.active_team!=static_cast<std::uint64_t>(frame->stamp.identity.team)) throw std::runtime_error("Capture active team mismatch");
        const auto &live=options.live_session->state();
        if(auto why=bnw_renderer_scene_revision_mismatch(scene,live.world,live.setup.game_turn,frame->stamp.identity.player))
            throw std::runtime_error("Capture underlying live scene mismatch: "+*why);
        const auto player=live.setup.players.find(frame->stamp.identity.player);
        if(player==live.setup.players.end() || !player->second.alive || player->second.team!=frame->stamp.identity.team)
            throw std::runtime_error("Capture live player/team changed");
        const auto &fog=runtime::bnw_live_plot_visibility_cache(live);
        if(fog.grid.width!=frame->visibility.width || fog.grid.height!=frame->visibility.height || fog.grid.size()!=frame->visibility.plots.size() || frame->stamp.identity.team<0 || static_cast<std::uint64_t>(frame->stamp.identity.team)>=fog.team_slots)
            throw std::runtime_error("Capture live fog shape changed");
        for(std::size_t i=0;i<frame->visibility.plots.size();++i) {
            const auto &cell=live.world.map_projection->cells[i];
            if(!cell.revealed_teams) throw std::runtime_error("Capture live reveal unavailable");
            const auto count=fog.counts.at(i*fog.wire_slots+static_cast<std::uint64_t>(frame->stamp.identity.team));
            if(count<0) throw std::runtime_error("Capture live fog count is negative");
            const auto actual=count>0?ui::WorldViewPlotFog::visible:cell.revealed_teams->contains(frame->stamp.identity.team)?ui::WorldViewPlotFog::explored:ui::WorldViewPlotFog::hidden;
            if(actual!=frame->visibility.plots[i]) throw std::runtime_error("Capture live fog changed");
        }
        if(frame->stamp.selection.unit) {
            const auto army=live.maintenance.players.find(frame->stamp.identity.player);
            if(army==live.maintenance.players.end() || !army->second.units.contains(*frame->stamp.selection.unit)) throw std::runtime_error("Capture selected unit ownership changed");
        }
        if(frame->stamp.selection.city) {
            const auto cities=live.cities.players.find(frame->stamp.identity.player);
            if(cities==live.cities.players.end() || !cities->second.cities.contains(*frame->stamp.selection.city)) throw std::runtime_error("Capture selected city ownership changed");
        }
        if(options.rendered_fog && !options.rendered_fog->matches(*frame,scene,live))
            throw std::runtime_error("Capture native fog rendered receipt is stale");
        if(options.validate_live_scene) options.validate_live_scene();
        if(!owner.is_current(frame)) throw std::runtime_error("Capture validator invalidated frame");
    };
    validate(); const auto before=capture_detail::stamp(*frame), loaded=capture_detail::state(*frame,scene), camera=capture_detail::camera(*frame);
    const auto before_utc=options.utc_now(); const auto pixels=readback(); const auto image_utc=options.utc_now();
    validate(); const auto after_utc=options.utc_now();
    const auto stride=static_cast<std::uint64_t>(width)*3;
    if(static_cast<std::uint64_t>(height)>std::numeric_limits<std::size_t>::max()/stride || pixels.size()!=stride*height) throw std::runtime_error("Capture RGB8 byte count differs");
    const auto header="P6\n"+std::to_string(width)+" "+std::to_string(height)+"\n255\n";
    std::vector<std::uint8_t> ppm(header.begin(),header.end()); ppm.reserve(ppm.size()+pixels.size());
    for(std::int32_t y=height-1;y>=0;--y) ppm.insert(ppm.end(),pixels.begin()+y*stride,pixels.begin()+(y+1)*stride);
    BnwClientCaptureFiles files{options.output_prefix,options.output_prefix};
    files.image+=".ppm"; files.observation+=".native.json";
    std::ostringstream out; out.imbue(std::locale::classic()); out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "{\"schema\":\"civis-native-frame-capture-v1\",\"context\":" << capture_detail::quote(options.context)
        << ",\"animation_phase\":{\"material_time\":" << options.material_time << "},\"image\":{\"path\":" << capture_detail::quote(files.image.filename().string())
        << ",\"sha256\":" << capture_detail::quote(runtime::identify_bytes(ppm).sha256) << "},\"pixel_format\":\"RGB8-top-down\",\"framebuffer\":[" << width << ',' << height
        << "],\"before_owner\":" << before << ",\"after_owner\":" << capture_detail::stamp(*frame)
        << ",\"before_utc\":" << capture_detail::quote(before_utc) << ",\"image_utc\":" << capture_detail::quote(image_utc) << ",\"after_utc\":" << capture_detail::quote(after_utc)
        << ",\"camera\":" << camera << ",\"loaded_state\":" << loaded
        << ",\"scene_file_sha256\":" << capture_detail::quote(scene.scene_file_sha256) << ",\"payload_sha256\":" << capture_detail::quote(scene.restored_payload_sha256)
        << ",\"semantic_live_validation\":true,\"renderer_validation\":true,\"completeContext\":false,\"fullGraphicsParity\":false}";
    auto text=out.str();
    if(options.rendered_fog) {
        const auto &proof=*options.rendered_fog;
        std::ostringstream fog;
        fog << ",\"rendered_fog\":{\"schema\":\"civis-rendered-fog-frame-v1\",\"sequence\":" << proof.sequence()
            << ",\"render_revision\":" << proof.render_revision() << ",\"logical_revision\":" << capture_detail::quote(proof.logical_revision())
            << ",\"native_state_sha256\":" << capture_detail::quote(proof.native_state_digest())
            << ",\"prepared_frame_sha256\":" << capture_detail::quote(proof.prepared_frame_digest())
            << ",\"mask_texture\":" << proof.mask_texture() << ",\"render_extent\":[" << proof.render_extent()[0] << ',' << proof.render_extent()[1]
            << "],\"client_framebuffer\":[" << proof.presentation_extent()[0] << ',' << proof.presentation_extent()[1]
            << "],\"internal_scene_target\":" << (proof.internal_scene_target()?"true":"false") << "}";
        text.pop_back();text+=fog.str();text+='}';
    }
    capture_detail::exclusive_write(files.image,ppm);
    try { capture_detail::exclusive_write(files.observation,std::span(reinterpret_cast<const std::uint8_t *>(text.data()),text.size())); }
    catch(...) { std::filesystem::remove(files.image); throw; }
    return files;
}
} // namespace civ5::graphics
