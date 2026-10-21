#include "civ5/graphics/detail/bnw_font_texture_observation.hpp"
#include "civ5/ui/detail/bnw_font_texture_request.hpp"
#include "civ5/runtime/detail/bnw_font_resource_loan.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "../../validation/ui_screen_gl.hpp"
#include <atomic>
#include <thread>
#include <stdexcept>
#include "../../validation/bnw_font_texture_observation_storage.inc"
namespace civ5::graphics::detail {
struct BnwFontTextureBorrower::SourceViewPin final {
    const std::weak_ptr<ui::detail::BnwFontTextureRequest::Life> request;
    const std::weak_ptr<runtime::detail::BnwFontResourceLoan::Life> resource;
    const std::uint64_t occurrence,epoch;
    // Actual live arguments at entry; only callback-free weak/scalar copies.
    // No string allocation, Source observer or currentness callback is entered.
    SourceViewPin(const ui::detail::BnwFontTextureRequest& actual,
                  const runtime::detail::BnwFontResourceLoan& loan)
        :request(actual.life_),resource(loan.life_),occurrence(actual.life_->occurrence_),
         epoch(loan.life_->epoch.load()){}
    void first()const{
        const auto rq=request.lock();const auto rl=resource.lock();
        if(!rq||!rl)throw std::runtime_error("Font view pre-captured real request/resource retired");
        rq->require_current_owned(occurrence);rl->first(epoch);
    }
};
BnwFontTextureBorrower::BnwFontTextureBorrower(std::shared_ptr<const Impl> state):impl_(std::move(state)){}
BnwFontTextureBorrower::~BnwFontTextureBorrower()=default;

// Read-only fresh borrower check. It issues no new texture query or Font result.
void BnwFontTextureBorrower::require_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const runtime::detail::BnwFontResourceLoan& resource,
    const ui::detail::BnwFontTextureRequest& request)const{
    const auto saved=impl_;const SourceViewPin pins(request,resource);pins.first();saved->first();
    require_metadata_current_owned(saved,pins,operation,resource,request);
}
void BnwFontTextureBorrower::require_metadata_current_owned(
    const std::shared_ptr<const Impl>& saved,const SourceViewPin& pins,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const runtime::detail::BnwFontResourceLoan& resource,
    const ui::detail::BnwFontTextureRequest& request){
    const auto first=[&]{pins.first();saved->first();};
    first();const auto serial=operation.operation_serial();first();
    const auto full=[&]{
        first();request.require_current_owned(operation,resource);first();
        resource.require_current_owned(operation);first();operation.require_operation(serial);first();
    };
    full();
    if(resource.generation_!=saved->generation||resource.source_.owner_before(saved->source)||
       saved->source.owner_before(resource.source_))
        throw std::runtime_error("Font source view actual Source/generation changed");
    const auto key=std::string(request.source_owned(operation,resource));first();full();
    if(key!=saved->key)throw std::runtime_error("Font borrower is not this actual base/index request source");
    const assets::AssetKey asset_key(key);first();full();
    const auto bank=resource.bank_owner_owned(operation);first();full();
    if(!bank||bank.get()!=saved->bank.get()||bank.owner_before(saved->bank)||saved->bank.owner_before(bank))
        throw std::runtime_error("Font source view canonical Bank changed");
    const auto& capability=resource.bank_capability_owned(operation);first();full();
    // Re-read the SAME current staged winner. A prior archive hit/digest or
    // retained old bytes alone cannot seal the optional BC reader input.
    const auto selected=capability.loaded(asset_key);first();full();
    if(!selected.bytes||selected.bytes.get()!=saved->loaded.bytes.get()||
       selected.bytes.owner_before(saved->loaded.bytes)||saved->loaded.bytes.owner_before(selected.bytes)||
       selected.payload_identity.bytes!=saved->loaded.payload_identity.bytes||
       selected.payload_identity.sha256!=saved->loaded.payload_identity.sha256||
       selected.candidate.normalized_name!=saved->loaded.candidate.normalized_name||
       selected.candidate.provenance.source_id!=saved->loaded.candidate.provenance.source_id||
       selected.candidate.provenance.source_relative_path!=saved->loaded.candidate.provenance.source_relative_path||
       selected.candidate.provenance.archive_relative_path!=saved->loaded.candidate.provenance.archive_relative_path)
        throw std::runtime_error("Font source view winner/bytes identity replaced");
    full();
}
assets::CivisLoadedResource BnwFontTextureBorrower::captured_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const runtime::detail::BnwFontResourceLoan& resource,
    const ui::detail::BnwFontTextureRequest& request)const{
    const auto saved=impl_;const SourceViewPin pins(request,resource);pins.first();saved->first();
    require_metadata_current_owned(saved,pins,operation,resource,request);pins.first();saved->first();
    if(request.optional_index_owned(operation,resource)){
        pins.first();saved->first();throw std::runtime_error("Font BC source view requires actual original BASE request");
    }
    pins.first();saved->first();
    // Complete immutable metadata pin survives allocation/new_handler while
    // this is copied. It contains weak GPU payload authority, never GL ownership.
    auto result=saved->loaded;pins.first();saved->first();
    require_metadata_current_owned(saved,pins,operation,resource,request);pins.first();saved->first();return result;
}

BnwFontTextureObservation::BnwFontTextureObservation(std::unique_ptr<Impl> state):impl_(std::move(state)){}
BnwFontTextureObservation::~BnwFontTextureObservation()=default;
void BnwFontTextureObservation::require_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const runtime::detail::BnwFontResourceLoan& resource,
    const ui::detail::BnwFontTextureRequest& request)const{
    // Complete private VALUE pin before the first callbackful Source/Request
    // check. No Impl/this access follows observers or allocation callbacks.
    const auto creator=impl_->creator;
    const auto request_life=impl_->request_life;
    const auto resource_life=impl_->resource_life;
    const auto occurrence=impl_->occurrence;
    const auto resource_epoch=impl_->resource_epoch;
    const auto serial=impl_->serial;
    const auto request_identity=impl_->request_identity;
    const auto resource_identity=impl_->resource_identity;
    const auto borrower=impl_->borrower;
    const auto first=[&]{
        if(creator!=std::this_thread::get_id())throw std::runtime_error("Font observation foreign creator");
        const auto rq=request_life.lock();const auto rl=resource_life.lock();
        if(!rq||!rl||!borrower)throw std::runtime_error("Font actual query resource/request retired");
        rq->require_current_owned(occurrence);rl->first(resource_epoch);borrower->impl_->first();
    };
    first();
    if(&request!=request_identity||&resource!=resource_identity)
        throw std::runtime_error("Font texture observation uses another actual query request");
    request.require_current_owned(operation,resource);first();
    if(request.texture_occurrence_owned(operation,resource)!=occurrence)
        throw std::runtime_error("Font texture query occurrence replaced");
    const auto life=request.texture_lifetime_owned(operation,resource);first();
    if(life.owner_before(request_life)||request_life.owner_before(life))
        throw std::runtime_error("Font texture request control replaced");
    operation.require_operation(serial);first();
    resource.require_current_owned(operation);first();
}
bool BnwFontTextureObservation::observed_null_owned()const{
    impl_->first();return false; // Only payload form exists; NULL issuer remains STOP.
}
std::shared_ptr<const BnwFontTextureBorrower> BnwFontTextureObservation::borrower_owned()const{
    impl_->first();return impl_->borrower;
}
}
