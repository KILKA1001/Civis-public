#pragma once
#include "civ5/runtime/startup_loading.hpp"
#include "civ5/ui/frontend_menu.hpp"
#include "civ5/ui/glyphgen.hpp"

namespace civ5::ui {

enum class FrontendStartupPhase { loading, legal, menu, failed, cancelled };
struct FrontendStartupSkin {
    std::string background, logo, panel_style;
    std::string legal_body_key, rating_body_key, continue_key;
    std::string selected_legal_source;
    // Copy observations from the actual active XML/Lua catalog. A bootstrap
    // presenter can leave this empty until its minimum resource phase returns.
    static FrontendStartupSkin from_sources(const FrontendSourceCatalog &);
};
// Product-owned EN/RU captions for preparation work, separate from original
// legal/text rows. Feed these to the existing font helper before painting.
std::map<std::string, std::string, std::less<>> frontend_startup_captions(bool russian);
struct FrontendStartupTextBlock {
    std::vector<std::string> lines;
    std::int32_t line_height = 0;
};
using FrontendStartupTextLayout = std::function<FrontendStartupTextBlock(std::string_view, std::int32_t)>;
// Plain-notice adapter using the existing selected GlyphGen advances. No
// independent font metrics, renderer, glyph cache or native Label are created.
FrontendStartupTextBlock layout_frontend_startup_text(const GlyphGenGeometry &,
    std::string_view, std::int32_t width);

class FrontendStartupSession {
  public:
    FrontendStartupSession(std::vector<runtime::StartupLoadingStage>, FrontendLocalizer = {},
        runtime::StartupLoading::Observer = {}, bool legal_already_shown = false);
    FrontendStartupSession(const FrontendStartupSession &) = delete;
    FrontendStartupSession &operator=(const FrontendStartupSession &) = delete;
    runtime::StartupLoadingPhase advance();
    void set_skin(FrontendStartupSkin);
    void set_text_layout(FrontendStartupTextLayout);
    void scroll(std::int32_t rows);
    void request_exit();
    FrontendStartupPhase phase() const;
    runtime::StartupLoadingSnapshot snapshot() const { return loading_.snapshot(); }
    FrontendMenuPlan compose(std::int32_t width, std::int32_t height,
        std::optional<WorldViewPoint> pointer = {}) const;
    // Native host still validates that this plan/viewport was actually presented
    // and calls click only after DispatchMessage returns, like the menu route.
    UiCommandResult click(const FrontendMenuPlan &, std::int32_t down_x, std::int32_t down_y,
                          std::int32_t up_x, std::int32_t up_y);
    bool menu_ready() const { return phase() == FrontendStartupPhase::menu; }
  private:
    runtime::StartupLoading loading_;
    FrontendLocalizer localize_;
    FrontendStartupSkin skin_;
    FrontendStartupTextLayout text_layout_;
    std::size_t scroll_ = 0;
    mutable std::size_t maximum_scroll_ = 0;
    mutable bool composing_ = false;
    bool legal_shown_ = false, exit_requested_ = false;
    std::uint64_t revision_ = 0;
    std::string text(std::string_view) const;
    void require_mutation_allowed() const;
};
} // namespace civ5::ui
