#include "civ5/ui/bnw_info_default_form.hpp"
#include "civ5/ui/bnw_city_banner_appearance.hpp"
#include "civ5/ui/bnw_unit_panel_stat_rows_feed.hpp"
#pragma once
#include "../src/ui/detail/civis_frontend_party_routing_owner.hpp"
#include "../src/ui/detail/civis_frontend_alpha_land_frame.hpp"
#include "civ5/ui/glyphgen.hpp"
#include "civ5/ui/minimap_panel.hpp"
#include "civ5/ui/minimap_ui.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/ui/minimap_saved_map.hpp"
#include "civ5/ui/minimap_saved_reveal.hpp"
#include "civ5/ui/minimap_saved_ice_plots.hpp"
#include "civ5/ui/minimap_mountain_markers.hpp"
#include "civ5/ui/minimap_city_markers.hpp"
#include "civ5/ui/minimap_coast_edge_candidates.hpp"
#include "civ5/ui/minimap_native_edge_records.hpp"
#include "civ5/ui/minimap_native_bounds_fixture.hpp"
#include "civ5/ui/minimap_captured_gandhi_culture.hpp"
#include "civ5/ui/minimap_world_bridge.hpp"
#include "civ5/ui/minimap_scan_window.hpp"
#include "civ5/ui/minimap_viewport_ring.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include "civ5/ui/bnw_city_production_backend.hpp"
#include "civ5/ui/selected_city_production_services.hpp"
#include "civ5/runtime/bnw_city_production_turn.hpp"
#include "civ5/ui/bnw_train_renderer_refresh_receipt.hpp"
#include "civ5/ui/bnw_alpha_progression_backend.hpp"
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
#include "bnw_research_choice_restart_fixture.hpp"
#endif
#include "civ5/ui/live_session_frame.hpp"
#include "civ5/ui/civis_client_audio_queue_owner.hpp"
#include "civ5/ui/civis_bank_audio_source_owner.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/graphics/bnw_pending_unit_visual_owner.hpp"
#include "gl_bnw_warrior_owned_scene.hpp"
#include "civ5/ui/bnw_manual_cycle_key_input.hpp"
#include "civ5/ui/bnw_tech_award_input_dispatch.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/ui/bnw_system_font_cache_owner.hpp"
#ifdef CIV5_WORLDVIEW_SYSTEM_TITLE_MEASURE_GATES
#include "../tests/bnw_worldview_system_title_run_live_gates.hpp"
#endif
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_tech_tree_lifecycle.hpp"
#include "civ5/graphics/bnw_client_capture.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <fstream>
#include "civ5/graphics/bnw_ui_source_observation.hpp"
#include "civ5/ui/live_city_banner_projection.hpp"
#include "civ5/ui/world_plot_pick.hpp"
#include "civ5/ui/live_pointer_priority.hpp"
#include "civ5/ui/win32_viewport_transaction.hpp"
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/ui/bnw_worldview_renderer_projection_loan.hpp"
#include "civ5/graphics/bnw_train_presented_frame_proof.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/trade_route_overview_input.hpp"
#include "civ5/ui/economic_overview_scene.hpp"
#include "civ5/assets/ui_block_atlas.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include "civ5/scene/saved_fog_of_war.hpp"
#include "civ5/graphics/gl_bnw_natural_wonder_anchor.hpp"
#include "civ5/graphics/bnw_deferred_fog_client.hpp"
#include "civ5/graphics/bnw_camera_fog_client.hpp"
#include "civ5/terrain/river_surface.hpp"
#include "gdi_extended_font.hpp"
#include "gl_minimap_surface_diagnostic.hpp"
#include "gl_minimap_viewport_ring.hpp"
#include "ui_screen_gl.hpp"
#include "ui_glyphgen_styles.hpp"
#include "retail_preview_camera.hpp"
#include "trade_route_tab_geometry.hpp"
#include "economic_overview_geometry.hpp"
#include "economic_popup_feed_mux.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <limits>
#include <optional>
#include <memory>
#include <sstream>
#include <span>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>

// Manual harness around the production camera and renderers. LAND can replay
// prior GL caches or run retained generation. Original D3D9 parity is a
// separate complete-scene validation route.
namespace {
// Timing observations only: native error state survives both clock and I/O.
void camera_stage_mark(const char* stage,const char* edge) noexcept {
    const int previous_errno=errno;const DWORD previous_native_error=GetLastError();
    struct RestoreErrors {
        int crt;DWORD native;
        ~RestoreErrors() noexcept {errno=crt;SetLastError(native);}
    } restore{previous_errno,previous_native_error};
    try {
        const auto tick=std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::fprintf(stderr,"[CIVIS_CAMERA_STAGE] %s %s steady_ns=%lld\n",
                     stage,edge,static_cast<long long>(tick));
        std::fflush(stderr);
    } catch(...) {} // Diagnostic failure cannot change renderer execution.
}
// Capture the completed preview framebuffer, including UI, before swapping.
// OpenGL returns its lower row first; the PPM writer reverses those rows.
std::vector<unsigned char> read_preview_frame_pixels(GlContext &context, int width, int height) {
    const auto read_pixels = reinterpret_cast<PFNGLREADPIXELSPROC>(context.proc_address("glReadPixels"));
    const auto read_buffer = reinterpret_cast<PFNGLREADBUFFERPROC>(context.proc_address("glReadBuffer"));
    const auto &api = context.api();
    const std::array<GLenum, 4> fields{GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_ROWS,
                                     GL_PACK_SKIP_PIXELS};
    std::array<GLint, 4> prior{};
    GLint prior_buffer = 0, prior_source = 0;
    api.GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prior_buffer);
    api.GetIntegerv(GL_READ_BUFFER, &prior_source);
    for (std::size_t i = 0; i < fields.size(); ++i) {
        api.GetIntegerv(fields[i], &prior[i]);
        api.PixelStorei(fields[i], i == 0 ? 1 : 0);
    }
    api.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    read_buffer(GL_BACK);
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 3);
    read_pixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    read_buffer(static_cast<GLenum>(prior_source));
    api.BindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(prior_buffer));
    for (std::size_t i = 0; i < fields.size(); ++i)
        api.PixelStorei(fields[i], prior[i]);
    context.check("Capture preview framebuffer");
    return pixels;
}
// PPM keeps this diagnostic independent of an image-encoding dependency.
void save_preview_frame_pixels(const fs::path &path, int width, int height,
                               const std::vector<unsigned char>& pixels) {
    std::ofstream image(path, std::ios::binary);
    image << "P6\n" << width << ' ' << height << "\n255\n";
    const auto stride = static_cast<std::size_t>(width) * 3;
    for (int y = height - 1; y >= 0; --y)
        image.write(reinterpret_cast<const char *>(pixels.data() + y * stride), stride);
    if (!image)
        throw std::runtime_error("Could not write preview screenshot");
}
// Diagnostic timing/storage must preserve the renderer's native/CRT errors.
struct FirstPresentedObservationErrors {
    const int crt=errno;const DWORD native=GetLastError();
    ~FirstPresentedObservationErrors() noexcept {errno=crt;SetLastError(native);}
};
std::int64_t first_presented_observation_tick() noexcept {
    const FirstPresentedObservationErrors restore;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
// Opaque User32 DPI contexts are recorded as identity bits only. These optional
// dynamically resolved queries avoid importing Win10-only APIs or changing SDK
// version flags; no interpretation of pseudo-handles/equality is performed.
#if defined(DPI_AWARENESS_CONTEXT_UNAWARE)
using FirstPresentedDpiContext = DPI_AWARENESS_CONTEXT;
using FirstPresentedDpiAwareness = DPI_AWARENESS;
#else
using FirstPresentedDpiContext = HANDLE; // Opaque pointer-sized User32 handle ABI.
using FirstPresentedDpiAwareness = int; // SDK enum has the same signed 32-bit ABI.
#endif
struct FirstPresentedDpiObservation {
    bool module_available=false,thread_api_available=false,window_api_available=false;
    bool thread_query_called=false,window_query_called=false;
    std::uintptr_t thread_context=0,window_context=0;
    const char* thread_status="NOT_OBSERVED";
    const char* window_status="NOT_OBSERVED";
    bool awareness_api_available=false,window_dpi_api_available=false,system_dpi_api_available=false;
    bool thread_awareness_called=false,window_awareness_called=false;
    bool window_dpi_called=false,system_dpi_called=false;
    int thread_awareness=-1,window_awareness=-1;
    UINT window_dpi=0,system_dpi=0;
    const char* thread_awareness_status="NOT_OBSERVED";
    const char* window_awareness_status="NOT_OBSERVED";
    const char* window_dpi_status="NOT_OBSERVED";
    const char* system_dpi_status="NOT_OBSERVED";
    std::int64_t begin_tick=0,end_tick=0;
};
FirstPresentedDpiObservation observe_first_presented_dpi(
    const civ5::graphics::GlContextNativeIdentity& identity) noexcept {
    const FirstPresentedObservationErrors restore;
    FirstPresentedDpiObservation result;
    result.begin_tick=first_presented_observation_tick();
    const auto module=GetModuleHandleW(L"user32.dll"); // Already loaded only; never LoadLibrary.
    result.module_available=module!=nullptr;
    using ThreadQuery=FirstPresentedDpiContext (WINAPI *)();
    using WindowQuery=FirstPresentedDpiContext (WINAPI *)(HWND);
    const auto thread_query=module?reinterpret_cast<ThreadQuery>(
        GetProcAddress(module,"GetThreadDpiAwarenessContext")):nullptr;
    const auto window_query=module?reinterpret_cast<WindowQuery>(
        GetProcAddress(module,"GetWindowDpiAwarenessContext")):nullptr;
    result.thread_api_available=thread_query!=nullptr;
    result.window_api_available=window_query!=nullptr;
    if(!identity.owner_thread) {
        result.thread_status=result.window_status="SKIPPED_FOREIGN_OWNER_THREAD";
    } else {
        result.thread_status=module?(thread_query?"RETURNED_NULL":"API_UNAVAILABLE"):"MODULE_UNAVAILABLE";
        result.window_status=module?(window_query?"RETURNED_NULL":"API_UNAVAILABLE"):"MODULE_UNAVAILABLE";
        if(thread_query) {
            result.thread_query_called=true;
            result.thread_context=reinterpret_cast<std::uintptr_t>(thread_query());
            if(result.thread_context)result.thread_status="RETURNED_NON_NULL_OPAQUE_CONTEXT";
        }
        if(!identity.window)result.window_status="SKIPPED_NULL_IDENTITY_WINDOW";
        else if(window_query) {
            result.window_query_called=true;
            result.window_context=reinterpret_cast<std::uintptr_t>(
                window_query(reinterpret_cast<HWND>(identity.window)));
            if(result.window_context)result.window_status="RETURNED_NON_NULL_OPAQUE_CONTEXT";
        }
    }
    // Optional scalar queries are serialized after the raw handle samples.
    // They attest only this AFTER-P6 interval, never awareness during drawing.
    using AwarenessQuery=FirstPresentedDpiAwareness (WINAPI *)(FirstPresentedDpiContext);
    using WindowDpiQuery=UINT (WINAPI *)(HWND);
    using SystemDpiQuery=UINT (WINAPI *)();
    const auto awareness_query=module?reinterpret_cast<AwarenessQuery>(
        GetProcAddress(module,"GetAwarenessFromDpiAwarenessContext")):nullptr;
    const auto window_dpi_query=module?reinterpret_cast<WindowDpiQuery>(
        GetProcAddress(module,"GetDpiForWindow")):nullptr;
    const auto system_dpi_query=module?reinterpret_cast<SystemDpiQuery>(
        GetProcAddress(module,"GetDpiForSystem")):nullptr;
    result.awareness_api_available=awareness_query!=nullptr;
    result.window_dpi_api_available=window_dpi_query!=nullptr;
    result.system_dpi_api_available=system_dpi_query!=nullptr;
    const auto awareness_status=[](int value) noexcept {
        return value==-1?"RETURNED_INVALID":
            (value>=0 && value<=2?"RETURNED_DOCUMENTED_AWARENESS":"RETURNED_OTHER_ENUM_VALUE");
    };
    if(!identity.owner_thread) {
        result.thread_awareness_status=result.window_awareness_status=
            result.window_dpi_status=result.system_dpi_status="SKIPPED_FOREIGN_OWNER_THREAD";
    } else {
        result.thread_awareness_status=result.window_awareness_status=
            module?(awareness_query?"SKIPPED_NULL_CONTEXT":"API_UNAVAILABLE"):"MODULE_UNAVAILABLE";
        result.window_dpi_status=module?(window_dpi_query?"SKIPPED_NULL_IDENTITY_WINDOW":"API_UNAVAILABLE"):"MODULE_UNAVAILABLE";
        result.system_dpi_status=module?(system_dpi_query?"NOT_OBSERVED":"API_UNAVAILABLE"):"MODULE_UNAVAILABLE";
        if(awareness_query && result.thread_context) {
            result.thread_awareness_called=true;
            result.thread_awareness=static_cast<int>(awareness_query(
                reinterpret_cast<FirstPresentedDpiContext>(result.thread_context)));
            result.thread_awareness_status=awareness_status(result.thread_awareness);
        }
        if(awareness_query && result.window_context) {
            result.window_awareness_called=true;
            result.window_awareness=static_cast<int>(awareness_query(
                reinterpret_cast<FirstPresentedDpiContext>(result.window_context)));
            result.window_awareness_status=awareness_status(result.window_awareness);
        }
        if(window_dpi_query && identity.window) {
            result.window_dpi_called=true;
            result.window_dpi=window_dpi_query(reinterpret_cast<HWND>(identity.window));
            result.window_dpi_status=result.window_dpi?"RETURNED_NONZERO":"RETURNED_ZERO";
        }
        if(system_dpi_query) {
            result.system_dpi_called=true;
            result.system_dpi=system_dpi_query();
            result.system_dpi_status=result.system_dpi?"RETURNED_NONZERO":"RETURNED_ZERO";
        }
    }
    result.end_tick=first_presented_observation_tick();
    return result;
}
// Optional observability only. No exported field grants source/currentness,
// Native GUI or complete-WORLD authority. Existing loan/viewport guards remain
// the sole authorization and are not called again by this file-only helper.
void save_first_presented_post_metadata(const fs::path& path,int width,int height,
    const std::vector<unsigned char>& pixels,const civ5::ui::BnwClientFrameStamp& stamp,
    bool present_returned,bool client_post_returned,std::int64_t present_tick,
    std::int64_t client_post_tick,const civ5::graphics::GlContextNativeIdentity& native_identity) {
    const FirstPresentedObservationErrors restore;
    if(!present_returned || !client_post_returned || width<=0 || height<=0 ||
       stamp.viewport.framebuffer_width!=width || stamp.viewport.framebuffer_height!=height)
        throw std::runtime_error("First-presented metadata lacks returned edges or matching frame");
    const auto stride=static_cast<std::uint64_t>(width)*3;
    if(static_cast<std::uint64_t>(height)>std::numeric_limits<std::size_t>::max()/stride ||
       pixels.size()!=stride*height)
        throw std::runtime_error("First-presented metadata RGB shape differs");
    const auto header="P6\n"+std::to_string(width)+" "+std::to_string(height)+"\n255\n";
    if(pixels.size()>std::numeric_limits<std::size_t>::max()-header.size())
        throw std::runtime_error("First-presented metadata P6 size overflow");
    const auto expected_size=header.size()+pixels.size();
    const auto before_size=fs::file_size(path);
    const auto before_time=fs::last_write_time(path);
    if(before_size!=expected_size || expected_size>static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max()))
        throw std::runtime_error("First-presented physical P6 size differs");
    std::ifstream image(path,std::ios::binary);
    if(!image)throw std::runtime_error("First-presented physical P6 unavailable");
    std::vector<std::uint8_t> physical(expected_size);
    image.read(reinterpret_cast<char*>(physical.data()),static_cast<std::streamsize>(physical.size()));
    if(image.gcount()!=static_cast<std::streamsize>(physical.size()) || !image)
        throw std::runtime_error("First-presented physical P6 read failed");
    image.close();
    if(image.fail() || fs::file_size(path)!=before_size || fs::last_write_time(path)!=before_time ||
       !std::equal(header.begin(),header.end(),physical.begin()))
        throw std::runtime_error("First-presented physical P6 changed or has wrong header");
    for(int y=0;y<height;++y) {
        const auto p6_offset=header.size()+static_cast<std::size_t>(y)*stride;
        const auto source_offset=static_cast<std::size_t>(height-1-y)*stride;
        if(!std::equal(pixels.begin()+source_offset,pixels.begin()+source_offset+stride,
                       physical.begin()+p6_offset))
            throw std::runtime_error("First-presented physical pixels differ from same readback");
    }
    const auto physical_identity=civ5::runtime::identify_bytes(physical);
    const auto pixel_identity=civ5::runtime::identify_bytes(
        std::span<const std::uint8_t>(physical).subspan(header.size()));
    // This sample occurs only AFTER returned present/POST, the existing file-write
    // current bookend and physical P6 verification. It attests no earlier DPI state.
    const auto dpi_observation=observe_first_presented_dpi(native_identity);
    using civ5::graphics::capture_detail::quote;
    std::ostringstream out;out.imbue(std::locale::classic());
    out << "{\"schema\":\"civis-first-presented-file-observation-v1\","
        << "\"publication\":\"COMPLETE\",\"renderer_branch\":\"UNKNOWN\","
        << "\"authoritative_run_id\":null,\"output_directory\":" << quote(path.parent_path().generic_string())
        << ",\"native_process_id\":" << quote(std::to_string(GetCurrentProcessId()))
        << ",\"screen_loan_width\":" << width << ",\"screen_loan_height\":" << height
        << ",\"gui_frame_revision\":" << quote(std::to_string(stamp.identity.frame))
        << ",\"client_frame_stamp\":{\"owner\":" << quote(std::to_string(stamp.identity.owner))
        << ",\"revision\":" << quote(std::to_string(stamp.identity.revision))
        << ",\"frame\":" << quote(std::to_string(stamp.identity.frame))
        << ",\"session\":" << quote(stamp.identity.session)
        << ",\"player\":" << stamp.identity.player << ",\"team\":" << stamp.identity.team
        << ",\"turn\":" << stamp.identity.turn << "},"
        << "\"observed_edges\":{\"context_present_returned\":true,"
        << "\"client_frame_POST_returned\":true,\"present_returned_steady_ns\":" << quote(std::to_string(present_tick))
        << ",\"client_POST_returned_steady_ns\":" << quote(std::to_string(client_post_tick)) << "},"
        << "\"image\":{\"path\":" << quote(path.filename().generic_string())
        << ",\"bytes\":" << quote(std::to_string(physical_identity.bytes))
        << ",\"sha256\":" << quote(physical_identity.sha256)
        << ",\"rgb_payload_sha256\":" << quote(pixel_identity.sha256)
        << ",\"pixel_format\":\"RGB8-top-down\",\"readback_source\":\"GL_BACK-before-swap\","
        << "\"same_readback_bytes_verified_after_file_close\":true},"
        << "\"native_identity_observation\":{\"window_uintptr\":" << quote(std::to_string(native_identity.window))
        << ",\"dc_uintptr\":" << quote(std::to_string(native_identity.dc))
        << ",\"context_uintptr\":" << quote(std::to_string(native_identity.context))
        << ",\"owner_thread_at_identity_read\":" << (native_identity.owner_thread?"true":"false")
        << ",\"identity_only_no_liveness_or_current_grant\":true},"
        << "\"dpi_context_observation\":{\"stage\":\"AFTER_PRESENT_POSTGUARDS_AND_P6_FILE_VERIFICATION\","
        << "\"begin_steady_ns\":" << quote(std::to_string(dpi_observation.begin_tick))
        << ",\"end_steady_ns\":" << quote(std::to_string(dpi_observation.end_tick))
        << ",\"user32_already_loaded\":" << (dpi_observation.module_available?"true":"false")
        << ",\"thread_api_available\":" << (dpi_observation.thread_api_available?"true":"false")
        << ",\"window_api_available\":" << (dpi_observation.window_api_available?"true":"false")
        << ",\"thread_query_called\":" << (dpi_observation.thread_query_called?"true":"false")
        << ",\"window_query_called\":" << (dpi_observation.window_query_called?"true":"false")
        << ",\"thread_status\":" << quote(dpi_observation.thread_status)
        << ",\"window_status\":" << quote(dpi_observation.window_status)
        << ",\"thread_context_uintptr\":" << (dpi_observation.thread_context?quote(std::to_string(dpi_observation.thread_context)):"null")
        << ",\"window_context_uintptr\":" << (dpi_observation.window_context?quote(std::to_string(dpi_observation.window_context)):"null")
        << ",\"contexts_uninterpreted\":true,\"throughout_draw_state_attested\":false,"
        << "\"physical_drawable_join_attested\":false},"
        << "\"dpi_scalar_observation\":{\"stage\":\"AFTER_PRESENT_POSTGUARDS_AND_P6_FILE_VERIFICATION\","
        << "\"awareness_api_available\":" << (dpi_observation.awareness_api_available?"true":"false")
        << ",\"window_dpi_api_available\":" << (dpi_observation.window_dpi_api_available?"true":"false")
        << ",\"system_dpi_api_available\":" << (dpi_observation.system_dpi_api_available?"true":"false")
        << ",\"thread_awareness_called\":" << (dpi_observation.thread_awareness_called?"true":"false")
        << ",\"window_awareness_called\":" << (dpi_observation.window_awareness_called?"true":"false")
        << ",\"window_dpi_called\":" << (dpi_observation.window_dpi_called?"true":"false")
        << ",\"system_dpi_called\":" << (dpi_observation.system_dpi_called?"true":"false")
        << ",\"thread_awareness_status\":" << quote(dpi_observation.thread_awareness_status)
        << ",\"window_awareness_status\":" << quote(dpi_observation.window_awareness_status)
        << ",\"window_dpi_status\":" << quote(dpi_observation.window_dpi_status)
        << ",\"system_dpi_status\":" << quote(dpi_observation.system_dpi_status)
        << ",\"thread_awareness_value\":" << (dpi_observation.thread_awareness_called?std::to_string(dpi_observation.thread_awareness):"null")
        << ",\"window_awareness_value\":" << (dpi_observation.window_awareness_called?std::to_string(dpi_observation.window_awareness):"null")
        << ",\"window_dpi_value\":" << (dpi_observation.window_dpi_called?std::to_string(dpi_observation.window_dpi):"null")
        << ",\"system_dpi_value\":" << (dpi_observation.system_dpi_called?std::to_string(dpi_observation.system_dpi):"null")
        << ",\"serial_observations_not_atomic\":true,\"throughout_draw_state_attested\":false,"
        << "\"physical_drawable_join_attested\":false},"
        << "\"physical_client_extent_attested\":false,\"complete_WORLD_attested\":false,"
        << "\"native_label_ready\":false,\"full_graphics_parity\":false,"
        << "\"qualification\":\"Own Civis file/present observations; original/source/world branch and physical HWND extent not inferred\"}";
    const auto text=out.str();
    auto final=path;final+=".postpresent.json";
    auto pending=path;pending+=".postpresent.json.pending";
    if(fs::exists(final))throw std::runtime_error("First-presented metadata already exists");
    civ5::graphics::capture_detail::exclusive_write(pending,
        std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()));
    try {
        // Same-directory Win32 no-replace move: std::filesystem::rename may
        // replace an existing destination despite the earlier exists check.
        // Consumers ignore .pending and require COMPLETE in the final file.
        if(!MoveFileExW(pending.c_str(),final.c_str(),0)) {
            const auto move_error=GetLastError();
            throw std::runtime_error("First-presented metadata no-replace move failed: "+
                                     std::to_string(move_error));
        }
    } catch(...) {
        std::error_code ignored;fs::remove(pending,ignored);throw;
    }
}

void save_preview_frame(GlContext &context, const fs::path &path, int width, int height) {
    const auto pixels = read_preview_frame_pixels(context, width, height);
    save_preview_frame_pixels(path, width, height, pixels);
}

struct PreviewInput {
    struct UiWheelEvent {
        std::int32_t x = 0, y = 0, delta = 0;
        std::uint16_t flags = 0;
    };
    civ5::ui::ClientWindowInput router;
    std::vector<CameraInputEvent> events;
    std::vector<UiWheelEvent> live_ui_wheels;
    std::array<bool, 256> keys{};
    bool close = false, reset = false, cancel = false, failed = false;
    bool coordinate_cleanup_pending = false;
    bool inertia = true, straight = false;
    std::int32_t pointer_x = -1, pointer_y = -1;
    bool pointer_valid = false;
    bool pointer_left_down = false;
    std::int32_t pointer_press_x = -1, pointer_press_y = -1;
    bool top_panel_click_pending = false;
    std::int32_t top_panel_click_x = -1, top_panel_click_y = -1;
    bool search_focus = false, search_cancel = false;
    std::vector<char16_t> search_characters;
    std::optional<civ5::ui::MinimapPanelPlan> minimap_hit_panel;
    bool minimap_options_visible = false;
    std::array<std::optional<bool>, civ5::ui::minimap_map_option_count> minimap_map_display{};
    bool minimap_pressed = false, minimap_click_pending = false;
    std::vector<civ5::ui::WorldViewRect> live_hit_regions;
    bool live_ui_pressed = false;
    bool live_modal_open = false;
    std::int32_t minimap_press_x = -1, minimap_press_y = -1;
    std::int32_t minimap_click_x = -1, minimap_click_y = -1;
    struct WindowMouseRecord {std::uint64_t sequence;UINT message;std::int32_t x,y;};
    struct LeftWindowClick {WindowMouseRecord press,release;};
    std::uint64_t observed_window_mouse_sequence=0;
    std::optional<WindowMouseRecord> observed_left_press;
    std::optional<LeftWindowClick> observed_left_click;
    void observe_window_mouse(UINT message,std::int32_t x,std::int32_t y) {
        check(observed_window_mouse_sequence<std::numeric_limits<std::uint64_t>::max(),"Window mouse trace overflow");
        const WindowMouseRecord actual{++observed_window_mouse_sequence,message,x,y};
        if(message==WM_LBUTTONDOWN){observed_left_press=actual;observed_left_click.reset();}
        else if(message==WM_LBUTTONUP){
            if(observed_left_press)observed_left_click=LeftWindowClick{*observed_left_press,actual};
            observed_left_press.reset();
        }
    }
    unsigned pointer_messages = 0, wheel_messages = 0;
};
std::uint32_t preview_window_dpi(HWND window) {
    using GetWindowDpi = UINT (WINAPI *)(HWND);
    const auto function = reinterpret_cast<GetWindowDpi>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    const auto dpi = function ? function(window) : 96u;
    return dpi ? dpi : 96u;
}
constexpr auto preview_window_property = L"Civis.CameraPreview.Input";
struct PreviewWindowState {
    PreviewInput *input = nullptr;
    WNDPROC previous = nullptr;
};
LRESULT CALLBACK camera_preview_proc(HWND window, UINT message, WPARAM w, LPARAM l) {
    const auto *state = static_cast<PreviewWindowState *>(GetPropW(window, preview_window_property));
    auto *input = state ? state->input : nullptr;
    if (!input)
        return DefWindowProcW(window, message, w, l);
    try {
        if (message == WM_CLOSE) {
            input->close = true;
            return 0;
        }
        if (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP) {
            input->pointer_x = static_cast<std::int16_t>(LOWORD(l));
            input->pointer_y = static_cast<std::int16_t>(HIWORD(l));
            input->pointer_valid = true;
            const auto in_rect = [&](civ5::ui::MinimapRect rect) {
                return input->pointer_x >= rect.x && input->pointer_y >= rect.y &&
                       std::int64_t(input->pointer_x) < std::int64_t(rect.x) + rect.width &&
                       std::int64_t(input->pointer_y) < std::int64_t(rect.y) + rect.height;
            };
            const auto in_minimap = [&] {
                if (!input->minimap_hit_panel) return false;
                const auto &panel = *input->minimap_hit_panel;
                return in_rect(panel.map) || in_rect(panel.strategic) || in_rect(panel.options) ||
                       (input->minimap_options_visible && in_rect(panel.options_panel));
            };
            const auto in_live_ui = [&] {
                return civ5::ui::live_ui_blocks_camera_pointer(input->live_hit_regions,
                    input->live_modal_open, true, input->pointer_x, input->pointer_y);
            };
            const auto pointer_kind = message == WM_MOUSEMOVE
                ? civ5::ui::ClientPointerKind::move : message == WM_LBUTTONDOWN
                ? civ5::ui::ClientPointerKind::left_down : civ5::ui::ClientPointerKind::left_up;
            const auto routed = input->router.pointer(pointer_kind,
                {input->pointer_x, input->pointer_y});
            if (routed.cancel_camera) input->cancel = true;
            input->observe_window_mouse(message,input->pointer_x,input->pointer_y);
            if (message == WM_LBUTTONDOWN) {
                input->pointer_left_down = true;
                input->pointer_press_x = input->pointer_x;
                input->pointer_press_y = input->pointer_y;
                input->live_ui_pressed = in_live_ui() ||
                    (routed.owner == civ5::ui::ClientInputOwner::ui && !in_minimap());
                input->minimap_pressed = !input->live_ui_pressed && in_minimap();
                if (input->minimap_pressed) {
                    input->minimap_press_x = input->pointer_x;
                    input->minimap_press_y = input->pointer_y;
                }
            } else if (message == WM_LBUTTONUP) {
                if (input->pointer_left_down && !input->minimap_pressed) {
                    input->top_panel_click_pending = true;
                    input->top_panel_click_x = input->pointer_x;
                    input->top_panel_click_y = input->pointer_y;
                }
                if (input->pointer_left_down && input->minimap_pressed && in_minimap()) {
                    input->minimap_click_pending = true;
                    input->minimap_click_x = input->pointer_x;
                    input->minimap_click_y = input->pointer_y;
                }
                input->pointer_left_down = false;
            }
            if (input->minimap_pressed) {
                if (message == WM_LBUTTONUP) input->minimap_pressed = false;
                ++input->pointer_messages;
                return 0;
            }
            if (input->live_ui_pressed || routed.owner == civ5::ui::ClientInputOwner::ui) {
                if (message == WM_LBUTTONUP) input->live_ui_pressed = false;
                ++input->pointer_messages;
                return 0;
            }
            const auto kind = message == WM_MOUSEMOVE     ? CameraPointerEventKind::move
                              : message == WM_LBUTTONDOWN ? CameraPointerEventKind::left_down
                                                          : CameraPointerEventKind::left_up;
            check(input->events.size() < 4096, "Preview input queue overflow");
            input->events.push_back(CameraPointerEvent{kind, static_cast<std::int16_t>(LOWORD(l)),
                                                       static_cast<std::int16_t>(HIWORD(l)),
                                                       static_cast<std::uint32_t>(w)});
            ++input->pointer_messages;
            return 0;
        }
        if (message == WM_MOUSEWHEEL) {
            POINT point{static_cast<std::int16_t>(LOWORD(l)),
                        static_cast<std::int16_t>(HIWORD(l))};
            check(ScreenToClient(window, &point), "Could not map preview wheel to client pixels");
            check(input->live_ui_wheels.size() < 256, "Preview wheel queue overflow");
            input->live_ui_wheels.push_back({point.x, point.y,
                static_cast<std::int16_t>(HIWORD(w)), LOWORD(w)});
            ++input->wheel_messages;
            return 0;
        }
        if (message == WM_CHAR && input->search_focus) {
            check(input->search_characters.size() < 256,
                  "UI text input queue overflow");
            input->search_characters.push_back(static_cast<char16_t>(w));
            return 0;
        }
        if (message == WM_KEYDOWN && w < input->keys.size()) {
            if (input->search_focus) {
                if (w == VK_ESCAPE) {
                    input->search_cancel = true;
                    input->search_focus = false;
                }
                return 0;
            }
            if (input->router.keyboard_owner() == civ5::ui::ClientInputOwner::ui)
                return 0;
            if (!input->keys[w]) {
                if (w == VK_ESCAPE)
                    input->close = true;
                if (w == 'R')
                    input->reset = true;
                if (w == 'I')
                    input->inertia = !input->inertia;
                if (w == 'Z')
                    input->straight = !input->straight;
            }
            input->keys[w] = true;
            return 0;
        }
        if (message == WM_KEYUP && w < input->keys.size()) {
            input->keys[w] = false;
            return 0;
        }
        if (message == WM_KILLFOCUS || message == WM_CANCELMODE ||
            (message == WM_CAPTURECHANGED && reinterpret_cast<HWND>(l) != window)) {
            input->router.cancel();
            input->cancel = true;
            input->keys.fill(false);
            input->search_focus = false;
            input->search_cancel = true;
            input->search_characters.clear();
            input->pointer_valid = false;
            input->pointer_left_down = false;
            input->pointer_press_x = input->pointer_press_y = -1;
            input->top_panel_click_pending = false;
            input->top_panel_click_x = input->top_panel_click_y = -1;
            input->live_ui_wheels.clear();
            input->minimap_pressed = input->minimap_click_pending = false;
            input->live_ui_pressed = false;
            input->minimap_press_x = input->minimap_press_y = -1;
            input->minimap_click_x = input->minimap_click_y = -1;
        }
    } catch (...) {
        input->failed = true;
    }
    return state && state->previous ? CallWindowProcW(state->previous, window, message, w, l)
                                     : DefWindowProcW(window, message, w, l);
}
struct PreviewWindowBinding {
    HWND window;
    PreviewWindowState state;
    PreviewWindowBinding(HWND w, PreviewInput &input) : window(w), state{&input, nullptr} {
        check(GetPropW(window, preview_window_property) == nullptr,
              "Preview window already has an input owner");
        check(SetPropW(window, preview_window_property, &state), "Cannot bind preview input state");
        state.previous = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(camera_preview_proc)));
        if (!state.previous) {
            RemovePropW(window, preview_window_property);
            throw std::runtime_error("Cannot bind preview input window");
        }
    }
    ~PreviewWindowBinding() {
        if (GetCapture() == window) ReleaseCapture();
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(state.previous));
        RemovePropW(window, preview_window_property);
    }
};
void camera_preview_event(HWND window, PreviewInput &input,
                          const civ5::ui::ClientWindowEvent &event) {
    using Kind=civ5::ui::ClientWindowEventKind;
    using PointerKind=civ5::ui::ClientPointerKind;
    UINT message=0;
    WPARAM w=event.key;
    LPARAM l=0;
    switch(event.kind) {
    case Kind::close: message=WM_CLOSE; break;
    case Kind::key_down: message=WM_KEYDOWN; break;
    case Kind::key_up: message=WM_KEYUP; break;
    case Kind::character: message=WM_CHAR; break;
    case Kind::cancelled:
        // Binding delivered a completed release before its own ReleaseCapture.
        // Preserve the queued recovered producer release and the UI click.
        // External capture loss while still pressed remains a cancellation.
        if(event.native_flags==WM_CAPTURECHANGED && !event.pointer.cancel_camera &&
            !input.pointer_left_down) return;
        message=event.native_flags; break;
    case Kind::resize: case Kind::dpi_changed: return; // Transaction owns retirement.
    case Kind::pointer: {
        check(event.pointer.framebuffer.has_value(),"Native preview event has no drawable coordinate");
        const auto point=*event.pointer.framebuffer;
        check(point.x>=INT16_MIN && point.x<=INT16_MAX && point.y>=INT16_MIN && point.y<=INT16_MAX,
              "Native preview coordinate exceeds recovered signed-word producer");
        l=MAKELPARAM(static_cast<std::int16_t>(point.x),static_cast<std::int16_t>(point.y));
        message=event.pointer_kind==PointerKind::move ? WM_MOUSEMOVE :
            event.pointer_kind==PointerKind::left_down ? WM_LBUTTONDOWN :
            event.pointer_kind==PointerKind::left_up ? WM_LBUTTONUP : WM_MOUSEWHEEL;
        w=event.pointer_kind==PointerKind::wheel
            ? MAKEWPARAM(static_cast<WORD>(event.native_flags),static_cast<WORD>(event.wheel_delta))
            : static_cast<WPARAM>(event.native_flags);
        break;
    }
    }
        if (message == WM_CLOSE) {
            input.close = true;
            return;
        }
        if (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP) {
            input.pointer_x = static_cast<std::int16_t>(LOWORD(l));
            input.pointer_y = static_cast<std::int16_t>(HIWORD(l));
            input.pointer_valid = true;
            const auto in_rect = [&](civ5::ui::MinimapRect rect) {
                return input.pointer_x >= rect.x && input.pointer_y >= rect.y &&
                       std::int64_t(input.pointer_x) < std::int64_t(rect.x) + rect.width &&
                       std::int64_t(input.pointer_y) < std::int64_t(rect.y) + rect.height;
            };
            const auto in_minimap = [&] {
                if (!input.minimap_hit_panel) return false;
                const auto &panel = *input.minimap_hit_panel;
                return in_rect(panel.map) || in_rect(panel.strategic) || in_rect(panel.options) ||
                       (input.minimap_options_visible && in_rect(panel.options_panel));
            };
            const auto in_live_ui = [&] {
                return civ5::ui::live_ui_blocks_camera_pointer(input.live_hit_regions,
                    input.live_modal_open, true, input.pointer_x, input.pointer_y);
            };
            const auto pointer_kind = message == WM_MOUSEMOVE
                ? civ5::ui::ClientPointerKind::move : message == WM_LBUTTONDOWN
                ? civ5::ui::ClientPointerKind::left_down : civ5::ui::ClientPointerKind::left_up;
            (void)pointer_kind; // Native binding already routed this exact event.
            const auto &routed = event.pointer;
            if (routed.cancel_camera) input.cancel = true;
            input.observe_window_mouse(message,input.pointer_x,input.pointer_y);
            if (message == WM_LBUTTONDOWN) {
                input.pointer_left_down = true;
                input.pointer_press_x = input.pointer_x;
                input.pointer_press_y = input.pointer_y;
                input.live_ui_pressed = in_live_ui() ||
                    (routed.owner == civ5::ui::ClientInputOwner::ui && !in_minimap());
                input.minimap_pressed = !input.live_ui_pressed && in_minimap();
                if (input.minimap_pressed) {
                    input.minimap_press_x = input.pointer_x;
                    input.minimap_press_y = input.pointer_y;
                }
            } else if (message == WM_LBUTTONUP) {
                if (input.pointer_left_down && !input.minimap_pressed) {
                    input.top_panel_click_pending = true;
                    input.top_panel_click_x = input.pointer_x;
                    input.top_panel_click_y = input.pointer_y;
                }
                if (input.pointer_left_down && input.minimap_pressed && in_minimap()) {
                    input.minimap_click_pending = true;
                    input.minimap_click_x = input.pointer_x;
                    input.minimap_click_y = input.pointer_y;
                }
                input.pointer_left_down = false;
            }
            if (input.minimap_pressed) {
                if (message == WM_LBUTTONUP) input.minimap_pressed = false;
                ++input.pointer_messages;
                return;
            }
            if (input.live_ui_pressed || routed.owner == civ5::ui::ClientInputOwner::ui) {
                if (message == WM_LBUTTONUP) input.live_ui_pressed = false;
                ++input.pointer_messages;
                return;
            }
            const auto kind = message == WM_MOUSEMOVE     ? CameraPointerEventKind::move
                              : message == WM_LBUTTONDOWN ? CameraPointerEventKind::left_down
                                                          : CameraPointerEventKind::left_up;
            check(input.events.size() < 4096, "Preview input queue overflow");
            input.events.push_back(CameraPointerEvent{kind, static_cast<std::int16_t>(LOWORD(l)),
                                                       static_cast<std::int16_t>(HIWORD(l)),
                                                       static_cast<std::uint32_t>(w)});
            ++input.pointer_messages;
            return;
        }
        if (message == WM_MOUSEWHEEL) {
            POINT point{static_cast<std::int16_t>(LOWORD(l)),
                        static_cast<std::int16_t>(HIWORD(l))};
            check(input.live_ui_wheels.size() < 256, "Preview wheel queue overflow");
            input.live_ui_wheels.push_back({point.x, point.y,
                static_cast<std::int16_t>(HIWORD(w)), LOWORD(w)});
            ++input.wheel_messages;
            return;
        }
        if (message == WM_CHAR && input.search_focus) {
            check(input.search_characters.size() < 256,
                  "UI text input queue overflow");
            input.search_characters.push_back(static_cast<char16_t>(w));
            return;
        }
        if (message == WM_KEYDOWN && w < input.keys.size()) {
            if (input.search_focus) {
                if (w == VK_ESCAPE) {
                    input.search_cancel = true;
                    input.search_focus = false;
                }
                return;
            }
            if (input.router.keyboard_owner() == civ5::ui::ClientInputOwner::ui)
                return;
            if (!input.keys[w]) {
                if (w == VK_ESCAPE)
                    input.close = true;
                if (w == 'R')
                    input.reset = true;
                if (w == 'I')
                    input.inertia = !input.inertia;
                if (w == 'Z')
                    input.straight = !input.straight;
            }
            input.keys[w] = true;
            return;
        }
        if (message == WM_KEYUP && w < input.keys.size()) {
            input.keys[w] = false;
            return;
        }
        if (message == WM_KILLFOCUS || message == WM_CANCELMODE ||
            (message == WM_CAPTURECHANGED && reinterpret_cast<HWND>(l) != window)) {
            input.router.cancel();
            input.cancel = true;
            input.keys.fill(false);
            input.search_focus = false;
            input.search_cancel = true;
            input.coordinate_cleanup_pending=true;
            input.pointer_valid = false;
            input.pointer_left_down = false;
            input.pointer_press_x = input.pointer_press_y = -1;
            input.top_panel_click_pending = false;
            input.top_panel_click_x = input.top_panel_click_y = -1;
            input.coordinate_cleanup_pending=true;
            input.minimap_pressed = input.minimap_click_pending = false;
            input.live_ui_pressed = false;
            input.minimap_press_x = input.minimap_press_y = -1;
            input.minimap_click_x = input.minimap_click_y = -1;
        }
}
GlTexture make_glyphgen_rgba_texture(const GlContext &context, const civ5::ui::GlyphGenFont &font,
                                     const civ5::assets::DdsData &index, const civ5::assets::DdsData &packed) {
    const auto atlas = civ5::ui::reconstruct_glyphgen_rgba(font, index, packed);
    const civ5::assets::Rgba8Level level{atlas.width, atlas.height, atlas.rgba};
    return GlTexture::rgba8(context, std::span<const civ5::assets::Rgba8Level>(&level, 1));
}
struct PreviewMinimapChromeSource {
    civ5::ui::MinimapPanelLayout layout;
    civ5::ui::TopPanelGrid9Style style;
    civ5::assets::DdsData grid, strategic, options, fog;
    civ5::assets::Rgba8Level background, corner;
    std::optional<GlTexture> partial_map;
};
struct PreviewMinimapChrome {
    civ5::ui::MinimapPanelLayout layout;
    civ5::ui::TopPanelGrid9Style style;
    GlTexture grid, background, strategic, options, fog, corner;
    std::optional<GlTexture> partial_map;
    std::uint32_t grid_width, grid_height;
    PreviewMinimapChrome(const GlContext &context, PreviewMinimapChromeSource source)
        : layout(std::move(source.layout)), style(std::move(source.style)),
          grid(context, source.grid, DdsSamplingColor::srgb),
          background(GlTexture::rgba8(context, std::span<const civ5::assets::Rgba8Level>{
                                                   &source.background, 1}, DdsSamplingColor::srgb)),
          strategic(context, source.strategic, DdsSamplingColor::srgb),
          options(context, source.options, DdsSamplingColor::srgb),
          fog(context, source.fog, DdsSamplingColor::srgb),
          corner(GlTexture::rgba8(context, std::span<const civ5::assets::Rgba8Level>{
                                               &source.corner, 1}, DdsSamplingColor::srgb)),
          partial_map(std::move(source.partial_map)),
          grid_width(source.grid.width()), grid_height(source.grid.height()) {}
};
} // anonymous namespace: preserve existing helper storage.
namespace civ5::validation {
struct PreviewPresenter {
    GlRenderApi gl;
    GLuint program = 0, vao = 0, ui_program = 0, ui_vao = 0;
    GLint ui_rect = -1, ui_viewport = -1, ui_uv_rect = -1, ui_alpha_only = -1;
    GLint ui_force_opaque = -1, ui_tint = -1;
    template<class T>friend struct CameraPreviewLoop;
  private:
    std::uint64_t actual_client_present_returns_=0;
  public:
    civ5::ui::TopPanelLayout top_panel_layout;
    civ5::ui::TopPanelGrid9Style tooltip_style;
    civ5::ui::TopPanelLocalizationCatalog localization;
    GlTexture top_panel_bar;
    GlTexture tooltip_texture;
    GlTexture over_supply_texture;
    civ5::ui::GlyphGenFont top_panel_font;
    GlTexture top_panel_font_texture;
    civ5::ui::GlyphGenFont right_panel_font;
    GlTexture right_panel_font_texture;
    civ5::ui::GlyphGenFont icon_font;
    GlTexture icon_font_texture;
    GlTexture divider_texture;
    std::uint32_t divider_texture_width = 0, divider_texture_height = 0;
    civ5::ui::GlyphGenGeometry top_panel_geometry;
    civ5::ui::GlyphGenGeometry right_panel_geometry;
    civ5::ui::GlyphGenGeometry icon_geometry;
    std::unordered_map<std::string, std::uint32_t> icon_mapping;
    civ5::ui::TopPanelRuntime top_panel_runtime;
    civ5::ui::TopPanelControls top_panel_controls;
    std::optional<PreviewMinimapChrome> minimap_chrome;
    std::function<void(std::int32_t, std::int32_t)> prepare_live_screens;
    std::function<void(std::int32_t, std::int32_t)> paint_world_anchors;
    std::function<void(std::int32_t, std::int32_t)> paint_live_screens;
    std::function<bool()> client_frame_current;
    std::function<bool()> paint_frame_current;
    std::function<void()> commit_painted_frame;
    std::function<void()> complete_presented_research_refresh;
    std::shared_ptr<const civ5::ui::UiScreenGlRenderer::PresentationTarget> tech_award_presentation_target;
    std::shared_ptr<const civ5::ui::BnwTrainRendererRefreshReceipt> pending_train_presentation;
    std::shared_ptr<const civ5::ui::BnwOwnedCameraGuiScreen> pending_train_gui;
    std::function<void(const civ5::graphics::BnwTrainPresentedFrameProof&)> complete_presented_train_refresh;
    std::shared_ptr<civ5::graphics::BnwPendingUnitVisualOwner> pending_visual_receiver;
    std::shared_ptr<const civ5::materials::MaterialWorld> pending_visual_world;
    std::weak_ptr<const void> pending_visual_paint_lifetime;
    std::weak_ptr<civ5::ui::BnwClientSceneFrame> center_unit_scene;
    civ5::graphics::GlBnwWarriorOwnedScene* center_unit_imported=nullptr;
    std::weak_ptr<const void> center_unit_imported_lifetime;
    std::shared_ptr<civ5::graphics::BnwInitialCenterUnitRendererStorage> center_unit_renderer_storage;
    static std::weak_ptr<const void> observe_center_imported_lifetime(civ5::graphics::GlBnwWarriorOwnedScene* owner) {
        if(!owner)return {};
        if(owner->center_creator_!=std::this_thread::get_id())throw std::runtime_error("Center renderer camera foreign creator");
        return owner->center_lifetime_;
    }
    std::uint64_t completed_present_count=0;
    fs::path first_presented_capture_path;
    // Observation data only: same immutable scene stamp, copied without callbacks.
    std::optional<civ5::ui::BnwClientFrameStamp> first_presented_frame_stamp;
    std::function<void(const std::function<void(
        const civ5::ui::BnwOwnedCameraGuiScreen::Loan&)>&)> with_first_presented_screen;
    std::optional<civ5::ui::WorldViewRect> painted_tech_navigation_hit;
    std::function<void(std::int32_t, std::int32_t)> capture_client_frame;
    std::optional<civ5::ui::TopPanelPresentation> live_top_panel_presentation;
    std::optional<std::string> last_tooltip_text;
    std::optional<civ5::ui::TopPanelRect> last_science_hit;
    std::optional<civ5::ui::TopPanelRect> last_gold_hit;
    std::size_t last_tooltips_validated = 0;
    explicit PreviewPresenter(const GlContext &context, civ5::ui::TopPanelLayout layout,
                              civ5::ui::TopPanelGrid9Style style, civ5::ui::TopPanelLocalizationCatalog texts,
                              const civ5::assets::DdsData &top_panel_bar_data,
                              const civ5::assets::DdsData &tooltip_data,
                              const civ5::assets::DdsData &over_supply_data, civ5::ui::GlyphGenFont font,
                              PreviewAlphaFont prepared_font,
                              civ5::ui::GlyphGenFont right_font, PreviewAlphaFont prepared_right_font,
                              const civ5::assets::DdsData &divider,
                              civ5::ui::GlyphGenFont icons, const civ5::assets::DdsData &icon_index,
                              const civ5::assets::DdsData &icon_packed,
                              std::unordered_map<std::string, std::uint32_t> mapping,
                              std::function<civ5::ui::TopPanelState()> state_provider,
                              civ5::ui::TopPanelActionSink action_sink = {},
                              std::optional<PreviewMinimapChromeSource> minimap_source = {})
        : gl(context), top_panel_layout(std::move(layout)), tooltip_style(std::move(style)),
          localization(std::move(texts)),
          top_panel_bar(context, top_panel_bar_data, DdsSamplingColor::srgb),
          tooltip_texture(context, tooltip_data, DdsSamplingColor::srgb),
          over_supply_texture(context, over_supply_data, DdsSamplingColor::srgb),
          top_panel_font(std::move(font)),
          top_panel_font_texture(GlTexture::alpha8(context, prepared_font.atlas.width,
                                                   prepared_font.atlas.height, prepared_font.atlas.alpha)),
          right_panel_font(std::move(right_font)),
          right_panel_font_texture(GlTexture::alpha8(context, prepared_right_font.atlas.width,
                                                     prepared_right_font.atlas.height,
                                                     prepared_right_font.atlas.alpha)),
          icon_font(std::move(icons)),
          icon_font_texture(make_glyphgen_rgba_texture(context, icon_font, icon_index, icon_packed)),
          divider_texture(context, divider, DdsSamplingColor::srgb),
          divider_texture_width(divider.width()), divider_texture_height(divider.height()),
          top_panel_geometry(std::move(prepared_font.geometry)),
          right_panel_geometry(std::move(prepared_right_font.geometry)),
          icon_geometry(civ5::ui::glyphgen_geometry(icon_font)),
          icon_mapping(std::move(mapping)),
          top_panel_runtime(
              std::move(state_provider),
              {.text = [this](std::string_view key, const std::vector<std::string> &args) {
                  return localization.format(key, args);
              }},
              std::move(action_sink)),
          top_panel_controls(top_panel_layout, {.text = [this](std::string_view key,
                                                              const std::vector<std::string> &args) {
              return localization.format(key, args);
          }}) {
        if (minimap_source)
            minimap_chrome.emplace(context, std::move(*minimap_source));
        const char *vertex = "#version 460 core\n"
                             "out vec2 uv; void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);"
                             "uv=p;gl_Position=vec4(p*2.-1.,0,1);}";
        const char *fragment =
            "#version 460 core\n"
            "layout(binding=0) uniform sampler2D scene; in vec2 uv; out vec4 color;"
            "void main(){ivec2 size=textureSize(scene,0);"
            // Scene textures retain D3D top-to-bottom rows; window fragments
            // use GL's lower-left origin. Convert only at presentation.
            "ivec2 pixel=clamp(ivec2(uv*size),ivec2(0),size-1);pixel.y=size.y-1-pixel.y;"
            "vec3 c=clamp(texelFetch(scene,pixel,0).rgb,0.,1.);"
            "color=vec4(mix(12.92*c,1.055*pow(c,vec3(1./2.4))-.055,greaterThan(c,vec3(.0031308))),1);}";
        const auto vs = gl.CreateShader(GL_VERTEX_SHADER), ps = gl.CreateShader(GL_FRAGMENT_SHADER);
        gl.ShaderSource(vs, 1, &vertex, nullptr);
        gl.CompileShader(vs);
        gl.ShaderSource(ps, 1, &fragment, nullptr);
        gl.CompileShader(ps);
        program = gl.CreateProgram();
        gl.AttachShader(program, vs);
        gl.AttachShader(program, ps);
        gl.LinkProgram(program);
        gl.DeleteShader(vs);
        gl.DeleteShader(ps);
        GLint linked = 0;
        gl.GetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            gl.DeleteProgram(program);
            throw std::runtime_error("Preview display shader failed");
        }
        gl.CreateVertexArrays(1, &vao);

        // A triangle strip uses TL, TR, BL, BR. DDS and GlyphGen rows already
        // follow the authored top-to-bottom UV coordinates, without a flip.
        const char *ui_vertex = "#version 460 core\n"
                                "uniform vec4 rect; uniform vec2 viewport; out vec2 uv;"
                                "void main(){vec2 p=vec2((gl_VertexID&1)!=0?1.:0.,"
                                "(gl_VertexID>=2)?1.:0.); vec2 xy=rect.xy+p*rect.zw;"
                                "gl_Position=vec4(xy.x/viewport.x*2.-1.,1.-xy.y/viewport.y*2.,0.,1.);"
                                "uv=p;}";
        const char *ui_fragment = "#version 460 core\n"
                                  "layout(binding=0) uniform sampler2D source; uniform vec4 uvRect;"
                                  "uniform vec4 tint; uniform int alphaOnly,forceOpaque; in vec2 uv; out vec4 color;"
                                  "void main(){vec2 t=vec2(uvRect.x+uv.x*uvRect.z,"
                                  "uvRect.y+uv.y*uvRect.w); vec4 texel=texture(source,t);"
                                  "color=alphaOnly!=0?vec4(tint.rgb,texel.a*tint.a):"
                                  "vec4(texel.rgb*tint.rgb,(forceOpaque!=0?1.:texel.a)*tint.a);} ";
        const auto ui_vs = gl.CreateShader(GL_VERTEX_SHADER), ui_ps = gl.CreateShader(GL_FRAGMENT_SHADER);
        gl.ShaderSource(ui_vs, 1, &ui_vertex, nullptr);
        gl.CompileShader(ui_vs);
        gl.ShaderSource(ui_ps, 1, &ui_fragment, nullptr);
        gl.CompileShader(ui_ps);
        ui_program = gl.CreateProgram();
        gl.AttachShader(ui_program, ui_vs);
        gl.AttachShader(ui_program, ui_ps);
        gl.LinkProgram(ui_program);
        gl.DeleteShader(ui_vs);
        gl.DeleteShader(ui_ps);
        GLint ui_linked = 0;
        gl.GetProgramiv(ui_program, GL_LINK_STATUS, &ui_linked);
        if (!ui_linked) {
            gl.DeleteProgram(ui_program);
            ui_program = 0;
            throw std::runtime_error("TopPanel display shader failed");
        }
        ui_rect = gl.GetUniformLocation(ui_program, "rect");
        ui_viewport = gl.GetUniformLocation(ui_program, "viewport");
        ui_uv_rect = gl.GetUniformLocation(ui_program, "uvRect");
        ui_alpha_only = gl.GetUniformLocation(ui_program, "alphaOnly");
        ui_force_opaque = gl.GetUniformLocation(ui_program, "forceOpaque");
        ui_tint = gl.GetUniformLocation(ui_program, "tint");
        if (ui_rect < 0 || ui_viewport < 0 || ui_uv_rect < 0 || ui_alpha_only < 0 ||
            ui_force_opaque < 0 || ui_tint < 0) {
            gl.DeleteProgram(ui_program);
            ui_program = 0;
            throw std::runtime_error("TopPanel display shader uniforms missing");
        }
        gl.CreateVertexArrays(1, &ui_vao);
    }
    ~PreviewPresenter() {
        gl.DeleteVertexArrays(1, &ui_vao);
        gl.DeleteProgram(ui_program);
        gl.DeleteVertexArrays(1, &vao);
        gl.DeleteProgram(program);
    }
    void verify_scene_orientation(GlContext &context, int width, int height) {
        check(width >= 4 && height >= 4, "Preview orientation probe needs a visible viewport");
        // Upload rows in the same top-to-bottom order as the scene texture:
        // red/green on top, blue/yellow on the bottom.
        const civ5::assets::Rgba8Level pattern{
            2, 2, {255, 0, 0, 255, 0, 255, 0, 255,
                   0, 0, 255, 255, 255, 255, 0, 255}};
        auto scene = GlTexture::rgba8(context, std::span<const civ5::assets::Rgba8Level>(&pattern, 1));
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.Viewport(0, 0, width, height);
        gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        for (auto flag : {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_SCISSOR_TEST, GL_FRAMEBUFFER_SRGB})
            gl.Disable(flag);
        context.api().BindTextureUnit(0, scene.id());
        gl.BindSampler(0, 0);
        gl.UseProgram(program);
        gl.BindVertexArray(vao);
        gl.DrawArrays(GL_TRIANGLES, 0, 3);
        // GenericWorldAnchor is a world-space UI layer over the completed
        // scene, under the authored top panel and other screen contexts.
        if (paint_world_anchors) paint_world_anchors(width, height);
        const auto pixels = read_preview_frame_pixels(context, width, height);
        const auto pixel = [&](int x, int gl_y) {
            const auto offset = (std::size_t(gl_y) * width + x) * 3;
            return std::array{pixels[offset], pixels[offset + 1], pixels[offset + 2]};
        };
        check(pixel(width / 4, 3 * height / 4) == std::array<unsigned char, 3>{255, 0, 0} &&
                  pixel(3 * width / 4, 3 * height / 4) == std::array<unsigned char, 3>{0, 255, 0} &&
                  pixel(width / 4, height / 4) == std::array<unsigned char, 3>{0, 0, 255} &&
                  pixel(3 * width / 4, height / 4) == std::array<unsigned char, 3>{255, 255, 0},
              "Preview scene presentation is vertically or horizontally inverted");
    }
    std::optional<civ5::ui::TopPanelAction>
    draw(GlContext &context, GLuint image, int width, int height, std::int32_t pointer_x = -1,
         std::int32_t pointer_y = -1, bool pointer_valid = false, bool pointer_left_down = false,
         bool click_pending = false, std::int32_t press_x = -1, std::int32_t press_y = -1,
         std::int32_t click_x = -1, std::int32_t click_y = -1, const fs::path &capture_path = {},
         bool validate_all_tooltips = false, bool minimap_options_visible = false,
         const std::array<std::optional<bool>, civ5::ui::minimap_map_option_count> *
             minimap_map_display = nullptr) {
        const bool trace_first_presentation=completed_present_count==0;
        if(trace_first_presentation)camera_stage_mark("first_paint","BEGIN");
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.Viewport(0, 0, width, height);
        gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        for (auto flag : {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_SCISSOR_TEST, GL_FRAMEBUFFER_SRGB})
            gl.Disable(flag);
        context.api().BindTextureUnit(0, image);
        gl.BindSampler(0, 0);
        gl.UseProgram(program);
        gl.BindVertexArray(vao);
        gl.DrawArrays(GL_TRIANGLES, 0, 3);

        const auto plan = civ5::ui::plan_top_panel(top_panel_layout, static_cast<std::uint32_t>(width),
                                                    static_cast<std::uint32_t>(height));
        gl.Enable(GL_BLEND);
        gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        gl.UseProgram(ui_program);
        gl.BindVertexArray(ui_vao);
        gl.BindSampler(0, 0);
        const std::array<float, 2> viewport{static_cast<float>(width), static_cast<float>(height)};
        gl.ProgramUniform2fv(ui_program, ui_viewport, 1, viewport.data());
        const auto draw_rect = [&](const GlTexture &texture, const civ5::ui::TopPanelRect &rect,
                                   const std::array<float, 4> &uv, bool alpha_only,
                                   const std::array<float, 4> &tint,
                                   bool force_opaque = false) {
            const std::array<float, 4> pixels{static_cast<float>(rect.x), static_cast<float>(rect.y),
                                               static_cast<float>(rect.width), static_cast<float>(rect.height)};
            texture.bind(0);
            gl.ProgramUniform4fv(ui_program, ui_rect, 1, pixels.data());
            gl.ProgramUniform4fv(ui_program, ui_uv_rect, 1, uv.data());
            gl.ProgramUniform1i(ui_program, ui_alpha_only, alpha_only ? 1 : 0);
            gl.ProgramUniform1i(ui_program, ui_force_opaque, force_opaque ? 1 : 0);
            gl.ProgramUniform4fv(ui_program, ui_tint, 1, tint.data());
            gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        };
        draw_rect(top_panel_bar, plan.center_bar, {0, 0, 1, 1}, false, {1, 1, 1, 1});
        draw_rect(top_panel_bar, plan.right_bar, {0, 0, 1, 1}, false, {1, 1, 1, 1});
        if (minimap_chrome) {
            const auto &mini = *minimap_chrome;
            const auto panel = civ5::ui::plan_minimap_panel(
                mini.layout, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
            const auto draw_minimap_rect = [&](const GlTexture &texture, civ5::ui::MinimapRect rect,
                                               std::array<float, 4> uv,
                                               bool force_opaque = false,
                                               float alpha = 1.0f) {
                check(rect.x >= 0 && rect.y >= 0 && rect.width > 0 && rect.height > 0,
                      "Minimap chrome rectangle is invalid");
                draw_rect(texture,
                          {static_cast<std::uint32_t>(rect.x), static_cast<std::uint32_t>(rect.y),
                           rect.width, rect.height}, uv, false, {1, 1, 1, alpha}, force_opaque);
            };
            const auto left_width = mini.style.source[0].width;
            const auto right_width = mini.style.source[2].width;
            const auto top_height = mini.style.source[0].height;
            const auto bottom_height = mini.style.source[6].height;
            const auto draw_grid9 = [&](civ5::ui::MinimapRect target) {
                check(left_width > 0 && right_width > 0 && top_height > 0 && bottom_height > 0 &&
                          target.width >= static_cast<std::uint32_t>(left_width + right_width) &&
                          target.height >= static_cast<std::uint32_t>(top_height + bottom_height),
                      "Minimap Grid9 corners exceed its destination");
                const std::array<std::int32_t, 4> xs{
                    target.x, target.x + left_width,
                    target.x + static_cast<std::int32_t>(target.width) - right_width,
                    target.x + static_cast<std::int32_t>(target.width)};
                const std::array<std::int32_t, 4> ys{
                    target.y, target.y + top_height,
                    target.y + static_cast<std::int32_t>(target.height) - bottom_height,
                    target.y + static_cast<std::int32_t>(target.height)};
                for (std::size_t index = 0; index < mini.style.source.size(); ++index) {
                    const auto &source = mini.style.source[index];
                    const auto col = index % 3, row = index / 3;
                    check(source.x >= 0 && source.y >= 0 && source.width > 0 && source.height > 0 &&
                              static_cast<std::uint64_t>(source.x + source.width) <= mini.grid_width &&
                              static_cast<std::uint64_t>(source.y + source.height) <= mini.grid_height,
                          "Minimap Grid9 source exceeds its DDS");
                    draw_minimap_rect(mini.grid,
                                      {xs[col], ys[row], static_cast<std::uint32_t>(xs[col + 1] - xs[col]),
                                       static_cast<std::uint32_t>(ys[row + 1] - ys[row])},
                                      {static_cast<float>(source.x) / mini.grid_width,
                                       static_cast<float>(source.y) / mini.grid_height,
                                       static_cast<float>(source.width) / mini.grid_width,
                                       static_cast<float>(source.height) / mini.grid_height});
                }
            };
            if (minimap_options_visible)
                draw_grid9(panel.options_panel);
            draw_minimap_rect(mini.background, panel.background, {0, 0, 1, 1});
            // XML paints SideStack, the background and the button stack before
            // the map frame. Keep that order at their overlapping edges.
            draw_minimap_rect(mini.strategic, panel.strategic, {0, 0, 1, 1}, false, .5f);
            draw_minimap_rect(mini.options, panel.options, {0, 0, 1, 1});
            draw_grid9(panel.frame);
            draw_minimap_rect(mini.corner, panel.corner, {0, 0, 1, 1});
            if (mini.partial_map) {
                // Render targets use GL's bottom row first. The UI quad uses
                // authored top-to-bottom UVs, so invert V exactly once here.
                // The map target already contains blended sprite colors;
                // blending its fractional alpha again would darken their edges.
                draw_minimap_rect(*mini.partial_map, panel.map, {0, 1, 1, -1}, true);
            } else {
                draw_minimap_rect(mini.fog, panel.map, {0, 0, 1, 1});
            }
        }
        const auto resolver = [this](std::string_view name) -> std::optional<std::uint32_t> {
            const auto found = icon_mapping.find(std::string(name));
            return found == icon_mapping.end() ? std::nullopt : std::optional<std::uint32_t>(found->second);
        };
        const auto draw_projected_line = [&](const civ5::ui::GlyphGenFont &text_font,
                                             const civ5::ui::GlyphGenGeometry &text_geometry,
                                             const GlTexture &text_texture,
                                             const civ5::ui::GlyphGenInlineLine &line,
                                             std::int32_t origin_x, std::int32_t baseline_y) -> std::int32_t {
            for (const auto &glyph : line.glyphs) {
                const auto &font = glyph.icon ? icon_font : text_font;
                const auto atlas_width = static_cast<float>(font.atlas_width());
                const auto atlas_height = static_cast<float>(font.atlas_height());
                const auto x = origin_x + glyph.left_from_line_origin;
                const auto y = baseline_y - glyph.top_from_baseline;
                check(x >= 0 && y >= 0 && glyph.width > 0 && glyph.height > 0,
                      "TopPanel markup projected outside the viewport");
                check(static_cast<std::uint64_t>(x) + glyph.width <= static_cast<std::uint32_t>(width) &&
                          static_cast<std::uint64_t>(y) + glyph.height <= static_cast<std::uint32_t>(height),
                      "TopPanel markup exceeds the viewport");
                const civ5::ui::TopPanelRect rect{static_cast<std::uint32_t>(x),
                                                  static_cast<std::uint32_t>(y),
                                                  static_cast<std::uint32_t>(glyph.width),
                                                  static_cast<std::uint32_t>(glyph.height)};
                const std::array<float, 4> uv{
                    static_cast<float>(glyph.atlas_x) / atlas_width,
                    static_cast<float>(glyph.atlas_y) / atlas_height,
                    static_cast<float>(glyph.width) / atlas_width,
                    static_cast<float>(glyph.height) / atlas_height};
                const std::array<float, 4> tint{
                    glyph.color[0] / 255.f, glyph.color[1] / 255.f, glyph.color[2] / 255.f, glyph.color[3] / 255.f};
                draw_rect(glyph.icon ? icon_font_texture : text_texture, rect, uv, !glyph.icon, tint);
            }
            return line.advance_width;
        };
        const auto draw_line = [&](const civ5::ui::GlyphGenFont &text_font,
                                   const civ5::ui::GlyphGenGeometry &text_geometry,
                                   const GlTexture &text_texture, std::string_view markup,
                                   std::int32_t origin_x, std::int32_t baseline_y) -> std::int32_t {
            return draw_projected_line(text_font, text_geometry, text_texture,
                                       civ5::ui::project_glyphgen_markup(text_geometry, icon_geometry, markup, resolver),
                                       origin_x, baseline_y);
        };
        if (minimap_options_visible && minimap_chrome) {
            const auto panel = civ5::ui::plan_minimap_panel(minimap_chrome->layout,
                static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
            for (const auto &source : minimap_chrome->layout.map_options) {
                const auto index = static_cast<std::size_t>(source.option);
                const auto row = civ5::ui::minimap_map_display_row(panel.options_panel, index);
                const auto value = minimap_map_display ? (*minimap_map_display)[index]
                                                       : std::optional<bool>{};
                // '?' is an indeterminate checkbox, never an unchecked guess.
                const std::string marker = value ? (*value ? "[X]" : "[ ]") : "[?]";
                draw_line(top_panel_font, top_panel_geometry, top_panel_font_texture,
                          marker, row.x, row.y + 21);
                draw_line(top_panel_font, top_panel_geometry, top_panel_font_texture,
                          localization.format(source.text_key), row.x + 34, row.y + 21);
            }
        }
        if (prepare_live_screens) prepare_live_screens(width, height);
        const auto *presentation = &top_panel_controls.apply(
            live_top_panel_presentation ? live_top_panel_presentation : top_panel_runtime.present());
        const auto info_stack = civ5::ui::plan_top_panel_stack(top_panel_layout.info_stack);
        check(info_stack.growth == civ5::ui::TopPanelStackGrowth::right,
              "TopPanel info stack changed its authored growth direction");
        std::int32_t left = info_stack.offset_x + 8;
        const std::int32_t left_baseline = 17 + info_stack.offset_y;
        const std::int32_t left_hit_y = std::max(0, left_baseline - 19);
        std::array<civ5::ui::TopPanelRect, 10> left_hits{};
        const auto draw_value = [&](const std::optional<std::string> &value,
                                    const std::optional<std::string> &tooltip, std::size_t index) {
            if (!presentation->show_info_stack || !value)
                return;
            const auto value_width = draw_line(top_panel_font, top_panel_geometry, top_panel_font_texture, *value,
                                               left, left_baseline);
            left_hits[index] = {static_cast<std::uint32_t>(left), static_cast<std::uint32_t>(left_hit_y),
                                static_cast<std::uint32_t>(value_width), 28};
            left += value_width + info_stack.padding;
        };
        draw_value(presentation->science_text, presentation->science_tooltip, 0);
        draw_value(presentation->gold_text, presentation->gold_tooltip, 1);
        draw_value(presentation->international_trade_routes_text,
                   presentation->international_trade_routes_tooltip, 2);
        draw_value(presentation->happiness_text, presentation->happiness_tooltip, 3);
        draw_value(presentation->golden_age_text, presentation->golden_age_tooltip, 4);
        draw_value(presentation->culture_text, presentation->culture_tooltip, 5);
        draw_value(presentation->tourism_text, presentation->tourism_tooltip, 6);
        draw_value(presentation->faith_text, presentation->faith_tooltip, 7);
        draw_value(presentation->resource_text, presentation->resource_tooltip, 8);
        last_science_hit = presentation->show_info_stack && presentation->science_text
                            ? std::optional<civ5::ui::TopPanelRect>(left_hits[0])
                            : std::nullopt;
        last_gold_hit = presentation->show_info_stack && presentation->gold_text
                            ? std::optional<civ5::ui::TopPanelRect>(left_hits[1])
                            : std::nullopt;
        if (presentation->show_info_stack && presentation->show_unit_supply) {
            const auto supply_element = std::find_if(
                top_panel_layout.info_stack.children.begin(), top_panel_layout.info_stack.children.end(),
                [](const civ5::ui::TopPanelElement &element) {
                    return element.attribute("ID") == "UnitSupplyString";
                });
            check(supply_element != top_panel_layout.info_stack.children.end(),
                  "TopPanel unit-supply source changed");
            const auto supply_size = civ5::ui::top_panel_element_size(*supply_element);
            const auto supply_offset = civ5::ui::top_panel_element_offset(*supply_element);
            const auto supply_y = info_stack.offset_y + supply_offset.y - 2;
            check(supply_y >= 0 && supply_size.width != 0 && supply_size.height != 0,
                  "TopPanel unit-supply geometry exceeds the authored panel");
            const civ5::ui::TopPanelRect supply_hit{static_cast<std::uint32_t>(left),
                                                    static_cast<std::uint32_t>(supply_y), supply_size.width,
                                                    supply_size.height};
            draw_rect(over_supply_texture, supply_hit, {0, 0, 1, 1}, false, {1, 1, 1, 1});
            left_hits[9] = {static_cast<std::uint32_t>(left), static_cast<std::uint32_t>(std::max(0, supply_y - 1)),
                            supply_size.width, 28};
        }
        const auto measure_right_line = [&](std::string_view markup) {
            return civ5::ui::project_glyphgen_markup(right_panel_geometry, icon_geometry, markup, resolver)
                .advance_width;
        };
        const auto right_stack = civ5::ui::plan_top_panel_stack(top_panel_layout.right_stack);
        check(right_stack.growth == civ5::ui::TopPanelStackGrowth::left,
              "TopPanel right stack changed its authored growth direction");
        const auto divider_element = std::find_if(
            top_panel_layout.right_stack.children.begin(), top_panel_layout.right_stack.children.end(),
            [](const civ5::ui::TopPanelElement &element) {
                return element.name == "Image" &&
                       element.attribute("Texture") == "assets\\UI\\Art\\WorldView\\Divider.dds";
            });
        check(divider_element != top_panel_layout.right_stack.children.end(),
              "TopPanel right stack divider source changed");
        const auto divider_size = civ5::ui::top_panel_element_size(*divider_element);
        const auto divider_offset = civ5::ui::top_panel_element_texture_offset(*divider_element);
        check(divider_size.width != 0 && divider_size.height != 0,
              "TopPanel divider has an empty authored size");
        const auto divider_texture_width_f = static_cast<float>(divider_texture_width);
        const auto divider_texture_height_f = static_cast<float>(divider_texture_height);
        check(divider_offset.x >= 0 && divider_offset.y >= 0 &&
                  divider_offset.x + static_cast<std::int32_t>(divider_size.width) <=
                      static_cast<std::int32_t>(divider_texture_width) &&
                  divider_offset.y < static_cast<std::int32_t>(divider_texture_height),
              "TopPanel divider texture crop exceeds its source DDS");
        const auto divider_source_height = divider_texture_height - static_cast<std::uint32_t>(divider_offset.y);
        const std::array<float, 4> authored_divider_uv{
            static_cast<float>(divider_offset.x) / divider_texture_width_f,
            static_cast<float>(divider_offset.y) / divider_texture_height_f,
            static_cast<float>(divider_size.width) / divider_texture_width_f,
            static_cast<float>(divider_source_height) / divider_texture_height_f};
        const std::int32_t right_baseline = 20 + right_stack.offset_y;
        const std::int32_t right_hit_y = std::max(0, right_baseline - 20);
        const auto draw_right_line = [&](std::string_view markup, std::int32_t origin_x, bool hovered,
                                         bool pressed) {
            const auto text = pressed ? "[COLOR:255:255:200:200]" + std::string(markup) + "[/COLOR]"
                              : hovered ? "[COLOR:255:255:255:255]" + std::string(markup) + "[/COLOR]"
                                        : std::string(markup);
            draw_line(right_panel_font, right_panel_geometry, right_panel_font_texture, text, origin_x,
                      20 + right_stack.offset_y);
        };
        const civ5::ui::TopPanelTextLocalizer localizer{
            [this](std::string_view key, const std::vector<std::string> &args) {
                return localization.format(key, args);
            }};
        const auto localized_right = [&](std::string_view key) {
            return localizer.text(key, {});
        };
        std::int32_t right_cursor = width - right_stack.offset_x;
        const auto menu_width = measure_right_line(presentation->menu_button->text);
        right_cursor -= menu_width;
        const auto menu_x = right_cursor;
        check(menu_x >= 0, "TopPanel menu button exceeds the viewport");
        const civ5::ui::TopPanelRect menu_hit{static_cast<std::uint32_t>(menu_x),
                                              static_cast<std::uint32_t>(right_hit_y),
                                              static_cast<std::uint32_t>(menu_width), 28};
        const auto menu_hover = pointer_valid && pointer_x >= static_cast<std::int32_t>(menu_hit.x) &&
                                pointer_x < static_cast<std::int32_t>(menu_hit.x + menu_hit.width) && pointer_y >= 0 &&
                                pointer_y < 32;
        const auto menu_pressed = menu_hover && pointer_left_down;
        draw_right_line(presentation->menu_button->text, menu_x, menu_hover, menu_pressed);
        right_cursor = menu_x - right_stack.padding;
        check(right_cursor - static_cast<std::int32_t>(divider_size.width) >= 0,
              "TopPanel first divider exceeds the viewport");
        draw_rect(divider_texture,
                  {static_cast<std::uint32_t>(right_cursor - static_cast<std::int32_t>(divider_size.width)), 8,
                   divider_size.width, divider_size.height},
                  authored_divider_uv, false, {1, 1, 1, 1});
        right_cursor -= static_cast<std::int32_t>(divider_size.width) + right_stack.padding;
        const auto help = localized_right("TXT_KEY_HELP_CAPS");
        const auto help_width = measure_right_line(help);
        right_cursor -= help_width;
        const auto help_x = right_cursor;
        check(help_x >= 0, "TopPanel help button exceeds the viewport");
        const civ5::ui::TopPanelRect help_hit{static_cast<std::uint32_t>(help_x),
                                              static_cast<std::uint32_t>(right_hit_y),
                                              static_cast<std::uint32_t>(help_width), 28};
        const auto help_hover = pointer_valid && pointer_x >= static_cast<std::int32_t>(help_hit.x) &&
                                pointer_x < static_cast<std::int32_t>(help_hit.x + help_hit.width) && pointer_y >= 0 &&
                                pointer_y < 32;
        const auto help_pressed = help_hover && pointer_left_down;
        draw_right_line(help, help_x, help_hover, help_pressed);
        right_cursor = help_x - right_stack.padding;
        check(right_cursor - static_cast<std::int32_t>(divider_size.width) >= 0,
              "TopPanel second divider exceeds the viewport");
        draw_rect(divider_texture,
                  {static_cast<std::uint32_t>(right_cursor - static_cast<std::int32_t>(divider_size.width)), 8,
                   divider_size.width, divider_size.height},
                  authored_divider_uv, false, {1, 1, 1, 1});
        right_cursor -= static_cast<std::int32_t>(divider_size.width) + right_stack.padding;
        const auto date_width = measure_right_line(presentation->current_date_text);
        right_cursor -= date_width;
        const auto date_x = right_cursor;
        check(date_x >= 0, "TopPanel date exceeds the viewport");
        draw_right_line(presentation->current_date_text, date_x, false, false);
        right_cursor = date_x - right_stack.padding;
        const auto turn_width = measure_right_line(presentation->current_turn_text);
        right_cursor -= turn_width;
        check(right_cursor >= 0, "TopPanel turn exceeds the viewport");
        draw_right_line(presentation->current_turn_text, right_cursor, false, false);
        const std::array<const std::optional<std::string> *, 10> info_tooltips{
            &presentation->science_tooltip, &presentation->gold_tooltip,
            &presentation->international_trade_routes_tooltip, &presentation->happiness_tooltip,
            &presentation->golden_age_tooltip, &presentation->culture_tooltip,
            &presentation->tourism_tooltip, &presentation->faith_tooltip,
            &presentation->resource_tooltip, &presentation->unit_supply_tooltip};
        if (validate_all_tooltips) {
            last_tooltips_validated = 0;
            const auto validate = [&](std::string_view name, std::string_view text) {
                try {
                    const auto planned = civ5::ui::plan_top_panel_tooltip(
                        top_panel_layout, tooltip_style, top_panel_geometry, icon_geometry, text, resolver,
                        {640, 38}, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
                    check(!planned.lines.empty(), "TopPanel tooltip projection is empty");
                    const auto &content = planned.grid.content;
                    for (std::size_t line = 0; line < planned.lines.size(); ++line)
                        for (const auto &glyph : planned.lines[line].glyphs) {
                            const auto x = std::int64_t{planned.text_origin.x} + glyph.left_from_line_origin;
                            const auto y = std::int64_t{planned.text_origin.y} +
                                           static_cast<std::int64_t>(line) * planned.line_step -
                                           glyph.top_from_baseline;
                            check(x >= content.x && y >= content.y &&
                                      x + glyph.width <= std::int64_t{content.x} + content.width &&
                                      y + glyph.height <= std::int64_t{content.y} + content.height,
                                  "TopPanel tooltip glyph exceeds its content frame");
                        }
                    ++last_tooltips_validated;
                } catch (const std::exception &error) {
                    throw std::runtime_error("TopPanel " + std::string(name) + " tooltip: " + error.what());
                }
            };
            validate("menu", presentation->menu_button->tooltip);
            validate("help", localized_right("TXT_KEY_CIVILOPEDIA_TOOLTIP"));
            constexpr std::array<std::string_view, 10> names{
                "science", "gold", "trade routes", "happiness", "golden age", "culture",
                "tourism", "faith", "resource", "unit supply"};
            for (std::size_t i = 0; i < info_tooltips.size(); ++i)
                if (*info_tooltips[i])
                    validate(names[i], **info_tooltips[i]);
        }
        std::optional<std::string> hover_tooltip;
        if (menu_hover)
            hover_tooltip = presentation->menu_button->tooltip;
        else if (help_hover)
            hover_tooltip = localized_right("TXT_KEY_CIVILOPEDIA_TOOLTIP");
        else if (pointer_valid && pointer_y >= 0 && pointer_y < 32) {
            for (std::size_t i = 0; i < left_hits.size(); ++i) {
                const auto &hit = left_hits[i];
                if (pointer_x >= static_cast<std::int32_t>(hit.x) &&
                    pointer_x < static_cast<std::int32_t>(hit.x + hit.width) && info_tooltips[i]->has_value()) {
                    hover_tooltip = *info_tooltips[i];
                    break;
                }
            }
        }
        if (hover_tooltip) {
            last_tooltip_text = *hover_tooltip;
            const auto tooltip_plan = civ5::ui::plan_top_panel_tooltip(
                top_panel_layout, tooltip_style, top_panel_geometry, icon_geometry, *hover_tooltip, resolver,
                {pointer_x, 32 + 6}, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
            const auto texture_width = static_cast<float>(tooltip_texture.levels().front().width);
            const auto texture_height = static_cast<float>(tooltip_texture.levels().front().height);
            for (std::size_t i = 0; i < tooltip_plan.grid.destination.size(); ++i) {
                const auto &source = tooltip_style.source[i];
                const auto source_x = source.x + tooltip_style.state_offset.x;
                const auto source_y = source.y + tooltip_style.state_offset.y;
                const std::array<float, 4> uv{
                    static_cast<float>(source_x) / texture_width,
                    static_cast<float>(source_y) / texture_height,
                    static_cast<float>(source.width) / texture_width,
                    static_cast<float>(source.height) / texture_height};
                draw_rect(tooltip_texture, tooltip_plan.grid.destination[i], uv, false, {1, 1, 1, 1});
            }
            std::size_t line_number = 0;
            for (const auto &line : tooltip_plan.lines) {
                draw_projected_line(top_panel_font, top_panel_geometry, top_panel_font_texture, line,
                                    tooltip_plan.text_origin.x,
                                    tooltip_plan.text_origin.y + line_number * tooltip_plan.line_step);
                ++line_number;
            }
        } else {
            last_tooltip_text.reset();
        }
        // Exact painted Science target used by the existing TopPanel dispatch.
        painted_tech_navigation_hit.reset();
        if(presentation->science_action==civ5::ui::TopPanelAction::open_technology_tree && left_hits[0].width>0)
            painted_tech_navigation_hit=civ5::ui::WorldViewRect{
                static_cast<std::int32_t>(left_hits[0].x),0,static_cast<std::int32_t>(left_hits[0].width),32};
        const auto inside = [](const civ5::ui::TopPanelRect &hit, std::int32_t x, std::int32_t y) {
            return x >= static_cast<std::int32_t>(hit.x) &&
                   x < static_cast<std::int32_t>(hit.x + hit.width) && y >= 0 && y < 32;
        };
        std::optional<civ5::ui::TopPanelAction> action;
        if (click_pending && inside(menu_hit, press_x, press_y) && inside(menu_hit, click_x, click_y))
            action = presentation->menu_button->action;
        else if (click_pending && inside(help_hit, press_x, press_y) && inside(help_hit, click_x, click_y))
            action = presentation->civilopedia_action;
        else if (click_pending) {
            const std::array<const civ5::ui::TopPanelAction *, 10> actions{
                &presentation->science_action, nullptr, nullptr, nullptr, nullptr,
                &presentation->culture_action, nullptr, nullptr, nullptr, nullptr};
            for (std::size_t i = 0; i < left_hits.size(); ++i)
                if (actions[i] && inside(left_hits[i], press_x, press_y) && inside(left_hits[i], click_x, click_y)) {
                    action = *actions[i];
                    break;
                }
        }
        gl.Disable(GL_BLEND);
        if (paint_live_screens) paint_live_screens(width, height);
        if (commit_painted_frame) commit_painted_frame();
        if(trace_first_presentation)camera_stage_mark("first_paint","END");
        // Painting may call external owners. Never capture or swap a stale
        // scene/UI pair, even when the write kept the same game turn.
        if (client_frame_current && !client_frame_current())
            throw std::runtime_error("BNW client frame became stale before presentation");
        if (capture_client_frame) capture_client_frame(width, height);
        if (client_frame_current && !client_frame_current())
            throw std::runtime_error("BNW client frame became stale during capture");
        if (!capture_path.empty())
            save_preview_frame(context, capture_path, width, height);
        if (client_frame_current && !client_frame_current())
            throw std::runtime_error("BNW viewport publication became stale before swap");
        bool observed_present_returned=false,observed_client_post_returned=false;
        std::int64_t observed_present_tick=0,observed_client_post_tick=0;
        const auto present_current_frame = [&] {
            if(trace_first_presentation)camera_stage_mark("first_present","BEGIN");
            context.present();
            observed_present_returned=true;
            observed_present_tick=first_presented_observation_tick();
            if (client_frame_current && !client_frame_current())
                throw std::runtime_error("BNW viewport publication became stale before action dispatch");
            // Record only an actually installed POST check; no absent-guard grant.
            observed_client_post_returned=static_cast<bool>(client_frame_current);
            if(client_frame_current){
                check(actual_client_present_returns_<std::numeric_limits<std::uint64_t>::max(),"Actual client presentation counter overflow");
                ++actual_client_present_returns_;
            }
            observed_client_post_tick=first_presented_observation_tick();
            if(tech_award_presentation_target) {
                const auto target=tech_award_presentation_target;
                target->acknowledge(context); // Closed actual presenter, after Swap and native/source POST.
                if(client_frame_current&&!client_frame_current())
                    throw std::runtime_error("TechAward viewport retired after actual-present acknowledgement");
            }
            if(trace_first_presentation)camera_stage_mark("first_present","END");
        };
        if(trace_first_presentation && !first_presented_capture_path.empty() && with_first_presented_screen){
            // This short real screen loan starts AFTER paint/commit and ends
            // BEFORE research completion or input dispatch can change the GUI.
            with_first_presented_screen([&](const civ5::ui::BnwOwnedCameraGuiScreen::Loan& screen){
                const auto current = [&] {
                    screen.require_current();
                    check(screen.width()==width && screen.height()==height,
                          "First presented capture differs from actual composed viewport");
                    if(paint_frame_current && !paint_frame_current())
                        throw std::runtime_error("First presented capture has a stale Scene/UI pair");
                    if(client_frame_current && !client_frame_current())
                        throw std::runtime_error("First presented capture has a stale viewport publication");
                    screen.require_current();
                };
                current();
                const auto pixels=read_preview_frame_pixels(context,width,height); // GL_BACK, before Swap.
                current();
                present_current_frame(); // Native SwapBuffers success plus existing viewport bookend.
                current();
                bool p6_written=false;
                try {
                    save_preview_frame_pixels(first_presented_capture_path,width,height,pixels);
                    p6_written=true;
                }catch(...){
                    // Only optional PPM storage failed, after an actual returned present.
                    camera_stage_mark("first_capture","WRITE_FAILED");
                }
                current(); // Source/GL failures remain outside the storage-only catch.
                if(p6_written) {
                    const FirstPresentedObservationErrors restore_observation_errors;
                    try {
                        if(!first_presented_frame_stamp)
                            throw std::runtime_error("First-presented scene stamp observation unavailable");
                        save_first_presented_post_metadata(first_presented_capture_path,width,height,pixels,
                            *first_presented_frame_stamp,observed_present_returned,observed_client_post_returned,
                            observed_present_tick,observed_client_post_tick,context.native_identity());
                    } catch(...) {
                        // Optional sidecar failure cannot change successful P6/present.
                        camera_stage_mark("first_capture_metadata","WRITE_FAILED");
                    }
                }
            });
        }else{
            if(trace_first_presentation)
                camera_stage_mark("first_capture","NO_PUBLISHED_SCREEN_OR_OUTPUT");
            present_current_frame(); // Optional capture cannot prevent the legacy render path.
        }
        check(completed_present_count<std::numeric_limits<std::uint64_t>::max(),"Actual present trace overflow");
        ++completed_present_count; // Only a returned present and its viewport bookend.
        if (complete_presented_research_refresh) complete_presented_research_refresh();
        if (client_frame_current && !client_frame_current())
            throw std::runtime_error("BNW viewport publication changed during research presentation completion");
        if(pending_train_presentation){
            // Only this existing draw owner can mint: actual world/UI paint,
            // returned context.present and source/viewport guards precede it.
            if(paint_frame_current&&!paint_frame_current())
                throw std::runtime_error("TRAIN actual painted Scene/UI retired before proof");
            check(pending_train_gui&&complete_presented_train_refresh,
                  "TRAIN real pending presentation lost its GUI/target");
            civ5::graphics::BnwTrainPresentedFrameProof::with_actual_presented_frame(
                pending_train_presentation,pending_train_gui,complete_presented_train_refresh);
            if(client_frame_current&&!client_frame_current())
                throw std::runtime_error("TRAIN viewport retired during presentation completion");
            // Actual copied target was destroyed inside its proof scope.
            complete_presented_train_refresh={};pending_train_gui.reset();pending_train_presentation.reset();
        }
        if(pending_visual_receiver){
            check(!pending_visual_paint_lifetime.expired()&&pending_visual_world,
                  "Pending visual camera source retired after actual present");
            if(paint_frame_current&&!paint_frame_current())
                throw std::runtime_error("Pending visual terrain has no current painted Scene/UI");
            if(client_frame_current&&!client_frame_current())
                throw std::runtime_error("Pending visual terrain viewport retired after actual present");
            pending_visual_receiver->bind_actual_camera_terrain(
                pending_visual_world,pending_visual_paint_lifetime);
            if(paint_frame_current&&!paint_frame_current())
                throw std::runtime_error("Pending visual terrain Scene/UI retired during attachment");
            if(client_frame_current&&!client_frame_current())
                throw std::runtime_error("Pending visual terrain viewport retired during attachment");
        }
        if(center_unit_imported&&!center_unit_renderer_storage) {
            if(center_unit_imported_lifetime.expired()||pending_visual_paint_lifetime.expired())
                throw std::runtime_error("Center renderer actual owner retired after present");
            const auto scene=center_unit_scene.lock();
            if(scene) {
                if(paint_frame_current&&!paint_frame_current())throw std::runtime_error("Center renderer painted Scene/UI retired");
                if(client_frame_current&&!client_frame_current())throw std::runtime_error("Center renderer native viewport retired");
                if(center_unit_imported_lifetime.expired()||pending_visual_paint_lifetime.expired())
                    throw std::runtime_error("Center renderer retired in final camera bookend");
                center_unit_renderer_storage=civ5::graphics::BnwInitialCenterUnitRendererStorage::after_actual_present(
                    scene,*center_unit_imported,center_unit_imported_lifetime,pending_visual_paint_lifetime,completed_present_count);
            }
        }
        if (action)
            top_panel_runtime.dispatch(*action);
        return action;
    }
};
} // namespace civ5::validation
namespace {
using civ5::validation::PreviewPresenter;
bool preview_patch_visible(const std::array<float, 6> &bounds, const LandMaterialConstants &camera) {
    // Conservative diagnostic culling only. Include a world-unit margin and
    // the entire A8 displacement range; verify against unculled raster below.
    std::array<bool, 6> outside{true, true, true, true, true, true};
    for (unsigned corner = 0; corner < 8; ++corner) {
        std::array<double, 4> p{1, 1, 1, 1}, clip{};
        for (unsigned axis = 0; axis < 3; ++axis)
            p[axis] = corner & (1u << axis) ? bounds[axis + 3] + 1. : bounds[axis] - 1.;
        for (unsigned col = 0; col < 4; ++col)
            for (unsigned row = 0; row < 4; ++row)
                clip[col] += p[row] * camera.view_projection[row][col];
        const auto x = clip[0], y = clip[1], z = clip[2], w = clip[3];
        const std::array<bool, 6> rejected{x<-w, x> w, y<-w, y> w, z<0, z> w};
        for (unsigned i = 0; i < 6; ++i)
            outside[i] = outside[i] && rejected[i];
    }
    return std::none_of(outside.begin(), outside.end(), [](bool value) { return value; });
}

template <class LandRenderer>
void preview_camera_events(LandRenderer &land, std::span<const CameraMotionEvent> events) {
    if constexpr (requires { land.camera_events(events); })
        land.camera_events(events);
}


// Owns the existing viewer HWND through initial EARLY acquisition/adoption.
// It is a validation-local lifetime object, not a native App/UIManager issuer.
struct PreparedCameraViewer final : std::enable_shared_from_this<PreparedCameraViewer> {
    using Transaction = civ5::ui::Win32ViewportTransaction;
    // Actual stock owner and initial physical source are installed by the
    // named stock renderer. GUI is issued later, after THIS live layout.
    std::shared_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner> stock_source_owner;
    std::shared_ptr<const civ5::graphics::BnwOwnedCameraRendererWindowLease> renderer_window_owner;
    std::shared_ptr<const civ5::ui::BnwOwnedCameraGuiScreen> gui_screen;
    void retire_gui_screen()noexcept {if(gui_screen)gui_screen->retire();gui_screen.reset();}
    std::shared_ptr<GlContext> context;
    HWND window = nullptr;
    DWORD thread = GetCurrentThreadId();
    int width = 0, height = 0, client_width = 0, client_height = 0;
    const std::shared_ptr<PreviewInput> input_owner;
    PreviewInput& input;
    bool existing_menu_binding=false;
    // Native-only pointer capture; no provider or GL dispatch is permitted.
    std::function<void(const civ5::ui::ClientWindowEvent&)> native_pointer_capture;
    RECT client_before_layout{},client_after_layout{};
    std::uint32_t dpi_before_layout=0,dpi_after_layout=0;
    std::weak_ptr<const std::uint8_t> last_run_paint_lifetime;
    std::optional<Transaction::Ticket> initial_ticket;
    std::uint64_t viewport_version = 0;
    std::optional<Transaction::RetirementReason> last_retirement_reason;
    bool viewport_retired_pending = false, native_window_close_retired = false;
    std::shared_ptr<const civ5::ui::BnwWorldViewWindowInputLoan> native_close_occurrence;
    bool initial_epoch_retired = false, alive = true, run_active = false;
    HGLRC initial_wgl = nullptr;
    HDC initial_dc = nullptr;
    std::function<void()> retire_metadata;
    // Installed only by the live native run; never by fixture/diagnostic input.
    std::function<void(const civ5::ui::ClientWindowEvent&)> world_plot_input;
    std::function<void(const civ5::ui::ClientWindowEvent&)> audio_window_input;
    std::shared_ptr<civ5::ui::CivisClientAudioQueueOwner> client_audio;
    std::string client_audio_error;
    void retire_client_audio()noexcept {
        audio_window_input={};
        if(client_audio){try{
            const auto stopped=client_audio->close();
            if(!stopped.backend.device_released)client_audio_error="Civis actual audio device release failed: "+stopped.backend.device.last_error;
        }catch(const std::exception& error){client_audio_error=error.what();}client_audio.reset();}
    }
    // Members are detached explicitly while input/context still live.
    std::unique_ptr<Transaction> viewport_transaction;
    std::unique_ptr<PreviewWindowBinding> diagnostic_binding;

    PreparedCameraViewer(const PreparedCameraViewer&) = delete;
    PreparedCameraViewer& operator=(const PreparedCameraViewer&) = delete;
    ~PreparedCameraViewer() {
        retire_client_audio(); //Stop/join real PCM before actual HWND/binding detaches.
        alive = false;
        initial_epoch_retired = true;
        retire_metadata = {};
        viewport_transaction.reset();
        diagnostic_binding.reset();
    }
    static std::shared_ptr<PreparedCameraViewer> create(std::shared_ptr<GlContext> context,
            const RetailPreviewCamera* retail_camera, bool live_native) {
        check(context != nullptr, "Viewer preparation needs the SAME actual GL context");
        auto owner = std::shared_ptr<PreparedCameraViewer>(new PreparedCameraViewer);
        owner->context = std::move(context);
        owner->window = static_cast<HWND>(owner->context->native_window_handle());
        owner->width = retail_camera ? retail_camera->width : 1280;
        owner->height = retail_camera ? retail_camera->height : 960;
        owner->client_height = retail_camera ? owner->height :
            std::min(owner->height, GetSystemMetrics(SM_CYSCREEN) - 100);
        owner->client_width = retail_camera ? owner->width : owner->client_height * 4 / 3;
        check(owner->client_width > 0 && owner->client_height > 0 &&
              IsWindow(owner->window) && GetWindowThreadProcessId(owner->window, nullptr)==owner->thread,
              "Viewer preparation requires the available SAME owned HWND/thread");
        check(GetClientRect(owner->window,&owner->client_before_layout)!=FALSE,
              "Actual viewer pre-layout client measurement unavailable");
        owner->dpi_before_layout=preview_window_dpi(owner->window);
        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT rectangle{0, 0, owner->client_width, owner->client_height};
        check(AdjustWindowRect(&rectangle, style, FALSE) != FALSE, "Preview client dimensions");
        SetWindowLongPtrW(owner->window, GWL_STYLE, style);
        check(SetWindowPos(owner->window, nullptr,
                  (GetSystemMetrics(SM_CXSCREEN) - owner->client_width) / 2, 25,
                  rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,
                  SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED) != FALSE,
              "Preview window layout");
        check(GetClientRect(owner->window,&owner->client_after_layout)!=FALSE,
              "Actual viewer post-layout client measurement unavailable");
        owner->dpi_after_layout=preview_window_dpi(owner->window);
        owner->input.router.set_viewport({owner->client_width, owner->client_height,
            owner->client_width, owner->client_height, preview_window_dpi(owner->window)});
        owner->initial_wgl = wglGetCurrentContext();
        owner->initial_dc = wglGetCurrentDC();
        if (live_native) {
            // The sole existing transaction owns both input binding and version.
            // Its construction retirement occurs before initial_ticket is pinned.
            owner->viewport_transaction = std::make_unique<Transaction>(owner->window,
                owner->input.router,
                [raw=owner.get()](const Transaction::Retirement& retired) {
                    raw->viewport_version = retired.version;
                    raw->last_retirement_reason = retired.reason;
                    raw->viewport_retired_pending = true;
                    if (raw->initial_ticket) raw->initial_epoch_retired = true;
                    if (retired.reason==Transaction::RetirementReason::close)
                        raw->native_window_close_retired = true;
                    raw->input.cancel = true;
                    raw->input.pointer_valid = false;
                    raw->retire_client_audio(); //Actual source/window retirement cannot leave OS PCM active.
                    // Copy callable before invocation. Runtime attachment is
                    // cleared before its borrowed camera locals are destroyed.
                    const auto metadata = raw->retire_metadata;
                    if (metadata) metadata();
                }, [raw=owner.get()](const civ5::ui::ClientWindowEvent& event) {
                    if(event.kind==civ5::ui::ClientWindowEventKind::close && raw->stock_source_owner) {
                        check(event.worldview_input && !raw->native_close_occurrence,
                              "WorldView close requires one genuine native occurrence");
                        // The genuine viewport/native source validates the loan;
                        // Camera retains the occurrence without issuing or claiming it.
                        raw->native_close_occurrence=event.worldview_input;
                    }
                    camera_preview_event(raw->window, raw->input, event);
                    const auto audio=raw->audio_window_input;
                    if(audio)audio(event);
                    const auto pick=raw->world_plot_input;
                    if(pick)pick(event);
                });
            owner->initial_ticket = owner->viewport_transaction->request();
            check(owner->initial_ticket && owner->initial_ticket->viewport().client_width==owner->client_width &&
                  owner->initial_ticket->viewport().client_height==owner->client_height,
                  "Initial native viewport differs from established camera extent");
            owner->viewport_retired_pending = false; // No earlier publication.
            owner->require_initial_ticket();
        } else owner->diagnostic_binding = std::make_unique<PreviewWindowBinding>(owner->window, owner->input);
        return owner;
    }
    bool initial_epoch_current() const noexcept {
        // Do not call request/current from inside the transaction commit gate:
        // request intentionally refuses while committing. Revision is updated
        // exclusively by the SAME real transaction's retirement callback.
        if (!alive || GetCurrentThreadId()!=thread || initial_epoch_retired ||
            viewport_retired_pending || !initial_ticket || !viewport_transaction ||
            viewport_transaction->failed() || viewport_version!=initial_ticket->version() ||
            initial_ticket->window()!=window || !IsWindow(window) || IsIconic(window) ||
            GetWindowThreadProcessId(window,nullptr)!=thread ||
            GetPropW(window,L"Civis.WindowResizeTransaction")!=viewport_transaction.get() ||
            wglGetCurrentContext()!=initial_wgl || wglGetCurrentDC()!=initial_dc ||
            !initial_wgl || !initial_dc || WindowFromDC(initial_dc)!=window) return false;
        RECT actual{};
        return GetClientRect(window,&actual) && actual.left==0 && actual.top==0 &&
            actual.right==client_width && actual.bottom==client_height &&
            initial_ticket->viewport()==civ5::ui::ClientViewport{client_width,client_height,
                client_width,client_height,preview_window_dpi(window)};
    }
    void require_initial_ticket() {
        check(initial_epoch_current(), "EARLY viewer initial epoch retired/unavailable; refresh is unsupported");
        const auto now = viewport_transaction->request();
        check(now && now->window()==initial_ticket->window() &&
              now->version()==initial_ticket->version() && now->viewport()==initial_ticket->viewport() &&
              initial_epoch_current(), "EARLY viewer ticket changed across actual native request");
    }
    civ5::runtime::BnwStockLuaClientExtentProvider early_client_provider() {
        require_initial_ticket();
        const std::weak_ptr<PreparedCameraViewer> weak=shared_from_this();
        const auto native=civ5::runtime::bnw_stock_lua_win32_client_extent(
            reinterpret_cast<std::uintptr_t>(window),
            "Civis.CameraPreview.HWND."+std::to_string(reinterpret_cast<std::uintptr_t>(window))+
                ".transaction."+std::to_string(reinterpret_cast<std::uintptr_t>(viewport_transaction.get())),
            [weak,creator_thread=thread] {
                if(GetCurrentThreadId()!=creator_thread) return std::uint64_t{0};
                const auto owner=weak.lock();
                return owner && owner->initial_epoch_current() ? owner->viewport_version : std::uint64_t{0};
            });
        return {[weak,native,creator_thread=thread]() -> std::optional<civ5::runtime::BnwStockLuaClientExtent> {
            // Refuse BEFORE strong retention on a foreign thread; last scope
            // destruction and HWND binding teardown remain on the creator.
            if(GetCurrentThreadId()!=creator_thread) return std::nullopt;
            const auto owner=weak.lock(); // Retain before every native callback/query.
            if (!owner || !owner->initial_epoch_current()) return std::nullopt;
            const auto observed=native.current();
            if (!observed || observed->revision!=owner->initial_ticket->version() ||
                !owner->initial_epoch_current()) return std::nullopt;
            return observed;
        }};
    }

    void write_window_state(fs::path path,std::string phase) {
        check(alive && GetCurrentThreadId()==thread,"Viewer state evidence requires live creator thread");
        // Actual request may retire a missed native resize. Report that result,
        // never restore an acquisition epoch merely for a diagnostic snapshot.
        const auto observed=viewport_transaction ? viewport_transaction->request() :
            std::optional<Transaction::Ticket>{};
        RECT actual{};
        const bool rectangle_available=GetClientRect(window,&actual)!=FALSE;
        DWORD pid=0;
        const DWORD tid=GetWindowThreadProcessId(window,&pid);
        const bool initial_current=initial_epoch_current();
        std::ostringstream receipt;
        receipt << "{\"schema\":\"civis-initial-viewer-window-state-v1\",\"phase\":\""<<phase
            <<"\",\"hwnd\":\""<<reinterpret_cast<std::uintptr_t>(window)
            <<"\",\"currentProcess\":"<<GetCurrentProcessId()<<",\"creatorThread\":"<<thread
            <<",\"actualWindowProcess\":"<<pid<<",\"actualWindowThread\":"<<tid
            <<",\"measuredBeforeLayout\":["<<client_before_layout.left<<','<<client_before_layout.top
            <<','<<client_before_layout.right<<','<<client_before_layout.bottom
            <<"],\"measuredAfterLayout\":["<<client_after_layout.left<<','<<client_after_layout.top
            <<','<<client_after_layout.right<<','<<client_after_layout.bottom
            <<"],\"dpiBeforeLayout\":"<<dpi_before_layout<<",\"dpiAfterLayout\":"<<dpi_after_layout
            <<",\"actualDpiNow\":"<<preview_window_dpi(window)
            <<",\"currentRectangleAvailable\":"<<(rectangle_available?"true":"false")
            <<",\"currentRectangle\":["<<actual.left<<','<<actual.top<<','<<actual.right<<','<<actual.bottom
            <<"],\"initialTicketRevision\":"<<(initial_ticket?initial_ticket->version():0)
            <<",\"retirementRevision\":"<<viewport_version<<",\"currentTicketRevision\":";
        if(observed) receipt<<observed->version(); else receipt<<"null";
        receipt<<",\"initialEpochCurrent\":"<<(initial_current?"true":"false")
            <<",\"initialEpochRetired\":"<<(initial_epoch_retired?"true":"false")
            <<",\"hwndAvailable\":"<<(IsWindow(window)?"true":"false")
            <<",\"minimized\":"<<(IsIconic(window)?"true":"false")
            <<",\"borrowedPaintCallbacksAlive\":"<<(!last_run_paint_lifetime.expired()?"true":"false")
            <<",\"nativeAppAuthority\":false,\"nativePixelParity\":false,\"full63\":false}\n";
        blend_validation::text(path,receipt.str());
    }
  private:
    friend class civ5::ui::detail::CivisFrontendPartyRoutingOwner;
    friend class civ5::ui::detail::CivisFrontendPartyRendererBackend;
    template<class T>friend struct civ5::validation::CameraPreviewLoop;
    explicit PreparedCameraViewer(std::shared_ptr<PreviewInput> actual=std::make_shared<PreviewInput>())
        :input_owner(std::move(actual)),input(*input_owner){}
    class PartyLoopReturn final {
        template<class T>friend struct civ5::validation::CameraPreviewLoop;
        friend class civ5::ui::detail::CivisFrontendPartyRendererBackend;
        const DWORD creator_;
        const HWND window_;
        const std::weak_ptr<PreviewInput> input_;
        const std::weak_ptr<GlContext> context_;
        const std::uint64_t presents_;
        const std::shared_ptr<const civ5::ui::BnwWorldViewWindowInputLoan> close_;
        PartyLoopReturn(const PreparedCameraViewer& owner,std::uint64_t n)
            :creator_(owner.thread),window_(owner.window),input_(owner.input_owner),context_(owner.context),presents_(n),close_(owner.native_close_occurrence){
            check(close_!=nullptr,"Party Return has no actual returned native shutdown occurrence");
            check(n>0,"Party Return has no actual source-checked presentation");
        }
      public:
        void require_for_owned(const PreparedCameraViewer& owner)const{
            check(GetCurrentThreadId()==creator_&&owner.alive&&owner.thread==creator_&&owner.window==window_&&presents_>0,
                  "Party loop Return belongs to another or retired target");
            const auto input=input_.lock();const auto context=context_.lock();
            check(input&&context&&input.get()==owner.input_owner.get()&&context.get()==owner.context.get()&&
                  !input.owner_before(owner.input_owner)&&!owner.input_owner.owner_before(input)&&
                  !context.owner_before(owner.context)&&!owner.context.owner_before(context),
                  "Party loop Return target controls changed");
            check(owner.native_close_occurrence&&owner.native_close_occurrence.get()==close_.get()&&
                  !owner.native_close_occurrence.owner_before(close_)&&!close_.owner_before(owner.native_close_occurrence),
                  "Party loop Return native close control changed");
        }
    };
    std::unique_ptr<const PartyLoopReturn> party_loop_return_;
    std::weak_ptr<civ5::ui::detail::CivisFrontendPartyRoutingOwner> party_routing_;
    void (*drain_existing_)(civ5::ui::Win32ViewportTransaction&)=nullptr;
};

struct PreparedCameraRunAttachment final {
    std::shared_ptr<PreparedCameraViewer> viewer;
    explicit PreparedCameraRunAttachment(std::shared_ptr<PreparedCameraViewer> owner,
            std::function<void()> retire) : viewer(std::move(owner)) {
        check(viewer && viewer->thread==GetCurrentThreadId() && !viewer->run_active,
              "Viewer camera run reentry/foreign thread");
        viewer->retire_metadata=std::move(retire);
        viewer->run_active=true;
    }
    ~PreparedCameraRunAttachment() {
        viewer->retire_gui_screen();
        viewer->stock_source_owner.reset();
        viewer->renderer_window_owner.reset();
        viewer->world_plot_input={};
        viewer->retire_metadata={};
        viewer->run_active=false;
    }
    PreparedCameraRunAttachment(const PreparedCameraRunAttachment&)=delete;
    PreparedCameraRunAttachment& operator=(const PreparedCameraRunAttachment&)=delete;
};

// Declare this after every object borrowed by paint/current callbacks. Its
// destructor retires closures before those camera dependencies unwind, including
// exceptions. The HWND/transaction itself remains owned by the caller.
struct PreparedCameraPublicationGuard final {
    std::shared_ptr<PreparedCameraViewer> viewer;
    std::shared_ptr<const std::uint8_t>& token;
    void retire_borrowed_callbacks() noexcept {
        viewer->retire_gui_screen();
        token.reset();
        viewer->world_plot_input={};
        viewer->native_pointer_capture={};
        viewer->retire_metadata={};
    }
    ~PreparedCameraPublicationGuard() { retire_borrowed_callbacks(); }
    PreparedCameraPublicationGuard(std::shared_ptr<PreparedCameraViewer> owner,
            std::shared_ptr<const std::uint8_t>& lifetime) : viewer(std::move(owner)), token(lifetime) {}
    PreparedCameraPublicationGuard(const PreparedCameraPublicationGuard&)=delete;
    PreparedCameraPublicationGuard& operator=(const PreparedCameraPublicationGuard&)=delete;
};


// Short negative gate on the SAME actual viewer after accepted initial launch.
// No frame/publication is fabricated by this exception fixture. The late input
// queries the expired gate installed by the actual initial composed GL paint.
void verify_prepared_viewer_lifetime(std::shared_ptr<PreparedCameraViewer> viewer,
        std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner> package,fs::path output) {
    check(viewer && package && viewer->initial_epoch_current() &&
          !viewer->run_active && viewer->last_run_paint_lifetime.expired(),
          "Lifetime gate needs accepted initial launch with camera locals released");
    viewer->require_initial_ticket();
    const auto lease=package->early_input_lease();
    lease->require_current();
    bool borrowed_live=true;
    unsigned live_callbacks=0,dead_callbacks=0;
    std::weak_ptr<const std::uint8_t> exception_token;
    struct BorrowedSentinel {
        bool& alive;
        ~BorrowedSentinel(){alive=false;}
    };
    struct DiagnosticUnwind {};
    try {
        BorrowedSentinel borrowed{borrowed_live};
        PreparedCameraRunAttachment attachment(viewer,[&] {
            // The counters remain alive for the whole gate. Never dereference
            // a dead fixture object merely to discover a lifetime regression.
            if(borrowed_live) ++live_callbacks; else ++dead_callbacks;
        });
        auto token=std::make_shared<const std::uint8_t>(0);
        exception_token=token;
        PreparedCameraPublicationGuard guard(viewer,token);
        throw DiagnosticUnwind{};
    } catch(const DiagnosticUnwind&) {}
    check(!borrowed_live && exception_token.expired() && !viewer->retire_metadata && !viewer->run_active,
          "Exception cleanup retained borrowed paint/metadata callbacks");
    const auto version_before=viewer->viewport_version;
    const auto pointer_messages_before=viewer->input.pointer_messages;
    const auto queued_events_before=viewer->input.events.size();
    const auto queued_ui_wheels_before=viewer->input.live_ui_wheels.size();
    const auto queued_search_before=viewer->input.search_characters.size();
    const auto wheel_messages_before=viewer->input.wheel_messages;
    const auto window=viewer->window;
    // The real transaction's retained gate has a weak expired camera token.
    // An actual native input must reject before any dead source/sink dispatch.
    SendMessageW(window,WM_MOUSEMOVE,0,MAKELPARAM(1,1));
    check(IsWindow(window) && viewer->initial_epoch_retired && viewer->viewport_retired_pending &&
          viewer->viewport_version>version_before &&
          viewer->last_retirement_reason==PreparedCameraViewer::Transaction::RetirementReason::source_rejected &&
          !viewer->initial_epoch_current() &&
          viewer->input.pointer_messages==pointer_messages_before &&
          viewer->input.events.size()==queued_events_before &&
          viewer->input.live_ui_wheels.size()==queued_ui_wheels_before &&
          viewer->input.search_characters.size()==queued_search_before &&
          viewer->input.wheel_messages==wheel_messages_before &&
          live_callbacks==0 && dead_callbacks==0,
          "Late real input invoked dead callbacks or did not retire initial lease");
    bool lease_refused=false,package_refused=false;
    try{lease->require_current();}catch(const std::exception&){lease_refused=true;}
    try{(void)package->current();}catch(const std::exception&){package_refused=true;}
    check(lease_refused && package_refused,"Old initial lease/package survived actual late input retirement");
    viewer->write_window_state(output/"early-viewer-after-lifetime-gates.json","after-diagnostic-retirement-before-teardown");
    std::ostringstream report;
    report<<"{\"schema\":\"civis-real-viewer-lifetime-gates-v1\",\"exceptionCleanupPassed\":true,"
        "\"exceptionFixtureOnly\":true,\"actualRendererExceptionAcceptance\":false,"
        "\"actualExpiredPaintGateUsed\":true,\"actualNativeMouseMoveSubmitted\":true,"
        "\"versionBefore\":"<<version_before<<",\"versionAfter\":"<<viewer->viewport_version
        <<",\"actualRetirementReason\":\"source_rejected\",\"lateMetadataCallbacks\":"<<dead_callbacks
        <<",\"queuedCameraEventsBefore\":"<<queued_events_before
        <<",\"queuedCameraEventsAfter\":"<<viewer->input.events.size()
        <<",\"queuedUiWheelEventsBefore\":"<<queued_ui_wheels_before
        <<",\"queuedUiWheelEventsAfter\":"<<viewer->input.live_ui_wheels.size()
        <<",\"queuedSearchCharactersBefore\":"<<queued_search_before
        <<",\"queuedSearchCharactersAfter\":"<<viewer->input.search_characters.size()
        <<",\"latePointerSinkDispatches\":"<<(viewer->input.pointer_messages-pointer_messages_before)
        <<",\"oldLeaseRefused\":true,\"oldPackageRefused\":true,"
        "\"fixturePublicationInstalled\":false,\"secondWindow\":false,\"epochRefresh\":false,"
        "\"nativeLifetimeParity\":false,\"full63\":false}\n";
    blend_validation::text(output/"early-viewer-lifetime-gates.json",report.str());
}

} // anonymous helper namespace; actual loop has a named closed issuer.
namespace civ5::validation {
template <class LandRenderer>struct CameraPreviewLoop final {
static void run(std::shared_ptr<GlContext> context, std::shared_ptr<const MaterialWorld> world,
                        LandRenderer &land, Control &controls, GlLandMaterialInputs land_scene,
                        const fs::path &root, const fs::path &output, bool smoke,
                        civ5::ui::TopPanelRuntime::StateProvider top_panel_state_provider,
                        bool original_lighting = false, const GlDistanceFog *distance_fog = nullptr,
                        bool original_fow = false, const GlColorKeys *color_keys = nullptr,
                        bool original_rivers = false,
                        civ5::ui::TopPanelActionSink top_panel_action_sink = {},
                        const civ5::assets::ResourceMountTable *ui_resources_override = nullptr,
                        const civ5::saves::SavedMapTeamArtObservation *saved_art = nullptr,
                        const civ5::saves::SavedMapProjection *saved_minimap = nullptr,
                        bool smoke_fixture_tooltips_expected = true,
                        std::string_view ui_language = "en_US",
                        std::optional<std::int32_t> saved_turn_zero_year = {},
                        const fs::path &native_ice_records_path = {},
                        bool focus_revealed_ice = false,
                        const fs::path &native_minimap_edge_records_path = {},
                        std::string_view saved_save_sha256 = {},
                        const fs::path &native_minimap_city_marker_fixture_path = {},
                        civ5::ui::LiveUiHost::FrameProvider live_ui_provider = {},
                        civ5::ui::UiGameplayBackend *live_ui_backend = nullptr,
                        const civ5::ui::LiveBnwPreviewInput *live_bnw_preview = nullptr,
                        const RetailPreviewCamera *retail_camera = nullptr,
                        const civ5::ui::MinimapNativeBoundsFixture *native_minimap_bounds = nullptr,
                        bool captured_g0_selected_marker_visible = false,
                        const civ5::graphics::BnwRendererSceneReceipt *live_scene_receipt = nullptr,
                        civ5::graphics::BnwClientCaptureHook live_client_capture = {},
                        std::shared_ptr<civ5::graphics::BnwUiSourceObservation> ui_source_observation = {},
                        civ5::graphics::GlBnwWarriorOwnedScene *owned_warrior = nullptr,
                        std::shared_ptr<PreparedCameraViewer> prepared_viewer = {},
                        bool early_initial_smoke = false,
                        std::shared_ptr<const civ5::ui::SelectedBnwPreviewHost> font_source_parent = {},
                        std::optional<civ5::assets::ResourceMountTable>* actual_main_ui_owner = nullptr,
                        bool worldview_font_source_gates = false,
                        bool production_component_validation = false
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                        ,const civ5::ui::SelectedBnwReadOnlyPreview* restart_preview = nullptr,
                        const fs::path& restart_actual_input = {},
                        const fs::path& restart_actual_game_root = {}
#endif
                        ,std::shared_ptr<civ5::ui::BnwWorldViewFontSourceScope> actual_own_font_source = {},
                        civ5::ui::detail::CivisFrontendAlphaLandFrame* actual_alpha_frame = nullptr,
                        const civ5::ui::detail::CivisFrontendPartyMaterialStorage* actual_material = nullptr
                        ) {
    check(bool(actual_alpha_frame)==bool(actual_material),"Actual alpha Camera source pair absent");
    if(actual_alpha_frame){
        check(prepared_viewer&&prepared_viewer->existing_menu_binding&&live_bnw_preview&&actual_own_font_source,
            "Own saved Camera requires actual prepared Party/source/font owners");
        actual_alpha_frame->require_camera_boundary_owned();
        check(actual_material->world_owned().get()==world.get(),"Actual Camera material generation differs");
    }
    camera_stage_mark("run_camera_preview","BEGIN");
    // Capture the actual unique renderer lifetime before any source callbacks.
    const auto center_imported_lifetime=PreviewPresenter::observe_center_imported_lifetime(owned_warrior);
    if(production_component_validation){
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
        check(restart_preview&&live_bnw_preview&&prepared_viewer&&prepared_viewer->stock_source_owner,
              "Production component validation requires the actual observed selected preview/stock owner");
#else
        throw std::runtime_error("Production component validation requires the existing observed-preview fixture supplier");
#endif
    }
    struct CameraRunStageExit {
        ~CameraRunStageExit() noexcept {camera_stage_mark("run_camera_preview","END");}
    } camera_run_stage_exit; // Scope exit, including unwind; never a success flag.
    check(!font_source_parent || (live_bnw_preview &&
          &font_source_parent->session()==&live_bnw_preview->session),
          "WorldView font source needs the genuine SAME selected host/session");
    check(!worldview_font_source_gates || (font_source_parent && early_initial_smoke),
          "WorldView font source gates require genuine owned-host EARLY initial smoke");
    // Own-C++ source lifetime, never native App/Context/Label authority.
    std::shared_ptr<civ5::ui::BnwWorldViewFontSourceScope> worldview_font_source;
    struct RetireWorldViewFontSource {
        std::shared_ptr<civ5::ui::BnwWorldViewFontSourceScope>& source;
        ~RetireWorldViewFontSource() { if(source) source->retire(); }
    };
    // Setup fallback: no published paint callbacks exist before the late guard.
    RetireWorldViewFontSource setup_font_retirement{worldview_font_source};
    check(!owned_warrior || (live_bnw_preview && live_scene_receipt),
          "Owned Warrior draw needs the actual live session and reconciled scene");
    check(!focus_revealed_ice || (saved_art && saved_minimap),
          "Ice focus requires a saved map and fog state");
    check(!native_minimap_bounds || (saved_minimap && saved_art),
          "Native minimap bounds require the selected saved map");
    std::optional<civ5::ui::LiveBnwPreviewBinding> live_bnw_binding;
    std::shared_ptr<civ5::ui::BnwClientSceneFrame> live_client_scene;
    std::shared_ptr<civ5::graphics::BnwPendingUnitVisualOwner> live_pending_unit_visuals;
    std::unique_ptr<civ5::graphics::BnwCameraFogClient> live_fog_client;
    float live_material_time = 0;
    std::function<bool()> live_camera_current;
    std::function<bool()> live_world_frame_current;
    std::function<bool()> live_minimap_current;
    std::unique_ptr<civ5::ui::BnwMinimapFogPreparationOwner> live_minimap_preparation;
    if (live_bnw_preview) {
        check(live_scene_receipt != nullptr,
              "Live BNW rendering requires an applied renderer scene receipt");
        check(!live_ui_provider && !live_ui_backend,
              "Live BNW preview cannot also use a saved-only UI provider");
    }
    const bool parent_prepared = static_cast<bool>(prepared_viewer);
    check(!early_initial_smoke || (parent_prepared && live_bnw_preview && smoke),
          "EARLY initial smoke requires genuine parent-prepared live BNW rendering");
    if (parent_prepared) {
        check(prepared_viewer->context==context && live_bnw_preview &&
              (actual_alpha_frame || (prepared_viewer->width==(retail_camera ? retail_camera->width : 1280) &&
              prepared_viewer->height==(retail_camera ? retail_camera->height : 960))),
              "Camera needs the SAME prepared context and authored extent");
        prepared_viewer->require_initial_ticket();
    } else prepared_viewer=PreparedCameraViewer::create(context,retail_camera,live_bnw_preview!=nullptr);
    const int width=prepared_viewer->width, height=prepared_viewer->height;
    const HWND window=prepared_viewer->window;
    const int client_width=prepared_viewer->client_width, client_height=prepared_viewer->client_height;
    PreviewInput& input=prepared_viewer->input;
    using ViewportTransaction=civ5::ui::Win32ViewportTransaction;
    std::optional<ViewportTransaction::Ticket> drawing_viewport=prepared_viewer->initial_ticket,published_viewport;
    auto& viewport_version=prepared_viewer->viewport_version;
    auto& viewport_retired_pending=prepared_viewer->viewport_retired_pending;
    auto& native_window_close_retired=prepared_viewer->native_window_close_retired;
    std::vector<civ5::ui::ClientRect> candidate_hits;
    ViewportTransaction::SourceTargetsCurrent published_viewport_gate;
    auto& viewport_transaction=prepared_viewer->viewport_transaction;
    auto& diagnostic_binding=prepared_viewer->diagnostic_binding;
    if(prepared_viewer->stock_source_owner) {
        check(viewport_transaction!=nullptr,
              "Selected WorldView input requires the genuine viewport owner");
        // Bind the actual canonical Source once. Transaction captures native
        // input before its internal close admission, then drops its Source pin
        // before invoking the borrowed outer camera sink.
        viewport_transaction->bind_worldview_input_source_owned(
            std::weak_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner>(
                prepared_viewer->stock_source_owner));
    }
    if(const auto routing=prepared_viewer->party_routing_.lock()){
        check(prepared_viewer->stock_source_owner!=nullptr,"Party input has no actual bound Source");
        routing->begin_camera_input_owned(*prepared_viewer->stock_source_owner);
    }
    PreparedCameraRunAttachment run_attachment(prepared_viewer,[&] {
        // Retire source authorization BEFORE scene/borrowed metadata invalidation.
        if(worldview_font_source) worldview_font_source->retire();
        // Only metadata while source/message stacks still borrow scene objects.
        if(live_client_scene) live_client_scene->invalidate();
    });
    // A retained transaction gate must never call dead camera stack locals.
    // This expires on every return/exception before parent final source checks.
    auto paint_run_lifetime=std::make_shared<const std::uint8_t>(0);
    prepared_viewer->last_run_paint_lifetime=paint_run_lifetime;
    const auto owned_wgl_context=wglGetCurrentContext();
    const auto owned_wgl_dc=wglGetCurrentDC();
    const auto same_extent_candidate_current=[&] {
        if(!viewport_transaction) return true; // Existing saved diagnostic path.
        if(parent_prepared && !prepared_viewer->initial_epoch_current()) return false;
        if(viewport_retired_pending || !drawing_viewport ||
            drawing_viewport->version()!=viewport_version ||
            drawing_viewport->window()!=window || viewport_transaction->failed() ||
            !IsWindow(window) || IsIconic(window)) return false;
        RECT actual{};
        return GetClientRect(window,&actual) && actual.left==0 && actual.top==0 &&
            actual.right==client_width && actual.bottom==client_height &&
            drawing_viewport->viewport()==civ5::ui::ClientViewport{client_width,client_height,
                client_width,client_height,preview_window_dpi(window)} &&
            owned_wgl_context && owned_wgl_dc && wglGetCurrentContext()==owned_wgl_context &&
            wglGetCurrentDC()==owned_wgl_dc && WindowFromDC(owned_wgl_dc)==window;
    };
    std::optional<civ5::assets::ResourceMountTable> owned_ui_resources;
    const civ5::assets::ResourceMountTable *ui_resources = ui_resources_override;
    if(actual_own_font_source) {
        check(font_source_parent && font_source_parent->uses_civis_resource_bank(),
              "Own font scope requires the actual Civis bank host");
        const auto canonical_font_source=actual_own_font_source->retain_owner();
        check(canonical_font_source && canonical_font_source.get()==actual_own_font_source.get() &&
              !canonical_font_source.owner_before(actual_own_font_source) &&
              !actual_own_font_source.owner_before(canonical_font_source),
              "Own font scope differs from its canonical source owner");
        canonical_font_source->require_same_civis_resource_host(*font_source_parent);
        canonical_font_source->require_current();
        check(canonical_font_source->locale().language==ui_language && !actual_main_ui_owner,
              "Own font scope locale or table ownership differs");
        const auto& own_resources=canonical_font_source->resources();
        check(!ui_resources_override || ui_resources_override==&own_resources,
              "Own font source requires its exact retained resource table");
        worldview_font_source=canonical_font_source;
        ui_resources=&own_resources;
    }
    if (!ui_resources) {
        const civ5::assets::AssetEnvironment ui_environment(root);
        const auto ui_source = archive(ui_environment, "Resource/DX9/UITextures.fpk");
        // Keep authored UI XML on the same mount table that owns the preview's
        // loose base files and UI archive. A game-screen owner can inject its
        // save-derived table through ui_resources_override without changing
        // the presenter setup or falling back to a raw filesystem path.
        const auto loose_ui_source = ui_environment.loose_source();
        owned_ui_resources.emplace(civ5::assets::resource_lookup(loose_ui_source),
                                   civ5::assets::resource_lookup_all(loose_ui_source),
                                   civ5::assets::resource_keys(loose_ui_source));
        owned_ui_resources->mount({1,
                                   1,
                                   0,
                                   civ5::assets::resource_lookup(ui_source),
                                   true,
                                   true,
                                   civ5::assets::resource_lookup_all(ui_source),
                                   civ5::assets::resource_keys(ui_source)});
        ui_resources = &*owned_ui_resources;
    }
    if(font_source_parent && !worldview_font_source) {
        // The override is borrowed unless this exact owning slot accompanies it.
        // Move the actual table, never copy EARLY resources or create shared aliases.
        if(ui_resources_override) {
            check(actual_main_ui_owner && actual_main_ui_owner->has_value() &&
                  &actual_main_ui_owner->value()==ui_resources_override,
                  "WorldView font source override lacks its exact owning handoff");
            worldview_font_source=civ5::ui::BnwWorldViewFontSourceScope::create(
                std::move(actual_main_ui_owner->value()),std::string(ui_language));
            actual_main_ui_owner->reset();
        } else {
            check(!actual_main_ui_owner && owned_ui_resources && ui_resources==&*owned_ui_resources,
                  "WorldView local font source ownership differs");
            worldview_font_source=civ5::ui::BnwWorldViewFontSourceScope::create(
                std::move(*owned_ui_resources),std::string(ui_language));
            owned_ui_resources.reset();
        }
        ui_resources=&worldview_font_source->resources();
    } else check(!actual_main_ui_owner,"Borrowed legacy camera cannot adopt a font source table");
    std::unique_ptr<civ5::assets::ScopedResourceResolveObserver> ui_source_observer;
    if (ui_source_observation)
        ui_source_observer = std::make_unique<civ5::assets::ScopedResourceResolveObserver>(
            *ui_resources, [observation = ui_source_observation](const auto &key, auto resources, bool all) {
                observation->observe(key,resources,all);
            });
    auto top_panel_layout = civ5::ui::read_top_panel(*ui_resources);
    const auto top_panel_source = ui_resources->resolve(civ5::assets::AssetKey("TopPanel.xml"));
    const auto *top_panel_source_handle = std::get_if<civ5::assets::AssetHandle>(&top_panel_source.asset);
    check(top_panel_source_handle != nullptr, "Active TopPanel XML source is missing or ambiguous");
    const bool expansion2_ui = top_panel_source_handle->candidate().original_name.starts_with(
        "Assets/DLC/Expansion2/UI/InGame/");
    blend_validation::text(output / "top-panel-source.txt",
                           top_panel_source_handle->candidate().original_name + "\n");
    const auto ui_find = [&](const civ5::assets::AssetKey &key) {
        return ui_resources->resolve(key).asset;
    };
    const auto top_panel_binding = civ5::ui::bind_top_panel_bar(top_panel_layout, *ui_resources);
    const auto top_panel_lookup = ui_find(civ5::assets::AssetKey(top_panel_binding.entry_name));
    check(std::holds_alternative<civ5::assets::AssetHandle>(top_panel_lookup),
          "TopPanelBar lookup changed after binding");
    const auto top_panel_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(top_panel_lookup).read_bytes());
    std::optional<PreviewMinimapChromeSource> minimap_source;
    std::optional<civ5::ui::MinimapVisibleBounds> saved_minimap_visible;
    // Historical saved diagnostic image is post-fog; live fog never reuses it.
    std::vector<std::uint8_t> saved_minimap_base_bgra;
    std::unique_ptr<GlMinimapSurfaceDiagnostic> live_minimap_surface;
    std::optional<civ5::ui::MinimapTileGrid> live_minimap_initial_grid;
    std::optional<civ5::ui::BnwMinimapFogGeometry> live_minimap_geometry;
    if (expansion2_ui) {
        auto layout = civ5::ui::read_minimap_panel(*ui_resources);
        check(layout.source_key == "Assets/DLC/Expansion2/UI/InGame/WorldView/MiniMapPanel.xml",
              "Active BNW MiniMapPanel XML source differs from the selected UI skin");
        auto style = civ5::ui::read_top_panel_grid9_style(*ui_resources, layout.frame_style);
        const auto authored_dds = [&](std::string_view name) {
            const auto selected = ui_find(civ5::assets::AssetKey(
                civ5::assets::resource_basename(std::string(name))));
            const auto *handle = std::get_if<civ5::assets::AssetHandle>(&selected);
            check(handle && handle->candidate().provenance.archive_relative_path ==
                                "Resource/DX9/Expansion2UITextures.fpk",
                  ("BNW minimap art did not come from the selected expansion archive; requested=" +
                   std::string(name) + "; variant=" +
                   (handle ? "selected" : std::holds_alternative<civ5::assets::AmbiguousAsset>(selected)
                                             ? "ambiguous" : "missing") +
                   "; original=" + (handle ? handle->candidate().original_name : "<none>") +
                   "; source_kind=" + (handle ? handle->candidate().provenance.kind ==
                                                         civ5::assets::SourceKind::fpk ? "fpk" : "loose"
                                               : "<none>") +
                   "; archive=" + (handle ? handle->candidate().provenance.archive_relative_path.value_or("<none>")
                                           : "<none>")).c_str());
            return civ5::assets::DdsData::parse(handle->read_bytes());
        };
        const auto fog_lookup = ui_find(civ5::assets::AssetKey("MiniMapFog.dds"));
        const auto *fog_handle = std::get_if<civ5::assets::AssetHandle>(&fog_lookup);
        check(fog_handle && fog_handle->candidate().provenance.archive_relative_path ==
                                "Resource/DX9/MiscTextures.fpk",
              "Minimap parchment did not come from the installed MiscTextures archive");
        auto fog_data = civ5::assets::DdsData::parse(fog_handle->read_bytes());
        check(fog_data.width() == 256 && fog_data.height() == 256,
              "Retail minimap parchment DDS dimensions changed");
        const auto background_atlas = authored_dds("BottomRight128x224.dds");
        const auto background_index = authored_dds("BottomRight128x224-index.dds");
        auto background_image = civ5::assets::expand_ui_block_atlas(background_atlas, background_index);
        check(background_image.width == 128 && background_image.height == 224,
              "BNW minimap background expansion did not recover the authored control size");
        // The selected official Upgrade1 package replaces this corner pair.
        // Keep the real selected winner and the reader's SAME-container,
        // index-identity and byte-stability checks; do not remount or fall back.
        auto corner_binding = civ5::assets::bind_ui_texture(
            *ui_resources, "LowerRightCorner_MiniMap.dds", true);
        const auto &corner_source = corner_binding.source_identity.provenance;
        check(corner_source.kind == civ5::assets::SourceKind::fpk &&
                  (corner_source.archive_relative_path == "Resource/DX9/Expansion2UITextures.fpk" ||
                   corner_source.archive_relative_path == "Resource/DX9/Upgrade1Textures.fpk"),
              ("BNW minimap corner did not come from the selected expansion or official Upgrade1 archive; archive=" +
               corner_source.archive_relative_path.value_or("<none>")).c_str());
        check(corner_binding.expanded.has_value(),
              "BNW minimap corner requires its actual paired indexed image");
        auto corner_image = std::move(*corner_binding.expanded);
        check(corner_image.width == 101 && corner_image.height == 72,
              "BNW minimap corner expansion did not recover the authored control size");
        const auto frame_plan = civ5::ui::plan_minimap_panel(
            layout, static_cast<std::uint32_t>(client_width),
            static_cast<std::uint32_t>(client_height));
        const auto rect_json = [](civ5::ui::MinimapRect rect) {
            return "[" + std::to_string(rect.x) + "," + std::to_string(rect.y) + "," +
                   std::to_string(rect.width) + "," + std::to_string(rect.height) + "]";
        };
        std::ostringstream report;
        report << "{\"schema\":\"civ5-minimap-chrome-v1\",\"source\":\""
               << layout.source_key << "\",\"frame\":" << rect_json(frame_plan.frame)
               << ",\"map\":" << rect_json(frame_plan.map)
               << ",\"background\":" << rect_json(frame_plan.background)
               << ",\"corner\":" << rect_json(frame_plan.corner)
               << ",\"strategic\":" << rect_json(frame_plan.strategic)
               << ",\"options\":" << rect_json(frame_plan.options)
               << ",\"mapTextureReady\":false,\"partialSurfaceDisplayed\":"
               << ((saved_minimap && saved_art) ? "true" : "false")
               << ",\"fogBaseReady\":true"
               << ",\"backgroundReconstructed\":true,\"cornerReconstructed\":true}\n";
        blend_validation::text(output / "minimap-chrome.json", report.str());
        minimap_source.emplace(PreviewMinimapChromeSource{
            std::move(layout), std::move(style),
            authored_dds("Grid9DetailSix140.dds"),
            authored_dds("MainStrategicButton.dds"),
            authored_dds("NotificationNotes.dds"), std::move(fog_data),
            std::move(background_image), std::move(corner_image)});
    }
    std::vector<std::uint32_t> revealed_ice_plot_indices;
    bool focused_ice_currently_visible = false;
    if (saved_minimap && saved_art && expansion2_ui) {
        const auto &terrain = world->terrain();
        auto grid = civ5::ui::project_saved_minimap_terrain(*saved_minimap);
        if (live_bnw_preview) live_minimap_initial_grid = grid;
        const auto revealed = civ5::ui::apply_saved_minimap_reveals(grid, *saved_art);
        const auto coast_candidates = civ5::ui::build_minimap_coast_edge_candidates(
            terrain.source().layout, terrain.coast());
        const auto coast_accepted = std::count_if(
            coast_candidates.begin(), coast_candidates.end(), [&](const auto &candidate) {
                return civ5::ui::minimap_coast_edge_registration_allowed(
                    candidate, terrain.source().layout.source_grid, saved_minimap->wrap_x);
            });
        std::vector<std::uint8_t> coast_candidate_stream{'M', 'C', 'E', 'C'};
        const auto append_coast_u32 = [&](std::uint32_t value) {
            for (unsigned shift = 0; shift < 32; shift += 8)
                coast_candidate_stream.push_back(static_cast<std::uint8_t>(value >> shift));
        };
        const auto append_coast_f32 = [&](float value) {
            append_coast_u32(std::bit_cast<std::uint32_t>(value));
        };
        check(coast_candidates.size() <= std::numeric_limits<std::uint32_t>::max(),
              "Minimap coast candidate count exceeds export format");
        append_coast_u32(static_cast<std::uint32_t>(coast_candidates.size()));
        for (const auto &edge : coast_candidates) {
            check(edge.direction < 12, "Minimap coast owner yielded an unsupported direction");
            for (auto v : edge.worker_cube) append_coast_u32(static_cast<std::uint32_t>(v));
            for (auto v : edge.table_cube) append_coast_u32(static_cast<std::uint32_t>(v));
            append_coast_u32(edge.direction);
            for (const auto &p : edge.world_points) {
                append_coast_f32(p.x);
                append_coast_f32(p.y);
            }
            append_coast_u32(edge.contour_index);
            append_coast_u32(edge.pair_index);
        }
        blend_validation::write(output / "minimap-coast-edge-candidates.bin",
                                coast_candidate_stream);
        blend_validation::text(
            output / "minimap-coast-edge-candidates.json",
            "{\"schema\":\"civ5-minimap-coast-edge-candidates-v1\",\"count\":" +
                std::to_string(coast_candidates.size()) +
                ",\"acceptedByRetailBoundsGuards\":" +
                std::to_string(coast_accepted) +
                ",\"source\":\"controlled_coast_perturbation_cursor\","
                "\"registrationGuardsEvaluated\":true,"
                "\"candidateStreamFiltered\":false,\"rendered\":false}\n");
        const auto transform = civ5::ui::minimap_world_transform(
            terrain.source().layout, terrain.owner().parameters);
        auto bounds = civ5::ui::plan_minimap_visible_bounds(
            transform, terrain.owner().parameters, 0, 0,
            static_cast<std::int32_t>(grid.width()) - 1,
            static_cast<std::int32_t>(grid.height()) - 1, saved_minimap->wrap_x);
        if (native_minimap_bounds) {
            // The original post-load singleton supplies kind-7 world-space
            // segments and texture geometry. Edge subdivision is still the
            // locally derived mesh setting, not a claimed captured field.
            bounds.segments = native_minimap_bounds->visible.segments;
            bounds.segment_count = native_minimap_bounds->visible.segment_count;
            bounds.texture = native_minimap_bounds->visible.texture;
        }
        check(bounds.segment_count == 1,
              "The minimap scan needs a separately captured second wrap segment");
        const auto scan_window = civ5::ui::minimap_scan_window(
            bounds.segments[0], terrain.source().layout, terrain.owner().parameters);
        if (live_bnw_preview)
            live_minimap_geometry = civ5::ui::BnwMinimapFogGeometry{
                scan_window, transform, bounds.edge_subdivisions};
        blend_validation::text(
            output / "minimap-fog-scan.json",
            "{\"schema\":\"civ5-minimap-fog-scan-v1\",\"cellWindowInclusive\":[" +
                std::to_string(scan_window.left) + "," +
                std::to_string(scan_window.top) + "," +
                std::to_string(scan_window.right) + "," +
                std::to_string(scan_window.bottom) +
                "],\"source\":\"native_consumer_inverse_bounds_rule\","
                "\"fullGraphicsParity\":false}\n");
        const auto controlled_coast_records = civ5::ui::MinimapCoastEdgeRecordTable::build(
            coast_candidates, terrain.source().layout.source_grid,
            saved_minimap->wrap_x, transform);
        check(controlled_coast_records.accepted_pairs() ==
                  static_cast<std::size_t>(coast_accepted),
              "Minimap coast record table disagrees with registration guards");
        std::size_t controlled_coast_lookup_hits = 0;
        const auto controlled_coast_lookup = [&](const civ5::ui::MinimapEdgeQuery &query) {
            auto found = controlled_coast_records.lookup(query);
            controlled_coast_lookup_hits += found.has_value();
            return found;
        };
        const auto controlled_coast_meshes = civ5::ui::build_minimap_render_meshes(
            grid, scan_window,
            transform, bounds, controlled_coast_lookup);
        std::vector<GlMinimapCultureColor> captured_culture_colors;
        if (native_minimap_bounds) {
            check(saved_save_sha256 ==
                      "b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab",
                  "Native culture grid belongs to another saved game");
            civ5::ui::apply_gandhi_native_culture(grid);
            captured_culture_colors.push_back(
                {1, civ5::ui::gandhi_native_culture_rgb});
        }
        std::optional<civ5::ui::MinimapNativeEdgeRecordTable> native_edge_records;
        if (!native_minimap_edge_records_path.empty()) {
            const auto bytes = blend_validation::read(native_minimap_edge_records_path);
            native_edge_records.emplace(civ5::ui::MinimapNativeEdgeRecordTable::parse(
                bytes, saved_save_sha256, terrain.source().layout.source_grid));
            check(native_edge_records->size() == 1294,
                  "Captured native minimap edge table is incomplete");
        }
        std::size_t native_edge_lookup_hits = 0;
        const auto native_edge_lookup = [&](const civ5::ui::MinimapEdgeQuery &query)
            -> std::optional<civ5::ui::MinimapTerrainEdgeRecord> {
            if (!native_edge_records) return std::nullopt;
            auto found = native_edge_records->lookup(query);
            native_edge_lookup_hits += found.has_value();
            return found;
        };
        const auto meshes = civ5::ui::build_minimap_render_meshes(
            grid, scan_window,
            transform, bounds, native_edge_lookup);
        blend_validation::text(
            output / "minimap-native-edge-records.json",
            "{\"schema\":\"civ5-minimap-native-edge-preview-v1\",\"source\":\"" +
                std::string(native_edge_records ? "captured_retail_dx9_same_save" :
                            "synthetic_fallback") +
                "\",\"recordSlots\":" +
                std::to_string(native_edge_records ? native_edge_records->size() : 0) +
                ",\"lookupHits\":" + std::to_string(native_edge_lookup_hits) +
                ",\"fullGraphicsParity\":false}\n");
        const auto mountains = civ5::ui::build_minimap_mountain_markers(
            *saved_minimap, transform, terrain.owner().parameters, bounds);
        std::vector<GlMinimapKind5Marker> native_city_draws;
        if (!native_minimap_city_marker_fixture_path.empty()) {
            check(saved_save_sha256 ==
                      "b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab",
                  "Native city marker capture belongs to another saved game");
            // The pinned JSON fixture (validated by SHA-256 at entry) has three
            // identical stable reads of this exact 16-byte first-kind-5 record.
            const std::array<civ5::ui::MinimapCityMarker, 1> captured{{
                {831.965576171875f, 166.319580078125f, 25, 15,
                 {255, 153, 49, 255}}}};
            const auto cities = civ5::ui::reconcile_native_minimap_city_markers(
                *saved_minimap, *saved_art, transform, captured);
            const auto &delhi = saved_minimap->cells.at(15u * saved_minimap->grid.width + 32u);
            check(cities.size() == 1 && saved_art->active_player == 0 &&
                      saved_art->active_team == 0 &&
                      delhi.plot_city && delhi.plot_city->owner == 0 &&
                      delhi.plot_city->id == 8192 && mountains.radius == 32.0f,
                  "Captured Gandhi city marker differs from saved owner or radius");
            for (const auto &city : cities) {
                const auto argb = (std::uint32_t(city.rgba[3]) << 24) |
                                  (std::uint32_t(city.rgba[0]) << 16) |
                                  (std::uint32_t(city.rgba[1]) << 8) |
                                  std::uint32_t(city.rgba[2]);
                bool included = false;
                for (std::uint32_t segment = 0; segment < bounds.segment_count; ++segment) {
                    const auto &box = bounds.segments[segment];
                    if (city.world_x + mountains.radius < box.min_x ||
                        city.world_x - mountains.radius > box.max_x ||
                        city.world_y + mountains.radius < box.min_y ||
                        city.world_y - mountains.radius > box.max_y)
                        continue;
                    native_city_draws.push_back({city.world_x, city.world_y,
                                                 mountains.radius, argb,
                                                 civ5::ui::minimap_city_fade.x,
                                                 civ5::ui::minimap_city_fade.y, segment});
                    included = true;
                }
                check(included, "Captured Gandhi city marker lies outside the minimap");
            }
        }
        const auto native_city_count = native_city_draws.size();
        if (captured_g0_selected_marker_visible) {
            check(saved_save_sha256 ==
                      "b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab" &&
                      saved_art && saved_art->active_player == 0 && saved_art->active_team == 0 &&
                      retail_camera && retail_camera->width == 1914 &&
                      retail_camera->height == 1045 && native_minimap_bounds &&
                      native_city_count == 1 && mountains.radius == 32.0f,
                  "Visible G0 selected marker requires the captured Gandhi state");
            // The second retail kind-5 list contains this ID-zero record in the
            // retained alpha-255 series. This is an explicit visible-state
            // replay; it does not infer the blink phase of an ordinary frame.
            constexpr float selected_x = 799.965576171875f;
            constexpr float selected_y = 110.89398193359375f;
            const auto saved_warrior_plot = civ5::ui::minimap_project_cube(
                transform, {25.0f, 14.0f, -39.0f, 1.0f});
            check(std::abs(saved_warrior_plot.x - selected_x) <= 0.001f &&
                      std::abs(saved_warrior_plot.y - selected_y) <= 0.001f,
                  "Captured selected marker differs from the saved warrior plot");
            bool included = false;
            for (std::uint32_t segment = 0; segment < bounds.segment_count; ++segment) {
                const auto &box = bounds.segments[segment];
                if (selected_x + mountains.radius < box.min_x ||
                    selected_x - mountains.radius > box.max_x ||
                    selected_y + mountains.radius < box.min_y ||
                    selected_y - mountains.radius > box.max_y)
                    continue;
                native_city_draws.push_back({selected_x, selected_y, mountains.radius,
                                             0xFFFFFFFFu,
                                             civ5::ui::minimap_city_fade.x,
                                             civ5::ui::minimap_city_fade.y, segment});
                included = true;
            }
            check(included, "Captured G0 selected marker lies outside the minimap");
            blend_validation::text(output / "minimap-selected-kind5-visible.json",
                "{\"schema\":\"civ5-g0-selected-kind5-visible-preview-v1\","
                "\"source\":\"captured_retail_second_kind5_alpha255\","
                "\"id\":0,\"worldXY\":[799.965576171875,110.89398193359375],"
                "\"radius\":32,\"argb\":\"FFFFFFFF\","
                "\"blinkPhaseInferred\":false,\"fullMinimapParityProven\":false}\n");
        }
        blend_validation::text(
            output / "minimap-native-city-markers.json",
            "{\"schema\":\"civ5-minimap-native-city-preview-v1\",\"source\":\"" +
                std::string(native_minimap_city_marker_fixture_path.empty()
                                ? "no_runtime_capture_for_this_save"
                                : "captured_retail_dx9_same_save") +
                "\",\"recordCount\":" + std::to_string(native_city_count) +
                ",\"rendered\":" +
                std::string(native_city_count == 0 ? "false" : "true") +
                ",\"cityCreatedCallbackTraced\":false,\"fullGraphicsParity\":false}\n");
        std::vector<std::uint8_t> mountain_stream{'M', 'M', 'M', '1'};
        const auto append_mountain_u32 = [&](std::uint32_t value) {
            for (unsigned shift = 0; shift < 32; shift += 8)
                mountain_stream.push_back(static_cast<std::uint8_t>(value >> shift));
        };
        const auto append_mountain_float = [&](float value) {
            const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
            mountain_stream.insert(mountain_stream.end(), bytes, bytes + sizeof(value));
        };
        append_mountain_float(mountains.radius);
        for (const auto &indices : mountains.visible_indices) {
            check(indices.size() <= std::numeric_limits<std::uint32_t>::max(),
                  "Minimap mountain marker count is invalid");
            append_mountain_u32(static_cast<std::uint32_t>(indices.size()));
        }
        for (const auto &indices : mountains.visible_indices)
            for (const auto index : indices) {
                const auto &marker = mountains.source.at(index);
                append_mountain_u32(marker.map_x);
                append_mountain_u32(marker.map_y);
                append_mountain_float(marker.center.x);
                append_mountain_float(marker.center.y);
            }
        blend_validation::write(output / "minimap-mountain-markers.bin", mountain_stream);
        const auto ice_plots = civ5::ui::project_saved_minimap_ice_plots(
            *saved_minimap, transform);
        std::optional<civ5::ui::MinimapNativeIceRecords> native_ice;
        if (!native_ice_records_path.empty())
            native_ice = civ5::ui::read_minimap_native_ice_records(native_ice_records_path);
        std::vector<civ5::ui::MinimapIceRegion> ice_regions;
        ice_regions.reserve(native_ice ? native_ice->regions.size() : ice_plots.size());
        std::size_t revealed_ice = 0;
        for (const auto &plot : ice_plots) {
            if (!native_ice) ice_regions.push_back({plot.bounds});
            if (saved_art->plots.at(plot.plot_index).revealed == true) {
                ++revealed_ice;
                revealed_ice_plot_indices.push_back(plot.plot_index);
            }
        }
        if (native_ice)
            for (const auto &mesh : native_ice->regions)
                ice_regions.push_back({{mesh.box[0], mesh.box[1], mesh.box[2], mesh.box[3]}});
        const auto ice_draws = civ5::ui::plan_minimap_ice_draws(
            bounds.segments[0], terrain.owner().parameters.world_span_x,
            ice_regions);
        saved_minimap_visible = meshes.visible;
        if (native_minimap_bounds) {
            const auto &native = *native_minimap_bounds;
            const auto &box = native.visible.segments[0];
            const float marker_x = native.visible.texture.padding_x +
                (native.delhi_world[0] - box.min_x) * native.visible.texture.active_width /
                (box.max_x - box.min_x);
            const float marker_y = (box.max_y - native.delhi_world[1]) * 190.f /
                (box.max_y - box.min_y);
            std::ostringstream evidence;
            evidence << std::setprecision(9)
                     << "{\"schema\":\"civ5-minimap-native-bounds-preview-v1\","
                     << "\"selectedSaveSha256\":\"" << saved_save_sha256 << "\","
                     << "\"segmentCount\":" << native.visible.segment_count
                     << ",\"firstBounds\":[" << box.min_x << ',' << box.min_y << ','
                     << box.max_x << ',' << box.max_y << "],"
                     << "\"heightSource\":" << native.height_source
                     << ",\"planeHeight\":" << native.plane_height
                     << ",\"projectedDelhiMapLocal\":[" << marker_x << ',' << marker_y << "],"
                     << "\"originalDelhiMapLocal\":["
                     << native.reference_marker_map_local[0] << ','
                     << native.reference_marker_map_local[1] << "],"
                     << "\"capturedCameraCornersApplied\":false,"
                     << "\"fullGraphicsParity\":false}\n";
            blend_validation::text(output / "minimap-native-bounds.json", evidence.str());
        }
        const auto &visible = meshes.visible.segments[0];
        // Preserve the exact diagnostic mesh used for this GPU draw so the
        // original DX9 control can rerun on the same saved-state geometry.
        std::vector<std::uint8_t> triangle_stream{'M', 'M', 'S', '1'};
        const auto append_u32 = [&](std::uint32_t value) {
            for (unsigned shift = 0; shift < 32; shift += 8)
                triangle_stream.push_back(static_cast<std::uint8_t>(value >> shift));
        };
        const auto append_layer = [&](const civ5::ui::MinimapRenderLayer &layer) {
            if (layer.triangles.size() % 3 ||
                layer.triangle_count() > std::numeric_limits<std::uint32_t>::max())
                throw std::runtime_error("Invalid minimap diagnostic mesh extent");
            append_u32(static_cast<std::uint32_t>(layer.triangle_count()));
            static_assert(sizeof(civ5::ui::MinimapPoint2) == 2 * sizeof(float));
            const auto *bytes = reinterpret_cast<const std::uint8_t *>(layer.triangles.data());
            triangle_stream.insert(triangle_stream.end(), bytes,
                                   bytes + layer.triangles.size() * sizeof(layer.triangles[0]));
        };
        append_layer(meshes.coast);
        append_layer(meshes.land);
        append_layer(meshes.fog);
        blend_validation::write(output / "minimap-surface-triangles.bin", triangle_stream);
        // The retail kind-3 minimap pass reuses the river surface VB/IB from
        // 0x019FC464. Keep this candidate separate from MMS1 because this
        // preview's saved-river CRT cursor is still controlled, not recovered.
        const auto &rivers = terrain.rivers();
        check(rivers && rivers->generated,
              "Saved minimap river surface has no generated river geometry");
        const auto &river_geometry = *rivers->generated;
        civ5::terrain::RiverSurfaceSettings river_settings{};
        river_settings.offset = {terrain.owner().parameters.center_x,
                                 terrain.owner().parameters.center_y, 0.0f};
        const auto river_surface = civ5::terrain::build_river_surface(
            river_geometry.centerlines, river_geometry.bank_lines,
            river_geometry.sampled_heights, river_settings);
        check(river_surface.indices.size() % 3 == 0 &&
                  river_surface.indices.size() / 3 <= std::numeric_limits<std::uint32_t>::max(),
              "Saved minimap river surface triangle count is invalid");
        std::vector<std::uint8_t> river_stream{'M', 'M', 'R', '1'};
        for (unsigned shift = 0; shift < 32; shift += 8)
            river_stream.push_back(static_cast<std::uint8_t>(
                (river_surface.indices.size() / 3) >> shift));
        std::vector<civ5::ui::MinimapPoint2> river_triangles;
        river_triangles.reserve(river_surface.indices.size());
        for (const auto index : river_surface.indices) {
            const auto &vertex = river_surface.vertices.at(index);
            river_triangles.push_back({vertex[0], vertex[1]});
            for (float coordinate : {vertex[0], vertex[1]}) {
                const auto *bytes = reinterpret_cast<const std::uint8_t *>(&coordinate);
                river_stream.insert(river_stream.end(), bytes, bytes + sizeof(float));
            }
        }
        blend_validation::write(output / "minimap-river-shallows-triangles.bin", river_stream);
        const auto original_dds = [&](std::string_view name) {
            const auto selected = ui_find(civ5::assets::AssetKey(std::string(name)));
            const auto *handle = std::get_if<civ5::assets::AssetHandle>(&selected);
            check(handle && handle->candidate().provenance.archive_relative_path ==
                                "Resource/DX9/MiscTextures.fpk",
                  "Minimap GPU source did not come from the installed MiscTextures archive");
            return civ5::assets::DdsData::parse(handle->read_bytes());
        };
        const std::array<civ5::assets::DdsData, 6> sources{
            original_dds("MiniMapWater.dds"), original_dds("MiniMapShallowWater.dds"),
            original_dds("MiniMapGrass.dds"), original_dds("MiniMapFog.dds"),
            original_dds("MiniMapMountain.dds"), original_dds("MiniMapIce.dds")};
        const auto culture_source = native_minimap_bounds
            ? std::optional<civ5::assets::DdsData>(original_dds("MiniMapCulture.dds"))
            : std::nullopt;
        auto surface_owner = std::make_unique<GlMinimapSurfaceDiagnostic>(
            context, sources, culture_source ? &*culture_source : nullptr);
        auto &surface = *surface_owner;
        std::optional<GlMinimapSurfaceDiagnostic::PrefogSource> prefog_source;
        if (live_bnw_preview) {
            check(live_scene_receipt && live_minimap_geometry,
                  "Live minimap has no selected declared scene/geometry binding");
            prefog_source = GlMinimapSurfaceDiagnostic::PrefogSource{
                *live_scene_receipt, *live_minimap_geometry};
        }
        surface.render(meshes, river_triangles, mountains, ice_plots,
                       terrain.owner().parameters.world_span_x,
                       native_ice ? &*native_ice : nullptr,
                       output / "minimap-water-gpu.ppm",
                       output / "minimap-coast-gpu.ppm",
                       output / "minimap-land-gpu.ppm",
                       output / (native_ice ? "minimap-ice-native-gpu.ppm" :
                                              "minimap-ice-candidate-gpu.ppm"),
                       output / "minimap-river-gpu.ppm",
                       output / "minimap-mountain-gpu.ppm", 0.5f,
                       output / "minimap-fog-surface-only-gpu.ppm",
                       output / "minimap-mountain-first-before-gpu.ppm",
                       output / "minimap-mountain-first-after-gpu.ppm",
                       native_city_draws, captured_culture_colors,
                       output / "minimap-culture-gpu.ppm",
                       prefog_source.has_value(), prefog_source ? &*prefog_source : nullptr);
        surface.save_ppm(output / "minimap-surface-gpu.ppm");
        const auto minimap_bgra = surface.output.read_mip_raw(0);
        saved_minimap_base_bgra = minimap_bgra;
        check(minimap_bgra.size() == 320u * 190u * 4u,
              "Minimap GPU readback extent changed");
        for (std::size_t pixel = 0; pixel < 320u * 190u; ++pixel)
            check(minimap_bgra[pixel * 4u + 3u] == 255,
                  "Minimap GPU alpha differs from the opaque DX9 target");
        GlMinimapSurfaceDiagnostic controlled_coast_surface(context, sources);
        controlled_coast_surface.render(
            controlled_coast_meshes, river_triangles, mountains, ice_plots,
            terrain.owner().parameters.world_span_x,
            native_ice ? &*native_ice : nullptr,
            {}, output / "minimap-coast-records-controlled-coast-gpu.ppm",
            output / "minimap-coast-records-controlled-land-gpu.ppm",
            {}, {}, {}, 0.5f);
        controlled_coast_surface.save_ppm(output / "minimap-coast-records-controlled-gpu.ppm");
        const auto controlled_bgra = controlled_coast_surface.output.read_mip_raw(0);
        check(controlled_bgra.size() == minimap_bgra.size(),
              "Controlled coast minimap GPU readback differs in size");
        std::size_t coast_changed_pixels = 0;
        std::uint32_t coast_max_channel_difference = 0;
        for (std::size_t pixel = 0; pixel < 320u * 190u; ++pixel) {
            bool changed = false;
            for (std::size_t channel = 0; channel < 3; ++channel) {
                const auto a = minimap_bgra[pixel * 4u + channel];
                const auto b = controlled_bgra[pixel * 4u + channel];
                const auto difference = a > b ? a - b : b - a;
                coast_max_channel_difference = std::max(
                    coast_max_channel_difference, std::uint32_t(difference));
                changed |= difference != 0;
            }
            coast_changed_pixels += changed;
            check(controlled_bgra[pixel * 4u + 3u] == 255,
                  "Controlled coast minimap alpha is not opaque");
        }
        check(coast_changed_pixels != 0,
              "Registered controlled coast records did not affect the minimap");
        blend_validation::text(
            output / "minimap-coast-records-controlled.json",
            "{\"schema\":\"civ5-minimap-coast-records-controlled-v1\","
            "\"acceptedPairs\":" + std::to_string(controlled_coast_records.accepted_pairs()) +
            ",\"distinctSlots\":" + std::to_string(controlled_coast_records.distinct_slots()) +
            ",\"recordLookupHits\":" + std::to_string(controlled_coast_lookup_hits) +
            ",\"changedPixelsAgainstFallback\":" + std::to_string(coast_changed_pixels) +
            ",\"maximumChannelDifference\":" +
                std::to_string(coast_max_channel_difference) +
            ",\"exactOriginalPointValues\":false,\"diagnosticOnly\":true,"
            "\"displayedInPreview\":false}\n");
        minimap_source->partial_map.emplace(std::move(surface.output));
        if (live_bnw_preview) live_minimap_surface = std::move(surface_owner);
        std::ostringstream report;
        std::size_t culture_triangles = 0;
        for (const auto &layer : meshes.culture)
            culture_triangles += layer.mesh.triangle_count();
        report << std::setprecision(9)
               << "{\"schema\":\"civ5-minimap-surface-gpu-diagnostic-v1\","
               << "\"width\":320,\"height\":190,\"paddingX\":"
               << bounds.texture.padding_x << ",\"activeWidth\":"
               << bounds.texture.active_width << ",\"revealedCells\":" << revealed
               << ",\"grid\":[" << grid.width() << ',' << grid.height() << ']'
               << ",\"bounds\":[" << visible.min_x << ',' << visible.min_y << ','
               << visible.max_x << ',' << visible.max_y << ']'
               << ",\"boundsSource\":\""
               << (native_minimap_bounds ? "passive_retail_dx9_same_save" :
                   "derived_full_grid") << "\""
               << ",\"edgeSubdivisionsSource\":\"derived_preview\""
               << ",\"coastTriangles\":" << meshes.coast.triangle_count()
               << ",\"landTriangles\":" << meshes.land.triangle_count()
               << ",\"cultureBatches\":" << meshes.culture.size()
               << ",\"cultureTriangles\":" << culture_triangles
               << ",\"fogTriangles\":" << meshes.fog.triangle_count()
               << ",\"visibleSegments\":" << bounds.segment_count
               << ",\"secondBounds\":[" << bounds.segments[1].min_x << ','
               << bounds.segments[1].min_y << ',' << bounds.segments[1].max_x
               << ',' << bounds.segments[1].max_y << ']'
               << ",\"riverShallowsTriangles\":" << river_surface.indices.size() / 3
               << ",\"riverShallowsFromControlledCursor\":true"
               << ",\"riverShallowsRendered\":true"
               << ",\"mountainSourceCount\":" << mountains.source.size()
               << ",\"mountainVisibleFirst\":" << mountains.visible_indices[0].size()
               << ",\"mountainVisibleSecond\":" << mountains.visible_indices[1].size()
               << ",\"mountainRadius\":" << mountains.radius
               << ",\"mountainSpritesRendered\":true"
               << ",\"nativeCityMarkerCount\":" << native_city_count
               << ",\"nativeCityMarkerSource\":\""
               << (native_minimap_city_marker_fixture_path.empty()
                       ? "none" : "captured_retail_dx9_same_save") << "\""
               << ",\"capturedSelectedMarkerVisible\":"
               << (captured_g0_selected_marker_visible ? "true" : "false")
               << ",\"icePlotCount\":" << ice_plots.size()
               << ",\"iceDrawCount\":" << ice_draws.size()
               << ",\"revealedIceCells\":" << revealed_ice
               << ",\"iceGeometrySource\":\""
               << (native_ice ? "offline_retail_region_records" : "saved_feature_hex_candidate")
               << "\""
               << ",\"nativeIceShaderBoxSource\":\""
               << (native_ice ? "global_component" : "none") << "\""
               << ",\"nativeIceRegionCount\":" << (native_ice ? native_ice->regions.size() : 0)
               << ",\"nativeIceTriangles\":" << (native_ice ? native_ice->indices / 3 : 0)
               << ",\"nativeIceVertexFormat\":\""
               << (native_ice ? "short2n_index16" : "none") << "\""
               << ",\"nativeIceGpuBytes\":"
               << (native_ice ? 8ull * native_ice->packed_vertices + 2ull * native_ice->indices : 0)
               << ",\"nativeIceRegionTableCaptured\":false"
               << ",\"surfacePixelShift\":0.5"
               << ",\"opaqueAlphaPixels\":60800"
               << ",\"completeNativePassList\":false}\n";
        blend_validation::text(output / "minimap-surface-gpu.json", report.str());
    }
    check(ui_language == "en_US" || ui_language == "ru_RU", "Preview UI language is unsupported");
    const bool russian_ui = ui_language == "ru_RU";
    const std::string language_directory = russian_ui ? "RU_RU" : "EN_US";
    const std::string language_root = russian_ui ? "Language_RU_RU" : "Language_en_US";
    const auto metadata_key = "Assets/Gameplay/XML/NewText/" +
                              std::string(russian_ui ? "Russian.xml" : "English.xml");
    const auto metadata_lookup = ui_find(civ5::assets::AssetKey(metadata_key));
    const auto *metadata_handle = std::get_if<civ5::assets::AssetHandle>(&metadata_lookup);
    check(metadata_handle != nullptr, "Preview language metadata is unavailable or ambiguous");
    const auto metadata_bytes = metadata_handle->read_bytes();
    const auto metadata = civ5::content::read_xml_metadata(
        std::string(metadata_bytes.begin(), metadata_bytes.end()));
    const auto *languages = metadata.child("Languages");
    check(metadata.name == "GameData" && languages != nullptr && languages->children.size() == 1 &&
              languages->children[0].name == "Row", "Preview language metadata has changed");
    const auto &language_row = languages->children[0];
    const auto *language_type = language_row.child("Type");
    const auto *language_table = language_row.child("TableName");
    check(language_type && language_table && language_type->text == ui_language &&
              language_table->text == language_root, "Preview language metadata differs from selection");
    std::string extended_face;
    if (russian_ui) {
        const auto *extended = language_row.child("UseExtendedFont");
        const auto *face = language_row.child("FontName");
        check(extended && face && extended->text == "1" && !face->text.empty(),
              "Russian preview requires the authored extended font");
        extended_face = face->text;
    }
    const auto localization_prefix = "Assets/Gameplay/XML/NewText/" + language_directory;
    const auto localization_root = root / localization_prefix;
    auto localization_keys = ui_resources->keys(localization_prefix);
    if (localization_keys.empty())
        for (const auto &entry : std::filesystem::recursive_directory_iterator(localization_root))
            if (entry.is_regular_file() && entry.path().extension() == ".xml")
                localization_keys.push_back(civ5::assets::path_utf8(entry.path().lexically_relative(root)));
    localization_keys.erase(std::remove_if(localization_keys.begin(), localization_keys.end(),
        [](const std::string &key) {
            auto extension = std::filesystem::path(key).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return extension != ".xml" || key.find("/LeaderDialog/") != std::string::npos ||
                   key.find("Scenario") != std::string::npos;
        }), localization_keys.end());
    std::sort(localization_keys.begin(), localization_keys.end());
    auto localization = civ5::ui::TopPanelLocalizationCatalog::read_resources(
        *ui_resources, localization_keys, language_root);
    if (expansion2_ui) {
        const auto expansion2_prefix = "Assets/DLC/Expansion2/Gameplay/XML/Text/" + language_directory + "/";
        for (const auto *name : {"CIV5GameTextInfos_InGameScreens_Inherited_Expansion2.xml",
                                 "CIV5GameTextInfos_InGameScreens_Expansion2.xml",
                                 "CIV5GameTextInfos_WorldView_Expansion2.xml"}) {
            const auto extra = civ5::ui::TopPanelLocalizationCatalog::read_resources(
                *ui_resources, {expansion2_prefix + name}, language_root);
            for (const auto &[key, value] : extra.rows)
                localization.rows.insert_or_assign(key, value);
        }
    }
    std::optional<std::string> saved_turn_zero_date;
    if (saved_turn_zero_year) {
        const auto key = *saved_turn_zero_year < 0 ? "TXT_KEY_TIME_BC" : "TXT_KEY_TIME_AD";
        const auto magnitude = *saved_turn_zero_year < 0
                                   ? -static_cast<std::int64_t>(*saved_turn_zero_year)
                                   : static_cast<std::int64_t>(*saved_turn_zero_year);
        saved_turn_zero_date = localization.format(key, {std::to_string(magnitude)});
        blend_validation::text(output / "saved-turn-date.txt",
                               "year=" + std::to_string(*saved_turn_zero_year) + " key=" + key +
                                   " date=" + *saved_turn_zero_date + "\n");
    }
    auto tooltip_style = civ5::ui::read_top_panel_grid9_style(
        *ui_resources, top_panel_layout.tooltip_grid.attribute("Style"));
    check(tooltip_style.texture == "Assets/UI/Art/Controls/9 Grids/Grid9Black8.dds",
          "TopPanel tooltip style changed its authored texture");
    const auto tooltip_lookup = ui_find(civ5::assets::AssetKey("Grid9Black8.dds"));
    check(std::holds_alternative<civ5::assets::AssetHandle>(tooltip_lookup),
          "TopPanel Grid9Black8 lookup changed after binding");
    const auto tooltip_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(tooltip_lookup).read_bytes());
    check(tooltip_data.width() == 64 && tooltip_data.height() == 64,
          "TopPanel Grid9Black8 dimensions changed after binding");
    const auto over_supply_lookup = ui_find(civ5::assets::AssetKey("OverSupplyLimit.dds"));
    check(std::holds_alternative<civ5::assets::AssetHandle>(over_supply_lookup),
          "TopPanel OverSupplyLimit.dds is unavailable or ambiguous");
    const auto over_supply_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(over_supply_lookup).read_bytes());
    check(over_supply_data.width() == 22 && over_supply_data.height() == 16,
          "TopPanel OverSupplyLimit.dds dimensions changed from the authored 22x16 source");
    auto top_panel_font = civ5::ui::read_glyphgen(*ui_resources, "Assets/UI/Fonts/Tw Cent MT/TwCenMT16.ggxml");
    const auto base_font = std::find_if(top_panel_font.textures.begin(), top_panel_font.textures.end(),
                                        [](const auto &texture) { return texture.name() == "TwCenMT16Base"; });
    check(base_font != top_panel_font.textures.end(), "TwCenMT16 Base texture declaration is missing");
    const auto font_asset = ui_find(civ5::assets::AssetKey(base_font->source()));
    const auto font_index_asset = ui_find(
        civ5::assets::AssetKey(base_font->source().substr(0, base_font->source().size() - 4) + "-index.dds"));
    check(std::holds_alternative<civ5::assets::AssetHandle>(font_asset) &&
              std::holds_alternative<civ5::assets::AssetHandle>(font_index_asset),
          "TwCenMT16 GlyphGen assets are unavailable or ambiguous");
    const auto font_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(font_asset).read_bytes());
    const auto font_index_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(font_index_asset).read_bytes());
    auto right_panel_font = civ5::ui::read_glyphgen(*ui_resources, "Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml");
    const auto right_base_font = std::find_if(
        right_panel_font.textures.begin(), right_panel_font.textures.end(),
        [](const auto &texture) { return texture.name() == "TwCenMT20Base"; });
    check(right_base_font != right_panel_font.textures.end(), "TwCenMT20 Base texture declaration is missing");
    const auto right_font_asset = ui_find(civ5::assets::AssetKey(right_base_font->source()));
    const auto right_font_index_asset = ui_find(
        civ5::assets::AssetKey(right_base_font->source().substr(0, right_base_font->source().size() - 4) +
                               "-index.dds"));
    check(std::holds_alternative<civ5::assets::AssetHandle>(right_font_asset) &&
              std::holds_alternative<civ5::assets::AssetHandle>(right_font_index_asset),
          "TwCenMT20 GlyphGen assets are unavailable or ambiguous");
    const auto right_font_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(right_font_asset).read_bytes());
    const auto right_font_index_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(right_font_index_asset).read_bytes());
    const auto divider_asset = ui_find(civ5::assets::AssetKey("Divider.dds"));
    check(std::holds_alternative<civ5::assets::AssetHandle>(divider_asset),
          "TopPanel Divider.dds is unavailable or ambiguous");
    const auto divider_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(divider_asset).read_bytes());
    check(divider_data.width() == 14 && divider_data.height() == 6,
          "TopPanel Divider.dds dimensions changed from the authored 14x6 source");
    auto icon_font = civ5::ui::read_glyphgen(
        *ui_resources, expansion2_ui
            ? "Assets/DLC/Expansion2/UI/Fonts/Font Icons/FontIcons.ggxml"
            : "Assets/UI/Fonts/Font Icons/FontIcons.ggxml");
    const auto icon_texture = std::find_if(icon_font.textures.begin(), icon_font.textures.end(),
                                           [](const auto &texture) { return texture.source() == "FontIcons.dds"; });
    check(icon_texture != icon_font.textures.end(), "FontIcons texture declaration is missing");
    const auto icon_asset = ui_find(civ5::assets::AssetKey(icon_texture->source()));
    const auto icon_index_asset = ui_find(
        civ5::assets::AssetKey(icon_texture->source().substr(0, icon_texture->source().size() - 4) + "-index.dds"));
    check(std::holds_alternative<civ5::assets::AssetHandle>(icon_asset) &&
              std::holds_alternative<civ5::assets::AssetHandle>(icon_index_asset),
          "FontIcons DDS assets are unavailable or ambiguous");
    const auto icon_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(icon_asset).read_bytes());
    const auto icon_index_data = civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(icon_index_asset).read_bytes());
    const auto mapping_lookup = ui_find(
        civ5::assets::AssetKey("Assets/Gameplay/XML/GameInfo/CIV5IconFontMapping.xml"));
    const auto *mapping_handle = std::get_if<civ5::assets::AssetHandle>(&mapping_lookup);
    check(mapping_handle != nullptr, "CIV5IconFontMapping source is unavailable or ambiguous");
    const auto mapping_bytes = mapping_handle->read_bytes(64ULL * 1024 * 1024);
    const auto mapping_document = civ5::content::read_xml_metadata(
        std::string(mapping_bytes.begin(), mapping_bytes.end()));
    const auto mapping_node = mapping_document.child("IconFontMapping");
    check(mapping_node != nullptr, "CIV5IconFontMapping source is missing");
    std::unordered_map<std::string, std::uint32_t> icon_mapping;
    for (const auto &row : mapping_node->children) {
        check(row.name == "Row", "CIV5IconFontMapping contains a non-Row child");
        const auto name = row.child("IconName");
        const auto value = row.child("IconMapping");
        check(name != nullptr && value != nullptr && !name->text.empty() && !value->text.empty(),
              "CIV5IconFontMapping row is incomplete");
        std::size_t consumed = 0;
        const auto number = std::stoul(value->text, &consumed);
        check(consumed == value->text.size() && number <= 65535, "CIV5IconFontMapping value is invalid");
        check(icon_mapping.emplace(name->text, static_cast<std::uint32_t>(number)).second,
              "CIV5IconFontMapping contains a duplicate icon");
    }
    if (expansion2_ui) {
        const auto extra_lookup = ui_find(civ5::assets::AssetKey(
            "Assets/DLC/Expansion2/Gameplay/XML/GameInfo/CIV5IconFontMapping_Expansion2.xml"));
        const auto *extra_handle = std::get_if<civ5::assets::AssetHandle>(&extra_lookup);
        check(extra_handle != nullptr, "BNW icon mapping source is unavailable or ambiguous");
        const auto extra_bytes = extra_handle->read_bytes(64ULL * 1024 * 1024);
        const auto extra_document = civ5::content::read_xml_metadata(
            std::string(extra_bytes.begin(), extra_bytes.end()));
        const auto extra_node = extra_document.child("IconFontMapping");
        check(extra_node != nullptr, "BNW icon mapping root is missing");
        for (const auto &row : extra_node->children) {
            check(row.name == "Row" || row.name == "InsertOrAbort",
                  "BNW icon mapping contains an unknown operation");
            if (row.child("IconFontTexture"))
                continue;
            const auto name = row.child("IconName");
            const auto value = row.child("IconMapping");
            check(name && value && !name->text.empty() && !value->text.empty(),
                  "BNW icon mapping row is incomplete");
            std::size_t consumed = 0;
            const auto number = std::stoul(value->text, &consumed);
            check(consumed == value->text.size() && number <= 65535,
                  "BNW icon mapping value is invalid");
            icon_mapping.insert_or_assign(name->text, static_cast<std::uint32_t>(number));
        }
        check(icon_mapping.contains("ICON_TOURISM") && icon_mapping.contains("ICON_TURNS_REMAINING"),
              "BNW TopPanel icon mapping is incomplete");
    }
    check(icon_mapping.contains("ICON_RESEARCH") && icon_mapping.contains("ICON_GOLD") &&
              icon_mapping.contains("ICON_RES_IRON"),
          "TopPanel icon mapping is incomplete");
    unsigned top_panel_actions = 0;
    std::optional<civ5::ui::TopPanelAction> last_top_panel_action;
    std::vector<civ5::ui::TopPanelActionEvent> top_panel_events;
    const auto record_top_panel_action = [&](const civ5::ui::TopPanelActionEvent &event) {
        ++top_panel_actions;
        last_top_panel_action = event.action;
        top_panel_events.push_back(event);
        if (top_panel_action_sink)
            top_panel_action_sink(event);
    };
    auto prepared_font = prepare_preview_alpha_font(top_panel_font, font_index_data, font_data,
                                                    localization, extended_face);
    auto right_font_texts = localization;
    if (live_bnw_preview || live_ui_provider) {
        // The right-panel renderer also paints Civilopedia articles. Feed its
        // prepared atlas the exact active BNW article text, including glyphs
        // absent from the smaller TopPanel/InGame localization selection.
        const auto pedia_texts = civ5::ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
            *ui_resources, language_root);
        const auto articles = civ5::ui::read_civilopedia_entries(*ui_resources,
            [&pedia_texts](std::string_view key) -> std::optional<std::string> {
                const auto found = pedia_texts.rows.find(std::string(key));
                return found == pedia_texts.rows.end() ? std::nullopt :
                    std::optional<std::string>(found->second);
            });
        for (std::size_t i = 0; i < articles.size(); ++i) {
            right_font_texts.rows.insert_or_assign("__civis_pedia_title_" +
                std::to_string(i), articles[i].title);
            right_font_texts.rows.insert_or_assign("__civis_pedia_body_" +
                std::to_string(i), articles[i].description);
        }
    }
    auto prepared_right_font = prepare_preview_alpha_font(right_panel_font, right_font_index_data,
                                                          right_font_data, right_font_texts, extended_face);
    {
        std::ofstream report(output / "ui-language.json");
        report << "{\"schema\":\"civ5-preview-ui-language-v1\",\"language\":\"" << ui_language
               << "\",\"extendedFont\":\"" << extended_face
               << "\",\"topPanelAddedGlyphs\":" << prepared_font.extended_glyphs
               << ",\"rightPanelAddedGlyphs\":" << prepared_right_font.extended_glyphs << "}\n";
        check(static_cast<bool>(report), "Could not write preview UI language report");
    }
    auto localized_state_provider = [source = std::move(top_panel_state_provider),
                                     saved_turn_zero_date, expansion2_ui]() {
        auto state = source();
        state.expansion2_ui = expansion2_ui;
        if (saved_turn_zero_date)
            state.turn_date = *saved_turn_zero_date;
        return state;
    };
    PreviewPresenter presenter(*context, std::move(top_panel_layout), std::move(tooltip_style),
                               std::move(localization), top_panel_data, tooltip_data, over_supply_data,
                               std::move(top_panel_font), std::move(prepared_font),
                               std::move(right_panel_font), std::move(prepared_right_font),
                               divider_data, std::move(icon_font), icon_index_data, icon_data,
                               std::move(icon_mapping),
                               std::move(localized_state_provider),
                               record_top_panel_action, std::move(minimap_source));
    presenter.center_unit_imported=owned_warrior;
    presenter.center_unit_imported_lifetime=center_imported_lifetime;
    // Only the explicitly supplied existing RunOutput directory is eligible.
    // No cwd fallback, stale-file reuse or activation-source dependency.
    if(!output.empty()){
        std::error_code capture_error;
        const auto capture_directory=fs::canonical(output,capture_error);
        if(!capture_error && fs::is_directory(capture_directory,capture_error) && !capture_error){
            const auto candidate=capture_directory/"first-presented.ppm";
            const bool already_exists=fs::exists(candidate,capture_error);
            if(!capture_error && !already_exists)presenter.first_presented_capture_path=candidate;
        }
    }
    std::optional<civ5::ui::WorldViewSkin> live_world_skin;
    std::vector<civ5::ui::SystemPediaEntry> live_pedia_entries;
    std::optional<civ5::ui::UiContextRegistry> live_registry;
    std::optional<civ5::ui::UiScreenController> live_shared_screens;
    // Local owner dies BEFORE its borrowed shared screen controller.
    // Stored callbacks use weak ownership; retained frames fail closed.
    std::shared_ptr<civ5::ui::BnwTechTreeLifecycle> live_tech_tree_lifecycle;
    std::optional<civ5::ui::EconomicOverviewScene> live_economic_scene;
    std::string live_economic_feed_blocker;
    civ5::validation::PopupFeedProvider live_other_popup_feed;
    auto live_popup_mux_status =
        std::make_shared<civ5::validation::PopupFeedMuxStatus>();
    // Destruction order: actual UI host/compositor retire before their raw
    // production backend; its callbacks retain these genuine same-world services.
    std::shared_ptr<civ5::ui::SelectedCityProductionServices> live_selected_production_services;
    std::unique_ptr<civ5::ui::BnwCityProductionUiBackend> live_selected_production_backend;
    // Explicit validation-only ingress, never ordinary EndTurn. The request is
    // queued only by a genuine enabled TRAIN hit and performed typed dispatch.
    std::optional<civ5::ui::CityChooseProduction> validation_train_choice;
    std::shared_ptr<const civ5::runtime::BnwCityProductionTurnPending> validation_train_cursor;
    std::shared_ptr<const civ5::ui::SelectedCityTrainCreateSource> validation_completed_train_source;
    std::optional<civ5::ui::SelectedTrainInitializationObservation> validation_train_initialization;
    bool validation_train_component_requested=false,validation_train_initialization_requested=false;
    bool validation_train_component_attempted=false;
    const auto write_production_validation=[&](const std::string& actual){
        if(!output.empty()){
            std::ofstream report(output/"production-component-validation.log",std::ios::app);
            if(report)report<<actual<<'\n'; // observations only, no alpha-ready receipt
        }
    };
    std::shared_ptr<const civ5::ui::BnwTrainRendererRefreshReceipt> live_train_renderer_refresh;
    std::shared_ptr<civ5::ui::LiveUiHost> live_ui_host;
    std::shared_ptr<civ5::ui::BnwAlphaProgressionBackend> live_alpha_research_backend;
    std::optional<civ5::ui::TradeRouteTabSourceProof> live_trade_tab_proof;
    std::optional<civ5::ui::TradeRouteOverviewInput> live_trade_tabs;
    std::optional<civ5::ui::UiScreenCompositor> live_compositor;
    std::shared_ptr<civ5::ui::UiGlyphGenStyleSet> live_font_styles;
    std::optional<civ5::ui::UiScreenGlRenderer> live_screen_painter;
    std::optional<civ5::ui::BnwWorldViewRendererRunLoan::Binding> worldview_projection_binding;
    std::function<void()> worldview_projection_initialize;
    std::shared_ptr<const civ5::ui::BnwTechAwardAnimationSample> tech_award_update_sample;
    std::unique_ptr<civ5::ui::BnwScrollAnimationLoopTick> scroll_loop_tick;
    std::shared_ptr<civ5::graphics::GlBnwNaturalWonderAnchorBackend>
        natural_wonder_anchors;
    std::shared_ptr<civ5::graphics::GlBnwWorldAnchorScene> world_anchor_scene;
    std::optional<PreviewAlphaFont> live_title_font;
    std::optional<civ5::graphics::GlTexture> live_title_font_texture;
    std::shared_ptr<const civ5::ui::BnwTechAwardInputDispatch> pending_tech_award_input;
    std::optional<civ5::ui::LiveUiFrame> live_ui_frame;
    std::optional<civ5::ui::ComposedUiFrame> live_screen_frame;
    std::optional<civ5::ui::MeasuredReadonlyTextReport> last_readonly_text_report;
    std::optional<std::string> last_readonly_tooltip_text;
    civ5::ui::CivilopediaSearchEditor search_editor;
    civ5::ui::PopupNameEditor popup_name_editor;
    std::ofstream live_screen_report;
    struct ResearchClickObservation {
        civ5::ui::BnwClientFrameStamp before;
        std::uint64_t ui_sequence=0,ui_snapshot=0,generation=0,dispatch_sequence=0;
        civ5::ui::ProgressionChooseTechnology command;
        std::string context;
        civ5::ui::WorldViewRect bounds;
        std::size_t hit_index=0;
        PreviewInput::LeftWindowClick mouse;
        std::vector<std::int64_t> before_queue;
        std::shared_ptr<const civ5::runtime::BnwResearchChoiceCommit> receipt;
    };
    std::uint64_t composed_click_dispatch_sequence=0;
    std::optional<ResearchClickObservation> research_click;
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
    std::unique_ptr<civ5::tests::BnwResearchChoiceRestartFixture> research_restart_fixture;
    bool research_restart_verified=false;
#endif
    const auto research_json=[](std::string_view value){
        std::ostringstream text;text << '"';
        for(const unsigned char c:value){
            if(c=='"'||c=='\\')text << '\\' << static_cast<char>(c);
            else if(c<32)text << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << unsigned(c) << std::dec;
            else text << static_cast<char>(c);
        }text << '"';return text.str();
    };
    const auto research_stamp_json=[&](const civ5::ui::BnwClientFrameStamp& stamp,
        std::uint64_t ui_sequence,std::uint64_t ui_snapshot,std::uint64_t generation,
        const std::vector<std::int64_t>& actual_queue){
        std::ostringstream text;const auto& id=stamp.identity;
        text << "{\"frameOwner\":" << id.owner << ",\"frameRevision\":" << id.revision
             << ",\"sceneFrame\":" << id.frame << ",\"frameSequence\":" << ui_sequence
             << ",\"snapshotFrame\":" << ui_snapshot << ",\"uiSourceRevision\":" << id.ui_source_revision
             << ",\"selectedOwner\":" << research_json(id.session) << ",\"generation\":" << generation
             << ",\"player\":" << id.player << ",\"team\":" << id.team << ",\"turn\":" << id.turn
             << ",\"mapGuidBytes\":[";
        for(std::size_t n=0;n<id.map_guid.size();++n){if(n)text << ',';text << unsigned(id.map_guid[n]);}
        text << "],\"queue\":[";
        for(std::size_t n=0;n<actual_queue.size();++n){if(n)text << ',';text << actual_queue[n];}
        text << "]}";return text.str();
    };
    std::function<civ5::ui::UiFrameSnapshot(const civ5::ui::UiFrameSnapshot &)>
        project_live_cities;
    std::vector<civ5::ui::LiveCityBannerProjectionAudit> projected_city_audit;
    std::shared_ptr<const civ5::ui::TopPanelLocalizationCatalog> live_text_catalog;
    if (live_bnw_preview) {
        live_registry.emplace(civ5::ui::UiContextRegistry::from_resources(
            *ui_resources));
        live_shared_screens.emplace(*live_registry);
        live_economic_scene.emplace(live_bnw_preview->session,
            live_bnw_preview->active_player_id, *live_shared_screens,
            *ui_resources);
        live_text_catalog = std::make_shared<civ5::ui::TopPanelLocalizationCatalog>(
            civ5::ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
                *ui_resources, language_root));
        auto input_with_city_text = *live_bnw_preview;
        if(input_with_city_text.frame_sources.unit_stats_database &&
            input_with_city_text.frame_sources.unit_stats_types && input_with_city_text.frame_sources.unit_stats_identity)
            civ5::ui::install_bnw_unit_panel_stat_rows_feed(input_with_city_text.frame_sources,
                input_with_city_text.session,input_with_city_text.active_player_id,live_text_catalog,
                russian_ui ? "ru_RU" : "en_US");
        if (input_with_city_text.pregame_roster && input_with_city_text.frame_sources.database) {
            struct AppearanceBinding {
                std::uint64_t generation;
                std::shared_ptr<civ5::ui::BnwCityBannerAppearanceOwner> owner;
            };
            auto appearance=std::make_shared<AppearanceBinding>(AppearanceBinding{
                input_with_city_text.session.state_generation(),
                std::make_shared<civ5::ui::BnwCityBannerAppearanceOwner>(
                    input_with_city_text.session,*input_with_city_text.pregame_roster,
                    *input_with_city_text.frame_sources.database,*ui_resources)});
            auto *roster=input_with_city_text.pregame_roster;
            auto *database=input_with_city_text.frame_sources.database;
            auto *expected_session=&input_with_city_text.session;
            input_with_city_text.frame_sources.city_banner_appearance=
                [appearance,expected_session,roster,database,ui_resources](const auto &session, auto active, auto player,
                    auto city, auto frame) -> std::optional<civ5::ui::BnwCityBannerAppearance> {
                    if (&session!=expected_session) return std::nullopt;
                    if (appearance->generation!=session.state_generation()) {
                        auto replacement=std::make_shared<civ5::ui::BnwCityBannerAppearanceOwner>(
                            session,*roster,*database,*ui_resources);
                        appearance->owner=std::move(replacement);
                        appearance->generation=session.state_generation();
                    }
                    return appearance->owner->capture(active,player,city,frame);
                };
            input_with_city_text.frame_sources.city_banner_appearance_current=
                [appearance,expected_session](const auto &session,const auto &receipt) {
                    return &session==expected_session && appearance->generation==session.state_generation() &&
                        appearance->owner->matches(receipt);
                };
        }
        input_with_city_text.frame_sources.localize_city =
            [live_text_catalog](std::string_view key) -> std::optional<std::string> {
                const auto found = live_text_catalog->rows.find(std::string(key));
                return found == live_text_catalog->rows.end() ? std::nullopt
                    : std::optional<std::string>(found->second);
            };
        input_with_city_text.frame_sources.localize_info =
            [live_text_catalog](std::string_view key) -> std::string {
                const auto found = live_text_catalog->rows.find(std::string(key));
                return found == live_text_catalog->rows.end()
                    ? std::string(key) : civ5::ui::bnw_info_default_form(*live_text_catalog,key);
            };
        civ5::ui::LiveSessionUiSources economic_sources;
        if (!live_economic_scene->install_popup_feed(
                economic_sources, live_economic_feed_blocker)) {
            live_economic_scene.reset();
            live_shared_screens.reset();
        } else {
            live_other_popup_feed = input_with_city_text.frame_sources.active_popups;
            input_with_city_text.frame_sources.active_popups =
                civ5::validation::compose_popup_feeds(
                    live_other_popup_feed,
                    std::move(economic_sources.active_popups),
                    live_popup_mux_status);
        }
        if (live_shared_screens) {
            live_tech_tree_lifecycle = std::make_shared<civ5::ui::BnwTechTreeLifecycle>(
                input_with_city_text.session, input_with_city_text.active_player_id,
                *live_shared_screens, *ui_resources,
                [&live_screen_report](const civ5::ui::BnwTechTreeEffectObservation &observed) {
                    live_screen_report << "tech-tree local-effect=" << static_cast<int>(observed.effect)
                        << " player=" << observed.occurrence.player_id << " turn=" << observed.occurrence.turn
                        << " civis-occurrence=" << observed.occurrence.token
                        << " semaphore=" << observed.semaphore
                        << " Data2=" << observed.data2 << " ProcessedResult=" << observed.processed_result
                        << " nativeVM=false RefreshDisplay=false\n";
                });
            if (ui_source_observation)
                blend_validation::text(output / "tech-tree-source-admission.json",
                    "{\"schema\":\"civis-tech-tree-source-admission-v1\",\"status\":\"COMPLETED\","
                    "\"tech_tree_constructed\":true,\"ui_sources\":" +
                    ui_source_observation->snapshot_json(
                        ui_source_observer && ui_source_observer->failed()) + "}\n");
            auto previous_augment = input_with_city_text.frame_sources.augment_snapshot;
            input_with_city_text.frame_sources.augment_snapshot =
                [weak = std::weak_ptr<civ5::ui::BnwTechTreeLifecycle>(live_tech_tree_lifecycle),
                 previous = std::move(previous_augment)](const auto &session, civ5::ui::UiFrameSnapshot frame) {
                    const auto owner = weak.lock();
                    if (!owner) throw std::logic_error("TechTree lifecycle retired before UI publication");
                    if (previous) frame = previous(session, std::move(frame));
                    return owner->augment_snapshot(session, frame);
                };
        } else if (ui_source_observation) {
            blend_validation::text(output / "tech-tree-source-admission.json",
                "{\"schema\":\"civis-tech-tree-source-admission-v1\",\"status\":\"SKIPPED_NOT_ACCEPTED\","
                "\"tech_tree_constructed\":false,\"ui_sources\":null}\n");
        }
        if(worldview_font_source) {
            check(worldview_font_source->locale().language==ui_language && live_text_catalog &&
                  &input_with_city_text.session==&font_source_parent->session(),
                  "WorldView source quote differs from actual camera localization/session");
            // AFTER real localize_info extraction and preserved TechTree/base augmentation.
            auto previous=input_with_city_text.frame_sources.augment_snapshot;
            input_with_city_text.frame_sources.augment_snapshot=
                [weak=std::weak_ptr<civ5::ui::BnwWorldViewFontSourceScope>(worldview_font_source),
                 previous=std::move(previous),expected=&input_with_city_text.session]
                (const auto& session,civ5::ui::UiFrameSnapshot frame) {
                    check(&session==expected,"WorldView source augmentation foreign session");
                    const auto source=weak.lock();
                    check(static_cast<bool>(source),"WorldView font camera source retired");
                    const auto base=frame; // Original owners must survive earlier augmenters too.
                    check(civ5::ui::ui_owners_current(base),"WorldView base UI owners stale before augmentation");
                    if(previous) frame=previous(session,std::move(frame));
                    check(civ5::ui::ui_owners_current(base),"WorldView base UI owners changed during augmentation");
                    frame=source->augment_snapshot(std::move(frame));
                    check(civ5::ui::ui_owners_current(base) && civ5::ui::ui_owners_current(frame),
                          "WorldView UI source owners changed before publication");
                    return frame;
                };
        }
        const auto external_city_selection = input_with_city_text.frame_sources.selected_city;
        input_with_city_text.frame_sources.selected_city =
            [&live_ui_host, external_city_selection,
             player = input_with_city_text.active_player_id]() -> std::optional<std::int64_t> {
                if (!live_ui_host)
                    return external_city_selection ? external_city_selection() : std::nullopt;
                const auto selected = live_ui_host->selected_city();
                if (!selected) return std::nullopt;
                check(selected->first == player, "UI city selection belongs to another player");
                return selected->second;
            };
        if(font_source_parent) {
            // Authenticate source payload/player in the existing scene owner;
            // no frame identity rewrite or second FrameOwner is installed.
            live_client_scene=civ5::ui::BnwClientSceneFrame::create_selected_screen_scene(
                font_source_parent,*live_scene_receipt,input_with_city_text.frame_sources);
            live_pending_unit_visuals=civ5::graphics::BnwPendingUnitVisualOwner::attach_to_selected_scene(live_client_scene);
            presenter.pending_visual_receiver=live_pending_unit_visuals;
            presenter.pending_visual_world=world;
            presenter.pending_visual_paint_lifetime=paint_run_lifetime;
            presenter.center_unit_scene=live_client_scene;
        } else live_client_scene=std::make_shared<civ5::ui::BnwClientSceneFrame>(
            input_with_city_text.session,*live_scene_receipt,
            input_with_city_text.active_player_id,input_with_city_text.frame_sources);
        if (live_minimap_surface) {
            check(live_minimap_initial_grid && live_minimap_geometry,
                  "Live minimap lost its pristine source grid/geometry");
            live_minimap_preparation = std::make_unique<civ5::ui::BnwMinimapFogPreparationOwner>(
                input_with_city_text.session, live_client_scene->owner(), *live_scene_receipt,
                std::move(*live_minimap_initial_grid), *live_minimap_geometry);
            live_minimap_initial_grid.reset();
        }
        live_bnw_binding.emplace(civ5::ui::bind_live_bnw_preview(
            std::move(input_with_city_text)));
        live_ui_provider = live_client_scene->ui_provider();
        live_world_frame_current = [&] {
            RECT actual{};
            return same_extent_candidate_current() && GetClientRect(window, &actual) &&
                actual.right == client_width && actual.bottom == client_height &&
                (!live_fog_client || live_fog_client->valid_handoff()) &&
                live_client_scene->current() &&
                live_client_scene->frame()->stamp.viewport.dpi == preview_window_dpi(window) &&
                live_camera_current && live_camera_current();
        };
        presenter.paint_frame_current = [&] {
            return live_world_frame_current() && (!live_minimap_current || live_minimap_current()) &&
                same_extent_candidate_current();
        };
        presenter.client_frame_current = [&] {
            return viewport_transaction && published_viewport && published_viewport_gate &&
                viewport_transaction->current(*published_viewport,published_viewport_gate);
        };
        if (live_client_capture)
            presenter.capture_client_frame = [&](std::int32_t w, std::int32_t h) {
                if (ui_source_observer && ui_source_observer->failed())
                    ui_source_observation->mark_observer_failed();
                live_client_capture(live_client_scene->owner(), live_client_scene->frame(),
                    live_client_scene->receipt(), w, h,
                    live_screen_frame && live_screen_frame->focused_context
                        ? std::string_view(*live_screen_frame->focused_context)
                        : live_ui_frame && live_ui_frame->world_view
                            ? std::string_view("WorldView") : std::string_view{},
                    live_material_time, live_client_scene->fog_receipt(),
                    [&] {
                        check(presenter.client_frame_current && presenter.client_frame_current(),
                              "BNW viewport or camera changed before readback");
                        auto pixels = read_preview_frame_pixels(*context, w, h);
                        check(presenter.client_frame_current && presenter.client_frame_current(),
                              "BNW viewport or camera changed during readback");
                        return pixels;
                    });
            };
        // The standalone party consumer permits source-owned navigation only.
        // An absent gameplay backend leaves every turn/train/research action unavailable.
        live_ui_backend = prepared_viewer->existing_menu_binding ? nullptr : live_bnw_binding->backend.get();
        if(prepared_viewer->stock_source_owner && !prepared_viewer->existing_menu_binding){
            check(live_client_scene!=nullptr,"Research backend needs the actual selected Scene");
            live_alpha_research_backend=prepared_viewer->stock_source_owner->create_alpha_progression_backend(live_client_scene);
            check(live_alpha_research_backend!=nullptr,"Canonical stock research backend absent");
            live_ui_backend=live_alpha_research_backend.get();
            // Same canonical stock owner supplies actual Host/session directly.
            // Research stays the general backend; production is its real own lane.
            live_selected_production_services=prepared_viewer->stock_source_owner->create_selected_city_production_services();
            check(live_selected_production_services!=nullptr,"Canonical selected production services absent");
            live_selected_production_backend=live_selected_production_services->make_backend();
            check(live_selected_production_backend!=nullptr,"Canonical selected production backend absent");
        }
    }
    if (live_ui_provider) {
        auto registry = live_registry ? *live_registry :
            civ5::ui::UiContextRegistry::from_resources(*ui_resources);
        live_world_skin.emplace(civ5::ui::WorldViewSkin::from_resources(*ui_resources, registry));
        const auto pedia_localization = live_text_catalog ? *live_text_catalog :
            civ5::ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
                *ui_resources, language_root);
        live_pedia_entries = civ5::ui::read_civilopedia_entries(*ui_resources,
            [&pedia_localization](std::string_view key) -> std::optional<std::string> {
                const auto it = pedia_localization.rows.find(std::string(key));
                if (it == pedia_localization.rows.end()) return std::nullopt;
                return it->second;
            });
        civ5::ui::LiveUiSources live_sources;
        if(!prepared_viewer->existing_menu_binding && font_source_parent&&prepared_viewer->renderer_window_owner&&prepared_viewer->viewport_transaction){
            const auto weak=std::weak_ptr<PreparedCameraViewer>(prepared_viewer);
            // The captured source survives from preparation through actual
            // first WM_ACTIVATE. The typed overload keeps the own Bank and
            // genuine NativeParent lanes disjoint; neither mints the other.
            const auto install_actual_audio=[&](const auto& audio_resource){
                prepared_viewer->audio_window_input=[weak,audio_resource](const civ5::ui::ClientWindowEvent& event){
                    const auto actual=weak.lock();if(!actual||event.kind!=civ5::ui::ClientWindowEventKind::activation||!event.activation)return;
                    try{
                        if(actual->client_audio)actual->client_audio->consume_actual_activation(*event.activation);
                        else{
                            check(actual->client_audio_error.empty(),"Retired Civis audio startup cannot silently restart");
                            actual->viewport_transaction->require_activation_window_owner(*event.activation);
                            actual->renderer_window_owner->with_actual_window_loan([&](const auto& physical){
                                actual->client_audio=civ5::ui::CivisClientAudioQueueOwner::from_first_actual_activation(audio_resource,*event.activation,physical);
                            });
                        }
                    }catch(const std::exception& error){actual->client_audio_error=error.what();actual->retire_client_audio();}
                };
            };
            if(font_source_parent->uses_civis_resource_bank()){
                // Actual same canonical Host/Bank XML, SoundData, settings and
                // admission WAV are prepared here, BEFORE ShowWindow below.
                install_actual_audio(civ5::ui::CivisBankAudioSourceOwner::prepare_from_selected_host(font_source_parent));
            }else{
                // The genuine Host supplies its adopted early Parent; its
                // existing canonical native Bank is borrowed unchanged.
                install_actual_audio(civ5::ui::BnwAudioCompletedResourceOwner::from_selected_host(font_source_parent));
            }
            live_sources.civis_audio_transition=[weak](std::uint64_t sequence,civ5::ui::UiAudioKind kind,std::string_view id){
                const auto actual=weak.lock();if(!actual||!actual->client_audio)return civ5::ui::LiveUiAudioResult{};
                try{return actual->client_audio->transition(sequence,kind,id);}
                catch(const std::exception& error){actual->client_audio_error=error.what();actual->retire_client_audio();return civ5::ui::LiveUiAudioResult{};}
            };
        }else if(font_source_parent&&font_source_parent->uses_civis_resource_bank()){
            check(false,"Own Bank client audio requires the actual viewport and physical renderer window owner");
        }
        live_sources.world_skin = &*live_world_skin;
        live_sources.pedia_entries = &live_pedia_entries;
        if (live_tech_tree_lifecycle) {
            live_sources.progression[static_cast<std::size_t>(civ5::ui::ProgressionScreen::technologies)] =
                &live_tech_tree_lifecycle->source();
            const auto weak = std::weak_ptr<civ5::ui::BnwTechTreeLifecycle>(live_tech_tree_lifecycle);
            const auto *session = &live_bnw_preview->session;
            live_sources.active_tech_tree_event = [weak, session] {
                const auto owner = weak.lock();
                return owner ? owner->active_event(*session) : std::optional<civ5::ui::ProgressionTechnologyTreeEvent>{};
            };
            live_sources.tech_tree_processed = [weak](const auto &event) {
                const auto owner = weak.lock();
                return owner ? owner->process(event) : civ5::ui::UiCommandResult{
                    civ5::ui::UiCommandStatus::unavailable, "TechTree local lifecycle owner retired"};
            };
            live_sources.tech_tree_local_dequeue_observed = [weak](const auto &event) {
                const auto owner = weak.lock();
                return owner && owner->local_dequeue_observed(event);
            };
        }
        if (live_bnw_preview) {
            live_sources.top_panel_layout = &presenter.top_panel_layout;
            live_sources.top_panel_localizer.text = [&presenter](
                std::string_view key, const std::vector<std::string> &args) {
                return presenter.localization.format(key, args);
            };
        }
        live_compositor.emplace(*ui_resources, registry, live_ui_backend,
            [&presenter](std::string_view key) -> std::optional<std::string> {
                const auto it = presenter.localization.rows.find(std::string(key));
                return it == presenter.localization.rows.end() ? std::nullopt
                    : std::optional<std::string>(it->second);
            });
        live_sources.production_backend = live_selected_production_backend
            ? live_selected_production_backend.get()
            : dynamic_cast<civ5::ui::BnwCityProductionUiBackend *>(live_ui_backend);
        live_sources.production_popup_source =
            &live_compositor->production_popup_source();
        if (live_bnw_preview) {
            live_ui_provider = [source = std::move(live_ui_provider),
                                &project_live_cities]() mutable {
                auto frame = source();
                return project_live_cities ? project_live_cities(frame) : frame;
            };
        }
        if (font_source_parent && live_client_scene) {
            if(live_shared_screens)
                live_ui_host=live_client_scene->create_selected_ui_host(*live_shared_screens,
                    std::move(live_ui_provider),live_ui_backend,std::move(live_sources));
            else
                live_ui_host=live_client_scene->create_selected_ui_host(registry,
                    std::move(live_ui_provider),live_ui_backend,std::move(live_sources));
        } else if (live_shared_screens)
            live_ui_host=std::make_shared<civ5::ui::LiveUiHost>(*live_shared_screens,
                std::move(live_ui_provider), live_ui_backend, std::move(live_sources));
        else
            live_ui_host=std::make_shared<civ5::ui::LiveUiHost>(registry,std::move(live_ui_provider),
                live_ui_backend,std::move(live_sources));
        live_trade_tab_proof.emplace(
            civ5::ui::inspect_trade_route_tab_source(*ui_resources));
        live_trade_tabs.emplace(*live_trade_tab_proof);
        // The active UnitPanel label starts with TwCenMT24. Bind its authored
        // GlyphGen Base layer instead of silently painting the 20-pixel atlas.
        // Retain the Base binding for plain title fixtures. Authored title
        // SoftShadow below uses the same style set as the other current labels.
        const auto title_font = civ5::ui::read_glyphgen(
            *ui_resources, "Assets/UI/Fonts/Tw Cent MT/TwCenMT24.ggxml");
        const auto title_base = std::find_if(title_font.textures.begin(), title_font.textures.end(),
            [](const auto &texture) { return texture.name() == "TwCenMT24Base"; });
        check(title_base != title_font.textures.end(), "TwCenMT24 Base texture declaration is missing");
        const auto title_asset = ui_find(civ5::assets::AssetKey(title_base->source()));
        const auto title_index_asset = ui_find(civ5::assets::AssetKey(
            title_base->source().substr(0, title_base->source().size() - 4) + "-index.dds"));
        check(std::holds_alternative<civ5::assets::AssetHandle>(title_asset) &&
                  std::holds_alternative<civ5::assets::AssetHandle>(title_index_asset),
              "TwCenMT24 GlyphGen Base assets are unavailable or ambiguous");
        const auto title_data = civ5::assets::DdsData::parse(
            std::get<civ5::assets::AssetHandle>(title_asset).read_bytes());
        const auto title_index_data = civ5::assets::DdsData::parse(
            std::get<civ5::assets::AssetHandle>(title_index_asset).read_bytes());
        live_title_font.emplace(prepare_preview_alpha_font(
            title_font, title_index_data, title_data, pedia_localization, extended_face,
            russian_ui ? 512u : 0u));
        live_title_font_texture.emplace(civ5::graphics::GlTexture::alpha8(
            *context, live_title_font->atlas.width, live_title_font->atlas.height,
            live_title_font->atlas.alpha));
        auto banner_texts = pedia_localization;
        std::size_t banner_text_index = 0;
        if (live_bnw_preview && live_text_catalog) {
        const auto &banner_state = live_bnw_preview->session.state();
        for (const auto &[owner, player_cities] : banner_state.cities.players) {
            (void)player_cities;
            const auto names = civ5::ui::capture_city_screen_frame(
                banner_state.cities, banner_state.world, owner, std::nullopt,
                [live_text_catalog](std::string_view key) -> std::optional<std::string> {
                    const auto found = live_text_catalog->rows.find(std::string(key));
                    return found == live_text_catalog->rows.end() ? std::nullopt
                        : std::optional<std::string>(found->second);
                });
            for (const auto &city : names.cities)
                if (city.name) banner_texts.rows.insert_or_assign(
                    "__civis_owned_city_name_" + std::to_string(banner_text_index++), *city.name);
        }
        }
        std::vector<civ5::ui::PreparedUiGlyphStyle> banner_styles;
        for (const auto &[font_name, style_name] :
             std::array<std::pair<std::string_view, std::string_view>, 6>{
                 {{"TwCenMT18", "Shadow"}, {"TwCenMT20", "Stroke"}, {"TwCenMT20", "Shadow"},
                  {"TwCenMT24", "SoftShadow"}, {"TwCenMT20", "SoftShadow"},
                  {"TwCenMT14", "SoftShadow"}}}) {
            auto authored = civ5::ui::load_glyphgen_style(*ui_resources, font_name, style_name);
            auto prepared = prepare_preview_alpha_font(authored.font, authored.base_index,
                authored.base, banner_texts, extended_face, russian_ui ? 512u : 0u);
            banner_styles.push_back({std::string(font_name), std::string(style_name),
                std::move(authored), std::move(prepared.atlas), std::move(prepared.geometry)});
        }
        live_font_styles = std::make_shared<civ5::ui::UiGlyphGenStyleSet>(
            *context, std::move(banner_styles));
        live_screen_painter.emplace(*context, *ui_resources,
            civ5::ui::UiGlyphBinding{&presenter.top_panel_geometry,
                                     &presenter.top_panel_font_texture},
            civ5::ui::UiGlyphBinding{&presenter.right_panel_geometry,
                                     &presenter.right_panel_font_texture},
            civ5::ui::UiGlyphBinding{&presenter.icon_geometry,
                                     &presenter.icon_font_texture},
            [&presenter](std::string_view name) -> std::optional<std::uint32_t> {
                const auto found = presenter.icon_mapping.find(std::string(name));
                return found == presenter.icon_mapping.end()
                    ? std::nullopt : std::optional<std::uint32_t>(found->second);
            },
            civ5::ui::UiGlyphBinding{&live_title_font->geometry,
                                     &*live_title_font_texture},
            [styles = live_font_styles](std::string_view font, std::string_view style) {
                return styles->resolve(font, style);
            });
        presenter.tech_award_presentation_target=live_screen_painter->tech_award_presentation_target();
        if(live_client_scene&&live_ui_host&&prepared_viewer->renderer_window_owner)
            tech_award_update_sample=live_screen_painter->begin_tech_award_ui_update(
                live_client_scene,live_ui_host,prepared_viewer->renderer_window_owner);
        if (live_bnw_preview->frame_sources.database) {
            const auto active_types=
                live_bnw_preview->frame_sources.database->cache_info_types();
            natural_wonder_anchors=std::make_shared<
                civ5::graphics::GlBnwNaturalWonderAnchorBackend>(
                    live_bnw_preview->session.state(),context,
                    *live_bnw_preview->frame_sources.database,
                    active_types,*ui_resources);
            world_anchor_scene=std::make_shared<
                civ5::graphics::GlBnwWorldAnchorScene>(
                    live_bnw_preview->session.state(),context,
                    natural_wonder_anchors);
        }
        live_screen_report.open(output / "ui-screen-render.txt");
        if (!live_economic_feed_blocker.empty())
            live_screen_report << "economic feed blocked=" <<
                live_economic_feed_blocker << '\n';
        presenter.prepare_live_screens = [&](std::int32_t w, std::int32_t h) {
            auto presented = civ5::ui::present_live_ui_frame(*live_ui_host,
                *live_compositor, w, h,
                [w, h](const civ5::ui::LiveUiFrame &frame,
                       const civ5::ui::CityScreenSource &source) {
                    return civ5::ui::measure_civis_production_popup(frame,
                        source, w, h);
                });
            live_ui_frame = std::move(presented.live);
            live_screen_frame = std::move(presented.composed);
            if(pending_tech_award_input&&pending_tech_award_input->phase()==
                civ5::ui::BnwTechAwardInputPhase::pending_frame) {
                check(live_client_scene&&live_client_scene->accepts(*live_ui_host,*live_ui_frame),
                      "TechAward closed frame lacks actual Scene/UI publication");
                live_ui_host->complete_tech_award_input_after_frame(pending_tech_award_input,*live_ui_frame);
                check(pending_tech_award_input->phase()==civ5::ui::BnwTechAwardInputPhase::completed,
                      "TechAward input did not consume the actual new frame");
                // Retain through actual paint/present and their postchecks.
            }
            if (!live_popup_mux_status->blocker.empty())
                live_screen_report << "popup feed blocked=" <<
                    live_popup_mux_status->blocker << '\n';
            if (live_economic_scene && live_bnw_preview &&
                live_ui_host->screens().focused_screen() ==
                    std::optional<std::string>{"EconomicOverview"}) {
                civ5::ui::EconomicBoundFields fields;
                const auto owner = live_economic_scene->current_owner(
                    live_bnw_preview->session, *live_ui_frame, w, h);
                if (owner) {
                    const auto rects = civ5::validation::measure_economic_overview_buttons(
                        *ui_resources, live_economic_scene->tab_source(), *owner);
                    if (rects) {
                        std::string blocker;
                        if (live_economic_scene->bind_popup(
                                live_bnw_preview->session, *live_ui_frame,
                                *rects, civ5::validation::live_happiness_enabled(
                                    live_bnw_preview->session), blocker))
                            fields = live_economic_scene->fields(
                                live_bnw_preview->session, *live_ui_frame, w, h);
                        else live_screen_report <<
                            "economic bind blocked=" << blocker << '\n';
                    } else live_screen_report <<
                        "economic bind blocked=selected XML geometry unavailable\n";
                } else live_screen_report <<
                    "economic bind blocked=current popup owner unavailable\n";
                live_screen_frame = live_compositor->compose(*live_ui_frame,
                    live_ui_host->screens(), w, h, nullptr, &fields);
                live_screen_report << "economic fields available=" <<
                    fields.available << " count=" << fields.fields.size() << '\n';
            }
            if (live_trade_tabs && live_trade_tab_proof &&
                live_ui_host->screens().focused_screen() ==
                    std::optional<std::string>{"TradeRouteOverview"}) {
                const auto *popup = live_ui_frame->snapshot.find<civ5::ui::PopupFrame>();
                if (popup && popup->events.size() == 1 &&
                    popup->events.front().context_id == "TradeRouteOverview") {
                    civ5::ui::TradeRouteTabOwner owner;
                    owner.player_id = popup->player_id;
                    owner.turn = popup->turn;
                    owner.owner_sequence = popup->owner_sequence;
                    owner.event_id = popup->events.front().event_id;
                    owner.frame = live_ui_frame->snapshot.frame();
                    owner.frame_owner = live_ui_frame->owner;
                    owner.frame_sequence = live_ui_frame->sequence;
                    owner.viewport_width = w;
                    owner.viewport_height = h;
                    // PopupFrame does not yet expose native Data2. Its initial
                    // tab remains unresolved until that owner value is carried.
                    owner.popup_data2 = std::nullopt;
                    owner.lua_sha256 = live_trade_tab_proof->lua_sha256;
                    owner.xml_sha256 = live_trade_tab_proof->xml_sha256;
                    if (auto rects = civ5::validation::measure_trade_route_tabs(
                            *ui_resources, *live_trade_tab_proof, owner)) {
                        std::string blocker;
                        if (!live_trade_tabs->bind(*live_ui_frame,
                                live_ui_host->screens(), *rects, blocker))
                            live_screen_report << "trade-tab bind blocked=" <<
                                blocker << '\n';
                    } else {
                        live_screen_report <<
                            "trade-tab bind blocked=selected XML tab geometry unavailable\n";
                    }
                }
            }
            if (live_bnw_preview) {
                check(live_ui_frame->top_panel.has_value(),
                      "Restored BNW frame has no TopPanel presentation");
                presenter.live_top_panel_presentation = live_ui_frame->top_panel;
            }
            const auto *focused = live_screen_frame->focused_context
                ? live_ui_host->screens().registry().find(*live_screen_frame->focused_context)
                : nullptr;
            input.live_modal_open = focused && focused->modal;
            const auto publish_input_hits = [&] {
                std::vector<civ5::ui::ClientRect> hits{{0, 0, w, 32}};
                for (const auto &r : input.live_hit_regions)
                    hits.push_back({r.x, r.y, r.width, r.height});
                if (input.minimap_hit_panel) {
                    const auto &p = *input.minimap_hit_panel;
                    const auto append_rect = [&](const civ5::ui::MinimapRect &r) {
                        check(r.width <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) &&
                              r.height <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()),
                              "Native minimap hit dimensions exceed client coordinate range");
                        hits.push_back({r.x, r.y, static_cast<std::int32_t>(r.width),
                                       static_cast<std::int32_t>(r.height)});
                    };
                    for (const auto &r : {p.map, p.strategic, p.options}) append_rect(r);
                    if (input.minimap_options_visible) append_rect(p.options_panel);
                }
                if(viewport_transaction) candidate_hits=std::move(hits);
                else if (input.router.set_ui(hits, input.live_modal_open, input.search_focus))
                    input.cancel = true;
            };
            input.live_hit_regions.clear();
            input.live_hit_regions.reserve(live_screen_frame->hits.size() +
                (live_ui_frame->world_view ? live_ui_frame->world_view->hits.size() : 0));
            for (const auto &hit : live_screen_frame->hits)
                input.live_hit_regions.push_back(hit.bounds);
            if (live_ui_frame->world_view)
                for (const auto &hit : live_ui_frame->world_view->hits)
                    input.live_hit_regions.push_back(hit.bounds);
            if (live_screen_frame->civilopedia_article_scroll_panel)
                input.live_hit_regions.push_back(
                    *live_screen_frame->civilopedia_article_scroll_panel);
            if (live_screen_frame->popup_scroll_panel)
                input.live_hit_regions.push_back(*live_screen_frame->popup_scroll_panel);
            if (live_screen_frame->production_scroll_panel)
                input.live_hit_regions.push_back(*live_screen_frame->production_scroll_panel);
            publish_input_hits();
            if(prepared_viewer->stock_source_owner){
                check(live_client_scene&&prepared_viewer->renderer_window_owner,
                      "Stock GUI requires actual selected Scene and canonical physical owner");
                // Actual post-command handoff: Scene.begin captured the earned
                // same-world revision, present_live_ui_frame captured its selected
                // city/queue and compositor now issues the real new GUI. Never
                // acknowledge the stale pre-command GUI/frame or synthesize a receipt.
                prepared_viewer->retire_gui_screen();
                auto gui=live_compositor->compose_owned_current_screen(live_client_scene,
                    live_ui_host,prepared_viewer->renderer_window_owner,*live_ui_frame);
                prepared_viewer->gui_screen=gui;
                live_compositor->bind_owned_research_renderer_publication(gui,
                    std::weak_ptr<const std::uint8_t>(paint_run_lifetime),
                    [&,weak=std::weak_ptr<const std::uint8_t>(paint_run_lifetime)](
                        const std::shared_ptr<const civ5::ui::BnwOwnedCameraGuiScreen>& next,
                        const civ5::ui::LiveUiFrame& actual_ui,const civ5::ui::ComposedUiFrame& actual_layout){
                        check(!weak.expired(),"Research renderer publication target retired");
                        next->require_current();
                        live_ui_frame=actual_ui;live_screen_frame=actual_layout;
                        presenter.live_top_panel_presentation=live_ui_frame->top_panel;
                        prepared_viewer->gui_screen=next;
                        check(!weak.expired(),"Research renderer retired during slot publication");
                        next->require_current();
                    });
                presenter.with_first_presented_screen =
                    [&,weak=std::weak_ptr<const std::uint8_t>(paint_run_lifetime)](
                        const std::function<void(const civ5::ui::BnwOwnedCameraGuiScreen::Loan&)>& target){
                        check(!weak.expired(),"First presented screen lender retired");
                        check(prepared_viewer->gui_screen!=nullptr,"First presented screen owner absent");
                        // Copy observation values before existing source callbacks; no
                        // second frame owner, capability, or retained source lifetime.
                        presenter.first_presented_frame_stamp.reset();
                        struct ClearObservedStamp {
                            std::optional<civ5::ui::BnwClientFrameStamp>& value;
                            ~ClearObservedStamp() noexcept {value.reset();}
                        } clear_observed_stamp{presenter.first_presented_frame_stamp};
                        {
                            // Restore diagnostic-copy errors BEFORE the existing loan,
                            // present and source callbacks; never cover that old flow.
                            const FirstPresentedObservationErrors restore_observation_errors;
                            try {
                                if(live_client_scene && live_client_scene->frame())
                                    presenter.first_presented_frame_stamp=live_client_scene->frame()->stamp;
                            } catch(...) {
                                presenter.first_presented_frame_stamp.reset();
                                camera_stage_mark("first_capture_metadata","STAMP_UNAVAILABLE");
                            }
                        }
                        // Borrow the actual renderer-held canonical leaf; no extra strong weak.lock pin.
                        prepared_viewer->gui_screen->with_actual_screen_loan(target);
                        check(!weak.expired(),"First presented screen lender retired during capture");
                    };
                const auto stock=prepared_viewer->stock_source_owner;
                if(const auto receipt=stock->pending_train_renderer_refresh()){
                    receipt->require_pending_current();
                    live_train_renderer_refresh=receipt;
                    presenter.pending_train_presentation=receipt;presenter.pending_train_gui=gui;
                    presenter.complete_presented_train_refresh=
                        [&,stock,receipt,gui,weak=std::weak_ptr<const std::uint8_t>(paint_run_lifetime)]
                        (const civ5::graphics::BnwTrainPresentedFrameProof& proof){
                        check(!weak.expired()&&prepared_viewer->gui_screen.get()==gui.get()&&
                              !prepared_viewer->gui_screen.owner_before(gui)&&!gui.owner_before(prepared_viewer->gui_screen),
                              "TRAIN renderer source/GUI retired before completion");
                        check(live_client_scene&&live_ui_host&&live_ui_frame&&live_screen_frame&&
                              live_client_scene->accepts(*live_ui_host,*live_ui_frame)&&
                              live_screen_frame->frame_owner==live_ui_frame->owner&&
                              live_screen_frame->frame_sequence==live_ui_frame->sequence&&
                              live_screen_frame->snapshot_frame==live_ui_frame->snapshot.frame(),
                              "TRAIN actual presented Scene/UI/layout changed");
                        proof.require_current_for_train(*receipt);
                        stock->complete_train_renderer_refresh_after_present(receipt,proof);
                        check(!weak.expired(),"TRAIN renderer retired during genuine completion");
                        gui->require_current();
                    };
                    return; // Pending receipt forbids unrelated attach/advance serials before present.
                }
                // The action already owns a true pending receipt. Public stock
                // operations would allocate an unrelated serial before paint.
                if(live_alpha_research_backend && live_alpha_research_backend->phase()==
                    civ5::ui::BnwAlphaResearchUiPhase::pending_refresh){
                    const auto commit=live_alpha_research_backend->last_commit();
                    check(commit!=nullptr,"Pending research lost its canonical commit");
                    gui->require_same_research_choice(*commit);return;
                }
                stock->attach_stock_gui_screen(gui);gui->require_current();
                // Bounded real SAME-coroutine progress through the proven
                // getters. Pending native registry is not loading completion.
                for(unsigned step=0;step<4;++step){
                    gui->require_current();
                    const auto progress=stock->advance_stock_bootstrap();
                    gui->require_current();
                    check(!progress.loading_complete,"Partial stock GUI must not claim completed loading");
                    if(progress.pending_api=="Controls.ScrollBar.SetSize")break;
                }
                gui->require_current();
            }
        };
        presenter.complete_presented_research_refresh = [&,weak=std::weak_ptr<const std::uint8_t>(paint_run_lifetime)] {
            check(!weak.expired(),"Research presentation target retired");
            if(!output.empty()&&font_source_parent&&live_client_scene&&live_ui_host&&live_ui_frame&&live_screen_frame&&prepared_viewer->gui_screen){
                const auto actual_frame=live_client_scene->frame();
                prepared_viewer->gui_screen->require_current();
                check(actual_frame&&live_client_scene->accepts(*live_ui_host,*live_ui_frame),"Presented research hit source stale");
                const auto* research=live_ui_frame->snapshot.find<civ5::ui::BnwResearchChoiceUiSnapshot>();
                check(research,"Presented research hit snapshot absent");
                std::ostringstream hits;
                hits << "{\"schema\":\"civis-presented-research-hitboxes-v1\",\"stage\":\"AFTER_PRESENT\",\"presentCount\":"
                     << presenter.completed_present_count << ",\"frame\":"
                     << research_stamp_json(actual_frame->stamp,live_ui_frame->sequence,live_ui_frame->snapshot.frame(),
                        font_source_parent->session().state_generation(),research->research_queue) << ",\"viewport\":["
                     << actual_frame->stamp.viewport.framebuffer_width << ',' << actual_frame->stamp.viewport.framebuffer_height
                     << "],\"hits\":[";
                bool first=true;
                if(presenter.painted_tech_navigation_hit){
                    const auto& hit=*presenter.painted_tech_navigation_hit;
                    hits << "{\"context\":\"TopPanel\",\"command\":\"TopPanelAction.open_technology_tree\","
                            "\"targetContext\":\"TechTree\",\"path\":\"painted-science-hit-dispatch_top_panel_action\",\"bounds\":["
                         << hit.x << ',' << hit.y << ',' << hit.width << ',' << hit.height << "]}";
                    first=false;
                }
                for(std::size_t n=0;n<live_screen_frame->hits.size();++n){
                    const auto& hit=live_screen_frame->hits[n];
                    const auto* gameplay=std::get_if<civ5::ui::UiGameplayCommand>(&hit.command);
                    const auto* choice=gameplay?gameplay->as<civ5::ui::ProgressionChooseTechnology>():nullptr;
                    const auto* open=std::get_if<civ5::ui::UiOpenScreen>(&hit.command);
                    if(!choice&&(!open||open->id!="TechTree"))continue;
                    if(!first)hits << ',';first=false;
                    hits << "{\"hitIndex\":" << n << ",\"context\":" << research_json(hit.context_id)
                         << ",\"command\":" << research_json(choice?"ProgressionChooseTechnology":"UiOpenScreen");
                    if(choice)hits << ",\"technology\":" << choice->technology << ",\"clearQueue\":" << (choice->clear_queue?"true":"false");
                    else hits << ",\"targetContext\":" << research_json(open->id);
                    hits << ",\"enabled\":" << (hit.enabled?"true":"false") << ",\"bounds\":["
                         << hit.bounds.x << ',' << hit.bounds.y << ',' << hit.bounds.width << ',' << hit.bounds.height << "]}";
                }
                hits << "]}\n";
                prepared_viewer->gui_screen->require_current();
                check(live_client_scene->frame()==actual_frame,"Research hit publication changed during formatting");
                blend_validation::text(output/"research-hitboxes.json",hits.str());
            }
            if(!live_alpha_research_backend || live_alpha_research_backend->phase()!=
                civ5::ui::BnwAlphaResearchUiPhase::pending_refresh)return;
            check(live_client_scene && live_ui_host && live_ui_frame && live_screen_frame &&
                  prepared_viewer->gui_screen,"Research presentation has no actual published screen");
            prepared_viewer->gui_screen->require_current();
            check(live_client_scene->accepts(*live_ui_host,*live_ui_frame) &&
                  live_ui_frame->owner==live_screen_frame->frame_owner &&
                  live_ui_frame->sequence==live_screen_frame->frame_sequence &&
                  live_ui_frame->snapshot.frame()==live_screen_frame->snapshot_frame,
                  "Research presented frame/layout identity differs");
            const auto result=live_alpha_research_backend->complete_after_refresh(
                *live_client_scene,*live_ui_host,*live_ui_frame);
            check(result.status==civ5::ui::UiCommandStatus::performed,
                  "Actually stored research did not complete its presented refresh");
            check(!weak.expired(),"Research presentation target retired during completion");
            prepared_viewer->gui_screen->require_current();
            if(!output.empty()&&research_click){
                const auto receipt=live_alpha_research_backend->last_commit();const auto& click=*research_click;
                check(receipt&&receipt.get()==click.receipt.get()&&!receipt.owner_before(click.receipt)&&
                      !click.receipt.owner_before(receipt),"Presented research receipt differs from actual mouse dispatch");
                const auto actual_frame=live_client_scene->frame();
                const auto actual_ui_sequence=live_ui_frame->sequence;
                const auto actual_ui_snapshot=live_ui_frame->snapshot.frame();
                check(actual_frame&&font_source_parent,"Research receipt actual source storage absent");
                receipt->require_stored_current(*font_source_parent);
                check(receipt->phase()==civ5::runtime::BnwResearchChoicePhase::complete&&
                      live_alpha_research_backend->phase()==civ5::ui::BnwAlphaResearchUiPhase::complete,
                      "Research receipt has no actually completed commit/backend");
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                if(research_restart_fixture&&!research_restart_verified){
                    check(restart_preview,"Research fixture lost its actual Preview wrapper");
                    const auto actual_restart=research_restart_fixture->verify_after_presented_choice(
                        *restart_preview,*live_alpha_research_backend,receipt);
                    prepared_viewer->gui_screen->require_current();receipt->require_stored_current(*font_source_parent);
                    check(live_client_scene->frame()==actual_frame,"Renderer source changed during actual research checkpoint verification");
                    std::ostringstream restart;
                    restart << "{\"schema\":\"civis-presented-research-checkpoint-observation-v1\","
                            "\"atomicWrites\":" << actual_restart.atomic_writes << ",\"normalLoadsSameProcess\":" << actual_restart.normal_loads
                            << ",\"retainedSections\":" << actual_restart.retained_sections
                            << ",\"partialResearchPreserved\":" << (actual_restart.partial_research_preserved?"true":"false")
                            << ",\"fullContinuationProven\":" << (actual_restart.full_continuation_proven?"true":"false")
                            << ",\"originalBnwParityProven\":" << (actual_restart.original_bnw_parity_proven?"true":"false") << "}\n";
                    blend_validation::text(output/"research-restart-observation.json",restart.str());
                    research_restart_verified=true;
                }
#endif
                std::uint64_t gui_revision=0;
                prepared_viewer->gui_screen->with_actual_screen_loan([&](const auto& loan){gui_revision=loan.frame_revision();});
                std::ostringstream queue;queue << '[';
                const auto actual_queue=receipt->queue();
                for(std::size_t n=0;n<actual_queue.size();++n){if(n)queue << ',';queue << actual_queue[n];}queue << ']';
                std::ostringstream input_trace;
                input_trace << "{\"delivery\":\"NATIVE_WINDOW_MOUSE\",\"control\":\"TechnologyButton\","
                    "\"path\":\"press-release-hit-test-dispatch\",\"button\":\"left\",\"command\":\"ProgressionChooseTechnology\","
                    "\"technology\":" << click.command.technology << ",\"clearQueue\":" << (click.command.clear_queue?"true":"false")
                    << ",\"context\":" << research_json(click.context) << ",\"hitIndex\":" << click.hit_index
                    << ",\"dispatchSequence\":" << click.dispatch_sequence << ",\"bounds\":[" << click.bounds.x << ',' << click.bounds.y
                    << ',' << click.bounds.width << ',' << click.bounds.height << "],\"windowMessages\":["
                    << "{\"message\":" << click.mouse.press.message << ",\"sequence\":" << click.mouse.press.sequence
                    << ",\"x\":" << click.mouse.press.x << ",\"y\":" << click.mouse.press.y << "},"
                    << "{\"message\":" << click.mouse.release.message << ",\"sequence\":" << click.mouse.release.sequence
                    << ",\"x\":" << click.mouse.release.x << ",\"y\":" << click.mouse.release.y << "}]}";
                const auto before_json=research_stamp_json(click.before,click.ui_sequence,click.ui_snapshot,click.generation,click.before_queue);
                const auto after_json=research_stamp_json(actual_frame->stamp,actual_ui_sequence,actual_ui_snapshot,
                    font_source_parent->session().state_generation(),actual_queue);
                std::ostringstream presented;
                presented << "{\"api\":\"GlContext.present\",\"completedPresentCount\":" << presenter.completed_present_count
                    << ",\"guiFrameRevision\":" << gui_revision << ",\"frameSequence\":" << actual_ui_sequence
                    << ",\"uiSourceRevision\":" << actual_frame->stamp.identity.ui_source_revision
                    << ",\"completion\":\"AFTER_PRESENT_AND_BACKEND_COMPLETE\"}";
                std::ostringstream report;
                report << "{\"schema\":\"civis-selected-research-interaction-v1\","
                    "\"producer\":\"native-selected-bnw-research-v1\",\"input\":" << input_trace.str()
                    << ",\"saveSha256\":" << research_json(saved_save_sha256)
                    << ",\"rulesIdentitySha256\":" << research_json(font_source_parent->identity().rules_sha256)
                    << ",\"player\":" << receipt->player() << ",\"team\":" << receipt->team()
                    << ",\"before\":" << before_json << ",\"after\":" << after_json
                    << ",\"commit\":{\"phase\":\"complete\",\"player\":" << receipt->player() << ",\"team\":" << receipt->team()
                    << ",\"technology\":" << click.command.technology << ",\"beforeRevision\":" << receipt->before_revision()
                    << ",\"afterRevision\":" << receipt->after_revision() << ",\"queue\":" << queue.str()
                    << ",\"completedDirtyCalls\":" << receipt->completed_dirty_calls()
                    << "},\"gui\":{\"frameOwner\":" << actual_frame->stamp.identity.owner
                    << ",\"frameSequence\":" << actual_ui_sequence << ",\"snapshotFrame\":" << actual_ui_snapshot
                    << ",\"uiSourceRevision\":" << actual_frame->stamp.identity.ui_source_revision << ",\"frameRevision\":" << gui_revision
                    << ",\"backendPhase\":\"complete\",\"commandStatus\":\"performed\"},\"presentation\":" << presented.str() << "}\n";
                check(!weak.expired()&&live_client_scene->frame()==actual_frame,"Research source changed during receipt formatting");
                prepared_viewer->gui_screen->require_current();receipt->require_stored_current(*font_source_parent);
                blend_validation::text(output/"research-input-trace.json",input_trace.str()+"\n");
                blend_validation::text(output/"research-frame-trace.json","{\"before\":"+before_json+",\"after\":"+after_json+"}\n");
                blend_validation::text(output/"research-presentation-trace.json",presented.str()+"\n");
                blend_validation::text(output/"research-interaction.json",report.str());
                research_click.reset();
            }
        };
        presenter.paint_live_screens = [&](std::int32_t w, std::int32_t h) {
            check(live_ui_frame && live_screen_frame,
                  "Live UI frame must be prepared before screen painting");
            const auto readonly_current = [&] {
                return (!live_client_scene ||
                    (presenter.paint_frame_current && presenter.paint_frame_current())) &&
                    live_ui_host && live_ui_frame && live_screen_frame &&
                    live_ui_host->is_current_frame(*live_ui_frame) &&
                    (!live_client_scene || live_client_scene->accepts(*live_ui_host,*live_ui_frame)) &&
                    live_ui_host->screens().focused_screen()==live_screen_frame->focused_context &&
                    live_ui_frame->owner==live_screen_frame->frame_owner &&
                    live_ui_frame->sequence==live_screen_frame->frame_sequence &&
                    live_ui_frame->snapshot.frame()==live_screen_frame->snapshot_frame;
            };
            check(readonly_current(),"Readonly source frame is stale before screen paint");
            // Consume the SAME loop-start sample after actual UI preparation
            // and final source checks, BEFORE any actual paint call.
            if(tech_award_update_sample) {
                check(live_client_scene&&live_client_scene->frame(),"TechAward UI update requires actual Scene frame");
                const auto held_ui=*live_ui_frame;const auto held_scene=live_client_scene->frame();
                const auto actual_current=[&] {
                    return readonly_current()&&live_client_scene&&live_ui_frame&&
                        live_client_scene->frame()==held_scene&&live_ui_frame->owner==held_ui.owner&&
                        live_ui_frame->sequence==held_ui.sequence&&live_ui_frame->snapshot.same_payload_storage(held_ui.snapshot);
                };
                check(actual_current(),"TechAward actual source retired before UI update");
                live_screen_painter->update_tech_award_animation(held_ui,*held_scene,tech_award_update_sample,actual_current);
                check(actual_current(),"TechAward actual source retired after UI update");
            }
            auto painted = [&] {
                if (prepared_viewer && prepared_viewer->stock_source_owner &&
                    prepared_viewer->gui_screen) {
                    check(bool(worldview_projection_initialize),
                          "WorldView actual Camera run initializer absent");
                    worldview_projection_initialize();
                    check(worldview_projection_binding.has_value(),
                          "WorldView actual Camera projection publication absent");
                    civ5::ui::BnwWorldViewRendererProjectionLoan projection(
                        *live_screen_painter,*worldview_projection_binding,live_client_scene,w,h);
                    return live_screen_painter->paint_owned_worldview(
                        *prepared_viewer->stock_source_owner,*prepared_viewer->gui_screen,projection,scroll_loop_tick.get());
                }
                return live_screen_painter->paint(*live_screen_frame,w,h);
            }();
            check(readonly_current(),"Readonly source frame became stale during screen paint");
            if(live_ui_frame->snapshot.find<civ5::ui::BnwTechAwardContextSnapshot>()) {
                // Retain this actual payload and canonical Scene frame across
                // all selected palette/font resolver callbacks.
                const auto held_ui=*live_ui_frame;
                check(bool(live_client_scene),"TechAward paint requires genuine Scene");
                const auto held_scene=live_client_scene->frame();
                check(bool(held_scene),"TechAward actual Scene frame is absent");
                const auto actual_current=[&] {
                    return readonly_current()&&live_client_scene&&live_ui_frame&&live_screen_frame&&
                        live_client_scene->frame()==held_scene&&
                        live_ui_frame->owner==held_ui.owner&&live_ui_frame->sequence==held_ui.sequence&&
                        live_ui_frame->snapshot.same_payload_storage(held_ui.snapshot);
                };
                check(actual_current(),"TechAward source retired before selected paint");
                auto award=live_screen_painter->paint_tech_award(held_ui,*held_scene,w,h,
                    input.pointer_valid?std::optional<civ5::ui::ClientPoint>{{input.pointer_x,input.pointer_y}}:std::nullopt,
                    actual_current);
                check(actual_current(),"TechAward source retired after selected paint");
                painted.rectangles+=award.rectangles;painted.textures+=award.textures;
                painted.glyphs+=award.glyphs;painted.icons+=award.icons;
                painted.unresolved.insert(painted.unresolved.end(),award.unresolved.begin(),award.unresolved.end());
            }

            last_readonly_text_report=painted.readonly_text;
            last_readonly_tooltip_text.reset();
            if(input.pointer_valid) {
                const civ5::ui::MeasuredReadonlyTextFrame current{live_ui_frame->owner,
                    live_ui_frame->sequence,live_ui_frame->snapshot.frame(),w,h};
                const auto hovered=civ5::ui::pick_measured_readonly_text(painted.readonly_text,current,
                    input.pointer_x,input.pointer_y,live_screen_frame->focused_context,readonly_current);
                if(hovered) {
                    const auto tooltip=civ5::ui::plan_top_panel_tooltip(presenter.top_panel_layout,
                        presenter.tooltip_style,presenter.top_panel_geometry,presenter.icon_geometry,hovered->tooltip,
                        [&presenter](std::string_view name)->std::optional<std::uint32_t> {
                            const auto found=presenter.icon_mapping.find(std::string(name));
                            return found==presenter.icon_mapping.end()?std::nullopt:std::optional<std::uint32_t>(found->second);
                        },{input.pointer_x,input.pointer_y+8},static_cast<std::uint32_t>(w),static_cast<std::uint32_t>(h));
                    check(readonly_current(),"Readonly tooltip source became stale before paint");
                    live_screen_painter->paint_readonly_tooltip(tooltip,presenter.tooltip_style,painted,w,h);
                    check(readonly_current(),"Readonly tooltip source became stale during paint");
                    last_readonly_tooltip_text=hovered->tooltip;
                    live_screen_report << "readonlyTooltip=" << hovered->context_id << ':' << hovered->control_id << '\n';
                }
            }
            live_screen_report << "frame=" << live_ui_frame->snapshot.frame()
                               << " rectangles=" << painted.rectangles
                               << " textures=" << painted.textures
                               << " glyphs=" << painted.glyphs;
            live_screen_report << " readonlyTextRegions=" << painted.readonly_text.regions.size();
            for(const auto &region:painted.readonly_text.regions)
                live_screen_report << " readonly=" << region.context_id << ':' << region.control_id << '@'
                    << region.bounds.x << ',' << region.bounds.y << ',' << region.bounds.width << ',' << region.bounds.height;
            if (live_bnw_preview && live_ui_frame->top_panel &&
                live_ui_frame->top_panel->gold_text)
                live_screen_report << " topPanelGold=" <<
                    *live_ui_frame->top_panel->gold_text;
            if (live_ui_frame->world_view) {
                live_screen_report << " worldPaints="
                                   << live_ui_frame->world_view->paints.size()
                                   << " worldHits="
                                   << live_ui_frame->world_view->hits.size();
                if (const auto *world_state =
                        live_ui_frame->snapshot.find<civ5::ui::WorldViewState>()) {
                    live_screen_report << " cityBanners=" << world_state->city_banners.size();
                    for (const auto &city : world_state->city_banners)
                        live_screen_report << " city=" << city.id << ':' << city.label
                                           << ':' << city.population << '@' << city.screen.x
                                           << ',' << city.screen.y;
                }
                for (const auto &city : projected_city_audit)
                    live_screen_report << " cityProjection=" << city.city_id << ':'
                        << city.world_x << ',' << city.world_y << ':' << city.clip_w
                        << ':' << city.ndc_x << ',' << city.ndc_y << ',' << city.ndc_z
                        << ":view" << city.view;
                for (const auto &hit : live_ui_frame->world_view->hits)
                    if (const auto *gameplay = std::get_if<civ5::ui::UiGameplayCommand>(
                            &hit.command);
                        gameplay && gameplay->as<civ5::ui::WorldViewAction>() &&
                        gameplay->as<civ5::ui::WorldViewAction>()->kind ==
                            civ5::ui::WorldViewActionKind::end_turn)
                        live_screen_report << " endTurnEnabled=" << hit.enabled
                                           << " endTurnReason=" << hit.unavailable_reason;
            }
            live_screen_report << '\n';
            for (const auto &reason : painted.unresolved)
                live_screen_report << "N/A " << reason << '\n';
        };
    }
    std::optional<civ5::ui::UiScreenController> minimap_controller;
    if (saved_minimap_visible && presenter.minimap_chrome) {
        input.minimap_hit_panel = civ5::ui::plan_minimap_panel(
            presenter.minimap_chrome->layout,
            static_cast<std::uint32_t>(client_width),
            static_cast<std::uint32_t>(client_height));
        minimap_controller.emplace(civ5::ui::UiContextRegistry::from_resources(*ui_resources));
        check(minimap_controller->registry().find("MiniMapPanel") != nullptr,
              "Active BNW UI registry has no MiniMapPanel");
    }
    std::ofstream minimap_actions(output / "minimap-actions.csv");
    minimap_actions << "frame,pressX,pressY,releaseX,releaseY,status,reason\n";
    std::ofstream panel_telemetry(output / "top-panel.csv");
    panel_telemetry << "frame,infoVisible,supplyVisible,turn,date\n";
    unsigned panel_frames = 0;
    const auto record_panel = [&] {
        const auto &panel = presenter.top_panel_controls.presentation();
        panel_telemetry << panel_frames++ << ',' << panel.show_info_stack << ','
                        << (panel.show_info_stack && panel.show_unit_supply) << ','
                        << panel.current_turn_text << ',' << panel.current_date_text << '\n';
    };
    SceneTarget target=actual_alpha_frame ? SceneTarget(context,actual_alpha_frame->camera_targets_owned()) : SceneTarget(context,width,height);
    check(target.width==static_cast<unsigned>(width)&&target.height==static_cast<unsigned>(height),"Camera actual target extent differs");
    // FOW culling writes final depth. Retain the real pre-FOW world draw on GPU.
    std::unique_ptr<GlTexture> world_pick_depth;
    PFNGLCOPYIMAGESUBDATAPROC copy_pick_depth=nullptr;
    std::shared_ptr<const civ5::ui::BnwClientFrame> world_pick_frame;
    civ5::terrain::CameraFrameState world_pick_camera{};
    if(viewport_transaction && live_bnw_preview) {
        world_pick_depth=std::make_unique<GlTexture>(GlTexture::render_target(
            *context,width,height,1,RenderTextureFormat::depth24_stencil8));
        copy_pick_depth=reinterpret_cast<PFNGLCOPYIMAGESUBDATAPROC>(context->proc_address("glCopyImageSubData"));
        check(copy_pick_depth!=nullptr,"Native world pick needs actual depth copy entry point");
    }
    const auto waves = civ5::assets::LeanData::parse(actual_alpha_frame
        ?actual_alpha_frame->stage_camera_asset_owned(civ5::assets::AssetKey("waterbumps16.lean"))
        :water_asset(root,"Resource/Common/TerrainModels.fpk","waterbumps16.lean"));
    const auto color = actual_alpha_frame ? civ5::assets::DdsData::parse(
        actual_alpha_frame->stage_camera_asset_owned(civ5::assets::AssetKey("waterdepthcolor_new.dds")),
        civ5::assets::NumericDdsProfile::legacy_d3d9,civ5::assets::DdsHeaderProfile::civ5_d3dx_size24) : civ5::assets::DdsData::parse(
        water_asset(root,"Resource/DX9/Upgrade1Textures.fpk","waterdepthcolor_new.dds"));
    const std::shared_ptr<const TerrainWorld> terrain(world, &world->terrain());
    GlWaterWorldRenderer water(context, terrain, waves, color, 3, X87Precision::double_precision);
    if(actual_alpha_frame)actual_alpha_frame->attach_camera_producer_owned(water);
    if(!actual_alpha_frame)water.prepare_current(); // Own basic water has no sampled current/reflection dependency.
    if (live_bnw_preview)
        project_live_cities = [terrain, &water, &projected_city_audit, client_width,
                               client_height](const civ5::ui::UiFrameSnapshot &frame) {
            return civ5::ui::project_live_city_banners(frame, terrain->source().layout,
                terrain->owner().parameters, water.camera_state(), client_width,
                client_height, &projected_city_audit);
        };
    if (natural_wonder_anchors)
        presenter.paint_world_anchors = [&](std::int32_t w,std::int32_t h) {
            const auto frame=natural_wonder_anchors->compose_frame(
                terrain->source().layout,terrain->owner().parameters,
                water.camera_state(),w,h);
            const auto report=live_screen_painter->paint(frame.overlay,w,h);
            natural_wonder_anchors->confirm_paint(
                frame,report.textures,report.unresolved);
            live_screen_report << "worldAnchorPaints=" << report.textures
                               << " retained="
                               << natural_wonder_anchors->retained_count()
                               << " ready="
                               << world_anchor_scene->supports_gameplay_world_anchor();
            for (std::size_t i=0;i<frame.projected_plots.size();++i) {
                const auto &dest=frame.overlay.paints[i].destination;
                live_screen_report << " anchor=" << frame.projected_plots[i]
                                   << '@' << dest.x+32 << ',' << dest.y-8;
            }
            live_screen_report << '\n';
        };
    auto reflection_depth = std::make_shared<GlTexture>(
        GlTexture::render_target(*context, 2048, 2048, 1, RenderTextureFormat::depth24_stencil8));
    if(!actual_alpha_frame)water.prepare_reflection(2,1,reflection_depth);
    const auto rcp = [](float value) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(value))); };
    const auto rsqrt = [](float value) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(value))); };
    const auto &domain = water.domain();
    WaterReflectionFrameSettings frame_settings{
        {23.75f, 61.125f, 117.5f, 38.25f}, {.3f, -.7f, .25f}, {domain.scales[0], domain.scales[1]}};
    if (original_lighting) {
        const auto lighting = terrain_lighting_defaults(X87Precision::double_precision);
        frame_settings.angles = lighting.angles;
        frame_settings.shadow_direction = lighting.shadow;
    }
    CameraControllerState prior{};
    prior.wrap_orientation = 1;
    auto controller = actual_alpha_frame ? actual_alpha_frame->initial_controller_for_camera_owned() : water.prepare_camera_startup(
        {static_cast<std::uint32_t>(width),static_cast<std::uint32_t>(height),11},prior,frame_settings,rcp,rsqrt);
    if(actual_alpha_frame){const auto actual=water.prepare_camera_motion(controller,{0,0,false,false,false,0},frame_settings,rcp,rsqrt);
        check(actual.controller.camera.world==controller.camera.world,"Actual saved initial Camera position changed");controller=actual.controller;}
    // Retires before its borrowed controller and water locals on every exit.
    std::unique_ptr<civ5::ui::BnwWorldViewRendererRunLoan> worldview_run_loan;
    struct RetireWorldViewProjectionInitializer {
        std::function<void()>& target;
        ~RetireWorldViewProjectionInitializer() noexcept { target=nullptr; }
    } retire_worldview_projection_initializer{worldview_projection_initialize};
    worldview_projection_initialize = [&] {
        check(live_screen_painter && prepared_viewer && prepared_viewer->stock_source_owner &&
              prepared_viewer->gui_screen,
              "WorldView actual Camera run origins absent");
        if (!worldview_run_loan) {
            worldview_run_loan.reset(new civ5::ui::BnwWorldViewRendererRunLoan(
                *live_screen_painter,water,controller,terrain,live_client_scene));
            worldview_projection_binding.emplace(worldview_run_loan->bind_owned());
        } else {
            // Recheck the same live run; never renew its weak Binding per frame.
            (void)worldview_run_loan->bind_owned();
        }
    };
    CameraPointerState pointer{width, height};
    const CameraInputSettings defaults{{1, 0}, false, true};
    float normalized_zoom=actual_alpha_frame ? (controller.zoom-controller.minimum_zoom)/(controller.maximum_zoom-controller.minimum_zoom) : 0;
    if(!actual_alpha_frame)for (const auto &step : camera_scene_input_steps()) {
        const auto result = water.prepare_camera_input(
            controller, pointer, {step.dt, step.commands, step.input_mode, defaults, step.events},
            frame_settings, rcp, rsqrt);
        preview_camera_events(land, result.motion.events);
        controller = result.motion.controller;
        normalized_zoom = result.motion.zoom[0];
        pointer = result.pointer;
    }
    if (saved_art && !actual_alpha_frame) {
        struct VisiblePlot { int x, y; bool currently_visible; };
        std::vector<VisiblePlot> visible;
        double mean_x = 0, mean_y = 0;
        const auto &layout = terrain->source().layout;
        const auto grid_width = layout.source_grid.width;
        for (const auto &plot : saved_art->plots) {
            if (plot.revealed != true ||
                (focus_revealed_ice
                    ? std::find(revealed_ice_plot_indices.begin(),
                                revealed_ice_plot_indices.end(), plot.plot_index) ==
                          revealed_ice_plot_indices.end()
                    : plot.visibility_count <= 0)) continue;
            const auto x = static_cast<int>(plot.plot_index % grid_width);
            const auto y = static_cast<int>(plot.plot_index / grid_width);
            visible.push_back({x, y, plot.visibility_count > 0});
            mean_x += x;
            mean_y += y;
        }
        check(!visible.empty(), focus_revealed_ice
            ? "Saved preview has no revealed ice plot to focus"
            : "Saved FOW preview needs a visible plot to focus the diagnostic camera");
        mean_x /= visible.size();
        mean_y /= visible.size();
        std::sort(visible.begin(), visible.end(), [&](const auto &a, const auto &b) {
            if (focus_revealed_ice && a.currently_visible != b.currently_visible)
                return a.currently_visible;
            const auto distance = [&](const auto &v) {
                const double dx = v.x - mean_x, dy = v.y - mean_y;
                return dx * dx + dy * dy;
            };
            return distance(a) < distance(b);
        });
        const auto &m = layout.source_grid.second_matrix;
        const auto &w = layout.transforms;
        const auto &height_owner = terrain->owner().parameters;
        bool focused = false;
        VisiblePlot focus{};
        Vector3 focus_ground{};
        Vector3 focus_camera{};
        for (const auto plot : visible) {
            const auto a = civ5::terrain::wrap_add(plot.x, civ5::terrain::wrap_neg(plot.y >> 1));
            const auto c = civ5::terrain::wrap_neg(civ5::terrain::wrap_add(a, plot.y));
            const auto grid_x = civ5::terrain::wrap_add(
                civ5::terrain::wrap_add(civ5::terrain::wrap_add(civ5::terrain::wrap_mul(a, m[0]), m[12]),
                                         civ5::terrain::wrap_mul(c, m[8])), civ5::terrain::wrap_mul(plot.y, m[4]));
            const auto grid_y = civ5::terrain::wrap_add(
                civ5::terrain::wrap_add(civ5::terrain::wrap_add(civ5::terrain::wrap_mul(plot.y, m[5]), m[13]),
                                         civ5::terrain::wrap_mul(c, m[9])), civ5::terrain::wrap_mul(a, m[1]));
            const float x = static_cast<float>(grid_x), y = static_cast<float>(grid_y), z = -x - y;
            const auto ground_x = height_owner.scale_x *
                (x * w[16] + z * w[24] + y * w[20] + w[28]) - height_owner.center_x;
            const auto ground_y = height_owner.scale_y *
                (x * w[17] + z * w[25] + y * w[21] + w[29]) - height_owner.center_y;
            Vector3 position{ground_x, ground_y, controller.target[2]};
            if (focus_revealed_ice) {
                // The northern ice is on the last saved row. Retail camera
                // movement stops short of the world edge; aim as close as its
                // configured bounds allow while testing the ice plot itself.
                const float slope = (-1.f / controller.camera.world[10]) *
                                    controller.camera.world[9];
                const float offset = (position[2] - layout.bounds[2]) * slope;
                const float low_y = std::max(layout.bounds[1], controller.movement_bounds[2]) - offset;
                const float high_y = std::min(layout.bounds[4], controller.movement_bounds[3]) - offset;
                check(low_y <= high_y, "Saved ice camera has no legal Y target");
                position[1] = std::clamp(position[1], low_y, high_y);
            }
            auto candidate = set_camera_position(layout, terrain->source().map_mode, controller,
                                                 position, X87Precision::double_precision);
            if (candidate.target != position) continue;
            controller = std::move(candidate);
            focused = true;
            focus = plot;
            focus_ground = {ground_x, ground_y, 0.f};
            focus_camera = position;
            focused_ice_currently_visible = plot.currently_visible;
            std::cout << (focus_revealed_ice
                              ? "Saved ice diagnostic camera focused plot "
                              : "Saved FOW diagnostic camera focused plot ")
                      << plot.x << ',' << plot.y << '\n';
            break;
        }
        check(focused, "Saved FOW visible plots are outside camera movement bounds");
        if (focus_revealed_ice)
            blend_validation::text(output / "saved-ice-camera-focus.json",
                "{\"schema\":\"civ5-saved-ice-camera-focus-v1\",\"plot\":[" +
                std::to_string(focus.x) + "," + std::to_string(focus.y) +
                "],\"revealedIcePlotCount\":" +
                std::to_string(revealed_ice_plot_indices.size()) +
                ",\"world\":[" + std::to_string(focus_ground[0]) + "," +
                std::to_string(focus_ground[1]) + "],\"cameraTarget\":[" +
                std::to_string(focus_camera[0]) + "," +
                std::to_string(focus_camera[1]) +
                "],\"currentlyVisible\":" +
                (focused_ice_currently_visible ? "true" : "false") + "}\n");
        const auto moved = water.prepare_camera_motion(controller, {0.f, 0, false, false, false, 0},
                                                       frame_settings, rcp, rsqrt);
        controller = moved.controller;
        normalized_zoom = moved.zoom[0];
        if (!focus_revealed_ice) {
            const auto &camera = water.camera_state();
            const auto vp = matrix_multiply(camera.views[0], camera.projection,
                                            X87Precision::double_precision, MatrixPath::sse);
            const auto clip = vector4_transform(
                {focus_ground[0], focus_ground[1], focus_ground[2], 1.f}, vp,
                X87Precision::double_precision, Vec4Path::sse);
            const float determinant = vp[0] * vp[5] - vp[4] * vp[1];
            check(std::abs(determinant) > 1e-6f, "Saved FOW diagnostic camera cannot center the visible plot");
            const Vector3 centered{
                controller.target[0] + (clip[0] * vp[5] - clip[1] * vp[4]) / determinant,
                controller.target[1] + (vp[0] * clip[1] - vp[1] * clip[0]) / determinant,
                controller.target[2]};
            auto aimed = set_camera_position(layout, terrain->source().map_mode, controller,
                                             centered, X87Precision::double_precision);
            check(aimed.target == centered, "Saved FOW centered camera is outside movement bounds");
            const auto aim_motion = water.prepare_camera_motion(
                aimed, {0.f, 0, false, false, false, 0}, frame_settings, rcp, rsqrt);
            controller = aim_motion.controller;
            normalized_zoom = aim_motion.zoom[0];
        }
        bool in_view = false;
        const auto &tiles = water.frame_state().bounds;
        for (unsigned view = 0; view < water.camera_state().view_count; ++view) {
            const auto &low = tiles.clipped_tile_low[view], &high = tiles.clipped_tile_high[view];
            std::cout << "Saved FOW camera tiles view " << view << ": " << low[0] << ',' << low[1]
                      << " .. " << high[0] << ',' << high[1] << '\n';
            const bool focus_in_this_view = low[0] <= focus.x && focus.x < high[0] &&
                                            low[1] <= focus.y && focus.y < high[1];
            in_view = in_view || focus_in_this_view;
            const auto vp = matrix_multiply(water.camera_state().views[view],
                                            water.camera_state().projection,
                                            X87Precision::double_precision, MatrixPath::sse);
            const auto clip = vector4_transform(
                {focus_ground[0], focus_ground[1], focus_ground[2], 1.f}, vp,
                X87Precision::double_precision, Vec4Path::sse);
            std::cout << "Saved FOW focused plot clip " << clip[0] << ',' << clip[1] << ','
                      << clip[2] << ',' << clip[3] << " NDC " << clip[0] / clip[3] << ','
                      << clip[1] / clip[3] << '\n';
            const float focus_limit = focus_revealed_ice ? 1.0f : .5f;
            check((focus_revealed_ice && !focus_in_this_view) ||
                      (std::abs(clip[0] / clip[3]) < focus_limit &&
                       std::abs(clip[1] / clip[3]) < focus_limit),
                  "Saved FOW focused plot was not centered in the camera image");
        }
        check(in_view, "Saved FOW camera did not include its focused plot");
    }
    if (retail_camera) {
        const auto &pose = retail_camera->camera;
        const Vector3 position{pose.world[12], pose.world[13], pose.world[14]};
        const auto &source = terrain->source();
        auto positioned = set_camera_position(source.layout, source.map_mode, controller,
                                              position, X87Precision::double_precision);
        check(positioned.target == position, "Retail camera pose is outside saved map movement bounds");
        positioned.camera = pose;
        positioned.flags[1] = positioned.flags[2] = positioned.flags[3] = 0;
        positioned.velocity = {};
        positioned.history = {};
        positioned.history_index = 0;
        const auto motion = water.prepare_camera_motion(
            positioned, {0.f, 0, false, false, false, 0}, frame_settings, rcp, rsqrt);
        check(motion.controller.camera.world == pose.world &&
              motion.controller.camera.field_of_view == pose.field_of_view &&
              motion.controller.camera.aspect_ratio == pose.aspect_ratio,
              "Retail camera pose changed during zero-time preparation");
        controller = motion.controller;
        normalized_zoom = motion.zoom[0];
        std::ostringstream report;
        report << std::setprecision(17)
               << "{\"schema\":\"civ5-retail-camera-preview-v1\",\"width\":" << width
               << ",\"height\":" << height << ",\"fov\":" << pose.field_of_view
               << ",\"aspect\":" << pose.aspect_ratio << ",\"world\":[";
        for (std::size_t i = 0; i < pose.world.size(); ++i) {
            if (i) report << ',';
            report << pose.world[i];
        }
        report << "],\"near\":" << pose.near_plane << ",\"far\":" << pose.far_plane
               << ",\"savedPrimitiveStateRecovered\":false}\n";
        blend_validation::text(output / "retail-camera-import.json", report.str());
    }
    // Save the exact produced camera as the reset location.
    const auto initial = controller;
    if (saved_minimap && saved_art && expansion2_ui) {
        // The save itself does not supply the live owner field at +0x277F4.
        // The separate passive retail fixture supplies it for this selected
        // save; retain the zero/unit probes for comparison with older runs.
        const auto &state = water.camera_state();
        const std::span<const ViewFrustum> frusta(state.frusta.data(), state.view_count);
        std::ostringstream report;
        report << std::setprecision(9)
               << "{\"schema\":\"civ5-minimap-camera-viewport-probe-v1\","
               << "\"poseFromSave\":false,\"ownerSourceValueKnown\":"
               << (native_minimap_bounds ? "true" : "false") << ','
               << "\"selectedSourceValue\":"
               << (native_minimap_bounds ? native_minimap_bounds->height_source : 0.0f) << ','
               << "\"viewCount\":" << state.view_count << ",\"heightScale\":"
               << terrain->owner().parameters.scale_z << ",\"frusta\":[";
        for (std::size_t view = 0; view < frusta.size(); ++view) {
            if (view) report << ',';
            report << '[';
            for (std::size_t plane = 0; plane < frusta[view].size(); ++plane) {
                if (plane) report << ',';
                report << '[';
                for (std::size_t axis = 0; axis < 4; ++axis) {
                    if (axis) report << ',';
                    report << frusta[view][plane][axis];
                }
                report << ']';
            }
            report << ']';
        }
        report << "],\"sourceVariations\":[";
        for (float source_value : {0.0f, 1.0f}) {
            if (source_value != 0.0f) report << ',';
            const float height = post_initialization_scalar(
                source_value, terrain->owner().parameters.scale_z);
            const auto viewports = civ5::ui::minimap_camera_viewports(frusta, height);
            report << "{\"sourceValue\":" << source_value << ",\"planeHeight\":"
                   << height << ",\"views\":[";
            for (std::size_t view = 0; view < viewports.size(); ++view) {
                if (view) report << ',';
                report << "{\"low\":[" << viewports[view].low[0] << ','
                       << viewports[view].low[1] << "],\"high\":["
                       << viewports[view].high[0] << ',' << viewports[view].high[1]
                       << "],\"corners\":[";
                for (std::size_t corner = 0; corner < 4; ++corner) {
                    if (corner) report << ',';
                    const auto &point = viewports[view].corners[corner];
                    report << '[' << point[0] << ',' << point[1] << ',' << point[2] << ']';
                }
                report << "]}";
            }
            report << "]}";
        }
        report << "]}\n";
        blend_validation::text(output / "minimap-camera-viewports.json", report.str());
        check(saved_minimap_visible && presenter.minimap_chrome &&
                  presenter.minimap_chrome->partial_map,
              "Saved preview kind-8 source has no partial minimap texture");
        const auto &minimap_bounds = *saved_minimap_visible;
        const auto &visible = minimap_bounds.segments[0];
        // The same-save passive DX9 singleton reports two active kind-8
        // camera views. Its post-load kind-7 segment is narrower than the
        // whole map, so the static full-grid span gate alone would hide the
        // ring that is present in the clean G0 frame. The captured 1.6-aspect
        // corner coordinates are not reused with this preview camera.
        const bool ring_required = native_minimap_bounds ||
            civ5::ui::minimap_needs_viewport_ring(
                visible, terrain->owner().parameters.world_span_x,
                terrain->owner().parameters.world_span_y);
        std::ostringstream ring_report;
        ring_report << std::setprecision(9)
                    << "{\"schema\":\"civ5-minimap-kind8-live-preview-probe-v1\","
                    << "\"syntheticPreviewCamera\":"
                    << (retail_camera ? "false" : "true")
                    << ",\"heightSourceKnown\":"
                    << (native_minimap_bounds ? "true" : "false")
                    << ",\"capturedActiveViewCount\":"
                    << (native_minimap_bounds ? 2 : 0) << ','
                    << "\"partialMapBackground\":true,\"ringRequired\":"
                    << (ring_required ? "true" : "false")
                    << ",\"bounds\":[" << visible.min_x << ',' << visible.min_y
                    << ',' << visible.max_x << ',' << visible.max_y << "],\"cases\":[";
        if (ring_required) {
            const auto background = presenter.minimap_chrome->partial_map->read_mip_raw(0);
            check(background.size() == 320u * 190u * 4u,
                  "Saved minimap partial texture readback extent changed");
            for (float source_value : {0.0f, 1.0f}) {
                if (source_value != 0.0f) ring_report << ',';
                const float plane_height = post_initialization_scalar(
                    source_value, terrain->owner().parameters.scale_z);
                const auto corners = civ5::ui::minimap_viewport_corners(frusta, plane_height);
                const auto ring = civ5::ui::build_minimap_viewport_ring(
                    corners, visible, minimap_bounds.texture.active_width);
                auto overlaid = GlTexture::render_target(
                    *context, 320, 190, 1, RenderTextureFormat::rgba8);
                const auto &api = context->api();
                const std::array<GLenum, 4> unpack_fields{
                    GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH,
                    GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS};
                std::array<GLint, 4> prior_unpack{};
                for (std::size_t field = 0; field < unpack_fields.size(); ++field) {
                    api.GetIntegerv(unpack_fields[field], &prior_unpack[field]);
                    api.PixelStorei(unpack_fields[field], field == 0 ? 4 : 0);
                }
                api.TextureSubImage2D(overlaid.id(), 0, 0, 0, 320, 190,
                                      GL_BGRA, GL_UNSIGNED_BYTE, background.data());
                for (std::size_t field = 0; field < unpack_fields.size(); ++field)
                    api.PixelStorei(unpack_fields[field], prior_unpack[field]);
                context->check("Copy saved minimap into kind-8 diagnostic target");
                GlRenderApi gl(*context);
                GLuint framebuffer = 0;
                gl.CreateFramebuffers(1, &framebuffer);
                gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, overlaid.id(), 0);
                const GLenum attachment = GL_COLOR_ATTACHMENT0;
                gl.NamedFramebufferDrawBuffers(framebuffer, 1, &attachment);
                check(gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) ==
                          GL_FRAMEBUFFER_COMPLETE,
                      "Saved minimap kind-8 diagnostic framebuffer is incomplete");
                gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                {
                    GlMinimapViewportRingPass pass(context);
                    pass.draw(ring, visible, 0.5f,
                              minimap_bounds.texture.padding_x,
                              minimap_bounds.texture.active_width);
                }
                gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
                gl.DeleteFramebuffers(1, &framebuffer);
                const auto name = source_value == 0.0f
                    ? "minimap-kind8-source0-gl.ppm"
                    : "minimap-kind8-source1-gl.ppm";
                GlMinimapSurfaceDiagnostic::save_ppm(overlaid, output / name);
                ring_report << "{\"sourceValue\":" << source_value
                            << ",\"planeHeight\":" << plane_height
                            << ",\"vertexCount\":" << ring.vertices.size()
                            << ",\"indexCount\":" << ring.indices.size()
                            << ",\"image\":\"" << name << "\"}";
            }
        }
        ring_report << "]}\n";
        blend_validation::text(output / "minimap-kind8-gl.json", ring_report.str());
    }
    if constexpr (requires { land.save_camera_motion(); })
        land.save_camera_motion();
    pointer = {width, height};
    GlWaterSurfaceInputs scene;
    if(!actual_alpha_frame)for (unsigned slot : {8u,9u,10u,11u,13u}) {
        if (slot == 11 && original_fow) {
            check(color_keys, "FOW water preview requires authored ColorKeys");
            scene.pixel[slot] = color_keys->at(ColorKeySystem::terrain_fow, 0).id();
            continue;
        }
        if (slot == 8 && distance_fog) {
            scene.pixel[slot] = distance_fog->ramp().id();
            continue;
        }
        auto image = slot == 11 ? identity_volume() : FloatImage{1, false, {0, 0, 0, 0}};
        if (slot == 10)
            image.values = {1, 0, 1, 1};
        scene.pixel[slot] = controls.image(image);
    }
    const auto sky = water_sky_pixels(actual_alpha_frame ? civ5::assets::DdsData::parse(
        actual_alpha_frame->stage_camera_asset_owned(civ5::assets::AssetKey("watersky.dds")),
        civ5::assets::NumericDdsProfile::legacy_d3d9,civ5::assets::DdsHeaderProfile::civ5_d3dx_size24) : civ5::assets::DdsData::parse(
        water_asset(root,"Resource/DX9/TerrainTextures.fpk","watersky.dds")));
    auto surface = initial_water_surface_settings(13000, water_sky_lighting(sky, rsqrt));
    WaterSurfaceFrame frame{};
    frame.fog_parameters = {.002f, 0, 0, 0};
    WaterSurfaceView surface_view{};
    surface_view.overlay = Matrix4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    std::unique_ptr<FogOfWarScene> fow;

    if (original_fow) {
        check(distance_fog != nullptr, "FOW preview requires the original fog and lighting frame");
        const auto* actual_fow=actual_material ? &actual_material->fog_assets_owned() : nullptr;
        fow=std::make_unique<FogOfWarScene>(context,*terrain,root,!saved_art,actual_fow);
        if(actual_alpha_frame)actual_alpha_frame->require_camera_boundary_owned();
        if (saved_art) {
            const auto counts = civ5::scene::apply_saved_map_fog(
                {static_cast<std::uint64_t>(fow->state.width()),
                 static_cast<std::uint64_t>(fow->state.height())},
                *saved_art, fow->state);
            fow->state.tick(0.f);
            if (live_bnw_preview) {
                check(live_scene_receipt && live_bnw_preview->scene_identity,
                      "Live BNW FOW requires its restored scene and rules identity");
                check(live_bnw_preview->activation_owner,
                      "Live BNW FOW requires the enclosing execution activation owner");
                live_fog_client = std::make_unique<civ5::graphics::BnwCameraFogClient>(
                    live_bnw_preview->session,*live_bnw_preview->activation_owner,
                    *live_bnw_preview->scene_identity,*live_scene_receipt,counts,fow->state,fow->map,
                    "production-camera-fow");
                live_client_scene->bind_native_fog(live_fog_client->binding());
                const auto &baseline=*live_fog_client->journal().published;
                blend_validation::text(output / "saved-fow-client-receipt.txt",
                    "source=production-camera-fow\nrevision=" + baseline.snapshot().binding.revision +
                    "\nsequence=" + std::to_string(baseline.sequence()) +
                    "\nevents=" + std::to_string(baseline.events_applied()) +
                    "\nqueue_known=true\nfullGraphicsParity=false\n");
            }
            blend_validation::text(output / "saved-fow-state.txt",
                "visible=" + std::to_string(counts.visible) + " explored=" +
                std::to_string(counts.explored) + " unexplored=" +
                std::to_string(counts.unexplored) + " visible_unrevealed=" +
                std::to_string(counts.visible_unrevealed) + "\n");
        }
    }
    std::unique_ptr<GlRiverWorldRenderer> rivers;
    GlRiverSurfaceInputs river_scene;
    if (original_rivers) {
        check(fow && color_keys, "River preview requires live FOW and authored ColorKeys");
        rivers = std::make_unique<GlRiverWorldRenderer>(context, terrain, load_river_world_assets(root));
        river_scene.pixel[4] = water.reflection_map().texture().id();
        river_scene.pixel[9] = scene.pixel[9];
        river_scene.pixel[11] = color_keys->at(ColorKeySystem::terrain_fow, 0).id();
        river_scene.pixel[12] = color_keys->at(ColorKeySystem::rivers, 0).id();
    }
    std::vector<std::array<float, 6>> bounds;
    for (std::size_t i = 0; i < world->cache().patches.size(); ++i)
        bounds.push_back(
            world->terrain().land().patch(i / world->cache().rows, i % world->cache().rows).culling_bounds());
    unsigned draws = 0;
    std::size_t warrior_draws = 0, warrior_hidden_frames = 0, warrior_changed_pixels = 0;
    bool draw_owned_warrior = true;
    GLint water_program = 0;
    const auto render = [&](bool cull) {
        if (fow && !live_fog_client)
            fow->prepare(water, *distance_fog, normalized_zoom);
        const civ5::ui::detail::CivisFrontendPartyMaterialStorage::FrameInputsLoan* actual_frame_loan=nullptr;
        const auto draw_scene = [&](const GlTexture *mask) {
            if(actual_alpha_frame)check(actual_frame_loan!=nullptr,"Actual water frame loan missing");
            world_pick_frame.reset();
            target.begin();
            if(live_pending_unit_visuals)
                live_pending_unit_visuals->draw_pending_slots(live_client_scene->frame());
            draws = 0;
            const auto &state = water.camera_state();
            if (distance_fog) {
                const auto global = distance_fog->frame(normalized_zoom);
                frame.fog_color = global.color;
                frame.fog_parameters = global.parameters;
            }
            for (unsigned view = 0; view < state.view_count; ++view) {
                auto camera = actual_alpha_frame ? fow->land.at(view) : overview_frame();
                const auto vp = matrix_multiply(state.views[view], state.projection,
                                                X87Precision::double_precision, MatrixPath::sse);
                for (unsigned row = 0; row < 4; ++row)
                    std::copy_n(vp.begin() + row * 4, 4, camera.view_projection[row].begin());
                std::copy_n(state.inverse_views[view].begin() + 12, 3, camera.camera.begin());
                if (distance_fog)
                    camera =
                        distance_fog->land_constants(camera, normalized_zoom, X87Precision::double_precision);
                if (fow)
                    camera = fow->land.at(view);
                if (mask) {
                    land_scene.pixel[4] = mask->id();
                    land_scene.pixel[5] = fow->map.clouds()[0].id();
                }
                if(actual_alpha_frame)actual_alpha_frame->apply_land_layers_owned(*actual_frame_loan,camera,land_scene);
                for (auto index : scene_land_indices(land, *world, view))
                    if (!cull || preview_patch_visible(bounds[index], camera)) {
                        land.draw(index, camera, land_scene, width, height);
                        ++draws;
                    }
            }
            if(actual_alpha_frame){
                actual_alpha_frame->draw_basic_water_owned(*actual_frame_loan);
            }else for (unsigned view = 0; view < state.view_count; ++view) {
                if (fow)
                    surface_view.overlay = fow->frame.overlays[view];
                if (mask)
                    scene.pixel[10] = mask->id();
                water.draw(view, water.constants(view, surface, frame, surface_view), scene, width, height);
            }
            if (!actual_alpha_frame && !water.draws().empty()) {
                GLint program = 0;
                context->api().GetIntegerv(GL_CURRENT_PROGRAM, &program);
                if (!water_program)
                    water_program = program;
                check(program == water_program, "Camera frame recompiled its water shader");
            }
            if (rivers) {
                river_scene.pixel[10] = mask->id();
                for (unsigned view = 0; view < state.view_count; ++view) {
                    surface_view.overlay = fow->frame.overlays[view];
                    rivers->draw(water.constants(view, surface, frame, surface_view), river_scene, width,
                                 height);
                }
            }
            if (owned_warrior && draw_owned_warrior) {
                const auto visible = civ5::graphics::selected_gandhi_warrior_from_live_scene(
                    *live_scene_receipt,live_bnw_preview->session.state(),live_bnw_preview->active_player_id);
                if (visible&&owned_warrior->center_unit_visibility_for_draw(live_bnw_preview->session.state())) {
                    for (unsigned view = 0; view < state.view_count; ++view) {
                        civ5::graphics::GlBnwWarriorFrame unit_frame;
                        unit_frame.view = state.views[view];
                        unit_frame.projection = state.projection;
                        unit_frame.width = width;
                        unit_frame.height = height;
                        const auto result = owned_warrior->draw(live_bnw_preview->session.state(),unit_frame);
                        check(result.drawn && result.members == 12 && result.triangles == 7920 &&
                                  !result.model_drawable && !result.full_graphics_parity,
                              "Owned Warrior shared scene draw contract differs");
                        ++warrior_draws;
                    }
                } else ++warrior_hidden_frames;
            }
            if(world_pick_depth) {
                check(live_client_scene && live_client_scene->ready_to_draw() &&
                    world_pick_depth->belongs_to(*context),"World pick depth has no actual frame owner");
                const auto picked=live_client_scene->frame();
                copy_pick_depth(target.depth,GL_TEXTURE_2D,0,0,0,0,
                    world_pick_depth->id(),GL_TEXTURE_2D,0,0,0,0,width,height,1);
                context->check("Retain actual world depth before FOW culling");
                check(live_client_scene->frame()==picked && live_client_scene->ready_to_draw(),
                    "World pick source changed while retaining depth");
                world_pick_camera=water.camera_state();world_pick_frame=picked;
            }
        };
        const auto actual_render=[&]{
        if (fow && live_fog_client) {
            const auto native_binding=live_fog_client->binding();
            auto receipt = fow->render_with_receipt(target,water,*distance_fog,normalized_zoom,
                controller,live_bnw_preview->session.state(),*live_bnw_preview->scene_identity,
                live_client_scene->receipt(),live_fog_client->journal(),*live_client_scene->frame(),
                [&](const GlTexture &mask) { draw_scene(&mask); },&native_binding);
            live_client_scene->accept_rendered_fog(std::move(receipt));
        } else if (fow)
            fow->render(target, [&](const GlTexture &mask) { draw_scene(&mask); });
        else
            draw_scene(nullptr);
        };
        if(actual_alpha_frame){
            actual_alpha_frame->with_current_camera_frame_owned(water,controller,[&](const auto& loan){
                actual_frame_loan=&loan;actual_render();loan.require_current();actual_frame_loan=nullptr;
            });
        }else actual_render();
    };
    const auto validate_culling = [&](const std::string &name) {
        render(false);
        const auto all = target.pixels();
        render(true);
        const auto culled = target.pixels();
        check(all == culled, "Preview culling changed the full Float32 frame");
        if (owned_warrior) {
            draw_owned_warrior = false;
            render(true);
            const auto without_unit = target.pixels();
            draw_owned_warrior = true;
            check(without_unit.size() == culled.size(), "Owned unit frame extent changed");
            std::size_t changed = 0;
            for (std::size_t i = 0; i < culled.size(); i += 4)
                changed += culled[i] != without_unit[i] || culled[i+1] != without_unit[i+1] ||
                           culled[i+2] != without_unit[i+2] || culled[i+3] != without_unit[i+3];
            warrior_changed_pixels = std::max(warrior_changed_pixels,changed);
            blend_validation::write(output / (name + "-without-warrior.f32"), without_unit);
            render(true);
            check(target.pixels() == culled, "Warrior draw failed to restore the same frame");
        }
        check(draws > 0 && draws < bounds.size(), "Preview did not reduce offscreen LAND draws");
        blend_validation::write(output / (name + ".f32"), culled);
    };
    const auto latch_live_client_frame = [&] {
        if (!live_client_scene) return;
        if (live_fog_client && live_fog_client->consume_committed())
            live_client_scene->invalidate();
        RECT actual{};
        check(GetClientRect(window, &actual) && actual.right == client_width &&
              actual.bottom == client_height,
              "BNW fixed preview viewport changed; renderer resize is required");
        // UI paints into the presented client framebuffer; LAND has a separate
        // fixed render target. Both use this frame's established camera.
        check(live_client_scene->begin(
            {client_width, client_height, client_width, client_height, preview_window_dpi(window)},
            controller) != nullptr, "BNW client viewport is not drawable");
        live_camera_current = [&, camera = controller.camera,
                               views = water.camera_state()] {
            const auto &actual = controller.camera;
            const auto &rendered = water.camera_state();
            if (actual.world != camera.world || actual.field_of_view != camera.field_of_view ||
                actual.aspect_ratio != camera.aspect_ratio || actual.near_plane != camera.near_plane ||
                actual.far_plane != camera.far_plane || rendered.view_count != views.view_count ||
                rendered.projection != views.projection) return false;
            for (unsigned view = 0; view < rendered.view_count; ++view)
                if (rendered.views[view] != views.views[view] ||
                    rendered.inverse_views[view] != views.inverse_views[view] ||
                    rendered.relative_offsets[view] != views.relative_offsets[view]) return false;
            return true;
        };
    };
    // Arm BEFORE the first actual snapshot publication; any setup/render failure
    // after this point retires the scope before earlier UI owners unwind.
    RetireWorldViewFontSource first_publication_font_retirement{worldview_font_source};
    latch_live_client_frame();
    scene_land_prepare(land, water.camera_state(), output / "scheduled-materials" / "0");
    if (smoke)
        validate_culling("initial");
    else
        render(true);
    scene_land_finish(land);
    if (saved_art) {
        const auto mask = fow->map.mask().read_mip_raw(0);
        std::array<std::size_t, 4> changed{};
        for (std::size_t i = 0; i < mask.size(); i += 4)
            for (std::size_t channel = 0; channel < changed.size(); ++channel)
                changed[channel] += mask[i + channel] != 0;
        check(changed[0] > 0 || (focus_revealed_ice && !focused_ice_currently_visible),
              "Saved FOW visible area was absent from the rendered mask");
        blend_validation::text(output / "saved-fow-mask.txt",
            "nonzero_rgba=" + std::to_string(changed[0]) + "," + std::to_string(changed[1]) +
            "," + std::to_string(changed[2]) + "," + std::to_string(changed[3]) + "\n");
        std::cout << "Saved FOW mask nonzero RGBA pixels " << changed[0] << ',' << changed[1]
                  << ',' << changed[2] << ',' << changed[3] << '\n';
        for (const auto &pass : fow->frame.commands.passes)
            std::cout << "Saved FOW pass " << static_cast<unsigned>(pass.kind) << " draws "
                      << pass.end - pass.begin << '\n';
    }
    GlRenderApi minimap_gl(*context);
    GLuint minimap_viewport_framebuffer = 0;
    std::optional<GlMinimapViewportRingPass> minimap_viewport_pass;
    const bool live_minimap_viewport = saved_minimap_visible &&
        presenter.minimap_chrome && presenter.minimap_chrome->partial_map &&
        (native_minimap_bounds || civ5::ui::minimap_needs_viewport_ring(
            saved_minimap_visible->segments[0], terrain->owner().parameters.world_span_x,
            terrain->owner().parameters.world_span_y));
    if (live_minimap_viewport) {
        check(saved_minimap_base_bgra.size() == 320u * 190u * 4u,
              "Live minimap viewport has no original surface image");
        minimap_viewport_pass.emplace(context);
        minimap_gl.CreateFramebuffers(1, &minimap_viewport_framebuffer);
        minimap_gl.NamedFramebufferTexture(
            minimap_viewport_framebuffer, GL_COLOR_ATTACHMENT0,
            presenter.minimap_chrome->partial_map->id(), 0);
        const GLenum attachment = GL_COLOR_ATTACHMENT0;
        minimap_gl.NamedFramebufferDrawBuffers(minimap_viewport_framebuffer, 1, &attachment);
        check(minimap_gl.CheckNamedFramebufferStatus(minimap_viewport_framebuffer, GL_FRAMEBUFFER) ==
                  GL_FRAMEBUFFER_COMPLETE,
              "Live minimap viewport framebuffer is incomplete");
        std::ostringstream viewport_source;
        viewport_source << std::setprecision(9)
                        << "{\"schema\":\"civ5-minimap-live-viewport-v1\","
                        << "\"cameraFromPreview\":true,\"capturedCameraCornersApplied\":false,"
                        << "\"capturedActiveViewCount\":"
                        << (native_minimap_bounds ? 2 : 0) << ','
                        << "\"heightSourceKnown\":"
                        << (native_minimap_bounds ? "true" : "false")
                        << ",\"heightSource\":"
                        << (native_minimap_bounds ? native_minimap_bounds->height_source : 0.0f)
                        << ",\"planeHeight\":"
                        << (native_minimap_bounds ? native_minimap_bounds->plane_height : 0.0f)
                        << "}\n";
        blend_validation::text(output / "minimap-live-viewport.json",
                               viewport_source.str());
    }
    const bool live_minimap_fog = live_minimap_surface && live_minimap_preparation &&
        live_client_scene && saved_minimap_visible && presenter.minimap_chrome &&
        presenter.minimap_chrome->partial_map;
    const auto minimap_wgl_context = wglGetCurrentContext();
    const auto minimap_wgl_dc = wglGetCurrentDC();
    std::shared_ptr<const civ5::ui::BnwMinimapFogPacket> live_minimap_packet;
    std::optional<GlMinimapSurfaceDiagnostic::AppliedFogReceipt> applied_minimap_fog;
    std::optional<civ5::ui::MinimapVisibleBounds> applied_minimap_ring_bounds;
    std::function<bool()> applied_minimap_ring_camera_current;
    std::uint64_t minimap_content_revision = 0, minimap_ring_revision = 0;
    std::uint64_t composed_minimap_revision = 0, composed_minimap_ring_revision = 0;
    const auto advance_minimap_revision = [](std::uint64_t &revision) {
        check(revision != std::numeric_limits<std::uint64_t>::max(),
              "Minimap composition revision exhausted");
        ++revision;
    };
    const auto same_minimap_bounds = [](const auto &a, const auto &b) {
        if (a.segment_count != b.segment_count || a.edge_subdivisions != b.edge_subdivisions ||
            a.inverse_edge_steps != b.inverse_edge_steps ||
            a.texture.padding_x != b.texture.padding_x || a.texture.active_width != b.texture.active_width ||
            a.texture.active_aspect != b.texture.active_aspect) return false;
        for (std::uint32_t segment = 0; segment < a.segment_count; ++segment)
            if (a.segments[segment].min_x != b.segments[segment].min_x ||
                a.segments[segment].min_y != b.segments[segment].min_y ||
                a.segments[segment].max_x != b.segments[segment].max_x ||
                a.segments[segment].max_y != b.segments[segment].max_y) return false;
        return true;
    };
    const auto minimap_ring_target_current = [&] {
        if (!live_minimap_viewport) return true;
        if (live_minimap_fog && (wglGetCurrentContext() != minimap_wgl_context ||
            wglGetCurrentDC() != minimap_wgl_dc || WindowFromDC(minimap_wgl_dc) != window)) return false;
        if (!minimap_viewport_pass || minimap_viewport_pass->context != context ||
            !presenter.minimap_chrome || !presenter.minimap_chrome->partial_map) return false;
        GLint attachment = 0, type = 0, level = -1;
        minimap_gl.GetNamedFramebufferAttachmentParameteriv(minimap_viewport_framebuffer,
            GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachment);
        minimap_gl.GetNamedFramebufferAttachmentParameteriv(minimap_viewport_framebuffer,
            GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
        minimap_gl.GetNamedFramebufferAttachmentParameteriv(minimap_viewport_framebuffer,
            GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);
        context->check("Validate minimap viewport-ring target");
        return type == GL_TEXTURE && level == 0 &&
            static_cast<GLuint>(attachment) == presenter.minimap_chrome->partial_map->id() &&
            minimap_gl.CheckNamedFramebufferStatus(minimap_viewport_framebuffer, GL_FRAMEBUFFER) ==
                GL_FRAMEBUFFER_COMPLETE;
    };
    const auto minimap_applied_inputs_current = [&] {
        return live_world_frame_current && live_world_frame_current() && live_minimap_packet &&
            applied_minimap_fog && live_minimap_surface && live_minimap_preparation &&
            saved_minimap_visible && presenter.minimap_chrome && presenter.minimap_chrome->partial_map &&
            same_minimap_bounds(applied_minimap_fog->render_bounds(), *saved_minimap_visible) &&
            live_minimap_surface->current(*applied_minimap_fog, *live_minimap_preparation,
                live_minimap_packet, *presenter.minimap_chrome->partial_map) &&
            live_world_frame_current() && live_camera_current && live_camera_current() &&
            wglGetCurrentContext() == minimap_wgl_context && wglGetCurrentDC() == minimap_wgl_dc;
    };
    if (live_minimap_fog)
        live_minimap_current = [&] {
            return composed_minimap_revision != 0 &&
                composed_minimap_revision == minimap_content_revision &&
                composed_minimap_ring_revision == minimap_ring_revision &&
                minimap_applied_inputs_current() &&
                (!live_minimap_viewport || (applied_minimap_ring_bounds &&
                    same_minimap_bounds(*applied_minimap_ring_bounds, *saved_minimap_visible) &&
                    applied_minimap_ring_camera_current && applied_minimap_ring_camera_current() &&
                    minimap_ring_target_current())) && minimap_applied_inputs_current();
        };
    const auto update_minimap_viewport = [&] {
        // Current fog is independent of whether kind-8 viewport ring is needed.
        if (!live_minimap_fog && !live_minimap_viewport) return;
        if (live_minimap_fog) {
            composed_minimap_revision = 0;
            applied_minimap_fog.reset(); live_minimap_packet.reset();
            applied_minimap_ring_bounds.reset(); applied_minimap_ring_camera_current = {};
            advance_minimap_revision(minimap_content_revision); // Before any target write.
            check(live_world_frame_current && live_world_frame_current(),
                  "Minimap source/camera is stale before CPU preparation");
            live_minimap_packet = live_minimap_preparation->prepare(live_client_scene->frame());
            check(live_world_frame_current(), "Minimap source/camera changed during preparation");
            applied_minimap_fog = live_minimap_surface->apply_current_fog(*live_minimap_preparation,
                live_minimap_packet, *presenter.minimap_chrome->partial_map);
            check(minimap_applied_inputs_current(), "Minimap actual fog application became stale");
        } else {
            // Saved-only diagnostic replay keeps its historical post-fog image.
            // The live path above never uses this image as a pre-fog base.
            const auto &api = context->api();
            const std::array<GLenum, 4> fields{GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH,
                GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS};
            std::array<GLint, 4> previous{};
            GLint previous_buffer = 0;
            api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &previous_buffer);
            for (std::size_t field = 0; field < fields.size(); ++field) {
                api.GetIntegerv(fields[field], &previous[field]);
                api.PixelStorei(fields[field], field == 0 ? 4 : 0);
            }
            api.BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            api.TextureSubImage2D(presenter.minimap_chrome->partial_map->id(), 0,
                0, 0, 320, 190, GL_BGRA, GL_UNSIGNED_BYTE, saved_minimap_base_bgra.data());
            for (std::size_t field = 0; field < fields.size(); ++field)
                api.PixelStorei(fields[field], previous[field]);
            api.BindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(previous_buffer));
        }
        if (live_minimap_viewport) {
            const auto camera = water.camera_state();
            const auto ring_camera_current = [&, rendered = camera] {
                const auto &actual = water.camera_state();
                return live_world_frame_current && live_world_frame_current() &&
                    actual.view_count == rendered.view_count && actual.projection == rendered.projection &&
                    actual.views == rendered.views && actual.inverse_views == rendered.inverse_views &&
                    actual.relative_offsets == rendered.relative_offsets && actual.frusta == rendered.frusta;
            };
            const std::span<const ViewFrustum> frusta(camera.frusta.data(), camera.view_count);
            const float plane_height = post_initialization_scalar(
                native_minimap_bounds ? native_minimap_bounds->height_source : 0.0f,
                terrain->owner().parameters.scale_z);
            if (native_minimap_bounds)
                check(std::abs(plane_height - native_minimap_bounds->plane_height) <= 1e-5f,
                      "Native minimap plane differs from preview height scale");
            const auto corners = civ5::ui::minimap_viewport_corners(frusta, plane_height);
            const auto bounds = *saved_minimap_visible;
            if (live_minimap_fog) {
                check(minimap_applied_inputs_current() && ring_camera_current() && minimap_ring_target_current(),
                      "Minimap source/camera/target is stale before viewport-ring draw");
                advance_minimap_revision(minimap_content_revision);
                advance_minimap_revision(minimap_ring_revision);
            }
            minimap_gl.BindFramebuffer(GL_FRAMEBUFFER, minimap_viewport_framebuffer);
            for (std::uint32_t segment = 0; segment < bounds.segment_count; ++segment) {
                const auto ring = civ5::ui::build_minimap_viewport_ring(
                    corners, bounds.segments[segment], bounds.texture.active_width);
                minimap_viewport_pass->draw(ring, bounds.segments[segment], 0.5f,
                    bounds.texture.padding_x, bounds.texture.active_width);
            }
            minimap_gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
            context->check("Draw minimap final viewport-ring composition");
            if (live_minimap_fog) {
                check(minimap_applied_inputs_current() && ring_camera_current() && minimap_ring_target_current(),
                      "Minimap source/camera/target changed during viewport-ring draw");
                applied_minimap_ring_bounds = bounds;
                applied_minimap_ring_camera_current = ring_camera_current;
            }
        }
        if (live_minimap_fog) {
            check(minimap_applied_inputs_current(), "Minimap fog source changed before composition publication");
            composed_minimap_revision = minimap_content_revision;
            composed_minimap_ring_revision = minimap_ring_revision;
            check(live_minimap_current(), "Minimap completed composition is stale");
            if (smoke) {
                const auto &id = live_minimap_packet->identity();
                const auto &bounds = applied_minimap_fog->render_bounds();
                std::ostringstream receipt;
                receipt << std::setprecision(9)
                    << "{\"schema\":\"civis-minimap-own-gl-application-v1\",\"ownGLApplied\":true,"
                    << "\"independentNativeBaseProof\":false,\"capturedOriginalPixelParity\":false,"
                    << "\"unnotifiedExternalGLWritesDetected\":false,"
                    << "\"clientOwner\":" << id.client.owner << ",\"clientRevision\":" << id.client.revision
                    << ",\"clientFrame\":" << id.client.frame << ",\"stateGeneration\":" << id.state_generation
                    << ",\"player\":" << id.client.player << ",\"team\":" << id.client.team
                    << ",\"turn\":" << id.client.turn << ",\"preparation\":" << id.preparation
                    << ",\"sourceGridSha256\":\"" << id.source_grid_sha256
                    << "\",\"contextSha256\":\"" << id.context_sha256
                    << "\",\"fogPayloadSha256\":\"" << id.fog_payload_sha256
                    << "\",\"prefogSha256\":\"" << applied_minimap_fog->prefog_sha256()
                    << "\",\"fogWriteRevision\":" << applied_minimap_fog->write_revision()
                    << ",\"compositionRevision\":" << composed_minimap_revision
                    << ",\"viewportRingRevision\":" << composed_minimap_ring_revision
                    << ",\"targetId\":" << applied_minimap_fog->target_id()
                    << ",\"nativeExtent\":[320,190],\"nativeWglContext\":\""
                    << reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())
                    << "\",\"nativeWglDC\":\"" << reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())
                    << "\",\"fogTriangles\":" << live_minimap_packet->fog_mesh().triangle_count()
                    << ",\"openedCells\":" << live_minimap_packet->opened_cells()
                    << ",\"viewportRingDrawn\":" << (live_minimap_viewport ? "true" : "false")
                    << ",\"renderBounds\":[" << bounds.segments[0].min_x << ',' << bounds.segments[0].min_y
                    << ',' << bounds.segments[0].max_x << ',' << bounds.segments[0].max_y
                    << "],\"activeWidth\":" << bounds.texture.active_width
                    << ",\"paddingX\":" << bounds.texture.padding_x << "}\n";
                // Each completed composition has a monotonic revision. Preserve
                // every receipt; camera updates must never overwrite an earlier one.
                const auto receipt_path = output / ("minimap-live-fog-own-gl-" +
                    std::to_string(composed_minimap_revision) + ".json");
                blend_validation::text(receipt_path, receipt.str());
                check(live_minimap_current(), "Minimap source changed while writing own GL sidecar");
            }
        }
    };

    update_minimap_viewport();
    if (live_minimap_viewport)
        GlMinimapSurfaceDiagnostic::save_ppm(
            *presenter.minimap_chrome->partial_map, output / "minimap-preview-gpu.ppm");
    // Actual owned target observations; cached width fields alone do not authorize a frame.
    const auto native_scene_target_current=[&] {
        try {
            if(!same_extent_candidate_current() || !target.color_storage || !target.depth_storage ||
                target.context!=context || !target.color_storage->belongs_to(*context) ||
                !target.depth_storage->belongs_to(*context) || target.color!=target.color_storage->id() ||
                target.depth!=target.depth_storage->id()) return false;
            const auto &api=context->api();
            const auto texture_current=[&](GLuint id,GLenum expected_format) {
                if(!api.IsTexture(id)) return false;
                GLint w=0,h=0,format=0;
                api.GetTextureLevelParameteriv(id,0,GL_TEXTURE_WIDTH,&w);
                api.GetTextureLevelParameteriv(id,0,GL_TEXTURE_HEIGHT,&h);
                api.GetTextureLevelParameteriv(id,0,GL_TEXTURE_INTERNAL_FORMAT,&format);
                return w==width && h==height && static_cast<GLenum>(format)==expected_format;
            };
            const auto attached=[&](GLenum point,GLuint texture) {
                GLint id=0,type=0,level=-1;
                target.gl.GetNamedFramebufferAttachmentParameteriv(target.fbo,point,
                    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&id);
                target.gl.GetNamedFramebufferAttachmentParameteriv(target.fbo,point,
                    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&type);
                target.gl.GetNamedFramebufferAttachmentParameteriv(target.fbo,point,
                    GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,&level);
                return static_cast<GLuint>(id)==texture && type==GL_TEXTURE && level==0;
            };
            if(!texture_current(target.color,GL_RGBA32F) ||
                !texture_current(target.depth,GL_DEPTH24_STENCIL8) ||
                !attached(GL_COLOR_ATTACHMENT0,target.color) ||
                !attached(GL_DEPTH_STENCIL_ATTACHMENT,target.depth) ||
                target.gl.CheckNamedFramebufferStatus(target.fbo,GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
                return false;
            context->check("Validate unchanged source camera scene target for viewport publication");
            return same_extent_candidate_current();
        } catch(...) {return false;}
    };
    if(viewport_transaction) presenter.commit_painted_frame=[&] {
        check(drawing_viewport && presenter.paint_frame_current && presenter.paint_frame_current() &&
            native_scene_target_current() && live_ui_host && live_ui_frame && live_screen_frame &&
            last_readonly_text_report,"Native viewport candidate lacks actual composed paint/source/target");
        const auto ticket=*drawing_viewport;
        const auto client_pin=live_client_scene->frame();
        const auto ui_pin=std::make_shared<const civ5::ui::LiveUiFrame>(*live_ui_frame);
        const auto report_pin=*last_readonly_text_report;
        const auto focus_pin=live_screen_frame->focused_context;
        const auto minimap_packet_pin=live_minimap_packet;
        const auto minimap_receipt_pin=applied_minimap_fog;
        const auto minimap_composition_pin=composed_minimap_revision;
        const auto minimap_ring_pin=composed_minimap_ring_revision;
        const GLuint color_pin=target.color,depth_pin=target.depth,fbo_pin=target.fbo;
        const auto observed_pins_current=[&,client_pin,ui_pin,report_pin,focus_pin,
            minimap_packet_pin,minimap_receipt_pin,minimap_composition_pin,minimap_ring_pin,
            color_pin,depth_pin,fbo_pin] {
            if(!live_client_scene || live_client_scene->frame()!=client_pin || !live_ui_frame ||
                !live_screen_frame || !last_readonly_text_report || target.color!=color_pin ||
                target.depth!=depth_pin || target.fbo!=fbo_pin ||
                live_ui_frame->owner!=ui_pin->owner || live_ui_frame->sequence!=ui_pin->sequence ||
                live_ui_frame->snapshot.frame()!=ui_pin->snapshot.frame() ||
                live_screen_frame->frame_owner!=ui_pin->owner ||
                live_screen_frame->frame_sequence!=ui_pin->sequence ||
                live_screen_frame->snapshot_frame!=ui_pin->snapshot.frame() ||
                live_screen_frame->focused_context!=focus_pin ||
                last_readonly_text_report->frame!=report_pin.frame ||
                report_pin.frame!=civ5::ui::MeasuredReadonlyTextFrame{ui_pin->owner,ui_pin->sequence,
                    ui_pin->snapshot.frame(),client_width,client_height} ||
                last_readonly_text_report->regions.size()!=report_pin.regions.size()) return false;
            for(std::size_t n=0;n<report_pin.regions.size();++n) {
                const auto &a=last_readonly_text_report->regions[n],&b=report_pin.regions[n];
                if(a.source_paint_index!=b.source_paint_index || a.context_id!=b.context_id ||
                    a.control_id!=b.control_id || a.tooltip!=b.tooltip || a.bounds.x!=b.bounds.x ||
                    a.bounds.y!=b.bounds.y || a.bounds.width!=b.bounds.width ||
                    a.bounds.height!=b.bounds.height) return false;
            }
            if(live_minimap_fog && (!minimap_packet_pin || !minimap_receipt_pin ||
                live_minimap_packet!=minimap_packet_pin || !applied_minimap_fog ||
                applied_minimap_fog->write_revision()!=minimap_receipt_pin->write_revision() ||
                applied_minimap_fog->target_id()!=minimap_receipt_pin->target_id() ||
                composed_minimap_revision!=minimap_composition_pin ||
                composed_minimap_ring_revision!=minimap_ring_pin)) return false;
            return true;
        };
        const ViewportTransaction::SourceTargetsCurrent source_gate=
            [&,ticket,client_pin,ui_pin,focus_pin,observed_pins_current,
             minimap_packet_pin,minimap_receipt_pin](const ViewportTransaction::Ticket &requested) {
                if(requested.window()!=ticket.window() || requested.version()!=ticket.version() ||
                    requested.viewport()!=ticket.viewport() || !observed_pins_current() ||
                    !native_scene_target_current() || !live_ui_host->is_current_frame(*ui_pin) ||
                    !live_client_scene->accepts(*live_ui_host,*ui_pin) ||
                    live_ui_host->screens().focused_screen()!=focus_pin) return false;
                if(live_minimap_fog && (!live_minimap_current || !live_minimap_current() ||
                    !live_minimap_surface->current(*minimap_receipt_pin,*live_minimap_preparation,
                        minimap_packet_pin,*presenter.minimap_chrome->partial_map))) return false;
                return observed_pins_current() && native_scene_target_current() &&
                    live_client_scene->frame()==client_pin && live_ui_host->is_current_frame(*ui_pin) &&
                    live_client_scene->accepts(*live_ui_host,*ui_pin) &&
                    live_ui_host->screens().focused_screen()==focus_pin &&
                    observed_pins_current() && native_scene_target_current();
            };
        const ViewportTransaction::SourceTargetsCurrent gate=
            [weak=std::weak_ptr<const std::uint8_t>(paint_run_lifetime),source_gate]
            (const ViewportTransaction::Ticket& requested) {
                const auto alive=weak.lock();
                return alive && source_gate(requested);
            };
        const bool camera_gesture_before_commit=
            input.router.gesture_owner()==civ5::ui::ClientInputOwner::camera;
        check(viewport_transaction->commit(ticket,candidate_hits,input.live_modal_open,input.search_focus,gate),
              "Native same-extent viewport publication rejected");
        // set_ui() may cancel the current camera gesture when a modal opens.
        // Preserve that cancellation for the existing camera producer boundary.
        if(camera_gesture_before_commit &&
            input.router.gesture_owner()!=civ5::ui::ClientInputOwner::camera)
            input.cancel=true;
        published_viewport=ticket;
        published_viewport_gate=gate;
        check(viewport_transaction->current(ticket,gate),"Native viewport changed after actual paint commit");
    };


    struct PendingManualCycleInput {
        civ5::ui::BnwManualCycleSelectionCommand command;
        civ5::ui::LiveUiFrame ui;
        std::shared_ptr<const civ5::ui::BnwClientFrame> frame;
        ViewportTransaction::Ticket ticket;
        ViewportTransaction::SourceTargetsCurrent gate;
    };
    civ5::ui::BnwManualCycleKeyInput manual_cycle_keys;
    std::vector<PendingManualCycleInput> pending_manual_cycles;
    struct ObservedTechAwardInput {
        civ5::ui::BnwTechAwardInput command;
        civ5::ui::LiveUiFrame ui;
        std::shared_ptr<const civ5::ui::BnwClientFrame> frame;
        ViewportTransaction::Ticket ticket;
        ViewportTransaction::SourceTargetsCurrent gate;
        std::weak_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner> source;
    };
    std::optional<ObservedTechAwardInput> observed_tech_award_input;
    struct ObservedTechAwardPress {
        ObservedTechAwardInput request;
        std::array<float,4> bounds;
    };
    std::optional<ObservedTechAwardPress> observed_tech_award_press;
    bool world_plot_pressed=false,world_plot_dragged=false;
    civ5::ui::ClientPoint world_plot_press{};
    std::function<void(const civ5::ui::ClientWindowEvent&)> native_world_plot_input;
    if(prepared_viewer->existing_menu_binding)prepared_viewer->native_pointer_capture=
        [&,weak=std::weak_ptr<const std::uint8_t>(paint_run_lifetime)](const civ5::ui::ClientWindowEvent& event){
            if(weak.expired())throw std::runtime_error("Party native capture target retired");
            if(event.kind!=civ5::ui::ClientWindowEventKind::pointer ||
               event.pointer_kind!=civ5::ui::ClientPointerKind::move)return;
            const auto* award=live_ui_frame ? live_ui_frame->snapshot.find<civ5::ui::BnwTechAwardContextSnapshot>() : nullptr;
            if(!award || !award->context.visible || !award->context.queued || !live_screen_frame ||
               live_screen_frame->focused_context!=std::optional<std::string>{"TechAwardPopup"})return;
            check(event.pointer_move&&live_screen_painter&&live_client_scene&&live_ui_frame,
                  "Party TechAward native pointer capture has no actual displayed target");
            live_screen_painter->capture_tech_award_pointer(event.pointer_move,*live_ui_frame,live_client_scene->frame());
        };
    if(viewport_transaction && live_bnw_preview) native_world_plot_input=
        [&,weak=std::weak_ptr<const std::uint8_t>(paint_run_lifetime)](const civ5::ui::ClientWindowEvent& event) {
            using Kind=civ5::ui::ClientWindowEventKind;using Pointer=civ5::ui::ClientPointerKind;
            const auto alive=weak.lock();if(!alive)return;
            // SAME completed-release exception as camera_preview_event: the
            // binding delivers left_up before synchronous ReleaseCapture.
            // Focus loss, active capture loss, resize and close still cancel.
            if(event.kind==Kind::cancelled&&event.native_flags==WM_CAPTURECHANGED&&
               !event.pointer.cancel_camera&&!input.pointer_left_down)return;
            if(event.kind==Kind::cancelled || event.kind==Kind::close ||
               event.kind==Kind::resize || event.kind==Kind::dpi_changed || event.pointer.cancel_camera) {
                world_plot_pressed=false;world_plot_dragged=false;
                observed_tech_award_input.reset();observed_tech_award_press.reset();
                manual_cycle_keys.cancel();pending_manual_cycles.clear();return;
            }
            // Data capture only on the real transaction-delivered Windows event.
            // No provider/VM call executes on the native input stack. Retain the
            // frame actually shown, never retarget the event to next begin_frame.
            const auto* award=live_ui_frame?
                live_ui_frame->snapshot.find<civ5::ui::BnwTechAwardContextSnapshot>():nullptr;
            const bool award_modal=award&&award->context.visible&&award->context.queued&&
                live_screen_frame&&live_screen_frame->focused_context==
                    std::optional<std::string>{"TechAwardPopup"};
            if(award_modal) {
                if(event.kind==Kind::pointer&&event.pointer_kind==Pointer::move) {
                    check(event.pointer_move&&live_screen_painter&&live_client_scene&&live_ui_frame,
                          "TechAward WM_MOUSEMOVE has no actual native/renderer loan");
                    if(!prepared_viewer->existing_menu_binding)
                        live_screen_painter->capture_tech_award_pointer(event.pointer_move,*live_ui_frame,live_client_scene->frame());
                    return; // Existing-menu native capture already ran before Return.
                }
                const auto capture=[&](civ5::ui::BnwTechAwardInput command) {
                    check(live_client_scene&&live_ui_host&&published_viewport&&published_viewport_gate&&
                          prepared_viewer->stock_source_owner&&!viewport_retired_pending&&
                          published_viewport->viewport()==input.router.viewport(),
                          "TechAward input lacks actual published renderer/source/viewport");
                    check(!pending_tech_award_input&&!observed_tech_award_input,
                          "TechAward input already queued or awaiting a new frame");
                    return ObservedTechAwardInput{command,*live_ui_frame,live_client_scene->frame(),
                        *published_viewport,published_viewport_gate,prepared_viewer->stock_source_owner};
                };
                if(event.kind==Kind::key_down) {
                    if((event.native_flags&(std::uint32_t{1}<<30))==0&&
                       (event.key==VK_ESCAPE||event.key==VK_RETURN))
                        observed_tech_award_input=capture(event.key==VK_ESCAPE?
                            civ5::ui::BnwTechAwardInput::escape:civ5::ui::BnwTechAwardInput::return_key);
                    return;
                }
                if(event.kind==Kind::key_up)return;
                if(event.kind==Kind::pointer&&event.pointer.framebuffer) {
                    const auto point=*event.pointer.framebuffer;
                    const auto viewport=input.router.viewport();
                    const auto layout=award->context.client_layout;
                    check(published_viewport&&!viewport_retired_pending&&viewport.drawable()&&
                          published_viewport->viewport()==viewport&&layout&&layout->viewport==viewport&&
                          live_client_scene&&live_client_scene->frame()&&
                          award->publication.viewport==viewport&&
                          live_client_scene->frame()->stamp.viewport==viewport,
                          "TechAward pointer lacks SAME published source/client/framebuffer viewport");
                    // Input producer already scaled native client -> framebuffer.
                    // Invert that single actual ratio for authored CLIENT rects;
                    // no DPI scale or guessed window size is applied here.
                    const double client_x=double(point.x)*viewport.client_width/viewport.framebuffer_width;
                    const double client_y=double(point.y)*viewport.client_height/viewport.framebuffer_height;
                    const auto inside=[&](const std::array<float,4>& r) {
                        return r[2]>0&&r[3]>0&&client_x>=r[0]&&client_y>=r[1]&&
                            client_x<double(r[0])+r[2]&&client_y<double(r[1])+r[3];
                    };
                    if(event.pointer_kind==Pointer::left_down) {
                        observed_tech_award_press.reset();
                        check(layout->viewport.client_width==client_width&&
                              layout->viewport.client_height==client_height,
                              "TechAward input layout differs from actual client viewport");
                        // Source-authored Button nodes and current measured geometry;
                        // image/label descendants do not manufacture command slots.
                        for(auto node=layout->nodes.rbegin();node!=layout->nodes.rend();++node) {
                            if(node->hidden||(node->kind!="Button"&&node->kind!="GridButton")||!inside(node->rect))continue;
                            if(node->id=="ContinueButton"||node->id=="CloseButton")
                                observed_tech_award_press=ObservedTechAwardPress{capture(node->id=="ContinueButton"?
                                    civ5::ui::BnwTechAwardInput::continue_button:civ5::ui::BnwTechAwardInput::close_button),node->rect};
                            break;
                        }
                    } else if(event.pointer_kind==Pointer::left_up) {
                        // This real modal owns the release; a non-button hit
                        // is not passed to unrelated generic popup commands.
                        input.top_panel_click_pending=false;
                        if(observed_tech_award_press) {
                            const auto& press=*observed_tech_award_press;
                            if(inside(press.bounds)&&press.request.ui.owner==live_ui_frame->owner&&
                               press.request.ui.sequence==live_ui_frame->sequence&&
                               press.request.ui.snapshot.same_payload_storage(live_ui_frame->snapshot)&&
                               press.request.frame==live_client_scene->frame()) {
                                check(!observed_tech_award_input,"TechAward duplicate native input");
                                observed_tech_award_input=press.request;
                            }
                        }
                        observed_tech_award_press.reset();
                    }
                    return;
                }
            } else observed_tech_award_press.reset();
            if(event.kind==Kind::key_down || event.kind==Kind::key_up) {
                const bool world_input=event.owner==civ5::ui::ClientInputOwner::camera &&
                    !input.search_focus && !input.live_modal_open && !input.top_panel_click_pending &&
                    !input.minimap_click_pending && !input.coordinate_cleanup_pending;
                const auto filter=manual_cycle_keys.key(event.key,event.kind==Kind::key_down,
                    (event.native_flags&(std::uint32_t{1}<<30))!=0,world_input,
                    (GetKeyState(VK_CONTROL)&0x8000)!=0,(GetKeyState(VK_MENU)&0x8000)!=0,
                    (GetKeyState(VK_SHIFT)&0x8000)!=0);
                if(filter && font_source_parent && live_client_scene && live_ui_host && live_ui_frame &&
                    published_viewport && published_viewport_gate && !viewport_retired_pending) {
                    // Retain the publication actually shown when the key arrived.
                    // The queue cannot retarget this intent to the next begin().
                    check(pending_manual_cycles.size()<64,"Manual cycle input queue overflow");
                    PendingManualCycleInput request{{civ5::runtime::BnwUnitCycleDirection::forward,*filter},
                        *live_ui_frame,live_client_scene->frame(),*published_viewport,published_viewport_gate};
                    const auto current=[&] {
                        return request.frame && request.gate && viewport_transaction &&
                            viewport_transaction->current(request.ticket,request.gate) &&
                            live_client_scene->frame()==request.frame &&
                            civ5::ui::BnwManualCycleKeyInput::world_frame(*live_client_scene,*live_ui_host,request.ui);
                    };
                    if(current() && live_screen_frame && !live_screen_frame->focused_context &&
                        live_screen_frame->frame_owner==request.ui.owner &&
                        live_screen_frame->frame_sequence==request.ui.sequence && current())
                        pending_manual_cycles.push_back(std::move(request));
                }
                return;
            }
            if(event.kind!=Kind::pointer)return;
            const bool camera_owned=event.pointer.owner==civ5::ui::ClientInputOwner::camera &&
                !input.live_modal_open && !input.live_ui_pressed && !input.minimap_pressed;
            if(event.pointer_kind==Pointer::left_down) {
                world_plot_pressed=camera_owned && event.pointer.capture && event.pointer.framebuffer.has_value();
                world_plot_dragged=false;
                if(world_plot_pressed)world_plot_press=*event.pointer.framebuffer;
                return;
            }
            if(event.pointer_kind==Pointer::move && world_plot_pressed &&
               (!event.pointer.framebuffer || *event.pointer.framebuffer!=world_plot_press))
                world_plot_dragged=true;
            if(event.pointer_kind!=Pointer::left_up)return;
            const bool click=world_plot_pressed && !world_plot_dragged && camera_owned &&
                event.pointer.framebuffer && *event.pointer.framebuffer==world_plot_press && event.pointer.release;
            world_plot_pressed=false;world_plot_dragged=false;if(!click)return;
            const auto ticket=published_viewport;const auto gate=published_viewport_gate;
            const auto picked=world_pick_frame;
            const auto current=[&] {
                return ticket && gate && world_pick_depth && picked &&
                    prepared_viewer->thread==GetCurrentThreadId() &&
                    world_pick_depth->belongs_to(*context) && native_scene_target_current() &&
                    live_client_scene && live_client_scene->frame()==picked && live_client_scene->current() &&
                    viewport_transaction->current(*ticket,gate);
            };
            if(!current())return;
            const auto point=*event.pointer.framebuffer;const auto &viewport=ticket->viewport();
            std::optional<std::int64_t> plot;
            if(point.x>=0 && point.y>=0 && point.x<viewport.framebuffer_width &&
               point.y<viewport.framebuffer_height) {
                // Same center/nearest texel mapping as PreviewPresenter's scene shader.
                const auto x=static_cast<GLint>((std::int64_t(point.x)*2+1)*width/(2*std::int64_t(viewport.framebuffer_width)));
                const auto y=static_cast<GLint>((std::int64_t(point.y)*2+1)*height/(2*std::int64_t(viewport.framebuffer_height)));
                const auto &api=context->api();
                const std::array<GLenum,8> fields{GL_PACK_ALIGNMENT,GL_PACK_ROW_LENGTH,GL_PACK_SKIP_ROWS,
                    GL_PACK_SKIP_PIXELS,GL_PACK_IMAGE_HEIGHT,GL_PACK_SKIP_IMAGES,GL_PACK_SWAP_BYTES,GL_PACK_LSB_FIRST};
                std::array<GLint,8> prior{};GLint buffer=0;
                api.GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&buffer);
                for(std::size_t n=0;n<fields.size();++n) {
                    api.GetIntegerv(fields[n],&prior[n]);api.PixelStorei(fields[n],n==0?1:0);
                }
                api.BindBuffer(GL_PIXEL_PACK_BUFFER,0);float depth=1;
                api.GetTextureSubImage(world_pick_depth->id(),0,x,y,0,1,1,1,GL_DEPTH_COMPONENT,
                    GL_FLOAT,sizeof(depth),&depth);
                api.BindBuffer(GL_PIXEL_PACK_BUFFER,static_cast<GLuint>(buffer));
                for(std::size_t n=0;n<fields.size();++n)api.PixelStorei(fields[n],prior[n]);
                context->check("Read one actual owned world pick depth texel");
                if(!current())return;
                try {
                    plot=civ5::ui::world_plot_from_rendered_depth(*picked,terrain->source().layout,
                        terrain->owner().parameters,world_pick_camera,x,y,width,height,depth,rcp);
                } catch(const std::invalid_argument&) {
                    return; // Reject uncertain geometry; no canonical write or GPU-error suppression.
                }
            }
            if(!current())return;
            live_client_scene->select_plot(plot,picked);
            world_pick_frame.reset();input.top_panel_click_pending=false;
            input.cancel=true;input.events.clear();
            live_ui_frame.reset();live_screen_frame.reset();
            last_readonly_text_report.reset();last_readonly_tooltip_text.reset();
            presenter.live_top_panel_presentation.reset();
        };

    bool worldview_font_gate_completed=false;
    const auto verify_worldview_font_source=[&] {
        if(!worldview_font_source_gates || worldview_font_gate_completed) return;
        check(worldview_font_source && font_source_parent && live_client_scene && live_ui_host &&
              live_ui_frame && live_ui_frame->world_view && live_screen_frame && published_viewport && published_viewport_gate &&
              viewport_transaction,"WorldView font gate lacks actual published camera/UI owners");
        const auto accepted=live_client_scene->frame();
        const auto ui_pin=*live_ui_frame;
        const auto composed_pin=*live_screen_frame;
        const auto& paints=composed_pin.paints;
        std::optional<civ5::ui::WorldViewPaint> current_title_command;
        for(const auto& paint:paints) if(paint.unit_name_font_selection) {
            check(!current_title_command,"WorldView font gate has ambiguous actual UnitName paint");
            current_title_command=paint;
        }
        if(!current_title_command) return; // Never select/fabricate a title just to pass.
        const auto ticket=*published_viewport;
        const auto current=[&] {
            return prepared_viewer->initial_epoch_current() && live_client_scene->frame()==accepted &&
                live_client_scene->current() && live_ui_host->is_current_frame(ui_pin) &&
                live_client_scene->accepts(*live_ui_host,ui_pin) &&
                live_ui_frame && live_ui_frame->owner==ui_pin.owner && live_ui_frame->sequence==ui_pin.sequence &&
                live_ui_frame->snapshot.frame()==ui_pin.snapshot.frame() && live_screen_frame &&
                live_screen_frame->frame_owner==composed_pin.frame_owner &&
                live_screen_frame->frame_sequence==composed_pin.frame_sequence &&
                live_screen_frame->snapshot_frame==composed_pin.snapshot_frame &&
                live_screen_frame->focused_context==composed_pin.focused_context &&
                composed_pin.frame_owner==ui_pin.owner && composed_pin.frame_sequence==ui_pin.sequence &&
                composed_pin.snapshot_frame==ui_pin.snapshot.frame() &&
                viewport_transaction->current(ticket,published_viewport_gate);
        };
        check(current(),"WorldView font gate current composed publication stale");
        const auto create_title_issuer=[&] {
            return actual_own_font_source
                ? civ5::ui::BnwWorldViewTitleFontIssuer::create_from_civis_resources(
                    font_source_parent,live_client_scene->owner(),accepted,worldview_font_source)
                : civ5::ui::BnwWorldViewTitleFontIssuer::create_from_adopted_host(
                    font_source_parent,live_client_scene->owner(),accepted,worldview_font_source);
        };
        auto issuer=create_title_issuer();
        const auto issued=issuer->current(); // Copy before policy/resource/provider callbacks.
        const auto& a=*current_title_command;const auto& b=issued.title;
        const auto rectangle=[](const auto& r){return std::tuple{r.x,r.y,r.width,r.height};};
        const auto request=[](const auto& r){return std::tuple{r.source_xml_key,r.context_id,r.control_id,
            r.text,r.style,r.player_id,r.selected_unit_id,r.snapshot_frame,r.turn,r.button_width};};
        check(a.kind==b.kind && a.text==b.text && a.style==b.style && a.rgba==b.rgba && a.tint==b.tint &&
              rectangle(a.destination)==rectangle(b.destination) &&
              a.clip.has_value()==b.clip.has_value() && (!a.clip||rectangle(*a.clip)==rectangle(*b.clip)) &&
              a.unit_name_font_selection && b.unit_name_font_selection &&
              request(*a.unit_name_font_selection)==request(*b.unit_name_font_selection) &&
              a.font_style && b.font_style && a.font_style->font==b.font_style->font &&
              a.font_style->style==b.font_style->style && a.font_style->colors==b.font_style->colors &&
              a.font_style->float_colors==b.font_style->float_colors &&
              &issuer->resources()==ui_resources,
              "Issuer title/source differs from actual current UnitName command");
        const auto refusal=[](auto&& action) {try {action();}catch(const std::exception&){return true;}return false;};
        const auto copied=issuer->descriptor(0);
        check(refusal([&]{issuer->require_descriptor(copied);}),"Copied WorldView descriptor accepted");
        const auto foreign=create_title_issuer();
        const auto* foreign_descriptor=&foreign->descriptor(0);
        check(refusal([&]{issuer->require_descriptor(*foreign_descriptor);}),
              "Foreign issuer descriptor accepted by current WorldView owner");
        auto policy=civ5::ui::BnwClientTextPolicyOwner::create_from_worldview_title(issuer);
        const auto policy_frame=policy->current();
        check(policy_frame.backend==civ5::ui::BnwClientTextBackend::native_system_font,
              "Selected effective Languages policy does not admit native-system-font diagnostic");
        check(refusal([&]{(void)civ5::ui::BnwSystemFontCpuOwner::create_from_worldview_title_descriptor(
            policy,copied);}) && refusal([&]{
                (void)civ5::ui::BnwSystemFontCpuOwner::create_from_worldview_title_descriptor(
                    policy,*foreign_descriptor);}),"Copied/foreign descriptor reached GDI construction");
        const auto json=[](std::string_view s) {
            std::ostringstream o;o << '"';
            for(const unsigned char c:s) {
                if(c=='"'||c=='\\') o << '\\' << static_cast<char>(c);
                else if(c<32) o << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << unsigned(c) << std::dec;
                else o << static_cast<char>(c);
            }o << '"';return o.str();
        };
        std::ostringstream diagnostic;
        diagnostic << "{\"schema\":\"civis-worldview-current-font-source-gates-v1\","
            "\"ownCurrentTitleAdmission\":true,\"composedFramePaintCommitted\":true,\"actualUnitNameCommandPresent\":true,"
            "\"actualUnitNameGlyphDrawVerified\":false,\"nativeSelectedFont\":false,\"copiedDescriptorRefused\":true,"
            "\"foreignDescriptorRefused\":true,\"nativeLabelReady\":false,\"nativeContextReady\":false,"
            "\"nativeGetSizeVal\":false,\"wholeRunCLASSINVerified\":false,\"rendererEffectsExtended\":false,"
            "\"brightnessReinterpretedAsAlpha\":false,\"atlasPairUploaded\":false,\"nativePixelParity\":false,"
            "\"publicationOwner\":" << accepted->stamp.identity.owner << ",\"publicationRevision\":"
            << accepted->stamp.identity.revision << ",\"publicationFrame\":" << accepted->stamp.identity.frame
            << ",\"selectedUnit\":" << *accepted->stamp.selection.unit << ",\"uiSnapshotFrame\":"
            << ui_pin.snapshot.frame() << ",\"titleUtf8\":" << json(a.text) << ",\"language\":"
            << json(issued.locale.language) << ",\"effectiveFontName\":" << json(issued.language_row.font_name)
            << ",\"widthScale\":" << std::setprecision(std::numeric_limits<float>::max_digits10)
            << issued.language_row.width_scale << ",\"heightScale\":" << issued.language_row.height_scale
            << ",\"candidates\":[";
        for(std::size_t n=0;n<3;++n) {
            check(current(),"WorldView publication changed before actual GDI candidate");
            const auto* descriptor=&issuer->descriptor(n);
            const auto descriptor_copy=*descriptor;
            issuer->require_descriptor(*descriptor);
            auto cpu=civ5::ui::BnwSystemFontCpuOwner::create_from_worldview_title_descriptor(policy,*descriptor);
            const auto cpu_frame=cpu->current();
            auto cache=civ5::ui::BnwSystemFontCacheOwner::create(cpu);
            check(cpu_frame.measurement_available && cpu_frame.cpu_raster_available && !cpu_frame.gpu_pair_ready,
                  "Actual WorldView CPU font readiness scope differs");
            if(n) diagnostic << ',';
            diagnostic << "{\"fontKey\":" << json(descriptor_copy.font_key()) << ",\"fontSha256\":"
                << json(descriptor_copy.font_sha256()) << ",\"adjustedAscent\":" << descriptor_copy.adjusted_ascent()
                << ",\"sourceLineHeight\":" << descriptor_copy.source_line_height() << ",\"actualFace\":"
                << json(cpu_frame.actual_face) << ",\"fontBinarySha256\":" << json(cpu_frame.font_binary_sha256)
                << ",\"logfontWidth\":" << cpu_frame.logfont_width << ",\"logfontHeight\":" << cpu_frame.logfont_height
                << ",\"cellWidth\":" << cpu_frame.cell_width << ",\"cellHeight\":" << cpu_frame.cell_height
                << ",\"gcpFlags\":" << cpu_frame.gcp_flags << ",\"samples\":[";
            const std::array<std::uint32_t,3> codepoints{'A',0x0416,' '};
            for(std::size_t k=0;k<codepoints.size();++k) {
                const auto cp=codepoints[k];
                const auto measured=cpu->measure(cp);
                check(measured.glyph_index!=0 && measured.glyph_index!=0xffff && measured.pointers_preserved,
                      "Actual GDI ASCII/Russian/space glyph is unavailable");
                const auto quote=cache->measure(cp);
                const auto stage=cache->stage_raster(cp,quote);
                const auto repeated=cache->stage_raster_glyph_index(quote);
                const auto& pixels=stage.cpu;
                check(cache->current(quote) && quote.slot.glyph_index==measured.glyph_index &&
                      quote.slot.advance==measured.advance && pixels.glyph_index==measured.glyph_index &&
                      pixels.width==measured.advance && pixels.height==cpu_frame.cell_height && pixels.width>=0 &&
                      pixels.width<=cpu_frame.cell_width &&
                      pixels.green_brightness.size()==static_cast<std::size_t>(pixels.width)*pixels.height &&
                      pixels.shadow_brightness.size()==pixels.green_brightness.size() &&
                      repeated.cpu.green_brightness==pixels.green_brightness &&
                      repeated.cpu.shadow_brightness==pixels.shadow_brightness &&
                      !stage.atlas_pair_uploaded && !quote.slot.raster_uploaded,
                      "Actual GDI full-height raster/cache repeat or scope mismatch");
                const auto ink=std::count_if(pixels.green_brightness.begin(),pixels.green_brightness.end(),
                    [](std::uint8_t p){return p!=255;});
                check(cp==' ' ? ink==0 : ink>0,"Actual ASCII/Russian ink or space zero-ink raster mismatch");
                if(k) diagnostic << ',';
                diagnostic << "{\"codepoint\":" << cp << ",\"actualGlyphId\":" << measured.glyph_index
                    << ",\"classInput\":" << unsigned(measured.class_input) << ",\"advanceAB\":" << measured.advance
                    << ",\"payloadWidth\":" << pixels.width << ",\"payloadHeight\":" << pixels.height
                    << ",\"inkPixels\":" << ink << ",\"greenBrightnessSha256\":"
                    << json(civ5::runtime::identify_bytes(pixels.green_brightness).sha256)
                    << ",\"filteredBrightnessSha256\":"
                    << json(civ5::runtime::identify_bytes(pixels.shadow_brightness).sha256) << '}';
            }
            diagnostic << "]}";
            issuer->require_descriptor(*descriptor);(void)cpu->current();
            check(current(),"WorldView publication changed during actual GDI candidate");
        }
        #ifdef CIV5_WORLDVIEW_SYSTEM_TITLE_MEASURE_GATES
        const auto measured_title=civ5::tests::exercise_current_worldview_system_title_run(issuer);
        check(current(),"WorldView title measurement hook retired the actual committed source");
        std::ostringstream run_diagnostic;
        run_diagnostic << "{\"schema\":\"civis-current-worldview-system-title-measure-v1\","
            "\"actualCurrentTitle\":true,\"independentActualGdiOraclePassed\":true,"
            "\"realResourceReentryAndFacadeRetirementGatesPassed\":true,"
            "\"sourceRetirementGateExecuted\":false,\"atlasPairUploaded\":false,"
            "\"shaderDrawAccepted\":false,\"windowCompositionAccepted\":false,"
            "\"nativeGetSizeVal\":false,\"nativeLabelReady\":false,\"originalIncomingCwObserved\":false,"
            "\"titleUtf8\":" << json(measured_title.source.title.text)
            << ",\"selectedCandidate\":" << measured_title.selected_candidate
            << ",\"ownCursorCw\":" << measured_title.own_x87_control_word
            << ",\"widthReadIndices\":[" << measured_title.measured_indices[0] << ',' << measured_title.measured_indices[1]
            << "],\"widthReadValues\":[" << measured_title.measured_widths[0] << ',' << measured_title.measured_widths[1]
            << "],\"measureCalls\":[" << measured_title.measure_calls[0] << ',' << measured_title.measure_calls[1] << ','
            << measured_title.measure_calls[2] << "],\"drawWidth\":" << measured_title.draw_width
            << ",\"sourceLineHeight\":" << measured_title.source_line_height
            << ",\"sourceAdjustedAscent\":" << measured_title.source_label_ascent
            << ",\"glyphCount\":" << measured_title.pending_run.gcp.glyphs.size() << "}\n";
        (void)issuer->current();check(current(),"WorldView title measurement stale before receipt publication");
        blend_validation::text(output/"worldview-system-title-measure-gates.json",run_diagnostic.str());
        check(current(),"WorldView title measurement stale after receipt publication");
        #endif
        diagnostic << "],\"currentBeforePublication\":true}\n";
        (void)issuer->current();worldview_font_source->require_current();
        check(current(),"WorldView font gate stale before diagnostic publication");
        blend_validation::text(output/"worldview-font-source-gates.json",diagnostic.str());
        check(current(),"WorldView font gate stale after diagnostic publication");
        worldview_font_gate_completed=true;
    };
    PreparedCameraPublicationGuard publication_guard(prepared_viewer,paint_run_lifetime);
    struct RetireActualClientAudio {
        std::shared_ptr<PreparedCameraViewer> viewer;
        ~RetireActualClientAudio()noexcept{viewer->retire_client_audio();}
    } retire_actual_client_audio{prepared_viewer};
    // Declared after publication guard: authentic failure retirement happens
    // before borrowed GUI/source/paint callbacks or their dependencies unwind.
    struct RetireTechAwardInput {
        std::shared_ptr<civ5::ui::LiveUiHost>& ui;
        std::shared_ptr<const civ5::ui::BnwTechAwardInputDispatch>& token;
        ~RetireTechAwardInput() noexcept {
            if(ui&&token)ui->retire_tech_award_input_after_failure(token);
        }
    } retire_tech_award_input{live_ui_host,pending_tech_award_input};
    prepared_viewer->world_plot_input=std::move(native_world_plot_input);
    // Declared after publication guard and ALL borrowed camera dependencies:
    // retire the source before metadata closures/dependencies unwind, even throw.
    RetireWorldViewFontSource published_font_retirement{worldview_font_source};
    if(parent_prepared) prepared_viewer->require_initial_ticket(); // Exact adoption-to-initial-paint fence.
    if(actual_alpha_frame)actual_alpha_frame->require_camera_boundary_owned();
    presenter.draw(*context, target.color, client_width, client_height, input.pointer_x, input.pointer_y,
                   input.pointer_valid, false, false, -1, -1, -1, -1, output / "preview.ppm",
                   smoke && smoke_fixture_tooltips_expected, input.minimap_options_visible,
                   &input.minimap_map_display);
    if(actual_alpha_frame)actual_alpha_frame->require_camera_boundary_owned();
    record_panel();
    verify_worldview_font_source();
    if(early_initial_smoke) {
        check(prepared_viewer->initial_epoch_current() && viewport_transaction && published_viewport &&
              published_viewport->version()==prepared_viewer->initial_ticket->version() &&
              published_viewport_gate && viewport_transaction->current(*published_viewport,published_viewport_gate) &&
              live_client_scene && live_ui_host && live_ui_frame &&
              live_ui_host->is_current_frame(*live_ui_frame) &&
              live_client_scene->accepts(*live_ui_host,*live_ui_frame) &&
              prepared_viewer->initial_epoch_current(),
              "EARLY initial smoke lost actual composed/painted source frame");
        blend_validation::text(output / "early-initial-viewer-frame.json",
            "{\"schema\":\"civis-early-initial-viewer-frame-v1\",\"actualComposedPaintCommitted\":true,"
            "\"sameParentOwnedHWND\":true,\"initialClientRevision\":"+
            std::to_string(prepared_viewer->initial_ticket->version())+
            ",\"nativeLabelInitialized\":false,\"nativePixelParity\":false,\"full63\":false}\n");
        check(!worldview_font_source_gates || worldview_font_gate_completed,
              "WorldView font gate has no ACTUAL current selected UnitName; no title fabricated");
        if(worldview_font_source) worldview_font_source->retire();
        publication_guard.retire_borrowed_callbacks(); // Before explicit GPU dependency cleanup.
        minimap_viewport_pass.reset();
        if(minimap_viewport_framebuffer) minimap_gl.DeleteFramebuffers(1,&minimap_viewport_framebuffer);
        return; // Parent retains HWND/transaction through final named-launch guards.
    }
    // Target the actual painted controls, independently of fixture tooltip
    // coverage: native text changes alter the authored hit widths.
    const bool smoke_science_action_expected=presenter.last_science_hit &&
        presenter.last_science_hit->width>0;
    const bool smoke_gold_hover_expected=presenter.last_gold_hit &&
        presenter.last_gold_hit->width>0 &&
        presenter.top_panel_controls.presentation().gold_tooltip.has_value();
    if (smoke && smoke_fixture_tooltips_expected) {
        check(presenter.last_tooltips_validated >= 8, "TopPanel fixture tooltip projection was incomplete");
        blend_validation::text(output / "top-panel-tooltip-projection.txt",
                               "PASS projected=" + std::to_string(presenter.last_tooltips_validated) + "\n");
    }
    SetWindowTextW(window, L"Civ5 camera | Drag LMB / Wheel | WASD / Arrows | Q "
                           L"E rotate | R reset | I "
                           L"inertia | Z straight zoom | Esc");
    if (!smoke) {
        ShowWindow(window, SW_SHOW);
        SetForegroundWindow(window);
    }
    blend_validation::text(output / "ready.txt", "READY camera preview, own C++ GL caches and live camera\n");
    std::cout << "READY camera preview: LMB drag, wheel zoom, WASD/arrows pan, "
                 "Q/E rotate, R reset, I "
                 "inertia, Z straight zoom, Esc close\n"
              << std::flush;
    using Clock = std::chrono::steady_clock;
    const auto started = Clock::now();
    WaterFrameClock clock;
    clock.tick(0);
    unsigned ticks = 0, captures = 0, releases = 0, inertia_ticks = 0, resets = 0,
             culling_checks = smoke ? 1 : 0;
    bool drag_changed = false, zoom_changed = false;
    const bool smoke_readonly_expected = live_bnw_preview &&
        live_bnw_preview->frame_sources.unit_stats_database &&
        live_bnw_preview->frame_sources.unit_stats_types &&
        live_bnw_preview->frame_sources.unit_stats_identity &&
        live_bnw_preview->frame_sources.selected_unit &&
        live_bnw_preview->frame_sources.selected_unit_stat_boxes;
    std::optional<std::string> smoke_strength_tooltip;
    bool smoke_strength_hover_verified=false, smoke_strength_clear_verified=false;
    const bool smoke_tech_tree_close_expected = smoke_science_action_expected &&
        live_bnw_preview && live_tech_tree_lifecycle;
    const unsigned smoke_exit_tick = smoke_tech_tree_close_expected ? 63u : 59u;
    // Navigation happens after this tick's presented frame. Allow the next
    // paint to publish the new context before another physical input arrives.
    const unsigned smoke_science_tick = smoke_tech_tree_close_expected ? 58u : 57u;
    const unsigned smoke_gold_tick = smoke_tech_tree_close_expected ? 62u : 58u;
    std::optional<std::string> smoke_tech_tree_parent;
    std::optional<civ5::ui::ProgressionTechnologyTreeEvent> smoke_tech_tree_occurrence;
    bool smoke_tech_tree_close_posted=false, smoke_tech_tree_close_verified=false;
    bool smoke_tech_tree_pre_window_close_current_verified=false;
    std::ofstream telemetry(output / "camera.csv");
    telemetry << "tick,dt,x,y,z,zoom,velocityX,velocityY,views,landDraws,inputMs,"
                 "renderMs,presentMs\n";
    const auto post = [&](UINT m, WPARAM w, LPARAM l) {
        check(PostMessageW(window, m, w, l) != FALSE, "Cannot post preview smoke input");
    };
    std::int32_t smoke_drag_x = client_width / 2;
    const std::int32_t smoke_drag_y = client_height / 2;
    if (smoke) {
        const auto clear_of_ui = [&](std::int32_t x) {
            return std::none_of(input.live_hit_regions.begin(),
                input.live_hit_regions.end(), [&](const auto &rect) {
                    return x <= rect.x + rect.width && x + 65 >= rect.x &&
                        smoke_drag_y <= rect.y + rect.height &&
                        smoke_drag_y + 26 >= rect.y;
                });
        };
        for (const auto candidate : {client_width / 2, client_width / 3,
                                     client_width / 4})
            if (clear_of_ui(candidate)) { smoke_drag_x = candidate; break; }
        check(clear_of_ui(smoke_drag_x),
              "Preview smoke camera gesture has no UI-free starting path");
    }
    civ5::runtime::BnwOwnedGameplayEventPumpLoan::Clock gameplay_pump_clock;
    while (!input.close) {
        scroll_loop_tick.reset();
        struct EndScrollTick{std::unique_ptr<civ5::ui::BnwScrollAnimationLoopTick>&p;~EndScrollTick(){p.reset();}} end_scroll_tick{scroll_loop_tick};
        const auto frame_start = Clock::now();
        //Previous actual UI publications are consumed outside render/native
        //mutation. No sound is created from this clock or camera movement.
        if(prepared_viewer->client_audio){try{
            for(const auto& audio:prepared_viewer->client_audio->pump()){
                if(audio.status==civ5::ui::CivisAudioEventStatus::failed||audio.status==civ5::ui::CivisAudioEventStatus::busy)
                    prepared_viewer->client_audio_error=audio.error;
                if(!output.empty())live_screen_report<<"civis-audio sequence="<<audio.event.sequence<<" status="<<static_cast<int>(audio.status)<<" source="<<audio.native_basename<<" error="<<audio.error<<'\n';
            }
        }catch(const std::exception& error){prepared_viewer->client_audio_error=error.what();prepared_viewer->retire_client_audio();}}

        if (smoke) {
            if (ticks == 2) {
                post(WM_MOUSEMOVE, 0, MAKELPARAM(smoke_drag_x, smoke_drag_y));
                post(WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(smoke_drag_x, smoke_drag_y));
            }
            if (ticks >= 3 && ticks <= 14)
                post(WM_MOUSEMOVE, MK_LBUTTON,
                     MAKELPARAM(smoke_drag_x + int(ticks - 2) * 5,
                                smoke_drag_y + int(ticks - 2) * 2));
            if (ticks == 15) {
                post(WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(smoke_drag_x + 65, smoke_drag_y + 26));
                post(WM_LBUTTONUP, 0, MAKELPARAM(smoke_drag_x + 65, smoke_drag_y + 26));
            }
            if (ticks == 21)
                post(WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0);
            if (ticks == 38) {
                post(WM_KEYDOWN, 'R', 0);
                post(WM_KEYUP, 'R', 0);
            }
            if (ticks == 40) {
                post(WM_KEYDOWN, 'I', 0);
                post(WM_KEYUP, 'I', 0);
                post(WM_KEYDOWN, 'Z', 0);
                post(WM_KEYUP, 'Z', 0);
            }
            if (ticks == 44) {
                post(WM_KEYDOWN, 'D', 0);
                post(WM_KEYDOWN, 'Q', 0);
            }
            if (ticks == 49) {
                post(WM_KEYUP, 'D', 0);
                post(WM_KEYUP, 'Q', 0);
            }
            if (ticks == 52)
                post(WM_MOUSEWHEEL, MAKEWPARAM(0, static_cast<WORD>(-WHEEL_DELTA)), 0);
            if (ticks == 55) {
                post(WM_MOUSEMOVE, 0, MAKELPARAM(client_width - 100, 16));
                post(WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(client_width - 100, 16));
            }
            if (ticks == 56)
                post(WM_LBUTTONUP, 0, MAKELPARAM(client_width - 100, 16));
            if (ticks == smoke_science_tick && smoke_science_action_expected) {
                if (smoke_tech_tree_close_expected) {
                    check(live_shared_screens && !live_shared_screens->visible("TechTree") &&
                              !live_tech_tree_lifecycle->active_event(live_bnw_preview->session),
                          "Science smoke must start without a queued TechTree occurrence");
                    smoke_tech_tree_parent=live_shared_screens->focused_screen();
                }
                if (viewport_transaction)
                    check(published_viewport && published_viewport_gate &&
                              viewport_transaction->current(*published_viewport, published_viewport_gate),
                          "Science smoke requires a freshly presented parent after Help navigation");
                check(presenter.last_science_hit && presenter.last_science_hit->width>0,
                      "Painted TopPanel science target disappeared during smoke");
                const auto science_x=presenter.last_science_hit->x + presenter.last_science_hit->width/2;
                post(WM_MOUSEMOVE, 0, MAKELPARAM(science_x, 16));
                post(WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(science_x, 16));
                post(WM_LBUTTONUP, 0, MAKELPARAM(science_x, 16));
            }
            if (ticks == smoke_gold_tick) {
                if (smoke_gold_hover_expected) {
                    check(presenter.last_gold_hit && presenter.last_gold_hit->width > 0,
                          "Painted TopPanel gold hit target is missing");
                    post(WM_MOUSEMOVE, 0,
                         MAKELPARAM(presenter.last_gold_hit->x + presenter.last_gold_hit->width / 2, 16));
                } else {
                    post(WM_MOUSEMOVE, 0, MAKELPARAM(100, 16));
                }
            }
            if (ticks == 53 && smoke_readonly_expected) {
                check(live_screen_frame && !live_screen_frame->focused_context && last_readonly_text_report,
                      "Readonly smoke requires the current unobscured world frame");
                const auto strength=std::find_if(last_readonly_text_report->regions.begin(),
                    last_readonly_text_report->regions.end(),[](const auto &region) {
                        return region.control_id=="UnitStatStrength";
                    });
                check(strength != last_readonly_text_report->regions.end(),
                      "Actual UnitPanel strength glyph bounds are unavailable");
                smoke_strength_tooltip=strength->tooltip;
                post(WM_MOUSEMOVE,0,MAKELPARAM(strength->bounds.x+strength->bounds.width/2,
                                               strength->bounds.y+strength->bounds.height/2));
            }
            if (ticks == 54 && smoke_readonly_expected)
                post(WM_MOUSEMOVE,0,MAKELPARAM(client_width/2,client_height/2));
            if (ticks == 60 && smoke_tech_tree_close_expected) {
                check(live_client_scene && live_ui_host && live_ui_frame && live_screen_frame &&
                          live_screen_frame->focused_context == std::optional<std::string>{"TechTree"},
                      "Actual science occurrence did not compose focused TechTree");
                // Pin the preceding actually presented frame/control stream.
                // Source CloseButton layout supplies the typed measured hit;
                // no synthetic UiClose or guessed coordinates enter this route.
                const auto previous_client=live_client_scene->frame();
                const auto previous_live=*live_ui_frame;
                const auto previous_composed=*live_screen_frame;
                check(live_client_scene->accepts(*live_ui_host,previous_live) &&
                          previous_composed.frame_owner==previous_live.owner &&
                          previous_composed.frame_sequence==previous_live.sequence &&
                          previous_composed.snapshot_frame==previous_live.snapshot.frame(),
                      "TechTree close smoke has a stale preceding composed publication");
                const auto occurrence=live_tech_tree_lifecycle->active_event(live_bnw_preview->session);
                const auto *published=previous_live.snapshot.find<civ5::ui::ProgressionTechnologyTreeEvent>();
                check(occurrence && published && occurrence->token &&
                          occurrence->token==published->token && occurrence->player_id==published->player_id &&
                          occurrence->turn==published->turn && previous_live.technology_tree_close_ready,
                      "Actual TechTree CloseButton lacks its current owned occurrence");
                const auto close=std::find_if(previous_composed.hits.begin(),previous_composed.hits.end(),
                    [&](const civ5::ui::LiveUiHit &hit) {
                        const auto *command=std::get_if<civ5::ui::UiGameplayCommand>(&hit.command);
                        const auto *action=command?command->as<civ5::ui::ProgressionCloseTechnologyTree>():nullptr;
                        return hit.context_id=="TechTree" && command && command->source_id()=="TechTree" &&
                            action && action->event_token==occurrence->token;
                    });
                check(close!=previous_composed.hits.end() && close->enabled &&
                          close->bounds.width>0 && close->bounds.height>0,
                      "Actual source TechTree CloseButton is unavailable");
                const auto x=std::int64_t(close->bounds.x)+close->bounds.width/2;
                const auto y=std::int64_t(close->bounds.y)+close->bounds.height/2;
                check(x>=0 && y>=0 && x<client_width && y<client_height && x<=32767 && y<=32767 &&
                          live_client_scene->frame()==previous_client &&
                          live_client_scene->accepts(*live_ui_host,previous_live),
                      "TechTree source CloseButton changed before physical smoke input");
                smoke_tech_tree_occurrence=*occurrence;
                live_screen_report << "tech-tree authored-close-posted civis-occurrence=" << occurrence->token
                    << " precedingFrame=" << previous_live.snapshot.frame()
                    << " bounds=" << close->bounds.x << ',' << close->bounds.y << ','
                    << close->bounds.width << ',' << close->bounds.height
                    << " nativeVM=false RefreshDisplay=false\n";
                post(WM_MOUSEMOVE,0,MAKELPARAM(static_cast<int>(x),static_cast<int>(y)));
                post(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(static_cast<int>(x),static_cast<int>(y)));
                post(WM_LBUTTONUP,0,MAKELPARAM(static_cast<int>(x),static_cast<int>(y)));
                smoke_tech_tree_close_posted=true;
            }
            if (ticks == 61 && smoke_tech_tree_close_expected) {
                check(smoke_tech_tree_close_posted && smoke_tech_tree_occurrence && live_shared_screens &&
                          !live_shared_screens->visible("TechTree") &&
                          live_shared_screens->focused_screen()==smoke_tech_tree_parent &&
                          !live_tech_tree_lifecycle->active_event(live_bnw_preview->session) &&
                          live_tech_tree_lifecycle->semaphore()==0 && !live_tech_tree_lifecycle->uncertain(),
                      "Actual TechTree CloseButton did not balance/dequeue/retire/restore parent");
                smoke_tech_tree_close_verified=true;
            }
            if (ticks == smoke_exit_tick) {
                if(smoke_tech_tree_close_expected) {
                    // WM_CLOSE retires the presented ticket before the close sink.
                    // Prove the final frame while it is still a live publication.
                    check(!input.close && !native_window_close_retired &&
                              smoke_tech_tree_close_verified && smoke_tech_tree_occurrence &&
                              live_shared_screens && live_client_scene && live_ui_host && live_ui_frame &&
                              live_client_scene->accepts(*live_ui_host,*live_ui_frame) &&
                              viewport_transaction && published_viewport && published_viewport_gate &&
                              viewport_transaction->current(*published_viewport,published_viewport_gate) &&
                              !live_shared_screens->visible("TechTree") &&
                              live_shared_screens->focused_screen()==smoke_tech_tree_parent &&
                              !live_tech_tree_lifecycle->active_event(live_bnw_preview->session) &&
                              live_tech_tree_lifecycle->semaphore()==0 && !live_tech_tree_lifecycle->uncertain(),
                          "TechTree local lifecycle final pre-window-close current-frame proof is unavailable");
                    smoke_tech_tree_pre_window_close_current_verified=true;
                }
                post(WM_CLOSE, 0, 0);
            }
        }
        gameplay_pump_clock.enter_native_batch();
        MSG message{};
        bool native_quit_without_close=false;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                native_quit_without_close=true;
                input.close = true;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
            if(prepared_viewer->drain_existing_){
                check(viewport_transaction!=nullptr,"Party queued input lost actual viewport");
                prepared_viewer->drain_existing_(*viewport_transaction);
            }
            // Leave the exact WM_CLOSE native stacks before invoking Lua;
            // later queued input must not invalidate this live publication.
            if(message.message==WM_CLOSE && message.hwnd==window) break;
            if(input.close && prepared_viewer->stock_source_owner) break;
        }
        gameplay_pump_clock.leave_native_batch();
        check(!input.failed, "Preview Windows input processing failed");
        if (input.close) {
            if(prepared_viewer->stock_source_owner) {
                const auto native_close=prepared_viewer->native_close_occurrence;
                if(!native_close) {
                    check(!native_quit_without_close,
                          "WM_QUIT cannot authorize WorldView native shutdown");
                    // Esc requests an actual WM_CLOSE occurrence; it does not
                    // substitute a boolean for the registered shutdown loan.
                    input.close=false;
                    check(PostMessageW(window,WM_CLOSE,0,0)!=FALSE,
                          "Cannot request actual WorldView native close");
                    continue;
                }
                try {
                    check(viewport_transaction && live_screen_painter &&
                          prepared_viewer->gui_screen && live_client_scene &&
                          worldview_projection_binding.has_value() &&
                          !viewport_retired_pending && !native_window_close_retired,
                          "Native close lost its live renderer/source publication");
                    {
                        civ5::ui::BnwWorldViewRendererProjectionLoan projection(
                            *live_screen_painter,*worldview_projection_binding,
                            live_client_scene,client_width,client_height);
                        live_screen_painter->shutdown_owned_worldview(
                            *prepared_viewer->stock_source_owner,
                            *prepared_viewer->gui_screen,projection);
                    } // Genuine renderer/source/Grid POST and projection unwind.
                    if(prepared_viewer->existing_menu_binding){
                        // Stage the SAME completed source shutdown. The outer
                        // launch/material/target POST still needs this live Window.
                        viewport_transaction->require_staged_native_close_owned(native_close);
                    }else{
                        viewport_transaction->finalize_native_close(native_close);
                        prepared_viewer->native_close_occurrence.reset();
                    }
                } catch(...) {
                    if(viewport_transaction)viewport_transaction->abort_native_close();
                    throw;
                }
            }
            break;
        }
        if(viewport_transaction) {
            // This is the safe boundary after the native message/source stacks unwound.
            if(input.coordinate_cleanup_pending) {
                input.events.clear(); input.live_ui_wheels.clear(); input.search_characters.clear();
                input.coordinate_cleanup_pending=false;
                manual_cycle_keys.cancel();pending_manual_cycles.clear();
            }
            if(viewport_retired_pending) {
                published_viewport.reset(); published_viewport_gate={}; candidate_hits.clear();
                input.events.clear(); input.live_ui_wheels.clear(); input.search_characters.clear();
                input.keys.fill(false); input.search_focus=false; input.search_cancel=true;
                input.pointer_left_down=false; input.pointer_press_x=input.pointer_press_y=-1;
                input.top_panel_click_pending=false;
                input.top_panel_click_x=input.top_panel_click_y=-1;
                input.minimap_pressed=input.minimap_click_pending=input.live_ui_pressed=false;
                input.minimap_press_x=input.minimap_press_y=-1;
                input.minimap_click_x=input.minimap_click_y=-1;
                input.live_hit_regions.clear(); input.live_modal_open=false;
                live_ui_frame.reset(); live_screen_frame.reset();
                last_readonly_text_report.reset(); last_readonly_tooltip_text.reset();
                presenter.live_top_panel_presentation.reset();
                live_minimap_packet.reset(); applied_minimap_fog.reset();
                applied_minimap_ring_bounds.reset(); applied_minimap_ring_camera_current={};
                composed_minimap_revision=0;
                if(live_minimap_surface) live_minimap_surface->invalidate_applied_fog();
                manual_cycle_keys.cancel();pending_manual_cycles.clear();
                viewport_retired_pending=false;
            }
            if(parent_prepared) prepared_viewer->require_initial_ticket(); // No post-retirement refresh.
            drawing_viewport=viewport_transaction->request(); // Also detects missed WM_SIZE/DPI.
            check(!viewport_transaction->failed(),"Native viewport transaction failed");
            if(!drawing_viewport || drawing_viewport->viewport().client_width!=client_width ||
                drawing_viewport->viewport().client_height!=client_height) {
                // Changed extent has no proved normal renderer lens producer.
                // No begin_frame/paint/action/capture/swap occurs in this branch.
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                clock.tick(std::chrono::duration<double>(Clock::now()-started).count());
                continue;
            }
            // request() can discover a missed change and synchronously retire.
            if(viewport_retired_pending) continue;
        }
        if (!smoke && IsIconic(window)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            clock.tick(std::chrono::duration<double>(Clock::now() - started).count());
            continue;
        }
        if (input.cancel || input.reset) {
            pointer.flags &= ~std::uint32_t{4 | 8 | 64};
            pointer.buttons = 0;
            if (GetCapture() == window)
                ReleaseCapture();
            input.cancel = false;
        }
        if (input.reset) {
            manual_cycle_keys.cancel();pending_manual_cycles.clear();
            controller = initial;
            if constexpr (requires { land.restore_camera_motion(); })
                land.restore_camera_motion();
            pointer = {width, height};
            input.events.clear();
            input.live_ui_wheels.clear();
            input.reset = false;
            ++resets;
        }
        if(live_screen_painter) {
            const auto current=[&] {
                return !viewport_retired_pending&&viewport_transaction&&published_viewport&&published_viewport_gate&&
                    viewport_transaction->current(*published_viewport,published_viewport_gate)&&
                    live_client_scene&&live_ui_host&&live_ui_frame&&live_screen_frame&&
                    live_client_scene->accepts(*live_ui_host,*live_ui_frame)&&
                    live_screen_frame->focused_context==std::optional<std::string>{"TechAwardPopup"}&&
                    prepared_viewer->initial_epoch_current()&&
                    viewport_transaction->current(*published_viewport,published_viewport_gate);
            };
            live_screen_painter->consume_tech_award_pointer(current);
        }
        // Consume previous actual pointer receipts before minting the new
        // sample sequence. Both boundaries follow this loop's message batch.
        if(live_screen_painter&&live_client_scene&&live_ui_host&&prepared_viewer->renderer_window_owner){
            auto earned_scroll_tick=live_screen_painter->begin_scroll_animation_loop_tick_owned(
                live_client_scene,live_ui_host,prepared_viewer->renderer_window_owner);
            scroll_loop_tick=std::move(earned_scroll_tick.first);
            tech_award_update_sample=std::move(earned_scroll_tick.second);
        }
        // One genuine later camera-loop boundary for both message69 and53.
        // Construction is private to this actual run body; no public callback,
        // source-operation serial or test bool can publish native unwind.
        if(prepared_viewer&&prepared_viewer->stock_source_owner&&live_client_scene&&
           live_ui_host&&live_ui_frame&&published_viewport&&published_viewport_gate){
            const auto source_weak=std::weak_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner>(prepared_viewer->stock_source_owner);
            const auto viewer_weak=std::weak_ptr<PreparedCameraViewer>(prepared_viewer);
            const auto scene_weak=std::weak_ptr<civ5::ui::BnwClientSceneFrame>(live_client_scene);
            const auto ui_weak=std::weak_ptr<civ5::ui::LiveUiHost>(live_ui_host);
            const auto window_weak=std::weak_ptr<const civ5::graphics::BnwOwnedCameraRendererWindowLease>(prepared_viewer->renderer_window_owner);
            const auto expected_ui=*live_ui_frame;const auto expected_scene=live_client_scene->frame();
            const auto expected_ticket=*published_viewport;
            const auto actual_camera=[&]{
                const auto viewer=viewer_weak.lock();const auto scene=scene_weak.lock();const auto ui=ui_weak.lock();
                const auto source=source_weak.lock();const auto window_owner=window_weak.lock();
                const auto pure=[&]{
                    check(viewer&&viewer.use_count()>1&&viewer->alive&&!viewer->initial_epoch_retired&&
                      prepared_viewer&&viewer.get()==prepared_viewer.get()&&source&&source.use_count()>1&&
                      source.get()==viewer->stock_source_owner.get()&&!source.owner_before(viewer->stock_source_owner)&&!viewer->stock_source_owner.owner_before(source)&&
                      scene&&scene.use_count()>1&&scene.get()==live_client_scene.get()&&ui&&ui.use_count()>1&&ui.get()==live_ui_host.get()&&
                      window_owner&&window_owner.get()==viewer->renderer_window_owner.get()&&
                      viewer->viewport_transaction.get()==viewport_transaction.get()&&!input.close&&!input.failed&&
                      !native_window_close_retired&&!viewport_retired_pending&&published_viewport&&published_viewport_gate&&
                      published_viewport->version()==expected_ticket.version()&&live_ui_frame&&
                      live_ui_frame->owner==expected_ui.owner&&live_ui_frame->sequence==expected_ui.sequence&&
                      live_ui_frame->snapshot.same_payload_storage(expected_ui.snapshot)&&scene->frame()==expected_scene,
                      "Gameplay pump actual camera source/window/Scene/UI publication retired");
                };
                pure();window_owner->require_current();pure();
                check(viewport_transaction->current(expected_ticket,published_viewport_gate),"Gameplay pump actual viewport rejected");pure();
                check(scene->accepts(*ui,expected_ui),"Gameplay pump actual canonical Scene/UI rejected");pure();
                window_owner->require_current();pure();
            };
            civ5::runtime::BnwOwnedGameplayEventPumpLoan pump(gameplay_pump_clock,source_weak,scene_weak,ui_weak,actual_camera);
            // SourceOwner owns the actual dedicated later operation and issues
            // event-specific receiver loans. No activation is invented here.
            prepared_viewer->stock_source_owner->pump_gameplay_events_owned(pump);
            pump.complete();
        }
        // Genuine popup input executes before another begin_frame, using the
        // displayed frame captured on actual WM ingress. The real Source43
        // dispatcher owns the new SAMEVM operation and all native effects.
        if(observed_tech_award_input) {
            const auto request=std::move(*observed_tech_award_input);
            observed_tech_award_input.reset();observed_tech_award_press.reset();
            const auto current=[&] {
                return !viewport_retired_pending&&viewport_transaction&&request.gate&&
                    viewport_transaction->current(request.ticket,request.gate)&&
                    live_client_scene&&live_ui_host&&live_ui_frame&&live_screen_frame&&
                    live_client_scene->frame()==request.frame&&live_client_scene->accepts(*live_ui_host,request.ui)&&
                    live_ui_host->is_current_frame(request.ui)&&live_ui_frame->owner==request.ui.owner&&
                    live_ui_frame->sequence==request.ui.sequence&&
                    live_ui_frame->snapshot.same_payload_storage(request.ui.snapshot)&&
                    live_screen_frame->focused_context==std::optional<std::string>{"TechAwardPopup"}&&
                    viewport_transaction->current(request.ticket,request.gate);
            };
            check(current(),"TechAward observed input frame retired before dispatch");
            const auto source=request.source.lock();
            check(source&&source.get()==prepared_viewer->stock_source_owner.get()&&
                  !source.owner_before(prepared_viewer->stock_source_owner)&&
                  !prepared_viewer->stock_source_owner.owner_before(source),
                  "TechAward input actual SourceOwner differs");
            pending_tech_award_input=live_ui_host->prepare_tech_award_input(request.ui,request.command);
            try {
                check(current(),"TechAward source changed during genuine reservation");
                source->run_same_tech_award_input(pending_tech_award_input);
                pending_tech_award_input->require_current();
                check(pending_tech_award_input->phase()==civ5::ui::BnwTechAwardInputPhase::pending_frame&&
                      !viewport_retired_pending&&prepared_viewer->initial_epoch_current()&&
                      live_client_scene->frame()==request.frame&&
                      source.get()==prepared_viewer->stock_source_owner.get()&&
                      !source.owner_before(prepared_viewer->stock_source_owner)&&
                      !prepared_viewer->stock_source_owner.owner_before(source),
                      "TechAward native input source/viewport retired after its real effects");
            } catch(...) {
                live_ui_host->retire_tech_award_input_after_failure(pending_tech_award_input);
                throw;
            }
            // Retire stale GUI before its next public stock attach/advance. A new
            // real Scene/UI frame will publish the now-closed modal and paint it.
            live_client_scene->invalidate();
            prepared_viewer->retire_gui_screen();
            input.top_panel_click_pending=false;input.observed_left_click.reset();
            input.top_panel_click_x=input.top_panel_click_y=-1;
            input.live_ui_wheels.clear();input.search_characters.clear();
            input.minimap_click_pending=false;
            pending_manual_cycles.clear();
        }
        // Native message/source stacks have unwound. Dispatch the exact
        // observed frame once; a prior action retires later queued intents.
        auto manual_inputs=std::move(pending_manual_cycles);pending_manual_cycles.clear();
        for(const auto& request:manual_inputs) {
            const bool current=!input.search_focus && !input.live_modal_open &&
                !input.top_panel_click_pending && !input.minimap_click_pending &&
                viewport_transaction && request.gate &&
                viewport_transaction->current(request.ticket,request.gate) &&
                live_client_scene && live_ui_host && live_client_scene->frame()==request.frame &&
                civ5::ui::BnwManualCycleKeyInput::world_frame(*live_client_scene,*live_ui_host,request.ui) &&
                live_screen_frame && !live_screen_frame->focused_context &&
                live_screen_frame->frame_owner==request.ui.owner &&
                live_screen_frame->frame_sequence==request.ui.sequence &&
                viewport_transaction->current(request.ticket,request.gate) &&
                civ5::ui::BnwManualCycleKeyInput::world_frame(*live_client_scene,*live_ui_host,request.ui);
            const auto result=current ? live_client_scene->dispatch(*live_ui_host,request.ui,
                civ5::ui::UiCommand{civ5::ui::UiGameplayCommand::make("WorldView",request.command)}) :
                civ5::ui::UiCommandResult{civ5::ui::UiCommandStatus::invalid,"Observed manual key frame retired"};
            live_screen_report << "manual-cycle key filter=" << static_cast<int>(request.command.filter)
                << " status=" << static_cast<int>(result.status) << " reason=" << result.reason << '\n';
        }
        // Route one wheel through the previous still-current composed frame
        // before the camera consumes this tick's input. A successful local
        // scroll changes the UI sequence; subsequent wheels wait for the next
        // presented frame instead of using stale hit geometry.
        if (!input.live_ui_wheels.empty()) {
            const auto wheel = input.live_ui_wheels.front();
            input.live_ui_wheels.erase(input.live_ui_wheels.begin());
            const bool over_ui = civ5::ui::live_ui_blocks_camera_pointer(
                input.live_hit_regions, input.live_modal_open, true,
                wheel.x, wheel.y);
            civ5::ui::LiveUiInputResult routed;
            if (live_ui_host && live_ui_frame && live_screen_frame &&
                (!live_client_scene || live_client_scene->accepts(*live_ui_host, *live_ui_frame))) {
                routed = civ5::ui::wheel_composed_ui(*live_ui_host,
                    *live_ui_frame, *live_screen_frame,
                    wheel.x, wheel.y, wheel.delta);
                if (live_client_scene && (routed.command.status == civ5::ui::UiCommandStatus::performed ||
                    !live_ui_host->is_current_frame(*live_ui_frame))) live_client_scene->invalidate();
                live_screen_report << "wheel consumed=" << routed.consumed
                                   << " status=" << static_cast<int>(routed.command.status)
                                   << " reason=" << routed.command.reason << '\n';
            }
            if (input.live_modal_open)
                check(routed.consumed, "Focused modal UI did not consume wheel input");
            if (!routed.consumed && !over_ui) {
                check(input.events.size() < 4096, "Preview camera wheel queue overflow");
                input.events.push_back(CameraWheelEvent{
                    static_cast<std::int16_t>(wheel.delta), wheel.flags});
            }
        }
        // The render target has fixed dimensions. Map the actual client pixels
        // into it before the recovered Windows-coordinate producer sees them.
        for (auto &event : input.events)
            if (auto *p = std::get_if<CameraPointerEvent>(&event)) {
                p->x = static_cast<std::int16_t>(std::clamp(int(p->x) * width / client_width, -32768, 32767));
                p->y =
                    static_cast<std::int16_t>(std::clamp(int(p->y) * height / client_height, -32768, 32767));
            }
        const auto &k = input.keys;
        const std::uint8_t commands = ((k['A'] || k[VK_LEFT]) ? 1 : 0) | ((k['D'] || k[VK_RIGHT]) ? 2 : 0) |
                                      (civ5::ui::BnwManualCycleKeyInput::camera_up(bool(font_source_parent),k['W'],k[VK_UP]) ? 4 : 0) | ((k['S'] || k[VK_DOWN]) ? 8 : 0) |
                                      (k['Q'] ? 16 : 0) | (k['E'] ? 32 : 0);
        clock.tick(smoke ? double(ticks + 1) / 60
                         : std::chrono::duration<double>(frame_start - started).count());
        const auto result = water.prepare_camera_input(controller, pointer,
                                                       {static_cast<float>(clock.delta),
                                                        commands,
                                                        0,
                                                        {{1, 0}, input.straight, input.inertia},
                                                        input.events},
                                                       frame_settings, rcp, rsqrt);
        preview_camera_events(land, result.motion.events);
        controller = result.motion.controller;
        normalized_zoom = result.motion.zoom[0];
        pointer = result.pointer;
        input.events.clear();
        for (auto event : result.capture_events) {
            if (event == CameraCaptureEvent::capture) {
                ++captures;
                if (!smoke) {
                    SetCapture(window);
                    check(GetCapture() == window, "Preview mouse capture failed");
                }
            } else {
                ++releases;
                if (GetCapture() == window)
                    ReleaseCapture();
            }
        }
        const auto input_done = Clock::now();
        if ((controller.flags[1] & 1) && (controller.velocity[0] != 0 || controller.velocity[1] != 0))
            ++inertia_ticks;
        if (smoke && ticks == 18) {
            drag_changed = controller.camera.world[12] != initial.camera.world[12] &&
                           controller.camera.world[13] != initial.camera.world[13];
        }
        if (smoke && ticks == 34)
            zoom_changed = controller.camera.world[14] != initial.camera.world[14];
        surface = advance_water_surface(surface, clock.delta);
        live_material_time = static_cast<float>(clock.elapsed);
        if (fow)
            fow->state.tick(static_cast<float>(clock.delta));
        latch_live_client_frame();
        scene_land_prepare(land, water.camera_state(),
                           output / "scheduled-materials" / std::to_string(ticks + 1));
        if (smoke && (ticks == 18 || ticks == 34)) {
            validate_culling(ticks == 18 ? "drag" : "zoom");
            ++culling_checks;
        } else
            render(true);
        scene_land_finish(land);
        const auto render_done = Clock::now();
        const auto click_pending = input.top_panel_click_pending;
        const auto press_x = input.pointer_press_x;
        const auto press_y = input.pointer_press_y;
        const auto click_x = input.top_panel_click_x;
        const auto click_y = input.top_panel_click_y;
        const auto over_composed_hit = [&](std::int32_t x, std::int32_t y) {
            if (!live_screen_frame) return false;
            return std::any_of(live_screen_frame->hits.begin(),
                live_screen_frame->hits.end(), [&](const civ5::ui::LiveUiHit &hit) {
                    const auto &r = hit.bounds;
                    return x >= r.x && y >= r.y &&
                        std::int64_t(x) < std::int64_t(r.x) + r.width &&
                        std::int64_t(y) < std::int64_t(r.y) + r.height;
                });
        };
        const bool screen_owns_click = input.live_modal_open ||
            (over_composed_hit(press_x, press_y) &&
             over_composed_hit(click_x, click_y));
        update_minimap_viewport();
        const auto capture_path = smoke && (ticks == 18 || ticks == 34 || ticks == 56 ||
                                            (ticks == smoke_gold_tick && smoke_gold_hover_expected) ||
                                            (ticks == 53 && smoke_readonly_expected))
            ? output / (ticks == 18 ? "drag.ppm"
                        : ticks == 34 ? "zoom.ppm"
                        : ticks == 56 ? "help-tooltip.ppm"
                        : ticks == smoke_gold_tick ? "gold-tooltip.ppm" : "strength-tooltip.ppm")
            : fs::path{};
        if (live_minimap_viewport && !capture_path.empty())
            GlMinimapSurfaceDiagnostic::save_ppm(
                *presenter.minimap_chrome->partial_map,
                output / ("minimap-" + capture_path.stem().string() + "-gpu.ppm"));
        check(!live_client_scene || live_client_scene->current(),
              "BNW client frame became stale while drawing its world");
        if(actual_alpha_frame)actual_alpha_frame->require_camera_boundary_owned();
        const auto action = presenter.draw(*context, target.color, client_width, client_height, input.pointer_x,
                                           input.pointer_y, input.pointer_valid, input.pointer_left_down,
                                           click_pending && !screen_owns_click,
                                           press_x, press_y, click_x, click_y,
                                           capture_path, false, input.minimap_options_visible,
                                           &input.minimap_map_display);
        if(actual_alpha_frame)actual_alpha_frame->require_camera_boundary_owned();
        if(pending_tech_award_input) {
            try {
                check(live_client_scene&&live_ui_frame,"TechAward rendered source frame absent");
                const auto held_frame=live_client_scene->frame();
                const auto held_ui=*live_ui_frame;
                const auto* gui=prepared_viewer->gui_screen.get();
                const std::weak_ptr<const civ5::ui::BnwOwnedCameraGuiScreen> gui_control=prepared_viewer->gui_screen;
                check(pending_tech_award_input->phase()==civ5::ui::BnwTechAwardInputPhase::completed&&
                      live_client_scene&&live_ui_host&&live_ui_frame&&live_screen_frame&&
                      live_client_scene->accepts(*live_ui_host,*live_ui_frame)&&
                      live_ui_host->is_current_frame(*live_ui_frame),
                      "TechAward actual new closed UI frame was not rendered");
                pending_tech_award_input->require_current();
                check(prepared_viewer->initial_epoch_current()&&!viewport_retired_pending&&
                      live_client_scene->frame()==held_frame&&live_ui_frame->owner==held_ui.owner&&
                      live_ui_frame->sequence==held_ui.sequence&&
                      live_ui_frame->snapshot.same_payload_storage(held_ui.snapshot)&&
                      gui&&prepared_viewer->gui_screen.get()==gui&&
                      !gui_control.owner_before(prepared_viewer->gui_screen)&&
                      !prepared_viewer->gui_screen.owner_before(gui_control)&&
                      live_ui_frame->owner==live_screen_frame->frame_owner&&
                      live_ui_frame->sequence==live_screen_frame->frame_sequence&&
                      live_ui_frame->snapshot.frame()==live_screen_frame->snapshot_frame,
                      "TechAward renderer retired after final input source checks");
                pending_tech_award_input.reset();
            } catch(...) {
                live_ui_host->retire_tech_award_input_after_failure(pending_tech_award_input);
                throw;
            }
        }
        if(production_component_validation&&(validation_train_component_requested||validation_train_initialization_requested)){
            check(live_selected_production_services&&live_selected_production_backend&&live_client_scene&&
                  live_ui_host&&live_ui_frame&&validation_train_choice&&
                  live_client_scene->accepts(*live_ui_host,*live_ui_frame),
                  "Production validation requires genuine retained Services/backend and freshly published Scene/UI");
            const auto* city=civ5::ui::selected_city(live_ui_frame->snapshot);
            const auto* stamp=live_ui_frame->snapshot.find<civ5::ui::BnwClientFrameStamp>();
            check(city&&stamp&&city->id==validation_train_choice->city_id&&city->owner==stamp->identity.player&&
                  city->queue_resolved&&!city->queue.empty()&&city->queue.front().type==0&&
                  city->queue.front().item==validation_train_choice->item_id,
                  "Production validation selected actual TRAIN queue changed after the WM command");
            const auto actual_owner=city->owner,actual_city=city->id;
            const bool initialize=validation_train_initialization_requested;
            // Consuming the internal request is not completion authority: the
            // real cursor/Source/Core guards below prove every native stage.
            validation_train_component_requested=false;validation_train_initialization_requested=false;
            const auto dispatched=live_client_scene->dispatch(*live_ui_host,*live_ui_frame,[&]()->civ5::ui::UiCommandResult{
                if(!initialize){
                    check(!validation_train_component_attempted,"Production validation once component replay");
                    validation_train_component_attempted=true;
                    validation_train_cursor=live_selected_production_services->advance_train_component(actual_owner,actual_city);
                    for(std::size_t stage=0;stage<8;++stage){
                        validation_train_cursor->require_current();const auto progress=validation_train_cursor->progress();
                        if(progress.phase==civ5::runtime::BnwCityProductionTurnPhase::complete_component||
                           progress.phase==civ5::runtime::BnwCityProductionTurnPhase::pop_order_pending)break;
                        validation_train_cursor=live_selected_production_services->advance_train_component(actual_owner,actual_city,validation_train_cursor);
                    }
                    validation_train_cursor->require_current();const auto progress=validation_train_cursor->progress();
                    check(progress.phase==civ5::runtime::BnwCityProductionTurnPhase::complete_component||
                          progress.phase==civ5::runtime::BnwCityProductionTurnPhase::pop_order_pending,
                          "Production validation actual component did not reach bounded frontier");
                    write_production_validation("ACTUAL_COMPONENT owner="+std::to_string(actual_owner)+" city="+std::to_string(actual_city)+
                        " turn="+std::to_string(progress.turn)+" differenceTimes100="+std::to_string(progress.difference_times100)+
                        " productionNeeded="+std::to_string(progress.production_needed)+" phase="+std::to_string(static_cast<int>(progress.phase)));
                    if(progress.phase==civ5::runtime::BnwCityProductionTurnPhase::pop_order_pending){
                        validation_completed_train_source=live_selected_production_services->advance_completed_train_component(actual_owner,actual_city);
                        check(validation_completed_train_source!=nullptr,"Production validation real completion returned no source");
                        validation_train_initialization_requested=true; // require another REAL Scene publication
                        return {civ5::ui::UiCommandStatus::pending_refresh,"Actual completed TRAIN prefix/Help needs a fresh Scene before Init"};
                    }
                    // Same turn cannot be replayed to invent completion/cost.
                    return {civ5::ui::UiCommandStatus::pending_refresh,"Actual production component stored; cost not reached, full scheduler unavailable"};
                }
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                check(restart_preview&&validation_completed_train_source,"Production Init requires actual observed preview and native completed source");
                validation_train_initialization=live_selected_production_services->begin_completed_train_unit_initialization(
                    *restart_preview,*validation_completed_train_source,live_client_scene);
                const auto& native=*validation_train_initialization;
                if(native.renderer_refresh){
                    live_train_renderer_refresh=native.renderer_refresh;
                    native.renderer_refresh->require_pending_current();
                    write_production_validation("ACTUAL_INIT_PARTIAL_RENDER_PENDING "+native.result.reason);
                    return {civ5::ui::UiCommandStatus::pending_refresh,native.result.reason};
                }
                write_production_validation("ACTUAL_INIT_UNAVAILABLE_OR_UNCERTAIN "+native.result.reason);
                return {civ5::ui::UiCommandStatus::uncertain,native.result.reason};
#else
                return {civ5::ui::UiCommandStatus::unavailable,"Actual observed preview supplier not compiled"};
#endif
            });
            write_production_validation("ACTUAL_DISPATCH_STATUS "+std::to_string(static_cast<int>(dispatched.status))+" "+dispatched.reason);
            // dispatch invalidates its genuine FrameOwner on actual stores;
            // next normal latch/capture/compose/paint/present publishes it.
            // Do not reuse this stale GUI or process another old-frame input.
            input.observed_left_click.reset();continue;
        }

        if (smoke && ticks == 56)
            check(presenter.last_tooltip_text &&
                      *presenter.last_tooltip_text ==
                          presenter.localization.format("TXT_KEY_CIVILOPEDIA_TOOLTIP"),
                  "Help tooltip did not render its localized text");
        if (smoke && ticks == smoke_gold_tick && smoke_gold_hover_expected)
            check(presenter.last_tooltip_text &&
                      presenter.top_panel_controls.presentation().gold_tooltip &&
                      *presenter.last_tooltip_text==*presenter.top_panel_controls.presentation().gold_tooltip,
                  "Painted TopPanel gold target did not project its current tooltip");
        if (smoke && ticks == smoke_gold_tick && smoke_fixture_tooltips_expected)
            check(presenter.last_tooltip_text &&
                      presenter.last_tooltip_text->find("[ICON_BULLET]") != std::string::npos &&
                      presenter.last_tooltips_validated >= 8,
                  "TopPanel fixture tooltips or gold breakdown did not project");
        if (smoke && ticks == 53 && smoke_readonly_expected) {
            check(smoke_strength_tooltip && last_readonly_tooltip_text == smoke_strength_tooltip,
                  "Actual strength hover did not render the current source tooltip");
            smoke_strength_hover_verified=true;
        }
        if (smoke && ticks == 54 && smoke_readonly_expected) {
            check(!last_readonly_tooltip_text,"Readonly tooltip did not clear after pointer exit");
            smoke_strength_clear_verified=true;
        }
        record_panel();
        check(!live_client_scene || (presenter.client_frame_current && presenter.client_frame_current() &&
            live_client_scene->accepts(*live_ui_host,*live_ui_frame)),
              "BNW presented viewport/frame is stale before input dispatch");
        // A focused economic modal owns the whole click even when its native
        // popup owner or measured controls are incomplete. Generic overview
        // commands must never make that incomplete action look successful.
        const auto focused_input_screen = live_ui_host
            ? live_ui_host->screens().focused_screen() : std::nullopt;
        const auto *focused_input_context = live_ui_host && focused_input_screen
            ? live_ui_host->screens().registry().find(*focused_input_screen)
            : nullptr;
        const bool focused_input_modal = focused_input_context &&
            focused_input_context->modal;
        const bool economic_focus = focused_input_screen ==
            std::optional<std::string>{"EconomicOverview"};
        bool economic_click_consumed = click_pending && economic_focus;
        if (click_pending && live_economic_scene && live_bnw_preview &&
            live_ui_host && live_ui_frame) {
            const auto processed_before =
                live_economic_scene->lifecycle().processed_events().size();
            if (economic_focus) {
                const auto popup = live_economic_scene->popup_click(
                    live_bnw_preview->session, *live_ui_frame,
                    press_x, press_y, click_x, click_y);
                economic_click_consumed |= popup.consumed;
                if (popup.consumed) {
                    if (popup.action == civ5::ui::EconomicOverviewInputAction::close &&
                        popup.status == civ5::ui::EconomicOverviewInputStatus::performed)
                        check(live_economic_scene->lifecycle().processed_events().size() ==
                            processed_before + 1,
                            "EconomicOverview close did not publish Processed");
                    live_screen_report << "economic popup consumed=1 action=" <<
                        static_cast<int>(popup.action) << " status=" <<
                        static_cast<int>(popup.status) << " blocker=" <<
                        popup.blocker << " processed=" <<
                        live_economic_scene->lifecycle().processed_events().size() << '\n';
                }
            }
            if (!economic_click_consumed && !focused_input_modal &&
                presenter.last_gold_hit &&
                live_ui_frame->sequence <=
                    static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                const auto &r = *presenter.last_gold_hit;
                const auto &proof = live_economic_scene->gold_source();
                civ5::ui::EconomicGoldInputMeasurement measurement;
                measurement.click.player_id =
                    live_ui_frame->snapshot.active_player_id().value_or(-1);
                // A pre-existing notification popup owns the queue first.
                // Use an invalid event ID so the typed Gold route consumes
                // the click without creating a second modal owner.
                measurement.click.event_id = live_other_popup_feed &&
                    live_other_popup_feed(live_bnw_preview->session)
                    ? -1 : static_cast<std::int64_t>(live_ui_frame->sequence);
                measurement.click.turn = live_ui_frame->snapshot.turn().value_or(-1);
                measurement.click.frame = live_ui_frame->snapshot.frame();
                measurement.click.owner_sequence = live_ui_frame->sequence;
                measurement.click.viewport_width = client_width;
                measurement.click.viewport_height = client_height;
                measurement.click.gold_button = {
                    static_cast<std::int32_t>(r.x),
                    static_cast<std::int32_t>(r.y),
                    static_cast<std::int32_t>(r.width),
                    static_cast<std::int32_t>(r.height)};
                measurement.click.top_panel_lua_sha256 = proof.top_panel_lua_sha256;
                measurement.click.top_panel_xml_sha256 = proof.top_panel_xml_sha256;
                measurement.frame_owner = live_ui_frame->owner;
                measurement.frame_sequence = live_ui_frame->sequence;
                const auto gold = live_economic_scene->gold_click(
                    live_bnw_preview->session, *live_ui_frame, measurement,
                    client_width, client_height,
                    press_x, press_y, click_x, click_y);
                economic_click_consumed = gold.consumed;
                if (gold.status == civ5::ui::EconomicPopupQueueStatus::queued)
                    input.live_modal_open = true;
                if (gold.consumed)
                    live_screen_report << "economic gold consumed=1 status=" <<
                        static_cast<int>(gold.status) << " blocker=" <<
                        gold.blocker << '\n';
            }
        }
        const auto *popup_text_view = live_ui_frame
            ? live_ui_frame->snapshot.find<civ5::ui::PopupTextView>() : nullptr;
        if (input.search_cancel && popup_name_editor.focused() &&
            popup_text_view && live_ui_host && live_ui_frame) {
            const auto dispatch = [&] { return live_ui_host->dispatch(
                civ5::ui::UiCloseScreen{popup_text_view->context_id}, *live_ui_frame); };
            const auto routed = live_client_scene
                ? live_client_scene->dispatch(*live_ui_host, *live_ui_frame, dispatch) : dispatch();
            live_screen_report << "popup-name cancel status="
                               << static_cast<int>(routed.status)
                               << " reason=" << routed.reason << '\n';
        }
        if (input.search_cancel ||
            (search_editor.focused() &&
             (!live_screen_frame || live_screen_frame->focused_context !=
                 std::optional<std::string>{"CivilopediaScreen"})) ||
            (popup_name_editor.focused() &&
             (!live_screen_frame || !popup_text_view ||
              live_screen_frame->focused_context !=
                  std::optional<std::string>{popup_text_view->context_id}))) {
            search_editor.blur();
            popup_name_editor.blur();
            input.search_focus = input.search_cancel = false;
            input.search_characters.clear();
        }
        bool trade_tab_consumed = false;
        if (click_pending && !economic_click_consumed && live_trade_tabs &&
            live_ui_host && live_ui_frame &&
            (!live_client_scene || live_client_scene->accepts(*live_ui_host, *live_ui_frame))) {
            const auto routed = live_trade_tabs->click(*live_ui_frame,
                live_ui_host->screens(), press_x, press_y, click_x, click_y);
            trade_tab_consumed = routed.consumed;
            if (routed.consumed)
                live_screen_report << "trade-tab click consumed=1 status=" <<
                    static_cast<int>(routed.status) << " blocker=" <<
                    routed.blocker << '\n';
        }
        if (action) {
            search_editor.blur();
            popup_name_editor.blur();
            input.search_focus = false;
            check(!top_panel_events.empty() && top_panel_events.back().action == *action,
                  "TopPanel runtime did not record the returned action");
            if (live_ui_host && live_ui_frame) {
                const auto dispatch = [&] {
                    if (live_bnw_preview && *action == civ5::ui::TopPanelAction::open_technology_tree) {
                        if (!live_tech_tree_lifecycle || !live_client_scene)
                            return civ5::ui::UiCommandResult{civ5::ui::UiCommandStatus::unavailable,
                                "TechTree actual shared-controller/source owner is missing"};
                        return live_tech_tree_lifecycle->admit_science_action(live_bnw_preview->session,
                            live_client_scene->owner(), live_client_scene->frame(),
                            *live_ui_host, *live_ui_frame, top_panel_events.back());
                    }
                    return live_ui_host->dispatch_top_panel(*action, *live_ui_frame);
                };
                const auto routed = live_client_scene
                    ? live_client_scene->dispatch(*live_ui_host, *live_ui_frame, dispatch) : dispatch();
                live_screen_report << "top-panel status=" << static_cast<int>(routed.status)
                                   << " reason=" << routed.reason << '\n';
            }
        } else if (click_pending && !economic_click_consumed &&
                   !trade_tab_consumed && live_ui_host &&
                   live_ui_frame && live_screen_frame &&
                   (!live_client_scene || live_client_scene->accepts(*live_ui_host, *live_ui_frame))) {
            if (search_editor.select(*live_ui_host, *live_ui_frame,
                    *live_screen_frame, press_x, press_y, click_x, click_y)) {
                popup_name_editor.blur();
                input.search_focus = true;
                input.keys.fill(false);
                live_screen_report << "civilopedia-search focus=1\n";
            } else if (popup_name_editor.select(*live_ui_host, *live_ui_frame,
                    *live_screen_frame, press_x, press_y, click_x, click_y)) {
                input.search_focus = true;
                input.keys.fill(false);
                live_screen_report << "popup-name focus=1\n";
            } else {
                input.search_focus = false;
                civ5::ui::LiveUiInputResult routed;
                std::optional<ResearchClickObservation> clicked_research;
                std::optional<civ5::ui::CityChooseProduction> clicked_train;
                if((!output.empty()||production_component_validation) && input.observed_left_click && font_source_parent && live_client_scene){
                    const auto& mouse=*input.observed_left_click;
                    const auto hit_at=[&](std::int32_t x,std::int32_t y)->const civ5::ui::LiveUiHit*{
                        for(auto hit=live_screen_frame->hits.rbegin();hit!=live_screen_frame->hits.rend();++hit){
                            const auto& r=hit->bounds;
                            if(r.width>0&&r.height>0&&x>=r.x&&y>=r.y&&
                               std::int64_t(x)<std::int64_t(r.x)+r.width&&std::int64_t(y)<std::int64_t(r.y)+r.height)return &*hit;
                        }return nullptr;
                    };
                    const auto* pressed=hit_at(press_x,press_y);const auto* released=hit_at(click_x,click_y);
                    if(pressed&&pressed==released&&pressed->enabled&&mouse.press.x==press_x&&mouse.press.y==press_y&&
                       mouse.release.x==click_x&&mouse.release.y==click_y){
                        const auto* gameplay=std::get_if<civ5::ui::UiGameplayCommand>(&pressed->command);
                        const auto* technology=gameplay?gameplay->template as<civ5::ui::ProgressionChooseTechnology>():nullptr;
                        const auto* train=gameplay?gameplay->template as<civ5::ui::CityChooseProduction>():nullptr;
                        if(production_component_validation&&train&&gameplay->source_id()=="ProductionPopup"&&train->order_type==0)
                            clicked_train=*train;
                        const auto* stamp=live_ui_frame->snapshot.find<civ5::ui::BnwClientFrameStamp>();
                        const auto* research=live_ui_frame->snapshot.find<civ5::ui::BnwResearchChoiceUiSnapshot>();
                        if(technology&&stamp&&research){
                            check(composed_click_dispatch_sequence<std::numeric_limits<std::uint64_t>::max(),"Research click trace overflow");
                            clicked_research.emplace(ResearchClickObservation{*stamp,live_ui_frame->sequence,
                                live_ui_frame->snapshot.frame(),font_source_parent->session().state_generation(),
                                ++composed_click_dispatch_sequence,*technology,pressed->context_id,pressed->bounds,
                                static_cast<std::size_t>(pressed-live_screen_frame->hits.data()),mouse,research->research_queue,{}});
                        }
                    }
                }
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                if(clicked_research&&restart_preview&&!research_restart_fixture){
                    check(!restart_actual_input.empty()&&!restart_actual_game_root.empty(),"Research fixture original paths absent");
                    bool same_preview=false;
                    restart_preview->with_owned_host_camera_input([&](const auto& actual_host,const auto* actual_input){
                        check(actual_host&&actual_host.get()==font_source_parent.get()&&
                              !actual_host.owner_before(font_source_parent)&&!font_source_parent.owner_before(actual_host)&&
                              actual_input && &actual_input->session==&font_source_parent->session(),
                              "Research fixture wrapper is not the actual renderer Host/session");
                        same_preview=true;
                    });
                    check(same_preview,"Research fixture wrapper did not lend its actual Host");
                    research_restart_fixture=civ5::tests::BnwResearchChoiceRestartFixture::capture_before_action(
                        *restart_preview,live_client_scene->frame(),live_ui_host->screens().registry(),
                        restart_actual_input,restart_actual_game_root,output/"research-checkpoint-roundtrip");
                    prepared_viewer->gui_screen->require_current();
                    check(live_client_scene->accepts(*live_ui_host,*live_ui_frame),"Research source changed during pre-action fixture capture");
                }
#endif
                const auto dispatch = [&] {
                    routed = civ5::ui::click_composed_ui(*live_ui_host,
                        *live_ui_frame, *live_screen_frame,
                        press_x, press_y, click_x, click_y);
                    return routed.command;
                };
                if (live_client_scene)
                    routed.command = live_client_scene->dispatch(*live_ui_host, *live_ui_frame, dispatch);
                else dispatch();
                if(clicked_research&&routed.consumed&&routed.command.status==civ5::ui::UiCommandStatus::pending_refresh&&
                   live_alpha_research_backend){
                    const auto receipt=live_alpha_research_backend->last_commit();
                    check(receipt&&receipt->before_revision()==clicked_research->before.identity.ui_source_revision&&
                          receipt->player()==clicked_research->before.identity.player&&receipt->team()==clicked_research->before.identity.team,
                          "Research input trace differs from the actual issued commit");
                    clicked_research->receipt=receipt;research_click=std::move(clicked_research);
                }
                if(clicked_train&&routed.consumed&&routed.command.status==civ5::ui::UiCommandStatus::performed){
                    check(!validation_train_component_attempted&&!validation_train_component_requested,
                          "Production validation cannot replay the actual turn component");
                    validation_train_choice=*clicked_train;validation_train_component_requested=true;
                }
                input.observed_left_click.reset();
                if (input.live_modal_open)
                    check(routed.consumed, "Focused modal UI did not consume pointer click");
                live_screen_report << "click consumed=" << routed.consumed
                                   << " status=" << static_cast<int>(routed.command.status)
                                   << " reason=" << routed.command.reason << '\n';
            }
        }
        const bool text_input_pending =
            !input.search_characters.empty() || popup_name_editor.pending_submit();
        if (text_input_pending &&
            live_ui_host && live_ui_frame &&
            live_screen_frame && (search_editor.focused() || popup_name_editor.focused()) &&
            (!live_client_scene || live_client_scene->accepts(*live_ui_host, *live_ui_frame))) {
            const bool pedia = search_editor.focused();
            const auto routed = pedia
                ? search_editor.input(*live_ui_host, *live_ui_frame,
                    *live_screen_frame, input.search_characters)
                : popup_name_editor.input(*live_ui_host, *live_ui_frame,
                    *live_screen_frame, input.search_characters);
            live_screen_report << (pedia ? "civilopedia-search" : "popup-name") << " status="
                               << static_cast<int>(routed.status)
                               << " reason=" << routed.reason << '\n';
        }
        input.search_characters.clear();
        if (click_pending) {
            input.top_panel_click_pending = false;
            input.top_panel_click_x = input.top_panel_click_y = -1;
        }
        if (input.minimap_click_pending) {
            if (minimap_controller && saved_minimap_visible && presenter.minimap_chrome) {
                civ5::ui::MinimapUiState mini_state;
                mini_state.visible = *saved_minimap_visible;
                mini_state.visible_bounds_confirmed = native_minimap_bounds != nullptr;
                // The preview owns its local OptionsPanel visibility, but has
                // no strategic renderer or map-display mechanics owner.
                mini_state.strategic_view.reset();
                mini_state.options_visible = input.minimap_options_visible;
                mini_state.map_display = input.minimap_map_display;
                const auto mini_frame = civ5::ui::UiFrameSnapshot(ticks, 0, std::nullopt)
                    .with<civ5::ui::MinimapUiState>(
                        std::make_shared<const civ5::ui::MinimapUiState>(mini_state));
                civ5::ui::MinimapUiBackend mini_backend({
                    .pan_to_world = [&](float x, float y) {
                        const Vector3 target{x, y, controller.target[2]};
                        auto next = set_camera_position(
                            terrain->source().layout, terrain->source().map_mode, controller,
                            target, X87Precision::double_precision);
                        if (next.target != target) return false;
                        controller = std::move(next);
                        return true;
                    },
                    .set_options_visible = [&](bool visible) {
                        input.minimap_options_visible = visible;
                        return true;
                    }});
                const auto mini_plan = civ5::ui::plan_minimap_ui(
                    presenter.minimap_chrome->layout,
                    static_cast<std::uint32_t>(client_width),
                    static_cast<std::uint32_t>(client_height), mini_frame, &mini_backend);
                const auto pressed = civ5::ui::minimap_command_at(
                    mini_plan, mini_frame, input.minimap_press_x, input.minimap_press_y);
                const auto released = civ5::ui::minimap_command_at(
                    mini_plan, mini_frame, input.minimap_click_x, input.minimap_click_y);
                civ5::ui::UiCommandResult mini_result{
                    civ5::ui::UiCommandStatus::unavailable,
                    "Minimap control has no connected owner"};
                if (pressed && released &&
                    std::get<civ5::ui::UiGameplayCommand>(*pressed).type() ==
                        std::get<civ5::ui::UiGameplayCommand>(*released).type()) {
                    mini_result = minimap_controller->dispatch(*released, mini_frame, &mini_backend);
                } else if (pressed && released) {
                    mini_result = {civ5::ui::UiCommandStatus::invalid,
                                   "Press and release crossed controls"};
                }
                minimap_actions << ticks << ',' << input.minimap_press_x << ','
                                << input.minimap_press_y << ',' << input.minimap_click_x << ','
                                << input.minimap_click_y << ','
                                << static_cast<int>(mini_result.status) << ','
                                << mini_result.reason << '\n';
            }
            input.minimap_click_pending = false;
            input.minimap_click_x = input.minimap_click_y = -1;
        }
        const auto present_done = Clock::now();
        telemetry << ticks << ',' << clock.delta << ',' << controller.camera.world[12] << ','
                  << controller.camera.world[13] << ',' << controller.camera.world[14] << ','
                  << controller.zoom << ',' << controller.velocity[0] << ',' << controller.velocity[1] << ','
                  << water.camera_state().view_count << ',' << draws << ','
                  << std::chrono::duration<double, std::milli>(input_done - frame_start).count() << ','
                  << std::chrono::duration<double, std::milli>(render_done - input_done).count() << ','
                  << std::chrono::duration<double, std::milli>(present_done - render_done).count() << '\n';
        if (ticks % 60 == 0) {
            telemetry.flush();
            const auto title = L"Civ5 camera | LMB / Wheel | WASD / Arrows | Q E | R reset | I "
                               L"inertia=" +
                               std::wstring(input.inertia ? L"ON" : L"OFF") + L" | Z straight=" +
                               std::wstring(input.straight ? L"ON" : L"OFF") + L" | Esc";
            SetWindowTextW(window, title.c_str());
        }
        ++ticks;
        if (!smoke)
            std::this_thread::sleep_until(frame_start + std::chrono::milliseconds(16));
    }
    if (smoke) {
        std::cout << "Smoke ticks=" << ticks << "; capture=" << captures << "; release=" << releases
                  << "; reset=" << resets << "; wheels=" << input.wheel_messages << "; drag=" << drag_changed
                  << "; zoom=" << zoom_changed << "; inertia=" << inertia_ticks << "; cull=" << culling_checks
                  << "; topPanelActions=" << top_panel_actions
                  << '\n'
                  << std::flush;
        const bool top_panel_actions_match = smoke_science_action_expected
            ? top_panel_actions == 2 && top_panel_events.size() == 2 &&
                  top_panel_events[0].route == civ5::ui::TopPanelActionRoute::search_for_pedia_entry &&
                  top_panel_events[1].route == civ5::ui::TopPanelActionRoute::tech_tree_popup
            : top_panel_actions == 1 && top_panel_events.size() == 1 &&
                  top_panel_events[0].route == civ5::ui::TopPanelActionRoute::search_for_pedia_entry;
        const auto expected_last_action = smoke_science_action_expected
            ? civ5::ui::TopPanelAction::open_technology_tree
            : civ5::ui::TopPanelAction::open_civilopedia;
        if (smoke_tech_tree_close_expected) {
            check(smoke_tech_tree_pre_window_close_current_verified && input.close &&
                      native_window_close_retired && viewport_retired_pending &&
                      smoke_tech_tree_close_verified && smoke_tech_tree_occurrence && live_shared_screens &&
                      live_client_scene && live_ui_host && live_ui_frame && !live_client_scene->current() &&
                      !live_shared_screens->visible("TechTree") && live_shared_screens->focused_screen()==smoke_tech_tree_parent &&
                      !live_tech_tree_lifecycle->active_event(live_bnw_preview->session) &&
                      live_tech_tree_lifecycle->semaphore()==0 && !live_tech_tree_lifecycle->uncertain(),
                  "TechTree local lifecycle window-close retirement proof is unavailable");
            const auto effects=live_tech_tree_lifecycle->journal();
            const std::array expected{civ5::ui::BnwTechTreeEffect::queued,
                civ5::ui::BnwTechTreeEffect::show_semaphore_incremented,civ5::ui::BnwTechTreeEffect::popup_shown,
                civ5::ui::BnwTechTreeEffect::dequeued,civ5::ui::BnwTechTreeEffect::hide_semaphore_decremented,
                civ5::ui::BnwTechTreeEffect::popup_processed,civ5::ui::BnwTechTreeEffect::open_flag_cleared};
            check(effects.size()==expected.size(),"TechTree smoke did not observe exactly one seven-effect lifecycle");
            std::ostringstream receipt;
            receipt << "PASS Civis local TechTree science/source-CloseButton lifecycle; nativeVM=false RefreshDisplay=false\n"
                    << "civisOccurrence=" << smoke_tech_tree_occurrence->token << " player=" << smoke_tech_tree_occurrence->player_id
                    << " turn=" << smoke_tech_tree_occurrence->turn << " exitTick=" << ticks
                    << " restoredParent=" << smoke_tech_tree_parent.value_or("<world>") << '\n'
                    << "finalFrameCurrentBeforeWM_CLOSE=true windowCloseRetiredPublication=true\n";
            for (std::size_t n=0;n<effects.size();++n) {
                const auto &effect=effects[n];
                check(effect.effect==expected[n] && effect.occurrence.token==smoke_tech_tree_occurrence->token &&
                          effect.occurrence.player_id==smoke_tech_tree_occurrence->player_id && effect.occurrence.turn==smoke_tech_tree_occurrence->turn &&
                          effect.semaphore==(n==0 || n>=4 ? 0 : 1) && effect.data2==-1 && effect.processed_result==0 &&
                          !effect.native_vm_executed && !effect.refresh_display_executed,
                      "TechTree smoke effects are reordered/duplicated/foreign/uncertain");
                receipt << "effect=" << static_cast<int>(effect.effect) << " semaphore=" << effect.semaphore << '\n';
            }
            check(!live_shared_screens->visible("TechTree") && live_shared_screens->focused_screen()==smoke_tech_tree_parent &&
                      smoke_tech_tree_pre_window_close_current_verified && native_window_close_retired &&
                      !live_client_scene->current(),
                  "TechTree local lifecycle changed during final source guards");
            blend_validation::text(output / "tech-tree-local-lifecycle.txt",receipt.str());
        }
        check(ticks == smoke_exit_tick &&
                  (!smoke_readonly_expected || (smoke_strength_hover_verified && smoke_strength_clear_verified)) &&
                  captures == 1 && releases == 1 && resets == 1 && input.wheel_messages == 2 &&
                  drag_changed && zoom_changed && inertia_ticks > 0 && !input.inertia && input.straight &&
                  culling_checks == 3 && top_panel_actions_match && last_top_panel_action &&
                  *last_top_panel_action == expected_last_action &&
                  top_panel_events.back().action == expected_last_action,
              "Preview message/gesture/inertia/zoom/reset smoke checks");
        presenter.verify_scene_orientation(*context, client_width, client_height);
        blend_validation::text(output / "orientation-probe.txt",
                               "PASS asymmetric scene presentation: red/green top, blue/yellow bottom\n");
    }
    prepared_viewer->retire_client_audio();
    if(!prepared_viewer->client_audio_error.empty())
        std::cerr<<"Civis audio stopped/blocked: "<<prepared_viewer->client_audio_error<<'\n';
    if(worldview_font_source) worldview_font_source->retire();
    publication_guard.retire_borrowed_callbacks();
    if(!parent_prepared) {
        viewport_transaction.reset(); // Legacy detaches while HWND/input/borrowed owners remain alive.
        diagnostic_binding.reset();
    }
    minimap_viewport_pass.reset();
    if (minimap_viewport_framebuffer)
        minimap_gl.DeleteFramebuffers(1, &minimap_viewport_framebuffer);
    blend_validation::text(
        output / "result.txt",
        "PASS preview ticks=" + std::to_string(ticks) + "; captures=" + std::to_string(captures) +
            "; releases=" + std::to_string(releases) + "; inertiaTicks=" + std::to_string(inertia_ticks) +
            "; cullingFullImageChecks=" + std::to_string(culling_checks) +
            "; topPanelActions=" + std::to_string(top_panel_actions) + "\n");
    if (owned_warrior) {
        check(warrior_draws > 0, "Owned Warrior never reached the live visible draw route");
        check(!smoke || warrior_changed_pixels > 0, "Owned Warrior produced no pixels in the shared scene");
        blend_validation::text(output / "bnw-warrior-preview-draw.json",
            "{\"schema\":\"bnw-warrior-shared-preview-draw-v1\",\"status\":\"PASS\","
            "\"sharedCameraDepthFow\":true,\"liveVisibilityCheckedPerFrame\":true,\"draws\":" +
            std::to_string(warrior_draws) + ",\"hiddenFrames\":" + std::to_string(warrior_hidden_frames) +
            ",\"maxChangedScenePixels\":" + std::to_string(warrior_changed_pixels) +
            ",\"membersPerDraw\":12,\"trianglesPerDraw\":7920,\"bufferUploads\":" +
            std::to_string(owned_warrior->buffer_uploads()) +
            ",\"modelDrawable\":false,\"fullGraphicsParity\":false}\n");
    }
    std::cout << "PASS camera preview closed normally\n" << std::flush;
    if(prepared_viewer->existing_menu_binding){
        check(presenter.actual_client_present_returns_>0,"Party loop returned without actual source-checked SwapBuffers");
        check(viewport_transaction!=nullptr,"Party Return lost actual close transaction");
        viewport_transaction->require_staged_native_close_owned(prepared_viewer->native_close_occurrence);
        prepared_viewer->party_loop_return_=std::unique_ptr<const PreparedCameraViewer::PartyLoopReturn>(
            new PreparedCameraViewer::PartyLoopReturn(*prepared_viewer,presenter.actual_client_present_returns_));
    }
}
};
} // namespace civ5::validation
namespace {
// Preserve every existing caller and the run body's default arguments.
template <class LandRenderer>
void run_camera_preview(std::shared_ptr<GlContext> context, std::shared_ptr<const MaterialWorld> world,
                        LandRenderer &land, Control &controls, GlLandMaterialInputs land_scene,
                        const fs::path &root, const fs::path &output, bool smoke,
                        civ5::ui::TopPanelRuntime::StateProvider top_panel_state_provider,
                        bool original_lighting = false, const GlDistanceFog *distance_fog = nullptr,
                        bool original_fow = false, const GlColorKeys *color_keys = nullptr,
                        bool original_rivers = false,
                        civ5::ui::TopPanelActionSink top_panel_action_sink = {},
                        const civ5::assets::ResourceMountTable *ui_resources_override = nullptr,
                        const civ5::saves::SavedMapTeamArtObservation *saved_art = nullptr,
                        const civ5::saves::SavedMapProjection *saved_minimap = nullptr,
                        bool smoke_fixture_tooltips_expected = true,
                        std::string_view ui_language = "en_US",
                        std::optional<std::int32_t> saved_turn_zero_year = {},
                        const fs::path &native_ice_records_path = {},
                        bool focus_revealed_ice = false,
                        const fs::path &native_minimap_edge_records_path = {},
                        std::string_view saved_save_sha256 = {},
                        const fs::path &native_minimap_city_marker_fixture_path = {},
                        civ5::ui::LiveUiHost::FrameProvider live_ui_provider = {},
                        civ5::ui::UiGameplayBackend *live_ui_backend = nullptr,
                        const civ5::ui::LiveBnwPreviewInput *live_bnw_preview = nullptr,
                        const RetailPreviewCamera *retail_camera = nullptr,
                        const civ5::ui::MinimapNativeBoundsFixture *native_minimap_bounds = nullptr,
                        bool captured_g0_selected_marker_visible = false,
                        const civ5::graphics::BnwRendererSceneReceipt *live_scene_receipt = nullptr,
                        civ5::graphics::BnwClientCaptureHook live_client_capture = {},
                        std::shared_ptr<civ5::graphics::BnwUiSourceObservation> ui_source_observation = {},
                        civ5::graphics::GlBnwWarriorOwnedScene *owned_warrior = nullptr,
                        std::shared_ptr<PreparedCameraViewer> prepared_viewer = {},
                        bool early_initial_smoke = false,
                        std::shared_ptr<const civ5::ui::SelectedBnwPreviewHost> font_source_parent = {},
                        std::optional<civ5::assets::ResourceMountTable>* actual_main_ui_owner = nullptr,
                        bool worldview_font_source_gates = false,
                        bool production_component_validation = false
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                        ,const civ5::ui::SelectedBnwReadOnlyPreview* restart_preview = nullptr,
                        const fs::path& restart_actual_input = {},
                        const fs::path& restart_actual_game_root = {}
#endif
                        ,std::shared_ptr<civ5::ui::BnwWorldViewFontSourceScope> actual_own_font_source = {},
                        civ5::ui::detail::CivisFrontendAlphaLandFrame* actual_alpha_frame = nullptr,
                        const civ5::ui::detail::CivisFrontendPartyMaterialStorage* actual_material = nullptr
                        ) {
    civ5::validation::CameraPreviewLoop<LandRenderer>::run(std::move(context),
        std::move(world),
        land,
        controls,
        std::move(land_scene),
        root,
        output,
        smoke,
        std::move(top_panel_state_provider),
        original_lighting,
        distance_fog,
        original_fow,
        color_keys,
        original_rivers,
        std::move(top_panel_action_sink),
        ui_resources_override,
        saved_art,
        saved_minimap,
        smoke_fixture_tooltips_expected,
        ui_language,
        saved_turn_zero_year,
        native_ice_records_path,
        focus_revealed_ice,
        native_minimap_edge_records_path,
        saved_save_sha256,
        native_minimap_city_marker_fixture_path,
        std::move(live_ui_provider),
        live_ui_backend,
        live_bnw_preview,
        retail_camera,
        native_minimap_bounds,
        captured_g0_selected_marker_visible,
        live_scene_receipt,
        std::move(live_client_capture),
        std::move(ui_source_observation),
        owned_warrior,
        std::move(prepared_viewer),
        early_initial_smoke,
        std::move(font_source_parent),
        actual_main_ui_owner,
        worldview_font_source_gates,
        production_component_validation
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
        ,restart_preview,restart_actual_input,restart_actual_game_root
#endif
        ,std::move(actual_own_font_source),actual_alpha_frame,actual_material
        );
}
} // namespace
