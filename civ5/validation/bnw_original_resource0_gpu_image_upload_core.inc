// Included outside namespace civ5::ui, after real Ui lifetime definitions.
// Raw format21 UNORM storage only: no sampler, ID, registration or Draw grant.
namespace civ5::ui {
std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan>
UiScreenGlRenderer::upload_original_resource0_storage_owned(
 const std::shared_ptr<const detail::BnwOriginalNonFontDeviceInitializer>& offered_init,
 const runtime::BnwWorldViewSourceOperationLoan& op,
 const std::shared_ptr<const detail::BnwOriginalGeneratedResourceImageLoan>& offered_cpu){
 using Pin=graphics::detail::BnwRendererContextPin;
 using Channel=graphics::detail::BnwOwnedGlRetirementNode::ResourceImageChannel;
 using Gpu=graphics::detail::BnwNormalResourceGpuImageLoan;
 // Capture immutable original owners before any Source/native/allocator cut.
 const auto init=offered_init;const auto cpu=offered_cpu;
 const auto life=worldview_lifetime_;const auto*const renderer=this;
 const auto first=[&]{
  if(!life||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||life->renderer!=renderer||!init||!cpu)
   throw std::runtime_error("GPU upload original renderer/Init/CPU retired");
  op.require_origin_without_callbacks();init->require_physical_without_callbacks();cpu->require_physical_without_callbacks();
 };
 first();const auto i=init->retain_owner_owned_without_callbacks();
 const auto c=cpu->retain_owner_owned_without_callbacks();first();
 if(i.get()!=init.get()||i.owner_before(init)||init.owner_before(i)||
    c.get()!=cpu.get()||c.owner_before(cpu)||cpu.owner_before(c))
  throw std::runtime_error("GPU upload offered canonical owners differ");
 const auto full=[&]{first();init->require_gpu_resource0_creation_owned(op,cpu);first();};
 full();const auto pin=init->renderer_context_pin_owned(op,cpu);full();
 const auto epoch=init->initialization_epoch_without_callbacks();first();
 const auto current=[&]{first();const auto*api=Pin::NormalAccess::api_current_without_switch_owned(pin);first();return api;};
 const graphics::GlApi api=*current();first();
 if(!api.GetIntegerv||!api.GetError||!api.GetGraphicsResetStatus||!api.CreateTextures||!api.DeleteTextures||
    !api.TextureStorage2D||!api.TextureSubImage2D||!api.TextureParameteri||!api.PixelStorei||!api.BindBuffer)
  throw std::runtime_error("GPU upload actual loaded dispatch incomplete");
 const auto physical=[&]{const auto*p=current();
  if(p->GetIntegerv!=api.GetIntegerv||p->GetError!=api.GetError||p->GetGraphicsResetStatus!=api.GetGraphicsResetStatus||
     p->CreateTextures!=api.CreateTextures||p->DeleteTextures!=api.DeleteTextures||p->TextureStorage2D!=api.TextureStorage2D||
     p->TextureSubImage2D!=api.TextureSubImage2D||p->TextureParameteri!=api.TextureParameteri||
     p->PixelStorei!=api.PixelStorei||p->BindBuffer!=api.BindBuffer)
   throw std::runtime_error("GPU upload loaded dispatch changed");first();};
 const auto health=[&]{physical();const auto reset=api.GetGraphicsResetStatus();physical();
  if(reset!=GL_NO_ERROR)throw std::runtime_error("GPU upload context reset");
  const auto error=api.GetError();physical();
  if(error!=GL_NO_ERROR)throw std::runtime_error("GPU upload native error");};
 const auto effect=[&](auto&& call){full();health();physical();call();physical();health();full();};
 full();const auto dims=cpu->dimensions_owned(op);full();
 const auto format=cpu->original_d3d_format_owned(op);full();
 const auto count=cpu->original_level_count_owned(op);full();
 const auto cpu_epoch=cpu->original_initialization_epoch_owned(op);full();
 const auto levels=cpu->bgra8_levels_owned(op);full();
 if(dims!=std::array<std::uint32_t,2>{256,256}||format!=21||count!=7||levels.size()!=7||cpu_epoch!=epoch)
  throw std::runtime_error("GPU upload requires authentic format21 seven-level image");
 for(std::uint32_t level=0;level<7;++level){
  const auto pitch=cpu->row_pitch_owned(op,level);full();const auto size=256u>>level;
  const auto&v=levels[level];
  if(v.width!=size||v.height!=size||v.row_pitch!=size*4||pitch!=v.row_pitch||
     v.bgra.size()!=std::size_t(size)*size*4||!v.bgra.data())
   throw std::runtime_error("GPU upload immutable mip extent/pitch differs");
 }
 GLint limit=0;effect([&]{api.GetIntegerv(GL_MAX_TEXTURE_SIZE,&limit);});
 if(limit<256)throw std::runtime_error("GPU upload exceeds actual texture limit");
 full();auto texture=Channel::CreationAccess::make_image_owned(pin,api);full();
 auto unpack=Channel::CreationAccess::make_unpack_owned(pin,api);full();
 auto&name=Channel::CreationAccess::image_name_storage_without_callbacks(*texture);
 auto&state=Channel::CreationAccess::unpack_storage_without_callbacks(*unpack);
 constexpr std::array<GLenum,12> fields{GL_UNPACK_ALIGNMENT,GL_UNPACK_ROW_LENGTH,GL_UNPACK_IMAGE_HEIGHT,
  GL_UNPACK_SKIP_PIXELS,GL_UNPACK_SKIP_ROWS,GL_UNPACK_SKIP_IMAGES,GL_UNPACK_SWAP_BYTES,GL_UNPACK_LSB_FIRST,
  GL_UNPACK_COMPRESSED_BLOCK_WIDTH,GL_UNPACK_COMPRESSED_BLOCK_HEIGHT,GL_UNPACK_COMPRESSED_BLOCK_DEPTH,GL_UNPACK_COMPRESSED_BLOCK_SIZE};
 const auto baseline=[&]{
  physical();health();
  for(std::size_t n=0;n<fields.size();++n){GLint value=0;physical();api.GetIntegerv(fields[n],&value);physical();health();
   if(value!=state.saved[n])throw std::runtime_error("GPU upload unpack restoration differs");}
  GLint buffer=0;physical();api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&buffer);physical();health();
  if(buffer!=state.buffer)throw std::runtime_error("GPU upload PBO restoration differs");
 };
 try{
  // One actual tuple: no Source/allocator callback is interleaved with capture.
  full();health();
  for(std::size_t n=0;n<fields.size();++n){physical();api.GetIntegerv(fields[n],&state.saved[n]);physical();health();state.captured=n+1;}
  physical();api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&state.buffer);physical();health();state.buffer_captured=true;
  full();
  // Saved flags arm BEFORE each real mutation, including a throwing POST.
  effect([&]{state.buffer_changed=true;api.BindBuffer(GL_PIXEL_UNPACK_BUFFER,0);});
  for(std::size_t n=0;n<fields.size();++n)
   effect([&]{state.changed=n+1;api.PixelStorei(fields[n],n==0?1:0);});
  effect([&]{api.CreateTextures(GL_TEXTURE_2D,1,&name);});
  if(!name)throw std::runtime_error("GPU upload generated no texture name");
  effect([&]{api.TextureStorage2D(name,7,GL_RGBA8,256,256);});
  for(std::uint32_t level=0;level<7;++level){
   // FULL may run callbacks. Recheck actual unpack values immediately before upload.
   full();health();
   for(std::size_t n=0;n<fields.size();++n){GLint value=0;physical();api.GetIntegerv(fields[n],&value);physical();health();
    if(value!=(n==0?1:0))throw std::runtime_error("GPU upload normalized unpack state changed");}
   GLint buffer=0;physical();api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&buffer);physical();health();
   if(buffer)throw std::runtime_error("GPU upload PBO rebound before owned bytes");
   physical();const auto&v=levels[level];
   api.TextureSubImage2D(name,static_cast<GLint>(level),0,0,static_cast<GLsizei>(v.width),static_cast<GLsizei>(v.height),
    GL_BGRA,GL_UNSIGNED_BYTE,v.bgra.data());physical();health();full();
  }
  effect([&]{api.TextureParameteri(name,GL_TEXTURE_BASE_LEVEL,0);});
  effect([&]{api.TextureParameteri(name,GL_TEXTURE_MAX_LEVEL,6);});
  while(state.changed){const auto n=state.changed-1;
   effect([&]{api.PixelStorei(fields[n],state.saved[n]);});state.changed=n;}
  effect([&]{api.BindBuffer(GL_PIXEL_UNPACK_BUFFER,static_cast<GLuint>(state.buffer));});state.buffer_changed=false;
  const auto restored=Channel::CreationAccess::restore_unpack_without_switch(*unpack);
  if(!restored.complete||restored.cleanup_failure)throw std::runtime_error("GPU upload unpack restoration incomplete");
  full();baseline();
  const auto result=Gpu::seal_uploaded_owned(init,cpu,pin,std::move(texture),epoch,op);
  full();baseline();result->require_same_inputs_without_callbacks(init,cpu,pin,epoch);
  result->require_source_owned(op);first();baseline();return result;
 }catch(...){
  // Only the actual owned image/unpack are retired; no registration or result escapes.
  const auto restored=Channel::CreationAccess::restore_unpack_without_switch(*unpack);
  if(texture)Channel::CreationAccess::quarantine_image_without_callbacks(*texture);
  (void)restored;throw;
 }
}
void UiScreenGlRenderer::require_original_resource0_storage_current_owned(
 const std::shared_ptr<const detail::BnwOriginalNonFontDeviceInitializer>& offered_init,
 const runtime::BnwWorldViewSourceOperationLoan& op,
 const std::shared_ptr<const detail::BnwOriginalGeneratedResourceImageLoan>& offered_cpu,
 const std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan>& offered_gpu)const{
 const auto init=offered_init;const auto cpu=offered_cpu;const auto gpu=offered_gpu;
 const auto life=worldview_lifetime_;const auto*const renderer=this;
 const auto first=[&]{if(!life||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||life->renderer!=renderer||!init||!cpu||!gpu)
  throw std::runtime_error("GPU current original renderer/image controls retired");
  op.require_origin_without_callbacks();init->require_physical_without_callbacks();cpu->require_physical_without_callbacks();
  gpu->require_physical_without_callbacks();};
 first();const auto i=init->retain_owner_owned_without_callbacks();const auto c=cpu->retain_owner_owned_without_callbacks();
 const auto g=gpu->retain_owner_owned_without_callbacks();first();
 if(i.get()!=init.get()||i.owner_before(init)||init.owner_before(i)||c.get()!=cpu.get()||c.owner_before(cpu)||cpu.owner_before(c)||
    g.get()!=gpu.get()||g.owner_before(gpu)||gpu.owner_before(g))throw std::runtime_error("GPU current canonical owners differ");
 const auto epoch=init->initialization_epoch_without_callbacks();first();
 init->require_gpu_resource0_image_current_owned(op,cpu,epoch);first();
 const auto pin=init->renderer_context_pin_owned(op,cpu);first();
 gpu->require_same_inputs_without_callbacks(init,cpu,pin,epoch);first();
 (void)graphics::detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);first();
 gpu->require_source_owned(op);first();
 init->require_gpu_resource0_image_current_owned(op,cpu,epoch);first();
 // The last FULL can run callbacks. No callbackful cut follows these actual
 // current/reset/error observations; this checks storage currentness only.
 using Pin=graphics::detail::BnwRendererContextPin;
 const graphics::GlApi api=*Pin::NormalAccess::api_current_without_switch_owned(pin);first();
 if(!api.GetGraphicsResetStatus||!api.GetError)throw std::runtime_error("GPU current health dispatch absent");
 const auto physical=[&]{first();
  const auto*p=Pin::NormalAccess::api_current_without_switch_owned(pin);first();
  if(p->GetGraphicsResetStatus!=api.GetGraphicsResetStatus||p->GetError!=api.GetError)
   throw std::runtime_error("GPU current health dispatch changed");
  gpu->require_same_inputs_without_callbacks(init,cpu,pin,epoch);first();};
 physical();const auto reset=api.GetGraphicsResetStatus();physical();
 if(reset!=GL_NO_ERROR)throw std::runtime_error("GPU current context reset");
 const auto error=api.GetError();physical();
 if(error!=GL_NO_ERROR)throw std::runtime_error("GPU current native error");
}
}
