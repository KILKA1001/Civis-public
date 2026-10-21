#pragma once
#include "bnw_native_package_load_actor.hpp"
#include "bnw_native_skin_resource_query_output_producer.hpp"
#include <algorithm>
#include <variant>

namespace civ5::ui::detail {
class BnwNativeSelectedCategoryRecordBorrower;
//A selected receiver slot can reference only this actual manager source.
//Constructor is private; SAME822A50 manager attachment/admission belongs to
//Warm/Parent. No old descriptor, global cache selection or path can issue it.
class BnwNativeSelectedCategoryManagerSource {
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeSelectedCategoryRecordBorrower;
    using Load=BnwNativePackageLoadActor;
    using Incoming=std::variant<assets::AssetHandle,UnreadableLoose>;
    using RecordConsumer=std::function<void(const BnwNativeArchiveRecordDiagnostic*,
                                           const std::u16string*,const std::string*)>;
    const std::weak_ptr<Load::State> receiver_;
    const std::uint32_t category_;
    const std::thread::id thread_=std::this_thread::get_id();
    BnwNativeSelectedCategoryManagerSource(const Load& actual,std::uint32_t category)
        :receiver_(actual.state_),category_(category) {
        if(category>=3)throw std::runtime_error("Selected manager category outside actual receiver slots");
        actual.state_->current();
    }
    //Held references address the real selected manager and its actual table;
    //they must not be assembled from a copied registry or diagnostic map.
    class ManagerLoan {
        friend class BnwNativeSelectedCategoryManagerSource;
        friend class BnwNativeSelectedCategoryRecordBorrower;
        friend class BnwNativeGlobalResourceWarmOwner;
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        const BnwNativeSelectedCategoryManagerSource& source;
        const void* const actual_manager;
        const std::uint8_t& actual_enabled89;
        const NativeResourceRecordIndex& actual_table;
        ManagerLoan(const BnwNativeSelectedCategoryManagerSource& owner,const void* manager,
                    const std::uint8_t& enabled,const NativeResourceRecordIndex& table)
            :source(owner),actual_manager(manager),actual_enabled89(enabled),actual_table(table) {
            if(!manager)throw std::runtime_error("Selected manager loan lacks actual822A50 receiver");
        }
        ManagerLoan(const ManagerLoan&)=delete;
        ManagerLoan&operator=(const ManagerLoan&)=delete;
    };
    using ManagerConsumer=std::function<void(const ManagerLoan&)>;
    //Actual issuer must hold its current Stage/Parent fence across callback.
    //No sealed-stage requirement: native flag80 queries the currently executed
    //own manager operations, before final Name/cache promotion when necessary.
    virtual void with_current_manager(const ManagerConsumer&)const=0;
    virtual void require_manager_current(const ManagerLoan&)const=0;
    //SAME selected variant must match a real native admitted node retained by
    //the exact Stage receipt/stat owner. Unknown node source MUST throw; it
    //cannot turn a present selected row into an inspected null/MISS result.
    virtual void with_selected_native_record(const ManagerLoan&,std::uint32_t,
        const Incoming&,const RecordConsumer&)const=0;
    virtual void invalidate()const noexcept=0;
    void require_bound_receiver(const Load::ResourceLoan& resource)const {
        resource.require_current();
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Selected manager foreign thread");
        const auto actual=receiver_.lock(),expected=resource.state.lock();
        if(!actual||actual.get()!=expected.get()||receiver_.owner_before(resource.state)||
           resource.state.owner_before(receiver_))throw std::runtime_error("Selected manager belongs to another actual Package receiver");
        const auto& slot=actual->category_slots[category_];
        if(!slot||slot->lock().get()!=this)throw std::runtime_error("Selected manager actual receiver slot replaced/retired");
        actual->current();resource.require_current();
    }
  public:
    virtual ~BnwNativeSelectedCategoryManagerSource()=default;
    BnwNativeSelectedCategoryManagerSource(const BnwNativeSelectedCategoryManagerSource&)=delete;
};

class BnwNativeSelectedCategoryRecordBorrower final {
    friend class BnwNativePackageResourceCheckpoint;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    using Load=BnwNativePackageLoadActor;
    using Source=BnwNativeSelectedCategoryManagerSource;
    using Output=BnwNativeSkinResourceQueryOutputProducer;
    using Consumer=Source::RecordConsumer;
    BnwNativeSelectedCategoryRecordBorrower()=delete;
    static void need(bool condition,const char* error) {if(!condition)throw std::runtime_error(error);}
    //83AE20 copy/slash conversion,81DA10 last-backslash or first-colon split,
    //then83B4E0 actual retained platform lowercase/CRC. Empty basename has
    //CRCFFFFFFFF. The SAME receiver/core input owns the captured LC_CTYPE;
    //no guessed C mapping or caller-supplied language can enter this method.
    static std::uint32_t key_83ae20(std::string raw,const Load::ResourceLoan& resource) {
        std::replace(raw.begin(),raw.end(),'/','\\');
        const auto slash=raw.find_last_of('\\');
        const auto colon=slash==std::string::npos?raw.find(':'):std::string::npos;
        const auto split=slash!=std::string::npos?slash:colon;
        if(split!=std::string::npos)raw.erase(0,split+1);
        resource.require_current();
        for(unsigned char byte:raw)
            need(byte>=32&&byte<=126,"Selected manager query requires printable native ASCII basename input");
        //Uses the private Core held guard and SAME early Boot input affinity;
        //it never reenters Core State::Scope or borrows a different input.
        const auto lowered=resource.platform_lowercase_ascii_for_83b4e0(raw);
        resource.require_current();std::uint32_t crc=0xffffffffU;
        //Actual CRT can emit a high byte from ASCII I under its captured locale.
        //CRC must consume those emitted bytes, with no ASCII re-folding/filter.
        for(unsigned char byte:lowered) {
            crc^=byte;for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320U:0U);
        }
        resource.require_current();return crc;
    }
    static void with_selected_record(const Load::ResourceLoan& resource,std::uint32_t category,
                                     const std::string& raw,const Consumer& consume) {
        resource.require_current();need(category<3,"Selected flag80 requires actual named category0..2");
        const auto actual=resource.state.lock();need(bool(actual),"Selected query receiver retired");
        const auto& slot=actual->category_slots[category];
        //6F7D30 actually initializes these pointers to zero. Only the private
        //actual manager issuer can populate them; expired populated != zero.
        if(!slot) {
            consume(nullptr,nullptr,nullptr);resource.require_current();
            need(!actual->category_slots[category],"Selected zero receiver slot changed during output callback");return;
        }
        const auto selected=slot->lock();need(bool(selected),"Selected native manager expired, not a MISS");
        need(selected->category_==category,"Selected source belongs to another actual receiver category slot");
        selected->require_bound_receiver(resource);
        try {
            const auto query=Output::c_prefix(raw);
            std::size_t loans=0;
            selected->with_current_manager([&](const Source::ManagerLoan& manager){
                need(loans++==0 && &manager.source==selected.get(),"Selected source returned foreign/repeated manager loan");
                const auto enabled=manager.actual_enabled89;
                const auto check=[&]{
                    selected->require_bound_receiver(resource);selected->require_manager_current(manager);selected->require_bound_receiver(resource);
                    need(manager.actual_enabled89==enabled,"Selected native enable field changed during output borrow");
                };
                check();
                if(!manager.actual_enabled89||query.empty()) {consume(nullptr,nullptr,nullptr);check();return;}
                const auto key=key_83ae20(query,resource);check();
                const auto row=manager.actual_table.records.find(key);
                if(row==manager.actual_table.records.end()) {
                    consume(nullptr,nullptr,nullptr);check();
                    need(!manager.actual_table.records.contains(key),"Selected inspected MISS changed during output callback");return;
                }
                const auto* const entry=std::addressof(row->second);std::size_t callbacks=0;
                selected->with_selected_native_record(manager,key,*entry,[&](const auto* node,const auto* prefix,const auto* first){
                    need(callbacks++==0&&node&&node->crc==key,"Selected present row lacks SAME actual native record");
                    const auto native_words=*node;
                    check();need(manager.actual_table.records.contains(key)&&std::addressof(manager.actual_table.records.at(key))==entry,
                        "Selected native row replaced during source callback");
                    consume(node,prefix,first);check();
                    need(*node==native_words,"Selected native words changed during output consumer");
                    need(manager.actual_table.records.contains(key)&&std::addressof(manager.actual_table.records.at(key))==entry,
                        "Selected native row replaced by output consumer");
                });
                need(callbacks==1,"Selected native witness source omitted held callback");check();
            });
            need(loans==1,"Selected source omitted actual manager loan");selected->require_bound_receiver(resource);
        }catch(...){selected->invalidate();resource.invalidate();throw;}
    }
    class OutputSource final:private Output::Source {
        friend class BnwNativeSelectedCategoryRecordBorrower;
        const Load::ResourceLoan& resource;
        const std::uint32_t category;
        mutable bool failed=false;
        OutputSource(const Load::ResourceLoan& actual,std::uint32_t selected)
            :resource(actual),category(selected){require_current();}
        void require_current()const override {need(!failed,"Selected output source failed");resource.require_current();}
        void with_native_record(const std::string& query,const Output::Source::Consumer& consumer)const override {
            require_current();with_selected_record(resource,category,query,consumer);require_current();
        }
        std::string copied_receiver_for_62ec20_fallback()const override {
            require_current();auto result=resource.copied_receiver_for_62ec20();require_current();return result;
        }
        void invalidate()const noexcept override {failed=true;resource.invalidate();}
    };
    //Only actual54B8D0 flags80/param7=0 named-category caller may consume this
    //private kernel. It never calls global8211F0 or borrows another manager.
    static std::optional<std::string> output_5eb290_flags80(const Load::ResourceLoan& resource,
            std::uint32_t category,const std::string& raw) {
        need(category<3,"Selected output requires named category0..2");
        OutputSource source(resource,category);return Output::produce(raw,source);
    }
};
} //namespace civ5::ui::detail
