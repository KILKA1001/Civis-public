#pragma once
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include <thread>
#include <memory>
#include <stdexcept>
#include <cstdint>

namespace civ5::ui::detail { class CivisFrontendPartyRoutingOwner; }
namespace civ5::runtime {
// APP owns state objects; APP+6F0 and +6F4 are current/pending references.
// C0 is the original GameViewState, established by loader403510/7352D0.
// A loaded Host is retained by the actual frontend GameView transition.
class BnwCivisApplicationStateOwner final {
    friend class ui::detail::CivisFrontendPartyRoutingOwner;
    struct State final {
        std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
        std::shared_ptr<BnwSelectedStockLuaSourceOwner> source;
        std::shared_ptr<const LiveSessionState> world;
        std::uint64_t generation=0;
    };
    const std::thread::id creator_=std::this_thread::get_id();
    const std::uintptr_t window_;
    bool alive_=true,failed_=false,transitioning_=false;
    const std::shared_ptr<State> menu_=std::make_shared<State>(); // original BC
    std::shared_ptr<State> c0_,c4_,c8_; // C4/C8 remain genuinely uncreated in this saved-game frontend.
    State* current_=nullptr; // 6F0
    State* pending_=nullptr; // 6F4
    explicit BnwCivisApplicationStateOwner(std::uintptr_t window):window_(window) {
        need(window_!=0,"Application actual frontend window absent");
        request(menu_.get()); publish_pending();
    }
    static void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
    void first()const {
        need(creator_==std::this_thread::get_id(),"Application belongs to another frontend thread");
        need(alive_&&!failed_,"Application retired or failed");
    }
    void request(State* next){if(next!=nullptr)pending_=next;} // 4EAF40 only accepts nonNULL.
    void publish_pending(){
        first();need(!transitioning_,"Application transition reentered");
        transitioning_=true;
        struct End{bool& active;~End(){active=false;}} end{transitioning_};
        while(pending_!=nullptr){
            // Original4848AF leaves OLD before publishing/clearing NEW.
            if(current_!=nullptr&&current_->source)current_->source->retire_stock_bootstrap();
            current_=pending_;pending_=nullptr; // 48469A/4848B7, BEFORE the new enter body.
            if(current_->source){
                need(current_->host&&current_->world,"GameView loaded owner absent");
                const auto current_world=current_->host->session().read_state_lease();
                need(current_world.get()==current_->world.get()&&
                    !current_world.owner_before(current_->world)&&!current_->world.owner_before(current_world)&&
                    current_->host->session().state_generation()==current_->generation,
                    "GameView loaded session changed before enter");
                (void)current_->source->start_stock_bootstrap_from_civis_resources();
            }
            first();
        }
    }
    static std::shared_ptr<BnwCivisApplicationStateOwner> create_menu_owned(std::uintptr_t actual_window){
        auto owner=std::shared_ptr<BnwCivisApplicationStateOwner>(new BnwCivisApplicationStateOwner(actual_window));
        owner->self_=owner;return owner;
    }
public:
    BnwCivisApplicationStateOwner(const BnwCivisApplicationStateOwner&)=delete;
    BnwCivisApplicationStateOwner& operator=(const BnwCivisApplicationStateOwner&)=delete;
    class CapturedState final {
        friend class BnwCivisApplicationStateOwner;
        const State* const state_;
        explicit CapturedState(const State* state):state_(state){}
    public:
        CapturedState(const CapturedState&)=default;
    };
    // Called on the real load route, after loaded Host/scene reconciliation,
    // instead of starting stock bootstrap independently of application state.
    void enter_loaded_game_owned(const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,
        const std::shared_ptr<BnwSelectedStockLuaSourceOwner>& source,
        const assets::CivisResourceBankOwner& bank){
        first();need(host&&source&&!c0_&&current_==menu_.get()&&!pending_,"GameView entry repeated or absent");
        host->require_same_civis_resources(bank);first();
        need(bank.actual_hwnd()==window_,"GameView load belongs to another application window");first();
        auto game=std::make_shared<State>();game->host=host;game->source=source;
        game->world=host->session().read_state_lease();game->generation=host->session().state_generation();
        need(game->world&&game->generation!=0,"GameView has no restored actual world");
        const auto prior=host->bnw_civis_application_state_.lock();
        need(!prior,"Loaded Host already belongs to an application");
        c0_=std::move(game);host->bnw_civis_application_state_=shared_from_owner();
        request(c0_.get());
        try{publish_pending();}catch(...){failed_=true;throw;}
    }
    CapturedState capture_current_owned()const {first();return CapturedState(current_);}
    // Fresh APP read AFTER actual PreGame return; current6F0 is deliberately
    // not reread. Compare the BEFORE capture to CURRENT C0/C8/C4 aliases.
    bool game_context_gate_owned(const CapturedState& captured)const {
        first();return captured.state_!=nullptr &&
            (captured.state_==c0_.get()||captured.state_==c8_.get()||captured.state_==c4_.get());
    }
    void require_loaded_host_owned(const ui::SelectedBnwPreviewHost& host)const {
        first();need(c0_&&c0_->host.get()==&host&&c0_->world,
            "Application has another actual loaded Host");
        const auto world=host.session().read_state_lease();
        need(world.get()==c0_->world.get()&&!world.owner_before(c0_->world)&&!c0_->world.owner_before(world)&&
            host.session().state_generation()==c0_->generation,"Application loaded world was replaced");
    }
    void retire_owned()noexcept {
        if(creator_!=std::this_thread::get_id())std::terminate();
        if(!alive_)return;
        alive_=false;
        if(current_&&current_->source)current_->source->retire_stock_bootstrap();
        current_=nullptr;pending_=nullptr; // original exit4848E6
    }
private:
    std::weak_ptr<BnwCivisApplicationStateOwner> self_;
    std::shared_ptr<BnwCivisApplicationStateOwner> shared_from_owner()const {
        auto owner=self_.lock();need(owner&&owner.get()==this,"Application canonical owner absent");return owner;
    }
};
}
