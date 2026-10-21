#pragma once
#include "civ5/runtime/bnw_research_choice_commit.hpp"
#include "civ5/ui/progression_screens.hpp"
#include <memory>

namespace civ5::ui {
class SelectedBnwPreviewHost;
class BnwClientSceneFrame;
class LiveUiHost;
struct LiveUiFrame;
enum class BnwAlphaResearchUiPhase { idle, executing, pending_refresh, complete, uncertain };

// Separate first-alpha backend for the existing TechTree command IDs. Neither
// the legacy BnwLiveProgressionBackend nor its generic transaction is replaced.
// Only a genuine SourceOwner binds this backend to its registered actual scene.
class BnwAlphaProgressionBackend final:public UiGameplayBackend {
  public:
    ~BnwAlphaProgressionBackend();
    BnwAlphaProgressionBackend(const BnwAlphaProgressionBackend&)=delete;
    BnwAlphaProgressionBackend& operator=(const BnwAlphaProgressionBackend&)=delete;
    UiCommandResult availability(const UiGameplayCommand&,const UiFrameSnapshot&)const override;
    UiCommandResult execute(const UiGameplayCommand&,const UiFrameSnapshot&)override;
    // AFTER old dispatch returns, genuine caller performs scene.begin and
    // LiveUiHost.begin_frame. This checks the actual registered fresh publication;
    // It additionally requires the compositor-issued actual GUI publication and
    // SourceOwner finalization. A copied stamp/sequence cannot produce success.
    UiCommandResult complete_after_refresh(BnwClientSceneFrame&,LiveUiHost&,const LiveUiFrame&);
    BnwAlphaResearchUiPhase phase()const;
    std::shared_ptr<const runtime::BnwResearchChoiceCommit> last_commit()const;
  private:
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwAlphaProgressionBackend(std::unique_ptr<Impl>);
    // SourceOwner must provide its actual canonical factory-issued self, actual
    // selected Host and registered scene. Not a public callback/issuer adapter.
    static std::shared_ptr<BnwAlphaProgressionBackend> bind(
        std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>,
        std::shared_ptr<const SelectedBnwPreviewHost>,std::shared_ptr<BnwClientSceneFrame>);
    std::shared_ptr<const BnwAlphaProgressionBackend> retain_self()const;
};
}
