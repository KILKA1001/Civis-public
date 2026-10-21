//Lexical body in the SAME direct457270 caller AFTER real completedRead514
//copy/append. Executing OWN x64 selected-positive-loose7493B0 return, then
//actual598490/4E8040/544D50 and real first71C790(+2E4) effects in order.
{
 using PathReturn=State::Impl::Lua514PathReturn;
 using Completion=detail::NotificationContextRootReadCompletionSlot;
 using Watcher=detail::NotificationContextRootFileWatcher;
 const auto first=[&]{
  require(frame&&frame->creator==std::this_thread::get_id()&&frame->alive.load()&&!frame->failed.load(),
   "Lua514 resolver actual direct caller retired FIRST");
  owner->require_operation_origin_without_callbacks(operation,source_scope);
  operation.require_origin_without_callbacks();actual->layout_origin();
  require(State::Impl::same(actual->source,owner)&&State::Impl::same(frame->source,owner)&&
   frame->operation==&operation&&frame->serial==operation.operation_serial()&&frame->source_scope==source_scope&&
   actual->context_direct_load_frame.lock()==frame&&State::Impl::same(actual->context_ordinary_state,state)&&
   state&&state->impl_&&state->impl_->creator==std::this_thread::get_id()&&state->impl_->alive.load()&&
   !state->impl_->failed.load()&&State::Impl::same(state->impl_->native_constructor_return,constructor)&&
   State::Impl::same(actual->context_lua_preparation,prepared)&&State::Impl::same(frame->prepared,prepared)&&
   State::Impl::same(actual->context_gui_port,manager)&&actual->context_layout_baseline,
   "Lua514 resolver SAME actual Source/VM518/Context/direct caller changed");
 };
 first();const auto prefix=state->impl_->lua_name514_prefix_return;
 const auto read=actual->context_root_read_pending_completion;
 require(prefix&&read&&prefix->read.lock()==read&&!state->impl_->lua514_path_return&&
  !state->impl_->file_watcher_2e4,"Lua514 resolver requires real514 prefix and first actual2E4 occurrence");
 const auto read_guard=Completion::Guard(read->impl_);
 read_guard.require_completed_current_without_callbacks();first();
 const auto receiver=actual->context_layout_baseline->base_.receiver_owned;
 require(receiver!=nullptr,"Lua514 resolver actual current Context receiver absent");
 read_guard.require_actual_root_receiver_without_callbacks(*receiver);first();
 const auto logical514=state->impl_->file_514_value;
 const auto b4=state->impl_->b4_value;
 require(logical514&&State::Impl::same(logical514,prefix->value)&&prefix->creator==std::this_thread::get_id()&&
  prefix->serial==frame->serial&&prefix->source_scope==source_scope&&state->impl_->file_514_revision==2&&
  State::Impl::same(prefix->b4,b4)&&prefix->b4_revision==state->impl_->b4_revision&&
  prefix->effects==std::array{State::Impl::LuaName514PrefixReturn::Effect::copy_current_b4,
      State::Impl::LuaName514PrefixReturn::Effect::append_lua},
  "Lua514 resolver query is not SAME current actual copy/append value");
 const auto topology=actual->topology_revision,effects=actual->effect_revision,sequence=actual->layout_sequence;
 const auto root_sequence=actual->context_layout_baseline->sequence_;
 auto expected514=logical514;std::uint64_t expected_revision=2;
 std::shared_ptr<ContextRootWatcherCustody> expected_watcher;
 const auto current=[&]{
  first();read_guard.require_completed_current_without_callbacks();first();
  require(State::Impl::same(actual->context_root_read_pending_completion,read)&&
   State::Impl::same(actual->context_layout_baseline->base_.receiver_owned,receiver)&&
   state->impl_->lua_name514_prefix_return==prefix&&!state->impl_->lua514_path_return&&
   State::Impl::same(state->impl_->b4_value,b4)&&state->impl_->b4_revision==prefix->b4_revision&&
   State::Impl::same(state->impl_->file_514_value,expected514)&&state->impl_->file_514_revision==expected_revision&&
   State::Impl::same(state->impl_->file_watcher_2e4,expected_watcher)&&actual->topology_revision==topology&&
   actual->effect_revision==effects&&actual->layout_sequence==sequence&&actual->context_layout_baseline->sequence_==root_sequence,
   "Lua514 resolver actual afterRead fields/owned514/2E4 history changed");
  constructor->impl_->port->require_completed();
  require(constructor->impl_->port->digest_2e4&&
   constructor->impl_->port->digest_2e4->handle_0==0xffffffff&&
   constructor->impl_->port->digest_2e4->handle_4==0xffffffff&&!constructor->impl_->port->digest_2e4->opened_8,
   "Lua514 first2E4 watcher lacks genuine inactive constructor witness");
 };
 const auto full=[&]{current();read_guard.require_source_owned(operation);current();};
 std::string query;query.reserve(logical514->value_.size());full();
 for(const auto unit:logical514->value_){
  require(unit>0&&unit<128,"Lua514 nonASCII query requires actual ACP converter/locale owner");
  query.push_back(static_cast<char>(unit));
 }
 full();const assets::AssetKey key(query);full();
 require(key.original_name()==query&&key.safe_relative_path()&&key.normalized_name().ends_with(".lua"),
  "Lua514 actual relative ASCII query lost semantics before resource lookup");
 //Resource lender performs the actual current namespace query on SAME Bank.
 //Its positive loose/offset0 proof is NOT a prepared_source filename or a
 //raw native821370 record/bit19 grant. Miss/archive/ambiguity stay strict.
 const auto physical=operation.context_lua_physical_source_owned(key);full();
 require(physical!=nullptr,"Lua514 genuine positive loaded loose physical source missing");
 physical->require_current_without_callbacks();
 const auto bank=operation.selected_provider_startup_bank_owned();full();
 physical->require_same_bank_without_callbacks(*bank);
 const auto bytes=physical->actual_bytes_owned();full();physical->require_same_bank_without_callbacks(*bank);
 const auto identity=physical->payload_identity_owned();full();physical->require_same_bank_without_callbacks(*bank);
 require(bytes&&bytes->size()==identity.bytes,"Lua514 actual loaded byte source retired/substituted");
 auto path=physical->physical_path_owned();full();physical->require_same_bank_without_callbacks(*bank);
 auto wide_path=path.u16string();full();physical->require_same_bank_without_callbacks(*bank);
 require(!wide_path.empty()&&wide_path.size()<256&&
  std::find(wide_path.begin(),wide_path.end(),char16_t{})==wide_path.end(),
  "Lua514 selected positive loose path requires actual terminated<256 fixed260 route");
 //Actual output4F13F0 is born empty before4DC8C0; genuineNULL refs make
 //48A0F0 clear214/210 then20C/208 and reach407FB0 unconditionally.
 //Use actual original Shlwapi callees, never string-copy/canonical guesses.
 std::array<char16_t,260> fixed{};
 std::int32_t append_result=0,canonical_result=0;
 #if defined(_WIN32)
 {
  static_assert(sizeof(wchar_t)==sizeof(char16_t));
  struct SystemModuleLease final {
   HMODULE value=nullptr;
   bool release_attempted=false;
   ~SystemModuleLease()noexcept{
    if(value&&!release_attempted){release_attempted=true;
     if(FreeLibrary(value))value=nullptr;
     //Failed release retains the OS module reference; no future function
     //pointer/return escapes this failed/unwinding scope.
    }
   }
  } module;
  module.value=LoadLibraryExW(L"shlwapi.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);full();
  require(module.value!=nullptr,"Lua514 actual System32 Shlwapi owner unavailable");
  using Append=BOOL(WINAPI*)(LPWSTR,LPCWSTR);
  using Remove=void(WINAPI*)(LPWSTR);
  const auto append=reinterpret_cast<Append>(GetProcAddress(module.value,"PathAppendW"));full();
  const auto remove=reinterpret_cast<Remove>(GetProcAddress(module.value,"PathRemoveBlanksW"));full();
  const auto canonical=reinterpret_cast<Append>(GetProcAddress(module.value,"PathCanonicalizeW"));full();
  require(append&&remove&&canonical,"Lua514 required original Shlwapi exports unavailable");
  std::array<wchar_t,260> input{},live{},normalized{};
  for(std::size_t i=0;i<wide_path.size();++i)input[i]=static_cast<wchar_t>(wide_path[i]);
  full();
  append_result=append(live.data(),input.data());full(); //actual4DC8C9 ignoresBOOL
  remove(live.data());full(); //actual407FBA void; NULLcache branch beforethiscall
  canonical_result=canonical(normalized.data(),live.data());full();
  if(canonical_result==1)live=normalized; //actual407FE6 exactCMP1, all520 bytes
  for(std::size_t i=0;i<fixed.size();++i)fixed[i]=static_cast<char16_t>(live[i]);
  full();
  module.release_attempted=true;const auto released=FreeLibrary(module.value);
  if(released)module.value=nullptr;
  full();require(released!=FALSE,"Lua514 owned system module retirement failed; no resolver return");
 }
 full(); //actual module-scope DTOR before interpreting/output publication
 #else
 throw std::runtime_error("required real Win32 Lua514 path helper supplier on this platform");
 #endif
 const auto nul=std::find(fixed.begin(),fixed.end(),char16_t{});
 require(fixed[0]!=u'\0'&&nul!=fixed.end()&&static_cast<std::size_t>(nul-fixed.begin())<256&&
  fixed[255]!=u'/'&&fixed[255]!=u'\\',
  "Lua514 actual Shlwapi output requires chosen positive terminated<256 branch");
 std::u16string resolved_value(fixed.begin(),nul);full();
 auto resolved=std::shared_ptr<const Wide>(new Wide(std::move(resolved_value)));full();
 physical->require_same_bank_without_callbacks(*bank);
 const auto temporary=std::make_shared<const PathReturn::Temporary>(physical,resolved,fixed,append_result,canonical_result);full();
 physical->require_same_bank_without_callbacks(*bank);
 //749430 returns SAME actual temporary object.598490 compares its firstWORD
 //to0: positive nonempty =>AL0, therefore originalJNZ4574D1 is NOTtaken and
 //4E8040 returns that object's buffer.544D50 then updates liveContext514.
 require(temporary->fixed[0]!=u'\0'&&!temporary->reference214&&!temporary->reference20c,
  "Lua514 selected resolver did not genuinely return positive fixed-path temporary");
 auto assigned=temporary->value->value_;full();
 auto physical514=std::shared_ptr<const Wide>(new Wide(std::move(assigned)));full();
 require(physical514->value_==resolved->value_&&
  physical514->terminated_data_without_callbacks()[resolved->value_.size()]==u'\0',
  "Lua514 actual544D50 physical path/NUL assignment return unsupported");
 state->impl_->file_514_value=physical514;expected514=physical514;
 ++state->impl_->file_514_revision;++expected_revision;full();
 //Actual4574EB reaches SAME Context+2E4 with the returned temporary asarg
 //and flags0x10. Reuse genuine71C790 owner, independently from XML'sB8.
 auto watcher=Watcher::allocate_first_owned(temporary->fixed);full();
 auto quarantine=std::make_unique<ContextRootWatcherCustody::QuarantinedField>(watcher);full();
 auto custody=std::make_shared<ContextRootWatcherCustody>(std::move(quarantine));full();
 require(!state->impl_->file_watcher_2e4,"Lua514 actual2E4 watcher publisher replayed");
 state->impl_->file_watcher_2e4=custody;expected_watcher=custody;full();
 watcher->capture_file_attributes_owned();full();
 watcher->open_change_notification_owned();full();
 watcher->register_wait_owned();full();
 watcher->require_reached_without_callbacks();physical->require_same_bank_without_callbacks(*bank);full();
 //Return holds the actual temporary until original postread/script/GC/tail
 //cleanup, NOT an already-executed686F10 or whole457270 completion.
 auto returned=std::make_shared<const PathReturn>(read,prefix,temporary,physical,bytes,identity,physical514,custody,
  source_scope,frame->serial,topology,effects,sequence,root_sequence,query);full();
 state->impl_->lua514_path_return=returned;
 const auto published=[&]{
  first();read_guard.require_completed_current_without_callbacks();first();
  physical->require_same_bank_without_callbacks(*bank);first();
  require(state->impl_->lua514_path_return==returned&&returned->creator==std::this_thread::get_id()&&
   returned->read.lock()==read&&returned->prefix==prefix&&returned->temporary==temporary&&
   returned->scope==source_scope&&returned->serial==frame->serial&&returned->query==key.original_name()&&
   State::Impl::same(state->impl_->file_514_value,returned->value)&&state->impl_->file_514_revision==3&&
   State::Impl::same(state->impl_->file_watcher_2e4,returned->watcher)&&
   actual->topology_revision==returned->topology&&actual->effect_revision==returned->effects&&
   actual->layout_sequence==returned->sequence&&actual->context_layout_baseline->sequence_==returned->root_sequence,
   "Lua514 executing path/assignment/2E4 actual return changed before next native call");
  watcher->require_reached_without_callbacks();
 };
 published();read_guard.require_source_owned(operation);published();
} //No caller passes a bool/path/filename/ready seal; no VM resume here.
