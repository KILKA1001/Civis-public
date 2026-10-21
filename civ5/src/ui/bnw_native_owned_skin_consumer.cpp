#include "bnw_native_owned_skin_consumer.hpp"
#include <cstdint>
#include <clocale>
#include <string_view>
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace civ5::ui::detail {
void BnwNativeOwnedSkinConsumer::require_current()const {
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned Skin consumer foreign thread");
    const auto storage=shared_from_this();
    if(failed_)throw std::runtime_error("Owned Skin consumer failed actual operation");
    source_->require_current();
    if(failed_)throw std::runtime_error("Owned Skin consumer retired in actual source callback");
}
bool BnwNativeOwnedSkinConsumer::load_slot_6d4bd0(Occurrence& occurrence,unsigned slot) {
    const auto check=[&]{require_current();occurrence.require_current();require_current();};
    check();
    if(occurrence.first_pair_source_present_48d2b0(slot)) {
        check();const auto enabled=occurrence.first_pair_source_enabled_47d680(slot);check();
        if(enabled)return false;
        occurrence.enable_actual_pair_767410(slot);check();
        occurrence.write_actual_instance_enabled_byte(slot);check();return true;
    }
    check();
    const auto directories=occurrence.query_owned_54b8d0(slot,Query::directory,0);check();
    const auto paks=occurrence.query_owned_54b8d0(slot,Query::paks,0);check();
    if(!directories.empty()||!paks.empty()) {
        occurrence.load_actual_paks_6f84f0(slot,paks);check();
        for(const auto& path:directories) {
            occurrence.register_actual_directory_419930_82a470(slot,path,7,"*.xml;*.ggxml");check();
        }
    }
    // File production occurs after Paks/Directory effects against the current
    // manager universe. Style and Color each have their own later query.
    const auto files=occurrence.query_owned_54b8d0(slot,Query::file,0x80);check();
    for(const auto& path:files) {
        occurrence.register_actual_file_419930_82a7a0(slot,path,7);check();
    }
    const auto styles=occurrence.query_owned_54b8d0(slot,Query::style,0x80);check();
    if(!styles.empty()) {
        occurrence.begin_actual_style_loading(slot);check();
        for(const auto& path:styles) {
            const auto result=occurrence.try_actual_style_load_8348c0_79f460_834b80(slot,path);check();
            if(result==NativeLoadResult::native_true)break;
        }
        occurrence.end_actual_style_loading(slot);check();
    }
    const auto colors=occurrence.query_owned_54b8d0(slot,Query::color,0x80);check();
    if(!colors.empty()) {
        occurrence.begin_actual_color_loading(slot);check();
        for(const auto& path:colors) {
            const auto result=occurrence.try_actual_color_load_601ca0(slot,path);check();
            if(result==NativeLoadResult::native_true)break;
        }
        occurrence.end_actual_color_loading(slot);check();
    }
    occurrence.publish_actual_name_5d5550(slot);check();
    occurrence.write_actual_instance_enabled_byte(slot);check();return true;
}
bool BnwNativeOwnedSkinConsumer::select_and_load_660330(int slot) {
    // Reject a foreign thread BEFORE reading/mutating non-atomic state.
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned Skin selector foreign thread");
    const auto storage=shared_from_this();
    if(active_){failed_=true;source_->invalidate_actual_operation();throw std::runtime_error("Owned Skin selector reentry");}
    if(slot<-1||slot>1)throw std::runtime_error("Owned Skin selector lacks actual slot0/1/-1 input");
    active_=true;
    struct Exit {BnwNativeOwnedSkinConsumer& consumer;~Exit(){consumer.active_=false;}} exit{*this};
    try {
        require_current(); // Actual active guard already covers the first source callback.
        bool changed=false;
        source_->with_actual_roster_660330([&](Occurrence& occurrence) {
            require_current();occurrence.require_current();
            const auto matches=occurrence.matches_actual_selection_64f970();
            occurrence.require_current();require_current();
            if(!matches)return;
            const unsigned begin=slot==-1?0u:static_cast<unsigned>(slot);
            const unsigned end=slot==-1?2u:begin+1u;
            for(unsigned current=begin;current<end;++current)
                changed=load_slot_6d4bd0(occurrence,current)||changed;
            occurrence.require_current();require_current();
        });
        require_current();source_->reconcile_actual_colors_462c20();require_current();
        completed_slot_selectors_.push_back(slot);require_current();
        return changed; //Native AL reports performed changes, never readiness.
    }catch(...){failed_=true;source_->invalidate_actual_operation();throw;}
}
}

