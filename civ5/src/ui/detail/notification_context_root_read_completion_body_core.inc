// After actual E and Slot::Impl. Every completion branch below executes its
// real current XML effects. No completed payload exists before all six local
// cleanups and actual cursor/frame DTOR + LAST Source FULL/pure physical POST.
void BnwWorldViewControlTree::ContextRootInstanceMutation::first()const {
 const auto e=actual_cursor.lock();require(e&&e->current_instance.get()==this,"actual InstanceA0 mutation retired/canonical changed");
 const auto frame=e->frame.lock();require(frame!=nullptr,"actual InstanceA0 read frame retired");frame->first();
 const auto root=e->core.lock();const auto state=root?root->context_ordinary_state:nullptr;
 require(root&&state&&state->impl_&&constructor&&constructor->impl_&&constructor->impl_->alive.load()&&
  !constructor->impl_->failed.load()&&state->impl_->native_constructor_prefix==constructor&&constructor->impl_->port&&
  constructor->impl_->port->instance_a0&&&*constructor->impl_->port->instance_a0==map&&e->document==document&&
  e->ordinal==ordinal&&ordinal<e->child_count&&declaration==&document->storage->document->children[ordinal]&&
  declaration->name=="Instance"&&selected&&!declaration->children.empty()&&selected.get()==&declaration->children.front(),
  "actual InstanceA0 constructor/map/current XML declaration/first child changed");
 const auto name=std::find_if(declaration->attributes.begin(),declaration->attributes.end(),[](const auto&a){return a.first=="Name";});
 require(name!=declaration->attributes.end()&&e->name_string18.value==name->second&&
  key==content::saved_info_type_hash(name->second),"actual InstanceA0 queried String18/Name CRC changed");
 constructor->impl_->port->require_reached(); // constructor history, not live empty A0 assertion
 const auto at=map->cells.find(key);const bool inserted=key_absent&&stage!=Stage::before_lookup;
 const bool stored=stage==Stage::cursor_stored;
 require(map->cells.size()==before_count+(inserted?1u:0u)&&map->count==map->cells.size()&&
  map->revision==before_revision+(inserted?1u:0u)+(stored?1u:0u),
  "actual InstanceA0 numeric lookup/store journal changed");
 if(key_absent&&stage==Stage::before_lookup)require(at==map->cells.end(),"actual InstanceA0 pre-lookup key appeared");
 else require(at!=map->cells.end()&&at->second.selected==(stored?selected:prior),"actual InstanceA0 cell current stage changed");
 frame->first();
}

