#include "civ5/ui/detail/bnw_normal_nonfont_pipeline.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace civ5::ui::detail {
#include "bnw_normal_nonfont_native_effects_core.inc"
namespace {
using NativeRegion=BnwNormalNonFontNativeRegion;
void nonfont_region_first(const NativeRegion& region,std::size_t minimum){
    nonfont_native_need(region.owner&&region.bytes&&region.extent>=minimum,
        "NonFont original retained native region is absent/short");
    // native_base is uint32_t; subtract within uint64_t before comparing a
    // possibly64-bit extent. Never add SIZE_MAX to the native base.
    nonfont_native_need(region.extent<=0x100000000ull-std::uint64_t(region.native_base),
        "NonFont original native mapping exceeds32bit domain");
}
void nonfont_queue_owner_first(const NativeRegion& region){
    // The actual no-registration INDEX0/missing-resource branch need not
    // have writable queue capacity. Retain its genuine canonical owner and
    // native identity; only a positive32-entry publication requires bytes.
    nonfont_native_need(bool(region.owner),"NonFont original queue owner absent");
    nonfont_native_need(region.extent<=0x100000000ull-std::uint64_t(region.native_base),
        "NonFont original queue mapping exceeds32bit domain");
    nonfont_native_need(region.extent==0||region.bytes!=nullptr,
        "NonFont positive queue extent has no original physical bytes");
}
bool nonfont_same_owner(const std::shared_ptr<void>& a,const std::shared_ptr<void>& b)noexcept{
    return !a.owner_before(b)&&!b.owner_before(a);
}
bool nonfont_same_region(const NativeRegion& a,const NativeRegion& b)noexcept{
    return a.bytes==b.bytes&&a.extent==b.extent&&a.native_base==b.native_base&&
        nonfont_same_owner(a.owner,b.owner);
}
void nonfont_mapping_overlap(const NativeRegion& a,const NativeRegion& b){
    const auto begin=std::max(std::uint64_t(a.native_base),std::uint64_t(b.native_base));
    const auto end=std::min(std::uint64_t(a.native_base)+a.extent,
        std::uint64_t(b.native_base)+b.extent);
    if(begin<end)nonfont_native_need(nonfont_same_owner(a.owner,b.owner)&&
        a.bytes+static_cast<std::size_t>(begin-a.native_base)==
        b.bytes+static_cast<std::size_t>(begin-b.native_base),
        "NonFont original native mappings alias different physical owners");
}
NativeRegion nonfont_subregion(const NativeRegion& region,std::uint32_t base,std::size_t extent){
    nonfont_region_first(region,1);
    nonfont_native_need(base>=region.native_base,"NonFont native subrange precedes owner");
    const auto offset=std::size_t(base-region.native_base);
    nonfont_native_range(region.extent,offset,extent);
    return {region.owner,region.bytes+offset,extent,base};
}
}

// The keyed Root capture CALLSITE must supply genuine canonical storage.
// A mapping/Root Guard alone never proves Original constructor or helper ABI.
struct BnwNormalNonFontNativePool::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const CapturedFields fields;
    std::atomic<bool> alive{true},failed{false};
    explicit Impl(CapturedFields value):fields(std::move(value)){}
    void first()const{
        nonfont_native_need(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "NonFont native pool original facade retired/foreign/failed");
        fields.root.require_physical_current_without_callbacks();
        nonfont_native_need(fields.operation&&fields.serial,
            "NonFont native pool actual Source association absent");
        nonfont_region_first(fields.owner,0x4e);
        nonfont_region_first(fields.payload,0xf0);
        nonfont_queue_owner_first(fields.queue);
        nonfont_region_first(fields.cleanup_owner,0x2d);
        nonfont_native_need(nonfont_native_u32(fields.owner.bytes,fields.owner.extent,0x24)==
            fields.payload.native_base&&nonfont_native_u32(fields.owner.bytes,
                fields.owner.extent,0x14)==fields.queue.native_base,
            "NonFont native pool genuine backing association changed");
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();nonfont_native_need(&operation==fields.operation,
            "NonFont native pool Source differs before borrowed operation");
        fields.root.require_source_owned(operation);first();
        operation.require_operation(fields.serial);first();
    }
};
BnwNormalNonFontNativePool::BnwNormalNonFontNativePool(std::shared_ptr<Impl> value):impl_(std::move(value)){nonfont_native_need(bool(impl_),"NonFont pool control absent");}
BnwNormalNonFontNativePool::~BnwNormalNonFontNativePool(){if(impl_)impl_->alive.store(false);}
std::shared_ptr<const BnwNormalNonFontNativePool> BnwNormalNonFontNativePool::capture_owned(
    const BnwNormalNonFontCaptureKey&,const runtime::BnwWorldViewSourceOperationLoan& operation,
    CapturedFields fields){
    // Copy every strong owner/root control before Source or allocation.
    auto retained=std::move(fields);
    retained.root.require_physical_current_without_callbacks();
    nonfont_native_need(retained.operation==&operation&&retained.serial,
        "NonFont native pool capture Source association differs");
    retained.root.require_source_owned(operation);
    retained.root.require_physical_current_without_callbacks();
    operation.require_operation(retained.serial);
    retained.root.require_physical_current_without_callbacks();
    auto storage=std::make_shared<Impl>(std::move(retained));
    storage->full(operation);
    auto result=std::shared_ptr<const BnwNormalNonFontNativePool>(new BnwNormalNonFontNativePool(storage));
    storage->full(operation);return result;
}
void BnwNormalNonFontNativePool::require_physical_without_callbacks()const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont original pool control absent");state->first();
}
void BnwNormalNonFontNativePool::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont original pool control absent");
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}
NativeRegion BnwNormalNonFontNativePool::owner_region_without_callbacks()const{const auto state=impl_;state->first();return state->fields.owner;}
NativeRegion BnwNormalNonFontNativePool::payload_region_without_callbacks()const{const auto state=impl_;state->first();return state->fields.payload;}
NativeRegion BnwNormalNonFontNativePool::queue_region_without_callbacks()const{const auto state=impl_;state->first();return state->fields.queue;}
NativeRegion BnwNormalNonFontNativePool::cleanup_owner_region_without_callbacks()const{const auto state=impl_;state->first();return state->fields.cleanup_owner;}
void BnwNormalNonFontNativePool::require_record_available_without_callbacks()const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont original pool control absent");state->first();
    (void)nonfont_native_prepare_f0(state->fields.owner.bytes,state->fields.owner.extent,
        state->fields.payload.native_base,state->fields.payload.extent);state->first();
}
void BnwNormalNonFontNativePool::cleanup_native_8eefe0_without_callbacks()const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont cleanup pool control absent");state->first();
    // Actual global5E8[14], distinct from original global6F0 lender owner.
    // Repeated byte0 is an observable original effect; adjacent bytes untouched.
    *reinterpret_cast<volatile std::uint8_t*>(state->fields.cleanup_owner.bytes+0x2c)=0;
    state->first();
}

