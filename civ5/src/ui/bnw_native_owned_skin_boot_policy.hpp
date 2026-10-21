#pragma once
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include <span>
#include <functional>
#include <optional>
#include <cstdint>
#include <limits>
#include <thread>
#include <utility>
#include <stdexcept>
#include <memory>
#include <string>
#include <string_view>
#include <map>
#include <cstring>

namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
// Explicit supported standalone Civis Windows boot policy. The stock 628BE0
// upstream gate is not claimed: this policy chooses its real BaseGame/Common,
// then BaseGame/PC constraints for the SAME selected BNW input epoch.
// Private successful-return transitions must be called by the genuine caller
// after select_and_load_660330(-1) and its actual 462C20 reconciliation return.
// The separate installed origin below is bound to the real724CC0 gate
// and actual778E40 writes; the saved policy keeps its original contract.
class BnwNativeOwnedSkinBootPolicy final
    : public std::enable_shared_from_this<BnwNativeOwnedSkinBootPolicy> {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    using Input=runtime::BnwSelectedLanguageInputOwner;
    using Boot=::civ5::ui::BnwNativeResourceBootSettingsOwner;
    using Attribute=std::pair<std::string,std::string>;
    enum class Phase { fresh,common_active,pc_published,pc_active,returned,global_idle,global_active,failed };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::weak_ptr<const BnwNativeEarlyInputLease> lease_;
    const std::weak_ptr<const Input> input_;
    const std::weak_ptr<const Boot> boot_;
    // Distinct actual installed caller origin. Expiry never selects saved.
    class InstalledSource {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        InstalledSource()=default;
      public:
        virtual ~InstalledSource()=default;
        InstalledSource(const InstalledSource&)=delete;
        virtual void require_actual_inputs_current(const Input&,const Boot&)const=0;
        virtual void with_actual_global_constraint_read(const BnwNativeOwnedSkinBootPolicy&,
            const std::function<void(const std::string&,const std::string&)>&)const {
            throw std::runtime_error("Installed global match requires NEW actual slot read loan");
        }

        virtual void require_actual_initial_constraint_close_current(const BnwNativeOwnedSkinBootPolicy&)const {
            throw std::runtime_error("Actual initial constraint close requires its new genuine caller");
        }
        virtual void require_actual_pc_constraint_current(const BnwNativeOwnedSkinBootPolicy&)const {
            throw std::runtime_error("Actual PC setter requires its new genuine resource operation");
        }
        virtual void require_actual_pc_selection_return_current(const BnwNativeOwnedSkinBootPolicy&)const {
            throw std::runtime_error("Actual second660330 return requires its genuine new invocation");
        }
        // A metadata/currentness loan alone cannot publish PC. Only the new
        // real post-Common operation implements this distinct write affinity.
        virtual void require_actual_pc_publication_current(const BnwNativeOwnedSkinBootPolicy&)const {
            throw std::runtime_error("Installed PC publication has no actual caller operation");
        }
    };
    std::optional<std::weak_ptr<const InstalledSource>> installed_source_;
    // Actual installed initial caller owns this table/header. Saved policy
    // does not allocate it. Value/container semantics are qualified C++;
    // Original allocator/layout/refcount/free-address parity is not claimed.
    class InitialConstraintOwner {
        using Table=std::map<std::string,std::string,std::less<>>;
        std::unique_ptr<Table> header_;
        bool cleared_=false,released_=false;
        Table& live() {need(bool(header_)&&!released_,"Initial constraint header retired");return *header_;}
        const Table& live()const {need(bool(header_)&&!released_,"Initial constraint header retired");return *header_;}
      public:
        InitialConstraintOwner()=default;
        explicit InitialConstraintOwner(std::unique_ptr<Table> header):header_(std::move(header)) {
            need(bool(header_),"Initial constraint owner has no actual table");
        }
        static InitialConstraintOwner create_actual() {return InitialConstraintOwner(std::make_unique<Table>());}
        bool empty()const{return live().empty();}
        std::size_t size()const{return live().size();}
        const std::string& at(const char* key)const{return live().at(key);}
        auto find(const char* key){return live().find(key);}
        auto end(){return live().end();}
        template<class Key,class Value>auto insert_or_assign(Key&& key,Value&& value){
            return live().insert_or_assign(std::forward<Key>(key),std::forward<Value>(value));
        }
        bool has_header()const{return bool(header_);}
        bool clear_returned()const{return cleared_;}
        bool release_returned()const{return released_;}
        void clear_actual_entries() {
            need(!cleared_&&!released_,"Initial6B9530 clear replay");live().clear();
            need(live().empty(),"Initial6B9530 actual table did not clear");cleared_=true;
        }
        void release_actual_header() {
            need(header_&&cleared_&&!released_&&header_->empty(),"Initial caller header release before real clear/replay");
            header_.reset();released_=true; // Actual C++ container destruction, not just a completion flag.
        }
    };
    InitialConstraintOwner installed_constraints_;

    unsigned installed_setters_returned_=0;
    bool installed_pc_publication_attempted_=false,installed_pc_publication_returned_=false;
    std::size_t installed_pc_copied_bytes_=0;
    bool installed_pc_constraint_attempted_=false,installed_pc_constraint_returned_=false;
    bool installed_pc_selection_finish_attempted_=false,installed_pc_selection_returned_=false;
    bool installed_initial_close_attempted_=false,installed_initial_close_call_returned_=false;
    Phase phase_=Phase::fresh;
    std::uint64_t global_constraint_generation_=0;
    mutable bool comparison_active_=false;
    mutable bool installed_global_read_failed_=false;
    bool global_transition_active_=false;
    class Activity final {
        bool& active_;
    public:
        explicit Activity(bool& active):active_(active) {
            need(!active_,"Owned Skin policy reentrant operation");active_=true;
        }
        ~Activity(){active_=false;}
        Activity(const Activity&)=delete;
        Activity& operator=(const Activity&)=delete;
    };
    std::string actual_global_set_,actual_global_platform_;
    std::string actual_platform_; // Real policy publication is initially absent.
    static void need(bool value,const char* message) {
        if(!value)throw std::runtime_error(message);
    }
    BnwNativeOwnedSkinBootPolicy(const std::shared_ptr<const BnwNativeEarlyInputLease>& lease,
        const std::shared_ptr<const Input>& input,const std::shared_ptr<const Boot>& boot)
        :lease_(lease),input_(input),boot_(boot) {
        need(lease&&input&&boot,"Owned Skin boot policy actual inputs absent");
        require_inputs_current(); // Constructor storage is not yet published.
    }
    BnwNativeOwnedSkinBootPolicy(const std::shared_ptr<const Input>& input,
        const std::shared_ptr<const Boot>& boot,const std::shared_ptr<const InstalledSource>& source)
        :input_(input),boot_(boot),installed_source_(source),installed_constraints_(InitialConstraintOwner::create_actual()) {
        need(input&&boot&&source,"Installed Skin policy actual inputs/source absent");
        require_inputs_current();
    }
    static std::shared_ptr<BnwNativeOwnedSkinBootPolicy> capture_installed(
        const std::shared_ptr<const Input>& input,const std::shared_ptr<const Boot>& boot,
        const std::shared_ptr<const InstalledSource>& source) {
        return std::shared_ptr<BnwNativeOwnedSkinBootPolicy>(new BnwNativeOwnedSkinBootPolicy(input,boot,source));
    }
    // Two actual778E40 literal writes after genuine724CC0 zero return.
    // This leaves the first660330 pending; no return or PC value is supplied.
    void publish_installed_common_778e40() {
        need(thread_==std::this_thread::get_id(),"Installed Common native setter foreign creator");
        const auto storage=shared_from_this();require_current();
        need(installed_source_&&phase_==Phase::fresh&&installed_constraints_.empty()&&
             installed_setters_returned_==0&&actual_platform_.empty(),"Installed Common native constraint replay/out of actual zero gate");
        installed_constraints_.insert_or_assign(std::string("set"),std::string("BaseGame"));
        require_current();installed_setters_returned_=1;
        installed_constraints_.insert_or_assign(std::string("platform"),std::string("Common"));
        require_current();installed_setters_returned_=2;
        phase_=Phase::common_active;require_current();
    }
    // Actual supported6415F0(PC) ->6EDE20: strlen, owned copy, terminating NUL.
    // SAME canonical per-boot policy field models the published text value.
    // No arbitrary caller string, filename/field0 capability or Original
    // FString allocator/layout/refcount parity is supplied by this value port.
    void publish_installed_pc_6415f0() {
        need(thread_==std::this_thread::get_id(),"Installed6415F0 foreign creator");
        need(installed_source_&&!installed_pc_publication_attempted_,"Installed6415F0 absent origin/replay");
        installed_pc_publication_attempted_=true; // Negative occurrence BEFORE callbacks/allocation.
        try {
            const auto storage=shared_from_this();const auto source=installed_source_->lock();
            need(bool(source),"Installed6415F0 actual caller source expired");
            source->require_actual_pc_publication_current(*this);require_current();
            need(phase_==Phase::common_active&&actual_platform_.empty()&&!installed_pc_publication_returned_&&
                 installed_pc_copied_bytes_==0&&installed_setters_returned_==2&&
                 installed_constraints_.size()==2&&installed_constraints_.at("set")=="BaseGame"&&
                 installed_constraints_.at("platform")=="Common"&&!comparison_active_&&!global_transition_active_,
                 "Installed6415F0 requires actual Common return before PC value/constraint effects");
            static constexpr char actual_pc[]="PC"; // Pinned Original literal00A035B8, nonempty branch only.
            const auto length=std::strlen(actual_pc);
            need(length==2,"Installed6415F0 actual literal changed");
            std::string replacement(actual_pc,length); // Actual C++ storage/copy; allocation failure never becomes success.
            require_current();source->require_actual_pc_publication_current(*this);
            actual_platform_.swap(replacement);installed_pc_copied_bytes_=length;
            require_current();phase_=Phase::pc_published;require_current();
            source->require_actual_pc_publication_current(*this);
            installed_pc_publication_returned_=true;
        }catch(...){phase_=Phase::failed;throw;}
    }

    // Actual778E40 existing-key branch: SAME platform value, no third key.
    void publish_installed_pc_constraint_778e40() {
        need(thread_==std::this_thread::get_id(),"Installed778E40 PC foreign creator");
        need(installed_source_&&!installed_pc_constraint_attempted_,"Installed778E40 PC absent origin/replay");
        installed_pc_constraint_attempted_=true;
        try {
            const auto storage=shared_from_this();const auto source=installed_source_->lock();need(bool(source),"Installed PC setter source expired");
            source->require_actual_pc_constraint_current(*this);require_current();
            need(phase_==Phase::pc_published&&actual_platform_=="PC"&&installed_pc_publication_returned_&&
                 installed_pc_copied_bytes_==2&&!installed_pc_constraint_returned_&&installed_setters_returned_==2&&
                 installed_constraints_.size()==2&&installed_constraints_.at("set")=="BaseGame"&&
                 installed_constraints_.at("platform")=="Common"&&!comparison_active_&&!global_transition_active_,
                 "Installed PC setter requires actual6415F0 and SAME prior Common constraint");
            const auto found=installed_constraints_.find("platform");
            need(found!=installed_constraints_.end(),"Actual778E40 PC lost existing6D0090 key");
            found->second=actual_platform_; // Qualified real C++ value copy of actual published PC; no allocator/layout parity claim.
            installed_setters_returned_=3;phase_=Phase::pc_active;require_current();
            source->require_actual_pc_constraint_current(*this);installed_pc_constraint_returned_=true;
        }catch(...){phase_=Phase::failed;throw;}
    }
    void finish_installed_pc_selection() {
        need(thread_==std::this_thread::get_id(),"Installed second660330 return foreign creator");
        need(installed_source_&&!installed_pc_selection_finish_attempted_,"Installed second660330 return absent origin/replay");
        installed_pc_selection_finish_attempted_=true;
        try {
            const auto storage=shared_from_this();const auto source=installed_source_->lock();need(bool(source),"Installed PC selection source expired");
            source->require_actual_pc_selection_return_current(*this);require_current();
            need(phase_==Phase::pc_active&&installed_pc_constraint_returned_&&!installed_pc_selection_returned_&&
                 installed_setters_returned_==3&&actual_platform_=="PC"&&installed_constraints_.size()==2&&
                 installed_constraints_.at("set")=="BaseGame"&&installed_constraints_.at("platform")=="PC",
                 "Installed PC return lacks actual setter/second kernel return");
            phase_=Phase::returned;require_current();source->require_actual_pc_selection_return_current(*this);
            installed_pc_selection_returned_=true;
        }catch(...){phase_=Phase::failed;throw;}
    }

    void close_installed_initial_constraints_6b9530() {
        need(thread_==std::this_thread::get_id(),"Installed6B9530 foreign creator");
        need(installed_source_&&!installed_initial_close_attempted_,"Installed6B9530 absent origin/replay");
        installed_initial_close_attempted_=true;
        try {
            const auto storage=shared_from_this();const auto source=installed_source_->lock();need(bool(source),"Installed6B9530 source expired");
            source->require_actual_initial_constraint_close_current(*this);require_current();
            need(phase_==Phase::returned&&actual_platform_=="PC"&&installed_pc_selection_returned_&&installed_setters_returned_==3&&
                 installed_constraints_.size()==2&&installed_constraints_.at("set")=="BaseGame"&&installed_constraints_.at("platform")=="PC"&&
                 !comparison_active_&&!global_transition_active_,"Installed6B9530 requires genuine actual PC selection normal return");
            installed_constraints_.clear_actual_entries();require_current();
            source->require_actual_initial_constraint_close_current(*this);
            installed_constraints_.release_actual_header();require_current();
            source->require_actual_initial_constraint_close_current(*this);installed_initial_close_call_returned_=true;
        }catch(...){phase_=Phase::failed;throw;}
    }

    static std::shared_ptr<BnwNativeOwnedSkinBootPolicy> capture(
        const std::shared_ptr<const BnwNativeEarlyInputLease>& lease,
        const std::shared_ptr<const Input>& input,const std::shared_ptr<const Boot>& boot) {
        return std::shared_ptr<BnwNativeOwnedSkinBootPolicy>(new BnwNativeOwnedSkinBootPolicy(lease,input,boot));
    }
    void require_current()const {
        need(thread_==std::this_thread::get_id(),"Owned Skin boot policy foreign creator thread");
        const auto storage=shared_from_this();
        require_inputs_current();
    }
    void require_inputs_current()const {
        need(thread_==std::this_thread::get_id(),"Owned Skin boot policy foreign creator thread");
        need(phase_!=Phase::failed&&!installed_global_read_failed_,"Owned Skin boot policy failed operation");
#ifndef _WIN32
        throw std::runtime_error("Owned Skin PC policy requires the actual Windows source");
#endif
        if(installed_source_) {
            const auto source=installed_source_->lock();const auto input=input_.lock();const auto boot=boot_.lock();
            need(source&&input&&boot&&lease_.expired(),"Installed Skin policy source/inputs expired or mixed with saved origin");
            source->require_actual_inputs_current(*input,*boot);input->require_same_boot(*boot);
            source->require_actual_inputs_current(*input,*boot);return;
        }
        const auto lease=lease_.lock();const auto input=input_.lock();const auto boot=boot_.lock();
        need(lease&&input&&boot&&lease.use_count()>1&&input.use_count()>1&&boot.use_count()>1,
            "Owned Skin boot policy actual external inputs retired");
        lease->require_current();input->require_same_boot(*boot);
        input->require_same_early_game_root(lease->game_root());
        const auto& profile=lease->profile();const auto& catalog=lease->catalog();
        need(profile.can_activate&&profile.enabled_mods.empty()&&!profile.native_mods_require_port,
            "Owned Skin boot policy unsupported selected content profile");
        bool bnw=false;
        for(const auto index:profile.enabled_dlc)
            if(content::canonical_guid(catalog.packages.at(index).id)=="6da07636-4123-4018-b643-6575b4ec336b")bnw=true;
        need(bnw,"Owned Skin boot policy is not the actual selected BNW profile");
        lease->require_current();input->require_same_boot(*boot);
        need(lease.use_count()>1&&input.use_count()>1&&boot.use_count()>1,
            "Owned Skin boot policy inputs retired during source callbacks");
    }
    void require_same_lease(const BnwNativeEarlyInputLease& actual)const {
        need(thread_==std::this_thread::get_id(),"Owned Skin boot affinity foreign creator thread");
        const auto storage=shared_from_this();require_current();need(!installed_source_,"Installed Skin policy has no saved early lease");const auto lease=lease_.lock();
        need(lease&&lease.get()==&actual,"Owned Skin boot policy substituted early lease");
        require_current();
    }
    void begin_common() {
        const auto storage=shared_from_this();require_current();need(!installed_source_,"Installed Common requires actual778E40 writes from its genuine zero gate");
        need(phase_==Phase::fresh&&actual_platform_.empty(),"Owned Skin Common selection replay");
        phase_=Phase::common_active;
    }
    void common_660330_returned_publish_pc() {
        const auto storage=shared_from_this();require_current();need(!installed_source_,"Mandatory actual installed first660330 normal return before PC publication");
        need(phase_==Phase::common_active&&actual_platform_.empty(),"Owned Skin PC publication before Common return");
        // The actual supported Windows policy writer owns its PC text. It is
        // not a borrowed caller string or a fabricated Original global object.
        actual_platform_="PC";phase_=Phase::pc_published;require_current();
    }
    void begin_pc() {
        const auto storage=shared_from_this();require_current();need(!installed_source_,"Installed PC selection requires genuine next778E40 and its new second660330 operation");
        need(phase_==Phase::pc_published&&actual_platform_=="PC","Owned Skin PC selection before real publication");
        phase_=Phase::pc_active;
    }
    void pc_660330_returned() {
        const auto storage=shared_from_this();require_current();need(!installed_source_,"Installed PC return requires genuine new second660330 return affinity");
        need(phase_==Phase::pc_active&&actual_platform_=="PC","Owned Skin PC selection return out of order");
        phase_=Phase::returned;require_current();
    }
    // Only the actual Parent/Warm global4691 supplier opens this constraint
    // around a real6FA850 or660330 call under the SAME Header/Bank operation.
    // It conveys selector text, not permission to issue any resource/roster.
    void begin_global_constraint(std::string_view set,std::string_view platform) {
        need(thread_==std::this_thread::get_id(),"Global Skin constraint foreign creator thread");
        const auto storage=shared_from_this();
        need(!comparison_active_,"Global Skin constraint changed during actual comparison");
        need(!installed_source_,"Installed global matching requires its NEW per-slot constraint read loan");
        Activity transition(global_transition_active_);require_current();
        need((phase_==Phase::returned||phase_==Phase::global_idle)&&actual_platform_=="PC",
            "Global Skin constraint before actual BaseGame prefix return or during another constraint");
        set=set.substr(0,set.find('\0'));platform=platform.substr(0,platform.find('\0'));
        // Supported no-mod BNW installed sets use literal constraints. A
        // nonliteral authored set requires the full native wildcard parser;
        // refuse it rather than treating its pattern as string equality.
        need(set.find_first_of("*?;.")==std::string_view::npos,
            "Global Skin set needs unsupported native wildcard/dot/semicolon constraint");
        need(platform=="Common"||platform==actual_platform_,"Global Skin substituted published platform");
        need(global_constraint_generation_!=std::numeric_limits<std::uint64_t>::max(),
            "Global Skin constraint generation exhausted");
        actual_global_set_=std::string(set);actual_global_platform_=std::string(platform);
        ++global_constraint_generation_;phase_=Phase::global_active;require_current();
    }
    void end_global_constraint() {
        need(thread_==std::this_thread::get_id(),"Global Skin constraint end foreign creator thread");
        const auto storage=shared_from_this();
        need(!comparison_active_,"Global Skin constraint ended during actual comparison");
        Activity transition(global_transition_active_);require_current();
        need(phase_==Phase::global_active,"Global Skin constraint end outside its actual call");
        need(global_constraint_generation_!=std::numeric_limits<std::uint64_t>::max(),
            "Global Skin constraint generation exhausted");
        ++global_constraint_generation_;
        actual_global_set_.clear();actual_global_platform_.clear();phase_=Phase::global_idle;require_current();
    }
    // Only a genuine Parent/Warm supplier may pass its SAME retained,
    // immutable transferred891960 attributes inside the actual50 loan.
    // Return FIRST folded key's owned C-prefix value; absent != empty.
    std::optional<std::string> first_actual_attribute_6d0090(
        std::span<const Attribute> actual_attributes,std::string_view key)const {
        need(thread_==std::this_thread::get_id(),"Owned Skin attribute query foreign creator thread");
        const auto storage=shared_from_this();
        need(!global_transition_active_,"Owned Skin attribute query during constraint transition");
        Activity observation(comparison_active_);require_current();
        const auto phase=phase_;const auto generation=global_constraint_generation_;
        need((phase==Phase::returned||phase==Phase::global_idle)&&actual_platform_=="PC",
            "Owned Skin global attribute query before actual BaseGame prefix return or during constraint");
        const auto input=input_.lock();need(input&&input.use_count()>1,"Owned Skin attribute CRT input retired");
        const auto prefix=[](std::string_view text){return text.substr(0,text.find('\0'));};
        const auto uppercase=[&](std::string_view text) {
            require_current();auto value=input->platform_uppercase_ascii(prefix(text));require_current();
            need(input.use_count()>1&&phase_==phase&&global_constraint_generation_==generation,
                "Owned Skin attribute source/constraint changed during comparison");return value;
        };
        const auto expected=uppercase(key);std::optional<std::string> result;
        for(const auto& row:actual_attributes)if(uppercase(row.first)==expected) {
            result=std::string(prefix(row.second));break;
        }
        require_current();need(phase_==phase&&global_constraint_generation_==generation,
            "Owned Skin attribute source changed after query");return result;
    }
    bool actual_is_basegame_stricmp(const std::string& actual)const {
        need(thread_==std::this_thread::get_id(),"Owned Skin BaseGame comparison foreign creator thread");
        const auto storage=shared_from_this();
        const auto owned=actual.substr(0,actual.find('\0'));
        need(!global_transition_active_,"Owned Skin BaseGame comparison during constraint transition");
        Activity observation(comparison_active_);require_current();
        const auto phase=phase_;const auto generation=global_constraint_generation_;
        need((phase==Phase::returned||phase==Phase::global_idle)&&actual_platform_=="PC",
            "Owned Skin BaseGame comparison before actual prefix return or during constraint");
        const auto input=input_.lock();need(input&&input.use_count()>1,"Owned Skin BaseGame CRT input retired");
        const bool result=input->platform_ascii_compare(owned,"BaseGame")==0;
        require_current();need(input.use_count()>1&&phase_==phase&&global_constraint_generation_==generation,
            "Owned Skin BaseGame comparison source/constraint changed");return result;
    }
    bool matches_actual_attributes_64f970(std::span<const Attribute> actual_attributes)const {
        need(thread_==std::this_thread::get_id(),"Owned Skin comparison foreign creator thread");
        const auto storage=shared_from_this();
        const bool installed_global=installed_source_&&installed_initial_close_call_returned_;
        if(installed_global&&comparison_active_){installed_global_read_failed_=true;
            throw std::runtime_error("Installed global matcher reentry");}
        need(!global_transition_active_,"Owned Skin comparison during constraint transition");
        Activity comparison(comparison_active_);require_current();
        const auto phase=phase_;const auto generation=global_constraint_generation_;
        need(phase==Phase::common_active||phase==Phase::pc_active||phase==Phase::global_active||
             (installed_global&&phase==Phase::returned),"Owned Skin selector outside genuine active phase");
        const auto input=input_.lock();need(input&&input.use_count()>1,"Owned Skin selector actual CRT input retired");
        const auto prefix=[](std::string_view text){return text.substr(0,text.find('\0'));};
        const auto uppercase=[&](std::string_view text) {
            require_current();auto value=input->platform_uppercase_ascii(prefix(text));require_current();
            need(input.use_count()>1&&phase_==phase&&global_constraint_generation_==generation,"Owned Skin selector source/constraint changed during comparison");
            return value;
        };
        // Actual81D100(flag0) is a CRT-toupper wildcard/semicolon matcher.
        // These fixed patterns contain neither wildcard, dot nor separator:
        // its native81D1C0 first linear branch is exactly uppercase equality.
        // 6D0090 selects the FIRST matching key; duplicate later values cannot
        // replace it. The genuine caller supplies SAME retained extraattrs.
        const auto matches=[&](std::string_view key,std::string_view expected) {
            const auto pattern_key=uppercase(key);const auto pattern_value=uppercase(expected);
            for(const auto& row:actual_attributes)if(uppercase(row.first)==pattern_key)
                return uppercase(row.second)==pattern_value;
            return false;
        };
        if(installed_global) {
            try {
                need(phase==Phase::returned&&installed_constraints_.clear_returned()&&
                     installed_constraints_.release_returned()&&!installed_constraints_.has_header()&&
                     installed_setters_returned_==3&&actual_platform_=="PC",
                     "Installed global match cannot reuse/refill initial constraints");
                const auto source=installed_source_->lock();need(bool(source),"Installed global match source retired");
                bool entered=false,body_returned=false,result=false;
                {
                    const std::function<void(const std::string&,const std::string&)> consume=
                        [&](const std::string& set,const std::string& platform) {
                            need(thread_==std::this_thread::get_id(),"Installed global constraint reader foreign creator");
                            if(entered){installed_global_read_failed_=true;
                                throw std::runtime_error("Installed global constraint reader replay");}
                            entered=true;require_current();
                            // Qualification stays explicit. Text alone is not authority:
                            // new installed continuation must lend SAME real slot read.
                            need(prefix(set).find_first_of("*?;.")==std::string_view::npos,
                                 "Installed global set needs unsupported native pattern supplier");
                            need(prefix(platform)=="Common"||prefix(platform)==actual_platform_,
                                 "Installed global match substituted canonical PC platform");
                            result=matches("set",set)&&matches("platform",platform);
                            require_current();need(phase_==phase&&global_constraint_generation_==generation&&
                                !installed_global_read_failed_,"Installed global match changed source/policy");
                            body_returned=true;
                        };
                    source->with_actual_global_constraint_read(*this,consume);require_current();
                }
                require_current();
                need(entered&&body_returned&&!installed_global_read_failed_,
                     "Installed global constraint reader omitted/aborted real read");
                return result;
            }catch(...){installed_global_read_failed_=true;throw;}
        }
        const auto expected_set=installed_source_?installed_constraints_.at("set"):(phase==Phase::global_active?actual_global_set_:std::string("BaseGame"));
        const auto expected_platform=installed_source_?installed_constraints_.at("platform"):phase==Phase::global_active?actual_global_platform_:
            (phase==Phase::common_active?std::string("Common"):actual_platform_);
        const bool result=matches("set",expected_set)&&matches("platform",expected_platform);
        require_current();need(phase_==phase&&global_constraint_generation_==generation,"Owned Skin selector changed active constraint");return result;
    }
    // Full global/Bank terminal callers must additionally observe that the
    // genuine supplier ended its last matching/loading constraint. The
    // broader prefix return check remains valid inside actual active calls.
    void require_global_constraints_idle()const {
        need(thread_==std::this_thread::get_id(),"Owned Skin terminal constraint foreign creator thread");
        const auto storage=shared_from_this();
        need(!global_transition_active_,"Owned Skin terminal during constraint transition");
        Activity observation(comparison_active_);
        const auto phase=phase_;const auto generation=global_constraint_generation_;
        need((phase==Phase::returned||phase==Phase::global_idle)&&
             actual_global_set_.empty()&&actual_global_platform_.empty(),
             "Owned Skin terminal retains an active global constraint");
        require_current();
        need(phase_==phase&&global_constraint_generation_==generation&&actual_platform_=="PC"&&
             actual_global_set_.empty()&&actual_global_platform_.empty(),
             "Owned Skin terminal constraint changed during source callbacks");
    }
    void require_returned()const {
        need(thread_==std::this_thread::get_id(),"Owned Skin boot return foreign creator thread");
        const auto storage=shared_from_this();require_current();need((phase_==Phase::returned||phase_==Phase::global_idle||phase_==Phase::global_active)&&actual_platform_=="PC","Owned Skin boot selections not returned");
    }
    void invalidate()noexcept {
        if(thread_==std::this_thread::get_id())phase_=Phase::failed;
    }
};
} // namespace civ5::ui::detail
