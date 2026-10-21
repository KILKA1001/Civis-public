// PRIVATE selected529520 LuaContext branch. Own x64 allocation retains the
// executed40F constructor; no Original x86 pool addresses/counter equivalence.
// Failed/partial children stay quarantined; no fake pool return or decrement.

void BnwWorldViewControlTree::construct_root_context_child_prefix_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::shared_ptr<const detail::NotificationContextRootReadChildrenLoan>& loan){
 using State=detail::NotificationLayoutContextOrdinaryState;
 require(loan!=nullptr,"529520 requires genuine lexical RootRead cursor");
 const auto guard=loan->capture_guard_without_callbacks();guard.require_source_owned(operation);
 const auto e=loan->impl_->evidence;const auto actual=e->core.lock();
 require(actual&&actual==core_,"529520 substituted real parent Core");
 const auto* declaration=&guard.actual_declaration_without_callbacks();
 const auto document=e->document;const auto realm=guard.actual_context_realm_without_callbacks();
 const auto source=e->source;const auto gui=actual->context_gui_port;
 const auto first=[&]{guard.require_current_without_callbacks();
  require(e->operation==&operation&&actual->context_root_children_evidence.lock()==e&&
   &guard.actual_declaration_without_callbacks()==declaration&&declaration->name=="LuaContext"&&
   State::Impl::same(guard.actual_context_realm_without_callbacks(),realm),
   "529520 actual cursor/parentTHIS/name changed");};
 const auto full=[&]{first();guard.require_source_owned(operation);first();};
 full();require(gui!=nullptr,"529520 actual selected GUI retired");
 //64A120/8353A0 tests length10 AND strncmp LuaContext: no tag-prefix alias.
 require(declaration->name.size()==10&&declaration->name=="LuaContext",
  "529520 concrete factory branch unsupported before allocation");
 const auto attr=[&](std::string_view name)->const std::string*{
  const auto at=std::find_if(declaration->attributes.begin(),declaration->attributes.end(),
   [name](const auto&a){return a.first==name;});return at==declaration->attributes.end()?nullptr:&at->second;};
 const auto* id=attr("ID");const auto* file=attr("FileName");
 require(id&&!id->empty()&&file&&!file->empty()&&declaration->children.empty(),
  "selected Context factory needs actual ID/FileName custody and empty ordinary declaration children");
 const std::string context=*id;full(); //immutable current XML84 stays strongly held across copying.
 const auto descriptor=source->prepared_source(context);full();
 require(descriptor&&descriptor->context()==context,"actual selected child descriptor absent");
 auto operand=operation.context_loader_resource_operand_owned(descriptor);full();
 const auto request=operand.request_;
 require(request&&request->origin_==runtime::detail::ContextLoaderRequestCustody::Origin::retained_lua_context_declaration&&
  request->declaration_&&request->declaration_->name==declaration->name&&
  request->declaration_->text==declaration->text&&request->declaration_->attributes==declaration->attributes&&
  request->declaration_->children.empty()&&request->context_==context&&request->original_utf8_==*file,
  "529520 new mutable cursor must equal SAME actual retained Source declaration, not basename/root metadata");
 const auto declared_parent=source->selected_context_declared_parent_owned(context);full();
 require(declared_parent==actual->context,"selected descriptor belongs to different real parent Context");
 //Actual owned attempts persist through failures, before child construction.
 //Own heap is a qualified portable675AB0 storage replacement with no reuse;
 //bad_alloc is an error, never represented as the Original nullable success.
 auto attempt=std::make_shared<ContextRootChildAttempt>(guard,document,declaration,realm,descriptor,std::move(operand),context);full();
 require(std::none_of(actual->context_root_child_attempts.begin(),actual->context_root_child_attempts.end(),
  [&](const auto&a){return a&&a->document==document&&a->declaration==declaration;}),
  "529520 actual cursor cannot replay its prior allocation attempt");
 actual->context_root_child_attempts.reserve(actual->context_root_child_attempts.size()+1);full();
 actual->context_root_child_attempts.push_back(attempt);full();
 auto child=capture_owned(source,context,operation,gui);full();
 require(child&&child->core_&&child->core_!=actual,"675AB0 own child allocation/40F constructor did not return distinct Context");
 attempt->child=child;attempt->child_core=child->core_;full();
 const auto physical=attempt->child_core->context_ordinary_state;
 require(physical&&physical->impl_&&physical->impl_->native_constructor_return,
  "child Context lacks genuine completed40F construction");
 attempt->constructor=physical->impl_->native_constructor_return;
 attempt->constructor->impl_->port->require_completed();full();
 attempt->receiver=attempt->child_core->context_layout_baseline->base_.receiver_owned;
 require(attempt->receiver&&attempt->receiver->impl_->context.lock()==physical,
  "child Context constructor ordinary receiver is not physical subtype owner");
 //52955F/561 actual A368D0+44=4FEE80, not direct46FF80/v10.
 child->parse_retained_context_xml_wrapper_prefix_owned(operation,loan.get());full();
 //Only after actual wrapper return: deferred/text/wide/cursor temporaries and
 //its lexical frame have undergone DTOR. Pending DATA was not return authority.
 const auto pending=attempt->pending_wrapper;
 require(pending&&!attempt->wrapper&&attempt->child_core->context_xml_wrapper_frame.expired()&&
  pending->state==physical&&pending->realm==realm&&pending->sequence==attempt->child_core->context_layout_baseline->sequence_&&
  pending->effects==attempt->child_core->effect_revision&&pending->b4_revision==physical->impl_->b4_revision&&
  physical->impl_->wrapper_context_realm_3c==realm&&
  pending->children==attempt->child_core->context_layout_baseline->base_.children_78,
  "actual wrapper common/3C/DTOR return physical observations changed");
 full();attempt->wrapper=pending;attempt->pending_wrapper.reset();full();
 const auto factory=publish_context_root_child_factory_return_owned(operation,attempt);
 guard.require_source_owned(operation);guard.require_child_factory_return_identity_without_callbacks(*factory);
 prepare_root_context_child_controls_export_prefix_owned(operation,attempt);
 require(attempt->controls_v64_return&&!attempt->pending_controls_v64_return,"actual child4FD4D2/v64 did not return after its lexical DTOR");
 attach_distinct_context_child_owned(operation,loan,attempt);
 require(attempt->attach_progress&&attempt->child_core->context_root_distinct_attach_return,
  "actual distinct Context673730 did not return physical attachment DATA");
 return; // actual factory/v64/parentv38 done; enclosing RootRead must advance/cleanup
}

