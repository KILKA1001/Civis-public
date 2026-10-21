// AFTER complete direct caller, actual5761 return, GC/cursor implementations.
// This is a distinct issuer. No legacy parsed/ChildSlot10 frame is fabricated.
struct BnwWorldViewControlTree::ContextDirectPostLuaFrame final {
 struct Domain {std::shared_ptr<Core> core;std::uint64_t topology,effects,sequence;};
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<Core> actual;
 const std::shared_ptr<ContextDirectLoadFrame> caller;
 const std::shared_ptr<BnwWorldViewControlTree> tree;
 const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn> initialized;
 const std::shared_ptr<const detail::NotificationContextPostreadRootCDataReturn> cdata;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t source_scope,serial;
 std::vector<Domain> domains;
 std::vector<std::shared_ptr<Core::InitialLuaMutationJournal>> journals;
 context_constructor_suffix_detail::NamedReceiverMap588* alias_map=nullptr;
 std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> alias_value;
 std::size_t alias_after_count=0;
 std::uint64_t alias_after_revision=0;
 bool alive=true,returned=false;
 ContextDirectPostLuaFrame(std::shared_ptr<Core> root,std::shared_ptr<ContextDirectLoadFrame> outer,
  std::shared_ptr<BnwWorldViewControlTree> retained,
  std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn> completed)
  :actual(std::move(root)),caller(std::move(outer)),tree(std::move(retained)),initialized(std::move(completed)),
   cdata(caller->state->impl_->postread_root_cdata_return),operation(caller->operation),
   source_scope(caller->source_scope),serial(caller->serial){
  require_context_initialization_return_owned(actual,initialized);caller->full();
  require(tree&&tree->core_==actual&&cdata&&!actual->context_initial_lua_started&&
   !actual->context_initial_lua_gc_return&&!actual->context_cursor_restore_return&&!actual->context_loader_cleanup_return,
   "Direct initialLua genuine new constructor/5761/CData occurrence absent");
  for(const auto&entry:initialized->impl_->domains){
   const auto core=entry.core.lock();require(core&&core->topology_revision==entry.topology&&core->effect_revision==entry.effects,
    "Direct initialLua actual5761 domain afterstate changed");
   require(core->source==actual->source&&!core->initial_lua_mutation_journal,
    "Direct initialLua foreign domain or nested writer");
   domains.push_back({core,entry.topology,entry.effects,core->layout_sequence});
  }
  require(!domains.empty()&&std::any_of(domains.begin(),domains.end(),[&](const auto&d){return d.core==actual;}),
   "Direct initialLua actual root domain absent");
  tree->require_context_direct_initialization_history_owned(*operation,initialized); // outer idle VM query only.
 }
 void poison()noexcept {caller->failed.store(true);for(const auto&d:domains)d.core->failed.store(true);}
 void first()const{
  require(creator==std::this_thread::get_id()&&alive&&caller&&caller->creator==creator&&
   caller->alive.load()&&!caller->failed.load(),"Direct Lua tail lexical caller retired FIRST");
  actual->layout_origin();
  actual->source->require_operation_origin_without_callbacks(*operation,source_scope);
  require(operation->operation_serial()==serial&&actual->context_direct_load_frame.lock()==caller&&
   caller->core.lock()==actual&&caller->source==actual->source&&caller->state==actual->context_ordinary_state&&
   caller->constructor==caller->state->impl_->native_constructor_return&&
   caller->manager==actual->context_gui_port&&caller->cursor==actual->context_cursor_load_prefix&&
   caller->prepared==actual->context_lua_preparation&&actual->context_layout_baseline&&
   !actual->context_layout_baseline->parsed_source_serial_&&
   actual->layout_initialization_return&&actual->layout_initialization_return->native_return_==initialized&&
   caller->state->impl_->postread_root_cdata_return==cdata,
   "Direct Lua tail actual constructor/read/5761/518/GUI/CData identity changed");
  const auto&done=*initialized->impl_;
  require(done.lifetime&&done.lifetime->creator==creator&&!done.lifetime->failed.load()&&
   done.lifetime->destroyed&&done.lifetime->destruction_passed&&done.core.lock()==actual&&
   !actual->context_initialization_active&&!actual->busy,
   "Direct Lua tail actual5761 body/destruction history retired");
  require(journals.empty()||journals.size()==domains.size(),"Direct Lua tail writer domains changed");
  if(alias_map){
   auto*const port=caller->constructor->impl_->port.get();
   require(port&&port->map588&&&*port->map588==alias_map&&alias_value&&
    alias_map->cells.size()>=alias_after_count&&alias_map->revision>=alias_after_revision,
    "Direct515120 actual constructor/map mutation history changed");
   const auto cell=alias_map->cells.find(content::saved_info_type_hash(".."));
   require(cell!=alias_map->cells.end()&&cell->second==alias_value,"Direct515120 actual first-wins cell changed");
  }
  for(std::size_t i=0;i<domains.size();++i){
   const auto&d=domains[i];d.core->layout_origin();
   require(d.core->source==actual->source&&d.core->topology_revision==d.topology,"Direct Lua topology/source changed");
   if(journals.empty()){
    require(!d.core->initial_lua_mutation_journal&&d.core->effect_revision==d.effects&&d.core->layout_sequence==d.sequence,
     "Direct Lua tail unrecorded store outside its actual stage");
   }else{
    const auto&j=journals[i];
    require(j&&d.core->initial_lua_mutation_journal==j&&j->creator==creator&&j->operation==operation&&j->serial==serial&&
     j->topology==d.topology&&j->before_effects==d.effects&&j->before_sequence==d.sequence&&
     j->effects==d.core->effect_revision&&j->sequence==d.core->layout_sequence,
     "Direct Lua tail writer journal is not SAME preimage/current domain");
    auto effects=j->before_effects,sequence=j->before_sequence;
    for(const auto&store:j->stores){
     const auto step=store.writer==Core::InitialLuaWriter::receiver_store?1u:0u;
     require(store.before_effects==effects&&effects<UINT64_MAX&&store.after_effects==effects+1&&
      store.before_sequence==sequence&&sequence<=UINT64_MAX-step&&store.after_sequence==sequence+step,
      "Direct Lua tail journal gap/replay/unknown store");
     effects=store.after_effects;sequence=store.after_sequence;
    }
    require(effects==j->effects&&sequence==j->sequence,"Direct Lua tail physical afterstate lacks writer history");
   }
  }
  operation->require_origin_without_callbacks();actual->layout_origin();
 }
 // No nested prepared-state query inside a real allocating/busy Lua callback.
 // The typed Host frame owns physical SAME518/env/ref validation there.
 void full()const {first();operation->require_current();first();}
 void begin_writers_owned(){
  full();require(journals.empty(),"Direct Lua journal reentry");journals.reserve(domains.size());
  for(const auto&d:domains){
   require(!d.core->initial_lua_mutation_journal,"Direct Lua foreign current journal");
   journals.push_back(std::make_shared<Core::InitialLuaMutationJournal>(*operation,d.topology,d.effects,d.sequence));
  }
  // All allocating journal preparation precedes publication or any Lua store.
  for(std::size_t i=0;i<domains.size();++i)domains[i].core->initial_lua_mutation_journal=journals[i];
  full();
 }
 void seal_writers_owned(){first();for(const auto&j:journals){require(!j->sealed,"Direct Lua duplicate journal seal");j->sealed=true;}first();}
 void adopt_sealed_writers_owned(){
  first();require(journals.size()==domains.size(),"Direct Lua returned journal domain count differs");
  for(const auto&j:journals)require(j->sealed,"Direct Lua adoption before actual body/GC/callback return");
  // No callback or allocation between closed validation and row advancement.
  for(std::size_t i=0;i<domains.size();++i){
   domains[i].effects=journals[i]->effects;domains[i].sequence=journals[i]->sequence;
   domains[i].core->initial_lua_mutation_journal.reset();
  }
  journals.clear();
  caller->accept_store_without_callbacks(); // only the closed writer return earns this root row.
  first();caller->full();full();
 }
};