#include "bnw_native_global_color_registry_bank.hpp"
#include "bnw_native_ui_skin_poststage_instance.hpp"
namespace civ5::ui::detail {
namespace {
void bank_owned_need(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
}
void BnwNativeGlobalColorRegistryBank::require_owned_numeric_context()const {
    // Explicit supported Civis arithmetic context. This reads actual current
    // CRT/SSE state and never changes locale/control bits to obtain a pass.
    // Original MSVCR90/thread-context equivalence is a separate open claim.
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Color numeric context foreign thread");
    const auto* ctype=std::setlocale(LC_CTYPE,nullptr);
    bank_owned_need(ctype&&std::string_view(ctype)=="C","Owned Color unsupported actual CRT CTYPE context");
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    // MXCSR round-to-nearest, flush-to-zero off, denormals-are-zero off.
    // Sticky exception status flags are observations, not authority.
    bank_owned_need((_mm_getcsr()&0xe040U)==0,"Owned Color unsupported actual SSE numeric context");
#else
    throw std::runtime_error("Owned Color actual SSE numeric context unavailable");
#endif
}
void BnwNativeGlobalColorRegistryBank::attach_same_owned_source(std::shared_ptr<const OwnedSource> source) {
    // Rejected foreign calls never read or mutate creator-thread state.
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Bank issuer foreign thread");
    OwnedCallScope operation(shared_from_this());
    try {
        bank_owned_need(source&&!owned_source_&&roster_.empty()&&owned_roster_.empty()&&
            !closed_.load()&&phase_==Phase::cold&&sources_.empty()&&global_&&
            global_->colors_.empty()&&!global_->sets_,
            "Owned Bank issuer needs SAME genuinely fresh cold constructor without legacy data");
        require_owned_numeric_context();source->require_same_bank_current(*this,nullptr,0);require_owned_numeric_context();
        bank_owned_need(!closed_.load()&&phase_==Phase::cold,"Owned Bank lost actual cold constructor during issuer validation");
        owned_source_=std::move(source);require_owned_current();
    }catch(...){if(source)source->invalidate_actual_operation();poison();throw;}
}
void BnwNativeGlobalColorRegistryBank::require_owned_current(
    const content::XmlNode* document,std::uint64_t occurrence)const {
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Bank currentness foreign thread");
    const bool completed=active_completed_resource_operation_!=nullptr;
    try {
        if(completed){
            require_owned_numeric_context();require_completed_operation_loan_current(document,occurrence);
            require_owned_numeric_context();return;
        }
        bank_owned_need(!closed_.load()&&phase_!=Phase::failed&&owned_source_,"Owned Bank actual issuer retired");
        if(active_owned_skin_operation_)require_owned_operation_loan_current();
        const auto source=owned_source_;
        require_owned_numeric_context();source->require_same_bank_current(*this,document,occurrence);require_owned_numeric_context();
        bank_owned_need(!closed_.load()&&phase_!=Phase::failed&&owned_source_.get()==source.get(),
            "Owned Bank actual source changed during currentness callback");
    }catch(...){if(!completed&&owned_source_)owned_source_->invalidate_actual_operation();
        const_cast<BnwNativeGlobalColorRegistryBank*>(this)->poison();throw;}
}
void BnwNativeGlobalColorRegistryBank::initialize_owned_field(
    const std::shared_ptr<BnwNativeColorSlotStorage>& field) {
    bank_owned_need(field&&!field->sets_,"Owned Bank constructor field replay");
    const std::weak_ptr<BnwNativeGlobalColorRegistryBank> weak=shared_from_this();
    field->sets_.reset(new BnwNativeColorSet64Storage([weak](const content::XmlNode* doc,std::uint64_t occurrence){
        const auto bank=weak.lock();bank_owned_need(bool(bank),"Owned ColorSet actual Bank retired");
        bank->require_owned_current(doc,occurrence);
    }));
}
void BnwNativeGlobalColorRegistryBank::load_owned_base() {
    OwnedCallScope operation(shared_from_this());
    try {
        require_owned_current();bank_owned_need(phase_==Phase::cold&&sources_.empty()&&global_&&
            global_->colors_.empty()&&!global_->sets_&&roster_.empty()&&owned_roster_.empty(),
            "Owned base Color loader requires SAME actual fresh cold constructor");
        std::size_t callbacks=0;
        owned_source_->with_actual_base_color_document([&](const auto& document) {
            bank_owned_need(callbacks++==0&&document,"Owned base Color issuer repeated/omitted actual parser document");
            require_owned_current(document.get(),1);
            sources_.push_back({document,1,false});initialize_owned_field(global_);phase_=Phase::base_colors;
            const auto actual=sources_.front().document;
            //Reuse the accepted601CA0 component behavior against the actual
            //selected Color XML occurrence. This is not a descriptor DOM.
            for(const auto& node:actual->children)if(node.name=="Color") {
                const std::string* name=nullptr;const std::string* text=nullptr;
                std::set<std::string> attributes;
                for(const auto& [key,value]:node.attributes) {
                    bank_owned_need(attributes.insert(key).second,"Owned Color duplicate attribute conversion unsupported");
                    if(key=="Name")name=&value;
                    if(key=="Color")text=&value;
                }
                bank_owned_need(name&&text,"Owned Color native retained attribute buffer unavailable");
                for(const auto* value:{name,text})for(unsigned char byte:*value)
                    bank_owned_need(byte>=0x20&&byte<=0x7e,"Owned Color FString conversion unsupported");
                const auto key=native_color_load_detail::hash_query(*name);
                if(global_->colors_.contains(key))continue;
                BnwNativeColorSlotStorage::Value value;
                require_owned_numeric_context();
                if(!native_color_load_detail::trim(*text).empty()) {
                    native_color_load_detail::numeric(*text,value.bits);require_owned_numeric_context();value.known.fill(true);
                }
                require_owned_current(actual.get(),1);global_->colors_.emplace(key,value);
            }
            sources_.front().colors_loaded=true;phase_=Phase::base_sets;
            global_->sets_->read_from_same_xml(actual,1,[this](std::uint32_t key){return ordered_owned_lookup(key);});
            require_owned_current(actual.get(),1);phase_=Phase::base_loaded;
        });
        bank_owned_need(callbacks==1,"Owned base Color issuer did not return real parser occurrence");
        require_owned_current();
    }catch(...){if(owned_source_)owned_source_->invalidate_actual_operation();poison();throw;}
}
void BnwNativeGlobalColorRegistryBank::bind_same_owned_roster() {
    OwnedCallScope operation(shared_from_this());
    try {
        require_owned_current();bank_owned_need(phase_==Phase::base_loaded&&roster_.empty()&&owned_roster_.empty(),
            "Owned Bank constructor roster replay/out of genuine base order");
        std::size_t callbacks=0;
        owned_source_->with_actual_constructed_roster([&](const auto& actual) {
            bank_owned_need(callbacks++==0,"Owned Bank issuer repeated actual constructed roster");
            require_owned_current();
            for(const auto& instance:actual) {
                bank_owned_need(bool(instance),"Owned Bank genuine roster contains null constructor");
                instance->require_current();require_owned_current();
                for(unsigned slot=0;slot<2;++slot) {
                    bank_owned_need(!instance->color_storage_[slot]&&instance->enabled_[slot]==0,
                        "Owned Bank constructor storage replay or fabricated enabled byte");
                    auto field=std::shared_ptr<BnwNativeColorSlotStorage>(new BnwNativeColorSlotStorage);
                    initialize_owned_field(field);instance->color_storage_[slot]=std::move(field);
                    instance->require_current();require_owned_current();
                }
                owned_roster_.emplace_back(instance); //Source supplies actual native order under its fence.
            }
            require_owned_current();
        });
        bank_owned_need(callbacks==1,"Owned Bank issuer omitted its actual constructor roster");
        require_owned_current();phase_=Phase::roster_bound;
    }catch(...){if(owned_source_)owned_source_->invalidate_actual_operation();poison();throw;}
}
std::optional<BnwNativeGlobalColorRegistryBank::Bits>
BnwNativeGlobalColorRegistryBank::ordered_owned_lookup(std::uint32_t key)const {
    require_owned_current();
    bank_owned_need(key!=0x8f4455c8U&&key!=0x164d0472U,
        "Owned normal Color lookup lacks actual special player-interface supplier");
    const auto read=[&](const std::shared_ptr<BnwNativeColorSlotStorage>& field)->std::optional<Bits> {
        bank_owned_need(bool(field),"Owned Color constructor field absent");
        const auto found=field->colors_.find(key);
        if(found==field->colors_.end())return std::nullopt;
        bank_owned_need(std::all_of(found->second.known.begin(),found->second.known.end(),[](bool value){return value;}),
            "Owned Color selected hit contains unknown native allocator bytes");
        return found->second.bits;
    };
    for(auto position=owned_roster_.rbegin();position!=owned_roster_.rend();++position) {
        const auto instance=position->lock();bank_owned_need(bool(instance),"Owned Color actual roster constructor retired");
        instance->require_current();require_owned_current();
        for(unsigned slot=0;slot<2;++slot)if(instance->enabled_[slot]) {
            const auto value=read(instance->color_storage_[slot]);
            instance->require_current();require_owned_current();if(value)return value;
        }
    }
    const auto value=read(global_);require_owned_current();return value;
}
void BnwNativeGlobalColorRegistryBank::reconcile_owned_colors_body_462c20() {
        require_owned_current();bank_owned_need(phase_==Phase::roster_bound||phase_==Phase::initial_finalized,
            "Owned462C20 lacks SAME actual constructed Skin roster");
        const auto lookup=[this](std::uint32_t key){return ordered_owned_lookup(key);};
        //462C20 walks actual roster in reverse and refreshes both blocks,
        //regardless of slot enabled state. Global ColorSet is refreshed last.
        for(auto position=owned_roster_.rbegin();position!=owned_roster_.rend();++position) {
            const auto instance=position->lock();bank_owned_need(bool(instance),"Owned462C20 actual constructor retired");
            instance->require_current();require_owned_current();
            for(unsigned slot=0;slot<2;++slot) {
                const auto field=instance->color_storage_[slot];
                bank_owned_need(field&&field->sets_,"Owned462C20 actual ColorSet constructor field absent");
                field->sets_->refresh_named_colors(lookup);instance->require_current();require_owned_current();
            }
        }
        bank_owned_need(global_&&global_->sets_,"Owned462C20 actual global ColorSet absent");
        global_->sets_->refresh_named_colors(lookup);require_owned_current();
}
void BnwNativeGlobalColorRegistryBank::reconcile_owned_colors_462c20() {
    OwnedCallScope operation(shared_from_this());
    try {reconcile_owned_colors_body_462c20();}
    catch(...){if(owned_source_)owned_source_->invalidate_actual_operation();poison();throw;}
}
void BnwNativeGlobalColorRegistryBank::finalize_owned_initial_skin_colors() {
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Skin finalization foreign thread");
    OwnedCallScope operation(shared_from_this());
    try {
    require_owned_current();bank_owned_need(phase_==Phase::roster_bound,"Owned Skin initial finalization replay");
    //This gate requires the issuer's actual completed selected resource,
    //Style/Color/name/enable effects. Roster/availability/counts cannot pass it.
    owned_source_->require_initial_skin_effects_terminal(*this);require_owned_current();
    reconcile_owned_colors_body_462c20();
    owned_source_->require_initial_skin_effects_terminal(*this);require_owned_current();
    phase_=Phase::initial_finalized;
    }catch(...){if(owned_source_)owned_source_->invalidate_actual_operation();poison();throw;}
}
}

#include "bnw_native_owned_skin_occurrence.hpp"
namespace civ5::ui {
// This implementation retains the closed actual occurrence capability only.
// Each call takes a new stack loan from SAME returned Loaded/Parser; it never
// retains a ConstructorLoan reference or a copied parser identity as authority.
class BnwNativePackageDiscoveryOwner::OwnedSkinConstructorSource final
    : public detail::BnwNativeOwnedUiSkinPoststageInstance::ConstructorSource {
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence> occurrence_;
    void require_creator_thread()const {
        if(thread_!=std::this_thread::get_id())
            throw std::runtime_error("Owned Skin constructor source foreign thread");
    }
  public:
    explicit OwnedSkinConstructorSource(
        std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence> occurrence)
        :occurrence_(std::move(occurrence)) {
        if(!occurrence_)throw std::runtime_error("Owned Skin constructor actual occurrence absent");
        require_current();
    }
    void require_current()const override {
        require_creator_thread();occurrence_->require_current();
    }
    const detail::BnwNativeOwnedSkinOccurrence& actual_occurrence()const override {
        require_current();return *occurrence_;
    }
    std::uint32_t actual_null_output_54b8d0(unsigned slot,
        const char* selector,std::uint32_t filter)const override {
        require_creator_thread();
        std::size_t callbacks=0;std::uint32_t result=0;
        occurrence_->with_actual_constructor_loan([&](const auto& loan) {
            if(callbacks++!=0)throw std::runtime_error("Owned Skin actual constructor loan repeated");
            loan.require_current();
            result=loan.actual_null_output_54b8d0(slot,selector,filter);
            loan.require_current();
        });
        if(callbacks!=1)throw std::runtime_error("Owned Skin actual constructor loan omitted");
        require_current();return result;
    }
    void invalidate_actual_operation()const noexcept override {
        if(thread_==std::this_thread::get_id())occurrence_->invalidate_actual_operation();
    }
};
void BnwNativePackageDiscoveryOwner::construct_native_owned_skin_instances()const {
    // Called only inside the genuine new Skin HeaderGuard. The roster walk
    // rejects a foreign thread/replay before any instance can be constructed.
    with_native_owned_skin_occurrences([&](const auto& occurrence) {
        // Own the actual issued token across every callback-bearing stage.
        //48C800 performs genuine Skin/Directory54B8D0+891960 BEFORE798CE0.
        const auto token=occurrence;
        token->require_current();
        prepare_native_owned_skin_extra_attribute_query(token);
        token->require_current();
        const auto source=std::shared_ptr<const OwnedSkinConstructorSource>(
            new OwnedSkinConstructorSource(token));
        const auto instance=std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>(
            new detail::BnwNativeOwnedUiSkinPoststageInstance(source));
        instance->require_current();token->require_current();
        // Actual798CE0 normal return precedes genuine56FBE0 ordered-list
        //transfer. Retained const locals cannot be replaced by callbacks.
        bind_native_owned_skin_extra_attributes(token,instance);
        instance->require_current();token->require_current();
        // Actual transferred list precedes the genuine global roster append.
        retain_native_owned_skin_instance(token,instance);
        instance->require_current();occurrence->require_current();
    });
    complete_native_owned_skin_instance_constructors();
}
}

namespace civ5::ui {
class BnwNativePackageDiscoveryOwner::OwnedColorBankSource final
    : public detail::BnwNativeGlobalColorRegistryBank::OwnedSource {
    using Bank=detail::BnwNativeGlobalColorRegistryBank;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::weak_ptr<Bank> bank_;
    std::shared_ptr<Bank> borrow_bank()const {
        if(thread_!=std::this_thread::get_id())
            throw std::runtime_error("Owned Color Source foreign creator thread");
        const auto bank=bank_.lock();
        if(!bank)throw std::runtime_error("Owned Color actual Bank receiver retired");
        return bank;
    }
  public:
    explicit OwnedColorBankSource(const std::shared_ptr<Bank>& bank):bank_(bank) {
        if(!bank)throw std::runtime_error("Owned Color Source actual Bank absent");
    }
    void require_same_bank_current(const Bank& actual,const content::XmlNode* document,
        std::uint64_t ordinal)const override {
        const auto bank=borrow_bank();
        if(bank.get()!=&actual)throw std::runtime_error("Owned Color Source foreign Bank identity");
        //One accounted pin for this actual Bank callback. No independent
        //Parent weak lock can keep a dropped external Package owner alive.
        Bank::PackagePin held(*bank);
        bank->require_owned_numeric_context();
        held.parent->require_native_owned_color_bank_current(actual,document,ordinal);
        bank->require_owned_numeric_context();
    }
    void with_actual_base_color_document(
        const std::function<void(const std::shared_ptr<const content::XmlNode>&)>& input)const override {
        const auto bank=borrow_bank();const auto consume=input;
        if(!consume)throw std::runtime_error("Owned Color base consumer absent");
        Bank::PackagePin held(*bank);
        require_same_bank_current(*bank,nullptr,0);
        held.parent->with_native_owned_base_color_document(*bank,[&](const auto& document) {
            require_same_bank_current(*bank,document.get(),1);consume(document);
            require_same_bank_current(*bank,document.get(),1);
        });
        require_same_bank_current(*bank,nullptr,0);
    }
    void with_actual_constructed_roster(const std::function<void(
        const std::vector<std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>>&)>& input)const override {
        const auto bank=borrow_bank();const auto consume=input;
        if(!consume)throw std::runtime_error("Owned Color constructor roster consumer absent");
        Bank::PackagePin held(*bank);require_same_bank_current(*bank,nullptr,0);
        held.parent->with_native_owned_skin_constructed_roster([&](const auto& actual) {
            require_same_bank_current(*bank,nullptr,0);consume(actual);
            require_same_bank_current(*bank,nullptr,0);
        });
        require_same_bank_current(*bank,nullptr,0);
    }
    void require_initial_skin_effects_terminal(const Bank& actual)const override {
        const auto bank=borrow_bank();require_same_bank_current(actual,nullptr,0);
        Bank::PackagePin held(*bank);
        //The genuine Parent gate currently refuses until a successor executes
        //actual selection/slot effects and retains its successful-return journal.
        //Base/constructor completion cannot issue this terminal transition.
        held.parent->require_native_owned_initial_skin_effects_terminal(actual);
        require_same_bank_current(actual,nullptr,0);
    }
    void invalidate_actual_operation()const noexcept override {
        if(thread_!=std::this_thread::get_id())return;
        if(const auto bank=bank_.lock()) {
            try {
                Bank::PackagePin held(*bank);
                held.parent->invalidate_native_owned_color_operation();
            }catch(...){bank->poison();}
        }
    }
};
void BnwNativePackageDiscoveryOwner::prepare_native_owned_color_bank()const {
    //Real native base-before-instance order, under the actual Skin operation's
    //held Loaded/Core loan. The receiver carries SAME Parent/Default/catalog.
    const auto bank=construct_native_owned_color_bank_receiver();
    try {
        const std::shared_ptr<const detail::BnwNativeGlobalColorRegistryBank::OwnedSource> source(
            new OwnedColorBankSource(bank));
        begin_native_owned_color_bank_source_attachment(*bank,source.get());
        bank->attach_same_owned_source(source);
        complete_native_owned_color_bank_source_attachment(*bank);
        bank->load_owned_base();
    }catch(...){invalidate_native_owned_color_operation();throw;}
}
}

namespace civ5::ui::detail {
void BnwNativeGlobalColorRegistryBank::require_owned_instance_destination(
    const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>& instance,unsigned slot)const {
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Skin destination foreign thread");
    require_owned_current();
    bank_owned_need(instance&&slot<2&&phase_==Phase::roster_bound,"Owned Skin destination lacks actual roster/slot");
    instance->require_current();
    bool found=false;
    for(const auto& entry:owned_roster_)if(entry.lock()==instance){found=true;break;}
    bank_owned_need(found,"Owned Skin destination is not SAME constructed Bank roster member");
    instance->require_current();require_owned_current();
}
void BnwNativeGlobalColorRegistryBank::begin_owned_style_loading(
    const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>& instance,unsigned slot) {
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Style reset foreign thread");
    OwnedCallScope operation(shared_from_this());
    try {
        require_owned_instance_destination(instance,slot);
        auto& field=instance->style_storage_[slot];
        //834870 discards the SAME previous reader/parser before835570 sets
        //the real destination node to NULL. No source DOM or ready seal.
        field.reader.reset();field.destination={};
        require_owned_instance_destination(instance,slot);
    }catch(...){if(owned_source_)owned_source_->invalidate_actual_operation();poison();throw;}
}
bool BnwNativeGlobalColorRegistryBank::write_owned_style_destination_834b80(
    const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>& instance,unsigned slot,
    const std::shared_ptr<BnwNativeMountedStyleReader>& actual_reader) {
    require_owned_instance_destination(instance,slot);
    bank_owned_need(bool(actual_reader),"Owned834B80 actual reader absent");
    const auto actual_node=actual_reader->first_same_owned_element();
    //The parsed root pointer belongs to this actual retained reader. The
    //native writer leaves the destination untouched if no type1 node exists.
    if(!actual_node.node){require_owned_instance_destination(instance,slot);return false;}
    const auto document=actual_node.document.lock();
    bank_owned_need(bool(document),"Owned834B80 actual parsed anchor retired");
    document->require_current();
    instance->style_storage_[slot].destination=actual_node;
    document->require_current();require_owned_instance_destination(instance,slot);return true;
}
bool BnwNativeGlobalColorRegistryBank::try_load_owned_style_8348c0(
    const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>& instance,unsigned slot,
    const std::shared_ptr<const BnwNativeOwnedSkinSelectedInput>& input) {
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Style loader foreign thread");
    OwnedCallScope operation(shared_from_this());
    try {
        require_owned_instance_destination(instance,slot);
        bank_owned_need(bool(input),"Owned Style load needs actual opened input receipt");
        const auto& occurrence=instance->constructor_source_->actual_occurrence();
        input->require_same_occurrence(occurrence,slot,"Style");require_owned_numeric_context();
        auto reader=std::shared_ptr<BnwNativeMountedStyleReader>(new BnwNativeMountedStyleReader(input));
        //8348C0 resets/owns its actual buffer/parser even on syntax false.
        instance->style_storage_[slot].reader=reader;
        const auto native_success=reader->try_parse_same_owned_source_834b30();
        input->require_same_occurrence(occurrence,slot,"Style");require_owned_instance_destination(instance,slot);
        if(!native_success)return false; //actual834B30 false: caller tries next emitted filename.
        //Retain the real source/buffer/parser before834B80 publishes its node.
        //Slot6D4BD0 ignores AL834B80; actual8348C0 success controls the break.
        instance->style_storage_[slot].reader=reader;
        (void)write_owned_style_destination_834b80(instance,slot,reader);
        input->require_same_occurrence(occurrence,slot,"Style");require_owned_instance_destination(instance,slot);
        return true;
    }catch(...){if(owned_source_)owned_source_->invalidate_actual_operation();poison();throw;}
}
}

namespace civ5::ui::detail {
bool BnwNativeGlobalColorRegistryBank::try_load_owned_color_601ca0(
    const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>& instance,unsigned slot,
    const std::shared_ptr<const BnwNativeOwnedSkinSelectedInput>& input) {
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Color loader foreign thread");
    OwnedCallScope operation(shared_from_this());
    try {
        require_owned_instance_destination(instance,slot);
        bank_owned_need(bool(input),"Owned Color load needs actual opened input receipt");
        const auto& occurrence=instance->constructor_source_->actual_occurrence();
        const auto check=[&]{
            input->require_same_occurrence(occurrence,slot,"Color");require_owned_instance_destination(instance,slot);
        };
        check();
        auto reader=std::shared_ptr<BnwNativeMountedStyleReader>(new BnwNativeMountedStyleReader(input));
        const auto native_success=reader->try_parse_same_owned_source_834b30();check();
        if(!native_success)return false; //real601CA0 reader false, no Color/ColorSet writes.
        const auto root=reader->first_same_owned_element();
        const auto document=root.document.lock();bank_owned_need(bool(document),"Owned Color scoped root anchor retired");
        auto name_local=reader->make_same_owned_string_local();auto value_local=reader->make_same_owned_string_local();
        const auto field=instance->color_storage_[slot];bank_owned_need(field&&field->sets_,"Owned Color actual constructor destination absent");
        for(const auto& node:root.same_matching_children_835660_8356c0("Color")) {
            check();document->require_current();
            const auto name=node.first_same_attribute_value_8355c0("Name");
            const auto converted_name=name.consume_same_string_local(name_local);
            bank_owned_need(converted_name!=BnwNativeMountedStyleReader::BorrowedNode::StringConsume::incomplete_adjacent_bytes,
                            "Owned Color Name needs unavailable native adjacent bytes");
            //Native601CA0 ignores conversion false, retaining the prior local.
            const auto actual_name=reader->copied_same_string_local(name_local);
            const auto key=native_color_load_detail::hash_query(actual_name);check();
            if(field->colors_.contains(key))continue;
            const auto color=node.first_same_attribute_value_8355c0("Color");
            const auto converted_value=color.consume_same_string_local(value_local);
            bank_owned_need(converted_value!=BnwNativeMountedStyleReader::BorrowedNode::StringConsume::incomplete_adjacent_bytes,
                            "Owned Color value needs unavailable native adjacent bytes");
            const auto actual_value=reader->copied_same_string_local(value_local);check();
            for(unsigned char byte:actual_value)bank_owned_need(byte>=0x20&&byte<=0x7e,"Owned Color selected numeric FString unsupported");
            BnwNativeColorSlotStorage::Value value;
            require_owned_numeric_context();
            if(!native_color_load_detail::trim(actual_value).empty()) {
                native_color_load_detail::numeric(actual_value,value.bits);require_owned_numeric_context();value.known.fill(true);
            }
            //Empty text preserves actual unknown allocator bytes. Normal
            //lookup rejects an unknown hit; no zero values are manufactured.
            document->require_current();check();field->colors_.emplace(key,value);check();
        }
        bank_owned_need(owned_selected_color_serial_<UINT64_MAX,"Owned Color actual load ordinal wrap unsupported");
        const auto actual_ordinal=++owned_selected_color_serial_;
        field->sets_->read_from_same_owned_reader(reader,actual_ordinal,name_local,value_local,
            [this](std::uint32_t key){return ordered_owned_lookup(key);});
        document->require_current();check();
        //Actual8348C0 returned normally. Empty root/Color/ColorSet is genuine
        //601CA0 true; the reader/root lasts through both passes then retires.
        return true;
    }catch(...){if(owned_source_)owned_source_->invalidate_actual_operation();poison();throw;}
}
}

namespace civ5::ui::detail {
BnwNativeGlobalColorRegistryBank::OwnedSkinOperation::OwnedSkinOperation(std::shared_ptr<BnwNativeGlobalColorRegistryBank> bank)
    :scope(std::move(bank)) {
    auto& actual=*scope.bank;
    bank_owned_need(!actual.active_owned_skin_operation_,"Owned Skin actual Bank operation loan replay");
    actual.active_owned_skin_operation_=this;actual.require_owned_operation_loan_current();
}
BnwNativeGlobalColorRegistryBank::OwnedSkinOperation::~OwnedSkinOperation() {
    //Stack loan is noncopyable/nonmovable and owns the actual originalScope.
    //It cannot outlive that Scope or be recreated from a caller boolean.
    if(scope.bank->active_owned_skin_operation_==this)scope.bank->active_owned_skin_operation_=nullptr;
    else scope.bank->poison();
}
BnwNativeGlobalColorRegistryBank::OwnedCallScope::OwnedCallScope(std::shared_ptr<BnwNativeGlobalColorRegistryBank> actual)
    :bank(std::move(actual)) {
    bank_owned_need(bank&&bank->thread_==std::this_thread::get_id(),"Owned Bank call-scope foreign creator thread");
    if(bank->active_owned_skin_operation_) {
        bank->require_owned_operation_loan_current();borrowed=bank->active_owned_skin_operation_;
    }else issued=std::make_unique<Scope>(bank);
}
void BnwNativeGlobalColorRegistryBank::require_owned_operation_loan_current()const {
    bank_owned_need(thread_==std::this_thread::get_id(),"Owned Bank operation loan foreign thread");
    const auto* actual=active_owned_skin_operation_;
    bank_owned_need(actual&&actual->scope.bank.get()==this&&busy_.load()&&!closed_.load()&&phase_!=Phase::failed,
                    "Owned Bank call lacks SAME actual owning stack Scope loan");
}
}

#include "bnw_native_owned_skin_boot_policy.hpp"
#include "bnw_native_owned_skin_global_selection_kernel.hpp"
#include "bnw_native_completed_resource_bank_capability.hpp"
#include "bnw_native_package_resource_checkpoint.hpp"
#include "bnw_native_standalone_package_loaded.hpp"
#include <bit>
#include <limits>
#include <type_traits>
#include "bnw_native_completed_skin_instance_observation.hpp"
namespace civ5::ui::detail {
std::optional<BnwNativeGlobalColorRegistryBank::Bits>
BnwNativeGlobalColorRegistryBank::ordered_completed_lookup(std::uint32_t key)const {
    CompletedCallScope scope(std::const_pointer_cast<BnwNativeGlobalColorRegistryBank>(shared_from_this()));
    try {
        require_owned_current();
        bank_owned_need(key!=0x8f4455c8U&&key!=0x164d0472U,
            "Completed normal Color lookup lacks actual special player-interface supplier");
        const auto read=[&](const std::shared_ptr<BnwNativeColorSlotStorage>& field)->std::optional<Bits> {
            bank_owned_need(bool(field),"Completed Color actual constructor field absent");
            const auto found=field->colors_.find(key);
            if(found==field->colors_.end())return std::nullopt;
            bank_owned_need(std::all_of(found->second.known.begin(),found->second.known.end(),[](bool known){return known;}),
                "Completed Color selected hit contains unknown native allocator bytes");
            return found->second.bits;
        };
        std::optional<Bits> result;
        // The actual Parent issuer, not a caller vector, supplies the complete
        // ordered roster under this SAME fresh Header/Loaded/Bank operation.
        scope.borrowed->parent_pin->parent->with_native_owned_completed_skin_instance_observations(
            [&](const auto& roster){
                require_owned_current();
                for(auto position=roster.rbegin();position!=roster.rend();++position){
                    const auto observation=*position;
                    bank_owned_need(bool(observation),"Completed Color genuine roster observation absent");
                    observation->require_current();require_owned_current();
                    observation->with_actual_instance([&](const auto& instance){
                        require_owned_current();
                        // Real returned instance bytes and Color fields only.
                        // Do not invoke its retired constructor/Skin guard.
                        for(unsigned slot=0;slot<2;++slot)if(instance.enabled_[slot]){
                            result=read(instance.color_storage_[slot]);
                            require_owned_current();if(result)break;
                        }
                    });
                    observation->require_current();require_owned_current();
                    if(result)break;
                }
                require_owned_current();
            });
        require_owned_current(); // Includes the Parent copied target destructor.
        if(!result)result=read(global_);
        require_owned_current();return result;
    }catch(...){scope.bank->poison();throw;}
}
}


namespace civ5::ui::detail {
BnwNativeGlobalColorRegistryBank::CompletedResourceOperation::CompletedResourceOperation(
    std::shared_ptr<BnwNativeGlobalColorRegistryBank> actual,
    std::shared_ptr<const BnwNativeCompletedResourceBankCapability> capability)
    :scope([&]{
        bank_owned_need(actual&&actual->thread_==std::this_thread::get_id(),
            "Completed Bank operation foreign creator thread");
        bank_owned_need(capability&&!actual->active_owned_skin_operation_&&!actual->active_completed_resource_operation_,
            "Completed Bank operation source absent/replayed");
        return actual;
    }()),owner(std::move(capability)) {
    auto& bank=*scope.bank;bool began=false;
    try {
        bank.require_owned_numeric_context();
        bank_owned_need(bank.phase_==Phase::initial_finalized&&bank.owned_source_,
            "Completed Bank operation lacks actual finalized receiver");
        parent_pin=std::make_unique<PackagePin>(bank);
        bank.active_completed_resource_operation_=this;
        parent_pin->parent->begin_native_owned_completed_color_bank_operation(bank,*owner,this);
        began=true;bank.require_completed_operation_loan_current();bank.require_owned_numeric_context();
    }catch(...){
        if(began)parent_pin->parent->end_native_owned_completed_color_bank_operation(bank,*owner,this);
        if(bank.active_completed_resource_operation_==this)bank.active_completed_resource_operation_=nullptr;
        bank.poison();throw;
    }
}
BnwNativeGlobalColorRegistryBank::CompletedResourceOperation::~CompletedResourceOperation() {
    // This private nonmovable stack object cannot migrate with its Scope/pin.
    if(thread!=std::this_thread::get_id())std::terminate();
    auto& bank=*scope.bank;
    if(parent_pin&&owner)
        parent_pin->parent->end_native_owned_completed_color_bank_operation(bank,*owner,this);
    else bank.poison();
    if(bank.active_completed_resource_operation_==this)bank.active_completed_resource_operation_=nullptr;
    else bank.poison();
    // Original tracked PackagePin member drops before the original Scope.
    // Cleanup creates no new throwing pin and invokes no initial Skin source.
}
BnwNativeGlobalColorRegistryBank::CompletedCallScope::CompletedCallScope(
    std::shared_ptr<BnwNativeGlobalColorRegistryBank> actual):bank(std::move(actual)) {
    bank_owned_need(bank&&bank->thread_==std::this_thread::get_id(),
        "Completed Bank call-scope foreign creator thread");
    bank->require_completed_operation_loan_current();borrowed=bank->active_completed_resource_operation_;
    bank_owned_need(borrowed&&borrowed->scope.bank.get()==bank.get(),
        "Completed Bank call-scope lost original new Scope");
}
void BnwNativeGlobalColorRegistryBank::require_completed_operation_loan_current(
    const content::XmlNode* document,std::uint64_t occurrence)const {
    bank_owned_need(thread_==std::this_thread::get_id(),"Completed Bank affinity foreign creator thread");
    const auto* operation=active_completed_resource_operation_;
    bank_owned_need(operation&&operation->thread==thread_&&operation->scope.bank.get()==this&&
        operation->owner&&operation->parent_pin&&operation->parent_pin->parent&&busy_.load()&&
        !closed_.load()&&phase_==Phase::initial_finalized&&!active_owned_skin_operation_,
        "Completed Bank query lacks actual new original Scope/Parent pin/capability");
    // Pure new Header/Loaded/roster/parser affinity. Never Source9/17 or
    // retired Consumer/Kernel/constructor currentness callbacks.
    operation->parent_pin->parent->require_native_owned_completed_color_bank_current(
        *this,*operation->owner,operation,document,occurrence);
}
}

namespace civ5::ui {
class BnwNativePackageDiscoveryOwner::OwnedSkinEffectsSource final
    : public detail::BnwNativeOwnedSkinConsumer::Source,
      public std::enable_shared_from_this<OwnedSkinEffectsSource> {
    friend class OwnedSkinEffectsOccurrence;
    friend class OwnedSkinGlobalSelectionSource;
    friend class OwnedSkinGlobalEffectsLoan;
    using Bank=detail::BnwNativeGlobalColorRegistryBank;
    using Consumer=detail::BnwNativeOwnedSkinConsumer;
    using Policy=detail::BnwNativeOwnedSkinBootPolicy;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::weak_ptr<Bank> bank_;
    const std::shared_ptr<Policy> policy_;
    std::shared_ptr<Bank> actual_bank()const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned Skin effects foreign creator thread");
        const auto bank=bank_.lock();
        if(!bank)throw std::runtime_error("Owned Skin actual effects Bank retired");
        return bank;
    }
    template<class Function> auto with_parent(Function&& operation)const {
        const auto storage=shared_from_this();const auto bank=actual_bank();
        Bank::PackagePin retained(*bank);
        const auto check=[&]{
            bank->require_owned_numeric_context();
            retained.parent->require_native_owned_color_bank_current(*bank,nullptr,0);
            retained.parent->require_native_owned_skin_effect_source_current(*policy_,static_cast<const Consumer::Source*>(this));
            policy_->require_current();bank->require_owned_numeric_context();
        };
        check();
        using Result=std::invoke_result_t<Function,const BnwNativePackageDiscoveryOwner&,Bank&>;
        static_assert(!std::is_reference_v<Result>);
        if constexpr(std::is_void_v<Result>){operation(*retained.parent,*bank);check();}
        else {auto result=operation(*retained.parent,*bank);check();return result;}
    }
  public:
    OwnedSkinEffectsSource(const std::shared_ptr<Bank>& bank,const std::shared_ptr<Policy>& policy)
        :bank_(bank),policy_(policy) {
        if(!bank||!policy)throw std::runtime_error("Owned Skin effects actual inputs absent");
    }
    void require_current()const override {with_parent([](auto&,auto&){});}
    void with_actual_roster_660330(const std::function<void(Consumer::Occurrence&)>&)override;
    void reconcile_actual_colors_462c20()override {
        with_parent([](auto&,auto& bank){bank.reconcile_owned_colors_462c20();});
    }
    void invalidate_actual_operation()noexcept override {
        if(thread_!=std::this_thread::get_id())return;
        policy_->invalidate();
        if(const auto bank=bank_.lock()) {
            if(bank->owned_source_)bank->owned_source_->invalidate_actual_operation();
            bank->poison();
        }
    }
};
// E2b2 identity/currentness adapter only. Genuine installed global frame is
// still required by the distinct default-refusing Parent/continuation join.
class BnwNativePackageDiscoveryOwner::OwnedSkinGlobalEffectsLoan final
    : public std::enable_shared_from_this<OwnedSkinGlobalEffectsLoan> {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class OwnedInstalledSkinGlobalSelectionOccurrence;
    friend class OwnedInstalledSkinGlobalSelectionSource;
    using Consumer=detail::BnwNativeOwnedSkinConsumer;
    using Policy=detail::BnwNativeOwnedSkinBootPolicy;
    using Bank=detail::BnwNativeGlobalColorRegistryBank;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::weak_ptr<OwnedSkinEffectsSource> effects_;
    const std::weak_ptr<Consumer> consumer_;
    const std::weak_ptr<Policy> policy_;
    const std::uint64_t serial_;
    OwnedSkinGlobalEffectsLoan(const std::shared_ptr<OwnedSkinEffectsSource>& effects,
        const std::shared_ptr<Consumer>& consumer,const std::shared_ptr<Policy>& policy,std::uint64_t serial)
        :effects_(effects),consumer_(consumer),policy_(policy),serial_(serial) {}
    static void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
  public:
    ~OwnedSkinGlobalEffectsLoan()=default; // Weak controls only; no callback during destruction.
    OwnedSkinGlobalEffectsLoan(const OwnedSkinGlobalEffectsLoan&)=delete;
    template<class Function> auto with_parent(Function&& operation)const {
        need(thread_==std::this_thread::get_id(),"Installed global effects loan foreign creator");
        const auto storage=shared_from_this();
        const auto effects=effects_.lock();const auto consumer=consumer_.lock();const auto policy=policy_.lock();
        need(effects&&consumer&&policy,"Installed global effects loan original controls retired");
        // Complete concrete type: real C++ base conversion AND control join.
        const std::shared_ptr<Consumer::Source> base=effects;
        need(base.get()==consumer->source_.get()&&!base.owner_before(consumer->source_)&&
            !consumer->source_.owner_before(base)&&effects->policy_.get()==policy.get()&&
            !effects->policy_.owner_before(policy)&&!policy.owner_before(effects->policy_),
            "Installed global effects loan lost exact typed Source/control/policy");
        const auto bank=effects->actual_bank();
        Bank::PackagePin retained(*bank); // Existing counted transient Parent pin, never stored.
        const auto check=[&]{retained.parent->require_installed_global_effects_join(
            *consumer,*policy,base.get(),InstalledGlobalBorrowCall::borrow,serial_);};
        check(); // Real NEW global affinity before any old Source currentness callback.
        using Result=std::invoke_result_t<Function,const BnwNativePackageDiscoveryOwner&,Bank&>;
        static_assert(!std::is_reference_v<Result>);
        if constexpr(std::is_void_v<Result>) {
            effects->with_parent([&](auto& parent,auto& actual_bank){
                need(&parent==retained.parent.get()&& &actual_bank==bank.get(),"Installed global loan substituted Parent/Bank");
                check();operation(parent,actual_bank);check();
            });check();
        }else {
            auto result=effects->with_parent([&](auto& parent,auto& actual_bank){
                need(&parent==retained.parent.get()&& &actual_bank==bank.get(),"Installed global loan substituted Parent/Bank");
                check();auto value=operation(parent,actual_bank);check();return value;
            });check();return result;
        }
    }
    void require_current()const {with_parent([](auto&,auto&){});}
    void with_actual_roster_660330(const std::function<void(Consumer::Occurrence&)>& input)const {
        with_parent([&](auto&,auto&){
            {
                const auto consume=input;require_current();
                need(bool(consume),"Installed global effects roster target absent");
                const auto effects=effects_.lock();need(bool(effects),"Installed global actual Source retired");
                effects->with_actual_roster_660330([&](Consumer::Occurrence& occurrence){
                    require_current();occurrence.require_current();consume(occurrence);
                    occurrence.require_current();require_current();
                });require_current();
            }
            require_current(); // Copied target destroyed before actual borrow POST.
        });
    }
};
std::shared_ptr<BnwNativePackageDiscoveryOwner::OwnedSkinGlobalEffectsLoan>
BnwNativePackageDiscoveryOwner::make_installed_global_effects_loan(
    const std::weak_ptr<OwnedSkinEffectsSource>& original,
    const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>& consumer,
    const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>& policy)const {
    using Consumer=detail::BnwNativeOwnedSkinConsumer;
    if(!consumer||!policy||consumer->thread_!=std::this_thread::get_id()||policy->thread_!=std::this_thread::get_id())
        throw std::runtime_error("Installed global typed factory foreign/absent canonical inputs");
    const auto effects=original.lock();
    if(!effects)throw std::runtime_error("Installed global typed factory original Source expired");
    const std::shared_ptr<Consumer::Source> base=effects; // Concrete Source is complete HERE.
    const auto actual_consumer=consumer->weak_from_this();const auto actual_policy=policy->weak_from_this();
    if(base.get()!=consumer->source_.get()||base.owner_before(consumer->source_)||consumer->source_.owner_before(base)||
       consumer.owner_before(actual_consumer)||actual_consumer.owner_before(consumer)||
       policy.owner_before(actual_policy)||actual_policy.owner_before(policy))
        throw std::runtime_error("Installed global typed factory pointer/control affiliation mismatch");
    const auto serial=require_installed_global_effects_join(*consumer,*policy,base.get(),InstalledGlobalBorrowCall::factory,0);
    const auto loan=std::shared_ptr<OwnedSkinGlobalEffectsLoan>(new OwnedSkinGlobalEffectsLoan(effects,consumer,policy,serial));
    loan->require_current();
    require_installed_global_effects_join(*consumer,*policy,base.get(),InstalledGlobalBorrowCall::factory,serial);
    return loan; // Weak original controls only; no global/resource normal-return seal.
}
class BnwNativePackageDiscoveryOwner::OwnedSkinEffectsOccurrence final
    : public detail::BnwNativeOwnedSkinConsumer::Occurrence {
    using Consumer=detail::BnwNativeOwnedSkinConsumer;
    using Query=Consumer::Query;
    using Result=Consumer::NativeLoadResult;
    using Instance=detail::BnwNativeOwnedUiSkinPoststageInstance;
    using Token=detail::BnwNativeOwnedSkinOccurrence;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<OwnedSkinEffectsSource> source_;
    const std::shared_ptr<const Instance> instance_;
    const std::shared_ptr<const Token> token_;
    struct IssuedVector {std::vector<std::string> values;std::size_t next=0;unsigned slot=0;bool issued=false;};
    std::array<IssuedVector,5> outputs_;
    static const char* property(Query query) {
        switch(query){case Query::directory:return "Directory";case Query::paks:return "Paks";
        case Query::file:return "File";case Query::style:return "Style";case Query::color:return "Color";}
        throw std::runtime_error("Owned Skin actual query property unavailable");
    }
    template<class Function> auto with_parent(Function&& operation)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned Skin effect occurrence foreign thread");
        const auto source=source_;const auto instance=instance_;const auto token=token_;
        return source->with_parent([&](auto& parent,auto& bank){
            instance->require_current();token->require_current();
            if(&instance->constructor_source_->actual_occurrence()!=token.get())
                throw std::runtime_error("Owned Skin effects instance has another actual token");
            using Return=std::invoke_result_t<Function,const BnwNativePackageDiscoveryOwner&,detail::BnwNativeGlobalColorRegistryBank&>;
            if constexpr(std::is_void_v<Return>){operation(parent,bank);instance->require_current();token->require_current();}
            else {auto result=operation(parent,bank);instance->require_current();token->require_current();return result;}
        });
    }
    std::size_t actual_next(unsigned slot,Query query,const std::string& path) {
        require_current();auto& output=outputs_.at(static_cast<std::size_t>(query));
        if(!output.issued||output.slot!=slot||output.next>=output.values.size()||output.values[output.next]!=path)
            throw std::runtime_error("Owned Skin effect is not next genuine query vector occurrence");
        return output.next;
    }
    void begin_selected(unsigned slot,Query query) {
        with_parent([&](auto& parent,auto& bank){
            const auto& output=outputs_.at(static_cast<std::size_t>(query));
            if(!output.issued||output.slot!=slot||output.values.empty()||output.next)
                throw std::runtime_error("Owned Skin selected begin is not authentic nonempty query");
            if(query==Query::style)bank.begin_owned_style_loading(instance_,slot);
            parent.begin_native_owned_skin_selected_read(token_,slot,property(query));
        });
    }
    Result try_selected(unsigned slot,Query query,const std::string& path) {
        const auto ordinal=actual_next(slot,query,path);
        const bool loaded=with_parent([&](auto& parent,auto& bank){
            const auto input=parent.read_native_owned_skin_selected_input(token_,slot,property(query),ordinal);
            if(!input)return false; // Genuine open failure only; next emitted filename.
            input->require_same_occurrence(*token_,slot,property(query));
            const auto result=query==Query::style?bank.try_load_owned_style_8348c0(instance_,slot,input):
                bank.try_load_owned_color_601ca0(instance_,slot,input);
            input->require_same_occurrence(*token_,slot,property(query));return result;
        });
        ++outputs_.at(static_cast<std::size_t>(query)).next;require_current();
        // Style result is actual8348C0; AL834B80 does not control slot break.
        return loaded?Result::native_true:Result::native_false;
    }
  public:
    OwnedSkinEffectsOccurrence(std::shared_ptr<OwnedSkinEffectsSource> source,
        std::shared_ptr<const Instance> instance,std::shared_ptr<const Token> token)
        :source_(std::move(source)),instance_(std::move(instance)),token_(std::move(token)) {
        if(!source_||!instance_||!token_)throw std::runtime_error("Owned Skin occurrence actual anchors absent");
        require_current();
    }
    void require_current()const override {with_parent([](auto&,auto&){});}
    bool matches_actual_selection_64f970()const override {
        return with_parent([&](auto& parent,auto&){
            bool matches=false;std::size_t callbacks=0;
            parent.with_native_owned_skin_extra_attributes(instance_,[&](const auto& actual){
                if(callbacks++)throw std::runtime_error("Owned Skin actual attribute loan repeated");
                matches=source_->policy_->matches_actual_attributes_64f970(actual);
            });
            if(callbacks!=1)throw std::runtime_error("Owned Skin actual attribute loan omitted");
            return matches;
        });
    }
    bool first_pair_source_present_48d2b0(unsigned slot)const override {
        return with_parent([&](auto& parent,auto&){return parent.native_owned_skin_first_pair_present(instance_,slot);});
    }
    unsigned char first_pair_source_enabled_47d680(unsigned slot)const override {
        return with_parent([&](auto& parent,auto&){return parent.native_owned_skin_first_pair_enabled(instance_,slot);});
    }
    std::vector<std::string> query_owned_54b8d0(unsigned slot,Query query,std::uint32_t filter)override {
        require_current();const auto expected=query==Query::directory||query==Query::paks?0u:0x80u;
        if(filter!=expected||slot>=2)throw std::runtime_error("Owned Skin genuine query filter/slot changed");
        const auto result=with_parent([&](auto& parent,auto&){return parent.query_native_owned_skin_outputs(token_,slot,property(query),filter);});
        auto& output=outputs_.at(static_cast<std::size_t>(query));output={result,0,slot,true};require_current();return result;
    }
    void load_actual_paks_6f84f0(unsigned slot,const std::vector<std::string>& paths)override {
        require_current();const auto& output=outputs_.at(static_cast<std::size_t>(Query::paks));
        if(!output.issued||output.slot!=slot||output.values!=paths)throw std::runtime_error("Owned Paks did not consume actual emitted vector");
        with_parent([&](auto& parent,auto&){parent.load_native_owned_skin_paks(instance_,slot);});
    }
    void register_actual_directory_419930_82a470(unsigned slot,const std::string& path,unsigned mode,const char* patterns)override {
        const auto ordinal=actual_next(slot,Query::directory,path);
        if(mode!=7||!patterns||std::string_view(patterns)!="*.xml;*.ggxml")throw std::runtime_error("Owned Skin Directory native parameters changed");
        with_parent([&](auto& parent,auto&){parent.register_native_owned_skin_directory(instance_,slot,ordinal);});
        ++outputs_.at(static_cast<std::size_t>(Query::directory)).next;require_current();
    }
    void register_actual_file_419930_82a7a0(unsigned slot,const std::string& path,unsigned mode)override {
        const auto ordinal=actual_next(slot,Query::file,path);
        if(mode!=7)throw std::runtime_error("Owned Skin File native mode changed");
        with_parent([&](auto& parent,auto&){parent.register_native_owned_skin_file(instance_,slot,ordinal);});
        ++outputs_.at(static_cast<std::size_t>(Query::file)).next;require_current();
    }
    void begin_actual_style_loading(unsigned slot)override {begin_selected(slot,Query::style);}
    Result try_actual_style_load_8348c0_79f460_834b80(unsigned slot,const std::string& path)override {return try_selected(slot,Query::style,path);}
    void end_actual_style_loading(unsigned slot)override {with_parent([&](auto& parent,auto&){parent.end_native_owned_skin_selected_read(token_,slot,"Style");});}
    void begin_actual_color_loading(unsigned slot)override {begin_selected(slot,Query::color);}
    Result try_actual_color_load_601ca0(unsigned slot,const std::string& path)override {return try_selected(slot,Query::color,path);}
    void end_actual_color_loading(unsigned slot)override {with_parent([&](auto& parent,auto&){parent.end_native_owned_skin_selected_read(token_,slot,"Color");});}
    void publish_actual_name_5d5550(unsigned slot)override {with_parent([&](auto& parent,auto&){parent.publish_native_owned_skin_name(instance_,slot);});}
    void enable_actual_pair_767410(unsigned slot)override {with_parent([&](auto& parent,auto&){parent.enable_native_owned_skin_pair(instance_,slot);});}
    void write_actual_instance_enabled_byte(unsigned slot)override {with_parent([&](auto& parent,auto&){parent.write_native_owned_skin_instance_enabled(instance_,slot);});}
};
void BnwNativePackageDiscoveryOwner::OwnedSkinEffectsSource::with_actual_roster_660330(
    const std::function<void(Consumer::Occurrence&)>& input) {
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned Skin roster foreign creator thread");
    const auto storage=shared_from_this();require_current();
    {
    const auto consume=input;require_current();if(!consume)throw std::runtime_error("Owned Skin kernel roster consumer absent");
    with_parent([&](auto& parent,auto&){
        parent.with_native_owned_skin_constructed_roster([&](const auto& roster){
            for(const auto& entry:roster) {
                const auto instance=entry;std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence> token;
                parent.with_native_owned_skin_occurrences([&](const auto& actual){
                    if(&instance->constructor_source_->actual_occurrence()==actual.get()) {
                        if(token)throw std::runtime_error("Owned Skin instance has duplicate genuine token");token=actual;
                    }
                });
                if(!token)throw std::runtime_error("Owned Skin instance lacks actual returned occurrence");
                OwnedSkinEffectsOccurrence occurrence(storage,instance,token);
                occurrence.require_current();consume(occurrence);occurrence.require_current();
            }
        });
    });
    }
    require_current(); // After copied callable target destruction.
}
class BnwNativePackageDiscoveryOwner::OwnedSkinGlobalSelectionSource final
    : public detail::BnwNativeOwnedSkinGlobalSelectionKernel::Source,
      public std::enable_shared_from_this<OwnedSkinGlobalSelectionSource> {
    friend class OwnedSkinGlobalSelectionOccurrence;
    friend class OwnedSkinGlobalSelectionRecord;
    using Kernel=detail::BnwNativeOwnedSkinGlobalSelectionKernel;
    using Consumer=detail::BnwNativeOwnedSkinConsumer;
    using Bank=detail::BnwNativeGlobalColorRegistryBank;
    using Policy=detail::BnwNativeOwnedSkinBootPolicy;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<OwnedSkinEffectsSource> effects_;
    const std::shared_ptr<Consumer> consumer_;
    const std::shared_ptr<Policy> policy_;
    std::optional<unsigned> actual_slot_;
    const std::vector<std::size_t>* actual_indices_=nullptr;
    template<class Function> auto with_parent(Function&& operation)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global Skin Source foreign thread");
        const auto storage=shared_from_this();const auto effects=effects_;const auto consumer=consumer_;const auto policy=policy_;
        return effects->with_parent([&](auto& parent,auto& bank){
            const auto check=[&]{parent.require_native_owned_skin_global_source_current(*policy,consumer,
                static_cast<const Kernel::Source*>(this));};
            check();
            using Result=std::invoke_result_t<Function,const BnwNativePackageDiscoveryOwner&,Bank&>;
            static_assert(!std::is_reference_v<Result>);
            if constexpr(std::is_void_v<Result>){operation(parent,bank);check();}
            else {auto result=operation(parent,bank);check();return result;}
        });
    }
  public:
    OwnedSkinGlobalSelectionSource(std::shared_ptr<OwnedSkinEffectsSource> effects,
        std::shared_ptr<Consumer> consumer,std::shared_ptr<Policy> policy)
        :effects_(std::move(effects)),consumer_(std::move(consumer)),policy_(std::move(policy)) {
        if(!effects_||!consumer_||!policy_)throw std::runtime_error("Owned global actual inputs absent");
    }
    void require_current()const override {with_parent([](auto&,auto&){});}
    std::string actual_published_platform_6415f0()const override {
        return with_parent([&](auto&,auto&){policy_->require_returned();return policy_->actual_platform_;});
    }
    bool actual_is_basegame_stricmp(const std::string& actual)const override {
        return with_parent([&](auto&,auto&){return policy_->actual_is_basegame_stricmp(actual);});
    }
    void with_actual_slot_scope(unsigned slot,
        const std::function<void(std::vector<std::size_t>&)>& input)override {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global slot foreign creator thread");
        if(policy_->installed_source_)throw std::runtime_error("Saved global Source cannot supply installed slot owners");
        if(slot>1||actual_slot_||actual_indices_) {
            invalidate_actual_operation();throw std::runtime_error("Owned global slot scope replay/overlap");
        }
        actual_slot_=slot;
        struct Exit {OwnedSkinGlobalSelectionSource& owner;~Exit(){owner.actual_indices_=nullptr;owner.actual_slot_.reset();}} exit{*this};
        const auto storage=shared_from_this();
        try {
            require_current();
            {
                std::vector<std::size_t> indices;
                actual_indices_=&indices;
                struct IndicesExit {OwnedSkinGlobalSelectionSource& owner;~IndicesExit(){owner.actual_indices_=nullptr;}} indices_exit{*this};
                {
                    const auto consume=input;require_current();
                    if(!consume)throw std::runtime_error("Owned global actual slot body absent");
                    consume(indices);require_current();
                }
                require_current();
            }
            require_current(); // copied target AND actual per-slot list destroyed before POST
        }catch(...){invalidate_actual_operation();throw;}
    }
    void with_actual_roster(const std::function<void(Kernel::Occurrence&)>&)override;
    void append_actual_matching_indices_6fa850(const std::string& set,const std::string& platform,
        bool reset,std::vector<std::size_t>& indices)override {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global index foreign thread");
        if(!actual_slot_||actual_indices_!=&indices){invalidate_actual_operation();
            throw std::runtime_error("Owned global substituted index list/scope");}
        with_parent([&](auto&,auto&){
            policy_->begin_global_constraint(set,platform);
            if(reset)indices.clear();
            std::size_t index=0;
            effects_->with_actual_roster_660330([&](Consumer::Occurrence& occurrence){
                require_current();occurrence.require_current();
                const bool matches=occurrence.matches_actual_selection_64f970();
                occurrence.require_current();require_current();
                if(matches)indices.push_back(index);
                if(index==std::numeric_limits<std::size_t>::max())throw std::runtime_error("Owned global dense index overflow");
                ++index;
            });
            require_current();policy_->end_global_constraint();policy_->require_global_constraints_idle();
        });
    }
    bool select_and_load_actual_660330(const std::string& set,const std::string& platform,int slot)override {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global selector foreign thread");
        if(!actual_slot_||static_cast<int>(*actual_slot_)!=slot||!actual_indices_){invalidate_actual_operation();
            throw std::runtime_error("Owned global selected660 outside same actual slot");}
        return with_parent([&](auto&,auto&){
            policy_->begin_global_constraint(set,platform);
            const bool result=consumer_->select_and_load_660330(slot);
            consumer_->require_current();require_current();
            policy_->end_global_constraint();policy_->require_global_constraints_idle();return result;
        });
    }
    void reconcile_actual_bank_462c20()override {
        with_parent([&](auto&,auto&){effects_->reconcile_actual_colors_462c20();});
    }
    void invalidate_actual_operation()noexcept override {
        if(thread_==std::this_thread::get_id())effects_->invalidate_actual_operation();
    }
};
class BnwNativePackageDiscoveryOwner::OwnedSkinGlobalSelectionRecord final
    : public detail::BnwNativeOwnedSkinGlobalSelectionKernel::Record {
    using Loaded=detail::BnwNativeStandalonePackageLoaded;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const OwnedSkinGlobalSelectionSource> source_;
    const std::shared_ptr<const Loaded> package_;
    template<class Function> auto with_descriptor(Function&& operation)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global Core record foreign thread");
        const auto source=source_;const auto package=package_;
        return source->with_parent([&](auto& parent,auto&){
            std::size_t found=0;
            parent.with_native_owned_returned_package_roster([&](const auto& roster){
                for(const auto& actual:roster)if(actual.get()==package.get())++found;
            });
            if(found!=1)throw std::runtime_error("Owned global Core record lost actual returned table membership");
            using Result=std::invoke_result_t<Function,const Loaded&>;
            static_assert(!std::is_reference_v<Result>);
            if constexpr(std::is_void_v<Result>){operation(*package);source->require_current();}
            else {auto result=operation(*package);source->require_current();return result;}
        });
    }
  public:
    OwnedSkinGlobalSelectionRecord(std::shared_ptr<const OwnedSkinGlobalSelectionSource> source,
        std::shared_ptr<const Loaded> package):source_(std::move(source)),package_(std::move(package)) {
        if(!source_||!package_)throw std::runtime_error("Owned global Core record actual anchors absent");require_current();
    }
    void require_current()const override {
        with_descriptor([](const auto& package){package.with_actual_descriptor([](const auto& loan){loan.require_current();});});
    }
    std::int32_t actual_priority_4c()const override {
        return with_descriptor([](const auto& package){std::int32_t result=0;
            package.with_actual_descriptor([&](const auto& loan){loan.require_current();result=loan.fields().priority;loan.require_current();});return result;});
    }
    std::uint32_t actual_current_mask_48()const override {
        return with_descriptor([](const auto& package){std::uint32_t result=0;
            package.with_actual_descriptor([&](const auto& loan){loan.require_current();result=loan.fields().current_category_mask();loan.require_current();});return result;});
    }
    std::int32_t actual_forced_gameplay_count_762400_1_1()const override {
        return with_descriptor([](const auto& package){return std::bit_cast<std::int32_t>(
            detail::BnwNativePackageResourceCheckpoint::forced_gameplay_count_from_loaded_762400_1_1(package));});
    }
};
class BnwNativePackageDiscoveryOwner::OwnedSkinGlobalSelectionOccurrence final
    : public detail::BnwNativeOwnedSkinGlobalSelectionKernel::Occurrence {
    using Kernel=detail::BnwNativeOwnedSkinGlobalSelectionKernel;
    using Instance=detail::BnwNativeOwnedUiSkinPoststageInstance;
    using Token=detail::BnwNativeOwnedSkinOccurrence;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<OwnedSkinGlobalSelectionSource> source_;
    const std::shared_ptr<const Instance> instance_;
    const std::shared_ptr<const Token> token_;
    template<class Function> auto with_parent(Function&& operation)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global Skin occurrence foreign thread");
        const auto source=source_;const auto instance=instance_;const auto token=token_;
        return source->with_parent([&](auto& parent,auto& bank){
            const auto check=[&]{instance->require_current();token->require_current();
                if(&instance->constructor_source_->actual_occurrence()!=token.get())
                    throw std::runtime_error("Owned global occurrence substituted actual constructor token");};
            check();using Result=std::invoke_result_t<Function,const BnwNativePackageDiscoveryOwner&,detail::BnwNativeGlobalColorRegistryBank&>;
            static_assert(!std::is_reference_v<Result>);
            if constexpr(std::is_void_v<Result>){operation(parent,bank);check();}
            else {auto result=operation(parent,bank);check();return result;}
        });
    }
  public:
    OwnedSkinGlobalSelectionOccurrence(std::shared_ptr<OwnedSkinGlobalSelectionSource> source,
        std::shared_ptr<const Instance> instance,std::shared_ptr<const Token> token)
        :source_(std::move(source)),instance_(std::move(instance)),token_(std::move(token)) {
        if(!source_||!instance_||!token_)throw std::runtime_error("Owned global occurrence missing actual anchors");require_current();
    }
    void require_current()const override {with_parent([](auto&,auto&){});}
    bool actual_constructor_available_26(unsigned slot)const override {
        if(slot>=2)throw std::runtime_error("Owned global constructor slot invalid");
        return with_parent([&](auto&,auto&){return instance_->availability_[slot];});
    }
    bool actual_instance_enabled_24(unsigned slot)const override {
        if(slot>=2)throw std::runtime_error("Owned global enabled slot invalid");
        return with_parent([&](auto&,auto&){return instance_->enabled_[slot]!=0;});
    }
    void disable_actual_pair_767410_then_instance(unsigned slot)override {
        with_parent([&](auto& parent,auto&){parent.disable_native_owned_skin_pair_then_instance(instance_,slot);});
    }
    std::optional<std::string> first_actual_set_6d0090()const override {
        return with_parent([&](auto& parent,auto&){
            std::optional<std::string> result;std::size_t emitted=0;
            parent.with_native_owned_skin_extra_attributes(instance_,[&](const auto& actual){
                if(emitted++)throw std::runtime_error("Owned global extra attribute loan replay");
                result=source_->policy_->first_actual_attribute_6d0090(actual,"set");
            });
            if(emitted!=1)throw std::runtime_error("Owned global extra attribute loan absent");return result;
        });
    }
    void with_first_actual_record_4138d0_605080(const std::function<void(const Kernel::Record&)>& input)const override {
        with_parent([&](auto& parent,auto&){
            {
                const auto consume=input;require_current();
                if(!consume)throw std::runtime_error("Owned global real Core consumer absent");
                const auto actual_package=token_->package_.lock();
                if(!actual_package)throw std::runtime_error("Owned global instance header Package retired");
                std::optional<std::array<std::uint8_t,16>> guid;
                actual_package->with_actual_descriptor([&](const auto& loan){loan.require_current();guid=loan.fields().written_guid;loan.require_current();});
                if(!guid)throw std::runtime_error("Owned global actual instance header GUID is unwritten");
                std::shared_ptr<const detail::BnwNativeStandalonePackageLoaded> first;
                parent.with_native_owned_returned_package_roster([&](const auto& roster){
                    for(const auto& entry:roster){
                        if(!entry)throw std::runtime_error("Owned global returned Core table contains null");
                        bool matches=false;
                        entry->with_actual_descriptor([&](const auto& loan){
                            loan.require_current();const auto& fields=loan.fields();
                            if((fields.current_flags()&4U)&&!(fields.current_flags()&64U)){
                                if(!fields.written_guid)throw std::runtime_error("Owned global eligible Core GUID is unwritten");
                                matches=*fields.written_guid==*guid;
                            }
                            loan.require_current();
                        });
                        if(matches){first=entry;break;}
                    }
                });
                if(first){OwnedSkinGlobalSelectionRecord record(source_,first);record.require_current();consume(record);record.require_current();}
            }
            require_current(); // Includes destruction of the copied callable target.
        });
    }
};
// E2 real private installed Record/Occurrence adapters. No Source/frame/return issuer yet.
class BnwNativePackageDiscoveryOwner::OwnedInstalledSkinGlobalSelectionRecord final
    : public detail::BnwNativeOwnedSkinGlobalSelectionKernel::Record {
    using Loaded=detail::BnwNativeStandalonePackageLoaded;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const OwnedSkinGlobalEffectsLoan> source_;
    const std::shared_ptr<const Loaded> package_;
    template<class Function> auto with_descriptor(Function&& operation)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global Core record foreign thread");
        const auto source=source_;const auto package=package_;
        return source->with_parent([&](auto& parent,auto&){
            std::size_t found=0;
            parent.with_native_owned_returned_package_roster([&](const auto& roster){
                for(const auto& actual:roster)if(actual.get()==package.get())++found;
            });
            if(found!=1)throw std::runtime_error("Owned global Core record lost actual returned table membership");
            using Result=std::invoke_result_t<Function,const Loaded&>;
            static_assert(!std::is_reference_v<Result>);
            if constexpr(std::is_void_v<Result>){operation(*package);source->require_current();}
            else {auto result=operation(*package);source->require_current();return result;}
        });
    }
    friend class OwnedInstalledSkinGlobalSelectionSource;
    friend class OwnedInstalledSkinGlobalSelectionOccurrence;
    OwnedInstalledSkinGlobalSelectionRecord(std::shared_ptr<const OwnedSkinGlobalEffectsLoan> source,
        std::shared_ptr<const Loaded> package):source_(std::move(source)),package_(std::move(package)) {
        if(!source_||!package_)throw std::runtime_error("Owned global Core record actual anchors absent");require_current();
    }
  public:
    void require_current()const override {
        with_descriptor([](const auto& package){package.with_actual_descriptor([](const auto& loan){loan.require_current();});});
    }
    std::int32_t actual_priority_4c()const override {
        return with_descriptor([](const auto& package){std::int32_t result=0;
            package.with_actual_descriptor([&](const auto& loan){loan.require_current();result=loan.fields().priority;loan.require_current();});return result;});
    }
    std::uint32_t actual_current_mask_48()const override {
        return with_descriptor([](const auto& package){std::uint32_t result=0;
            package.with_actual_descriptor([&](const auto& loan){loan.require_current();result=loan.fields().current_category_mask();loan.require_current();});return result;});
    }
    std::int32_t actual_forced_gameplay_count_762400_1_1()const override {
        return with_descriptor([](const auto& package){return std::bit_cast<std::int32_t>(
            detail::BnwNativePackageResourceCheckpoint::forced_gameplay_count_from_loaded_762400_1_1(package));});
    }
};
class BnwNativePackageDiscoveryOwner::OwnedInstalledSkinGlobalSelectionOccurrence final
    : public detail::BnwNativeOwnedSkinGlobalSelectionKernel::Occurrence {
    using Kernel=detail::BnwNativeOwnedSkinGlobalSelectionKernel;
    using Instance=detail::BnwNativeOwnedUiSkinPoststageInstance;
    using Token=detail::BnwNativeOwnedSkinOccurrence;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<OwnedSkinGlobalEffectsLoan> source_;
    const std::shared_ptr<const Instance> instance_;
    const std::shared_ptr<const Token> token_;
    template<class Function> auto with_parent(Function&& operation)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global Skin occurrence foreign thread");
        const auto source=source_;const auto instance=instance_;const auto token=token_;
        return source->with_parent([&](auto& parent,auto& bank){
            const auto check=[&]{instance->require_current();token->require_current();
                if(&instance->constructor_source_->actual_occurrence()!=token.get())
                    throw std::runtime_error("Owned global occurrence substituted actual constructor token");};
            check();using Result=std::invoke_result_t<Function,const BnwNativePackageDiscoveryOwner&,detail::BnwNativeGlobalColorRegistryBank&>;
            static_assert(!std::is_reference_v<Result>);
            if constexpr(std::is_void_v<Result>){operation(parent,bank);check();}
            else {auto result=operation(parent,bank);check();return result;}
        });
    }
    friend class OwnedInstalledSkinGlobalSelectionSource;
    OwnedInstalledSkinGlobalSelectionOccurrence(std::shared_ptr<OwnedSkinGlobalEffectsLoan> source,
        std::shared_ptr<const Instance> instance,std::shared_ptr<const Token> token)
        :source_(std::move(source)),instance_(std::move(instance)),token_(std::move(token)) {
        if(!source_||!instance_||!token_)throw std::runtime_error("Owned global occurrence missing actual anchors");require_current();
    }
  public:
    void require_current()const override {with_parent([](auto&,auto&){});}
    bool actual_constructor_available_26(unsigned slot)const override {
        if(slot>=2)throw std::runtime_error("Owned global constructor slot invalid");
        return with_parent([&](auto&,auto&){return instance_->availability_[slot];});
    }
    bool actual_instance_enabled_24(unsigned slot)const override {
        if(slot>=2)throw std::runtime_error("Owned global enabled slot invalid");
        return with_parent([&](auto&,auto&){return instance_->enabled_[slot]!=0;});
    }
    void disable_actual_pair_767410_then_instance(unsigned slot)override {
        with_parent([&](auto& parent,auto&){parent.disable_installed_global_skin_pair_then_instance(instance_,slot);});
    }
    std::optional<std::string> first_actual_set_6d0090()const override {
        return with_parent([&](auto& parent,auto&){
            std::optional<std::string> result;std::size_t emitted=0;
            parent.with_native_owned_skin_extra_attributes(instance_,[&](const auto& actual){
                if(emitted++)throw std::runtime_error("Owned global extra attribute loan replay");
                const auto policy=source_->policy_.lock();
                if(!policy)throw std::runtime_error("Installed global actual set policy retired");
                result=policy->first_actual_attribute_6d0090(actual,"set");
            });
            if(emitted!=1)throw std::runtime_error("Owned global extra attribute loan absent");
            parent.record_installed_global_skin_set_read(instance_,result);require_current();return result;
        });
    }
    void with_first_actual_record_4138d0_605080(const std::function<void(const Kernel::Record&)>& input)const override {
        with_parent([&](auto& parent,auto&){
            {
                const auto consume=input;require_current();
                if(!consume)throw std::runtime_error("Owned global real Core consumer absent");
                const auto actual_package=token_->package_.lock();
                if(!actual_package)throw std::runtime_error("Owned global instance header Package retired");
                std::optional<std::array<std::uint8_t,16>> guid;
                actual_package->with_actual_descriptor([&](const auto& loan){loan.require_current();guid=loan.fields().written_guid;loan.require_current();});
                if(!guid)throw std::runtime_error("Owned global actual instance header GUID is unwritten");
                std::shared_ptr<const detail::BnwNativeStandalonePackageLoaded> first;
                parent.with_native_owned_returned_package_roster([&](const auto& roster){
                    for(const auto& entry:roster){
                        if(!entry)throw std::runtime_error("Owned global returned Core table contains null");
                        bool matches=false;
                        entry->with_actual_descriptor([&](const auto& loan){
                            loan.require_current();const auto& fields=loan.fields();
                            if((fields.current_flags()&4U)&&!(fields.current_flags()&64U)){
                                if(!fields.written_guid)throw std::runtime_error("Owned global eligible Core GUID is unwritten");
                                matches=*fields.written_guid==*guid;
                            }
                            loan.require_current();
                        });
                        if(matches){first=entry;break;}
                    }
                });
                if(first){OwnedInstalledSkinGlobalSelectionRecord record(source_,first);record.require_current();consume(record);record.require_current();}
            }
            require_current(); // Includes destruction of the copied callable target.
        });
    }
};
// Actual installed Source: distinct from the saved Source, never owns a Parent.
class BnwNativePackageDiscoveryOwner::OwnedInstalledSkinGlobalSelectionSource final
    :public detail::BnwNativeOwnedSkinGlobalSelectionKernel::Source,
     public std::enable_shared_from_this<OwnedInstalledSkinGlobalSelectionSource> {
    friend class BnwNativePackageDiscoveryOwner;
    using Kernel=detail::BnwNativeOwnedSkinGlobalSelectionKernel;
    using Consumer=detail::BnwNativeOwnedSkinConsumer;
    using Policy=detail::BnwNativeOwnedSkinBootPolicy;
    using Bank=detail::BnwNativeGlobalColorRegistryBank;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<OwnedSkinGlobalEffectsLoan> loan_;
    const std::shared_ptr<Consumer> consumer_;
    const std::shared_ptr<Policy> policy_;
    mutable bool failed_=false;
    static void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
    OwnedInstalledSkinGlobalSelectionSource(std::shared_ptr<OwnedSkinGlobalEffectsLoan> loan,
        std::shared_ptr<Consumer> consumer,std::shared_ptr<Policy> policy)
        :loan_(std::move(loan)),consumer_(std::move(consumer)),policy_(std::move(policy)) {
        need(loan_&&consumer_&&policy_,"Installed global actual Source inputs absent");
        // No currentness callback before this complete object's typed registration.
    }
    template<class Function> auto with_parent(Function&& operation)const {
        need(thread_==std::this_thread::get_id(),"Installed global actual Source foreign creator");
        try {
            need(!failed_,"Installed global actual Source failed");
            const auto storage=shared_from_this();
            return loan_->with_parent([&](auto& parent,auto& bank){
                const auto check=[&]{need(!failed_,"Installed global actual Source invalidated");
                    parent.require_installed_global_source_current(static_cast<const Kernel::Source*>(this));};
                check();using Result=std::invoke_result_t<Function,const BnwNativePackageDiscoveryOwner&,Bank&>;
                static_assert(!std::is_reference_v<Result>);
                if constexpr(std::is_void_v<Result>){operation(parent,bank);check();}
                else {auto result=operation(parent,bank);check();return result;}
            });
        }catch(...){invalidate();throw;}
    }
    void invalidate()const noexcept {
        if(thread_!=std::this_thread::get_id())return;
        failed_=true;
        if(const auto effects=loan_->effects_.lock())effects->invalidate_actual_operation();
    }
  public:
    void require_current()const override {with_parent([](auto&,auto&){});}
    std::string actual_published_platform_6415f0()const override {
        return with_parent([&](auto&,auto&){return policy_->actual_platform_;});
    }
    bool actual_is_basegame_stricmp(const std::string& value)const override {
        return with_parent([&](auto& parent,auto&){
            parent.begin_installed_global_base_compare(value);
            const bool result=policy_->actual_is_basegame_stricmp(value);
            parent.record_installed_global_base_compare(value,result);return result;
        });
    }
    void with_actual_slot_scope(unsigned slot,const std::function<void(std::vector<std::size_t>&)>& input)override {
        with_parent([&](auto& parent,auto&){parent.with_installed_global_skin_slot(slot,input);});
    }
    void with_actual_roster(const std::function<void(Kernel::Occurrence&)>& input)override {
        with_parent([&](auto& parent,auto&){
            {const auto consume=input;require_current();need(bool(consume),"Installed global real67/exclusion target absent");
                parent.with_installed_global_kernel_roster([&](const auto& instance,const auto& token){
                    OwnedInstalledSkinGlobalSelectionOccurrence occurrence(loan_,instance,token);
                    require_current();occurrence.require_current();consume(occurrence);
                    occurrence.require_current();require_current();
                });require_current();}
            require_current(); // Actual copied target destroyed while SAME slot/body/physical frame live.
        });
    }
    void append_actual_matching_indices_6fa850(const std::string& set,const std::string& platform,
        bool reset,std::vector<std::size_t>& indices)override {
        with_parent([&](auto& parent,auto&){
            parent.with_installed_global_match_pass(set,platform,reset,indices,[&]{
                std::size_t index=0;
                loan_->with_actual_roster_660330([&](Consumer::Occurrence& occurrence){
                    parent.begin_installed_global_match_index(index);
                    require_current();occurrence.require_current();
                    const bool matches=occurrence.matches_actual_selection_64f970();
                    occurrence.require_current();require_current();
                    parent.record_installed_global_match_index(index,matches);
                    need(index!=std::numeric_limits<std::size_t>::max(),"Installed global actual dense index overflow");++index;
                });require_current();
            });
        });
    }
    bool select_and_load_actual_660330(const std::string& set,const std::string& platform,int slot)override {
        return with_parent([&](auto& parent,auto&){
            parent.prepare_installed_global_660(set,platform,slot);
            const bool changed=consumer_->select_and_load_660330(slot);
            parent.record_installed_global_660_return(consumer_,slot,changed);return changed;
        });
    }
    void reconcile_actual_bank_462c20()override {
        with_parent([&](auto& parent,auto&){parent.reconcile_installed_global_bank();});
    }
    void invalidate_actual_operation()noexcept override {
        invalidate();
    }
};
void BnwNativePackageDiscoveryOwner::run_installed_global_skin_kernel()const {
    with_installed_global_skin_inputs([&](const auto& original,const auto& consumer,const auto& policy){
        const auto loan=make_installed_global_effects_loan(original,consumer,policy);
        const auto concrete=std::shared_ptr<OwnedInstalledSkinGlobalSelectionSource>(
            new OwnedInstalledSkinGlobalSelectionSource(loan,consumer,policy));
        const std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel::Source> source=concrete;
        const std::shared_ptr<const void> erased=source; // Genuine typed pointer AND control conversion.
        record_installed_global_source(erased,loan);
        const auto kernel=std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel>(
            new detail::BnwNativeOwnedSkinGlobalSelectionKernel(source));
        record_installed_global_kernel(kernel);begin_installed_global_kernel(kernel);
        const bool changed=kernel->reconcile_4691e0(-1); // Sole real coupled installed invocation.
        record_installed_global_kernel_return(kernel,changed);
    });
}

