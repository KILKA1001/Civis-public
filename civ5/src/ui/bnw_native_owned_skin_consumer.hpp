#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <stdexcept>

namespace civ5::ui { class BnwNativePackageDiscoveryOwner; }
namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
// Execution kernel for the actual660330 ->6D4BD0 ->767410 ->462C20 route.
// Only the genuine retained Package/Warm issuer can implement Source. Its
// owned Loaded/Parser/epoch loans remain live through each occurrence callback.
// This class creates no resource manager, source kind, roster or ready seal.
class BnwNativeOwnedSkinConsumer final
    : public std::enable_shared_from_this<BnwNativeOwnedSkinConsumer> {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    enum class Query { directory, paks, file, style, color };
    enum class NativeLoadResult { native_false, native_true };
    class Occurrence {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Occurrence()=default;
      public:
        virtual ~Occurrence()=default;
        Occurrence(const Occurrence&)=delete;
        virtual void require_current()const=0;
        // Actual retained64F970 selection predicate; no copied availability.
        virtual bool matches_actual_selection_64f970()const=0;
        virtual bool first_pair_source_present_48d2b0(unsigned slot)const=0;
        virtual unsigned char first_pair_source_enabled_47d680(unsigned slot)const=0;
        // The issuer performs actual54B8D0 against its SAME held Loaded loan.
        // Slots0/1 are GameplaySkin/Skin. File/Style/Color use filter0x80.
        virtual std::vector<std::string> query_owned_54b8d0(unsigned slot,Query,std::uint32_t filter)=0;
        virtual void load_actual_paks_6f84f0(unsigned slot,const std::vector<std::string>&)=0;
        virtual void register_actual_directory_419930_82a470(unsigned slot,const std::string&,
            unsigned mode,const char* patterns)=0;
        virtual void register_actual_file_419930_82a7a0(unsigned slot,const std::string&,unsigned mode)=0;
        // These methods retain the real owned parser and temporarily disable
        // SAME actual global query manager before selected Style/Color loads.
        virtual void begin_actual_style_loading(unsigned slot)=0;
        virtual NativeLoadResult try_actual_style_load_8348c0_79f460_834b80(unsigned slot,const std::string&)=0;
        virtual void end_actual_style_loading(unsigned slot)=0;
        virtual void begin_actual_color_loading(unsigned slot)=0;
        virtual NativeLoadResult try_actual_color_load_601ca0(unsigned slot,const std::string&)=0;
        virtual void end_actual_color_loading(unsigned slot)=0;
        virtual void publish_actual_name_5d5550(unsigned slot)=0;
        // Setter writes mode1 to both nonnull actual pair members, in0/1
        // order, preserving records/cache/FIRST. Slot and member are distinct.
        virtual void enable_actual_pair_767410(unsigned slot)=0;
        virtual void write_actual_instance_enabled_byte(unsigned slot)=0;
    };
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;
        virtual void require_current()const=0;
        // Issuer walks the genuine native roster order, under its operation
        // fence. An equal copied list cannot issue Occurrence objects.
        virtual void with_actual_roster_660330(const std::function<void(Occurrence&)>&)=0;
        virtual void reconcile_actual_colors_462c20()=0;
        virtual void invalidate_actual_operation()noexcept=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<Source> source_;
    bool active_=false,failed_=false;
    //Private real normal-return journal. Only select_and_load_660330 writes
    //after actual roster and462C20 return/bookends; Parent/Warm also require
    //SAME Source, inactive successful object and their actual effect receipts.
    //This argument trace alone never supplies a terminal completion seal.
    std::vector<int> completed_slot_selectors_;
    explicit BnwNativeOwnedSkinConsumer(std::shared_ptr<Source> source):source_(std::move(source)) {
        if(!source_)throw std::invalid_argument("Owned Skin consumer lacks actual source issuer");
        source_->require_current();
    }
    bool load_slot_6d4bd0(Occurrence&,unsigned);
    // Slot selector is the actual660330 argument. -1 runs0 then1 inside each
    // genuine selected occurrence. No public caller can supply this argument.
    bool select_and_load_660330(int slot);
    void require_current()const;
  public:
    BnwNativeOwnedSkinConsumer(const BnwNativeOwnedSkinConsumer&)=delete;
};
}
