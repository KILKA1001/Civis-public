#pragma once
#include "civ5/ui/bnw_scroll_animation_update_loan.hpp"

#include "civ5/ui/screen_compositor.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/ui/bnw_worldview_renderer_projection_loan.hpp"
#include "civ5/ui/bnw_tech_award_context_snapshot.hpp"
#include "civ5/ui/glyphgen_style.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/ui/glyphgen.hpp"
#include "bnw_label_process_module_owner.hpp"
#include "civ5/graphics/detail/bnw_font_texture_observation.hpp"
#include "civ5/ui/detail/bnw_font_texture_request.hpp"
#include "civ5/ui/detail/bnw_label_system_font_request.hpp"
#include "civ5/ui/detail/bnw_label_system_font_source.hpp"
#include "civ5/ui/detail/bnw_label_actual_paint_call_frame.hpp"
#include "civ5/graphics/detail/bnw_label_system_font_atlas_metadata.hpp"
#include "civ5/graphics/gl_bnw_system_font_atlas.hpp"
#include "civ5/graphics/detail/bnw_renderer_context_pin.hpp"
#include "civ5/runtime/detail/bnw_font_resource_loan.hpp"
#include "civ5/ui/measured_readonly_text.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_bnw_font_shader.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/graphics/bnw_clear_hex_highlight_owner.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <thread>

namespace civ5::graphics::detail {class BnwLabelDefaultFontTargetLoan;class BnwNormalResourceGpuImageLoan;}
namespace civ5::validation {struct PreviewPresenter;}
namespace civ5::ui::detail {class BnwStandalonePositionedLabelRunLoan;class BnwNormalCaptionFp32RectLoan;}
namespace civ5::ui::detail {class BnwOriginalVertexDeclarationCreateLoan;class BnwOriginalVertexDeclarationDeviceResultLoan;}
namespace civ5::ui::detail {class BnwNormalRootPaintCallFrame;class BnwOriginalNonFontDeviceInitializer;class BnwOriginalGeneratedResourceImageLoan;class BnwNormalCommonChildCall;class BnwNormalButtonCaptionCall;class BnwWindowedRendererMatrixDataLoan;}
namespace civ5::runtime {class BnwSelectedStockLuaBootstrap;class BnwSelectedStockLuaSourceOwner;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {
class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;class BnwOwnedCameraGuiScreen;
class BnwTechAwardWindowPointerLoan;
class BnwNotificationPublicationLender;class BnwNotificationTerrainSceneLoan;

struct UiGlyphBinding {
    const GlyphGenGeometry *geometry = nullptr;
    const graphics::GlTexture *texture = nullptr;
};
struct UiGlyphStyleLayerBinding {
    UiGlyphBinding glyphs;
    std::size_t color_index = 0;
};
struct UiGlyphStyleBinding {
    const GlyphGenGeometry *layout_geometry = nullptr;
    std::vector<UiGlyphStyleLayerBinding> layers;
};
using UiGlyphStyleResolver = std::function<const UiGlyphStyleBinding*(std::string_view,std::string_view)>;
struct UiGlPaintReport {
    std::size_t rectangles = 0, textures = 0, glyphs = 0, icons = 0;
    std::vector<std::string> unresolved;
    MeasuredReadonlyTextReport readonly_text;
    // Measured own C++ choices, not a native metric/GPU applied receipt.
    std::vector<UnitNameOwnFontSelection> unit_name_own_font_selections;
    std::vector<UnitNameOwnFontSelectionRefusal> unit_name_font_refusals;
};

// Paints the ordered screen stream on the current GL framebuffer. The caller
// supplies the active GlyphGen atlases, including any locale extensions, and
// retains them for this renderer's lifetime. DDS/Grid9 art is resolved only
// through the same active base -> Expansion -> Expansion2 resource mounts.
class UiScreenGlRenderer {
  public:
#ifdef _WIN32
    class PresentationTarget final {
        friend class UiScreenGlRenderer;
        friend struct ::civ5::validation::PreviewPresenter;
        std::weak_ptr<BnwTechAwardAnimationSample::Clock> clock_;
        UiScreenGlRenderer* const renderer_;
        explicit PresentationTarget(UiScreenGlRenderer& owner):clock_(owner.tech_award_lifetime_),renderer_(&owner){}
        void acknowledge(const graphics::GlContext&)const;
      public:
        ~PresentationTarget()=default;
        PresentationTarget(const PresentationTarget&)=delete;
        PresentationTarget& operator=(const PresentationTarget&)=delete;
    };
    // Only the real PreviewPresenter can call this target after returned Swap
    // and its existing source/native POST. No caller bool issues presentation.
    std::shared_ptr<const PresentationTarget> tech_award_presentation_target();
#endif
    UiScreenGlRenderer(const graphics::GlContext &context,
                       const assets::ResourceMountTable &resources,
                       UiGlyphBinding font16, UiGlyphBinding font20,
                       UiGlyphBinding icons = {},
                       std::function<std::optional<std::uint32_t>(std::string_view)> icon_resolver = {},
                       UiGlyphBinding font24 = {}, UiGlyphStyleResolver font_styles = {});
    ~UiScreenGlRenderer();
    UiScreenGlRenderer(const UiScreenGlRenderer &) = delete;
    UiScreenGlRenderer &operator=(const UiScreenGlRenderer &) = delete;
    UiGlPaintReport paint(const ComposedUiFrame &frame, std::int32_t width,
                          std::int32_t height);

