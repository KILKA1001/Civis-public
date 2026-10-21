#pragma once
#include <memory>
#include <stdexcept>
#include <utility>
#include "civ5/graphics/detail/bnw_owned_gl_retirement_node.hpp"
#include "civ5/graphics/gl_context.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class BnwOriginalGeneratedResourceImageLoan;
class BnwOriginalNonFontDeviceInitializer;
class BnwNormalNonFontResourceLoan;
}
namespace civ5::graphics {class GlBnwNonFontPipeline;}
namespace civ5::graphics::detail {
class BnwRendererContextPin;
// Completed raw UNORM storage only. The actual Root initializer owns admission.
// No resource ID, material, sampler, Draw or completed-init capability.
class BnwNormalResourceGpuImageLoan final {
 friend class ::civ5::ui::UiScreenGlRenderer;
 // Consumer access is confined to ReaderAccess; Ui alone can name the ctor.
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 explicit BnwNormalResourceGpuImageLoan(std::shared_ptr<const Impl>);
 void require_physical_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_same_context_without_callbacks(const BnwRendererContextPin&)const;
 std::shared_ptr<const BnwNormalResourceGpuImageLoan>
 retain_owner_owned_without_callbacks()const;
 std::shared_ptr<const ::civ5::ui::detail::BnwOriginalGeneratedResourceImageLoan>
 original_cpu_image_owned_without_callbacks()const;
 GLuint texture_name_without_callbacks()const;
 static std::shared_ptr<const BnwNormalResourceGpuImageLoan> seal_uploaded_owned(
   std::shared_ptr<const ::civ5::ui::detail::BnwOriginalNonFontDeviceInitializer>,
   std::shared_ptr<const ::civ5::ui::detail::BnwOriginalGeneratedResourceImageLoan>,
   std::shared_ptr<const BnwRendererContextPin>,
   BnwOwnedGlRetirementNode::ResourceImageChannel::Owner,std::uint64_t,
   const runtime::BnwWorldViewSourceOperationLoan&);
 void require_same_inputs_without_callbacks(
   const std::shared_ptr<const ::civ5::ui::detail::BnwOriginalNonFontDeviceInitializer>&,
   const std::shared_ptr<const ::civ5::ui::detail::BnwOriginalGeneratedResourceImageLoan>&,
   const std::shared_ptr<const BnwRendererContextPin>&,std::uint64_t)const;
public:
 // Reader type is visible; only the two real consumers can call its private
 // checked methods. They are not friends of the enclosing loan or its ctor.
 class ReaderAccess final {
  friend class ::civ5::ui::detail::BnwNormalNonFontResourceLoan;
  friend class ::civ5::graphics::GlBnwNonFontPipeline;
  ReaderAccess()=delete;
  static std::shared_ptr<const BnwNormalResourceGpuImageLoan> checked(
    std::shared_ptr<const BnwNormalResourceGpuImageLoan> offered){
   if(!offered)throw std::runtime_error("Root GPU image reader offered no owner");
   const auto original=offered->retain_owner_owned_without_callbacks();
   if(original.get()!=offered.get()||original.owner_before(offered)||offered.owner_before(original))
    throw std::runtime_error("Root GPU image reader canonical shared owner differs");
   return original;
  }
  static void require_physical_without_callbacks(
    std::shared_ptr<const BnwNormalResourceGpuImageLoan> offered){
   const auto original=checked(std::move(offered));original->require_physical_without_callbacks();
  }
  static void require_source_owned(std::shared_ptr<const BnwNormalResourceGpuImageLoan> offered,
    const runtime::BnwWorldViewSourceOperationLoan& operation){
   const auto original=checked(std::move(offered));original->require_source_owned(operation);
   original->require_physical_without_callbacks();
  }
  static void require_same_context_without_callbacks(
    std::shared_ptr<const BnwNormalResourceGpuImageLoan> offered,const BnwRendererContextPin& pin){
   const auto original=checked(std::move(offered));original->require_same_context_without_callbacks(pin);
  }
  static std::shared_ptr<const BnwNormalResourceGpuImageLoan> retain_owner_owned_without_callbacks(
    std::shared_ptr<const BnwNormalResourceGpuImageLoan> offered){return checked(std::move(offered));}
  static std::shared_ptr<const ::civ5::ui::detail::BnwOriginalGeneratedResourceImageLoan>
    original_cpu_image_owned_without_callbacks(std::shared_ptr<const BnwNormalResourceGpuImageLoan> offered){
   const auto original=checked(std::move(offered));return original->original_cpu_image_owned_without_callbacks();
  }
  static GLuint texture_name_without_callbacks(std::shared_ptr<const BnwNormalResourceGpuImageLoan> offered){
   const auto original=checked(std::move(offered));return original->texture_name_without_callbacks();
  }
 };
 ~BnwNormalResourceGpuImageLoan();
 BnwNormalResourceGpuImageLoan(const BnwNormalResourceGpuImageLoan&)=delete;
 BnwNormalResourceGpuImageLoan&operator=(const BnwNormalResourceGpuImageLoan&)=delete;
};
}
