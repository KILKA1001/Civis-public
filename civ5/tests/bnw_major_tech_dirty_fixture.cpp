#include "bnw_major_tech_dirty_fixture.hpp"
#include "civ5/runtime/bnw_major_tech_transport_consumer.hpp"
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <stdexcept>
namespace civ5::tests {
void verify_major_tech_dirty_fixture(const ui::SelectedBnwPreviewHost& host,
 const ui::LiveUiHost& ui,const ui::LiveUiFrame& before,const ui::LiveUiFrame& after,
 const runtime::BnwMajorTeamHasTechPending& core,
 std::span<const std::shared_ptr<const runtime::BnwMajorTechTransportConsumption>> receipts){
 const auto need=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 core.require_stored_current(host);
 need(before.owner==&ui&&after.owner==&ui&&after.sequence>before.sequence,
  "Actual dirty begin_frame did not invalidate old selected UI sequence");
 const auto* flags=after.snapshot.find<ui::BnwMajorTechDirtySnapshot>();
 const auto* stamp=after.snapshot.find<ui::BnwClientFrameStamp>();
 need(flags&&stamp&&flags->misc_buttons&&flags->selection_buttons&&flags->research_buttons&&
  flags->revision==3&&flags->deliveries.size()==3&&receipts.size()==3,
  "Actual dirty frame lacks the genuine ordered three setter effects");
 need(stamp->identity.ui_source_revision==core.after_revision()&&
  host.session().state().ui_source_revision==core.after_revision(),
  "Actual dirty frame belongs to another earned world revision");
 const auto events=core.technology_transport_events();
 const std::array<const char*,3> names{"MiscButtons_DIRTY_BIT","SelectionButtons_DIRTY_BIT","ResearchButtons_DIRTY_BIT"};
 need(events.size()>=3,"Actual core has not reached all three native dirty setters");
 for(std::size_t n=0;n<3;++n){
  need(receipts[n]!=nullptr,"Actual dirty consumption receipt unavailable");
  receipts[n]->require_same_request(core);
  const auto& observed=flags->deliveries[n];const auto& native=events[n];
  need(native.kind==runtime::BnwMajorTechUiTransportKind::dirty&&native.native_name==names[n]&&
   observed.native_name==names[n]&&observed.occurrence==native.occurrence&&
   observed.generation==native.generation&&observed.team==native.team&&
   observed.technology==native.technology&&observed.player==native.player&&observed.turn==native.turn&&
   observed.world_revision==core.after_revision()&&receipts[n]->occurrence()==native.occurrence&&
   receipts[n]->kind()==native.kind,"Dirty effect/order/source differs from genuine reached queue");
 }
 core.require_stored_current(host);
 need(!host.session().readiness().can_continue(),"Partial tech dirty consumer cannot grant full continuation");
}
}