void BnwNativePackageDiscoveryOwner::OwnedSkinGlobalSelectionSource::with_actual_roster(
    const std::function<void(Kernel::Occurrence&)>& input) {
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned global roster foreign creator thread");
    const auto storage=shared_from_this();require_current();
    {
        const auto consume=input;require_current();
        if(!consume)throw std::runtime_error("Owned global roster consumer absent");
        with_parent([&](auto& parent,auto&){
            parent.with_native_owned_skin_constructed_roster([&](const auto& roster){
                for(const auto& entry:roster){
                    const auto instance=entry;
                    if(!instance)throw std::runtime_error("Owned global actual dense roster contains null");
                    std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence> token;
                    parent.with_native_owned_skin_occurrences([&](const auto& actual){
                        if(&instance->constructor_source_->actual_occurrence()==actual.get()){
                            if(token)throw std::runtime_error("Owned global roster token repeated");token=actual;
                        }
                    });
                    if(!token)throw std::runtime_error("Owned global roster actual token absent");
                    OwnedSkinGlobalSelectionOccurrence occurrence(storage,instance,token);
                    occurrence.require_current();consume(occurrence);occurrence.require_current();
                }
            });
        });
    }
    require_current();
}


std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>
BnwNativePackageDiscoveryOwner::drive_native_owned_skin_base_game_boot(
    const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>& actual_policy)const {
    // Both actual Source and Consumer remain owned across all client callbacks.
    const auto policy=actual_policy;if(!policy)throw std::runtime_error("Owned Skin genuine boot policy absent");
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> completed_owner;
    with_native_owned_skin_effect_bank([&](const auto& actual_bank){
        const auto bank=actual_bank;
        detail::BnwNativeGlobalColorRegistryBank::OwnedSkinOperation operation(bank);
        const auto source=std::make_shared<OwnedSkinEffectsSource>(bank,policy);
        try {
            begin_native_owned_skin_effect_source(*policy,static_cast<detail::BnwNativeOwnedSkinConsumer::Source*>(source.get()));
            const auto consumer=std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>(new detail::BnwNativeOwnedSkinConsumer(source));
            policy->begin_common();consumer->select_and_load_660330(-1);
            consumer->require_current();policy->common_660330_returned_publish_pc();
            policy->begin_pc();consumer->select_and_load_660330(-1);
            consumer->require_current();policy->pc_660330_returned();policy->require_returned();
            complete_native_owned_skin_base_game_effects(policy,consumer);
            const auto global_source=std::make_shared<OwnedSkinGlobalSelectionSource>(source,consumer,policy);
            begin_native_owned_skin_global_source(*policy,consumer,
                static_cast<detail::BnwNativeOwnedSkinGlobalSelectionKernel::Source*>(global_source.get()));
            const auto global_kernel=std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel>(
                new detail::BnwNativeOwnedSkinGlobalSelectionKernel(global_source));
            global_kernel->reconcile_4691e0(-1);
            global_kernel->require_current();consumer->require_current();policy->require_global_constraints_idle();
            complete_native_owned_skin_global_effects(policy,consumer,global_kernel);
            bank->finalize_owned_initial_skin_colors();
            complete_native_owned_color_bank_initialization();
            // Capture only while the real full-global/finalizer return and
            // original Bank stack operation are still current. Keep the owner
            // in caller storage; storing it in retained Kernel/Source cycles Impl.
            const auto actual_completed=capture_native_owned_completed_resource_bank_capability();
            if(!actual_completed||completed_owner)throw std::runtime_error("Owned completed resource capture absent/replayed");
            actual_completed->require_identity_current();completed_owner=actual_completed;
            // All actual objects and SAME stack Bank operation remain held
            // through the genuine full-global and finalizer normal returns.
            global_kernel->require_current();consumer->require_current();policy->require_global_constraints_idle();
        }catch(...){source->invalidate_actual_operation();throw;}
        // The actual owner survives this operation and the outer Skin fence;
        // its file/storage identity still grants no late native resource query.
    });
    if(!completed_owner)throw std::runtime_error("Owned full resource driver did not return genuine completed owner");
    completed_owner->require_identity_current();return completed_owner;
}
} // namespace civ5::ui

