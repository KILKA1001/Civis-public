#include "civ5/graphics/bnw_unit_model_resource_request_owner.hpp"
#include "bnw_unit_model_source_request.hpp"
#include <cmath>
namespace civ5::graphics {
namespace {void need(bool ok,const char* reason){if(!ok)throw std::invalid_argument(reason);}}
struct BnwUnitModelResourceRequestOwner::Impl {
    std::shared_ptr<const BnwUnitFxResourceChain> fx;
    BnwUnitModelConditionalRequest request;
    explicit Impl(std::shared_ptr<const BnwUnitFxResourceChain> source):fx(std::move(source)) {
        fx->validate_current();const auto& quote=fx->compiled_scale_quote();
        const auto scale=quote.compiled_rules().scale;
        need(std::isfinite(scale)&&scale>0,"Model source compiled member scale invalid");
        need(quote.model()==fx->definition().request,"Model source SAME compiled row/FX path differs");
        const auto fields=model_request_detail::prepare(std::bit_cast<std::uint32_t>(scale));
        request={quote.model(),fields.scale_bits,fields.load_flags,fields.modifier_required,
                 fields.modifier_scale_bits,fields.flags,fields.filter,fields.filter_count};
        // Preparing arguments does not consume scale into a resource. Placement
        // may use scale1 only after the missing resource conversion is owned.
        fx->validate_current();
    }
};
BnwUnitModelResourceRequestOwner::BnwUnitModelResourceRequestOwner(std::shared_ptr<const Impl> i):impl_(std::move(i)){}
std::shared_ptr<const BnwUnitModelResourceRequestOwner> BnwUnitModelResourceRequestOwner::create(std::shared_ptr<const BnwUnitFxResourceChain> fx) {
    need(bool(fx),"Model source SAME FXS chain absent");
    return std::shared_ptr<const BnwUnitModelResourceRequestOwner>(new BnwUnitModelResourceRequestOwner(std::make_shared<Impl>(std::move(fx))));
}
void BnwUnitModelResourceRequestOwner::validate_current() const {
    need(bool(impl_),"Model source request moved owner");impl_->fx->validate_current();
    const auto& quote=impl_->fx->compiled_scale_quote();
    need(impl_->request.model==quote.model()&&impl_->request.scale_operand_bits==std::bit_cast<std::uint32_t>(quote.compiled_rules().scale),"Model source retained member operand changed");
}
const BnwUnitModelConditionalRequest& BnwUnitModelResourceRequestOwner::conditional_request() const {validate_current();return impl_->request;}
const BnwUnitFxResourceChain& BnwUnitModelResourceRequestOwner::retained_fx_chain() const {validate_current();return *impl_->fx;}
void BnwUnitModelResourceRequestOwner::require_current_native_resource() const {
    validate_current();throw std::invalid_argument("Model source native manager/cache initialization and current lookup unavailable; resource scale not consumed");
}
} // namespace civ5::graphics