void BnwWorldViewControlTree::complete_root_context_child_common_wrapper_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<ContextXmlWrapperFrame>& frame){
 using State=detail::NotificationLayoutContextOrdinaryState;
 require(frame&&frame->parent_guard,"438170 Context child requires genuine parentRootRead argument");
 const auto parent_guard=*frame->parent_guard;parent_guard.require_source_owned(operation);frame->full();
 const auto parent_data=parent_guard.impl_->evidence;const auto root=parent_data->core.lock();
 require(root!=nullptr,"wrapper parent Context retired");
 const auto actual=core_;const auto state=frame->state;auto&base=*actual->context_layout_baseline;
 const auto receiver=base.base_.receiver_owned;
 const auto realm=parent_guard.actual_context_realm_without_callbacks();
 const auto* current_declaration=&parent_guard.actual_declaration_without_callbacks();
 const auto it=std::find_if(root->context_root_child_attempts.begin(),root->context_root_child_attempts.end(),
  [&](const auto&a){return a&&a->child_core==actual&&a->declaration==current_declaration;});
 require(it!=root->context_root_child_attempts.end(),"wrapper actual child allocation not retained by SAME Root cursor");
 const auto attempt=*it; //own value before callbacks or vector growth.
 const auto current=[&]{frame->first();parent_guard.require_current_without_callbacks();
  require(attempt->child_core==actual&&State::Impl::same(attempt->realm,realm)&&attempt->receiver==receiver&&
   &parent_guard.actual_declaration_without_callbacks()==current_declaration&&
   current_declaration->attributes==frame->declaration->attributes&&!attempt->wrapper,
   "wrapper SAME selected child/parent Realm/current declaration changed");};
 const auto full=[&]{current();parent_guard.require_source_owned(operation);current();frame->full();current();};
 const auto record=[&](detail::NotificationLayoutBaseline::Mutation kind){
  require(base.sequence_<UINT64_MAX&&actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,
   "Context common actual journal exhausted");base.history_.reserve(base.history_.size()+1);full();
  base.record_owned(kind);++actual->effect_revision;++actual->layout_sequence;frame->accept_native_store_without_callbacks();};
 full();
 //A genuine narrow selected profile; all other attributes require their real
 //ordered lookup/helpers rather than an ignored success/default.
 for(const auto&[name,value]:frame->declaration->attributes)
  require(name=="FileName"||name=="DeferLoad"||name=="ID"||name=="Hidden",
   "required Context438170 attribute/Style producer outside selected empty-child profile");
 require(current_declaration->children.empty()&&!base.base_.children_78&&receiver&&
  receiver->impl_->core.lock()==actual&&receiver->impl_->context.lock()==state,
  "Context438170 physical receiver/NULL78 constructor preimage changed");
 //4381AC actual second argument store BEFORE all common attribute lookups.
 record(detail::NotificationLayoutBaseline::Mutation::context_realm_3c);
 base.base_.context_realm_3c=realm;base.base_.context_3c_observation=detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm;
 state->impl_->wrapper_context_realm_3c=realm;frame->accept_native_store_without_callbacks();full();
 //Offset/Size/Anchor/AnchorSide/Color queries MISS in the admitted declaration.
 if(const auto*id=frame->attribute_without_callbacks("ID")){
  require(id->size()<32&&std::all_of(id->begin(),id->end(),[](unsigned char c){return c>0&&c<128;})&&!base.base_.id_58,
   "564300 actual terminated ASCII ID shorter32/constructorNULL required");
  if(!id->empty()){
   std::array<char,32>bytes{};std::copy(id->begin(),id->end(),bytes.begin());const auto length=std::uint32_t(id->size());
   auto value=std::shared_ptr<const detail::NotificationLayoutControlID58>(new detail::NotificationLayoutControlID58(bytes,length));full();
   require(!base.base_.id_58,"Context564300 ID changed during allocation");record(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
   base.base_.id_58=value;frame->accept_native_store_without_callbacks();full();
  }
 }
 if(const auto*hidden=frame->attribute_without_callbacks("Hidden");hidden&&!hidden->empty()&&
  (hidden->front()=='t'||hidden->front()=='T'||hidden->front()=='1')){
  record(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);base.base_.flags_40|=2U;
  frame->accept_native_store_without_callbacks();full();
 }
 //All remaining common flag/color/tooltip branches absent. Both named wrapper
 //child queries MISS;835620/direct835640 returns0 on actual empty declaration.
 auto array_fields=std::make_shared<detail::NotificationLayoutOrdinaryChildren78::Impl>(receiver);full();
 auto array=std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78>(new detail::NotificationLayoutOrdinaryChildren78(array_fields));full();
 require(!base.base_.children_78&&array_fields->ordered.empty(),"Context495E10 empty array allocator POST changed");
 record(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);base.base_.children_78=array;
 frame->accept_native_store_without_callbacks();full();array_fields->ordered.reserve(0);full();
 record(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);base.base_.flags_40|=8U;
 frame->accept_native_store_without_callbacks();full();
 //4387C9 cursorDTOR and sharedempty FString are portable lexical owners. No
 //callbacks/VM effects exist in the absent/empty admitted cleanup branches.
 {std::shared_ptr<const content::XmlNode> cursor=frame->declaration;cursor.reset();}
 full();
 //4FEFA8 repeats actual secondarg3C AFTER common438170 return.
 record(detail::NotificationLayoutBaseline::Mutation::context_realm_3c);
 state->impl_->wrapper_context_realm_3c=realm;base.base_.context_realm_3c=realm;
 base.base_.context_3c_observation=detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm;
 frame->accept_native_store_without_callbacks();full();
 //Retained query strings are lexical copies; source declaration custody is
 //not destroyed with the wrapper's own temporary cursor/String handles.
 const auto sequence=base.sequence_,effects=actual->effect_revision,b4_revision=state->impl_->b4_revision;
 auto returned=std::shared_ptr<const ContextRootChildAttempt::WrapperReturn>(new ContextRootChildAttempt::WrapperReturn(state,realm,array,sequence,effects,b4_revision));full();
 require(returned->sequence==base.sequence_&&returned->effects==actual->effect_revision&&
  returned->b4_revision==state->impl_->b4_revision&&state->impl_->wrapper_context_realm_3c==realm,
  "Context wrapper return allocation exact physical journals/3C changed");
 attempt->pending_wrapper=returned; //DATA only; enclosing String/cursor DTOR still mandatory.
 frame->full();parent_guard.require_source_owned(operation);frame->full();
}
