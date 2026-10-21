#include "bnw_tech_award_context_layout_fixture.hpp"
#include "civ5/ui/bnw_tech_award_context_owner.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace civ5::tests {
void verify_actual_tech_award_context_layout(const ui::BnwTechAwardContextOwner& owner){
 owner.require_current();const auto observed=owner.observation();
 const auto need=[](bool b){if(!b)throw std::runtime_error("Actual TechAward recursive layout verification failed");};
 need(observed.auto_size_complete&&bool(observed.client_layout));const auto& layout=*observed.client_layout;
 need(layout.viewport.drawable()&&!layout.nodes.empty()&&!layout.grids.empty());
 for(const auto& node:layout.nodes){
  for(float v:node.rect)need(std::isfinite(v));for(float v:node.bounds)need(std::isfinite(v));
  need(node.bounds[0]<=node.rect[0]&&node.bounds[1]<=node.rect[1]&&
       node.bounds[2]>=node.rect[0]+node.rect[2]&&node.bounds[3]>=node.rect[1]+node.rect[3]);
  // Actual recursively unioned descendants must fit their ancestor bounds,
  // including source-hidden/outside controls, rather than only visible ones.
  for(const auto& descendant:layout.nodes)if(descendant.path.starts_with(node.path+"/"))
   need(node.bounds[0]<=descendant.rect[0]&&node.bounds[1]<=descendant.rect[1]&&
        node.bounds[2]>=descendant.rect[0]+descendant.rect[2]&&node.bounds[3]>=descendant.rect[1]+descendant.rect[3]);
  if(node.kind=="Label")need(node.measured_label&&*node.measured_label<observed.measured_labels.size());
  if(!node.texture.empty())need(bool(node.selected_texture));
 }
 for(const auto& label:observed.measured_labels)for(const auto& line:label.layout.lines)for(const auto& glyph:line.styled_glyphs){
  if(!glyph.icon)continue;
  const auto& icon=*glyph.icon;need(icon.font&&icon.geometry&&icon.rgba_atlas&&!icon.font_sha256.empty()&&!icon.packed_sha256.empty()&&!icon.index_sha256.empty());
  need(icon.rgba_atlas->width==icon.geometry->atlas_width&&icon.rgba_atlas->height==icon.geometry->atlas_height&&
       icon.rgba_atlas->rgba.size()==std::uint64_t{icon.rgba_atlas->width}*icon.rgba_atlas->height*4&&
       glyph.glyph.code_point==icon.code_point&&bool(icon.font->glyph(icon.code_point)));
 }
 const auto grid=std::find_if(layout.nodes.begin(),layout.nodes.end(),[](const auto& n){return n.id=="OuterGrid";});
 need(grid!=layout.nodes.end()&&observed.measured_grid_width&&observed.measured_grid_height);
 need(grid->rect[2]==static_cast<float>(*observed.measured_grid_width)&&grid->rect[3]==static_cast<float>(*observed.measured_grid_height));
 for(const auto& graphic:layout.grids){
  need(bool(graphic.selected_texture));float area=0;
  for(std::size_t i=0;i<9;++i){const auto& rect=graphic.rects[i];const auto& uv=graphic.uvs[i];need(rect[2]>=0&&rect[3]>=0&&uv[0]>=0&&uv[1]>=0&&uv[0]+uv[2]<=1.000001f&&uv[1]+uv[3]<=1.000001f);area+=rect[2]*rect[3];}
  const auto node=std::find_if(layout.nodes.begin(),layout.nodes.end(),[&](const auto& n){return n.path==graphic.path;});
  need(node!=layout.nodes.end());need(std::abs(area-node->rect[2]*node->rect[3])<0.1f);
 }
 owner.require_current();
}
}
