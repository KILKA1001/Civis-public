#pragma once
#include "civ5/ui/client_window_input.hpp"
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <utility>
namespace civ5::content {struct XmlNode;}
namespace civ5::assets {struct UiTextureBinding;}
namespace civ5::ui {
struct BnwTechAwardContextObservation;
struct BnwTechAwardLayoutNode {
 std::string path,id,kind,style,texture,color,color_set;
 std::array<std::string,3> color_groups;
 std::array<std::int32_t,2> texture_offset{};
 std::shared_ptr<const assets::UiTextureBinding> selected_texture;
 std::array<float,4> rect{},bounds{};
 bool hidden=false,mouse_over_only=false,mouse_hide_only=false;
 // Actual expanded wrapper/attached parent, sealed by the private Context.
 std::string mouse_wrapper_path,mouse_parent_path;
 bool mouse_input_unsupported=false;
 std::string alpha_start,alpha_end,cycle,speed;
 // Exact selected expanded attributes; arithmetic DTO, not phase authority.
 std::vector<std::pair<std::string,std::string>> animation_profile;
 std::optional<std::size_t> measured_label;
};
struct BnwTechAwardGridSlices {
 std::string path,texture;
 std::shared_ptr<const assets::UiTextureBinding> selected_texture;
 // C, UC, LC, L, R, UL, UR, LL, LR. Rect and normalized selected DDS UV.
 std::array<std::array<float,4>,9> rects{},uvs{};
};
struct BnwTechAwardContextLayout {
 ClientViewport viewport;
 std::vector<BnwTechAwardLayoutNode> nodes;
 std::vector<BnwTechAwardGridSlices> grids;
 // Actual Civis geometry data, never Original GUIglobal or GPU presentation.
 static constexpr bool original_gui_global=false,gpu_presented=false;
};
// Arithmetic only. Its inputs grant no context/currentness/readiness authority.
BnwTechAwardContextLayout project_bnw_tech_award_context_layout(
 const content::XmlNode& context,const content::XmlNode& styles,
 const BnwTechAwardContextObservation&,ClientViewport);
BnwTechAwardGridSlices project_bnw_tech_award_grid_slices(
 const content::XmlNode& skin,const BnwTechAwardLayoutNode&,
 std::uint32_t texture_width,std::uint32_t texture_height);
}
