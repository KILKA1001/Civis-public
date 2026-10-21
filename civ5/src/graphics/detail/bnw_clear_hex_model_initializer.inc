// Included inside the existing civ5::ui namespace in the actual renderer TU.
void UiScreenGlRenderer::initialize_clear_hex_highlight_model_owned(
 const runtime::BnwWorldViewSourceOperationLoan& op,
 const std::shared_ptr<graphics::BnwClearHexHighlightOwner>& actual,std::size_t index,
 const assets::CivisLoadedResource& model_source)const {
 using Backend=graphics::detail::BnwClearHexModelBackend;
 using Pin=graphics::detail::BnwRendererContextPin;
 const auto life=worldview_lifetime_;const auto*const renderer=this;
 const auto preparation=clear_highlight_preparation_;
 const auto*const receiver=actual.get();
 // Creator denial precedes State pins, parsing, IO and non-atomic effects.
 if(!life||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
    !life->active||life->active_scope||life->renderer!=renderer||!actual||!preparation||preparation->load()!=1)
  throw std::runtime_error("HexModel initializer lost actual pre-Context renderer");
 actual->first_owned();op.require_origin_without_callbacks();
 const auto s=actual->state_;const auto owner_control=s->self;const auto serial=op.operation_serial();
 op.require_grid_scene_origin_without_callbacks(s->scene);s->first();
 const auto initial_source=op.clear_hex_highlight_source_origin_without_callbacks_owned();s->first();
 if(initial_source.owner_before(s->source)||s->source.owner_before(initial_source)||initial_source.expired())
  throw std::runtime_error("HexModel actual constructor uses unrelated Source control");
 if(index>=s->models.size())throw std::runtime_error("HexModel actual model style index missing");
 auto*const style_receiver=&s->models[index];
 if(style_receiver->constructor_entered||style_receiver->active_constructor){s->failed.store(true);throw std::runtime_error("HexModel actual style constructor reentry quarantined");}
 if(style_receiver->constructor_generation==UINT64_MAX)throw std::runtime_error("HexModel constructor generation exhausted");
 const auto next_generation=style_receiver->constructor_generation+1;
 const auto gpu=s->gpu;
 const graphics::detail::BnwClearHexModelConstructionScope* invocation=nullptr;
 std::uint64_t expected_generation=style_receiver->constructor_generation;
 std::weak_ptr<Backend> produced_backend;
 const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
 const auto first=[&]{
  op.require_origin_without_callbacks();s->first();
  if(!actual||actual.get()!=receiver||!same(actual,owner_control)||owner_control.expired()||
     actual->state_.get()!=s.get()||life.get()!=worldview_lifetime_.get()||!life->alive||life->failed||
     life->creator!=std::this_thread::get_id()||!life->active||life->active_scope||life->renderer!=renderer||
     clear_highlight_preparation_.get()!=preparation.get()||preparation->load()!=1)
   throw std::runtime_error("HexModel constructor SAME receiver/renderer retired or rebound");
  actual->first_owned();op.require_grid_scene_origin_without_callbacks(s->scene);s->first();
  if(!same(op.clear_hex_highlight_source_origin_without_callbacks_owned(),s->source)||
     index>=s->models.size()||&s->models[index]!=style_receiver||
     !style_receiver->constructor_entered||
     style_receiver->active_constructor!=invocation||style_receiver->constructor_generation!=expected_generation||
     (s->models[index].backend&&!same(s->models[index].backend,produced_backend))||
     (!s->models[index].backend&&!produced_backend.expired())||!s->models[index].declaration.model||
     *s->models[index].declaration.model!="SelectedAniOneTurn.fxsxml"||!s->gpu||!s->gpu->pin||s->gpu.get()!=gpu.get()||!same(s->gpu,gpu))
   throw std::runtime_error("HexModel actual style/input/context custody absent or construction replayed");
  if(invocation&&(invocation->creator_!=std::this_thread::get_id()||invocation->operation_!=&op||
     invocation->receiver_!=receiver||invocation->index_!=index||invocation->serial_!=serial||
     invocation->generation_!=expected_generation))throw std::runtime_error("HexModel lexical constructor occurrence changed");
  // Actual canonical Scene publication is still absent in this constructor.
  {const auto scene=s->scene.lock();s->first();if(!scene||scene->clear_hex_highlights_)throw std::runtime_error("HexModel prepublication Scene slot replaced/reentered");scene->verify_owned();s->first();}
  op.require_origin_without_callbacks();s->first();
  if(!Pin::NormalAccess::api_current_without_switch_owned(s->gpu->pin))throw std::runtime_error("HexModel genuine Window-issued context pin retired");
 };
 const auto full=[&]{first();op.require_current();first();op.require_operation(serial);first();
  {const auto origin=op.input_policy_window_origin_owned();first();
   if(origin.hwnd!=s->window.hwnd||origin.occurrence!=s->window.occurrence||origin.process!=s->window.process||origin.ui_thread!=s->window.ui_thread||!same(origin.window,s->window.window))throw std::runtime_error("HexModel actual Window occurrence changed");
   op.require_input_policy_window_owned(origin);first();}
  first();};
 //Independent creator-only refusal fence starts before the FIRST FULL and
 //survives lexical Scope retirement and the final callback-capable POST.
 //It grants nothing: the real Scope/Source/control/output checks stay separate.
 style_receiver->constructor_entered=true;
 std::shared_ptr<Backend::State> state;
 try{
  full();
  {
  graphics::detail::BnwClearHexModelConstructionScope scope(op,*receiver,index,serial,next_generation);
  first();style_receiver->active_constructor=&scope;style_receiver->constructor_generation=next_generation;
  invocation=&scope;expected_generation=next_generation;first();
  const auto& pins=Backend::root_pins_owned();first();
  if(!model_source.xml||!model_source.bytes||model_source.xml->name!="Asset"||
     model_source.candidate.normalized_name!=assets::AssetKey("SelectedAniOneTurn.fxsxml").normalized_name()||
     model_source.payload_identity.bytes!=pins.fxsxml_.bytes||model_source.payload_identity.sha256!=pins.fxsxml_.sha256||
     runtime::identify_bytes(*model_source.bytes).sha256!=pins.fxsxml_.sha256)
   throw std::runtime_error("HexModel actual Bank FXSXML differs from exported selected source");
  full();
  { // Bank/recipe/SDK parsing targets retire before final construction POST.
   const auto bank=op.selected_provider_startup_bank_owned();full();
   if(!bank||!same(bank,s->bank))throw std::runtime_error("HexModel actual selected Bank control changed");
   bank->require_boundary();full();
   {const auto capability=bank->capability();full();
    const auto winner=capability.loaded(assets::AssetKey("SelectedAniOneTurn.fxsxml"));full();
    const auto& a=model_source.candidate;const auto& b=winner.candidate;
    if(winner.bytes.get()!=model_source.bytes.get()||!same(winner.bytes,model_source.bytes)||
       winner.xml.get()!=model_source.xml.get()||!same(winner.xml,model_source.xml)||
       winner.payload_identity.bytes!=model_source.payload_identity.bytes||winner.payload_identity.sha256!=model_source.payload_identity.sha256||
       a.normalized_name!=b.normalized_name||a.payload_size!=b.payload_size||
       a.provenance.kind!=b.provenance.kind||a.provenance.source_id!=b.provenance.source_id||
       a.provenance.source_relative_path!=b.provenance.source_relative_path||a.provenance.archive_relative_path!=b.provenance.archive_relative_path)
     throw std::runtime_error("HexModel passed FXSXML is not SAME actual Bank winner/provenance/physical source");
   }
   full(); //actual capability/winner target retirement POST, before source use
   state=std::make_shared<Backend::State>(owner_control,bank,index);full();
   state->recipe=bank->source_recipe();full();
   if(state->recipe.recipe_identity.empty()||!state->recipe.profile.enabled_mods.empty())throw std::runtime_error("HexModel selected source profile/recipe unavailable or unsupported");
   state->inputs.push_back(model_source);full();
   std::size_t mesh_count=0,animation_count=0;std::set<std::string> texture_names;
   for(const auto& node:model_source.xml->children){
    if(node.name=="Mesh"){
     ++mesh_count;if(node.attribute("file")!="SelectedAniOneTurn.gr2")throw std::runtime_error("HexModel authored Mesh role changed");
    }else if(node.name=="Animation"){
     ++animation_count;if(node.attribute("file")!="SelectedAniOneTurn_SelectedAniOneTurn.gr2"||node.attribute("name")!="SelectedAniOneTurn"||node.attribute("ec")!="1000")throw std::runtime_error("HexModel authored Animation role/event changed");
     state->authored_animation_alias=node.attribute("name");
     const auto ec=node.attribute("ec");
     const auto parsed=std::from_chars(ec.data(),ec.data()+ec.size(),state->authored_animation_event_code);
     if(parsed.ec!=std::errc{}||parsed.ptr!=ec.data()+ec.size()||state->authored_animation_event_code!=1000)throw std::runtime_error("HexModel actual authored Animation event code not represented");
    }else if(node.name=="Texture"){
     const auto name=node.attribute("file");if(name.empty()||!texture_names.insert(name).second)throw std::runtime_error("HexModel authored texture declaration missing/duplicate");
    }
   }
   const std::set<std::string> selected_textures{"CenterOneTurnFrame.dds","CenterOneTurnFrameSpec.dds","CenterThreeTurnCenter.dds","CubeLight_Generic_32.dds","SpecGray.dds"};
   if(mesh_count!=1||animation_count!=1||texture_names!=selected_textures)throw std::runtime_error("HexModel selected authored dependency closure differs");
   const auto stage=[&](const std::string& name){
    full();bank->require_boundary();full();
    assets::CivisLoadedResource loaded;
    {const auto canonical=bank->retain_owner();full();if(canonical.get()!=bank.get()||!same(canonical,bank))throw std::runtime_error("HexModel actual Bank alias differs");
     canonical->load(assets::AssetKey(name));full();}
    full(); //after actual alias/loader targets retire
    {const auto capability=bank->capability();full();loaded=capability.loaded(assets::AssetKey(name));full();}
    full();if(!loaded.bytes||loaded.bytes->empty()||loaded.payload_identity.bytes!=loaded.bytes->size()||runtime::identify_bytes(*loaded.bytes).sha256!=loaded.payload_identity.sha256)throw std::runtime_error("HexModel SAME Bank returned missing/unidentified payload");
    state->inputs.push_back(loaded);full();return loaded;
   };
   {const auto mesh=stage("SelectedAniOneTurn.gr2");full();if(mesh.payload_identity.bytes!=pins.mesh_.bytes||mesh.payload_identity.sha256!=pins.mesh_.sha256)throw std::runtime_error("HexModel SDK export not bound to actual Bank Mesh bytes");}
   full();
   {const auto animation=stage("SelectedAniOneTurn_SelectedAniOneTurn.gr2");full();if(animation.payload_identity.bytes!=pins.animation_.bytes||animation.payload_identity.sha256!=pins.animation_.sha256)throw std::runtime_error("HexModel SDK export not bound to actual Bank Animation bytes");}
   full();for(const auto& name:texture_names){(void)stage(name);full();} //real authored resources; neutral exported material is ignored
   for(std::size_t i=0;i<pins.artifacts_.size();++i){full();const auto& artifact=pins.artifacts_[i];
    if(artifact.path.empty()||!artifact.path.is_absolute()||!artifact.identity.bytes||artifact.identity.bytes>64ULL*1024*1024||artifact.identity.sha256.size()!=64)throw std::runtime_error("HexModel completed Root export pin missing");
    auto bytes=std::make_shared<assets::Bytes>();full();
    {std::ifstream file(artifact.path,std::ios::binary);full();if(!file)throw std::runtime_error("HexModel pinned SDK document cannot open");
     bytes->resize(static_cast<std::size_t>(artifact.identity.bytes));full();
     file.read(reinterpret_cast<char*>(bytes->data()),static_cast<std::streamsize>(bytes->size()));full();if(!file||file.peek()!=std::char_traits<char>::eof())throw std::runtime_error("HexModel pinned SDK document extent changed");full();}
    full();const auto identity=runtime::identify_bytes(*bytes);full();if(identity.bytes!=artifact.identity.bytes||identity.sha256!=artifact.identity.sha256)throw std::runtime_error("HexModel actual SDK document digest changed");
    state->exports[i]=std::move(bytes);full();
   }
   full();state->model=Backend::State::assemble(state->exports,pins.mesh_,state->authored_animation_alias,state->authored_animation_event_code);full();
   for(std::size_t i=0;i<state->instances.size();++i){full();
    // OWN entering monotonic clock only. Never claims Original9671C0 snapshot.
    const auto clock=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    first();state->construct_instance(i,clock);full();
   }
   bank->require_boundary();full();
  }
  full(); //all parser/provider/Bank/loader targets have actually retired
  auto backend=std::shared_ptr<Backend>(new Backend(state));full();
  first();s->models[index].backend=std::move(backend);produced_backend=s->models[index].backend; //real own output, not a caller permission
  full();
  if(s->models[index].backend->state_.get()!=state.get())throw std::runtime_error("HexModel constructor return target replaced/retired");
  first();style_receiver->active_constructor=nullptr;invocation=nullptr;full();
  } //real constructor Scope and local model/target copies retire
  full();
  first();style_receiver->constructor_entered=false; //normal release; no later FULL/callback
 }catch(...){
  if(state)state->failed.store(true);s->failed.store(true);
  if(index<s->models.size()&& &s->models[index]==style_receiver&&style_receiver->active_constructor==invocation)
   style_receiver->active_constructor=nullptr;
  if(index<s->models.size()&& &s->models[index]==style_receiver)style_receiver->constructor_entered=false;
  invocation=nullptr;throw;
 }
}
