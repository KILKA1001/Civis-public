#pragma once

#include "civ5/ui/bnw_city_production_backend.hpp"
#include "civ5/ui/bnw_notification_popup_owner.hpp"

namespace civ5::ui {

// Retains the production backend identity expected by the live compositor,
// while routing ActionInfoPanel notification clicks to the post-load owner.
// Missing activation services keep those hits disabled.
class BnwSelectedNotificationBackend final : public BnwCityProductionUiBackend {
  public:
    BnwSelectedNotificationBackend(runtime::LiveSession &session,
        const runtime::ProductionProgressRules &rules,
        runtime::BnwProductionOrderServices production_services,
        BnwProductionPreview production_preview,
        BnwNotificationPopupOwner &notification_owner,
        BnwNotificationActivationServices notification_services,
        const content::RulesDatabase *database = nullptr,
        content::InfoLocalizer localize = {});

    UiCommandResult availability(const UiGameplayCommand &command,
                                 const UiFrameSnapshot &frame) const override;
    UiCommandResult execute(const UiGameplayCommand &command,
                            const UiFrameSnapshot &frame) override;

  private:
    runtime::LiveSession *session_;
    BnwNotificationPopupOwner *notification_owner_;
    BnwNotificationActivationServices notification_services_;
};

} // namespace civ5::ui
