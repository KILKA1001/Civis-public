#include "bnw_native_archive_registry_source.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include <iostream>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void refuses(F&& f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}need(failed,"Source diagnostic accepted unsupported branch");}
template<class T>concept PublicAcquire=requires{T::acquire({});};
template<class T>concept PublicHeaderAcquire=requires{T::acquire({},{},{},{});};
template<class T>concept PublicEntryPrepare=requires(const T& token){token.prepare_bound_construction_metadata();};
template<class T>concept PublicPrefixRegister=requires(const T& owner){owner.register_header_prefix({});};
}
int main(){try{
    static_assert(!std::is_default_constructible_v<BnwNativeArchiveV6StreamHeaderOwner> &&
                  !std::is_copy_constructible_v<BnwNativeArchiveV6StreamHeaderOwner> &&
                  !std::is_default_constructible_v<BnwNativeGlobalArchivePrefixRecord> &&
                  !std::is_aggregate_v<BnwNativeGlobalArchivePrefixRecord> &&
                  !PublicAcquire<BnwNativeGlobalArchivePrefixOwner> &&
                  !PublicHeaderAcquire<BnwNativeArchiveV6StreamHeaderOwner> &&
                  !PublicPrefixRegister<BnwNativeGlobalArchivePrefixOwner> &&
                  !PublicEntryPrepare<BnwNativeGlobalArchivePrefixRecord>);
    // Exact source transaction primitive, without a fake typed-owner factory.
    // Equal data at different addresses cannot cross privately sealed brands.
    int registry_id=0,record_id=0,header_id=0,construction_id=0,cold_id=0,lease_id=0,service_id=0,other_id=0;
    const BnwArchivePrefixIdentityDiagnostic identity{&registry_id,&record_id,&header_id,&construction_id,&cold_id,&lease_id,&service_id};
    BnwArchivePrefixPhaseDiagnostic transaction;transaction.seal_prepared(identity);
    refuses([&]{transaction.require_prepared_current(identity);});
    transaction.mark_prepared_current(identity);
    for(unsigned slot=0;slot<7;++slot){
        auto wrong=identity;
        switch(slot){case 0:wrong.registry=&other_id;break;case 1:wrong.record=&other_id;break;
            case 2:wrong.header=&other_id;break;case 3:wrong.construction=&other_id;break;
            case 4:wrong.cold=&other_id;break;case 5:wrong.lease=&other_id;break;default:wrong.service=&other_id;break;}
        refuses([&]{transaction.require_prepared_current(wrong);});
    }
    transaction.require_prepared_current(identity);
    static_assert(noexcept(transaction.publish_after_source_store()));
    int callbacks=0,source_stores=0;
    const auto preflight=[&]{++callbacks;transaction.require_prepared_current(identity);};
    preflight();const int callbacks_before_store=callbacks;
    ++source_stores;transaction.publish_after_source_store();
    need(callbacks==callbacks_before_store && source_stores==1,"Pure publication tail must not run callback/check after shared store");
    refuses([&]{transaction.require_prepared_current(identity);});
    transaction.start_entry(identity);
    refuses([&]{transaction.start_entry(identity);});
    refuses([&]{transaction.require_entry_current(identity);});
    transaction.mark_entry_current(identity);transaction.require_entry_current(identity);
    transaction.retire_entry();
    refuses([&]{transaction.require_entry_current(identity);});
    refuses([&]{transaction.start_entry(identity);});
    // Reuse production391 fence: even a caught nested-entry refusal poisons
    // the outer transaction, which therefore never reaches its source store.
    struct LifetimeDiagnostic {BnwNativeCallbackOwnerState state;mutable std::weak_ptr<const LifetimeDiagnostic> self;};
    auto mutable_owner=std::make_shared<LifetimeDiagnostic>();
    std::shared_ptr<const LifetimeDiagnostic> lifetime=mutable_owner;
    lifetime->self=lifetime;mutable_owner.reset();
    using Fence=BnwNativeCallbackOwnerGuard<LifetimeDiagnostic>;
    {
        Fence outer(lifetime->state,lifetime->self);need(outer.valid(),"Pure lifetime fixture invalid");
        try{Fence nested(lifetime->state,lifetime->self);need(nested.valid(),"Expected caught reentry refusal");}catch(const std::exception&){}
        need(!outer.valid(),"Caught registry reentry must poison outer operation");
        if(outer.valid())++source_stores;
    }
    need(source_stores==1,"Caught reentry allowed a publication store");
    auto retired=std::make_shared<LifetimeDiagnostic>();
    std::shared_ptr<const LifetimeDiagnostic> external=retired;retired->self=external;retired.reset();
    const auto weak=external->self;
    {
        Fence fence(external->state,weak);need(fence.valid(),"Pure retirement fixture invalid");
        external.reset();need(!fence.valid(),"Last external facade loss cannot be hidden by internal pin");
        if(fence.valid())++source_stores;
    }
    need(weak.expired() && source_stores==1,"Retired facade survived or published");
    auto parent_setup=std::make_shared<LifetimeDiagnostic>();
    std::shared_ptr<const LifetimeDiagnostic> parent=parent_setup;parent_setup->self=parent;parent_setup.reset();
    const auto parent_weak=parent->self;
    {
        const auto transient_wrapper_pin=parent_weak.lock();
        Fence fence(parent->state,parent_weak);
        need(fence.valid() && bnw_prefix_persistent_facade_diagnostic(parent_weak.use_count(),2),
             "Source entry fixture must distinguish external from TWO internal registry pins");
        parent.reset();
        need(fence.valid() && !bnw_prefix_persistent_facade_diagnostic(parent_weak.use_count(),2),
             "Transient wrapper pin must not hide loss of persistent registry facade");
        if(fence.valid() && bnw_prefix_persistent_facade_diagnostic(parent_weak.use_count(),2))++source_stores;
    }
    need(parent_weak.expired() && source_stores==1,"Transient parent retirement allowed store");
    std::array<std::uint8_t,14> bytes={6,0,0,0,0x46,0x50,0x4b,0x5f,0,0,0,0,0,0};
    need(bnw_v6_header_fields(bytes).declared_entry_count==0,"Source header0 count must remain valid before prefix");
    bytes[10]=0xff;bytes[11]=0xff;bytes[12]=0xff;bytes[13]=0xff;
    need(bnw_v6_header_fields(bytes).declared_entry_count==0xffffffffu,"Declared count is unsigned source field, not admission");
    for(const auto offset:{0u,4u,8u,9u}){auto unsupported=bytes;unsupported[offset]^=1;refuses([&]{bnw_v6_header_fields(unsupported);});}
    refuses([&]{bnw_v6_header_fields(std::span<const std::uint8_t>(bytes.data(),13));});
    BnwGlobalPrefixKernelDiagnostic prefix;
    need(!prefix.lookup(u"Resource\\DX9\\Stock.fpk"),"Fresh source prefix nonempty");
    need(prefix.lookup_or_append_successful_allocation(u"Resource\\DX9\\Stock.fpk")==0 &&
         prefix.lookup_or_append_successful_allocation(u"Resource\\DX9\\Stock.fpk")==0 && prefix.count()==1,
         "Native lookup must reuse exact first prefix");
    need(prefix.lookup_or_append_successful_allocation(u"Resource\\DX9\\STOCK.fpk")==1 &&
         prefix.lookup_or_append_successful_allocation(u"C:\\Original\\Resource\\DX9\\Stock.fpk")==2,
         "Raw case/relative prefix must not fold into IO alias");
    need(prefix.append_successful_allocation(u"Resource\\DX9\\Stock.fpk")==3 &&
         prefix.lookup(u"Resource\\DX9\\Stock.fpk")==0,"Native duplicate append/first lookup differs");
    constexpr char16_t nul_name[]=u"Resource\\DX9\\Stock.fpk\0ignored";
    need(prefix.lookup(std::u16string_view(nul_name,std::size(nul_name)-1))==0,"Native UTF16 comparison stops at NUL");
    BnwArchiveBasenameCacheKernelDiagnostic cache;
    need(!cache.publish_modeled_admission("Resource\\DX9\\Stock.fpk",11,0)&&!cache.lookup("Stock.fpk"),
         "Declared/0 count cannot publish cache");
    need(prefix.count()==4,"Prefix persists independently of0 admitted archive");
    need(cache.publish_modeled_admission("Resource\\DX9\\Stock.fpk",11,1)&&cache.lookup("Other\\STOCK.FPK")==11,
         "Native namespace cache key is lowercase raw basename");
    need(!cache.publish_modeled_admission("Other\\Stock.fpk",22,1)&&cache.lookup("Stock.fpk")==11,
         "Native full-key insertion must retain existing archive");
    need(cache.publish_modeled_admission("Resource/DX9/Stock.fpk",33,1)&&cache.lookup("Resource/DX9/Stock.fpk")==33,
         "Forward slash is not native backslash delimiter");
    BnwArchiveBasenameCacheKernelDiagnostic separate_namespace;
    need(!separate_namespace.lookup("Stock.fpk"),"Archive cache must not be global across namespaces");
    std::cout<<"PASS pure v6/prefix/brands/phase/replay/caught-reentry/retirement/pin-budget checks; concrete private header/prefix actual acceptance requires root Construction hooks, no entry admission\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