void BnwWorldViewControlTree::require_context_root_node_returns_without_callbacks(const ContextRootChildrenEvidence& e){
 require(e.completed_nodes.size()==e.ordinal&&e.document&&e.document->storage&&e.document->storage->document,
  "RootRead8356A0 has no actual return for every consumed node");
 const auto root=e.core.lock();require(root&&root->context_ordinary_state&&root->context_ordinary_state->impl_,
  "RootRead actual node-return owner retired");
 const auto pending=e.pending_completion.lock();
 const auto prefix=root->context_ordinary_state->impl_->native_constructor_prefix;
 require(prefix&&prefix->impl_&&prefix->impl_->port&&prefix->impl_->port->instance_a0,
  "RootRead actual constructor-owned InstanceA0 missing");
 auto& map=*prefix->impl_->port->instance_a0;
 std::shared_ptr<const ContextRootStyleCursorValue> latest_style;
 for(std::size_t i=0;i<e.completed_nodes.size();++i){
  const auto& n=e.completed_nodes[i];require(n&&n->actual_cursor.lock().get()==&e&&n->document==e.document&&n->ordinal==i&&
   n->declaration==&e.document->storage->document->children[i],"RootRead completed node receipt substituted actual cursor/XML/ordinal");
  if(n->kind==ContextRootNodeReturn::Kind::styles){
   require(n->declaration->name=="Styles"&&n->style&&n->style->document==e.document&&n->style->ordinal==i&&
    n->style->selected==n->declaration&&!n->instance_key&&!n->instance_selected&&n->attached.expired()&&n->child.expired()&&n->ordinary_controls.expired(),
    "RootRead actual Styles B0 store receipt changed");latest_style=n->style;
  }else if(n->kind==ContextRootNodeReturn::Kind::instance){
   require(n->declaration->name=="Instance"&&!n->style&&n->attached.expired()&&n->child.expired()&&n->ordinary_controls.expired(),"RootRead Instance produced live control");
   if(n->instance_key){
    require(n->instance_selected&&!n->declaration->children.empty()&&n->instance_selected.get()==&n->declaration->children.front(),
     "RootRead actual Instance first-child cursor changed");
    const auto attribute=std::find_if(n->declaration->attributes.begin(),n->declaration->attributes.end(),
     [](const auto&a){return a.first=="Name";});
    require(attribute!=n->declaration->attributes.end()&&*n->instance_key==content::saved_info_type_hash(attribute->second),
     "RootRead actual Instance Name/raw numeric CRC changed");
    // Native7A9FF5 replaces existing duplicate keys. Only the last current
    // earned writer can describe that live cell; earlier receipts are history.
    bool latest=true;for(std::size_t k=i+1;k<e.completed_nodes.size();++k)
     if(e.completed_nodes[k]->kind==ContextRootNodeReturn::Kind::instance&&e.completed_nodes[k]->instance_key==n->instance_key)latest=false;
    if(e.current_instance&&e.current_instance->key==*n->instance_key)latest=false;
    if(latest){const auto at=map.cells.find(*n->instance_key);require(at!=map.cells.end()&&at->second.selected==n->instance_selected,
      "RootRead current actual InstanceA0 final writer changed");}
   }else require(!n->instance_selected&&n->declaration->children.empty(),"RootRead absent first-child branch not actual empty Instance");
  }else if(n->kind==ContextRootNodeReturn::Kind::ordinary_control){
   const auto attached=n->attached.lock();const auto native=n->ordinary_controls.lock();
   require(attached&&native&&attached->impl_&&pending&&n->child.expired()&&!n->style&&!n->instance_key&&!n->instance_selected&&
    n->declaration->name!="LuaContext"&&n->declaration->name!="Styles"&&n->declaration->name!="Instance"&&
    attached->impl_->core.lock()==root&&attached->impl_->completed.impl_==pending->impl_,
    "RootRead ordinary node physical DATA/pending slot/tag changed");
   // This is a historical physical check, not a renewed expired v64/Read Guard.
   require_context_root_ordinary_attachment_physical_without_callbacks(attached->impl_);
   const auto& child=Core::node(root->nodes,*attached->impl_->child_id);
   require(child.root_context_attach_return==attached&&child.root_ordinary_controls_return==native&&
    child.full_xml_occurrence&&child.full_xml_occurrence->ordinal==i&&
    child.full_xml_occurrence->parent.lock()==root->context_root_parsed_xml_occurrence&&
    Core::same_layout_xml(child.full_xml_occurrence->value,*n->declaration),
    "RootRead ordinary node SAME returned factory/v64/parsed XML occurrence changed");
  }else{
   const auto attached=n->attached.lock();const auto child=n->child.lock();
   require(n->kind==ContextRootNodeReturn::Kind::distinct_context&&n->declaration->name=="LuaContext"&&attached&&child&&
    n->ordinary_controls.expired()&&
    child!=root&&pending&&attached->impl_&&attached->impl_->core.lock()==root&&
    attached->impl_->completed.impl_==pending->impl_&&child->context_root_distinct_attach_return==attached,
    "RootRead actual distinct Context673730 node return/pending slot changed");
   require_context_distinct_attachment_physical_without_callbacks(attached->impl_);
   const auto attempt=std::find_if(root->context_root_child_attempts.begin(),root->context_root_child_attempts.end(),
    [&](const auto&a){return a&&a->document==e.document&&a->declaration==n->declaration&&a->child_core==child;});
   require(attempt!=root->context_root_child_attempts.end()&&(*attempt)->controls_v64_return&&(*attempt)->attach_progress,
    "RootRead actual distinct factory/v64/attach retained provenance missing");
  }
 }
 if(e.current_style){
  require(e.ordinal<e.child_count&&e.current_style->document==e.document&&e.current_style->ordinal==e.ordinal&&
   e.current_style->selected==&e.document->storage->document->children[e.ordinal]&&e.current_style->selected->name=="Styles",
   "RootRead current actual Styles writer is not this lexical XML ordinal");latest_style=e.current_style;
 }
 require(e.styles==latest_style&&root->context_root_styles_value==latest_style,"RootRead actual last Styles B0 differs from earned history");
 require(map.count==map.cells.size(),"RootRead current actual InstanceA0 count differs from live cells");
 if(e.current_instance)e.current_instance->first();
}