struct BnwNormalNonFontNativeSlots::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const CapturedFields fields;
    std::atomic<bool> alive{true},failed{false};
    explicit Impl(CapturedFields value):fields(std::move(value)){}
    void first()const{
        nonfont_native_need(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "NonFont actual manager facade retired/foreign/failed");
        fields.root.require_physical_current_without_callbacks();
        nonfont_native_need(fields.operation&&fields.serial,
            "NonFont actual manager Source association absent");
        nonfont_region_first(fields.manager,0xd014bc);
        nonfont_region_first(fields.descriptors,0x64);
        nonfont_native_need(std::uint64_t(fields.manager.native_base)+0x81088==
            fields.descriptors.native_base,
            "NonFont descriptor backing is not original SAME manager+81088");
        nonfont_mapping_overlap(fields.manager,fields.descriptors);
        for(const auto& region:fields.mapped_regions)nonfont_region_first(region,1);
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();nonfont_native_need(&operation==fields.operation,
            "NonFont actual manager Source differs before borrowed operation");
        fields.root.require_source_owned(operation);first();
        operation.require_operation(fields.serial);first();
    }
};
BnwNormalNonFontNativeSlots::BnwNormalNonFontNativeSlots(std::shared_ptr<Impl> value):impl_(std::move(value)){nonfont_native_need(bool(impl_),"NonFont manager control absent");}
BnwNormalNonFontNativeSlots::~BnwNormalNonFontNativeSlots(){if(impl_)impl_->alive.store(false);}
std::shared_ptr<const BnwNormalNonFontNativeSlots> BnwNormalNonFontNativeSlots::capture_owned(
    const BnwNormalNonFontCaptureKey&,const runtime::BnwWorldViewSourceOperationLoan& operation,
    CapturedFields fields){
    auto retained=std::move(fields);
    retained.root.require_physical_current_without_callbacks();
    nonfont_native_need(retained.operation==&operation&&retained.serial,
        "NonFont actual manager capture Source association differs");
    retained.root.require_source_owned(operation);retained.root.require_physical_current_without_callbacks();
    operation.require_operation(retained.serial);retained.root.require_physical_current_without_callbacks();
    nonfont_region_first(retained.manager,0xd014bc);nonfont_region_first(retained.descriptors,0x64);
    nonfont_mapping_overlap(retained.manager,retained.descriptors);
    for(std::size_t i=0;i<retained.mapped_regions.size();++i){
        nonfont_region_first(retained.mapped_regions[i],1);
        nonfont_mapping_overlap(retained.manager,retained.mapped_regions[i]);
        nonfont_mapping_overlap(retained.descriptors,retained.mapped_regions[i]);
        for(std::size_t j=0;j<i;++j)nonfont_mapping_overlap(retained.mapped_regions[i],retained.mapped_regions[j]);
    }
    auto storage=std::make_shared<Impl>(std::move(retained));storage->full(operation);
    auto result=std::shared_ptr<const BnwNormalNonFontNativeSlots>(new BnwNormalNonFontNativeSlots(storage));
    storage->full(operation);return result;
}
void BnwNormalNonFontNativeSlots::require_physical_without_callbacks()const{const auto state=impl_;nonfont_native_need(bool(state),"NonFont manager control absent");state->first();}
void BnwNormalNonFontNativeSlots::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont manager control absent");
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}
NativeRegion BnwNormalNonFontNativeSlots::manager_region_without_callbacks()const{const auto state=impl_;state->first();return state->fields.manager;}
NativeRegion BnwNormalNonFontNativeSlots::descriptor_region_without_callbacks()const{const auto state=impl_;state->first();return state->fields.descriptors;}
NativeRegion BnwNormalNonFontNativeSlots::actual_bucket_region_without_callbacks(std::uint32_t base,std::size_t extent)const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont manager control absent");state->first();
    const auto contains=[&](const NativeRegion& region){return base>=region.native_base&&
        std::uint64_t(base-region.native_base)+extent<=region.extent;};
    if(contains(state->fields.manager))return nonfont_subregion(state->fields.manager,base,extent);
    for(const auto& region:state->fields.mapped_regions)if(contains(region))return nonfont_subregion(region,base,extent);
    throw std::runtime_error("NonFont selected original recycle backing has no retained physical supplier");
}
void BnwNormalNonFontNativeSlots::require_slot_and_release_available_without_callbacks()const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont original manager control absent");state->first();
    const auto& manager=state->fields.manager;
    nonfont_native_gate_available(manager.bytes,manager.extent);
    const auto cut=nonfont_native_prepare_fresh_slot(manager.bytes,manager.extent);
    const auto index=cut.limit_return_zero?0:cut.index;
    nonfont_native_range(state->fields.descriptors.extent,nonfont_native_index_offset(index,0x64,0),0x64);
    // Predict only the proved INC then eventual ADD-1 over the CURRENT count.
    // No synthetic count1/currentcounter1 or substituted free-list is stored.
    if(index!=0&&cut.prior_count==0){
        const auto selector=nonfont_native_u32(manager.bytes,manager.extent,0x80044);
        const auto offset=nonfont_native_index_offset(selector,0x410,0x80048);
        nonfont_native_range(manager.extent,offset,0x410);
        const auto used=nonfont_native_u32(manager.bytes,manager.extent,offset+4);
        const auto capacity=nonfont_native_u32(manager.bytes,manager.extent,offset+8);
        nonfont_native_need(used!=capacity,"NonFont native8E3360 growth supplier unavailable before F0 effects");
        nonfont_native_need(used<capacity,"NonFont original recycle bucket domain invalid before F0 effects");
        const auto base=nonfont_native_u32(manager.bytes,manager.extent,offset);
        const auto needed=nonfont_native_index_offset(used,4,4);
        const auto backing=actual_bucket_region_without_callbacks(base,needed);
        nonfont_native_need(base!=0&&std::uint64_t(base)+std::uint64_t(used)*4<=0xffffffffull,
            "NonFont original zero-recycle writable pointer unavailable before F0 effects");
        nonfont_native_range(backing.extent,nonfont_native_index_offset(used,4,0),4);
    }
    state->first();
}

