#pragma once

#include "civ5/runtime/bnw_production_order.hpp"
#include "civ5/ui/city_screens.hpp"
#include <functional>

namespace civ5::ui {

// The game owner supplies a complete, observational BNW eligibility query.
// It must account for the queue clear performed by pushOrder(bPop=true).
// Without this query, the UI never presents a production choice as available.
using BnwProductionPreview = std::function<UiCommandResult(
    const runtime::LiveSessionState &, std::int64_t, std::int64_t,
    const runtime::BnwProductionChoice &)>;

// Session, rules and the optional activated database are borrowed from the
// live game owner. Runtime services, preview and localizer are copied; their
// captures must remain valid for this backend's life.
class BnwCityProductionUiBackend : public UiGameplayBackend {
  public:
    BnwCityProductionUiBackend(runtime::LiveSession &session,
        const runtime::ProductionProgressRules &rules,
        runtime::BnwProductionOrderServices services,
        BnwProductionPreview preview,
        const content::RulesDatabase *database = nullptr,
        content::InfoLocalizer localize = {});

    UiCommandResult availability(const UiGameplayCommand &command,
                                 const UiFrameSnapshot &frame) const override;
    UiCommandResult execute(const UiGameplayCommand &command,
                            const UiFrameSnapshot &frame) override;
    std::optional<ProductionPopupChoices> capture_choices(
        const UiFrameSnapshot &frame) const;

  private:
    runtime::LiveSession *session_;
    const runtime::ProductionProgressRules *rules_;
    runtime::BnwProductionOrderServices services_;
    BnwProductionPreview preview_;
    const content::RulesDatabase *database_;
    content::InfoLocalizer localize_;
};

} // namespace civ5::ui
