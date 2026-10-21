#pragma once

#include "civ5/ui/city_screens.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/ideology_layout.hpp"
#include "civ5/ui/overview_screens.hpp"
#include "civ5/ui/popup_screens.hpp"
#include "civ5/ui/system_screens.hpp"
#include <array>
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"

namespace civ5::ui {
class BnwClientSceneFrame;

struct ComposedUiFrame {
    std::vector<WorldViewPaint> paints;
    std::vector<WorldViewOwnedPaintSpan> worldview_owned_paint_spans;
    std::vector<LiveUiHit> hits;
    std::vector<std::string> unresolved;
    std::optional<std::string> focused_context;
    std::optional<WorldViewRect> production_scroll_panel;
    std::optional<WorldViewRect> popup_scroll_panel;
    std::vector<PolicyScrollRegion> policy_scroll_regions;
    std::optional<TechPopupScrollRegion> tech_popup_scroll_region;
    std::optional<IdeologyScrollRegion> ideology_scroll_region;
    std::vector<IdeologyTooltipKey> ideology_tooltip_keys;
    std::vector<IdeologyTenetHeaderState> ideology_tenet_headers;
    std::optional<WorldViewRect> civilopedia_search_box;
    std::optional<WorldViewRect> civilopedia_article_scroll_panel;
    std::optional<WorldViewRect> popup_edit_box;
    std::uint64_t frame_sequence = 0, snapshot_frame = 0;
    const void *frame_owner = nullptr;
    std::vector<WorldViewReadonlyTextTooltip> readonly_tooltips;
};

struct MeasuredProductionRow {
    CityChooseProduction choice;
    WorldViewRect bounds;
};

// Screen-space rectangles come from the presenter after selected XML layout
// and dynamic ProdButton instances have been placed. Bounds alone never grant
// eligibility; the planned typed choice supplies enabled/reason.
struct MeasuredProductionPopup {
    std::string selected_source_path;
    std::optional<WorldViewRect> scroll_panel, up_button, down_button;
    std::vector<MeasuredProductionRow> rows;
};

// The renderer measures the selected XML controls and dynamic ProdButton
// instances after layout. Row bounds are in visible display order. A missing
// rectangle leaves that row without an input target.
struct ProductionPopupControlRects {
    const void *frame_owner = nullptr;
    std::uint64_t frame_sequence = 0, snapshot_frame = 0;
    std::string selected_source_path;
    std::optional<WorldViewRect> scroll_panel, up_button, down_button;
    std::vector<std::optional<WorldViewRect>> visible_choice_buttons;
};

// Converts the independent U4-U9 screen plans to one ordered paint/hit stream.
// The live host still owns frame identity, focus, and command dispatch.
class UiScreenCompositor {
  public:
    UiScreenCompositor(const assets::ResourceMountTable &resources,
                       const UiContextRegistry &registry,
                       UiGameplayBackend *backend = nullptr,
                       OverviewLocalizer localize = {},
                       OverviewLanguage overview_language = OverviewLanguage::english);
    ~UiScreenCompositor(){gui_producer_.life.reset();}
    ComposedUiFrame compose(const LiveUiFrame &frame,
                            const UiScreenController &screens,
                            std::int32_t width, std::int32_t height,
                            const MeasuredProductionPopup *production = nullptr,
                            const EconomicBoundFields *economic_fields = nullptr) const;
    // Executes THIS compositor against the genuine registered selected frame.
    std::shared_ptr<const BnwOwnedCameraGuiScreen> compose_owned_current_screen(
        const std::shared_ptr<BnwClientSceneFrame>&,
        const std::shared_ptr<LiveUiHost>&,
        const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&,
        const LiveUiFrame&,const MeasuredProductionPopup* production=nullptr,
        const EconomicBoundFields* economic_fields=nullptr)const;
    // Renderer installs its genuine scoped publication sink. It does not
    // issue a screen from arbitrary snapshots or acknowledge GPU presentation.
    void bind_owned_research_renderer_publication(
        const std::shared_ptr<const BnwOwnedCameraGuiScreen>&,
        const std::weak_ptr<const std::uint8_t>& renderer_lifetime,
        const std::function<void(const std::shared_ptr<const BnwOwnedCameraGuiScreen>&,
                                 const LiveUiFrame&,const ComposedUiFrame&)>&)const;
    const CityScreenSource &production_popup_source() const { return city_[1]; }