    // Consumes only the actual queued payload of this same current Scene frame.
    // Texture/font caches retain selected source controls; data alone issues no
    // native input, animation completion or Original UI registry receipt.
    UiGlPaintReport paint_tech_award(const LiveUiFrame&, const BnwClientFrame&,
        std::int32_t width, std::int32_t height, std::optional<ClientPoint>,
        const std::function<bool()>& source_current);
    // Only the native renderer loop mints an immutable one-update sample.
    std::shared_ptr<const BnwTechAwardAnimationSample> begin_tech_award_ui_update(
        const std::shared_ptr<BnwClientSceneFrame>&,const std::shared_ptr<LiveUiHost>&,
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&);
    void update_tech_award_animation(const LiveUiFrame&,const BnwClientFrame&,
        const std::shared_ptr<const BnwTechAwardAnimationSample>&,
        const std::function<bool()>& source_current);
#ifdef _WIN32
    // Data capture only while the real native sink loan is entered. Queue
    // consumption requires successful sink destruction and native unwind.
    void capture_tech_award_pointer(const std::shared_ptr<const BnwTechAwardWindowPointerLoan>&,
        const LiveUiFrame&,const std::shared_ptr<const BnwClientFrame>&);
    void consume_tech_award_pointer(const std::function<bool()>& source_current);
#endif
    // Uses the same retained glyph/icon owner and selected TTManager/Grid9
    // plan, immediately after paint. Compatibility metrics remain explicit.
    void paint_readonly_tooltip(const TopPanelTooltipPlan &, const TopPanelGrid9Style &,
        UiGlPaintReport &, std::int32_t width, std::int32_t height);