struct BnwNormalNonFontRecordLoan::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const BnwNormalNonFontPaintInvocation invocation;
    const std::shared_ptr<const BnwNormalNonFontNativePool> pool;
    const NativeRegion owner,payload;
    const NonFontNativeCursorCut cut;
    std::atomic<bool> alive{true},failed{false};
    bool published=false,complete=false;
    std::bitset<0xf0> coverage;
    Impl(BnwNormalNonFontPaintInvocation call,std::shared_ptr<const BnwNormalNonFontNativePool> source,
        NativeRegion actual_owner,NativeRegion backing,NonFontNativeCursorCut value)
        :invocation(std::move(call)),pool(std::move(source)),owner(std::move(actual_owner)),
        payload(std::move(backing)),cut(value){}
    void first()const{
        nonfont_native_need(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "NonFont original F0 loan retired/foreign/failed");
        invocation.require_current_without_callbacks();
        nonfont_native_need(pool&&invocation.native_pool_owned_without_callbacks()==pool,
            "NonFont F0 original pool/invocation differs");
        pool->require_physical_without_callbacks();
        nonfont_native_need(nonfont_same_region(owner,pool->owner_region_without_callbacks())&&
            nonfont_same_region(payload,pool->payload_region_without_callbacks()),
            "NonFont F0 original backing controls changed");
        if(published){
            nonfont_native_need(nonfont_native_u32(owner.bytes,owner.extent,0x30)==cut.pointer30&&
                nonfont_native_u32(owner.bytes,owner.extent,0x40)==cut.new_cursor&&
                nonfont_native_u16(owner.bytes,owner.extent,0x4c)==0xf0,
                "NonFont ACTIVE F0 occurrence changed");
        }else nonfont_native_need(nonfont_native_u32(owner.bytes,owner.extent,0x40)==cut.old_cursor,
            "NonFont F0 cursor changed before native publication");
        nonfont_native_range(payload.extent,cut.record_offset,0xf0);
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();invocation.require_source_owned(operation);first();pool->require_source_owned(operation);first();
    }
};
BnwNormalNonFontRecordLoan::BnwNormalNonFontRecordLoan(std::shared_ptr<Impl> state):impl_(std::move(state)){nonfont_native_need(bool(impl_),"NonFont F0 control absent");}
BnwNormalNonFontRecordLoan::~BnwNormalNonFontRecordLoan(){if(impl_)impl_->alive.store(false);}
std::unique_ptr<BnwNormalNonFontRecordLoan> BnwNormalNonFontRecordLoan::capture_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call){
    const auto invocation=call;invocation.require_current_without_callbacks();
    const auto pool=invocation.native_pool_owned_without_callbacks();
    const auto slots=invocation.native_slots_owned_without_callbacks();
    nonfont_native_need(bool(pool),"NonFont genuine source-pool supplier absent");pool->require_physical_without_callbacks();
    nonfont_native_need(bool(slots),"NonFont genuine current manager supplier absent");slots->require_physical_without_callbacks();
    pool->require_record_available_without_callbacks();slots->require_slot_and_release_available_without_callbacks();
    const auto owner=pool->owner_region_without_callbacks(),payload=pool->payload_region_without_callbacks();
    const auto cut=nonfont_native_prepare_f0(owner.bytes,owner.extent,payload.native_base,payload.extent);
    invocation.require_source_owned(operation);invocation.require_current_without_callbacks();
    pool->require_source_owned(operation);invocation.require_current_without_callbacks();pool->require_physical_without_callbacks();
    slots->require_source_owned(operation);invocation.require_current_without_callbacks();slots->require_physical_without_callbacks();
    nonfont_native_need(nonfont_same_region(owner,pool->owner_region_without_callbacks())&&
        nonfont_same_region(payload,pool->payload_region_without_callbacks())&&
        nonfont_native_u32(owner.bytes,owner.extent,0x40)==cut.old_cursor,
        "NonFont F0 authentic preallocation occurrence changed");
    auto state=std::make_shared<Impl>(invocation,pool,owner,payload,cut);
    try{
        state->full(operation);
        auto result=std::unique_ptr<BnwNormalNonFontRecordLoan>(new BnwNormalNonFontRecordLoan(state));
        state->full(operation);state->first();
        slots->require_source_owned(operation);state->first();slots->require_physical_without_callbacks();
        pool->require_record_available_without_callbacks();slots->require_slot_and_release_available_without_callbacks();state->first();
        nonfont_native_publish_f0(owner.bytes,cut);state->published=true;
        state->full(operation);return result;
    }catch(...){state->failed.store(true);throw;}
}
void BnwNormalNonFontRecordLoan::require_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call)const{
    const auto state=impl_;const auto invocation=call;nonfont_native_need(bool(state),"NonFont F0 control absent");
    state->first();state->invocation.require_same_invocation_without_callbacks(invocation);
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}
void BnwNormalNonFontRecordLoan::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call)const{
    const auto state=impl_;const auto invocation=call;nonfont_native_need(bool(state),"NonFont F0 control absent");
    state->first();state->invocation.require_same_invocation_without_callbacks(invocation);
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}
void BnwNormalNonFontRecordLoan::write_word_without_callbacks(std::uint32_t offset,std::uint32_t value){
    const auto state=impl_;state->first();nonfont_native_need(state->published&&!state->complete,"NonFont F0 write after seal");
    nonfont_native_range(0xf0,offset,4);nonfont_native_put32(state->payload.bytes+state->cut.record_offset,offset,value);
    for(std::size_t i=0;i<4;++i)state->coverage.set(offset+i);state->first();
}
void BnwNormalNonFontRecordLoan::write_word16_without_callbacks(std::uint32_t offset,std::uint16_t value){
    const auto state=impl_;state->first();nonfont_native_need(state->published&&!state->complete,"NonFont F0 write after seal");
    nonfont_native_range(0xf0,offset,2);nonfont_native_put16(state->payload.bytes+state->cut.record_offset,offset,value);
    state->coverage.set(offset);state->coverage.set(offset+1);state->first();
}
void BnwNormalNonFontRecordLoan::write_byte_without_callbacks(std::uint32_t offset,std::uint8_t value){
    const auto state=impl_;state->first();nonfont_native_need(state->published&&!state->complete,"NonFont F0 write after seal");
    nonfont_native_range(0xf0,offset,1);state->payload.bytes[state->cut.record_offset+offset]=std::byte(value);
    state->coverage.set(offset);state->first();
}
void BnwNormalNonFontRecordLoan::seal_complete_without_callbacks(const BnwNormalNonFontPaintInvocation& call){
    const auto state=impl_;const auto invocation=call;state->first();
    state->invocation.require_same_invocation_without_callbacks(invocation);
    nonfont_native_need(state->published&&!state->complete&&state->coverage.any(),
        "NonFont actual producer cannot seal unwritten/repeated F0");
    // Only the closed actual Button producer can call this after its proved
    // conditional write sequence. Unwritten native lanes retain prior bytes.
    state->complete=true;state->first();
}
BnwNormalNonFontRecordLoan::Guard::Guard(std::shared_ptr<Impl> state):impl_(std::move(state)){nonfont_native_need(bool(impl_),"NonFont F0 guard control absent");}
BnwNormalNonFontRecordLoan::Guard BnwNormalNonFontRecordLoan::capture_guard_without_callbacks()const{
    const auto state=impl_;state->first();return Guard(state);
}
void BnwNormalNonFontRecordLoan::Guard::require_physical_without_callbacks(const BnwNormalNonFontPaintInvocation& call)const{
    const auto state=impl_;state->first();state->invocation.require_same_invocation_without_callbacks(call);
}
void BnwNormalNonFontRecordLoan::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call)const{
    const auto state=impl_;const auto invocation=call;state->first();state->invocation.require_same_invocation_without_callbacks(invocation);
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}

