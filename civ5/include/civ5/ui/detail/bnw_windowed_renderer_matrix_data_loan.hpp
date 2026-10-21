#pragma once
#include <array>
#include "civ5/ui/detail/bnw_windowed_renderer_shared_descriptor_identity.hpp"
#include <cstdint>
#include <memory>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwFontShader;}
namespace civ5::ui {class BnwWorldViewControlRenderPublication;class BnwWorldViewControlTree;class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwNormalLabelMaterialLoan;
class BnwNormalGuiBatchPrefix;
class BnwNormalRootBatchHistory;
// Closed ported windowed-manager DATA. It is not an Original matrix pointer,
// paint-index, Alpha/AllowColor/ancestor/clip/default or draw/program authority.
class BnwWindowedRendererMatrixDataLoan final {
    friend class NotificationContextGuiPort;
    friend class BnwNormalLabelMaterialLoan;
    friend class BnwNormalGuiBatchPrefix;
    friend class BnwNormalRootBatchHistory;
    friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class ::civ5::ui::UiScreenGlRenderer;
    struct Impl;const std::shared_ptr<Impl> impl_;
    explicit BnwWindowedRendererMatrixDataLoan(std::shared_ptr<Impl>);
public:
    // Shader-readable lexical DATA view; the original facade still retires it.
    class ShaderAccess final {
        friend class BnwWindowedRendererMatrixDataLoan;
        friend class BnwNormalLabelMaterialLoan;
        friend class ::civ5::graphics::GlBnwFontShader;
        const std::shared_ptr<Impl> impl_;
        explicit ShaderAccess(std::shared_ptr<Impl>);
        void require_current_without_callbacks()const;
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        std::array<std::uint32_t,16> matrix_words_without_callbacks()const;
        std::array<std::uint32_t,2> arithmetic_words_without_callbacks()const;
        std::array<std::int32_t,2> dimensions_without_callbacks()const;
        std::array<std::uint32_t,2> numeric_controls_without_callbacks()const;
        std::array<std::uint32_t,2> numeric_return_controls_without_callbacks()const;
        std::uint64_t manager_revision_without_callbacks()const;
        BnwWindowedRendererSharedDescriptorIdentity shared_descriptor_identity_without_callbacks()const;
    public:
        ~ShaderAccess()=default;
        ShaderAccess(const ShaderAccess&)=delete;
        ShaderAccess&operator=(const ShaderAccess&)=delete;
        ShaderAccess(ShaderAccess&&)=delete;
        ShaderAccess&operator=(ShaderAccess&&)=delete;
    };
private:
    void require_current_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::array<std::uint32_t,16> matrix_words_without_callbacks()const;
    std::array<std::uint32_t,2> arithmetic_words_without_callbacks()const;
    std::array<std::int32_t,2> dimensions_without_callbacks()const;
    std::array<std::uint32_t,2> numeric_controls_without_callbacks()const;
    std::array<std::uint32_t,2> numeric_return_controls_without_callbacks()const;
    std::uint64_t manager_revision_without_callbacks()const;
    ShaderAccess capture_shader_access_without_callbacks()const;
public:
    ~BnwWindowedRendererMatrixDataLoan();
    BnwWindowedRendererMatrixDataLoan(const BnwWindowedRendererMatrixDataLoan&)=delete;
    BnwWindowedRendererMatrixDataLoan&operator=(const BnwWindowedRendererMatrixDataLoan&)=delete;
};
}
