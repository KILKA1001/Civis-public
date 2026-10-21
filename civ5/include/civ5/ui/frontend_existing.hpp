#pragma once
#include "civ5/ui/frontend_menu.hpp"
#include "civ5/ui/system_screens.hpp"

namespace civ5::ui {
// Portable source/data presentation only. A null player/turn frame does not
// issue native Civilopedia, stock Lua or gameplay readiness.
class FrontendExistingScreens {
  public:
    FrontendExistingScreens(const assets::ResourceMountTable &, FrontendLocalizer);
    std::optional<FrontendMenuPlan> present(const FrontendScreen &, const FrontendDraft &,
                                          std::int32_t width, std::int32_t height);
    UiCommandResult dispatch(const FrontendScreen &, const FrontendDraft &, const UiCommand &);
    std::optional<UiCommand> character(std::uint32_t utf16_unit);
    std::optional<UiCommand> key_down(std::uint32_t virtual_key);
    std::optional<UiCommand> wheel(std::int32_t delta) const;
    bool text_focused() const {
        return drawable_pedia_ && search_focused_ && pedia_controller_ &&
            pedia_controller_->focused_screen() == "CivilopediaScreen";
    }
    void cancel_text_input() { high_surrogate_.reset(); }
    std::vector<std::string> font_texts() const;
  private:
    FrontendLocalizer localize_;
    std::optional<SystemScreenSource> pedia_source_;
    SystemScreenFrame pedia_data_;
    SystemMenuSession pedia_session_;
    SystemGameplayBackend pedia_backend_;
    std::optional<UiScreenController> pedia_controller_;
    std::optional<UiFrameSnapshot> pedia_frame_;
    std::uint64_t frame_ = 0;
    bool search_focused_ = false;
    bool drawable_pedia_ = false;
    std::string search_input_;
    std::optional<std::uint32_t> high_surrogate_;
    std::string blocker_;
    UiFrameSnapshot snapshot(std::int32_t width, std::int32_t height);
};
} // namespace civ5::ui
