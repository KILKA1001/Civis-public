#include "civ5/runtime/bnw_grand_logging_continuation.hpp"
#include "civ5/runtime/bnw_grand_selection.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "bnw_grand_logging_diagnostic.hpp"
namespace civ5::runtime::grand_logging_detail {
namespace {
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
std::optional<std::string> observed(const Text&t,const char*null_value){
 if(t.kind==TextKind::unknown)return {};
 if(t.kind==TextKind::native_null)return std::string(null_value);
 need(t.kind==TextKind::bytes,"Grand logger source text kind invalid");
 // Native FString consumes a C string, not a length-bearing SQL/blob value.
 return t.value.substr(0,t.value.find('\0'));
}
}
Journal::Journal(bool city,std::int32_t active,std::array<std::int32_t,4> priorities,
 std::array<std::int32_t,4> pressure,std::optional<bool> interrupt)
 :active_(active),priorities_(priorities),pressure_(pressure),interrupt_(interrupt){
 need(active>=-1&&active<4,"Grand logger BNW-four-Info active invalid");
 need(!city||interrupt.has_value(),"Grand logger city reason0 requires observed interrupt field");
 progress_.kind=city?Kind::city:Kind::grand;
}
Progress Journal::progress()const{
 need(std::this_thread::get_id()==thread_,"Grand logger progress foreign thread");
 return progress_;
}
class Engine final {
 std::shared_ptr<Journal> j_;std::shared_ptr<const Source>s_;std::function<void()> owner_check_;bool entered_=false;
 void check(){
  need(!j_->poisoned_.load()&&!j_->progress_.quarantined,"Grand logging source operation quarantined");
 }
 void quarantine(const char*why)noexcept{
  j_->poisoned_.store(true);j_->progress_.quarantined=true;
  // Do not allocate on the uncertain-effect exception path.
  (void)why;
 }
 template<class Fn,class...Args>auto call(const Fn&original,Args&&...args){
  check();auto pinned=original;check();
  if constexpr(std::is_void_v<std::invoke_result_t<Fn,Args...>>){
   pinned(std::forward<Args>(args)...);check();
  }else{auto result=pinned(std::forward<Args>(args)...);check();return result;}
 }
 void current(){
  check();if(owner_check_)call(owner_check_);need(bool(s_->current),"Grand logger source currentness callback absent");
  if(!call(s_->current)){quarantine("source stale");throw std::runtime_error("Grand logging source currentness stale");}
 }
 template<class Fn,class...Args>auto effect(const Fn&fn,Args&&...args){
  j_->progress_.external_effect_in_flight=true;
  if constexpr(std::is_void_v<std::invoke_result_t<Fn,Args...>>){
   call(fn,std::forward<Args>(args)...);current();j_->progress_.external_effect_in_flight=false;
  }else{auto result=call(fn,std::forward<Args>(args)...);current();j_->progress_.external_effect_in_flight=false;return result;}
 }
 Outcome missing(const char*why){j_->progress_.frontier=why;return Outcome::dependency_pending;}
 void returned(){
  auto&p=j_->progress_;j_->receiver_.reset();j_->civilization_.clear();j_->filename_.clear();j_->message_.clear();
  if(p.kind==Kind::city){p.city_returned=true;p.step=Step::reason0;}
  else{p.grand_returned=true;p.step=Step::complete;}
 }
 public:
 Engine(std::shared_ptr<Journal>j,std::shared_ptr<const Source>s,std::function<void()>owner_check={}):j_(std::move(j)),s_(std::move(s)),owner_check_(std::move(owner_check)){
  need(j_&&s_,"Grand logging journal/source missing");
  need(std::this_thread::get_id()==j_->thread_,"Grand logger foreign thread refused before source access");
  if(j_->busy_){quarantine("reentry");throw std::runtime_error("Grand logger reentry refused");}
  check();need(j_->progress_.step!=Step::complete,"Grand logging continuation already completed; no replay");
  need(!j_->source_||j_->source_.get()==s_.get(),"Grand logger SAME source facade replaced");
  j_->busy_=true;entered_=true;if(!j_->source_)j_->source_=s_;
 }
 ~Engine(){if(entered_)j_->busy_=false;}
 Outcome run(){
  try{for(;;){
   current();auto&p=j_->progress_;const auto kind=p.kind;
   switch(p.step){
   case Step::logging:case Step::ai:{
    if(!s_->flag)return missing("actual current GC logging/AI getter required");
    const auto v=call(s_->flag,kind,p.step==Step::logging?Flag::logging:Flag::ai);current();
    if(!v)return missing("actual current GC logging/AI observation required");
    need(*v<=1,"Grand logger actual bool representation invalid");
    if(*v==0)returned();else p.step=p.step==Step::logging?Step::ai:Step::strings;
    break;}
   case Step::strings:
    if(!s_->construct_strings)return missing("native FString construction effect required");
    ++p.string_effect_attempts;effect(s_->construct_strings,kind);p.step=Step::civilization;break;
   case Step::civilization:{
    if(!s_->civilization)return missing("actual civilization description getter required");
    auto v=call(s_->civilization,kind);current();
    if(!v)return missing("actual civilization description observation required");
    auto text=observed(*v,"");if(!text)return missing("unobserved civilization is not source NULL");
    j_->civilization_=std::move(*text);p.step=Step::split;break;}
   case Step::split:{
    if(!s_->flag)return missing("actual GC split getter required");
    auto v=call(s_->flag,kind,Flag::split);current();
    if(!v)return missing("actual GC split observation required");
    need(*v<=1,"Grand logger actual split bool invalid");
    j_->filename_=kind==Kind::city?"CitySpecializationLog":"GrandStrategyAI_Log";
    if(*v)j_->filename_+="_"+j_->civilization_;
    j_->filename_+=".csv";p.step=Step::get_log;break;}
   case Step::get_log:
    if(!s_->get_log)return missing("actual LogManager GetLog effect required");
    ++p.get_log_attempts;j_->receiver_=effect(s_->get_log,kind,j_->filename_);
    need(j_->receiver_!=nullptr,"actual GetLog returned NULL; output effects uncertain");
    p.step=kind==Kind::city?Step::elapsed:Step::count;break;
   case Step::count:{
    if(!s_->strategy_count)return missing("actual native strategy Info count required");
    auto v=call(s_->strategy_count);current();if(!v)return missing("actual native strategy Info count unobserved");
    need(*v>=0&&*v<=4,"Grand logging continuation supports BNW up to four actual Info slots");
    j_->count_=*v;p.step=*v==0?Step::cleanup:Step::elapsed;break;}
   case Step::elapsed:{
    if(!s_->elapsed)return missing("actual elapsed-turn getter required");
    auto v=call(s_->elapsed,kind,j_->row_);current();if(!v)return missing("actual elapsed-turn value unobserved");
    j_->elapsed_=*v;p.step=Step::info;break;}
   case Step::info:{
    if(!s_->strategy_type)return missing("actual native Info observation required");
    const auto slot=kind==Kind::city?j_->active_:static_cast<std::int32_t>(j_->row_);
    auto v=call(s_->strategy_type,kind,slot);current();if(!v)return missing("actual native Info unobserved");
    auto type=observed(*v,kind==Kind::city?"":"Unknown Type");
    if(!type)return missing("unobserved Info is not source NULL");
    j_->message_=diagnostic::prefix(j_->elapsed_,j_->civilization_);
    if(kind==Kind::city){if(v->kind!=TextKind::native_null)j_->message_+="Update: New grand strategy: "+*type;}
    else{
     if(j_->active_==slot)j_->message_+="*** ";
     j_->message_+=*type+", "+std::to_string(j_->priorities_.at(j_->row_))+", "+std::to_string(j_->pressure_.at(j_->row_));
    }
    p.step=Step::msg;break;}
   case Step::msg:{
    if(!j_->receiver_->msg)return missing("actual retained log receiver Msg effect required");
    ++p.msg_attempts;effect(j_->receiver_->msg,j_->message_);
    if(kind==Kind::city){++p.confirmed_city_messages;p.step=Step::cleanup;}
    else{++p.confirmed_grand_messages;++j_->row_;p.step=j_->row_==static_cast<std::size_t>(j_->count_)?Step::cleanup:Step::elapsed;}
    break;}
   case Step::cleanup:
    if(!s_->destroy_strings)return missing("native FString cleanup effect required");
    ++p.string_effect_attempts;effect(s_->destroy_strings,kind);returned();break;
   case Step::reason0:
    // Actual NEW_GRAND_STRATEGY default branch writes no interrupt field.
    need(j_->interrupt_.has_value(),"reason0 interrupt observation lost");
    p.reason0_returned=true;p.kind=Kind::grand;p.step=Step::logging;j_->row_=0;break;
   case Step::complete:
    p.frontier="conditional logger bodies returned; actual enclosing Grand/player receipt remains private";
    return Outcome::conditional_loggers_returned;
   }
  }}catch(...){quarantine("uncertain callback/source failure");throw;}
 }
};
Outcome resume_diagnostic(std::shared_ptr<Journal>j,std::shared_ptr<const Source>s){Engine e(std::move(j),std::move(s));return e.run();}
}
namespace civ5::runtime {
BnwGrandLoggingContinuation::BnwGrandLoggingContinuation(const BnwGrandSelectionPending&s,
 const BnwTeamTechLuaEpoch&e,const content::RulesDatabase&db,
 std::shared_ptr<const BnwGrandLoggingSourceOwner>source)
 :selection_(&s),epoch_(&e),database_(&db),source_(std::move(source)){
 require_current();const auto&q=s.progress();
 journal_=std::make_shared<grand_logging_detail::Journal>(
   q.frontier==BnwGrandSelectionFrontier::city_specialization_log,
   q.decision.active_after,q.priorities,q.pressure_log_changes,q.interrupt_wonders);
}
void BnwGrandLoggingContinuation::require_current()const{
 static_assert(std::is_same_v<decltype(source_->epoch_owner_),std::shared_ptr<const BnwTeamTechLuaEpoch>>);
 if(!source_||!source_->globals_||!source_->logs_||!source_->database_||source_->database_.get()!=database_||
    !source_->activation_||source_->activation_->happiness().grand_selection()!=selection_||
    !source_->epoch_owner_||source_->epoch_owner_.get()!=epoch_||source_->epoch_!=epoch_||
    source_->selection_!=selection_||!source_->program_||!source_->require_current_)
  throw std::runtime_error("Grand logging actual normal startup/GC/log sink/retained activation owner unavailable");
 source_->require_current_();
}
grand_logging_detail::Outcome BnwGrandLoggingContinuation::resume(std::shared_ptr<BnwGrandLoggingContinuation>self){
 if(!self)throw std::invalid_argument("Grand logging actual continuation missing");
 if(!self->source_||!self->source_->program_)throw std::runtime_error("Grand logging genuine source capability missing");
 // The genuine private issuer must install program.current to recheck the SAME
 // GC, log manager, checked selection/epoch and retained parent at EVERY step.
 grand_logging_detail::Engine operation(self->journal_,self->source_->program_,[self]{self->require_current();});
 return operation.run();
}
}




