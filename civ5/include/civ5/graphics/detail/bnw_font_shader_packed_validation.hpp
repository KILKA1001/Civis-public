#pragma once
#include "civ5/graphics/gl_bnw_font_shader.hpp"
#include <cmath>
#include <stdexcept>
namespace civ5::graphics::detail {
// Private shared scalar-validation projection. GPU attributes NEVER use this
// float projection; the source packed bytes remain the uploaded vertex data.
inline BnwFontShaderBatch packed_validation_view(const BnwFontShaderPackedBatch&packed){
 BnwFontShaderBatch result;result.retained_pair=packed.retained_pair;
 result.controls=packed.controls;result.current=packed.current;
 result.caller_declared_diagnostic_colors=packed.caller_declared_diagnostic_colors;
 for(const auto&layer:packed.layers){BnwFontShaderLayer l;l.slot=layer.slot;
  for(const auto&q:layer.quads){BnwFontShaderQuad v;v.uploaded_slot=q.uploaded_slot;
   for(unsigned n=0;n<4;++n){const auto&s=q.vertices[n];
    if(!std::isfinite(s.x)||!std::isfinite(s.y)||!std::isfinite(s.z)||!std::isfinite(s.u)||!std::isfinite(s.v))
     throw std::runtime_error("Nonfinite packed source XYZ/UV input");
    v.vertices[n]={s.x,s.y,s.u,s.v,{}};
    for(unsigned c=0;c<4;++c)v.vertices[n].diagnostic_rgba[c]=float((s.rgba>>(8*c))&255u)/255.f;
   }l.quads.push_back(std::move(v));
  }result.layers.push_back(std::move(l));
 }return result;
}
} // namespace civ5::graphics::detail