struct detail::NotificationContextDirectLuaTailInput::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<BnwWorldViewControlTree::ContextDirectPostLuaFrame> frame;
 explicit Impl(std::shared_ptr<BnwWorldViewControlTree::ContextDirectPostLuaFrame> actual):frame(std::move(actual)){}
};
detail::NotificationContextDirectLuaTailInput::NotificationContextDirectLuaTailInput(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
detail::NotificationContextDirectLuaTailInput::~NotificationContextDirectLuaTailInput()=default;
void detail::NotificationContextDirectLuaTailInput::require_current_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan&op)const{
 require(impl_&&impl_->creator==std::this_thread::get_id()&&impl_->frame&&impl_->frame->operation==&op,
  "Direct Lua input creator/actual operation changed");impl_->frame->first();
 require(impl_->frame->actual->context_direct_lua_tail_input.lock().get()==this,"Direct Lua input replaced");impl_->frame->first();
}
void detail::NotificationContextDirectLuaTailInput::require_current_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op)const{
 require_current_without_callbacks(op);impl_->frame->full();require_current_without_callbacks(op);
}
void detail::NotificationContextDirectLuaTailInput::require_root_cdata_return_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op,const NotificationContextPostreadRootCDataReturn&returned)const{
 require_current_without_callbacks(op);
 require(impl_->frame->cdata.get()==&returned,"Direct Lua input is not SAME actual returned root CData");
 require_current_without_callbacks(op);
}
std::shared_ptr<BnwWorldViewControlTree> detail::NotificationContextDirectLuaTailInput::retain_actual_tree_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op)const{
 require_current_owned(op);const auto tree=impl_->frame->tree;
 require(tree&&tree->core_==impl_->frame->actual,"Direct Lua retained constructor Tree changed");
 require_current_owned(op);return tree;
}
std::weak_ptr<const void> detail::NotificationContextDirectLuaTailInput::actual_core_identity_owned(
 const runtime::BnwWorldViewSourceOperationLoan&op)const{
 require_current_owned(op);return impl_->frame->actual;
}

