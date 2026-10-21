#include "civ5/ui/bnw_scroll_animation_update_loan.hpp"
#include <cmath>
#include <stdexcept>
namespace civ5::ui {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class A,class B>bool same(const A&a,const B&b){return a&&b&&a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);}
}
BnwScrollAnimationLoopTick::State::State(const std::shared_ptr<const BnwTechAwardAnimationSample>& s,std::uint64_t n):sample(s),sequence(n){}
void BnwScrollAnimationLoopTick::State::origin()const {
    if(creator!=std::this_thread::get_id()){failed=true;throw std::runtime_error("Scroll loop tick foreign creator");}
    need(alive.load()&&!failed.load(),"Scroll loop tick retired or failed");
}
std::shared_ptr<const BnwTechAwardAnimationSample>BnwScrollAnimationLoopTick::State::current()const {
    origin();const auto s=sample.lock();need(bool(s),"Scroll actual loop sample retired");
    need(s->creator_==creator,"Scroll actual sample foreign creator");
    const auto clock=s->clock_.lock();
    need(clock&&clock->creator==creator&&clock->alive&&!clock->failed&&clock->sequence==sequence&&s->sequence_==sequence,
         "Scroll actual GetTickCount producer retired or replaced");
    {const auto self=s->self_.lock();const auto actual=clock->sample.lock();
        need(same(s,self)&&same(s,actual),"Scroll sample canonical pointer/control substituted");}
    need(!clock->scene.expired()&&!clock->ui.expired()&&!clock->window.expired(),"Scroll actual loop sources retired");
    origin();return s;
}
BnwScrollAnimationLoopTick::BnwScrollAnimationLoopTick(const std::shared_ptr<const BnwTechAwardAnimationSample>& sample)
    :state_([&]{need(sample&&sample->creator_==std::this_thread::get_id(),"Scroll loop requires actual creator sample");
        return std::make_shared<State>(sample,sample->sequence_);}()) {
    need(state_->sequence!=0,"Scroll actual loop sequence absent");state_->current();
}
BnwScrollAnimationLoopTick::~BnwScrollAnimationLoopTick()noexcept {state_->alive=false;}
BnwScrollAnimationUpdateLoan::BnwScrollAnimationUpdateLoan(const std::shared_ptr<BnwScrollAnimationLoopTick::State>& state):state_(state) {
    state_->origin();need(state_->entered.load()&&!state_->consumed,"Scroll update outside actual active loop issuer");
    require_current_without_callbacks();
}
BnwScrollAnimationUpdateLoan::~BnwScrollAnimationUpdateLoan()noexcept {alive_=false;}
void BnwScrollAnimationUpdateLoan::require_current_without_callbacks()const {
    // Independent life FIRST, before sample/clock or any borrowed caller.
    if(creator_!=std::this_thread::get_id()){state_->failed=true;throw std::runtime_error("Scroll update loan foreign creator");}
    need(alive_.load(),"Scroll update loan retired");
    state_->origin();need(state_->entered.load(),"Scroll update scope already left");
    const auto sample=state_->current();sample->require_current();state_->origin();
    need(state_->entered.load(),"Scroll update scope retired during sample check");
}
std::uint64_t BnwScrollAnimationUpdateLoan::tick_sequence_without_callbacks()const {
    require_current_without_callbacks();return state_->sequence;
}
float BnwScrollAnimationUpdateLoan::delta_seconds_without_callbacks()const {
    require_current_without_callbacks();const auto sample=state_->current();const auto result=sample->seconds_;
    need(std::isfinite(result)&&result>=0,"Scroll actual update delta invalid");require_current_without_callbacks();return result;
}
std::weak_ptr<BnwClientSceneFrame>BnwScrollAnimationUpdateLoan::actual_scene_without_callbacks()const {
    require_current_without_callbacks();const auto sample=state_->current();const auto clock=sample->clock_.lock();
    need(bool(clock),"Scroll actual clock retired");const auto result=clock->scene;require_current_without_callbacks();return result;
}
}
