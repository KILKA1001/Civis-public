#pragma once
#include "civ5/graphics/gl_context.hpp"
#include "civ5/ui/win32_client_window.hpp"
#include "civ5/ui/frontend_content.hpp"
#include <memory>
namespace civ5::validation {template<class T>struct CameraPreviewLoop;}
namespace civ5::runtime {class BnwSelectedStockLuaSourceOwner;}
namespace civ5::ui::detail {
class CivisFrontendPartyRendererBackend;
// Construct before Main's sole binding. The actual Camera input/router lives
// here; Main binds this exact object and forwards real events first.
class CivisFrontendPartyRoutingOwner final {
    struct State;
    std::shared_ptr<State> state_;
    explicit CivisFrontendPartyRoutingOwner(std::shared_ptr<State>);
    friend class CivisFrontendPartyRendererBackend;
    template<class T>friend struct ::civ5::validation::CameraPreviewLoop;
    void begin_camera_input_owned(const runtime::BnwSelectedStockLuaSourceOwner&);
public:
    static std::shared_ptr<CivisFrontendPartyRoutingOwner> create(
        std::shared_ptr<graphics::GlContext> actual_existing_context);
    ~CivisFrontendPartyRoutingOwner();
    CivisFrontendPartyRoutingOwner(const CivisFrontendPartyRoutingOwner&)=delete;
    CivisFrontendPartyRoutingOwner& operator=(const CivisFrontendPartyRoutingOwner&)=delete;
    ClientWindowInput& window_input()const;
    void attach_existing_binding(Win32ClientWindowBinding& actual);
    // Call after application/backend targets are released and native dispatch
    // has returned, BEFORE the sole binding destructor, also on unwind.
    void retire_owned()noexcept;
    // Native sink route only: actual source capture and bounded event queue.
    // No provider, GL, decode or nested message pump is invoked here.
    bool route(const ClientWindowEvent& actual_event);
};
// Separate stage AFTER the same existing binding is attached. World-input
// source provenance is a required producer boundary; no metadata fallback.
std::shared_ptr<FrontendBackend> create_civis_frontend_party_renderer_backend(
    const FrontendCatalogContentAdapter& actual_content,
    std::shared_ptr<CivisFrontendPartyRoutingOwner> actual_routing,
    std::filesystem::path actual_game_root,
    std::filesystem::path canonical_payload_root,
    std::filesystem::path actual_output_directory,
    std::function<void(std::string_view)> loading_progress = {});
}
