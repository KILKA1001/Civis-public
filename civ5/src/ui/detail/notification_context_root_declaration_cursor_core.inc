// PRIVATE Root7A9C child cursor. No ordinary id0 Runtime or cold XML metadata.
// Actual Controls lookup/Guard implementation is owned by Host, not this leaf.
struct BnwWorldViewControlTree::ContextRootStyleCursorValue final {
 const std::shared_ptr<ContextRootXml84> document;
 const content::XmlNode*const selected;
 const std::size_t ordinal;
 ContextRootStyleCursorValue(std::shared_ptr<ContextRootXml84> d,const content::XmlNode*p,std::size_t i)
  :document(std::move(d)),selected(p),ordinal(i){}
};

#include "notification_context_root_read_nodes_core.inc"

struct BnwWorldViewControlTree::ContextRootChildrenEvidence final {
 using State=detail::NotificationLayoutContextOrdinaryState;
 using Controls=detail::NotificationContextLuaControlsNamedReference;
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 const std::weak_ptr<Core> core;
 const std::weak_ptr<ContextRootReadFileFrame> frame;
 const std::shared_ptr<ContextRootXml84> document;
 const std::shared_ptr<const detail::NotificationContextRootDefaultFieldsReturn> defaults;
 const detail::NotificationContextRootDefaultFieldsReturn::Guard defaults_guard;
 const std::shared_ptr<const Controls> controls;
 const Controls::Guard controls_guard;
 const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> receiver;
 const std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn> base_constructor;
 const std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> native_constructor;
 const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::size_t child_count;
 std::size_t ordinal=0;
 std::shared_ptr<const ContextRootStyleCursorValue> styles;
 std::shared_ptr<const detail::NotificationLayoutContextRealmRef> realm;
 std::vector<std::shared_ptr<const ContextRootNodeReturn>> completed_nodes;
 std::shared_ptr<const ContextRootStyleCursorValue> current_style;
 std::shared_ptr<ContextRootInstanceMutation> current_instance;
 std::weak_ptr<const detail::NotificationContextRootReadCompletionSlot> pending_completion;
 std::shared_ptr<const detail::NotificationContextLuaControlsNamedRetirement> named_retirement;
 context_root_read_temporary_detail::XmlCursor cursor34;
 context_root_read_temporary_detail::XmlCursor cursor10;
 context_root_read_temporary_detail::CursorPhase cursor_phase=context_root_read_temporary_detail::CursorPhase::document;
 context_root_read_temporary_detail::NameString name_string18;
 std::array<ContextRootReadFileFrame::Cleanup,6> cleanup_effects;
 std::size_t cleanup_count=0;
 ContextRootChildrenEvidence(std::shared_ptr<Core> c,std::shared_ptr<ContextRootReadFileFrame> f,
  std::shared_ptr<ContextRootXml84> xml,std::shared_ptr<const detail::NotificationContextRootDefaultFieldsReturn> d,
  detail::NotificationContextRootDefaultFieldsReturn::Guard dg,std::shared_ptr<const Controls> refs,Controls::Guard cg,
  std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> r,
  std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn> bc,
  std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> nc,
  std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
  const runtime::BnwWorldViewSourceOperationLoan*op,std::size_t count)
  :core(c),frame(f),document(std::move(xml)),defaults(std::move(d)),defaults_guard(std::move(dg)),
   controls(std::move(refs)),controls_guard(std::move(cg)),receiver(std::move(r)),base_constructor(std::move(bc)),
   native_constructor(std::move(nc)),source(std::move(owner)),operation(op),child_count(count),
   cursor10(document&&document->storage?document->storage->document.get():nullptr){}
 void first()const {
  require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),"Root child cursor retired/failed/foreign FIRST");
  const auto read=frame.lock();require(read!=nullptr,"Root child cursor real lexical Read retired FIRST");
  read->first(); // canonicalSource pointer+storedscope FIRST, before old operand.
  const auto actual=core.lock();require(actual&&read->operation==operation&&
   State::Impl::same(actual->source,source)&&actual->context_root_children_evidence.lock().get()==this,
   "Root child cursor SAME entered physical Context/source/current frame changed");
  defaults_guard.require_current_without_callbacks(*operation,read->prepared);
  if(named_retirement)named_retirement->require_actual_nil_without_callbacks(*operation,*controls);
  else controls_guard.require_current_without_callbacks(*operation,read->prepared,defaults);
  require(document&&State::Impl::same(read->expected_xml84,document)&&document->storage&&document->storage->document&&
   document->parser_outcome==true&&document->stage==ContextRootXml84::Stage::defaults_complete&&
   document->storage->document->children.size()==child_count&&ordinal<=child_count&&
   State::Impl::same(document->defaults_return,defaults)&&State::Impl::same(read->receiver,receiver)&&
   State::Impl::same(actual->context_construction_return,base_constructor)&&
   State::Impl::same(read->constructor,native_constructor)&&State::Impl::same(actual->context_root_styles_value,styles),
   "Root child cursor actual new XML84/constructor/receiver/Styles cell changed");
  using Type=context_root_read_temporary_detail::XmlCursor::Type;
  using Phase=context_root_read_temporary_detail::CursorPhase;
  using Life=context_root_read_temporary_detail::NameString::Life;
  require(cleanup_count<=6&&read->cleanup_count==cleanup_count&&
   cursor34.type==(cleanup_count>=1?Type::retired_base_b76284:Type::cursor_835540)&&!cursor34.selected&&
   cursor10.type==(cleanup_count>=4?Type::retired_base_b76284:Type::cursor_835540)&&
   name_string18.life==(cleanup_count>=2?Life::retired_canonical_empty:Life::query_value)&&
   (cleanup_count<2||name_string18.value.empty())&&bool(named_retirement)==(cleanup_count>=3),
   "RootRead actual local cursor/String/LuaRef cleanup phase changed");
  static constexpr std::array cleanup_order{ContextRootReadFileFrame::Cleanup::cursor34_8353f0,
   ContextRootReadFileFrame::Cleanup::string18_retire,ContextRootReadFileFrame::Cleanup::lua2c_8f8820,
   ContextRootReadFileFrame::Cleanup::cursor10_8353f0,ContextRootReadFileFrame::Cleanup::wide280_686f10,
   ContextRootReadFileFrame::Cleanup::wide68_686f10};
  for(std::size_t i=0;i<cleanup_count;++i)require(cleanup_effects[i]==cleanup_order[i]&&read->cleanup_effects[i]==cleanup_order[i],
   "RootRead actual ordered local cleanup prefix changed");
  if(cursor_phase==Phase::document)require(ordinal==0&&cursor10.selected==document->storage->document.get(),
   "RootRead original10 document pre-query cursor changed");
  else if(cursor_phase==Phase::selected_child)require(ordinal<child_count&&
   cursor10.selected==&document->storage->document->children[ordinal],"RootRead actual current835640/8356A0 child changed");
  else require(ordinal==child_count&&cursor10.selected==(child_count?&document->storage->document->children.back():document->storage->document.get()),
   "RootRead actual835640/8356A0 FALSE incorrectly cleared/replaced selected4");
  require_context_root_node_returns_without_callbacks(*this);
  native_constructor->impl_->port->require_completed();
  if(styles){require(State::Impl::same(styles->document,document)&&styles->ordinal<child_count&&
   styles->selected==&document->storage->document->children[styles->ordinal]&&styles->selected->name=="Styles",
   "Root child cursor physical AC+4/B0 is not actual parsed Styles node");}
  if(realm){require(State::Impl::same(actual->context_realm_identity,realm),"Root child cursor actual parent THIS identity changed");
   realm->impl_->first(*operation);}
  read->first();
 }
 void full()const {first();const auto read=frame.lock();require(read!=nullptr,"Root cursor Read retired");
  read->full();first();
  if(named_retirement)named_retirement->require_actual_nil_owned(*operation,*controls);
  else controls_guard.require_current_owned(*operation,read->prepared,defaults);
  first();read->full();first();}
};