  private:
    friend class BnwWorldViewRendererProjectionLoan;
    friend class BnwWorldViewRendererRunLoan;
    friend class BnwWorldViewControlRenderPublication;
    friend class BnwClientSceneFrame;
    template<class LandRenderer>friend struct ::civ5::validation::CameraPreviewLoop;
    std::pair<std::unique_ptr<BnwScrollAnimationLoopTick>,std::shared_ptr<const BnwTechAwardAnimationSample>> begin_scroll_animation_loop_tick_owned(
        const std::shared_ptr<BnwClientSceneFrame>&,const std::shared_ptr<LiveUiHost>&,
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&);
    void advance_worldview_scroll_update_owned(const std::shared_ptr<BnwScrollAnimationLoopTick::State>&,
        runtime::BnwSelectedStockLuaSourceOwner&,const runtime::BnwWorldViewSourceOperationLoan&,
        const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&);
    friend class detail::BnwLabelProcessModuleOwner;
    friend class ::civ5::graphics::detail::BnwFontTextureBorrower;
    friend class ::civ5::graphics::detail::BnwLabelSystemFontAtlasMetadata;
    struct SystemFontCacheEntry;
    struct SystemFontCacheStorage;
    std::shared_ptr<SystemFontCacheStorage> system_font_cache_;
    std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> query_system_font_atlas_owned(
        const BnwWorldViewRendererProjectionLoan&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const detail::BnwLabelSystemFontRequest::Guard&);
    using WorldViewPaintLifetime=BnwWorldViewRendererProjectionLoan::Lifetime;
    const std::shared_ptr<WorldViewPaintLifetime> worldview_lifetime_=std::make_shared<WorldViewPaintLifetime>();
    std::shared_ptr<detail::BnwLabelProcessModuleOwner> label_module_;
    struct FontTextureCacheEntry;
    struct FontTextureCacheStorage;
    std::shared_ptr<FontTextureCacheStorage> font_texture_cache_;
    std::shared_ptr<const FontTextureCacheEntry> retain_font_texture_payload_owned(
        const BnwWorldViewRendererProjectionLoan&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const runtime::detail::BnwFontResourceLoan&,
        const detail::BnwFontTextureRequest&);
    friend class detail::BnwLabelActualPaintCallFrame;
    friend class detail::BnwNormalRootPaintCallFrame;
    friend class graphics::detail::BnwLabelDefaultFontTargetLoan;
    friend class detail::BnwOriginalNonFontDeviceInitializer;
    std::shared_ptr<const detail::BnwOriginalVertexDeclarationDeviceResultLoan> create_original_vertex_declaration_owned(
        const std::shared_ptr<const detail::BnwOriginalNonFontDeviceInitializer>&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const detail::BnwOriginalVertexDeclarationCreateLoan&);
    void require_original_vertex_declaration_device_current_owned(
        const std::shared_ptr<const detail::BnwOriginalVertexDeclarationDeviceResultLoan>&,
        const runtime::BnwWorldViewSourceOperationLoan&) const;
    struct OriginalResource0Storage;
    std::shared_ptr<const OriginalResource0Storage> original_resource0_storage_;
    std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan> upload_original_resource0_storage_owned(
        const std::shared_ptr<const detail::BnwOriginalNonFontDeviceInitializer>&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::BnwOriginalGeneratedResourceImageLoan>&);
    void require_original_resource0_storage_current_owned(
        const std::shared_ptr<const detail::BnwOriginalNonFontDeviceInitializer>&,
        const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const detail::BnwOriginalGeneratedResourceImageLoan>&,
        const std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan>&)const;
    std::shared_ptr<const detail::BnwLabelActualPaintCallFrame> capture_label_actual_paint_call_frame_owned();
    struct NormalLabelRunForActualItem final {
        std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan> run;
        std::size_t actual_index;
    };
    NormalLabelRunForActualItem capture_normal_label_run_for_actual_item_owned(
        std::shared_ptr<const detail::BnwLabelActualPaintCallFrame>);
    struct LabelPaintCapture;
    std::unique_ptr<LabelPaintCapture> capture_label_actual_positioned_owned(bool normal_only=false);
    // Renderer-owned physical library cache. Per-DATA logical use is separate.
    struct LabelOriginalProgramCache;
    std::shared_ptr<LabelOriginalProgramCache> label_original_program_cache_;
    // 0 idle,1 preparing,2 sticky failed. Constructor-created; no readiness flag.
    const std::shared_ptr<std::atomic<unsigned>> label_program_preparation_=std::make_shared<std::atomic<unsigned>>(0);
    struct LabelNativeProgramCapture;
    std::unique_ptr<LabelNativeProgramCapture> capture_label_native_programs_owned(std::shared_ptr<LabelPaintCapture>);
    // Lexical submit exclusion before role/source/allocator callbacks, no issuer.
    const std::shared_ptr<std::atomic<unsigned>> label_submission_=std::make_shared<std::atomic<unsigned>>(0);
    bool submit_current_normal_label_owned(std::size_t actual_index);
    struct LabelPaintItemScope;
    struct LabelDefaultFramebufferTarget;
    struct LabelDefaultRowsAdapter;
    struct LabelDefaultFontTargetBridgeState;
    std::unique_ptr<graphics::detail::BnwLabelDefaultFontTargetLoan> capture_label_default_font_target_owned(
        const LabelDefaultRowsAdapter&,const detail::BnwLabelPaintPositionedData&);
    std::unique_ptr<LabelDefaultRowsAdapter> begin_label_default_rows_adapter_owned(const LabelDefaultFramebufferTarget&);
    std::unique_ptr<LabelDefaultFramebufferTarget> capture_label_default_framebuffer_target_owned(const LabelPaintCapture&);
    struct WorldViewPaintScope;
    struct WorldViewGridProvider;
    //0 idle,1 actual PRE in progress,2 sticky failure; never a ready flag.
    const std::shared_ptr<std::atomic<unsigned>> clear_highlight_preparation_=std::make_shared<std::atomic<unsigned>>(0);
    void prepare_clear_hex_highlights_owned(const BnwWorldViewRendererProjectionLoan&,
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&)const;
    void initialize_clear_hex_highlight_model_owned(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<graphics::BnwClearHexHighlightOwner>&,std::size_t,
        const assets::CivisLoadedResource&)const;
    struct NormalButtonCaptionCapture;
    std::shared_ptr<NormalButtonCaptionCapture> capture_current_normal_button_caption_owned(
        const detail::BnwNormalCommonChildCall&,const detail::BnwNormalButtonCaptionCall&,
        std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>);
    // Geometry arguments remain private data from the pending exact native
    // projection mapping. No caller is issued until that mapping is closed.
    std::shared_ptr<const BnwWorldViewControlRenderPublication> append_current_normal_button_caption_publication_owned(
        std::shared_ptr<NormalButtonCaptionCapture>,
        std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>,WorldViewRect);
    void prepare_current_normal_button_caption_cpu_item_owned(std::shared_ptr<NormalButtonCaptionCapture>);
    std::unique_ptr<detail::BnwNormalLabelMaterialLoan> paint_current_normal_button_caption_owned(
        std::shared_ptr<NormalButtonCaptionCapture>,
        std::shared_ptr<const BnwWorldViewControlRenderPublication>);
    WorldViewPaintScope* worldview_scope_without_callbacks()const;
    // Private current-paint admission; Root enters the SAME GUI/window lender.
    void with_notification_terrain_scene_owned(BnwNotificationPublicationLender&,
        const std::function<void(BnwNotificationTerrainSceneLoan&)>&);
    std::shared_ptr<const BnwClientFrame> capture_worldview_projection_frame(const std::shared_ptr<BnwClientSceneFrame>&)const;
    void require_worldview_projection_current(const BnwWorldViewRendererProjectionLoan&)const;
    UiGlPaintReport paint_owned_worldview(runtime::BnwSelectedStockLuaSourceOwner&,
        const BnwOwnedCameraGuiScreen&,const BnwWorldViewRendererProjectionLoan&,const BnwScrollAnimationLoopTick* =nullptr);
    // Genuine queued WM_CLOSE only; SourceOwner owns close issuance and
    // synchronous Original shutdown callbacks. Caller retires AFTER unwind.
    void shutdown_owned_worldview(runtime::BnwSelectedStockLuaSourceOwner&,
        const BnwOwnedCameraGuiScreen&,const BnwWorldViewRendererProjectionLoan&);
    UiGlPaintReport paint_worldview_control_publication(const BnwWorldViewControlRenderPublication&,
        const ComposedUiFrame&,std::int32_t,std::int32_t);
    UiGlPaintReport paint_worldview_frame_owned(const ComposedUiFrame&,std::int32_t,std::int32_t);
    const UiGlyphStyleBinding* resolve_worldview_font_style(std::string_view,std::string_view);
    std::function<std::optional<std::uint32_t>(std::string_view)> worldview_icon_resolver();
    const graphics::GlContext &context_;
    const assets::ResourceMountTable &resources_;
    graphics::GlRenderApi gl_;
    UiGlyphBinding font16_, font20_, font24_, icons_;
    std::function<std::optional<std::uint32_t>(std::string_view)> icon_resolver_;
    UiGlyphStyleResolver font_styles_;
    graphics::GlTexture white_;
    std::unordered_map<std::string, graphics::GlTexture> textures_;
    // Retained native raw source/index and all authored mips, separate from
    // expanded/sRGB generic UI textures. Stable pointees; cache grants no draw.
    struct LayoutResourceEntry;
    std::unordered_map<std::string,std::shared_ptr<const LayoutResourceEntry>> layout_resource_entries_;
    const std::shared_ptr<std::atomic<bool>> layout_resource_active_=std::make_shared<std::atomic<bool>>(false);
    std::uint64_t layout_resource_revision_=0;
    // Actual entered Source callback, before immutable paint revisions. Root
    // provides the real NativeIssuer initializer; no public size/ready grant.
    void with_notification_layout_resources_owned(const BnwWorldViewRendererProjectionLoan&,
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&,const std::function<void()>&);
    struct MaskedTexture;
    std::unordered_map<std::string,std::shared_ptr<const MaskedTexture>> masked_textures_;
    const std::shared_ptr<std::atomic<bool>> masked_resource_active_=std::make_shared<std::atomic<bool>>(false);
    struct MaskedResources {
        std::array<const graphics::GlTexture*,4> textures{};
        std::array<std::string,4> keys;
        std::array<std::array<float,4>,8> registers{};
        std::array<std::array<std::uint32_t,2>,2> logical_sizes{};
    };
    void require_masked_resources_origin_without_callbacks()const;
    const MaskedTexture& masked_texture_owned(const std::string&);
    MaskedResources masked_resources_owned(const BnwWorldViewControlRenderPublication&,const WorldViewPaint&);
    std::unordered_map<std::string, std::optional<TopPanelGrid9Style>> styles_;
    GLuint program_ = 0, vao_ = 0;
    GLint rect_ = -1, viewport_ = -1, uv_ = -1, tint_ = -1, alpha_only_ = -1;