void BnwWorldViewControlTree::require_context_root_read_staged_before_frame_exit_owned(
 const std::shared_ptr<ContextRootReadFileFrame>& frame){
 require(frame!=nullptr,"RootRead staged result actual frame absent");frame->full();
 const auto root=frame->core.lock();const auto pending=root?root->context_root_read_pending_completion:nullptr;
 require(pending&&pending->impl_&&pending->impl_->staged_return&&!pending->impl_->completed_return,
  "RootRead actual all-node/six-cleanup staged body absent before frame DTOR");
 const auto data=pending->impl_->staged_return;const auto e=data->returned_cursor;
 require(data->returned_frame==frame&&data->actual_pending.lock()==pending&&e&&e==pending->impl_->actual_cursor.lock()&&
  !e->alive.load()&&!e->failed.load()&&e->frame.lock()==frame&&e->core.lock()==root&&e->ordinal==e->child_count&&
  e->cleanup_count==6&&frame->cleanup_count==6&&e->cleanup_effects==data->cleanup_effects&&e->cleanup_effects==frame->cleanup_effects&&
  e->named_retirement&&root->context_layout_children,
  "RootRead real E DTOR/all-node cleanup staging changed before actual frame DTOR");
 e->named_retirement->require_actual_nil_without_callbacks(*frame->operation,*e->controls);
 require_context_root_node_returns_without_callbacks(*e);frame->full();
}

std::shared_ptr<const BnwWorldViewControlTree::ContextRootNodeReturn>
BnwWorldViewControlTree::register_context_root_instance_a0_owned(const std::shared_ptr<ContextRootChildrenEvidence>& e){
 using Node=ContextRootNodeReturn;using Mutation=ContextRootInstanceMutation;
 e->full();const auto root=e->core.lock();const auto frame=e->frame.lock();const auto document=e->document;
 require(root&&frame&&e->ordinal<e->child_count&&!e->current_instance,"Instance requires genuine current lexical Root cursor");
 const auto ordinal=e->ordinal;const auto*declaration=&document->storage->document->children[ordinal];
 require(declaration->name=="Instance","Instance branch substituted XML element");
 const auto attribute=std::find_if(declaration->attributes.begin(),declaration->attributes.end(),[](const auto&a){return a.first=="Name";});
 require(attribute!=declaration->attributes.end(),"required actual Instance Name MISS fallback through real529520 ordinary dispatcher");
 //8355C0/8352A0 query and copy returns TRUE for an empty existing Name too.
 context_root_read_temporary_detail::XmlCursor attribute_cursor;
 auto queried=attribute->second;e->full();e->name_string18.value.swap(queried);e->full();
 attribute_cursor.retire_8353f0_without_callbacks();e->full(); //7A9FA8 before first-child query
 context_root_read_temporary_detail::XmlCursor first_child;
 std::shared_ptr<const content::XmlNode> selected;
 std::optional<std::uint32_t> hash;
 if(!declaration->children.empty()){
  first_child.selected=&declaration->children.front();
  selected=std::shared_ptr<const content::XmlNode>(document,first_child.selected); // real arena alias, no copy/control
  hash=content::saved_info_type_hash(e->name_string18.value);
  const auto constructor=root->context_ordinary_state->impl_->native_constructor_prefix;
  require(constructor&&constructor->impl_&&constructor->impl_->port&&constructor->impl_->port->instance_a0,"Instance actual A0 owner missing");
  auto*const map=&*constructor->impl_->port->instance_a0;constructor->impl_->port->require_reached();
  const auto old=map->cells.find(*hash);const bool absent=old==map->cells.end();
  const auto prior=absent?std::shared_ptr<const content::XmlNode>{}:old->second.selected;
  const auto count=map->cells.size();const auto revision=map->revision;
  require(revision<=UINT64_MAX-(absent?2u:1u)&&(!absent||count<UINT32_MAX),"Instance actual A0 count/mutation sequence exhausted");
  auto progress=std::make_shared<Mutation>(e,constructor,map,document,declaration,ordinal,count,revision,*hash,prior,selected,absent);e->full();
  require(!e->current_instance&&map->cells.size()==count&&map->revision==revision,"Instance actual A0 preimage changed during allocation");
  e->current_instance=progress;e->full();
  if(absent){
   context_constructor_prefix_detail::EmptyInstanceMapA0::Cells detached;
   detached.emplace(*hash,context_constructor_prefix_detail::EmptyInstanceMapA0::Cursor{});e->full();
   auto node=detached.extract(*hash);e->full();const auto inserted=map->cells.insert(std::move(node));
   require(inserted.inserted&&inserted.node.empty(),"Instance actual75AA30 NULL cursor insertion failed");
   ++map->count;++map->revision;progress->stage=Mutation::Stage::null_inserted;e->full();
  }
  //7A9FF5 overwrites the selected4 XML cursor even for an existing CRC key.
  const auto at=map->cells.find(*hash);require(at!=map->cells.end(),"Instance actual75AA30 cell absent after lookup");
  at->second.selected=selected;++map->revision;progress->stage=Mutation::Stage::cursor_stored;e->full();
 }
 first_child.retire_8353f0_without_callbacks();e->full(); //7A9FFC actual local first-child cursor DTOR
 auto result=std::make_shared<const Node>(e,document,declaration,ordinal,Node::Kind::instance,
  std::shared_ptr<const ContextRootStyleCursorValue>{},hash,selected);e->full();
 return result; // enclosing actual8356A0 adopts receipt, then retires mutation authority
}

