#pragma once
#include "civ5/content/manifest.hpp"
#include "bnw_native_standalone_package_loaded.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace civ5::ui { class BnwNativePackageDiscoveryOwner; class BnwNativePackageCategorySourceOwner; }
namespace civ5::ui::detail {
class BnwNativeGlobalColorRegistryBank;
// Package owns ONE empty ResourceManager registry before warm/Skin stages.
// The later Color bank borrows that SAME registry; individual Skin managers
// remain lazy (822A50 -> 82A050). Source roots identify genuine query tokens,
// NEVER loaded Style DOMs. Category managers share the cache, with a distinct
// source domain and SAME real CategorySource issuer.
class BnwNativeSkinDocumentRegistration final {
    friend class BnwNativeGlobalColorRegistryBank;
    friend class BnwNativeOwnedUiSkinPoststageInstance;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    enum class SourceKind : std::uint8_t {skin_slot,category,owned_package};
    using Current=std::function<void(const content::XmlNode*,std::uint64_t,std::uint32_t,SourceKind)>;
    struct State;
    struct Document {
        std::weak_ptr<State> owner;
        std::shared_ptr<const content::XmlNode> root;
        //Distinct source domain: actual owned834B30 Parser Document, never an
        //adapter into the old physical descriptor/content::XmlNode domain.
        const BnwNativePackageXmlParserOwner::Document* owned_document=nullptr;
        std::weak_ptr<const BnwNativeStandalonePackageLoaded> owned_package;
        std::uint64_t source_occurrence;
        std::uint32_t slot;
        SourceKind source_kind=SourceKind::skin_slot;
        std::vector<std::string> directory_roots6c;
        const std::uint8_t override88=1; //82A050
        // Exact 82A050 null-root constructor fields; 8C extent is not ABI storage.
        std::optional<std::string> name84=std::string{};
        std::optional<std::uint32_t> group7c=0,priority80=0;
        std::optional<std::uint8_t> current89=1;
    };
    using DocumentHandle=std::shared_ptr<Document>;
    struct State {
        Current current;
        const std::thread::id thread=std::this_thread::get_id();
        std::atomic<bool> closed{false},failed{false},busy{false};
        std::set<std::tuple<SourceKind,std::uint64_t,std::uint32_t>> admitted;
        //Owned factory identity is its retained closed shared owner, never an
        //address alone. Holding this token cannot supply Parent/Warm authority;
        //each admission and use still reborrows its genuine live issuer.
        std::map<std::shared_ptr<const BnwNativeStandalonePackageLoaded>,
                 std::set<std::pair<std::uint64_t,std::uint32_t>>,
                 std::owner_less<std::shared_ptr<const BnwNativeStandalonePackageLoaded>>> owned_admitted;
        std::vector<DocumentHandle> documents,cache;
        explicit State(Current guard):current(std::move(guard)){}
        static void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
        struct Scope {
            std::shared_ptr<State> state;
            explicit Scope(std::shared_ptr<State> value):state(std::move(value)) {
                need(state->thread==std::this_thread::get_id(),"Skin document foreign thread");
                need(!state->closed.load()&&!state->failed.load(),"Skin document owner retired");
                if(state->busy.exchange(true)){state->failed.store(true);throw std::runtime_error("Skin document reentry");}
            }
            ~Scope(){state->busy.store(false);}
            Scope(const Scope&)=delete;Scope&operator=(const Scope&)=delete;
        };
        void check(const content::XmlNode* root,std::uint64_t occurrence,std::uint32_t slot=2,
                   SourceKind kind=SourceKind::skin_slot) {
            need(!closed.load()&&!failed.load(),"Skin document retired before source callback");
            const auto guard=current;need(bool(guard),"Skin document actual source issuer absent");
            guard(root,occurrence,slot,kind);
            need(!closed.load()&&!failed.load(),"Skin document owner lost during source callback");
        }
        void check_document(const DocumentHandle& doc) {
            need(doc&&doc->owner.lock().get()==this,"Skin document belongs to another cache");
            need(std::find(documents.begin(),documents.end(),doc)!=documents.end(),"Skin document not retained by issuer");
            if(doc->source_kind==SourceKind::owned_package) {
                need(!doc->root&&doc->owned_document&&doc->source_occurrence&&doc->slot<3,
                     "Owned package registry document has a mixed/invalid source domain");
                const auto issued=doc->owned_package.lock();
                need(bool(issued),"Owned package registry lost actual returned factory token");
                const auto admission=owned_admitted.find(issued);
                need(admission!=owned_admitted.end()&&
                     admission->second.contains({doc->source_occurrence,doc->slot}),
                     "Owned registry document lost SAME issued factory owner/category/occurrence");
                issued->with_actual_descriptor([&](const auto& loan){
                    loan.require_current();
                    need(&loan.document()==doc->owned_document,
                         "Owned package registry no longer addresses SAME fresh ParserDoc");
                    loan.require_current();
                });
                need(!closed.load()&&!failed.load()&&doc->owner.lock().get()==this&&
                     std::find(documents.begin(),documents.end(),doc)!=documents.end(),
                     "Owned package registry document retired during source callbacks");
            }else check(doc->root.get(),doc->source_occurrence,doc->slot,doc->source_kind);
        }
    };
    std::shared_ptr<State> state_;
    explicit BnwNativeSkinDocumentRegistration(Current guard):state_(std::make_shared<State>(std::move(guard))) {
        auto state=state_;State::Scope scope(state);state->check(nullptr,0);
    }
    // Package supplies the guarded callback: genuine early CategorySource
    // stages and the later SAME bank/Skin slots have separate source domains.
    // Empty registry construction alone admits no document, mount or readiness.
    // 419930's lazy 822A50(0,0) creates a ResourceManager, not a Style DOM.
    DocumentHandle retain_resource_manager_from_same_source(
            std::shared_ptr<const content::XmlNode> root,std::uint64_t occurrence,std::uint32_t slot,SourceKind kind) {
        auto state=state_;State::Scope scope(state);
        try {
            State::need(root!=nullptr&&occurrence!=0&&slot<2,"Skin resource manager source/slot absent");
            state->check(root.get(),occurrence,slot,kind);
            State::need(state->admitted.insert({kind,occurrence,slot}).second,"Resource manager occurrence/domain replay");
            for(const auto& member:state->cache)state->check_document(member);
            auto doc=std::make_shared<Document>();doc->owner=state;doc->root=std::move(root);
            doc->source_occurrence=occurrence;doc->slot=slot;doc->source_kind=kind;
            state->documents.push_back(doc);
            //822AC0 ->822B50: a newly created zero-group/priority identity
            // is appended immediately, prior to Directory/Pak/Style effects.
            state->cache.push_back(doc);
            state->check_document(doc);state->check(nullptr,0);return doc;
        } catch(...){state->failed.store(true);throw;}
    }
    DocumentHandle retain_resource_manager_from_same_skin_source(
            std::shared_ptr<const content::XmlNode> root,std::uint64_t occurrence,std::uint32_t slot) {
        return retain_resource_manager_from_same_source(std::move(root),occurrence,slot,SourceKind::skin_slot);
    }
    DocumentHandle retain_resource_manager_from_same_category_source(
            std::shared_ptr<const content::XmlNode> root,std::uint64_t descriptor,std::uint32_t category) {
        return retain_resource_manager_from_same_source(std::move(root),descriptor,category,SourceKind::category);
    }
    DocumentHandle retain_resource_manager_from_same_owned_package(
            const std::shared_ptr<const BnwNativeStandalonePackageLoaded>& package,
            const BnwNativePackageXmlParserOwner::Document& actual_document,
            std::uint64_t occurrence,std::uint32_t category) {
        auto state=state_;State::Scope scope(state);
        try {
            State::need(package&&occurrence&&category<3,"Owned package manager factory source/category absent");
            package->with_actual_descriptor([&](const auto& loan){
                loan.require_current();State::need(&loan.document()==&actual_document,
                    "Owned manager factory supplied another actual ParserDoc");loan.require_current();
            });
            State::need(state->owned_admitted[package].insert({occurrence,category}).second,
                        "Owned package manager occurrence replay");
            for(const auto& prior:state->cache)state->check_document(prior);
            auto doc=std::make_shared<Document>();doc->owner=state;
            doc->owned_document=&actual_document;doc->owned_package=package;
            doc->source_occurrence=occurrence;doc->slot=category;doc->source_kind=SourceKind::owned_package;
            //Actual822A50→82A050 constructor zero group/priority/+89=1.
            //822AC0→822B50 appends immediately, before Pak/Directory effects.
            state->documents.push_back(doc);state->cache.push_back(doc);
            state->check_document(doc);return doc;
        }catch(...){state->failed.store(true);throw;}
    }
    template<class Update> static void update_document(std::shared_ptr<State> state,
            DocumentHandle doc,Update update) {
        State::Scope scope(state);
        try{state->check_document(doc);update(*doc);state->check_document(doc);}
        catch(...){state->failed.store(true);throw;}
    }
    // 82B3E0 FString assignment/clear. Embedded NUL follows C-string extent;
    // native allocator/codepage behaviour is not claimed by this field port.
    void set_name84(const DocumentHandle& doc,const char* name) {
        const std::string copied_name=name?std::string(name):std::string{};
        update_document(state_,doc,[&](Document& actual){actual.name84=copied_name;});
    }
    //82A470 pushes the ORIGINAL input FString into SAME +6C vector.
    //Its separate slash/trailing-slash/colon path transforms feed scanning,
    //not this prefix vector (PUSH EBX at82A570 verified). No file scan or
    //catalog completion is claimed by this field effect.
    void append_directory_root6c(const DocumentHandle& doc,const char* actual_directory_value) {
        const std::string root=actual_directory_value?std::string(actual_directory_value):std::string{};
        update_document(state_,doc,[&](Document& actual){actual.directory_roots6c.push_back(root);});
    }
    //82B380 exact native CRT prefix test, including empty registered prefix
    //and no path-component boundary check. This is NOT mounted-file lookup,
    //a registry MISS, nor a closed Directory/PAK producer.
    bool directory_prefix_matches(const DocumentHandle& source_doc,const char* actual_path) {
        auto state=state_;const auto doc=source_doc;
        State::need(actual_path!=nullptr,"Skin prefix path absent");
        const std::string path=actual_path;
        State::Scope scope(state);
        try {
            state->check_document(doc);bool matched=false;
#if defined(_WIN32)
            for(const auto& root:doc->directory_roots6c) {
                if(::_strnicmp(root.c_str(),path.c_str(),root.size())==0){matched=true;break;}
            }
#else
            throw std::runtime_error("Skin prefix native Windows CRT source unavailable");
#endif
            state->check_document(doc);return matched;
        } catch(...){state->failed.store(true);throw;}
    }
    void set_group7c(const DocumentHandle& doc,std::uint32_t group) {
        update_document(state_,doc,[&](Document& actual){actual.group7c=group;}); //82B420
    }
    void set_priority80(const DocumentHandle& doc,std::uint32_t priority) {
        update_document(state_,doc,[&](Document& actual){actual.priority80=priority;}); //82B430
    }
    void set_current89(const DocumentHandle& doc,std::uint8_t value) {
        update_document(state_,doc,[&](Document& actual){actual.current89=value;}); //82B3D0 raw byte
    }
    // 822B50 acts on actual document identity, not its path/name/hash. Zero
    // group+priority already present is unchanged; other registrations remove
    // that SAME identity then insert before first larger priority of its group,
    // or after last equal-group item. Unrelated groups retain native order.
    static void register_document_state(std::shared_ptr<State> state,DocumentHandle doc) {
        State::Scope scope(state);
        try {
            state->check(nullptr,0);
            for(const auto& member:state->cache)state->check_document(member);
            state->check_document(doc);
            State::need(doc->group7c&&doc->priority80,"Skin document registration fields unknown");
            auto& cache=state->cache;auto old=std::find(cache.begin(),cache.end(),doc);
            if(*doc->group7c==0&&*doc->priority80==0) {
                if(old==cache.end())cache.push_back(doc);
            } else {
                if(old!=cache.end())cache.erase(old);
                auto position=cache.end();
                for(auto it=cache.begin();it!=cache.end();++it) {
                    State::need((*it)->group7c&&(*it)->priority80,"Skin cache contains unknown native fields");
                    if(*(*it)->group7c!=*doc->group7c)continue;
                    position=it;
                    if(*doc->priority80<*(*it)->priority80)break;
                    position=std::next(it);
                }
                cache.insert(position,doc);
            }
            for(const auto& member:cache)state->check_document(member);
            state->check(nullptr,0);
        } catch(...){state->failed.store(true);throw;}
    }
    void register_document(const DocumentHandle& doc) {
        const auto state=state_;register_document_state(state,doc);
    }
    // 5D5550 exact two retained XML-doc slots. This operation does not set
    // Skin enabled24/25: Directory/Paks/load/Color/final scheduling remain
    // separate producer effects required before the bank can enable a slot.
    void register_skin_pair(const std::array<DocumentHandle,2>& actual_slots,
            const char* actual_namespace,std::uint32_t actual_package_priority) {
        auto state=state_;
        const auto slots=actual_slots;
        const std::string copied_namespace=actual_namespace?std::string(actual_namespace):std::string{};
        try {
            for(std::uint32_t slot=0;slot!=2;++slot) {
                const auto doc=slots[slot];if(!doc)continue;
                State::need(doc->source_kind==SourceKind::skin_slot,"Skin pair contains category manager");
                // Retain only State before any callback. The facade may be
                // released by that callback; no subsequent this/member read.
                update_document(state,doc,[&](Document& actual){actual.name84=copied_namespace;});
                update_document(state,doc,[](Document& actual){actual.group7c=0xfcfce8f9u;});
                update_document(state,doc,[&](Document& actual){actual.priority80=slot+actual_package_priority*2u;});
                register_document_state(state,doc);
            }
            State::Scope scope(state);state->check(nullptr,0);
        } catch(...){state->failed.store(true);throw;}
    }
    BnwNativeSkinDocumentRegistration(const BnwNativeSkinDocumentRegistration&)=delete;
    BnwNativeSkinDocumentRegistration&operator=(const BnwNativeSkinDocumentRegistration&)=delete;
public:
    ~BnwNativeSkinDocumentRegistration(){state_->closed.store(true);}
};
} //namespace civ5::ui::detail