    // Primitive only; private actual publication/frame and outside borrowed
    // texture/source tuple remain mandatory. Root owns its source factory and
    // dispatch from the canonical ControlQuote, not a caller bool/rect/phase.
    const std::shared_ptr<std::atomic<bool>> masked_draw_active_=std::make_shared<std::atomic<bool>>(false);
    GLuint masked_program_=0, masked_vao_=0, masked_buffer_=0, masked_index_buffer_=0;
    std::array<GLuint,4> masked_samplers_{};
    GLint masked_viewport_=-1;
    std::array<GLint,8> masked_registers_{};
    void create_masked_pipeline_owned();
    // Immutable publisher supplies corner order (1,1),(0,1),(1,0),(0,0).
    // Owned IBO [2,1,0,3,1,2] follows native primitive topology; no vertex/UV
    // fabrication or sorting. Actual clip/cull/viewport admission stays Root.
    void masked_quad_owned(const BnwWorldViewControlRenderPublication&,const WorldViewPaint&,
        const std::array<const graphics::GlTexture*,4>&,
        const std::array<std::string_view,4>&);
    using TechAwardPaintLifetime=BnwTechAwardAnimationSample::Clock;
    std::shared_ptr<TechAwardPaintLifetime> tech_award_lifetime_=std::make_shared<TechAwardPaintLifetime>();
    std::weak_ptr<const BnwTechAwardAnimationSample> tech_award_updated_sample_;
    std::weak_ptr<const BnwTechAwardAnimationWitness> tech_award_updated_context_;
    std::vector<std::pair<std::string,float>> tech_award_alphas_;
    struct AnimationSources;
#ifdef _WIN32
    struct PointerReceipt;
    std::vector<std::shared_ptr<const PointerReceipt>> tech_award_pointer_queue_;
    std::shared_ptr<const BnwTechAwardWindowPointerLoan> tech_award_pointer_previous_;
    std::weak_ptr<const BnwTechAwardAnimationWitness> tech_award_painted_context_;
    std::weak_ptr<const BnwClientFrame> tech_award_painted_frame_;
    std::optional<LiveUiFrame> tech_award_painted_ui_;
    std::weak_ptr<const BnwTechAwardAnimationWitness> tech_award_presented_context_;
    std::weak_ptr<const BnwClientFrame> tech_award_presented_frame_;
    void acknowledge_tech_award_presented(const graphics::GlContext&);
#endif
    struct TechAwardTexture {
        std::shared_ptr<const assets::UiTextureBinding> source;
        graphics::GlTexture texture;
    };
    struct TechAwardFont {
        std::string key, style, sha256;
        GlyphGenStyleAsset source;
        std::vector<graphics::GlTexture> textures;
    };
    std::vector<TechAwardTexture> tech_award_textures_;
    std::vector<TechAwardFont> tech_award_fonts_;
    void tech_award_quad(const graphics::GlTexture&, std::array<float,4> rect,
        std::array<float,4> uv, UiFloatColor tint, bool alpha_only,
        std::int32_t width, std::int32_t height);
    const graphics::GlTexture *texture(std::string_view key, UiGlPaintReport &report);
    const TopPanelGrid9Style *style(std::string_view name, UiGlPaintReport &report);
    void quad(const graphics::GlTexture &texture, const WorldViewRect &dest,
              std::array<float, 4> uv, std::uint32_t rgba, bool alpha_only,
              const WorldViewRect &clip, std::optional<UiFloatColor> float_tint = std::nullopt);
    void grid(const WorldViewPaint &paint, std::string_view name, UiGlPaintReport &report,
              const WorldViewRect &clip);
    std::optional<WorldViewRect> anchored_text(const WorldViewPaint &paint,
        UiGlPaintReport &report, const WorldViewRect &clip);
    void text(const WorldViewPaint &paint, UiGlPaintReport &report,
              const WorldViewRect &clip);
};

} // namespace civ5::ui
