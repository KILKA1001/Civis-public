#pragma once
#include "civ5/graphics/gl_context.hpp"
#include "civ5/ui/detail/bnw_original_vertex_declaration_setup_loan.hpp"
#include <cstdint>
#include <span>
#include <vector>
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::graphics::detail {
// Numeric conversion only: semantic usage/index is retained, no GL location
// is assigned. Genuine CreateLoan and current device remain caller obligations.
class BnwOriginalVertexDeclarationGlSchema final {
 friend class ::civ5::ui::UiScreenGlRenderer;
 BnwOriginalVertexDeclarationGlSchema()=delete;
 struct Attribute final {
  GLint components;
  GLenum scalar_type;
  GLboolean normalized;
  GLuint relative_offset;
  std::uint8_t native_type,usage,usage_index;
  std::uint32_t storage_bytes;
 };
 struct Schema final {std::vector<Attribute> attributes;std::uint32_t stride;};
 static Schema project(std::span<const ::civ5::ui::detail::BnwOriginalVertexDeclarationElement>);
};
}
