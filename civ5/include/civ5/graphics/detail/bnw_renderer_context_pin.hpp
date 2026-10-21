#pragma once
#include "civ5/graphics/gl_context.hpp"
#include <thread>
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::ui::detail {class BnwOriginalNonFontDeviceInitializer;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;class GlBnwFontShader;}
namespace civ5::graphics::detail {
class BnwLabelSystemFontAtlasMetadata;
class BnwOwnedGlRetirementNode;
// Closed Window-issued storage pin. Physical retention is not outside authority.
class BnwRendererContextPin final {
 friend class ::civ5::graphics::BnwOwnedCameraRendererWindowLease;
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class ::civ5::graphics::GlBnwFontShader;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::ui::detail::BnwOriginalNonFontDeviceInitializer;
 friend class BnwLabelSystemFontAtlasMetadata;
 friend class BnwOwnedGlRetirementNode;
 const std::shared_ptr<GlContext> context_;
 const std::shared_ptr<GlContextState> physical_;
 const std::weak_ptr<GlContext> canonical_;
 const std::thread::id thread_;
 const GlContextNativeIdentity identity_;
 class Capture final {
  friend class BnwRendererContextPin;
  friend class ::civ5::graphics::BnwOwnedCameraRendererWindowLease;
  std::shared_ptr<GlContext> context_;
  const std::shared_ptr<GlContextState> physical_;
  const std::weak_ptr<GlContext> canonical_;
  const std::thread::id thread_;
  GlContextNativeIdentity identity_;
  bool engaged_=false;
  Capture(std::weak_ptr<GlContext>,std::shared_ptr<GlContextState>);
  ~Capture();
  Capture(const Capture&)=delete;
 };
 // Only the closed retirement node may obtain physical cleanup dispatch.
 // Existing renderer/Metadata/Atlas/Shader friends cannot call this nested
 // private static method. It does not renew producer or Paint authority.
 class CleanupAccess final {
  friend class BnwOwnedGlRetirementNode;
  CleanupAccess()=delete;
  static const GlApi* api_if_current_without_switch(const BnwRendererContextPin&)noexcept;
 };
 // Normal producer use keeps original facade/outside ownership mandatory.
 // This is separate from closed retirement dispatch and issues no Source,
 // Paint, target, material or GPU effect authority. Caller retains SAME Pin
 // throughout every later query/use of the returned loaded dispatch pointer.
 class NormalAccess final {
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::graphics::GlBnwFontShader;
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  NormalAccess()=delete;
  static const GlApi* api_current_without_switch_owned(
      std::shared_ptr<const BnwRendererContextPin>);
 };
 explicit BnwRendererContextPin(Capture&);
 void first_without_callbacks()const;
 GlContext& context_reference_without_callbacks()const;
 public:
 ~BnwRendererContextPin();
 BnwRendererContextPin(const BnwRendererContextPin&)=delete;
 BnwRendererContextPin&operator=(const BnwRendererContextPin&)=delete;
};
}