struct BnwNormalNonFontSlotLoan::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const BnwNormalNonFontPaintInvocation invocation;
    const BnwNormalRootBatchHistory::Guard root;
    const std::shared_ptr<const BnwNormalNonFontNativeSlots> slots;
    const NativeRegion manager,descriptors;
    const NonFontNativeFreshSlotCut cut;
    const std::uint32_t index;
    const std::size_t descriptor_offset;
    std::atomic<bool> alive{true},failed{false};
    bool published=false,released=false,frozen_for_registration=false;
    Impl(BnwNormalNonFontPaintInvocation call,std::shared_ptr<const BnwNormalNonFontNativeSlots> actual,
        NativeRegion owner,NativeRegion backing,NonFontNativeFreshSlotCut value,std::uint32_t result_index,
        std::size_t offset):invocation(std::move(call)),root(invocation.captured_root_without_callbacks()),
        slots(std::move(actual)),manager(std::move(owner)),descriptors(std::move(backing)),cut(value),
        index(result_index),descriptor_offset(offset){}
    void first()const{
        nonfont_native_need(creator==std::this_thread::get_id()&&alive.load()&&!failed.load()&&!released,
            "NonFont original slot facade retired/foreign/failed/released");
        invocation.require_current_without_callbacks();root.require_physical_current_without_callbacks();
        nonfont_native_need(slots&&invocation.native_slots_owned_without_callbacks()==slots,
            "NonFont original slot/invocation control differs");
        slots->require_physical_without_callbacks();
        nonfont_native_need(nonfont_same_region(manager,slots->manager_region_without_callbacks())&&
            nonfont_same_region(descriptors,slots->descriptor_region_without_callbacks()),
            "NonFont original slot backing controls changed");
        nonfont_native_range(descriptors.extent,descriptor_offset,0x64);
        if(!published){
            const auto current=nonfont_native_prepare_fresh_slot(manager.bytes,manager.extent);
            nonfont_native_need(current.index==cut.index&&current.next_counter==cut.next_counter&&
                current.prior_count==cut.prior_count&&current.limit_return_zero==cut.limit_return_zero,
                "NonFont actual issuance changed before native effect");
        }else if(index)nonfont_native_need(nonfont_native_u32(manager.bytes,manager.extent,
            nonfont_native_index_offset(index,4,0x44))>0,
            "NonFont original slot indexed count no longer positive");
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();invocation.require_source_owned(operation);first();slots->require_source_owned(operation);first();
    }
    NonFontNativeReleaseCut prepare_release(NativeRegion& bucket)const{
        first();
        if(index==0)return nonfont_native_prepare_release(manager.bytes,manager.extent,0,0,0);
        const auto count=nonfont_native_u32(manager.bytes,manager.extent,
            nonfont_native_index_offset(index,4,0x44));
        if(count>1)return nonfont_native_prepare_release(manager.bytes,manager.extent,index,0,0);
        const auto selector=nonfont_native_u32(manager.bytes,manager.extent,0x80044);
        const auto offset=nonfont_native_index_offset(selector,0x410,0x80048);
        nonfont_native_range(manager.extent,offset,0x410);
        const auto used=nonfont_native_u32(manager.bytes,manager.extent,offset+4);
        const auto capacity=nonfont_native_u32(manager.bytes,manager.extent,offset+8);
        nonfont_native_need(used!=capacity,
            "NonFont native8E3360 zero-recycle growth supplier unavailable");
        const auto base=nonfont_native_u32(manager.bytes,manager.extent,offset);
        const auto needed=nonfont_native_index_offset(used,4,4);
        bucket=slots->actual_bucket_region_without_callbacks(base,needed);
        return nonfont_native_prepare_release(manager.bytes,manager.extent,index,base,bucket.extent);
    }
};
BnwNormalNonFontSlotLoan::BnwNormalNonFontSlotLoan(std::shared_ptr<Impl> state):impl_(std::move(state)){nonfont_native_need(bool(impl_),"NonFont slot control absent");}
BnwNormalNonFontSlotLoan::~BnwNormalNonFontSlotLoan(){if(impl_)impl_->alive.store(false);}
std::unique_ptr<BnwNormalNonFontSlotLoan> BnwNormalNonFontSlotLoan::capture_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call){
    const auto invocation=call;invocation.require_current_without_callbacks();
    const auto slots=invocation.native_slots_owned_without_callbacks();
    nonfont_native_need(bool(slots),"NonFont genuine current manager supplier absent");slots->require_physical_without_callbacks();
    slots->require_slot_and_release_available_without_callbacks();
    const auto manager=slots->manager_region_without_callbacks(),descriptors=slots->descriptor_region_without_callbacks();
    const auto cut=nonfont_native_prepare_fresh_slot(manager.bytes,manager.extent);
    const auto index=cut.limit_return_zero?0:cut.index;
    const auto descriptor_offset=nonfont_native_index_offset(index,0x64,0);
    nonfont_native_range(descriptors.extent,descriptor_offset,0x64);
    invocation.require_source_owned(operation);invocation.require_current_without_callbacks();
    slots->require_source_owned(operation);invocation.require_current_without_callbacks();slots->require_physical_without_callbacks();
    auto state=std::make_shared<Impl>(invocation,slots,manager,descriptors,cut,index,descriptor_offset);
    try{
        state->full(operation);
        auto result=std::unique_ptr<BnwNormalNonFontSlotLoan>(new BnwNormalNonFontSlotLoan(state));
        state->full(operation);state->first();
        slots->require_slot_and_release_available_without_callbacks();
        {
            NonFontNativeManagerGate gate(manager.bytes,manager.extent);
            state->first();const auto actual=nonfont_native_issue_fresh_slot(manager.bytes,cut);
            nonfont_native_need(actual==index,"NonFont native slot result association changed");
            state->published=true;
        } //8E5940 gate is released BEFORE actual8E6D60 memset/header stores.
        state->first();nonfont_native_initialize_descriptor100(descriptors.bytes+descriptor_offset);
        state->full(operation);return result;
    }catch(...){state->failed.store(true);throw;}
}
void BnwNormalNonFontSlotLoan::require_current_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call)const{
    const auto state=impl_;const auto invocation=call;state->first();state->invocation.require_same_invocation_without_callbacks(invocation);
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}
void BnwNormalNonFontSlotLoan::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call)const{
    const auto state=impl_;const auto invocation=call;state->first();state->invocation.require_same_invocation_without_callbacks(invocation);
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}
void BnwNormalNonFontSlotLoan::write_word_without_callbacks(std::uint32_t offset,std::uint32_t value){
    const auto state=impl_;state->first();nonfont_native_need(state->published&&!state->frozen_for_registration,"NonFont slot write before issuance/after freeze");
    nonfont_native_range(0x64,offset,4);nonfont_native_put32(state->descriptors.bytes+state->descriptor_offset,offset,value);state->first();
}
void BnwNormalNonFontSlotLoan::write_word16_without_callbacks(std::uint32_t offset,std::uint16_t value){
    const auto state=impl_;state->first();nonfont_native_need(state->published&&!state->frozen_for_registration,"NonFont slot write before issuance/after freeze");
    nonfont_native_range(0x64,offset,2);nonfont_native_put16(state->descriptors.bytes+state->descriptor_offset,offset,value);state->first();
}
void BnwNormalNonFontSlotLoan::write_byte_without_callbacks(std::uint32_t offset,std::uint8_t value){
    const auto state=impl_;state->first();nonfont_native_need(state->published&&!state->frozen_for_registration,"NonFont slot write before issuance/after freeze");
    nonfont_native_range(0x64,offset,1);state->descriptors.bytes[state->descriptor_offset+offset]=std::byte(value);state->first();
}
std::uint32_t BnwNormalNonFontSlotLoan::index_without_callbacks()const{const auto state=impl_;state->first();return state->index;}
std::array<std::byte,0x64> BnwNormalNonFontSlotLoan::descriptor_words_without_callbacks()const{
    const auto state=impl_;state->first();std::array<std::byte,0x64> bytes;
    std::memcpy(bytes.data(),state->descriptors.bytes+state->descriptor_offset,bytes.size());state->first();return bytes;
}
void BnwNormalNonFontSlotLoan::release_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call){
    const auto state=impl_;const auto invocation=call;state->first();state->invocation.require_same_invocation_without_callbacks(invocation);
    try{
        state->full(operation);NativeRegion bucket{};
        // All unavailable mandatory lanes fail before count decrement.
        (void)state->prepare_release(bucket);
        if(state->index!=0)nonfont_native_gate_available(state->manager.bytes,state->manager.extent);
        if(state->index==0){state->released=true;state->alive.store(false);}
        else {
            NonFontNativeManagerGate gate(state->manager.bytes,state->manager.extent);
            const auto cut=state->prepare_release(bucket);
            (void)nonfont_native_release_slot(state->manager.bytes,bucket.bytes,cut);
            state->released=true;state->alive.store(false);
        }
        // Retirement is permanent. Post Source uses independent original
        // Root/manager health; never the retired Slot's ACTIVE first.
        state->root.require_physical_current_without_callbacks();
        state->slots->require_source_owned(operation);state->root.require_physical_current_without_callbacks();
        state->root.require_source_owned(operation);state->root.require_physical_current_without_callbacks();
    }catch(...){state->failed.store(true);throw;}
}
BnwNormalNonFontSlotLoan::Guard::Guard(std::shared_ptr<Impl> state):impl_(std::move(state)){nonfont_native_need(bool(impl_),"NonFont slot guard control absent");}
BnwNormalNonFontSlotLoan::Guard BnwNormalNonFontSlotLoan::capture_guard_without_callbacks()const{const auto state=impl_;state->first();return Guard(state);}
void BnwNormalNonFontSlotLoan::Guard::require_physical_without_callbacks(const BnwNormalNonFontPaintInvocation& call)const{
    const auto state=impl_;state->first();state->invocation.require_same_invocation_without_callbacks(call);
}
void BnwNormalNonFontSlotLoan::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call)const{
    const auto state=impl_;const auto invocation=call;state->first();state->invocation.require_same_invocation_without_callbacks(invocation);
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}

