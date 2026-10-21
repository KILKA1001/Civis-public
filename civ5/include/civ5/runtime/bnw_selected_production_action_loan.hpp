#pragma once
#include <cstdint>
#include <memory>
#include <string>

namespace civ5::ui {
class SelectedBnwPreviewHost;
class SelectedCityProductionServices;
class SelectedMaintainPreparedAction;
class SelectedConstructPreparedAction;
class SelectedTrainPreparedAction;
}
namespace civ5::runtime {
class LiveSession;
struct LiveSessionState;
class BnwSelectedStockLuaSourceOwner;
enum class BnwSelectedProductionLuaEvent {player_can_maintain,city_can_maintain,player_can_construct,city_can_construct,player_can_train,city_can_train};
enum class SelectedProductionDisposition { eligible, committed, vetoed, blocked, uncertain };
struct SelectedProductionResult {
    SelectedProductionDisposition disposition=SelectedProductionDisposition::blocked;
    bool native_committed=false;
    std::string reason;
};
// Implemented by the existing SAME stock owner, never by a caller callback.
// A loan retains its canonical VM/Host/native UI owners. Construction, all
// methods and destruction check the real creator thread/current publication.
class BnwSelectedProductionActionLoan final {
 public:
    ~BnwSelectedProductionActionLoan();
    BnwSelectedProductionActionLoan(const BnwSelectedProductionActionLoan&)=delete;
    BnwSelectedProductionActionLoan& operator=(const BnwSelectedProductionActionLoan&)=delete;
    void require_same(const ui::SelectedBnwPreviewHost&,const LiveSession&,
                      std::int32_t owner,std::int32_t city)const;
    // Native predicate, actual PlayerCanMaintain then CityCanMaintain TestAll.
    // No absent-handler shortcut before the real selected registry is loaded.
    SelectedProductionResult preview_maintain(ui::SelectedMaintainPreparedAction&);
    // Repeats the actual ordered hooks; prepares the genuine dirty owner;
    // checks current native inputs and commits in-place, then publishes ordered
    // conditional SelectionButtons/CityScreen/PlotListButtons, unconditional
    // CityInfo, and SpecificCityInfoDirty(PRODUCTION). No clone/swap/new VM.
    // Lua effects cannot be rolled back here. Late failure is uncertain;
    // native_committed records whether the action's once store was reached.
    SelectedProductionResult execute_maintain(ui::SelectedMaintainPreparedAction&);
    // Exact source phases: action.require_player_native(), actual
    // PlayerCanConstruct(owner,building), action.city_native_allowed(), actual
    // CityCanConstruct(owner,city,building). City veto occurs AFTER Player hook.
    // Execute: prepare genuine Help dirty + city dirty owner, recheck native
    // inputs, private in-place stores, Help dirty FIRST then city notifications.
    SelectedProductionResult preview_construct(ui::SelectedConstructPreparedAction&);
    SelectedProductionResult execute_construct(ui::SelectedConstructPreparedAction&);
    SelectedProductionResult preview_train(ui::SelectedTrainPreparedAction&);
    SelectedProductionResult execute_train(ui::SelectedTrainPreparedAction&);
 private:
    friend class BnwSelectedStockLuaSourceOwner;
    struct Impl;
    explicit BnwSelectedProductionActionLoan(std::shared_ptr<Impl>)noexcept;
    std::shared_ptr<Impl> impl_;
};
// Supplier implements this entry through its private owner-issued factory.
// Required additions in SourceOwner: borrow_city_production(host,owner,city),
// and canonical retained owner validation before any callback. LiveSession
// supplies that owner a private typed in-place handoff (not mutable_state()).
std::unique_ptr<BnwSelectedProductionActionLoan> borrow_selected_city_production(
    std::shared_ptr<BnwSelectedStockLuaSourceOwner>,
    std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int32_t owner,std::int32_t city);
}

namespace civ5::ui {
// Private native write issuer; no caller-provided State or write callback.
// The supplier alone may consume the staged action against its SAME world.
class SelectedMaintainPreparedAction final {
 public:
    ~SelectedMaintainPreparedAction();
    SelectedMaintainPreparedAction(const SelectedMaintainPreparedAction&)=delete;
    SelectedMaintainPreparedAction& operator=(const SelectedMaintainPreparedAction&)=delete;
    std::int32_t owner()const noexcept;
    std::int32_t city()const noexcept;
    std::int32_t process()const noexcept;
    bool native_committed()const noexcept;
 private:
    friend class SelectedCityProductionServices;
    friend class runtime::BnwSelectedProductionActionLoan;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    struct Impl;
    explicit SelectedMaintainPreparedAction(std::unique_ptr<Impl>)noexcept;
    void require_for_source_host(const SelectedBnwPreviewHost&,const runtime::LiveSession&)const;
    void require_current_native()const;
    // All allocations and throwing validation precede the actual stores.
    // Requires exact SAME state address; final vector/map swaps and int stores
    // do not allocate. Does not publish UI, advance a turn or claim persistence.
    void apply_native_in_place(runtime::LiveSessionState&);
    std::unique_ptr<Impl> impl_;
};
}

namespace civ5::ui {
class SelectedConstructPreparedAction final {
 public:
    ~SelectedConstructPreparedAction();
    SelectedConstructPreparedAction(const SelectedConstructPreparedAction&)=delete;
    SelectedConstructPreparedAction& operator=(const SelectedConstructPreparedAction&)=delete;
    std::int32_t owner()const noexcept;
    std::int32_t city()const noexcept;
    std::int32_t building()const noexcept;
    bool native_committed()const noexcept;
 private:
    friend class SelectedCityProductionServices;
    friend class runtime::BnwSelectedProductionActionLoan;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    struct Impl;
    explicit SelectedConstructPreparedAction(std::unique_ptr<Impl>)noexcept;
    void require_player_native()const;
    bool city_native_allowed()const;
    void require_for_source_host(const SelectedBnwPreviewHost&,const runtime::LiveSession&)const;
    void require_current_native()const;
    void apply_native_in_place(runtime::LiveSessionState&);
    std::unique_ptr<Impl> impl_;
};
}

namespace civ5::ui {
class SelectedTrainPreparedAction final {
 public:
    ~SelectedTrainPreparedAction();
    SelectedTrainPreparedAction(const SelectedTrainPreparedAction&)=delete;
    SelectedTrainPreparedAction& operator=(const SelectedTrainPreparedAction&)=delete;
    std::int32_t owner()const noexcept;std::int32_t city()const noexcept;std::int32_t unit()const noexcept;
    bool native_committed()const noexcept;
 private:
    friend class SelectedCityProductionServices;
    friend class runtime::BnwSelectedProductionActionLoan;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit SelectedTrainPreparedAction(std::unique_ptr<Impl>)noexcept;
    void require_player_native()const;bool city_native_allowed()const;void require_current_native()const;
    void require_for_source_host(const SelectedBnwPreviewHost&,const runtime::LiveSession&)const;
    void apply_native_in_place(runtime::LiveSessionState&);
};
}
