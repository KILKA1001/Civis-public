#include "civ5/runtime/bnw_displayed_plot_yield.hpp"
#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include "civ5/runtime/live_session.hpp"
#include <climits>
#include <thread>
namespace civ5::runtime {
namespace {
void need(bool b,const char* m){if(!b)throw BnwDisplayedPlotYieldUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Displayed yield native index unavailable");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwDisplayedPlotYieldUnavailable&){throw;}
 catch(const std::exception& e){throw BnwDisplayedPlotYieldUnavailable(e.what());}
}
}
struct BnwDisplayedPlotYield::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const BnwNormalPlotYieldRules> rules;
 std::shared_ptr<const LiveSessionState> state;
 std::shared_ptr<const saves::SavedSession> source;
 SessionCheckpointIdentity identity;
 std::uint64_t generation=0;
 std::int32_t plot=-1,yield=-1;
};
BnwDisplayedPlotYield::BnwDisplayedPlotYield(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwDisplayedPlotYield::bind_initial(){
 const auto p=impl_;
 need(p&&p->session&&p->rules,"Displayed yield actual owning session/rules absent");
 need(p->thread==std::this_thread::get_id()&&p->session->plot_yield_refresh_thread_==std::this_thread::get_id(),"Displayed yield foreign session thread");
 p->rules->require_current(); // before identity/source/world
 need(p->session->bnw_game_debug_owner_current(),"Displayed yield requires genuine Game reset/Read owner");
 p->identity=p->session->debug_->identity;p->rules->require_current(p->identity.rules_sha256);
 p->state=p->session->state_;p->source=p->session->source_;p->generation=p->session->state_generation_;
 (void)value();
}
void BnwDisplayedPlotYield::require_current()const{
 const auto p=impl_;
 need(p&&p->thread==std::this_thread::get_id()&&p->session&&p->rules,"Displayed yield foreign thread/moved query");
 // Immutable constructor-thread member precedes private/source/world borrows.
 need(p->session->plot_yield_refresh_thread_==std::this_thread::get_id(),"Displayed yield foreign session thread");
 p->rules->require_current(p->identity.rules_sha256);
 need(p->session->bnw_game_debug_owner_current(),"Displayed yield genuine Game import owner retired");
 need(p->session->state_==p->state&&p->session->source_==p->source&&p->session->state_generation_==p->generation,
      "Displayed yield source allocation/generation retired");
 need(p->session->debug_->identity.payload_sha256==p->identity.payload_sha256&&
      p->session->debug_->identity.rules_sha256==p->identity.rules_sha256,
      "Displayed yield actual source identity retired");
 (void)p->session->bnw_game_debug_mode(); // genuine raw/cache transition check
}
std::int32_t BnwDisplayedPlotYield::value()const{return typed([&]{
 require_current();const auto p=impl_;
 const auto value=calculate_source(*p->session,*p->rules,p->plot,p->yield);
 require_current();return value;
});}
BnwDisplayedPlotYield inspect_bnw_displayed_plot_yield(
 std::shared_ptr<const LiveSession> session,std::shared_ptr<const BnwNormalPlotYieldRules> rules,
 std::int64_t plot,std::int64_t yield){return typed([&]{
 auto p=std::make_shared<BnwDisplayedPlotYield::Impl>();
 p->session=std::move(session);p->rules=std::move(rules);
 p->plot=native(plot);p->yield=native(yield);
 BnwDisplayedPlotYield result(std::move(p));result.bind_initial();return result;
});}
}