// Source7: private installed source for actual52D590, never a saved lease.
namespace civ5::ui::detail {
bool BnwNativeGlobalColorRegistryBank::load_installed_base() {
    OwnedCallScope operation(shared_from_this());
    try {
        require_owned_current();
        bank_owned_need(phase_==Phase::cold&&owned_source_&&sources_.empty()&&roster_.empty()&&owned_roster_.empty()&&
            global_&&global_->colors_.empty()&&!global_->sets_,"Installed4D96C0 reset requires actual fresh Bank storage");
        //Actual reset creates the empty global destination BEFORE the real
        //601CA0 input read. No early content-DOM parser is borrowed here.
        initialize_owned_field(global_);phase_=Phase::base_colors;
        std::size_t callbacks=0;bool returned_true=false;
        owned_source_->with_actual_installed_base_color_input([&](const auto& input){
            bank_owned_need(callbacks++==0,"Installed base Color repeated actual input loan");
            require_owned_current();
            if(!input)return; //Only the issuer's genuine two-NULL/failed-OS result.
            const auto check=[&]{input->require_current();require_owned_current();input->require_current();};
            check();
            auto reader=std::shared_ptr<BnwNativeMountedStyleReader>(new BnwNativeMountedStyleReader(input));
            const auto native_success=reader->try_parse_same_owned_source_834b30();check();
            if(!native_success)return; //Native601CA0 false does not load Color/ColorSet.
            const auto root=reader->first_same_owned_element();
            const auto document=root.document.lock();bank_owned_need(bool(document),"Installed base Color real parsed root retired");
            auto name_local=reader->make_same_owned_string_local();auto value_local=reader->make_same_owned_string_local();
            const auto field=global_;bank_owned_need(field&&field->sets_,"Installed base Color actual reset destination absent");
            for(const auto& node:root.same_matching_children_835660_8356c0("Color")) {
                check();document->require_current();
                const auto name=node.first_same_attribute_value_8355c0("Name");
                const auto converted_name=name.consume_same_string_local(name_local);
                bank_owned_need(converted_name!=BnwNativeMountedStyleReader::BorrowedNode::StringConsume::incomplete_adjacent_bytes,
                                "Owned Color Name needs unavailable native adjacent bytes");
                //Native601CA0 ignores conversion false, retaining the prior local.
                const auto actual_name=reader->copied_same_string_local(name_local);
                const auto key=native_color_load_detail::hash_query(actual_name);check();
                if(field->colors_.contains(key))continue;
                const auto color=node.first_same_attribute_value_8355c0("Color");
                const auto converted_value=color.consume_same_string_local(value_local);
                bank_owned_need(converted_value!=BnwNativeMountedStyleReader::BorrowedNode::StringConsume::incomplete_adjacent_bytes,
                                "Owned Color value needs unavailable native adjacent bytes");
                const auto actual_value=reader->copied_same_string_local(value_local);check();
                for(unsigned char byte:actual_value)bank_owned_need(byte>=0x20&&byte<=0x7e,"Owned Color selected numeric FString unsupported");
                BnwNativeColorSlotStorage::Value value;
                require_owned_numeric_context();
                if(!native_color_load_detail::trim(actual_value).empty()) {
                    native_color_load_detail::numeric(actual_value,value.bits);require_owned_numeric_context();value.known.fill(true);
                }
                //Empty text preserves actual unknown allocator bytes. Normal
                //lookup rejects an unknown hit; no zero values are manufactured.
                document->require_current();check();field->colors_.emplace(key,value);check();
            }
            //The global base destination is the genuine first601CA0 call.
            //Reuse the accepted real reader/locals/ColorSet conversion kernel.
            field->sets_->read_from_same_owned_reader(reader,1,name_local,value_local,
                [this](std::uint32_t key){return ordered_owned_lookup(key);});
            document->require_current();check();returned_true=true;
        });
        require_owned_current();bank_owned_need(callbacks==1,"Installed base Color omitted actual source return");
        phase_=Phase::base_loaded;require_owned_current();return returned_true;
    }catch(...){if(owned_source_)owned_source_->invalidate_actual_operation();poison();throw;}
}
}

