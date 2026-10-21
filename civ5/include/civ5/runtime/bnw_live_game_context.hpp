#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include "civ5/runtime/simulation_random.hpp"
#include <memory>
#include <mutex>
#include <thread>
namespace civ5::ui { class SelectedBnwPreviewHost; class BnwOwnedNotificationPlotFxReceiver; }
namespace civ5::graphics { class BnwInitialUnitShouldDimRendererLoan; }
namespace civ5::runtime {
struct LiveSessionState;class BnwAttritionDamageDllWrapperPrefix;class BnwLivePlayerGoldPending;class BnwSelectedStockLuaSourceOwner;class BnwNativeUnitAllocatorCore;class BnwUnitCombatWarnedRestorePlan;
// Closed CvDllGameContext/GetGame equivalent. Only the actual renderer loan
// joined to Core54 may acquire it; a scene or a decoded DTO is not an issuer.
class BnwLiveGameContext final : public std::enable_shared_from_this<BnwLiveGameContext> {
 public:
    ~BnwLiveGameContext();
    BnwLiveGameContext(const BnwLiveGameContext&)=delete;
    BnwLiveGameContext& operator=(const BnwLiveGameContext&)=delete;
 private:
    friend class graphics::BnwInitialUnitShouldDimRendererLoan;
    friend class ui::BnwOwnedNotificationPlotFxReceiver;
    friend class BnwSelectedStockLuaSourceOwner;friend class BnwLivePlayerGoldPending;
    friend class BnwNativeUnitAllocatorCore;
    friend class BnwAttritionDamageDllWrapperPrefix;
    struct DeficitRandomDiagnostic {
     std::int64_t actor=-1,turn=-1;std::uint64_t range=100,value=0,serial=0;
     SimulationRandomState before{},after{};
     // Genuine Civis backend. This is not an original RandCalls.csv receipt.
     const char* location="Disband rand";
    };
    struct LogicalGame;
    class GameReference final {
     public:
        ~GameReference();
        GameReference(const GameReference&)=delete;
        GameReference& operator=(const GameReference&)=delete;
        GameReference(GameReference&&)=delete;
        GameReference& operator=(GameReference&&)=delete;
     private:
        friend class BnwLiveGameContext;
        friend class graphics::BnwInitialUnitShouldDimRendererLoan;
        friend class ui::BnwOwnedNotificationPlotFxReceiver;
        friend class BnwSelectedStockLuaSourceOwner;friend class BnwLivePlayerGoldPending;
        friend class BnwNativeUnitAllocatorCore;
        friend class BnwAttritionDamageDllWrapperPrefix;
        class CombatWarnedReceipt final {
         public:
          ~CombatWarnedReceipt()=default;
          CombatWarnedReceipt(const CombatWarnedReceipt&)=delete;
          CombatWarnedReceipt& operator=(const CombatWarnedReceipt&)=delete;
          std::uint64_t ordinal()const noexcept{return ordinal_;}
          bool before()const noexcept{return before_;}
          bool after()const noexcept{return false;}
          bool was_stored()const noexcept{return stored_;}
          bool restored_history()const noexcept{return restored_;}
         private:
          friend class GameReference;
          explicit CombatWarnedReceipt(std::thread::id creator):creator_(creator){}
          const std::thread::id creator_;
          std::weak_ptr<const CombatWarnedReceipt> self_;
          std::weak_ptr<const BnwLiveGameContext> context_;
          std::weak_ptr<const ui::SelectedBnwPreviewHost> host_;
          std::weak_ptr<const LiveSessionState> world_;
          const GameReference* reference_=nullptr;
          std::uint64_t generation_=0,before_ordinal_=0,ordinal_=0;
          bool before_=false,stored_=false,restored_=false,original_read_qualified_=false;
        };
        struct CombatWarnedSnapshot {
          bool before=false,after=false;
          std::uint64_t before_ordinal=0,ordinal=0;
          bool restored_history=false;
        };
        CombatWarnedSnapshot snapshot_combat_warned_return(
          const std::shared_ptr<const CombatWarnedReceipt>&,bool after_destroy=false)const;
        std::shared_ptr<const CombatWarnedReceipt> prepare_restore_completed_combat_warned(
          const BnwUnitCombatWarnedRestorePlan&);
        void install_restore_completed_combat_warned(const BnwUnitCombatWarnedRestorePlan&,
          const std::shared_ptr<const CombatWarnedReceipt>&)noexcept;
        // Synchronous preparation identity only; cleared in the commit. The
        // authentic receipt keeps no raw Plan or caller-provided number grant.
        const BnwUnitCombatWarnedRestorePlan* prepared_combat_warned_plan_=nullptr;
        std::optional<bool> prepared_combat_warned_reset_;
        std::shared_ptr<CombatWarnedReceipt> prepared_combat_warned_mutable_;
        std::shared_ptr<const CombatWarnedReceipt> combat_warned_;
        std::shared_ptr<const CombatWarnedReceipt> set_combat_warned_false();
        void require_combat_warned_return(const std::shared_ptr<const CombatWarnedReceipt>&,
          bool after_destroy=false)const;
        std::shared_ptr<const CombatWarnedReceipt> issued_combat_warned_receipt_without_callbacks()const noexcept;
        struct DeficitRandomReceipt {
         std::weak_ptr<const BnwLivePlayerGoldPending> pending;
         const LiveSessionState* world=nullptr;
         std::int64_t actor=-1,turn=-1;
         std::uint64_t serial=0,revision_before=0,revision_after=0,value=0,trace_index=0;
         SimulationRandomState before{},after{},map{};
         bool stored=false;
        };
        std::optional<DeficitRandomReceipt> deficit_random_;
        void draw_actual_deficit_random100(LiveSessionState&,
          const std::shared_ptr<const BnwLivePlayerGoldPending>&,std::uint64_t,
          const BnwSelectedStockLuaSourceOwner&,const BnwNativeUnitAllocatorCore&);
        const DeficitRandomReceipt& require_actual_deficit_random_return(
          const BnwLivePlayerGoldPending&,bool after_destroy=false)const;
        explicit GameReference(std::shared_ptr<BnwLiveGameContext>);
        void require_actual_binding(const ui::SelectedBnwPreviewHost&) const;
        bool is_final_initialized() const;
        std::int32_t get_active_team() const;
        void destroy();
        void require_destroyed_for(const ui::SelectedBnwPreviewHost&) const;
        std::shared_ptr<BnwLiveGameContext> context_;
        std::shared_ptr<const ui::SelectedBnwPreviewHost> host_;
        std::unique_lock<std::recursive_mutex> lock_;
        const std::thread::id creator_;
        std::uint32_t reference_count_=1;
    };
    static std::shared_ptr<BnwLiveGameContext> retain_for_actual_host(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>);
    explicit BnwLiveGameContext(const std::shared_ptr<const ui::SelectedBnwPreviewHost>&);
    void require_current() const;
    std::unique_ptr<GameReference> get_game();
    std::weak_ptr<const ui::SelectedBnwPreviewHost> host_;
    std::unique_ptr<LogicalGame> game_;
    mutable std::recursive_mutex game_core_lock_;
    const std::thread::id creator_;
};
} // namespace civ5::runtime
