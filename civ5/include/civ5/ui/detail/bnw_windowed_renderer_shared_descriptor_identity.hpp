#pragma once
#include <memory>
namespace civ5::graphics {class GlBnwFontShader;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwWindowedRendererMatrixDataLoan;
class BnwNormalLabelMaterialLoan;
// Canonical GUI-owned PORT constant resource identity. No Original pool ID,
// GLuint, frame-data completeness or raw240-byte initialization is certified.
class BnwWindowedRendererSharedDescriptorIdentity final {
    friend class NotificationContextGuiPort;
    friend class BnwWindowedRendererMatrixDataLoan;
    friend class BnwNormalLabelMaterialLoan;
    friend class ::civ5::graphics::GlBnwFontShader;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwWindowedRendererSharedDescriptorIdentity(std::shared_ptr<Impl>);
    // These checks only inspect captured physical ownership and lifetime.
    // Matrix/Source observation is performed separately before capture.
    void require_current_without_callbacks()const;
    void require_same_owner_without_callbacks(const BnwWindowedRendererSharedDescriptorIdentity&)const;
public:
    BnwWindowedRendererSharedDescriptorIdentity(const BnwWindowedRendererSharedDescriptorIdentity&)=default;
    ~BnwWindowedRendererSharedDescriptorIdentity()=default;
};
}