namespace civ5::ui {
class BnwNativePackageDiscoveryOwner::InstalledColorSkinBankSource final
    :public detail::BnwNativeGlobalColorRegistryBank::OwnedSource {
    using Bank=detail::BnwNativeGlobalColorRegistryBank;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::weak_ptr<Bank> bank_;
    std::shared_ptr<Bank> borrow_bank()const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Installed Color source foreign creator");
        const auto bank=bank_.lock();if(!bank)throw std::runtime_error("Installed Color actual Bank retired");return bank;
    }
  public:
    explicit InstalledColorSkinBankSource(const std::shared_ptr<Bank>& bank):bank_(bank){
        if(!bank)throw std::runtime_error("Installed Color actual source Bank absent");
    }
    void require_same_bank_current(const Bank& actual,const content::XmlNode* document,std::uint64_t ordinal)const override {
        const auto bank=borrow_bank();if(bank.get()!=&actual)throw std::runtime_error("Installed Color source has another Bank");
        Bank::PackagePin held(*bank);bank->require_owned_numeric_context();
        held.parent->require_installed_color_skin_bank_current(actual,document,ordinal);bank->require_owned_numeric_context();
    }
    void with_actual_base_color_document(const std::function<void(const std::shared_ptr<const content::XmlNode>&)>&)const override {
        const auto bank=borrow_bank();require_same_bank_current(*bank,nullptr,0);
        throw std::runtime_error("Installed601CA0 requires its actual selected stream/Reader; early content-DOM route is unavailable");
    }
    void with_actual_installed_base_color_input(const std::function<void(
        const std::shared_ptr<const detail::BnwNativeOwnedSkinSelectedInput>&)>& input)const override {
        const auto bank=borrow_bank();Bank::PackagePin held(*bank);require_same_bank_current(*bank,nullptr,0);
        {
            const auto consume=input;require_same_bank_current(*bank,nullptr,0);
            if(!consume)throw std::runtime_error("Installed Color actual input consumer absent");
            held.parent->with_installed_color_skin_base_input([&](const auto& selected){
                require_same_bank_current(*bank,nullptr,0);consume(selected);require_same_bank_current(*bank,nullptr,0);
            });
            require_same_bank_current(*bank,nullptr,0);
        }
        require_same_bank_current(*bank,nullptr,0);
    }
    void with_actual_constructed_roster(const std::function<void(
        const std::vector<std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>>&)>& input)const override {
        const auto bank=borrow_bank();Bank::PackagePin held(*bank);require_same_bank_current(*bank,nullptr,0);
        {
            const auto consume=input;require_same_bank_current(*bank,nullptr,0);
            if(!consume)throw std::runtime_error("Installed Color actual roster consumer absent");
            held.parent->with_native_owned_skin_constructed_roster([&](const auto& actual){
                require_same_bank_current(*bank,nullptr,0);consume(actual);require_same_bank_current(*bank,nullptr,0);
            });
        }
        require_same_bank_current(*bank,nullptr,0);
    }
    void require_initial_skin_effects_terminal(const Bank& actual)const override {
        require_same_bank_current(actual,nullptr,0);
        throw std::runtime_error("Mandatory actual installed later660330/global Skin effects before terminal Color initialization");
    }
    void invalidate_actual_operation()const noexcept override {
        if(thread_!=std::this_thread::get_id())return;
        if(const auto bank=bank_.lock())try {Bank::PackagePin held(*bank);held.parent->invalidate_native_owned_color_operation();}
            catch(...){bank->poison();}
    }
};
bool BnwNativePackageDiscoveryOwner::prepare_installed_color_skin_bank()const {
    const auto bank=construct_installed_color_skin_bank_receiver();
    try {
        const std::shared_ptr<const detail::BnwNativeGlobalColorRegistryBank::OwnedSource> source(new InstalledColorSkinBankSource(bank));
        begin_native_owned_color_bank_source_attachment(*bank,source.get());bank->attach_same_owned_source(source);
        complete_native_owned_color_bank_source_attachment(*bank);
        const auto returned=bank->load_installed_base();require_installed_color_skin_operation_current();return returned;
    }catch(...){invalidate_native_owned_color_operation();throw;}
}
void BnwNativePackageDiscoveryOwner::construct_installed_color_skin_instance(
    const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>& occurrence)const {
    require_installed_color_skin_operation_current();
    const auto token=occurrence;if(!token)throw std::runtime_error("Installed48C800 actual occurrence absent");
    token->require_current();
    prepare_native_owned_skin_extra_attribute_query(token);
    token->require_current();
    const auto source=std::shared_ptr<const OwnedSkinConstructorSource>(new OwnedSkinConstructorSource(token));
    const auto instance=std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>(
        new detail::BnwNativeOwnedUiSkinPoststageInstance(source));
    instance->require_current();token->require_current();
    bind_native_owned_skin_extra_attributes(token,instance);
    instance->require_current();token->require_current();
    retain_native_owned_skin_instance(token,instance);
    instance->require_current();token->require_current();require_installed_color_skin_operation_current();
}
}