struct detail::NotificationContextRootReadChildrenLoan::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::shared_ptr<BnwWorldViewControlTree::ContextRootChildrenEvidence> evidence;
 explicit Impl(std::shared_ptr<BnwWorldViewControlTree::ContextRootChildrenEvidence> e):evidence(std::move(e)){}
 void first()const {require(creator==std::this_thread::get_id()&&alive.load()&&evidence,"Root child loan retired/foreign FIRST");evidence->first();}
};
detail::NotificationContextRootReadChildrenLoan::NotificationContextRootReadChildrenLoan(std::shared_ptr<Impl> value):impl_(std::move(value)){}
detail::NotificationContextRootReadChildrenLoan::~NotificationContextRootReadChildrenLoan(){impl_->alive.store(false);}
detail::NotificationContextRootReadChildrenLoan::Guard::Guard(std::shared_ptr<Impl> value):impl_(std::move(value)){}
detail::NotificationContextRootReadChildrenLoan::Guard detail::NotificationContextRootReadChildrenLoan::capture_guard_without_callbacks()const {
 const auto saved=impl_;saved->first();return Guard(saved);
}
void detail::NotificationContextRootReadChildrenLoan::Guard::require_current_without_callbacks()const {impl_->first();}
void detail::NotificationContextRootReadChildrenLoan::Guard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const {
 const auto saved=impl_;saved->first();require(saved->evidence->operation==&op,"Root cursor offered foreign Source operand");saved->evidence->full();saved->first();
}
std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>
detail::NotificationContextRootReadChildrenLoan::Guard::actual_root_receiver_without_callbacks()const {impl_->first();return impl_->evidence->receiver;}
std::shared_ptr<const detail::NotificationLayoutContextRealmRef>
detail::NotificationContextRootReadChildrenLoan::Guard::actual_context_realm_without_callbacks()const {
 impl_->first();require(impl_->evidence->realm!=nullptr,"Root cursor has not entered genuine7AA012 parent argument");return impl_->evidence->realm;
}
const content::XmlNode& detail::NotificationContextRootReadChildrenLoan::Guard::actual_declaration_without_callbacks()const {
 impl_->first();const auto saved=impl_->evidence;require(saved->ordinal<saved->child_count,"Root XML child cursor exhausted");
 return saved->document->storage->document->children[saved->ordinal];
}
// Source26 actual factory identity body follows the distinct child type.
#include "notification_context_root_read_pending_completion_core.inc"