void BnwWorldViewControlTree::cleanup_and_stage_context_root_read_owned(const std::shared_ptr<ContextRootChildrenEvidence>& e){
 using Slot=detail::NotificationContextRootReadCompletionSlot;
 using Cleanup=Slot::Impl::CompletedReturn::Cleanup;
 using FrameCleanup=ContextRootReadFileFrame::Cleanup;
 e->full();const auto frame=e->frame.lock();const auto root=e->core.lock();const auto pending=e->pending_completion.lock();
 require(frame&&root&&pending&&e->ordinal==e->child_count&&!e->current_instance&&!e->current_style&&e->cleanup_count==0&&
  pending==root->context_root_read_pending_completion&&!pending->impl_->completed_return&&!pending->impl_->staged_return&&
  frame->cleanup_count==0&&frame->temporary68&&frame->temporary280,"RootRead cleanup requires every real node/path temporary exactly once");
 std::size_t members=0;
 for(const auto& node:e->completed_nodes)if(node->kind==ContextRootNodeReturn::Kind::distinct_context||
  node->kind==ContextRootNodeReturn::Kind::ordinary_control){
  const auto member=node->attached.lock();require(member&&member->impl_&&member->impl_->append_index==members,
   "RootRead actual parent78 append order is not every completed live child");++members;
 }
 const auto ordered=root->context_layout_baseline->base_.children_78;
 require(ordered?(ordered->impl_&&ordered->impl_->parent.lock()==e->receiver&&ordered->impl_->ordered.size()==members):members==0,
  "RootRead parent78 contains unearned/missing children before cleanup");
 const auto effect=[&](Cleanup c,FrameCleanup f){
  require(e->cleanup_count<e->cleanup_effects.size()&&frame->cleanup_count==e->cleanup_count,"RootRead cleanup journals differ");
  e->cleanup_effects[e->cleanup_count++]=c;frame->cleanup_effects[frame->cleanup_count++]=f;
 };
 e->cursor34.retire_8353f0_without_callbacks();effect(Cleanup::cursor34_8353f0,FrameCleanup::cursor34_8353f0);e->full();
 e->name_string18.retire_to_empty_owned();effect(Cleanup::string18_retire,FrameCleanup::string18_retire);e->full();
 const auto&host=e->operation->context_lua_host_owned();e->full();
 const auto retired=host.retire_context_controls_named_reference_owned(*e->operation,frame->prepared,e->defaults,e->controls);
 require(retired!=nullptr,"RootRead actual local2C LuaRef8F8820 returned no nil receipt");
 e->named_retirement=retired;effect(Cleanup::lua2c_8f8820,FrameCleanup::lua2c_8f8820);e->full();
 e->cursor10.retire_8353f0_without_callbacks();effect(Cleanup::cursor10_8353f0,FrameCleanup::cursor10_8353f0);e->full();
 frame->temporary280->retire_686f10_without_callbacks();effect(Cleanup::wide280_686f10,FrameCleanup::wide280_686f10);e->full();
 frame->temporary68->retire_686f10_without_callbacks();effect(Cleanup::wide68_686f10,FrameCleanup::wide68_686f10);e->full();
 // Complete the actual Context child graph DATA from all earned root78
 // members. Empty is only earned after real XML traversal/zero child outcomes.
 require(!root->context_layout_children,"RootRead Context graph history already captured/replayed");
 std::vector<Core::ContextLayoutChild> graph;graph.reserve(e->completed_nodes.size());e->full();
 for(const auto& n:e->completed_nodes)if(n->kind==ContextRootNodeReturn::Kind::distinct_context){
  const auto child=n->child.lock();const auto attached=n->attached.lock();require(child&&attached,"RootRead completed child graph actual member retired");
  graph.push_back(Core::ContextLayoutChild{child,0,attached->impl_->append_index+1});
 }
 root->context_layout_children.emplace(std::move(graph));e->full();
 auto data=std::make_shared<const Slot::Impl::CompletedReturn>(frame,e,pending,e->cleanup_effects);e->full();
 require(!pending->impl_->staged_return&&!pending->impl_->completed_return,"RootRead staged return replayed");
 pending->impl_->staged_return=data;e->full(); // DATA only; E/frame lexical DTOR and LAST FULL still required
}