// Source8b: exactly the first actual Common660330. The genuine Source8a
// setters already ran; PC/second660330/global/finalizer remain future calls.
namespace civ5::ui {
void BnwNativePackageDiscoveryOwner::run_installed_first_common_skin_kernel()const {
    with_installed_first_common_skin_effect_bank([&](const auto& bank,const auto& policy){
        const auto source=std::make_shared<OwnedSkinEffectsSource>(bank,policy);
        try {
            begin_native_owned_skin_effect_source(*policy,
                static_cast<detail::BnwNativeOwnedSkinConsumer::Source*>(source.get()));
            const auto consumer=std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>(
                new detail::BnwNativeOwnedSkinConsumer(source));
            const bool changed=consumer->select_and_load_660330(-1);
            consumer->require_current();policy->require_current();
            record_installed_first_common_skin_consumer(consumer,source,changed);
            consumer->require_current();policy->require_current();
        }catch(...){source->invalidate_actual_operation();throw;}
    });
}
}


// Source8d continues the SAME protected Consumer, under a different actual
// invocation owner. Its active guard and selector journal remain the kernel's.
namespace civ5::ui {
void BnwNativePackageDiscoveryOwner::run_installed_pc_skin_kernel()const {
    with_installed_pc_skin_effect_bank([&](const auto&,const auto& policy,const auto& consumer){
        begin_installed_pc_skin_invocation(consumer);
        try {
            const bool changed=consumer->select_and_load_660330(-1);
            record_installed_pc_skin_consumer(consumer,changed);
            consumer->require_current();policy->require_current();
        }catch(...){consumer->source_->invalidate_actual_operation();throw;}
    });
}
}
