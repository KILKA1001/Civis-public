#pragma once
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/bnw_tech_award_context_owner.hpp"
namespace civ5::ui {
// Published only from an actual current LiveUiHost begin_frame after genuine
// stock handler completion and SAME stored world/source checks. Data itself
// grants no renderer/native input or Original GUIglobal authority.
struct BnwTechAwardContextSnapshot {
 BnwClientFrameStamp publication;
 std::uint64_t occurrence=0,generation=0;
 BnwTechAwardContextObservation context;
 std::shared_ptr<const BnwTechAwardAnimationWitness> animation;
 static constexpr bool original_gui_global_dimensions_proven=false;
 static constexpr bool gpu_presented=false;
};
}
