#pragma once
#include <array>
#include <cstdint>
#include <memory>

namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui::detail {
class BnwOriginalNonFontDeviceInitializer;

// The actual first manager+20DA8 constructor call, not a default sampler table.
// This result proves the copied fields of8F6D20 only. It contains no applied
// sampler, native resource ID, GPU name, sRGB decision, Draw or whole-init grant.
class BnwOriginalResourceSamplerDescriptorLoan final {
    friend class BnwOriginalNonFontDeviceInitializer;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwOriginalResourceSamplerDescriptorLoan(std::shared_ptr<Impl>);
    static std::shared_ptr<const BnwOriginalResourceSamplerDescriptorLoan>
    create_manager20da8_8e3bd2_owned(const BnwOriginalNonFontDeviceInitializer&,
        const runtime::BnwWorldViewSourceOperationLoan&);
public:
    // Raw stored words. The qualified8F6D60 field meanings are metadata only;
    // this API cannot issue an applied state. Native padding+1D..1F was not
    // written and is not manufactured.
    struct ConstructorFieldSnapshot final {
        const std::array<std::uint32_t,7> dwords_00_through_18;
        const std::uint8_t byte_1c;
    };

    // A SAME-descriptor retained data view for the proved resource0 storage
    // format. UNORM8 transport does not specify gamma/sampler/filtering state.
    class RawBgra8UnormStoragePolicy final {
        friend class BnwOriginalResourceSamplerDescriptorLoan;
        const std::shared_ptr<const BnwOriginalResourceSamplerDescriptorLoan> descriptor_;
        explicit RawBgra8UnormStoragePolicy(
            std::shared_ptr<const BnwOriginalResourceSamplerDescriptorLoan>);
    public:
        void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
        std::uint32_t original_d3d_storage_format_owned(
            const runtime::BnwWorldViewSourceOperationLoan&)const;
        std::array<std::uint8_t,4> component_bits_in_native_bgra_order_owned(
            const runtime::BnwWorldViewSourceOperationLoan&)const;
        // There is intentionally no sampler/draw/sRGB-ready accessor.
    };

    BnwOriginalResourceSamplerDescriptorLoan(const BnwOriginalResourceSamplerDescriptorLoan&)=delete;
    BnwOriginalResourceSamplerDescriptorLoan&operator=(const BnwOriginalResourceSamplerDescriptorLoan&)=delete;
    ~BnwOriginalResourceSamplerDescriptorLoan();
    void require_physical_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const BnwOriginalResourceSamplerDescriptorLoan>
    retain_owner_owned_without_callbacks()const;
    ConstructorFieldSnapshot constructor_fields_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
    RawBgra8UnormStoragePolicy raw_bgra8_unorm_storage_policy_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const;
};
}
