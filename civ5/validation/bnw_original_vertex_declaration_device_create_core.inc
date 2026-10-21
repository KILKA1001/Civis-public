// Include outside namespace civ5::ui after complete Ui Lifetime definitions.
// Exact Root InitPhase subset ABI is required. Root native-post successor must
// be joined before production publication; GPU-image phase is insufficient.
namespace civ5::ui {
std::shared_ptr<const detail::BnwOriginalVertexDeclarationDeviceResultLoan>
UiScreenGlRenderer::create_original_vertex_declaration_owned(
 const std::shared_ptr<const detail::BnwOriginalNonFontDeviceInitializer>& offered_init,
 const runtime::BnwWorldViewSourceOperationLoan& op,
 const detail::BnwOriginalVertexDeclarationCreateLoan& actual_create){
 using Pin=graphics::detail::BnwRendererContextPin;
 using Channel=graphics::detail::BnwOwnedGlRetirementNode::VertexDeclarationChannel;
 using Projection=graphics::detail::BnwOriginalVertexDeclarationGlSchema;
 using Result=detail::BnwOriginalVertexDeclarationDeviceResultLoan;
 const auto init=offered_init;const auto create=actual_create.retain_owner_owned_without_callbacks();
 const auto life=worldview_lifetime_;const auto*const renderer=this;
 const graphics::GlRenderApi render=gl_; // Actual Ui-loaded render dispatch, BEFORE Source/new_handler.
 const auto first=[&]{
  if(!life||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||life->renderer!=renderer||!init||!create)
   throw std::runtime_error("Declaration original renderer/Init/Create retired");
  op.require_origin_without_callbacks();init->require_physical_without_callbacks();create->require_physical_without_callbacks();
 };
 first();const auto canonical_init=init->retain_owner_owned_without_callbacks();first();
 if(canonical_init.get()!=init.get()||canonical_init.owner_before(init)||init.owner_before(canonical_init)||create.get()!=&actual_create)
  throw std::runtime_error("Declaration actual canonical Init/Create differs");
 const auto full=[&]{first();create->require_source_owned(op);first();};
 full();const auto epoch=init->initialization_epoch_without_callbacks();first();
 const auto slot=create->slot_owned(op);full();const auto elements=create->elements_owned(op);full();
 std::vector<detail::BnwOriginalVertexDeclarationElement> copied(elements.begin(),elements.end());full();
 const auto schema=Projection::project(copied);full();
 // Exact Root InitPhase1 ABI: genuine subset Window-issued Pin, not GPU phase.
 const auto pin=init->vertex_declaration_context_pin_owned(op,*create);full();
 const auto current=[&]{first();const auto*p=Pin::NormalAccess::api_current_without_switch_owned(pin);first();return p;};
 const graphics::GlApi api=*current();first();
 if(!api.GetIntegerv||!api.GetError||!api.GetGraphicsResetStatus||!render.CreateVertexArrays||!render.DeleteVertexArrays||
    !render.EnableVertexArrayAttrib||!render.VertexArrayAttribFormat||!render.VertexArrayAttribBinding)
  throw std::runtime_error("Declaration actual loaded DSA dispatch absent");
 std::array<GLenum,2> actual_post{};
 const auto physical=[&]{const auto*p=current();
  if(p->GetIntegerv!=api.GetIntegerv||p->GetError!=api.GetError||p->GetGraphicsResetStatus!=api.GetGraphicsResetStatus)
   throw std::runtime_error("Declaration physical dispatch changed");first();};
 const auto health=[&]{physical();actual_post[0]=api.GetGraphicsResetStatus();physical();
  if(actual_post[0]!=GL_NO_ERROR)throw std::runtime_error("Declaration actual device reset");
  actual_post[1]=api.GetError();physical();
  if(actual_post[1]!=GL_NO_ERROR)throw std::runtime_error("Declaration actual GL failure");};
 const auto effect=[&](auto&& call){full();health();physical();call();physical();health();full();};
 GLint max_attributes=0,max_bindings=0,max_offset=0;
 effect([&]{api.GetIntegerv(GL_MAX_VERTEX_ATTRIBS,&max_attributes);});
 effect([&]{api.GetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS,&max_bindings);});
 effect([&]{api.GetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET,&max_offset);});
 if(max_attributes<0||schema.attributes.size()>static_cast<std::size_t>(max_attributes)||max_bindings<1||max_offset<0)
  throw std::runtime_error("Declaration actual device limits insufficient");
 for(const auto&a:schema.attributes)if(a.relative_offset>static_cast<GLuint>(max_offset))
  throw std::runtime_error("Declaration actual relative offset exceeds device limit");
 full();auto node=Channel::CreationAccess::make_owned(pin,api,render);full();
 auto&vao=Channel::CreationAccess::name_storage_without_callbacks(*node);
 try{
  effect([&]{render.CreateVertexArrays(1,&vao);});
  if(!vao)throw std::runtime_error("Declaration actual CreateVertexArrays returned null");
  for(GLuint ordinal=0;ordinal<schema.attributes.size();++ordinal){const auto a=schema.attributes[ordinal];
   // Root-approved INTERNAL ordinal location only. Usage/index remains copied.
   effect([&]{render.VertexArrayAttribFormat(vao,ordinal,a.components,a.scalar_type,a.normalized,a.relative_offset);});
   effect([&]{render.VertexArrayAttribBinding(vao,ordinal,0);});
   effect([&]{render.EnableVertexArrayAttrib(vao,ordinal);});
  }
  full();health();
  // Root proposal exact setup gate is callback-free AFTER fresh Source FULL.
  init->require_vertex_declaration_setup_creation_owned(op);physical();health();
  const auto result=Result::seal_created_owned(init,create,pin,std::move(node),epoch,slot,std::move(copied),actual_post,api);
  full();init->require_vertex_declaration_setup_creation_owned(op);physical();health();
  result->require_physical_without_callbacks();first();return result;
 }catch(...){
  if(node)Channel::CreationAccess::quarantine_without_callbacks(*node);
  // Source/native failure poisons actual Root actor via its existing catch;
  // no false HRESULT/S_OK or successful null/skip result is returned.
  throw;
 }
}

