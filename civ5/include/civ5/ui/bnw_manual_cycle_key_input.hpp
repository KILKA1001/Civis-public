#pragma once
#include "civ5/runtime/bnw_unit_cycle.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include <cstdint>
#include <optional>
namespace civ5::ui {
// Raw key intent only. No frame, selected owner or completion authority.
// Native Win32 W and OEM slash bindings; conflicting stock controls remain
// outside this explicitly scoped manual client input route.
class BnwManualCycleKeyInput final {
 public:
 // Check the SAME actual receiver before and after reading its focus stack.
 // A focused nonmodal screen also owns keyboard controls. This bool issues
 // no frame authority; the dispatcher still checks the retained exact frame.
 static bool world_frame(const BnwClientSceneFrame& scene,const LiveUiHost& host,
     const LiveUiFrame& ui) {
     if(!scene.accepts(host,ui))return false;
     const bool focused=host.screens().focused_screen().has_value();
     return !focused&&scene.accepts(host,ui);
 }
 // The selected client reserves W for manual cycle. Arrow-up remains camera
 // input; the existing diagnostic camera retains its W binding.
 static bool camera_up(bool selected_client,bool w,bool up)noexcept {
     return up||(!selected_client&&w);
 }
 void cancel()noexcept {all_down_=workers_down_=false;}
 std::optional<runtime::BnwUnitCycleFilter> key(std::uint32_t code,bool down,
     bool native_previous_down,bool world_input,bool control,bool alt,bool shift)noexcept {
     bool* held=code==0x57?&all_down_:code==0xbf?&workers_down_:nullptr;
     if(!held)return {};
     if(!down){*held=false;return {};}
     const bool previous=*held;*held=true;
     if(previous||native_previous_down||!world_input||control||alt||shift)return {};
     return code==0x57?runtime::BnwUnitCycleFilter::all:runtime::BnwUnitCycleFilter::workers;
 }
 private:
 bool all_down_=false,workers_down_=false;
};
} // namespace civ5::ui
