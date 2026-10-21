#include "../src/ui/bnw_native_callback_owner_guard.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>
using namespace civ5::ui::detail;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F> void refuses(F f){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}need(failed,"callback fence unexpectedly accepted");}
struct Observations {int writes=0,returns=0,destroyed=0;};
struct Parent {int& retired;explicit Parent(int& n):retired(n){}~Parent(){++retired;}};
// A detached fixture adopts the exact production fence. It creates no native
// Config, archive, lease, HWND or stage permission and never runs an unsafe
// pre-fix owner. Observations live outside the owner to check deferred release.
class Probe final {
    BnwNativeCallbackOwnerState state_;
    std::weak_ptr<const Probe> self_;
    Observations& observations_;
    int borrowed_=7;
    std::weak_ptr<const Parent> parent_;
    std::shared_ptr<const Parent> permanent_parent_;
    explicit Probe(Observations& o):observations_(o){}
    using Fence=BnwNativeCallbackOwnerGuard<Probe>;
  public:
    ~Probe(){++observations_.destroyed;}
    static std::shared_ptr<const Probe> acquire(Observations& o){
        auto owner=std::shared_ptr<Probe>(new Probe(o));owner->self_=owner;return owner;
    }
    static std::shared_ptr<const Probe> with_parent(Observations& o,
        const std::shared_ptr<const Parent>& parent,bool permanent=false){
        auto owner=std::shared_ptr<Probe>(new Probe(o));owner->self_=owner;
        owner->parent_=parent;if(permanent)owner->permanent_parent_=parent;return owner;
    }
    bool parent_current()const noexcept{
        Fence fence(state_,self_);auto parent=parent_.lock();return fence.valid_parent(parent);
    }
    const int& parent_borrow(const std::function<void()>& callback)const{
        Fence fence(state_,self_);need(fence.valid(),"initial parent borrow refusal");
        {
            auto parent=parent_.lock();need(fence.valid_parent(parent),"parent absent at entry");
            callback();need(fence.valid_parent(parent),"last parent owner retired");
        } // Exactly the Config validate-local parent-pin lifetime.
        need(fence.valid(),"retired parent authorized borrowed frame");
        ++observations_.returns;return borrowed_;
    }
    bool current()const noexcept{
        Fence fence(state_,self_);return fence.valid();
    }
    void store(const std::function<void()>& callback)const{
        Fence fence(state_,self_);need(fence.valid(),"initial owner refusal");
        callback();need(fence.valid(),"owner lost/reentered after callback");
        ++observations_.writes;
    }
    const int& borrow(const std::function<void()>& callback)const{
        Fence fence(state_,self_);need(fence.valid(),"initial borrow refusal");
        callback();need(fence.valid(),"owner lost/reentered before borrowed return");
        ++observations_.returns;return borrowed_;
    }
    void cross_thread()const{
        Fence outer(state_,self_);need(outer.valid(),"cross-thread outer unavailable");
        bool accepted=true;
        std::thread worker([&]{Fence foreign(state_,self_);accepted=foreign.valid();});worker.join();
        need(!accepted&&outer.valid(),"foreign thread read/poisoned same-thread busy flags");
    }
    void caught_throwing_reentry()const{
        store([&]{refuses([&]{store([]{});});});
    }
};
void clean_and_multiple_owners(){
    Observations o;auto owner=Probe::acquire(o);
    need(owner->current(),"fresh current rejected");
    owner->store([]{});need(owner->borrow([]{})==7&&o.writes==1&&o.returns==1,"clean callback lost result");
    auto retained=owner;
    retained->store([&]{owner.reset();});
    need(o.writes==2&&o.destroyed==0&&retained->current(),"genuine second external owner rejected");
    retained.reset();need(o.destroyed==1,"external owner lifetime changed");
}
void drop_during_store_or_borrow(bool borrow){
    Observations o;auto owner=Probe::acquire(o);std::weak_ptr<const Probe> weak=owner;
    const auto* raw=owner.get();
    auto drop=[&]{owner.reset();need(!weak.expired()&&o.destroyed==0,"callback released owner before fence finished");};
    if(borrow)refuses([&]{(void)raw->borrow(drop);});else refuses([&]{raw->store(drop);});
    need(weak.expired()&&o.destroyed==1&&o.writes==0&&o.returns==0,"owner drop published or leaked");
}
void exception_loss_and_ordinary_retry(){
    Observations o;auto owner=Probe::acquire(o);std::weak_ptr<const Probe> weak=owner;const auto* raw=owner.get();
    refuses([&]{raw->store([&]{owner.reset();need(o.destroyed==0,"unwind owner unpinned");throw std::runtime_error("client error");});});
    need(weak.expired()&&o.destroyed==1&&o.writes==0,"owner dropped during throw was not retained/released");
    Observations stable;auto live=Probe::acquire(stable);
    refuses([&]{live->store([]{throw std::runtime_error("ordinary client error");});});
    need(live->current()&&stable.writes==0,"ordinary callback error changed source retry semantics");
    live->store([]{});need(stable.writes==1,"ordinary callback retry refused");
}
void caught_reentry_and_noexcept_false(){
    Observations o;auto owner=Probe::acquire(o);
    refuses([&]{owner->caught_throwing_reentry();});
    need(o.writes==0&&!owner->current(),"caught reentry authorized outer store or unpoisoned owner");
    Observations n;auto live=Probe::acquire(n);bool inner=true;
    // A bool current callback can be noexcept: the exact internal guard returns
    // false/poisons rather than throwing into that callback boundary.
    refuses([&]{live->store([&]() noexcept {inner=live->current();});});
    need(!inner&&n.writes==0&&!live->current(),"noexcept current reentry accepted or published");
    Observations d;auto drop=Probe::acquire(d);auto* raw=drop.get();
    refuses([&]{raw->store([&]{need(!raw->current(),"nested current accepted");drop.reset();throw std::runtime_error("caught client failure");});});
    need(d.destroyed==1&&d.writes==0,"reentry plus throwing release leaked/published");
}
void parent_retirement(){
    Observations o;int retired=0;std::shared_ptr<const Parent> parent=std::make_shared<Parent>(retired);
    auto owner=Probe::with_parent(o,parent);need(owner->parent_current(),"fresh weak parent refused");
    refuses([&]{(void)owner->parent_borrow([&]{parent.reset();need(retired==0,"local parent pin released during callback");});});
    need(retired==1&&o.returns==0&&!owner->current()&&!owner->parent_current(),
         "temporary parent pin authorized stale borrowed return or unpoisoned owner");
    Observations error;int failed_retired=0;std::shared_ptr<const Parent> failed_parent=std::make_shared<Parent>(failed_retired);
    auto error_owner=Probe::with_parent(error,failed_parent);
    refuses([&]{(void)error_owner->parent_borrow([&]{failed_parent.reset();throw std::runtime_error("client parent loss");});});
    need(failed_retired==1&&error.returns==0&&!error_owner->parent_current(),"throwing parent retirement authorized later cached authority");
    Observations combined;int both_retired=0;std::shared_ptr<const Parent> both_parent=std::make_shared<Parent>(both_retired);
    auto both=Probe::with_parent(combined,both_parent);
    refuses([&]{(void)both->parent_borrow([&]() noexcept {both_parent.reset();(void)both->parent_current();});});
    need(both_retired==1&&combined.returns==0&&!both->current(),"caught/false reentry plus parent loss returned old frame");
    // Archive owns service/lease permanently in Impl. Their external facade
    // reset is safe while this genuine persistent dependency remains alive.
    Observations archive;int dependency_retired=0;std::shared_ptr<const Parent> dependency=std::make_shared<Parent>(dependency_retired);
    auto construction=Probe::with_parent(archive,dependency,true);
    need(construction->parent_borrow([&]{dependency.reset();})==7&&dependency_retired==0&&archive.returns==1,
         "permanent archive dependency incorrectly treated as temporary validation pin");
    construction.reset();need(dependency_retired==1,"archive permanent dependency leaked");
    // Factory input-by-value is an owned predecessor for its unpublished scope.
    BnwNativeCallbackOwnerState state;BnwNativeCallbackConstructionGuard factory(state);
    int init_retired=0;std::shared_ptr<const Parent> input=std::make_shared<Parent>(init_retired);
    std::weak_ptr<const Parent> weak=input;auto local=weak.lock();
    need(factory.valid_parent(local),"genuine factory input owner rejected");
    local.reset();input.reset();need(init_retired==1,"factory parent fixture leaked");
}
void unpublished_and_thread(){
    BnwNativeCallbackOwnerState state;
    {BnwNativeCallbackConstructionGuard outer(state);need(outer.valid(),"fresh construction fence refused");
        BnwNativeCallbackConstructionGuard inner(state);need(!inner.valid()&&!outer.valid(),"construction reentry skipped");}
    {BnwNativeCallbackConstructionGuard poisoned(state);need(!poisoned.valid(),"construction poison lost");}
    Observations o;auto owner=Probe::acquire(o);owner->cross_thread();need(owner->current(),"foreign-thread attempt poisoned source thread");
}
}
int main(){try{
    using Fence=BnwNativeCallbackOwnerGuard<Probe>;
    static_assert(!std::is_copy_constructible_v<Fence> && !std::is_move_constructible_v<Fence>);
    static_assert(!std::is_copy_assignable_v<Fence> && !std::is_move_assignable_v<Fence>);
    static_assert(std::is_nothrow_destructible_v<Fence> && noexcept(std::declval<const Fence&>().valid()));
    static_assert(!std::is_copy_constructible_v<BnwNativeCallbackOwnerState>);
    clean_and_multiple_owners();drop_during_store_or_borrow(false);drop_during_store_or_borrow(true);
    exception_loss_and_ordinary_retry();caught_reentry_and_noexcept_false();parent_retirement();unpublished_and_thread();
    std::cout<<"PASS adopted native callback fence: real shared pin, last-owner drop, no stale borrow/store, caught/noexcept reentry, unwind, parent retirement/permanent dependency, thread-first refusal\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
