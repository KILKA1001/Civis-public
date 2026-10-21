#pragma once
namespace civ5::ui {class BnwOwnedCameraGuiScreen;}
#include <atomic>
#include <cstdint>
#include <functional>
#include <exception>
#include <limits>
#include <utility>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
namespace civ5::graphics {class BnwUnitDamageCommandOwner;class BnwSelectedPendingUnitVisualLoan;class BnwInitialCenterUnitRendererLoan;class BnwInitialUnitShouldDimRendererLoan;class BnwUnitShouldDimSourceLoan;}
namespace civ5::graphics {class BnwInitialUnitSetupGraphicalRendererLoan;}
namespace civ5::ui {class BnwEngineUiTurnOwner;class SelectedBnwPreviewHost;class BnwEngineUiEndTurnQueue;namespace detail {class BnwNativeGlobalColorRegistryBank;}}
namespace civ5::runtime {
namespace detail {class BnwWorldViewCanvasXmlMetadata;}
struct LiveSessionState;
class BnwSelectedStockLuaSourceOwner;
class BnwSelectedStockLuaBootstrap;
class BnwSelectedWorldViewContextOwner;
class BnwLocalClientScalarOwner;
class BnwMajorTechTransportRequest;
class BnwOwnedGameplayEventPumpLoan;
class BnwWorldViewSourceOperationLoan;
enum class BnwStockLuaOperationState {idle_suspended,active,returned_suspended,uncertain,retired};
enum class BnwStockLuaOperationKind {prepare,advance,attach_client,attach_native,read_native,production_city_screen,unit_dim,save_data_snapshot,attach_renderer_window,attach_completed_color,attach_gui_screen,research_choice,research_refresh,city_production_choice,train_pop_order_prefix,process_production,train_production,train_pop_order_tail,train_initialization,team_technology,train_renderer_refresh,engine_ui_turn,tech_award_input,policy_adoption,gameplay_event_pump,worldview_render};
enum class BnwStockLuaOperationOutcome {incomplete_suspended,uncertain};
// Diagnostic observations only. No record authorizes source execution or a VM epoch.
struct BnwStockLuaOperationRecord {
    std::uint64_t serial=0;
    BnwStockLuaOperationKind kind=BnwStockLuaOperationKind::prepare;
    BnwStockLuaOperationOutcome outcome=BnwStockLuaOperationOutcome::uncertain;
    bool vm_retained=false,host_retained=false;
    // Private actual pointer comparison and failure stamp; observation, not a token.
    bool same_vm_identity=false,host_failed=false;
    std::uint64_t retained_lua_bytes=0;
    std::uint64_t host_failure_epoch=0;
    std::string suspended_before;
};
struct BnwStockLuaOperationEvidence {
    BnwStockLuaOperationState state=BnwStockLuaOperationState::idle_suspended;
    std::vector<BnwStockLuaOperationRecord> operations;
    std::string error;
};
// Authentic weak descriptor for the SAME current active attempt. It is not an
// old per-call token. Only SourceOwner issues it; diagnostic constructors cannot.
// Graphics may retain it through a private Root-composed source factory, resolve
// each current serial, and require that serial again AFTER its own callback.
class BnwStockLuaAttemptLease final {
 public:
    BnwStockLuaAttemptLease()=delete;
    void require_active()const {(void)checked();}
    std::uint64_t operation_serial()const {return checked()->serial;}
    void require_operation(std::uint64_t serial)const {
        const auto c=checked();
        if(c->serial!=serial){c->invalidated.store(true);throw std::runtime_error("Stock source operation substituted");}
    }
    std::shared_ptr<const LiveSessionState> world_lease()const {return checked()->world;}
 private:
    friend class BnwMajorTechTransportRequest;
    friend class ::civ5::graphics::BnwUnitDamageCommandOwner;
    friend class ::civ5::ui::BnwEngineUiTurnOwner;
    friend class ::civ5::graphics::BnwInitialUnitSetupGraphicalRendererLoan;
    friend class BnwOwnedGameplayEventPumpLoan;
    friend class BnwWorldViewSourceOperationLoan;
    friend class BnwSelectedWorldViewContextOwner;
    friend class detail::BnwWorldViewCanvasXmlMetadata;
    friend class BnwUnitFlagHealthInstanceLoan;
    friend class ::civ5::ui::BnwEngineUiEndTurnQueue;
    friend class ::civ5::graphics::BnwInitialUnitShouldDimRendererLoan;
    friend class ::civ5::graphics::BnwUnitShouldDimSourceLoan;
    friend class BnwSelectedStockLuaSourceOwner;
    friend class BnwNativeUnitAllocatorCore;
    friend class ::civ5::graphics::BnwInitialCenterUnitRendererLoan;
    friend class ::civ5::graphics::BnwSelectedPendingUnitVisualLoan;
    friend class BnwSelectedStockLuaBootstrap;
    friend class BnwLocalClientScalarOwner;
    friend class BnwSpClientUpdateLoan;
    friend class BnwImportedPregameReadCache;
    friend class ::civ5::ui::detail::BnwNativeGlobalColorRegistryBank;
    struct Control {
        // Installed once from the actual private StockBootstrap weak allocation.
        std::function<void()> current;
        const std::thread::id creator=std::this_thread::get_id();
        // Actual private StockBootstrap factory control, seeded once. Weak
        // here: the loan holds a strong operation scope only while borrowed.
        std::weak_ptr<const void> source_owner;
        std::shared_ptr<const LiveSessionState> world;
        std::weak_ptr<const ui::SelectedBnwPreviewHost> selected_host;
        std::atomic<bool> invalidated{false};
        mutable std::atomic<std::uint64_t> transport_source_pins{0};
        std::uint64_t serial=0;
        const BnwSelectedStockLuaBootstrap* vm=nullptr;
    };
    friend class ::civ5::ui::BnwOwnedCameraGuiScreen;
    struct ScopeIdentity {
        std::shared_ptr<const Control> control;
        std::shared_ptr<const void> source_owner;
        const BnwSelectedStockLuaBootstrap* vm=nullptr;
        const LiveSessionState* world=nullptr;
        std::uint64_t serial=0;
        std::thread::id creator;
        ScopeIdentity()=default;
        ScopeIdentity(std::shared_ptr<const Control> c,std::shared_ptr<const void> s,
            const BnwSelectedStockLuaBootstrap* v,const LiveSessionState* w,
            std::uint64_t n,std::thread::id thread)
          :control(std::move(c)),source_owner(std::move(s)),vm(v),world(w),serial(n),creator(thread){acquire();}
        ScopeIdentity(const ScopeIdentity& other){
            other.require_creator();control=other.control;source_owner=other.source_owner;
            vm=other.vm;world=other.world;serial=other.serial;creator=other.creator;acquire();
        }
        ScopeIdentity(ScopeIdentity&& other)noexcept {other.require_creator();take(other);}
        ScopeIdentity& operator=(const ScopeIdentity& other){
            if(this!=&other){require_creator();ScopeIdentity copy(other);swap(copy);}return *this;
        }
        ScopeIdentity& operator=(ScopeIdentity&& other)noexcept {
            if(this!=&other){require_creator();other.require_creator();release();take(other);}return *this;
        }
        ~ScopeIdentity(){release();}
      private:
        bool counted_=false;
        void require_creator()const noexcept {
            if(control&&creator!=std::this_thread::get_id())std::terminate();
        }
        void acquire(){
            require_creator();
            if(!control||!source_owner)return;
            if(control->creator!=creator)throw std::runtime_error("Stock source scope creator differs");
            const auto old=control->transport_source_pins.fetch_add(1);
            if(old==std::numeric_limits<std::uint64_t>::max()){
                control->transport_source_pins.fetch_sub(1);
                throw std::runtime_error("Stock source scope pins exhausted");
            }
            counted_=true;
        }
        void release()noexcept {
            require_creator();
            if(counted_){control->transport_source_pins.fetch_sub(1);counted_=false;}
            source_owner.reset();control.reset();
        }
        void take(ScopeIdentity& other)noexcept {
            control=std::move(other.control);source_owner=std::move(other.source_owner);
            vm=other.vm;world=other.world;serial=other.serial;creator=other.creator;
            counted_=std::exchange(other.counted_,false);
            other.vm=nullptr;other.world=nullptr;other.serial=0;
        }
        void swap(ScopeIdentity& other)noexcept {
            control.swap(other.control);source_owner.swap(other.source_owner);
            std::swap(vm,other.vm);std::swap(world,other.world);std::swap(serial,other.serial);
            std::swap(creator,other.creator);std::swap(counted_,other.counted_);
        }
    };
    // Closed graphics borrower seam. Defined only in concrete SourceOwner TU,
    // where the genuine private StockBootstrap fields are visible. No current
    // provider, Lua/GUI/world getter or post-store revision callback is invoked.
    ScopeIdentity pin_active_scope_without_callbacks()const;
    void require_scope_identity_without_callbacks(const ScopeIdentity&)const;
    explicit BnwStockLuaAttemptLease(const std::shared_ptr<Control>& c):control_(c){}
    std::shared_ptr<Control> checked()const {
        const auto c=control_.lock();
        if(!c)throw std::runtime_error("Stock source attempt expired");
        try {
            // This authentic closure checks creation thread BEFORE all mutable
            // control/world/VM reads. A foreign violation changes atomics only.
            if(!c->current)throw std::runtime_error("Stock source attempt unbound");
            c->current();
            if(c->invalidated.load())throw std::runtime_error("Stock source attempt invalidated");
            return c;
        }catch(...){c->invalidated.store(true);throw;}
    }
    // New receiver operation versus authentic stored source identity. The
    // old Control is read only; its old serial is never admitted or rewritten.
    void require_same_source_as_stored(const BnwStockLuaAttemptLease& prior)const {
        const auto actual=checked();
        if(actual->creator!=std::this_thread::get_id())throw std::runtime_error("Popup input foreign source");
        const auto old=prior.control_.lock();
        if(!old||actual.get()!=old.get()||actual.owner_before(old)||old.owner_before(actual)||
           actual->vm!=old->vm||!actual->vm||actual->world.get()!=old->world.get()||
           actual->world.owner_before(old->world)||old->world.owner_before(actual->world))
            throw std::runtime_error("Popup input belongs to another source VM/world");
        const auto owner=actual->source_owner.lock(),stored=old->source_owner.lock();
        if(!owner||owner.get()!=stored.get()||owner.owner_before(stored)||stored.owner_before(owner))
            throw std::runtime_error("Popup input canonical source owner changed");
        const auto post=checked();
        if(post.get()!=actual.get()||post.owner_before(actual)||actual.owner_before(post))
            throw std::runtime_error("Popup input source changed after callbacks");
    }
    void invalidate()const noexcept {if(const auto c=control_.lock())c->invalidated.store(true);}
    std::weak_ptr<Control> control_;
};
}
