#pragma once
#include "civ5/ui/detail/bnw_original_vertex_declaration_setup_loan.hpp"
#include "civ5/graphics/detail/bnw_owned_gl_retirement_node.hpp"
#include <array>
#include <memory>
#include <vector>
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::ui::detail {
// An actual GL declaration invocation result. Internal GL locations are the
// declaration element ordinals, a Civis representation, not Original slots.
// Native semantic-to-selectedShader admission remains mandatory before Draw.
class BnwOriginalVertexDeclarationDeviceResultLoan final {
 friend class ::civ5::ui::UiScreenGlRenderer;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 explicit BnwOriginalVertexDeclarationDeviceResultLoan(std::shared_ptr<const Impl>);
 static std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan> seal_created_owned(
  std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>,
  std::shared_ptr<const BnwOriginalVertexDeclarationCreateLoan>,
  std::shared_ptr<const graphics::detail::BnwRendererContextPin>,
  graphics::detail::BnwOwnedGlRetirementNode::VertexDeclarationChannel::Owner,
  std::uint64_t,std::uint32_t,std::vector<BnwOriginalVertexDeclarationElement>,
  std::array<GLenum,2> actual_post,graphics::GlApi actual_physical_dispatch);
 void require_physical_without_callbacks()const;
 std::shared_ptr<const BnwOriginalNonFontDeviceInitializer>
 initializer_owner_owned_without_callbacks()const;
 std::shared_ptr<const graphics::detail::BnwRendererContextPin>
 context_pin_owned_without_callbacks()const;
 graphics::GlApi physical_dispatch_without_callbacks()const;
 std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>
 retain_owner_owned_without_callbacks()const;
public:
 // Root is a reader, not a friend of the facade constructor/factory. It supplies
 // genuine physical schema from private Create/Setup fields AFTER fresh FULL.
 class RootAccess final {
  friend class BnwOriginalNonFontDeviceInitializer;
  RootAccess()=delete;
  static void require_creation_without_callbacks(const BnwOriginalVertexDeclarationDeviceResultLoan&,
   const BnwOriginalNonFontDeviceInitializer&,std::uint64_t,const BnwOriginalVertexDeclarationCreateLoan&,
   std::uint32_t,std::span<const BnwOriginalVertexDeclarationElement>);
  static void require_current_without_callbacks(const BnwOriginalVertexDeclarationDeviceResultLoan&,
   const BnwOriginalNonFontDeviceInitializer&,std::uint64_t,std::uint32_t,
   std::span<const BnwOriginalVertexDeclarationElement>);
 };
 ~BnwOriginalVertexDeclarationDeviceResultLoan();
 BnwOriginalVertexDeclarationDeviceResultLoan(const BnwOriginalVertexDeclarationDeviceResultLoan&)=delete;
 BnwOriginalVertexDeclarationDeviceResultLoan&operator=(const BnwOriginalVertexDeclarationDeviceResultLoan&)=delete;
};
}