std::shared_ptr<const detail::NotificationContextRootReadCompletionSlot>
detail::NotificationContextRootReadChildrenLoan::Guard::capture_completion_slot_without_callbacks()const {
 impl_->first();return BnwWorldViewControlTree::capture_pending_context_root_read_completion_owned(*this);
}

void BnwWorldViewControlTree::read_context_root_declarations_prefix_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,std::shared_ptr<ContextRootReadFileFrame> frame){
 using E=ContextRootChildrenEvidence;using State=E::State;using Loan=detail::NotificationContextRootReadChildrenLoan;
 require(frame!=nullptr,"Root declarations actual read frame absent");frame->full();
 const auto actual=core_;require(actual&&frame->core.lock()==actual&&frame->operation==&operation,
  "Root declarations substituted physical Core/operation");
 const auto document=frame->expected_xml84;require(document&&document->storage&&document->storage->document&&document->defaults_return,
  "Root declarations require actual new parsed XML84 and returned5D fields");
 const auto defaults=document->defaults_return;
 const auto dg=defaults->capture_guard_without_callbacks(operation,frame->prepared);dg.require_current_owned(operation,frame->prepared);frame->full();
 // Genuine completeSetup decomposes its actual prior receipts in Host only.
 // The existing18 coldName lookup cannot be used while RootRead is busy=true.
 const auto&host=operation.context_lua_host_owned();frame->full();
 const auto controls=host.acquire_context_controls_named_reference_owned(operation,frame->prepared,frame->actual_name,frame->actual_setup,defaults);
 frame->full();dg.require_current_owned(operation,frame->prepared);frame->full();
 require(controls!=nullptr,"Root declarations require actual8F85A0 SAME518 named Controls ref");
 const auto cg=controls->capture_guard_without_callbacks(operation,frame->prepared,defaults);
 cg.require_current_owned(operation,frame->prepared,defaults);frame->full();
 require(actual->context_root_children_evidence.expired()&&!actual->context_root_styles_value,
  "Root declarations cannot replay prior child cursor/Styles stores");
 const auto base_constructor=actual->context_construction_return;
 const auto native_constructor=frame->constructor;const auto receiver=frame->receiver;const auto source=actual->source;
 const auto count=document->storage->document->children.size();
 const auto context_state=actual->context_ordinary_state;
 require(context_state&&context_state->impl_&&context_state->impl_->native_constructor_prefix&&
  context_state->impl_->native_constructor_prefix->impl_->port&&
  context_state->impl_->native_constructor_prefix->impl_->port->xml_ac&&
  !context_state->impl_->native_constructor_prefix->impl_->port->xml_ac->selected_4,
  "Root declarations AC+4 NULL preimage must come from genuine835540 constructor effect");
 auto e=std::make_shared<E>(actual,frame,document,defaults,dg,controls,cg,receiver,base_constructor,native_constructor,source,&operation,count);
 frame->full();dg.require_current_owned(operation,frame->prepared);frame->full();cg.require_current_owned(operation,frame->prepared,defaults);frame->full();
 require(actual->context_root_children_evidence.expired()&&!actual->context_root_styles_value,
  "Root declaration entry changed during allocation");actual->context_root_children_evidence=e;
 struct Exit{std::shared_ptr<E> e;~Exit(){e->alive.store(false);}}exit{e};
 try{
  using Node=ContextRootNodeReturn;
  using Phase=context_root_read_temporary_detail::CursorPhase;
  e->full();e->completed_nodes.reserve(count);e->full();
  // The pending slot also exists for a genuine zero-child result. Its local
  // loan is actually destroyed here; retained slot DATA is not completion.
  {
   auto fields=std::make_shared<Loan::Impl>(e);e->full();
   auto local=std::shared_ptr<const Loan>(new Loan(fields));e->full();
   const auto guard=local->capture_guard_without_callbacks();guard.require_source_owned(operation);
   const auto pending=guard.capture_completion_slot_without_callbacks();e->full();
   require(pending==actual->context_root_read_pending_completion&&e->pending_completion.lock()==pending,
    "RootRead actual pending slot was not captured by SAME entered XML cursor");
  }
  e->full();
  //835640 changes selected4 only on success. Zero children retain the root
  // document cursor; later8356A0 false retains the last actual child pointer.
  if(count){e->cursor10.selected=&document->storage->document->children.front();e->cursor_phase=Phase::selected_child;}
  else e->cursor_phase=Phase::exhausted;
  e->full();
  while(e->ordinal<e->child_count){
   e->full();const auto ordinal=e->ordinal;
   const auto*child=&document->storage->document->children[ordinal];
   require(e->cursor10.selected==child&&e->cursor_phase==Phase::selected_child,
    "RootRead actual835640/8356A0 selected XML cursor differs from current ordinal");
   std::shared_ptr<const Node> returned;
   if(child->name=="Styles"){
    auto value=std::make_shared<const ContextRootStyleCursorValue>(document,child,ordinal);e->full();
    actual->context_root_styles_value=value;e->styles=value;e->current_style=value;e->full();
    returned=std::make_shared<const Node>(e,document,child,ordinal,Node::Kind::styles,value);e->full();
   }else if(child->name=="Instance"){
    returned=register_context_root_instance_a0_owned(e);e->full();
   }else{
    //7AA012 supplies THIS Context even if its physical Base3C is nativeNULL.
    if(!actual->context_realm_identity){
     auto fields=std::make_shared<detail::NotificationLayoutContextRealmRef::Impl>(actual,source,base_constructor);e->full();
     fields->first(operation);e->first();
     auto realm=std::shared_ptr<const detail::NotificationLayoutContextRealmRef>(new detail::NotificationLayoutContextRealmRef(fields));e->full();
     realm->impl_->first(operation);e->first();require(!actual->context_realm_identity,"Root ContextTHIS replaced during allocation");
     actual->context_realm_identity=realm;e->realm=realm;
    }else e->realm=actual->context_realm_identity;
    e->full();
    {
     auto fields=std::make_shared<Loan::Impl>(e);e->full();
     auto loan=std::shared_ptr<const Loan>(new Loan(fields));e->full();
     const auto guard=loan->capture_guard_without_callbacks();guard.require_source_owned(operation);
     require(guard.actual_context_realm_without_callbacks()&&&guard.actual_declaration_without_callbacks()==child,
      "Root529520 current XML cursor/actual parentTHIS was substituted");
     if(child->name=="LuaContext"){
      construct_root_context_child_prefix_owned(operation,loan);e->full();
      const auto at=std::find_if(actual->context_root_child_attempts.begin(),actual->context_root_child_attempts.end(),
       [&](const auto&a){return a&&a->document==document&&a->declaration==child;});
      require(at!=actual->context_root_child_attempts.end()&&(*at)->child_core&&(*at)->controls_v64_return&&
       (*at)->attach_progress&&(*at)->child_core->context_root_distinct_attach_return,
       "RootRead actual529520/v44/v64/673730 did not return SAME physical distinct child");
      const auto attached=(*at)->child_core->context_root_distinct_attach_return;
      require_context_distinct_attachment_physical_without_callbacks(attached->impl_);
      returned=std::make_shared<const Node>(e,document,child,ordinal,Node::Kind::distinct_context,
       std::shared_ptr<const ContextRootStyleCursorValue>{},std::nullopt,
       std::shared_ptr<const content::XmlNode>{},attached,(*at)->child_core);e->full();
     }else{
      // Real529520/v44 must return before its actual v64, then673730 attaches.
      // A cold locator, parsed serial, or subtype name cannot replace any call.
      const auto factory=construct_root_ordinary_child_owned(operation,loan);e->full();
      require(factory!=nullptr,"RootRead ordinary factory did not return a genuine selected receiver");
      const auto native=consume_root_ordinary_child_controls_owned(operation,loan,factory);e->full();
      require(native!=nullptr,"RootRead ordinary v64 did not return its genuine tagged child");
      const auto attached=attach_context_returned_child_owned(operation,*loan,*native);e->full();
      require(attached&&attached->impl_,"RootRead ordinary673730 returned no physical attachment DATA");
      require_context_root_ordinary_attachment_physical_without_callbacks(attached->impl_);
      const auto& actual_child=Core::node(actual->nodes,*attached->impl_->child_id);
      require(actual_child.root_context_attach_return==attached&&actual_child.root_ordinary_controls_return==native,
       "RootRead ordinary actual factory/v64/attachment was replaced before node return");
      returned=std::make_shared<const Node>(e,document,child,ordinal,Node::Kind::ordinary_control,
       std::shared_ptr<const ContextRootStyleCursorValue>{},std::nullopt,
       std::shared_ptr<const content::XmlNode>{},attached,std::weak_ptr<Core>{},native);e->full();
      // Pending DATA is checked directly; initializer completion is not issued.
      require_context_root_ordinary_attachment_physical_without_callbacks(attached->impl_);
     }
    } // original child loan is now retired; the immutable node keeps physical DATA only.
    e->full();
   }
   require(returned&&returned->actual_cursor.lock()==e&&returned->document==document&&
    returned->declaration==child&&returned->ordinal==ordinal&&e->ordinal==ordinal&&
    e->completed_nodes.size()==ordinal&&e->completed_nodes.capacity()>=e->child_count,
    "RootRead cannot advance8356A0 without SAME actually executed node return");
   // No allocation/callback between adoption and the actual cursor advance.
   e->completed_nodes.push_back(returned);++e->ordinal;e->current_style.reset();e->current_instance.reset();
   if(e->ordinal<e->child_count)e->cursor10.selected=&document->storage->document->children[e->ordinal];
   else e->cursor_phase=Phase::exhausted; // native false preserves last selected4
   e->full();
  }
  cleanup_and_stage_context_root_read_owned(e);e->full();
 }catch(...){e->failed.store(true);throw;}
}
