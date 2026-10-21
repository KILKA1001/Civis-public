// Lexical PRIVATE fragment in the actual selected direct457270 caller,
// immediately AFTER the real7A9C30 RootRead has returned. It does not call
// RootRead/parser/constructor/Lua again and does not issue a5761/v10 seal.
// Original457486..4B1: fresh B4 ->514, then fresh514 length -> append4 '.lua'.
{
 using Completion=detail::NotificationContextRootReadCompletionSlot;
 using LuaNameReturn=State::Impl::LuaName514PrefixReturn;
 // Validate SAME Source/scope BEFORE the historical raw operation or after-
 // callback Core fields. The old direct frame's beforeRead sequence/effects
 // are deliberately NOT restamped: the real completedRead is the new lender.
 const auto first_post_read=[&]{
  require(frame&&frame->creator==std::this_thread::get_id()&&frame->alive.load()&&!frame->failed.load(),
   "PostRootRead514 actual direct caller retired/foreign/failed FIRST");
  owner->require_operation_origin_without_callbacks(operation,source_scope);
  operation.require_origin_without_callbacks();
  actual->layout_origin();
  require(State::Impl::same(actual->source,owner)&&State::Impl::same(frame->source,owner)&&
   actual->context_direct_load_frame.lock()==frame&&frame->operation==&operation&&frame->source_scope==source_scope&&
   frame->serial==operation.operation_serial()&&State::Impl::same(actual->context_ordinary_state,state)&&
   state&&state->impl_&&state->impl_->creator==std::this_thread::get_id()&&state->impl_->alive.load()&&
   !state->impl_->failed.load()&&State::Impl::same(state->impl_->native_constructor_return,constructor)&&
   State::Impl::same(actual->context_gui_port,manager)&&State::Impl::same(actual->context_lua_preparation,prepared)&&
   State::Impl::same(frame->prepared,prepared)&&State::Impl::same(frame->input,input)&&
   actual->context_layout_baseline&&actual->context_layout_baseline->base_.receiver_owned,
   "PostRootRead514 actual caller/receiver/ctor/518/Source changed");
 };
 first_post_read();
 const auto completed_read=actual->context_root_read_pending_completion;
 require(completed_read!=nullptr,"PostRootRead514 needs actual SAME completed RootRead, not pending DATA");
 const auto completed_guard=Completion::Guard(completed_read->impl_);
 completed_guard.require_completed_current_without_callbacks();first_post_read();
 const auto root_receiver=actual->context_layout_baseline->base_.receiver_owned;
 completed_guard.require_actual_root_receiver_without_callbacks(*root_receiver);first_post_read();
 // Native observes current B4 only AFTER7A9C30 normal return. No constructor
 // snapshot, normalized filename or authored XML may replace this operand.
 const auto b4_current=state->impl_->b4_value;
 const auto b4_revision=state->impl_->b4_revision;
 const auto before514=state->impl_->file_514_value;
 const auto initial514=constructor->impl_->port->wide514;
 require(b4_current&&before514&&initial514&&State::Impl::same(before514,initial514)&&
  before514->value_.empty()&&!state->impl_->file_514_revision&&!state->impl_->lua_name514_prefix_return&&
  !b4_current->value_.empty()&&b4_current->value_.size()<=static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())-4&&
  std::find(b4_current->value_.begin(),b4_current->value_.end(),char16_t{})==b4_current->value_.end()&&
  b4_current->terminated_data_without_callbacks()!=before514->terminated_data_without_callbacks(),
  "PostRootRead514 requires real first positive nonalias B4 and constructor-empty514");
 const auto after_read_topology=actual->topology_revision;
 const auto after_read_effects=actual->effect_revision;
 const auto after_read_sequence=actual->layout_sequence;
 const auto root_sequence=actual->context_layout_baseline->sequence_;
 auto expected514=before514;
 std::uint64_t expected_revision=0;
 const auto native_sequence=[&]{
  first_post_read();completed_guard.require_completed_current_without_callbacks();first_post_read();
  require(State::Impl::same(actual->context_root_read_pending_completion,completed_read)&&
   State::Impl::same(actual->context_layout_baseline->base_.receiver_owned,root_receiver)&&
   State::Impl::same(state->impl_->b4_value,b4_current)&&state->impl_->b4_revision==b4_revision&&
   State::Impl::same(state->impl_->file_514_value,expected514)&&state->impl_->file_514_revision==expected_revision&&
   !state->impl_->lua_name514_prefix_return&&actual->topology_revision==after_read_topology&&
   actual->effect_revision==after_read_effects&&actual->layout_sequence==after_read_sequence&&
   actual->context_layout_baseline->sequence_==root_sequence,
   "PostRootRead514 real operands/owned514 stores/afterRead topology changed");
  constructor->impl_->port->require_completed();
  completed_guard.require_actual_root_receiver_without_callbacks(*root_receiver);first_post_read();
 };
 const auto full=[&]{native_sequence();completed_guard.require_source_owned(operation);native_sequence();};
 std::array<LuaNameReturn::Effect,2> effects;
 std::size_t effect_count=0;
 const auto copy_length=b4_current->value_.size(); //actual current native signed length
 full();
 auto copy=b4_current->value_;full();
 auto copied514=std::shared_ptr<const Wide>(new Wide(std::move(copy)));full();
 require(copied514->value_.size()==copy_length&&copied514->value_==b4_current->value_&&
  copied514->terminated_data_without_callbacks()[copy_length]==u'\0',
  "PostRootRead514 actual81FEC0 copy/length/NUL return unsupported");
 // Genuine successful portable OWN UTF16 copy; no old x86 wrapper claim.
 state->impl_->file_514_value=copied514;expected514=copied514;
 ++state->impl_->file_514_revision;++expected_revision;
 effects[effect_count++]=LuaNameReturn::Effect::copy_current_b4;full();
 // Native loads [514] and its fresh length again before81F050. The suffix
 // comes from the qualified10B UTF16/NUL literalA3565C; source count is4.
 const auto current514=state->impl_->file_514_value;
 const auto current_length=current514->value_.size();
 require(current514==copied514&&current_length==copy_length,"PostRootRead514 fresh append operand changed");
 auto appended=current514->value_;full();
 appended.append(u".lua",4);full();
 auto returned514=std::shared_ptr<const Wide>(new Wide(std::move(appended)));full();
 require(returned514->value_.size()==current_length+4&&
  returned514->value_.compare(0,current_length,current514->value_)==0&&
  returned514->value_.compare(current_length,4,u".lua")==0&&
  returned514->terminated_data_without_callbacks()[current_length+4]==u'\0',
  "PostRootRead514 actual append4/current length/NUL return unsupported");
 state->impl_->file_514_value=returned514;expected514=returned514;
 ++state->impl_->file_514_revision;++expected_revision;
 effects[effect_count++]=LuaNameReturn::Effect::append_lua;full();
 require(effect_count==effects.size()&&expected_revision==2,"PostRootRead514 required two actual owned stores absent");
 // Private result captures only earned514 state. It grants neither native
 // path resolution/digest/script compile nor Context/Child/5761 completion.
 auto result=std::make_shared<const LuaNameReturn>(completed_read,b4_current,before514,copied514,returned514,
  source_scope,frame->serial,b4_revision,after_read_topology,after_read_effects,after_read_sequence,root_sequence,effects);
 full();
 state->impl_->lua_name514_prefix_return=result;
 const auto published=[&]{
  first_post_read();completed_guard.require_completed_current_without_callbacks();first_post_read();
  require(state->impl_->lua_name514_prefix_return==result&&result->creator==std::this_thread::get_id()&&
   result->read.lock()==completed_read&&result->source_scope==source_scope&&result->serial==frame->serial&&
   State::Impl::same(result->b4,b4_current)&&State::Impl::same(state->impl_->b4_value,result->b4)&&
   state->impl_->b4_revision==result->b4_revision&&State::Impl::same(state->impl_->file_514_value,result->value)&&
   state->impl_->file_514_revision==2&&result->effects==effects&&actual->topology_revision==result->topology&&
   actual->effect_revision==result->core_effects&&actual->layout_sequence==result->core_sequence&&
   actual->context_layout_baseline->sequence_==result->root_sequence,
   "PostRootRead514 owned completion/canonical physical514 changed before next call");
 };
 published();completed_guard.require_source_owned(operation);published();
} //All local copies/targets retire before the actual next resolver frontier.
