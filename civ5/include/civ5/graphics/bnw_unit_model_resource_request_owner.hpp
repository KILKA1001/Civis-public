#pragma once
#include "civ5/graphics/bnw_unit_fx_resource_chain.hpp"
namespace civ5::graphics {
// Prepared source arguments, not proof that the native manager exists or that
// its cache returned a resource. Scale is retained from the sealed SQL quote.
struct BnwUnitModelConditionalRequest {
    std::string model;
    std::uint32_t scale_operand_bits=0;
    std::uint32_t manager_load_flags=2;
    bool manager_modifier_required=false;
    std::uint32_t modifier_scale_bits=0,modifier_flags=0;
    std::uint32_t modifier_filter=0,modifier_filter_count=0;
};
class BnwUnitModelResourceRequestOwner {
 public:
    static std::shared_ptr<const BnwUnitModelResourceRequestOwner> create(
        std::shared_ptr<const BnwUnitFxResourceChain> same_fx_chain);
    const BnwUnitModelConditionalRequest& conditional_request() const;
    const BnwUnitFxResourceChain& retained_fx_chain() const;
    void validate_current() const;
    // No caller-supplied cache, manager-present switch or successful skip.
    // The mandatory native cache initialization/lookup owner is not available.
    [[noreturn]] void require_current_native_resource() const;
 private:
    struct Impl;std::shared_ptr<const Impl> impl_;
    explicit BnwUnitModelResourceRequestOwner(std::shared_ptr<const Impl>);
};
} // namespace civ5::graphics
