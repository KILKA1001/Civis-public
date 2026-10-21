#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace civ5::ui { class BnwNativePackageDiscoveryOwner; }
namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
// Actual 4691E0 global reconciliation AFTER the real BaseGame Common/PC
// 660330 prefix. Only the genuine Parent/Warm can implement these suppliers.
class BnwNativeOwnedSkinGlobalSelectionKernel final
    : public std::enable_shared_from_this<BnwNativeOwnedSkinGlobalSelectionKernel> {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    class Record {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Record()=default;
      public:
        virtual ~Record()=default;
        Record(const Record&)=delete;
        virtual void require_current()const=0;
        virtual std::int32_t actual_priority_4c()const=0;
        virtual std::uint32_t actual_current_mask_48()const=0;
        // SAME held Parsed/Core descriptor: category1, FORCE1. This ignores
        // mask bit1 while counting Gameplay siblings, then GameData fallback.
        // It must not reuse the existing force0 count that masks disabled rows.
        virtual std::int32_t actual_forced_gameplay_count_762400_1_1()const=0;
    };
    class Occurrence {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Occurrence()=default;
      public:
        virtual ~Occurrence()=default;
        Occurrence(const Occurrence&)=delete;
        virtual void require_current()const=0;
        virtual bool actual_constructor_available_26(unsigned slot)const=0;
        // Actual instance-header GUID, then FIRST same GUID registered Core
        // with bit2 set and bit6 clear. A missing real record invokes no body.
        // Unknown GUID/storage/registration must refuse, never invent a row.
        virtual void with_first_actual_record_4138d0_605080(
            const std::function<void(const Record&)>&)const=0;
        // FIRST native case-insensitive key "set" in genuine transferred
        // 891960 output. Missing and present-but-empty remain distinct.
        virtual std::optional<std::string> first_actual_set_6d0090()const=0;
        virtual bool actual_instance_enabled_24(unsigned slot)const=0;
        // Native 767410(0) first, actual instance enabled byte zero second.
        virtual void disable_actual_pair_767410_then_instance(unsigned slot)=0;
    };
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;
        // SAME live Parent/Header epoch, ordered returned Core table/roster,
        // actual BaseGame-prefix return and current owned Bank operation loan.
        // The adapter authenticates genuine canonical controls across callbacks.
        virtual void require_current()const=0;
        // One complete processed slot, including actual temporary teardown.
        // Lend SAME owner index list; invoke body exactly once synchronously.
        // No next-slot observations before this call actually returns.
        virtual void with_actual_slot_scope(unsigned slot,
            const std::function<void(std::vector<std::size_t>&)>&)=0;
        virtual std::string actual_published_platform_6415f0()const=0;
        virtual bool actual_is_basegame_stricmp(const std::string&)const=0;
        // Emit ONLY actual nonnull instances in their real roster order.
        virtual void with_actual_roster(const std::function<void(Occurrence&)>&)=0;
        // Real 6FA850 predicates use SAME transferred attributes/current CRT.
        // Preserve duplicates and dense nonnull roster indices. reset=true
        // clears the supplied observation list (5E5490), not enabled state.
        virtual void append_actual_matching_indices_6fa850(const std::string& set,
            const std::string& platform,bool reset,std::vector<std::size_t>&)=0;
        virtual bool select_and_load_actual_660330(const std::string& set,
            const std::string& platform,int slot)=0;
        virtual void reconcile_actual_bank_462c20()=0;
        virtual void invalidate_actual_operation()noexcept=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<Source> source_;
    bool active_=false,failed_=false;
    std::vector<int> normal_returned_arguments_;
    static void need(bool value,const char* message) {
        if(!value)throw std::runtime_error(message);
    }
    explicit BnwNativeOwnedSkinGlobalSelectionKernel(std::shared_ptr<Source> source)
        :source_(std::move(source)) {
        need(bool(source_),"Global Skin selection lacks actual source issuer");
        source_->require_current();
    }
    void require_current()const {
        need(thread_==std::this_thread::get_id(),"Global Skin selection foreign creator thread");
        need(!failed_,"Global Skin selection failed actual operation");
        source_->require_current();
        need(!failed_,"Global Skin selection retired during source callback");
    }
    std::optional<std::string> select_actual_set_67fdd0(unsigned slot) {
        std::int32_t highest=-1;
        std::optional<std::string> selected;
        require_current();
        source_->with_actual_roster([&](Occurrence& occurrence) {
            const auto check=[&]{require_current();occurrence.require_current();require_current();};
            check();const bool available=occurrence.actual_constructor_available_26(slot);check();
            if(!available)return;
            bool emitted=false;
            occurrence.with_first_actual_record_4138d0_605080([&](const Record& record) {
                need(!emitted,"Global Skin first-record supplier replay");emitted=true;
                const auto both=[&]{check();record.require_current();check();};
                both();const auto count=record.actual_forced_gameplay_count_762400_1_1();both();
                if(count!=0) {
                    const auto mask=record.actual_current_mask_48();both();
                    if((mask&2U)==0)return;
                }
                const auto priority=record.actual_priority_4c();both();
                // Real bound=-1, strict greater-than, earliest equal rank wins.
                if(priority<=highest)return;
                auto text=occurrence.first_actual_set_6d0090();both();
                selected=std::move(text);highest=priority;
            });
            check();
        });
        require_current();return selected;
    }
    bool reconcile_4691e0(int requested_slot) {
        need(thread_==std::this_thread::get_id(),"Global Skin operation foreign creator thread");
        const auto storage=shared_from_this();
        if(active_) {
            failed_=true;source_->invalidate_actual_operation();
            throw std::runtime_error("Global Skin selection reentry");
        }
        need(requested_slot>=-1&&requested_slot<=1,"Global Skin invalid actual slot selector");
        active_=true;
        struct Exit {BnwNativeOwnedSkinGlobalSelectionKernel& owner;~Exit(){owner.active_=false;}} exit{*this};
        try {
            require_current(); // Guard is already active across the first real source callback.
            bool changed=false;
            for(unsigned slot=0;slot<2;++slot) {
                if(requested_slot!=-1&&requested_slot!=static_cast<int>(slot))continue;
                bool entered=false,body_returned=false;
                source_->with_actual_slot_scope(slot,[&](std::vector<std::size_t>& included) {
                    if(entered){failed_=true;source_->invalidate_actual_operation();
                        throw std::runtime_error("Global Skin actual slot supplier replay");}
                    entered=true;
                    require_current();auto platform=source_->actual_published_platform_6415f0();require_current();
                    source_->append_actual_matching_indices_6fa850("BaseGame","Common",true,included);require_current();
                    source_->append_actual_matching_indices_6fa850("BaseGame",platform,false,included);require_current();
                    auto selected=select_actual_set_67fdd0(slot);require_current();
                    if(selected) {
                        const bool base=source_->actual_is_basegame_stricmp(*selected);require_current();
                        if(base)selected.reset();
                        else {
                            source_->append_actual_matching_indices_6fa850(*selected,"Common",false,included);require_current();
                            source_->append_actual_matching_indices_6fa850(*selected,platform,false,included);require_current();
                        }
                    }
                    changed=false; // Actual native return resets per processed slot.
                    std::size_t index=0;
                    source_->with_actual_roster([&](Occurrence& occurrence) {
                        const auto check=[&]{require_current();occurrence.require_current();require_current();};
                        check();
                        if(std::find(included.begin(),included.end(),index)==included.end()) {
                            const bool enabled=occurrence.actual_instance_enabled_24(slot);check();
                            if(enabled){occurrence.disable_actual_pair_767410_then_instance(slot);check();}
                            changed=changed||enabled;
                        }
                        ++index;check();
                    });
                    require_current();source_->reconcile_actual_bank_462c20();require_current();
                    if(selected) {
                        const bool common=source_->select_and_load_actual_660330(*selected,"Common",static_cast<int>(slot));require_current();
                        const bool pc=source_->select_and_load_actual_660330(*selected,platform,static_cast<int>(slot));require_current();
                        changed=changed||common||pc;
                    }
                    body_returned=true;
                });
                require_current();
                need(entered&&body_returned,"Global Skin actual slot supplier omitted/aborted body");
            }
            source_->reconcile_actual_bank_462c20();require_current();
            normal_returned_arguments_.push_back(requested_slot);require_current();
            return changed; // Observation only: never a Bank/startup completion seal.
        }catch(...){failed_=true;source_->invalidate_actual_operation();throw;}
    }
  public:
    BnwNativeOwnedSkinGlobalSelectionKernel(const BnwNativeOwnedSkinGlobalSelectionKernel&)=delete;
};
} // namespace civ5::ui::detail