  private:
    friend class BnwOwnedCameraGuiScreen;
    struct GuiRetainedStorageTag {};
    UiScreenCompositor(GuiRetainedStorageTag,const UiScreenCompositor&);
    // Each actual compositor facade has its own lifetime, including copies.
    // A GUI child holds only weak identity; it never prolongs borrowed localizers.
    struct GuiProducerWitness {
        const std::thread::id thread=std::this_thread::get_id();
        std::shared_ptr<const std::uint8_t> life=std::make_shared<const std::uint8_t>(0);
        GuiProducerWitness()=default;
        GuiProducerWitness(const GuiProducerWitness&):life(std::make_shared<const std::uint8_t>(0)){}
        GuiProducerWitness& operator=(const GuiProducerWitness&) {
            life.reset();return *this;
        }
    } gui_producer_;
    std::array<ProgressionScreenSource, 4> progression_;
    std::array<CityScreenSource, 2> city_;
    std::optional<content::XmlNode> city_list_xml_;
    std::vector<OverviewSource> overview_;
    PopupCatalog popups_;
    std::vector<SystemScreenSource> system_;
    UiGameplayBackend *backend_;
    // Actual constructor-minted callback storage. GUI snapshots share its
    // canonical target; copying borrowed std::function cannot destroy its source.
    std::shared_ptr<const OverviewLocalizer> gui_localizer_;
    OverviewLocalizer localize_;
    OverviewLanguage overview_language_;
};

struct PresentedUiFrame {
    LiveUiFrame live;
    ComposedUiFrame composed;
};

using ProductionPopupControlMeasurer = std::function<std::optional<ProductionPopupControlRects>(
    const LiveUiFrame &, const CityScreenSource &)>;

// Measures Civis' flat choices against the active BNW XML/Lua viewport and
// scroll buttons. Native Lua category placement remains unresolved.
std::optional<ProductionPopupControlRects> measure_civis_production_popup(
    const LiveUiFrame &frame, const CityScreenSource &source,
    std::int32_t width, std::int32_t height);

// Captures one live frame, obtains its laid-out controls from the presenter,
// and binds them to that frame's typed choices before composing input.
PresentedUiFrame present_live_ui_frame(LiveUiHost &host,
    const UiScreenCompositor &compositor, std::int32_t width,
    std::int32_t height, const ProductionPopupControlMeasurer &measure = {});

LiveUiInputResult click_composed_ui(LiveUiHost &host, const LiveUiFrame &frame,
    const ComposedUiFrame &composed, std::int32_t x, std::int32_t y);
// Complete pointer click: activation requires press and release on the same
// measured target. A focused modal screen consumes even an outside/mismatched
// click, so it cannot fall through to the world camera or another screen.
LiveUiInputResult click_composed_ui(LiveUiHost &host, const LiveUiFrame &frame,
    const ComposedUiFrame &composed, std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y);
LiveUiInputResult wheel_composed_ui(LiveUiHost &host, const LiveUiFrame &frame,
    const ComposedUiFrame &composed, std::int32_t x, std::int32_t y,
    std::int32_t wheel_delta);

// Presenter-owned edit focus. The selected XML source must expose
// SearchEditBox; its current screen-space rectangle comes from the composed
// frame. Win32 WM_CHAR supplies UTF-16 code units; one batch becomes one
// typed Civilopedia search command against the current immutable frame.
class CivilopediaSearchEditor {
  public:
    bool select(LiveUiHost &host, const LiveUiFrame &frame,
                const ComposedUiFrame &composed, std::int32_t press_x,
                std::int32_t press_y, std::int32_t release_x,
                std::int32_t release_y);
    UiCommandResult input(LiveUiHost &host, const LiveUiFrame &frame,
                          const ComposedUiFrame &composed,
                          std::span<const char16_t> characters);
    void blur() { focused_ = false; high_surrogate_ = 0; }
    bool focused() const { return focused_; }

  private:
    bool focused_ = false;
    char16_t high_surrogate_ = 0;
};

class PopupNameEditor {
  public:
    bool select(LiveUiHost &host, const LiveUiFrame &frame,
                const ComposedUiFrame &composed, std::int32_t press_x,
                std::int32_t press_y, std::int32_t release_x,
                std::int32_t release_y);
    UiCommandResult input(LiveUiHost &host, const LiveUiFrame &frame,
                          const ComposedUiFrame &composed,
                          std::span<const char16_t> characters);
    void blur() {
        focused_ = false;
        high_surrogate_ = 0;
        pending_submit_ = false;
        context_id_.clear();
        player_id_ = event_id_ = -1;
    }
    bool focused() const { return focused_; }
    bool pending_submit() const { return pending_submit_; }

  private:
    bool focused_ = false;
    char16_t high_surrogate_ = 0;
    bool pending_submit_ = false;
    std::string context_id_;
    std::int64_t player_id_ = -1, event_id_ = -1;
};

} // namespace civ5::ui