struct detail::NotificationContextDirectLoadReturn::Impl final {
 struct Domain {std::weak_ptr<BnwWorldViewControlTree::Core> core;std::uint64_t topology,effects,sequence;};
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<BnwWorldViewControlTree::Core> core;
 const std::shared_ptr<BnwWorldViewControlTree::ContextDirectLoadFrame> caller;
 const std::shared_ptr<const NotificationLayoutContextInitializationReturn> initialized;
 const std::shared_ptr<const NotificationContextInitialLuaGcReturn> gc;
 const std::shared_ptr<const NotificationContextCursorRestoreReturn> cursor;
 const std::shared_ptr<const NotificationContextLoaderCleanupReturn> cleanup;
 const std::shared_ptr<const NotificationContextPostreadRootCDataReturn> cdata;
 const runtime::LuaScriptHost*const host;
 const std::uint64_t serial;
 const std::vector<Domain> domains;
 context_constructor_suffix_detail::NamedReceiverMap588*const alias_map;
 const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> alias_value;
 const std::size_t alias_after_count;
 const std::uint64_t alias_after_revision;
 mutable bool published=false;
 explicit Impl(const BnwWorldViewControlTree::ContextDirectPostLuaFrame&frame,
  const runtime::LuaScriptHost& actual_host)
  :core(frame.actual),caller(frame.caller),initialized(frame.initialized),gc(frame.actual->context_initial_lua_gc_return),
   cursor(frame.actual->context_cursor_restore_return),cleanup(frame.actual->context_loader_cleanup_return),cdata(frame.cdata),
   host(&actual_host),serial(frame.serial),
   domains([&]{std::vector<Domain> result;for(const auto&d:frame.domains)result.push_back({d.core,d.topology,d.effects,d.sequence});return result;}()),
   alias_map(frame.alias_map),alias_value(frame.alias_value),alias_after_count(frame.alias_after_count),alias_after_revision(frame.alias_after_revision){}
};
detail::NotificationContextDirectLoadReturn::NotificationContextDirectLoadReturn(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
detail::NotificationContextDirectLoadReturn::~NotificationContextDirectLoadReturn()=default;

void BnwWorldViewControlTree::execute_context_direct_initial_lua_owned(
 const runtime::BnwWorldViewSourceOperationLoan&operation,
 const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>&initialized){
 const auto actual=core_;actual->layout_origin();const auto caller=actual->context_direct_load_frame.lock();
 require(caller&&caller->operation==&operation,"Direct initialLua actual caller absent");
 require_context_initialization_return_owned(actual,initialized);caller->full();
 const auto retained=actual->layout_resource_tree.lock();
 require(retained&&retained->core_==actual&&retained.get()==actual->layout_resource_actual_tree&&
  actual->layout_resource_operation==&operation&&actual->layout_resource_serial==caller->serial,
  "Direct initialLua retained actual constructor/resource Tree changed");
 auto tree=std::const_pointer_cast<BnwWorldViewControlTree>(retained);
 auto frame=std::make_shared<ContextDirectPostLuaFrame>(actual,caller,std::move(tree),initialized);
 auto input=std::shared_ptr<const detail::NotificationContextDirectLuaTailInput>(
  new detail::NotificationContextDirectLuaTailInput(std::make_shared<const detail::NotificationContextDirectLuaTailInput::Impl>(frame)));
 require(actual->context_direct_lua_tail_input.expired(),"Direct initialLua input replay");
 actual->context_direct_lua_tail_input=input;
 struct Exit {std::shared_ptr<ContextDirectPostLuaFrame>frame;~Exit(){if(!frame->returned)frame->poison();frame->alive=false;}}exit{frame};
 try{
  frame->full();
  require(actual->context_parent_kind.has_value(),"Direct initialLua actual Parent3C issuer absent");
  Core::ContextNamedReceiver receiver;
  if(*actual->context_parent_kind==Core::ContextParentKind::native_null){
   require(actual->context_parent_core.expired()&&!actual->context_parent_control_id,"Direct initialLua captured native NULL parent differs");
   receiver={actual,0};
  }else{
   const auto parent=actual->context_parent_core.lock();require(parent&&parent->source==actual->source,"Direct initialLua actual parent retired/foreign");
   if(*actual->context_parent_kind==Core::ContextParentKind::canonical_context)
    require(!actual->context_parent_control_id,"Direct initialLua Context parent has foreign control");
   else require(*actual->context_parent_kind==Core::ContextParentKind::canonical_control&&actual->context_parent_control_id&&
    Core::node(parent->nodes,actual->context_parent_control_id).alive,"Direct initialLua actual control parent absent");
   receiver={parent,actual->context_parent_control_id};
  }
  // Both Original4576CF/4576E4 operands are "..". The actual515120
  // receiver is this constructor's numeric map588, not a parallel string map.
  const auto parent_core=receiver.core.lock();require(parent_core!=nullptr,"Direct515120 selected parent retired");
  const auto selected_receiver=receiver.control?Core::node(parent_core->nodes,receiver.control).layout_baseline->base_.receiver_owned:
   parent_core->context_layout_baseline->base_.receiver_owned;
  require(selected_receiver!=nullptr,"Direct515120 genuine ordinary parent receiver absent");
  auto*const port=caller->constructor->impl_->port.get();require(port&&port->map588,"Direct515120 actual constructor map588 absent");
  port->require_completed();auto*const map=&*port->map588;const auto key=content::saved_info_type_hash("..");
  const auto old=map->cells.find(key);const bool absent=old==map->cells.end();
  const auto old_value=absent?std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>{}:old->second;
  const auto before_count=map->cells.size();const auto before_revision=map->revision;bool inserted=false,stored=false;
  const auto alias_full=[&]{frame->full();port->require_completed();
   require(caller->constructor->impl_->port.get()==port&&port->map588&&&*port->map588==map&&
    map->cells.size()==before_count+(inserted?1u:0u)&&
    map->revision==before_revision+(inserted?1u:0u)+(stored?1u:0u),"Direct515120 exact preimage/earned stage changed");
   const auto cell=map->cells.find(key);require((absent&&!inserted)?cell==map->cells.end():
    cell!=map->cells.end()&&cell->second==(stored?selected_receiver:old_value),"Direct515120 actual cell changed across FULL");};
  alias_full();require(before_revision<=UINT64_MAX-(absent?2u:1u),"Direct515120 map revision exhausted");
  if(absent){context_constructor_suffix_detail::NamedReceiverMap588::Cells detached;
   detached.emplace(key,std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>{});alias_full();
   auto node=detached.extract(key);alias_full();const auto result=map->cells.insert(std::move(node));
   require(result.inserted&&result.node.empty(),"Direct515120 real firstNULL insertion failed");
   inserted=true;++map->revision;alias_full();}
  if(!map->cells.find(key)->second){alias_full();auto cell=map->cells.find(key);
   require(cell!=map->cells.end()&&!cell->second,"Direct515120 real secondNULL lookup changed");
   cell->second=selected_receiver;stored=true;++map->revision;alias_full();}
  frame->alias_map=map;frame->alias_value=map->cells.find(key)->second;
  frame->alias_after_count=before_count+(inserted?1u:0u);
  frame->alias_after_revision=before_revision+(inserted?1u:0u)+(stored?1u:0u);
  frame->full();actual->context_initial_lua_started=true;
  const auto prior=actual->context_named_receivers.find("..");
  if(prior==actual->context_named_receivers.end())actual->context_named_receivers.emplace("..",receiver);
  else require(!prior->second.core.expired(),"Direct initialLua existing nonNULL parent alias retired");
  const auto before_topology=actual->topology_revision,before_effects=actual->effect_revision;
  const auto before_sequence=actual->context_layout_baseline->sequence_;
  frame->begin_writers_owned();
  frame->cdata->invoke_direct_initial_lua_owned(operation,input);frame->first();
  frame->cdata->require_direct_initial_lua_state_owned(operation,input);frame->first();
  frame->seal_writers_owned();
  std::vector<detail::NotificationContextInitialLuaGcReturn::Impl::Domain> history;
  history.reserve(frame->domains.size());
  for(std::size_t i=0;i<frame->domains.size();++i)history.push_back({frame->domains[i].core,frame->journals[i]});
  const auto gc_body=std::make_shared<const detail::NotificationContextInitialLuaGcReturn::Impl>(
   actual,operation,before_topology,before_effects,before_sequence,std::move(history));
  frame->first();actual->context_initial_lua_gc_return=std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn>(
   new detail::NotificationContextInitialLuaGcReturn(gc_body));frame->first();
  frame->adopt_sealed_writers_owned();
  const auto prefix=actual->context_cursor_load_prefix;const auto manager=actual->context_gui_port;
  frame->full();require(prefix&&prefix->impl_&&prefix->impl_->parser_serial==0&&
   prefix->impl_->core.lock()==actual&&prefix->impl_->manager==manager,
   "Direct cursor restore actual nonparser prefix changed");
  const auto changed=manager->set_cursor_owned(operation,prefix->impl_->previous_cursor);frame->first();
  require(changed.current==prefix->impl_->previous_cursor,"Direct cursor restoration did not return saved prior cursor");
  const auto restored_body=std::make_shared<const detail::NotificationContextCursorRestoreReturn::Impl>(
   actual,operation,actual->context_initial_lua_gc_return,changed,
   detail::NotificationContextCursorRestoreReturn::Impl::DirectLoadTag{});
  frame->full();actual->context_cursor_restore_return=std::shared_ptr<const detail::NotificationContextCursorRestoreReturn>(
   new detail::NotificationContextCursorRestoreReturn(restored_body));frame->full();
  // Original reads current flags only AFTER the real cursor restore.
  if(actual->context_layout_baseline->base_.flags_40&0x8000000u){
   const bool hidden=(actual->context_layout_baseline->base_.flags_40&2u)!=0;
   frame->begin_writers_owned();
   frame->cdata->invoke_direct_initial_showhide_owned(operation,input,actual->context_cursor_restore_return,hidden);
   frame->first();frame->seal_writers_owned();frame->adopt_sealed_writers_owned();
  }
  frame->full();
  actual->context_loader_cleanup_return=frame->cdata->cleanup_direct_loader_temporaries_owned(
   operation,input,actual->context_cursor_restore_return);
  require(actual->context_loader_cleanup_return!=nullptr,"Direct normal individual cleanup did not return");
  frame->full();require(!actual->context_direct_load_pending_return&&!actual->context_direct_load_return,"Direct pending completion replay");
  const auto& actual_host=operation.context_lua_host_owned();frame->full();
  const auto pending=std::shared_ptr<const detail::NotificationContextDirectLoadReturn>(
   new detail::NotificationContextDirectLoadReturn(std::make_shared<const detail::NotificationContextDirectLoadReturn::Impl>(*frame,actual_host)));
  frame->full();actual->context_direct_load_pending_return=pending;frame->full();frame->returned=true;
 }catch(...){frame->poison();throw;}
}

void BnwWorldViewControlTree::finalize_context_direct_load_return_owned(
 const runtime::BnwWorldViewSourceOperationLoan&operation){
 const auto actual=core_;actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
 const auto pending=actual->context_direct_load_pending_return;
 require(pending&&pending->impl_&&!actual->context_direct_load_return,"Direct normal lexical completion absent/replayed");
 const auto&done=*pending->impl_;const auto caller=done.caller;
 require(done.creator==std::this_thread::get_id()&&done.core.lock()==actual&&caller&&
  caller->creator==done.creator&&!caller->alive.load()&&!caller->failed.load()&&caller->normal_returned&&caller->destruction_passed&&
  caller->core.lock()==actual&&!done.published&&done.gc==actual->context_initial_lua_gc_return&&
  done.cursor==actual->context_cursor_restore_return&&done.cleanup==actual->context_loader_cleanup_return,
  "Direct normal completion requires actual returned Lua/GC/cursor/cleanup and caller lexical destruction");
 require_context_direct_initialization_history_owned(operation,done.initialized);
 for(const auto&entry:done.domains){const auto core=entry.core.lock();require(core&&core->source==actual->source&&
  core->topology_revision==entry.topology&&core->effect_revision==entry.effects&&core->layout_sequence==entry.sequence&&
  !core->initial_lua_mutation_journal,"Direct normal completion current recorded domain changed");}
 done.published=true;actual->context_direct_load_return=pending;actual->context_direct_load_pending_return.reset();
 (void)require_context_direct_load_return_without_callbacks(actual,operation);
}
std::shared_ptr<const detail::NotificationContextDirectLoadReturn>
BnwWorldViewControlTree::require_context_direct_load_return_without_callbacks(
 const std::shared_ptr<Core>&actual,const runtime::BnwWorldViewSourceOperationLoan&operation){
 require(actual&&actual->creator==std::this_thread::get_id()&&actual->facade_alive.load()&&!actual->failed.load(),
  "Direct normal receiver missing/retired before busy state");
 const bool expected_root_busy=require_normal_root_or_idle_origin_without_callbacks(actual);
 operation.require_origin_without_callbacks();
 require(require_normal_root_or_idle_origin_without_callbacks(actual)==expected_root_busy,"Direct normal actual entered scope changed");
 require(&actual->source->current_operation_loan()==&operation,"Direct normal historical return has foreign current Source");
 const auto returned=actual->context_direct_load_return;
 require(returned&&returned->impl_,"Direct normal complete return absent");const auto&done=*returned->impl_;
 require(done.creator==std::this_thread::get_id()&&done.published&&done.core.lock()==actual&&
  done.caller&&done.caller->creator==done.creator&&!done.caller->alive.load()&&!done.caller->failed.load()&&
  done.caller->normal_returned&&done.caller->destruction_passed&&done.caller->core.lock()==actual&&
  done.caller->source==actual->source&&actual->context_ordinary_state==done.caller->state&&
  actual->context_layout_baseline&&!actual->context_layout_baseline->parsed_source_serial_&&
  actual->context_gui_port==done.caller->manager&&actual->context_lua_preparation==done.caller->prepared&&
  actual->layout_initialization_return&&actual->layout_initialization_return->native_return_==done.initialized&&
  actual->context_initial_lua_gc_return==done.gc&&actual->context_cursor_restore_return==done.cursor&&
  actual->context_loader_cleanup_return==done.cleanup&&done.gc&&done.cursor&&done.cleanup&&
  actual->context_ordinary_state->impl_->postread_root_cdata_return==done.cdata,
  "Direct normal actual constructor/5761/Lua/GC/cursor/cleanup/lifetime history changed");
 require(done.initialized->impl_->lifetime&&done.initialized->impl_->lifetime->destroyed&&
  done.initialized->impl_->lifetime->destruction_passed&&!done.initialized->impl_->lifetime->failed.load(),
  "Direct normal genuine5761 destruction history retired");
 const auto&init=*done.initialized->impl_;const auto origin=init.origin;
 require(origin&&origin==actual->layout_context_initialization_origin&&origin->direct_caller_==done.caller&&
  origin->direct_constructor_==done.caller->constructor&&origin->direct_read_&&
  actual->context_construction_return==origin->construction_&&origin->tree_.lock()==actual&&
  origin->topology_==actual->topology_revision&&origin->parser_serial_==0&&
  init.source_serial==origin->direct_serial_&&origin->direct_serial_==done.serial&&
  origin->direct_scope_==done.caller->source_scope&&init.lifetime->creator==done.creator&&
  !actual->context_initialization_active&&init.core.lock()==actual&&
  done.caller->state->impl_&&done.caller->state->impl_->creator==done.creator&&
  done.caller->state->impl_->alive.load()&&!done.caller->state->impl_->failed.load()&&
  done.caller->state->impl_->core.lock()==actual&&done.caller->state->impl_->source.lock()==actual->source&&
  done.caller->constructor->impl_&&done.caller->constructor->impl_->creator==done.creator&&
  done.caller->constructor->impl_->alive.load()&&!done.caller->constructor->impl_->failed.load(),
  "Direct normal immutable5761 origin/native constructor custody changed");
 const auto read=origin->direct_read_->impl_;
 require(read&&read->creator==done.creator&&read->alive.load()&&read->core.lock()==actual&&read->source==actual->source&&
  read->canonical.lock()==origin->direct_read_&&actual->context_root_read_pending_completion==origin->direct_read_&&
  read->scope==origin->direct_scope_&&read->serial==origin->direct_serial_&&!read->staged_return&&read->completed_return,
  "Direct normal completed RootRead private publisher changed");
 const auto completed=read->completed_return;const auto read_frame=completed->returned_frame;
 const auto evidence=completed->returned_cursor;
 using Cleanup=ContextRootReadFileFrame::Cleanup;
 static constexpr std::array cleanup_order{Cleanup::cursor34_8353f0,Cleanup::string18_retire,Cleanup::lua2c_8f8820,
  Cleanup::cursor10_8353f0,Cleanup::wide280_686f10,Cleanup::wide68_686f10};
 using CursorType=context_root_read_temporary_detail::XmlCursor::Type;
 using NameLife=context_root_read_temporary_detail::NameString::Life;
 require(read_frame&&evidence&&read_frame==read->actual_frame.lock()&&evidence==read->actual_cursor.lock()&&
  completed->actual_pending.lock()==origin->direct_read_&&completed->cleanup_effects==cleanup_order&&
  !read_frame->alive.load()&&!read_frame->failed.load()&&!evidence->alive.load()&&!evidence->failed.load()&&
  read_frame->creator==done.creator&&evidence->creator==done.creator&&evidence->frame.lock()==read_frame&&
  evidence->core.lock()==actual&&read_frame->core.lock()==actual&&evidence->pending_completion.lock()==origin->direct_read_&&
  evidence->ordinal==evidence->child_count&&evidence->cleanup_count==cleanup_order.size()&&
  read_frame->cleanup_count==cleanup_order.size()&&evidence->cleanup_effects==cleanup_order&&
  read_frame->cleanup_effects==cleanup_order&&!evidence->current_instance&&!evidence->current_style&&
  evidence->document==read_frame->expected_xml84&&evidence->document&&evidence->document->storage&&
  evidence->document->storage->document&&evidence->document->storage->document->children.size()==evidence->child_count&&
  read->state==actual->context_ordinary_state&&read_frame->state.lock()==read->state&&
  read->state->impl_->native_constructor_return==read_frame->constructor&&read_frame->constructor==origin->direct_constructor_&&
  read->state->impl_->b4_value==read_frame->actual_b4&&read->state->impl_->b4_revision==read_frame->b4_revision&&
  read->state->impl_->file_510_value==read_frame->expected_510&&read->state->impl_->xml84_current==read_frame->expected_xml84&&
  read->state->impl_->file_watcher_b8==read_frame->expected_watcher_b8&&
  actual->context_layout_baseline->base_.receiver_owned==read->receiver&&evidence->receiver==read->receiver&&
  read_frame->receiver==read->receiver&&evidence->cursor34.type==CursorType::retired_base_b76284&&
  !evidence->cursor34.selected&&evidence->cursor10.type==CursorType::retired_base_b76284&&
  evidence->name_string18.life==NameLife::retired_canonical_empty&&evidence->name_string18.value.empty()&&
  evidence->named_retirement&&read_frame->temporary280&&read_frame->temporary68&&
  actual->context_layout_children&&actual->context_root_read_file_frame.expired(),
  "Direct normal real RootRead six-cleanup/destruction/physical resource custody changed");
 read_frame->temporary280->require_retired_without_callbacks();read_frame->temporary68->require_retired_without_callbacks();
 auto*const port=done.caller->constructor->impl_->port.get();
 require(port&&port->map588&&&*port->map588==done.alias_map&&done.alias_value&&
  done.alias_map->cells.size()>=done.alias_after_count&&done.alias_map->revision>=done.alias_after_revision,
  "Direct normal actual515120 history changed");
 const auto alias=done.alias_map->cells.find(content::saved_info_type_hash(".."));
 require(alias!=done.alias_map->cells.end()&&alias->second==done.alias_value,"Direct normal actual515120 alias replaced");
 for(const auto&entry:done.domains){const auto core=entry.core.lock();require(core!=nullptr,"Direct normal domain retired");core->layout_origin(core==actual?expected_root_busy:false);
  require(core->source==actual->source&&core->topology_revision==entry.topology&&core->effect_revision==entry.effects&&
   core->layout_sequence==entry.sequence&&!core->initial_lua_mutation_journal,
   "Direct normal current domain requires genuine postLua/return writer history");}
 // Historical caller operation pointer and old source serial are not read.
 operation.require_origin_without_callbacks();
 require(require_normal_root_or_idle_origin_without_callbacks(actual)==expected_root_busy,"Direct normal actual entered scope changed at POST");
 return returned;
}
std::shared_ptr<const detail::NotificationContextDirectLoadReturn>
BnwWorldViewControlTree::require_context_direct_load_return_owned(const runtime::BnwWorldViewSourceOperationLoan&operation)const{
 const auto returned=require_context_direct_load_return_without_callbacks(core_,operation);
 require(&operation.context_lua_host_owned()==returned->impl_->host,"Direct normal actual Host/VM incarnation changed");
 require_context_direct_initialization_history_owned(operation,returned->impl_->initialized);
 require_context_direct_load_return_without_callbacks(core_,operation);return returned;
}
