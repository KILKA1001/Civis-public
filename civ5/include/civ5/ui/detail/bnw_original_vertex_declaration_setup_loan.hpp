#pragma once
#include <cstdint>
#include <memory>
#include <span>

namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui::detail {
class BnwOriginalNonFontDeviceInitializer;
class BnwOriginalVertexDeclarationSetupLoan;
// Root/Graph alone issue this from a real device-backed invocation, including
// actual failed HRESULT/null output. This header supplies no result issuer.
class BnwOriginalVertexDeclarationDeviceResultLoan;

struct BnwOriginalVertexDeclarationElement final {
    std::uint16_t stream,offset;
    std::uint8_t type,method,usage,usage_index;
};
static_assert(sizeof(BnwOriginalVertexDeclarationElement)==8);

// Closed lexical operand for one actual positive Original declaration row.
// The slot is a vertex-format row, never a native resource ID or GPU name.
class BnwOriginalVertexDeclarationCreateLoan final {
    friend class BnwOriginalVertexDeclarationSetupLoan;
    friend class BnwOriginalNonFontDeviceInitializer; // Final callback-free physical checker only.
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwOriginalVertexDeclarationCreateLoan(std::shared_ptr<Impl>);
public:
    BnwOriginalVertexDeclarationCreateLoan(const BnwOriginalVertexDeclarationCreateLoan&)=delete;
    BnwOriginalVertexDeclarationCreateLoan&operator=(const BnwOriginalVertexDeclarationCreateLoan&)=delete;
    ~BnwOriginalVertexDeclarationCreateLoan();
    void require_physical_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const BnwOriginalVertexDeclarationCreateLoan>
    retain_owner_owned_without_callbacks()const;
    std::uint32_t slot_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::span<const BnwOriginalVertexDeclarationElement>
    elements_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
};

// The real ordered CPU actor plus retained authentic backend returns. The
// factory is private to Init; no caller DTO/count/ready flag can invoke it.
class BnwOriginalVertexDeclarationSetupLoan final {
    friend class BnwOriginalNonFontDeviceInitializer;
    friend class BnwOriginalVertexDeclarationCreateLoan;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwOriginalVertexDeclarationSetupLoan(std::shared_ptr<Impl>);
    static std::shared_ptr<const BnwOriginalVertexDeclarationSetupLoan>
    execute_manager8e1060_owned(const BnwOriginalNonFontDeviceInitializer&,
        const runtime::BnwWorldViewSourceOperationLoan&);
public:
    BnwOriginalVertexDeclarationSetupLoan(const BnwOriginalVertexDeclarationSetupLoan&)=delete;
    BnwOriginalVertexDeclarationSetupLoan&operator=(const BnwOriginalVertexDeclarationSetupLoan&)=delete;
    ~BnwOriginalVertexDeclarationSetupLoan();
    void require_physical_without_callbacks()const;
    void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    std::shared_ptr<const BnwOriginalVertexDeclarationSetupLoan>
    retain_owner_owned_without_callbacks()const;
    std::uint32_t stride_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::uint32_t slot)const;
    std::shared_ptr<const BnwOriginalVertexDeclarationDeviceResultLoan>
    declaration_result_owned(const runtime::BnwWorldViewSourceOperationLoan&,std::uint32_t slot)const;
};
}