void UiScreenGlRenderer::require_original_vertex_declaration_device_current_owned(
 const std::shared_ptr<const detail::BnwOriginalVertexDeclarationDeviceResultLoan>& offered,
 const runtime::BnwWorldViewSourceOperationLoan& op)const{
 using Pin=graphics::detail::BnwRendererContextPin;
 // Independent strong physical custody BEFORE any native observation. There
 // is no Source FULL, Root phase getter, allocating dispatch load or old Create.
 const auto result=offered;const auto life=worldview_lifetime_;const auto*const renderer=this;
 const auto first_renderer=[&]{
  if(!life||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||life->renderer!=renderer||!result)
   throw std::runtime_error("Declaration native post original renderer/result retired");
  op.require_origin_without_callbacks();result->require_physical_without_callbacks();
 };
 first_renderer();const auto original=result->retain_owner_owned_without_callbacks();first_renderer();
 if(original.get()!=result.get()||original.owner_before(result)||result.owner_before(original))
  throw std::runtime_error("Declaration native post result canonical control differs");
 const auto init=result->initializer_owner_owned_without_callbacks();first_renderer();
 const auto pin=result->context_pin_owned_without_callbacks();first_renderer();
 const graphics::GlApi api=result->physical_dispatch_without_callbacks();first_renderer();
 const auto physical=[&]{
  first_renderer();init->require_physical_without_callbacks();
  const auto*current=Pin::NormalAccess::api_current_without_switch_owned(pin);
  first_renderer();init->require_physical_without_callbacks();
  if(current->GetError!=api.GetError||current->GetGraphicsResetStatus!=api.GetGraphicsResetStatus)
   throw std::runtime_error("Declaration native post actual device dispatch differs");
 };
 // Journal in Result is creation evidence only. These are fresh observations
 // AFTER caller's last Root FULL; each has callback-free actual-current gates.
 physical();const auto reset=api.GetGraphicsResetStatus();physical();
 if(reset!=GL_NO_ERROR)throw std::runtime_error("Declaration native post actual reset");
 const auto error=api.GetError();physical();
 if(error!=GL_NO_ERROR)throw std::runtime_error("Declaration native post actual GL error");
 // Caller performs only Root PURE actual setup/table/schema/epoch matchers
 // afterwards. A later FULL must invoke this helper again before publishing.
}
}