void BnwWorldViewControlTree::require_context_root_read_terminal_without_callbacks(const detail::NotificationContextRootReadCompletionSlot::Impl& r){
 using Type=context_root_read_temporary_detail::XmlCursor::Type;
 using Life=context_root_read_temporary_detail::NameString::Life;
 const auto root=r.core.lock();const auto current=r.canonical.lock();
 require(root&&current&&r.creator==std::this_thread::get_id()&&r.alive.load()&&r.operation&&
  root->creator==r.creator&&root->facade_alive.load()&&!root->failed.load()&&root->source==r.source&&
  current->impl_.get()==&r&&root->context_root_read_pending_completion==current,
  "wholeRootRead return canonical actual Root/Source retired FIRST");
 r.source->require_operation_origin_without_callbacks(*r.operation,r.scope);
 const auto data=r.completed_return?r.completed_return:r.staged_return;
 require(data!=nullptr,"wholeRootRead has no actual all-node/six-cleanup staged result");
 const auto e=data->returned_cursor;const auto frame=data->returned_frame;
 require(e&&frame&&!e->alive.load()&&!e->failed.load()&&!frame->alive.load()&&!frame->failed.load()&&
  e==r.actual_cursor.lock()&&frame==r.actual_frame.lock()&&e->frame.lock()==frame&&e->core.lock()==root&&frame->core.lock()==root&&
  data->actual_pending.lock()==current&&e->pending_completion.lock()==current&&e->ordinal==e->child_count&&
  e->cleanup_count==6&&frame->cleanup_count==6&&e->cleanup_effects==data->cleanup_effects&&e->cleanup_effects==frame->cleanup_effects&&
  e->cursor34.type==Type::retired_base_b76284&&!e->cursor34.selected&&e->cursor10.type==Type::retired_base_b76284&&
  e->name_string18.life==Life::retired_canonical_empty&&e->name_string18.value.empty()&&e->named_retirement&&
  frame->temporary280&&frame->temporary68&&root->context_layout_children&&
  root->context_root_read_file_frame.expired()&&!root->busy&&r.operation->operation_serial()==r.serial,
  "wholeRootRead actual cursor/frame DTOR/all nodes/local temporaries/Source serial changed");
 static constexpr std::array expected{ContextRootReadFileFrame::Cleanup::cursor34_8353f0,
  ContextRootReadFileFrame::Cleanup::string18_retire,ContextRootReadFileFrame::Cleanup::lua2c_8f8820,
  ContextRootReadFileFrame::Cleanup::cursor10_8353f0,ContextRootReadFileFrame::Cleanup::wide280_686f10,
  ContextRootReadFileFrame::Cleanup::wide68_686f10};
 require(frame->cleanup_effects==expected,"wholeRootRead original six cleanup order changed");
 frame->temporary280->require_retired_without_callbacks();frame->temporary68->require_retired_without_callbacks();
 require(e->cursor_phase==context_root_read_temporary_detail::CursorPhase::exhausted&&
  e->cursor10.selected==(e->child_count?&e->document->storage->document->children.back():e->document->storage->document.get())&&
  frame->temporary68->origin==ContextRootReadFileFrame::PathTemporary::Origin::selected_loose7493b0&&
  frame->temporary280->origin==ContextRootReadFileFrame::PathTemporary::Origin::watch_path7a0030&&
  frame->temporary68->value==frame->expected_510&&frame->temporary280->value==frame->expected_510&&
  frame->temporary68->physical==e->document->physical&&frame->temporary280->physical==e->document->physical,
  "wholeRootRead actual exhausted8356A0 selected4/path68/path280 provenance changed");
 e->named_retirement->require_actual_nil_without_callbacks(*r.operation,*e->controls);
 //The former Root5D live Guard correctly stops after frame DTOR. Validate its
 //earned physical fields here without renewing that expired lexical Guard.
 const auto defaults=e->defaults?e->defaults->impl_:nullptr;
 const auto evidence=defaults?defaults->evidence:nullptr;
 require(defaults&&defaults->creator==r.creator&&defaults->alive.load()&&evidence&&
  evidence->creator==r.creator&&evidence->core.lock()==root&&evidence->frame.lock()==frame&&
  evidence->document.lock()==e->document&&evidence->prepared==frame->prepared&&evidence->occurrence==frame->occurrence&&
  e->document->defaults_return==e->defaults&&e->document->stage==ContextRootXml84::Stage::defaults_complete,
  "wholeRootRead actual returned5D fields provenance was substituted");
 const auto&fields=root->context_default_font_style;
 require(fields.creator==r.creator&&fields.allocation==root.get()&&!fields.reading&&!fields.read_source_serial&&
  fields.read_occurrence.expired()&&fields.font_538.get()==evidence->font&&fields.value==evidence->style&&
  fields.colors_53c==evidence->colors&&fields.effect_count==evidence->effects&&fields.effect_count&&
  fields.effect_count<=fields.effects.size()&&fields.effects[fields.effect_count-1]==Core::ContextDefaultFontStyle::Effect::read_return&&
  root->context_gui_port==evidence->gui,"wholeRootRead actual5D font/style/colors/GUI fields changed after LAST callback");
 if(evidence->font)evidence->font->require_current_without_callbacks();
 if(evidence->gui)evidence->gui->impl_->first(*r.operation);
 require_context_root_node_returns_without_callbacks(*e);
}
void BnwWorldViewControlTree::publish_context_root_read_after_scope_owned(const runtime::BnwWorldViewSourceOperationLoan& op){
 const auto actual=core_;require(actual&&actual->context_root_read_pending_completion,"wholeRootRead actual pending slot absent");
 const auto returned=actual->context_root_read_pending_completion;const auto r=returned->impl_;
 require(r&&r->staged_return&&!r->completed_return,"wholeRootRead requires genuine unissued staged body");
 require_context_root_read_terminal_without_callbacks(*r);require(r->operation==&op,"wholeRootRead different actual Source operand");
 op.require_current();require_context_root_read_terminal_without_callbacks(*r); // LAST FULL after real lexical DTOR
 r->completed_return=r->staged_return;r->staged_return.reset();
 require_context_root_read_completed_slot_without_callbacks(*r);
 detail::NotificationContextRootReadCompletionSlot::Guard guard(r);
 guard.require_source_owned(op);guard.require_actual_root_receiver_without_callbacks(*r->receiver);
}