struct BnwNormalNonFontItemOccurrence::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const BnwNormalRootBatchHistory::Guard root;
    const std::shared_ptr<const BnwNormalNonFontPaintInvocation::Impl> invocation_control;
    const std::shared_ptr<BnwNormalNonFontRecordLoan::Impl> record_control;
    const std::shared_ptr<BnwNormalNonFontSlotLoan::Impl> slot_control;
    const std::shared_ptr<const BnwNormalNonFontNativePool> pool;
    const std::shared_ptr<const BnwNormalNonFontNativeSlots> slots;
    const std::uint64_t control;
    const std::size_t ordered_ordinal;
    const std::uint32_t index,material_word,queue_ordinal,payload_pointer;
    const std::uint16_t configuration;
    const std::array<std::byte,0xf0> payload;
    const std::bitset<0xf0> coverage;
    const std::array<std::byte,0x64> descriptor;
    const std::array<std::byte,0x20> registration;
    const std::array<std::shared_ptr<const BnwNormalNonFontResourceLoan>,4> resources;
    bool sealed=false;
    mutable std::atomic<bool> failed{false};
    Impl(BnwNormalRootBatchHistory::Guard actual_root,
        std::shared_ptr<const BnwNormalNonFontPaintInvocation::Impl> actual_invocation,
        std::shared_ptr<BnwNormalNonFontRecordLoan::Impl> record,
        std::shared_ptr<BnwNormalNonFontSlotLoan::Impl> slot,
        std::uint64_t actual_control,std::size_t ordinal,std::uint32_t actual_material,
        std::uint16_t actual_configuration,std::uint32_t native_queue_ordinal,
        std::array<std::byte,0xf0> actual_payload,std::bitset<0xf0> actual_coverage,
        std::array<std::byte,0x64> actual_descriptor,std::array<std::byte,0x20> actual_registration,
        std::array<std::shared_ptr<const BnwNormalNonFontResourceLoan>,4> actual_resources)
        :root(std::move(actual_root)),invocation_control(std::move(actual_invocation)),
        record_control(std::move(record)),slot_control(std::move(slot)),pool(record_control->pool),
        slots(slot_control->slots),control(actual_control),ordered_ordinal(ordinal),index(slot_control->index),
        material_word(actual_material),queue_ordinal(native_queue_ordinal),payload_pointer(record_control->cut.pointer30),
        configuration(actual_configuration),payload(actual_payload),coverage(actual_coverage),
        descriptor(actual_descriptor),registration(actual_registration),resources(std::move(actual_resources)){}
    void associations()const{
        nonfont_native_need(creator==std::this_thread::get_id()&&!failed.load()&&invocation_control&&
            record_control&&slot_control&&pool&&slots,
            "NonFont historical original controls missing/foreign/failed");
        root.require_physical_current_without_callbacks();
        pool->require_physical_without_callbacks();slots->require_physical_without_callbacks();
        nonfont_native_need(record_control->creator==creator&&record_control->published&&record_control->complete&&
            !record_control->failed.load()&&record_control->pool==pool&&
            slot_control->creator==creator&&slot_control->published&&!slot_control->failed.load()&&
            slot_control->slots==slots&&slot_control->index==index&&index!=0&&
            record_control->invocation.impl_==invocation_control&&slot_control->invocation.impl_==invocation_control&&
            record_control->cut.pointer30==payload_pointer,
            "NonFont historical image is not SAME sealed original record/slot/invocation");
        nonfont_native_need(nonfont_native_u32(descriptor.data(),descriptor.size(),0)==0x41&&
            nonfont_native_u32(registration.data(),registration.size(),0)==index&&
            nonfont_native_u32(registration.data(),registration.size(),4)==0xffffffffu&&
            nonfont_native_u16(registration.data(),registration.size(),8)==configuration&&
            nonfont_native_u32(registration.data(),registration.size(),0x0c)==0&&
            nonfont_native_u32(registration.data(),registration.size(),0x10)==material_word&&
            nonfont_native_u32(registration.data(),registration.size(),0x14)==payload_pointer&&
            nonfont_native_u16(registration.data(),registration.size(),0x18)==0xf0,
            "NonFont immutable native registration/descriptor association differs");
        for(const auto& resource:resources)if(resource)resource->require_physical_without_callbacks();
    }
    void before(const BnwNormalNonFontPaintInvocation& invocation)const{
        associations();nonfont_native_need(!sealed,"NonFont repeated native registration");
        invocation.require_current_without_callbacks();
        nonfont_native_need(invocation.impl_==invocation_control,"NonFont registration current invocation differs");
        record_control->first();slot_control->first();
        nonfont_native_need(record_control->coverage==coverage&&
            std::memcmp(payload.data(),record_control->payload.bytes+record_control->cut.record_offset,payload.size())==0&&
            std::memcmp(descriptor.data(),slot_control->descriptors.bytes+slot_control->descriptor_offset,descriptor.size())==0,
            "NonFont sealed snapshots changed before actual registration");
        const auto owner=pool->owner_region_without_callbacks(),queue=pool->queue_region_without_callbacks();
        const auto cut=nonfont_native_prepare_registration(owner.bytes,owner.extent,queue.native_base,queue.extent,payload_pointer);
        nonfont_native_need(cut.old_count==queue_ordinal,
            "NonFont actual32-entry cursor changed before registration");
        const auto actual_entry=queue.bytes+cut.entry_offset;
        nonfont_native_need(std::memcmp(actual_entry+0x0a,registration.data()+0x0a,2)==0&&
            std::memcmp(actual_entry+0x1a,registration.data()+0x1a,6)==0,
            "NonFont untouched original32-entry lanes changed before native stores");
    }
    void before_full(const runtime::BnwWorldViewSourceOperationLoan& operation,
        const BnwNormalNonFontPaintInvocation& invocation)const{
        before(invocation);invocation.require_source_owned(operation);before(invocation);
        pool->require_source_owned(operation);before(invocation);
        slots->require_source_owned(operation);before(invocation);
        for(const auto& resource:resources)if(resource){resource->require_source_owned(operation);before(invocation);}
    }
    void first()const{
        nonfont_native_need(sealed,"NonFont historical item has no actual registered/released native result");
        associations();nonfont_native_need(slot_control->released,
            "NonFont historical registered item lacks actual8E4D80/4530 return");
        // Deliberately no ACTIVE Record/Slot/Invocation first, latest cursor,
        // count, native descriptor bytes or latest slot occupant comparison.
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();root.require_source_owned(operation);first();pool->require_source_owned(operation);first();
        slots->require_source_owned(operation);first();
        for(const auto& resource:resources)if(resource){resource->require_source_owned(operation);first();}
    }
};
BnwNormalNonFontItemOccurrence::BnwNormalNonFontItemOccurrence(std::shared_ptr<const Impl> state):impl_(std::move(state)){nonfont_native_need(bool(impl_),"NonFont item control absent");}
BnwNormalNonFontItemOccurrence::~BnwNormalNonFontItemOccurrence()=default;
std::shared_ptr<const BnwNormalNonFontItemOccurrence> BnwNormalNonFontItemOccurrence::register_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const BnwNormalNonFontPaintInvocation& call,
    BnwNormalNonFontRecordLoan& original_record,BnwNormalNonFontSlotLoan& original_slot,
    std::array<std::shared_ptr<const BnwNormalNonFontResourceLoan>,4> resources){
    // Save all ORIGINAL controls before the first Source callback. Never use
    // the borrowed unique Record/Slot facades again after these captures.
    const auto record=original_record.impl_;
    const auto slot=original_slot.impl_;
    const auto invocation=call;
    nonfont_native_need(record&&slot,"NonFont original record/slot controls absent");
    record->first();slot->first();
    record->invocation.require_same_invocation_without_callbacks(invocation);
    slot->invocation.require_same_invocation_without_callbacks(invocation);
    nonfont_native_need(record->complete&&slot->published&&!slot->frozen_for_registration&&slot->index!=0,
        "NonFont registration requires completed actual F0 and nonzero original index");
    const auto pool=record->pool;
    const auto slots=slot->slots;
    const auto root=invocation.captured_root_without_callbacks();
    const auto control=invocation.actual_control_without_callbacks();
    const auto ordinal=invocation.actual_ordered_ordinal_without_callbacks();
    const auto material=invocation.material_selector_without_callbacks();
    const auto configuration=invocation.native_configuration_without_callbacks();
    std::array<std::byte,0xf0> payload;
    std::memcpy(payload.data(),record->payload.bytes+record->cut.record_offset,payload.size());
    const auto coverage=record->coverage;
    std::array<std::byte,0x64> descriptor;
    std::memcpy(descriptor.data(),slot->descriptors.bytes+slot->descriptor_offset,descriptor.size());
    const auto owner=pool->owner_region_without_callbacks(),queue=pool->queue_region_without_callbacks();
    const auto queue_cut=nonfont_native_prepare_registration(owner.bytes,owner.extent,queue.native_base,
        queue.extent,record->cut.pointer30);
    std::array<std::byte,0x20> registration;
    std::memcpy(registration.data(),queue.bytes+queue_cut.entry_offset,registration.size());
    // Preserve untouched original entry bytes in the immutable future image.
    nonfont_native_put32(registration.data(),4,0xffffffffu);nonfont_native_put32(registration.data(),0,slot->index);
    nonfont_native_put16(registration.data(),8,configuration);nonfont_native_put32(registration.data(),0x0c,0);
    nonfont_native_put32(registration.data(),0x14,record->cut.pointer30);nonfont_native_put16(registration.data(),0x18,0xf0);
    nonfont_native_put32(registration.data(),0x10,material);
    NativeRegion bucket{};(void)slot->prepare_release(bucket); // Missing native lane =>no registration mutation.
    slot->frozen_for_registration=true;
    // These frozen fields have their own strong owner, allocated PRE/POST
    // before registration/count effects. No fake Current/Draw permission.
    std::shared_ptr<Impl> state;
    try{
        const auto before_allocation=[&]{
            record->first();slot->first();root.require_physical_current_without_callbacks();
            pool->require_physical_without_callbacks();slots->require_physical_without_callbacks();
            for(const auto& resource:resources)if(resource)resource->require_physical_without_callbacks();
            nonfont_native_need(record->coverage==coverage&&
                std::memcmp(payload.data(),record->payload.bytes+record->cut.record_offset,payload.size())==0&&
                std::memcmp(descriptor.data(),slot->descriptors.bytes+slot->descriptor_offset,descriptor.size())==0&&
                nonfont_native_u32(owner.bytes,owner.extent,0x0c)==queue_cut.old_count&&
                std::memcmp(queue.bytes+queue_cut.entry_offset+0x0a,registration.data()+0x0a,2)==0&&
                std::memcmp(queue.bytes+queue_cut.entry_offset+0x1a,registration.data()+0x1a,6)==0,
                "NonFont original allocation snapshots/current32 cursor changed");
        };
        before_allocation();invocation.require_source_owned(operation);before_allocation();
        pool->require_source_owned(operation);before_allocation();
        slots->require_source_owned(operation);before_allocation();
        for(const auto& resource:resources)if(resource){resource->require_source_owned(operation);before_allocation();}
        state=std::make_shared<Impl>(root,invocation.impl_,record,slot,control,ordinal,material,
            configuration,queue_cut.old_count,payload,coverage,descriptor,registration,std::move(resources));
        state->before_full(operation,invocation);
        auto result=std::shared_ptr<const BnwNormalNonFontItemOccurrence>(new BnwNormalNonFontItemOccurrence(state));
        state->before_full(operation,invocation);state->before(invocation);
        nonfont_native_gate_available(slot->manager.bytes,slot->manager.extent);
        (void)slot->prepare_release(bucket);
        nonfont_native_register32(owner.bytes,queue.bytes+queue_cut.entry_offset,queue_cut,slot->index,material,configuration);
        nonfont_native_need(std::memcmp(queue.bytes+queue_cut.entry_offset,registration.data(),registration.size())==0,
            "NonFont actual32-entry stores differ from retained original image");
        // Actual8E4D80 replaces incoming manager with SAME canonical native
        // slots receiver and tailcalls8E4530. No Source cut between these two
        // native effects; immutable original images already have strong custody.
        {
            NonFontNativeManagerGate gate(slot->manager.bytes,slot->manager.extent);
            const auto release_cut=slot->prepare_release(bucket);
            (void)nonfont_native_release_slot(slot->manager.bytes,bucket.bytes,release_cut);
            slot->released=true;slot->alive.store(false);
        }
        state->sealed=true;state->full(operation);return result;
    }catch(...){if(state)state->failed.store(true);record->failed.store(true);slot->failed.store(true);throw;}
}
BnwNormalNonFontItemOccurrence::Guard::Guard(std::shared_ptr<const Impl> state):impl_(std::move(state)){nonfont_native_need(bool(impl_),"NonFont item guard control absent");}
BnwNormalNonFontItemOccurrence::Guard BnwNormalNonFontItemOccurrence::capture_guard_without_callbacks()const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont original item control absent");state->first();return Guard(state);
}
void BnwNormalNonFontItemOccurrence::Guard::require_physical_current_without_callbacks()const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont historical item control absent");state->first();
}
void BnwNormalNonFontItemOccurrence::Guard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto state=impl_;nonfont_native_need(bool(state),"NonFont historical item control absent");
    try{state->full(operation);}catch(...){state->failed.store(true);throw;}
}
void BnwNormalNonFontItemOccurrence::Guard::require_same_item_without_callbacks(const Guard& other)const{
    const auto state=impl_,expected=other.impl_;nonfont_native_need(state&&expected,"NonFont SAME item control absent");
    state->first();expected->first();nonfont_native_need(state==expected,"NonFont historical item differs");
}
void BnwNormalNonFontItemOccurrence::Guard::require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard& other)const{
    const auto state=impl_;state->first();state->root.require_same_root_without_callbacks(other);state->first();
}
std::array<std::byte,0xf0> BnwNormalNonFontItemOccurrence::Guard::payload_without_callbacks()const{const auto state=impl_;state->first();return state->payload;}
std::bitset<0xf0> BnwNormalNonFontItemOccurrence::Guard::write_coverage_without_callbacks()const{const auto state=impl_;state->first();return state->coverage;}
std::array<std::byte,0x64> BnwNormalNonFontItemOccurrence::Guard::descriptor_without_callbacks()const{const auto state=impl_;state->first();return state->descriptor;}
std::array<std::byte,0x20> BnwNormalNonFontItemOccurrence::Guard::registration_without_callbacks()const{const auto state=impl_;state->first();return state->registration;}
std::uint32_t BnwNormalNonFontItemOccurrence::Guard::index_without_callbacks()const{const auto state=impl_;state->first();return state->index;}
std::array<std::shared_ptr<const BnwNormalNonFontResourceLoan>,4> BnwNormalNonFontItemOccurrence::Guard::resources_owned_without_callbacks()const{const auto state=impl_;state->first();return state->resources;}
} // namespace civ5::ui::detail
