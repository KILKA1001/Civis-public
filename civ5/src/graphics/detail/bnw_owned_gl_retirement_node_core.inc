// Included only by the Window TU; no independently callable cleanup surface.
namespace civ5::graphics::detail {
BnwOwnedGlRetirementNode::BnwOwnedGlRetirementNode(
 std::shared_ptr<const BnwRendererContextPin> pin,Kind kind,DeleteDispatch dispatch)noexcept
 :pin_(std::move(pin)),creator_(std::this_thread::get_id()),kind_(kind),dispatch_(dispatch){}
BnwOwnedGlRetirementNode::~BnwOwnedGlRetirementNode()noexcept{
 // Only the closed creator drain deletes nodes. Never silently lose IDs/Pin.
 if(creator_!=std::this_thread::get_id()||queue_custody_||
    quarantined_||deletion_in_flight_||!empty_without_callbacks())std::terminate();
}
void BnwOwnedGlRetirementNode::RetireDeleter::operator()(BnwOwnedGlRetirementNode* node)const noexcept{
 if(node)BnwOwnedGlRetirementNode::enqueue_owned(node);
}
BnwOwnedGlRetirementNode::Owner BnwOwnedGlRetirementNode::texture_pair_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,const GlApi& api){
 if(!pin)throw std::runtime_error("Owned texture retirement requires genuine Context Pin");
 pin->first_without_callbacks();
 const DeleteDispatch dispatch{api.GetError,api.GetGraphicsResetStatus,
  api.DeleteTextures,nullptr,nullptr,nullptr,nullptr,nullptr};
 if(!dispatch.error||!dispatch.reset||!dispatch.textures)
  throw std::runtime_error("Loaded texture cleanup dispatch unavailable");
 Owner node(new BnwOwnedGlRetirementNode(pin,Kind::texture_pair,dispatch));
 pin->first_without_callbacks();
 return node;
}
BnwOwnedGlRetirementNode::Owner BnwOwnedGlRetirementNode::shader_resources_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,const GlApi& physical_api,const GlRenderApi& api){
 if(!pin)throw std::runtime_error("Owned shader retirement requires genuine Context Pin");
 pin->first_without_callbacks();
 const DeleteDispatch dispatch{physical_api.GetError,physical_api.GetGraphicsResetStatus,
  nullptr,api.DeleteShader,api.DeleteProgram,
  api.DeleteVertexArrays,api.DeleteBuffers,api.DeleteSamplers};
 if(!dispatch.error||!dispatch.reset||!dispatch.stage||!dispatch.program||
    !dispatch.vao||!dispatch.buffers||!dispatch.sampler)
  throw std::runtime_error("Loaded shader deletion dispatch unavailable");
 Owner node(new BnwOwnedGlRetirementNode(pin,Kind::shader_resources,dispatch));
 pin->first_without_callbacks();
 return node;
}
BnwOwnedGlRetirementNode::TexturePairStorage&
BnwOwnedGlRetirementNode::texture_pair_storage_without_callbacks()noexcept{
 if(kind_!=Kind::texture_pair||queue_custody_)std::terminate();
 return pair_;
}
BnwOwnedGlRetirementNode::ShaderResourcesStorage&
BnwOwnedGlRetirementNode::shader_resources_storage_without_callbacks()noexcept{
 if(kind_!=Kind::shader_resources||queue_custody_)std::terminate();
 return shader_;
}

BnwOwnedGlRetirementNode::Owner BnwOwnedGlRetirementNode::PixelUnpackChannel::pixel_unpack_restore_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,const GlApi& api){
 return BnwOwnedGlRetirementNode::make_pixel_unpack_restore_owned(std::move(pin),api);
}
BnwOwnedGlRetirementNode::PixelUnpackStorage& BnwOwnedGlRetirementNode::PixelUnpackChannel::storage(
 BnwOwnedGlRetirementNode& node)noexcept{return node.pixel_unpack_storage_without_callbacks();}
BnwOwnedGlRetirementNode::RestoreStatus BnwOwnedGlRetirementNode::PixelUnpackChannel::restore(
 BnwOwnedGlRetirementNode& node)noexcept{return node.restore_pixel_unpack_if_current_without_switch();}
BnwOwnedGlRetirementNode::Owner BnwOwnedGlRetirementNode::make_pixel_unpack_restore_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,const GlApi& api){
 if(!pin)throw std::runtime_error("Owned unpack restore requires genuine Context Pin");
 pin->first_without_callbacks();
 // Snapshot ALL actual loaded dispatch by VALUE before operator new/new_handler.
 // No reference to the producer facade or borrowed dispatch is read afterwards.
 const DeleteDispatch dispatch{api.GetError,api.GetGraphicsResetStatus,
  nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,api.PixelStorei,api.BindBuffer};
 if(!dispatch.error||!dispatch.reset||!dispatch.unpack_store||!dispatch.unpack_bind)
  throw std::runtime_error("Loaded unpack restoration dispatch unavailable");
 Owner node(new BnwOwnedGlRetirementNode(pin,Kind::pixel_unpack_restore,dispatch));
 pin->first_without_callbacks();return node;
}
BnwOwnedGlRetirementNode::PixelUnpackStorage&
BnwOwnedGlRetirementNode::pixel_unpack_storage_without_callbacks()noexcept{
 if(kind_!=Kind::pixel_unpack_restore||queue_custody_||quarantined_)std::terminate();
 return unpack_;
}
void BnwOwnedGlRetirementNode::quarantine_unpack_without_callbacks(Failure failure)noexcept{
 unpack_.uncertain=true;unpack_.restored=false;quarantine_without_callbacks(failure);
}
const GlApi* BnwOwnedGlRetirementNode::clean_unpack_dispatch_without_switch()noexcept{
 const auto* api=BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_);
 if(!api)return nullptr; // Wrong-current/foreign means pending, no effect.
 if(api->GetError!=dispatch_.error||api->GetGraphicsResetStatus!=dispatch_.reset||
    api->PixelStorei!=dispatch_.unpack_store||api->BindBuffer!=dispatch_.unpack_bind){
  quarantine_unpack_without_callbacks(Failure::dispatch_mismatch);return nullptr;
 }
 // Pure physical current PRE/POST for EACH loaded native observation. No Source,
 // Paint admission, Context facade read/api(), proc load or current switch.
 const auto reset=dispatch_.reset();
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_unpack_without_callbacks(Failure::pre_observation_lost);return nullptr;
 }
 if(reset!=GL_NO_ERROR){quarantine_unpack_without_callbacks(Failure::preexisting_reset);return nullptr;}
 const auto error=dispatch_.error();
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_unpack_without_callbacks(Failure::pre_observation_lost);return nullptr;
 }
 if(error!=GL_NO_ERROR){quarantine_unpack_without_callbacks(Failure::preexisting_error);return nullptr;}
 return api;
}
bool BnwOwnedGlRetirementNode::unpack_post_without_switch(const GlApi* api)noexcept{
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_unpack_without_callbacks(Failure::post_observation_lost);return false;
 }
 const auto reset=dispatch_.reset();
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_unpack_without_callbacks(Failure::post_observation_lost);return false;
 }
 if(reset!=GL_NO_ERROR){quarantine_unpack_without_callbacks(Failure::post_reset);return false;}
 const auto error=dispatch_.error();
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_unpack_without_callbacks(Failure::post_observation_lost);return false;
 }
 if(error!=GL_NO_ERROR){quarantine_unpack_without_callbacks(Failure::post_error);return false;}
 return true;
}
BnwOwnedGlRetirementNode::RestoreStatus BnwOwnedGlRetirementNode::restore_pixel_unpack_if_current_without_switch()noexcept{
 RestoreStatus status;
 status.cleanup_failure=sticky_cleanup_failure_without_callbacks();
 if(kind_!=Kind::pixel_unpack_restore||creator_!=std::this_thread::get_id()||
    quarantined_||unpack_.uncertain)return status;
 if(unpack_.captured>pixel_unpack_fields_.size()||unpack_.changed>unpack_.captured||
    (unpack_.buffer_changed&&!unpack_.buffer_captured)){
  quarantine_unpack_without_callbacks(Failure::unpack_capture_invalid);
  status.cleanup_failure=true;return status;
 }
 // No changes means no native restoration effects. This covers factory/query
 // construction failure before any state change, still creator-only Pin drop.
 if(!unpack_.changed&&!unpack_.buffer_changed){
  unpack_.restored=true;status.complete=true;return status;
 }
 unpack_.restored=false;
 while(unpack_.changed){
  const auto index=unpack_.changed-1;
  const auto field=pixel_unpack_fields_[index];const auto value=unpack_.saved[index];
  const auto* api=clean_unpack_dispatch_without_switch();
  if(!api){status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;}
  // Arm BEFORE the effect. Changed count/saved VALUE remain intact until its
  // full physical reset/error POST; unverified effect is never retried.
  unpack_.uncertain=true;quarantined_=true;
  dispatch_.unpack_store(field,value);
  if(!unpack_post_without_switch(api)){status.cleanup_failure=true;return status;}
  unpack_.changed=index;unpack_.uncertain=false;quarantined_=false;
 }
 if(unpack_.buffer_changed){
  const auto buffer=static_cast<GLuint>(unpack_.buffer);
  const auto* api=clean_unpack_dispatch_without_switch();
  if(!api){status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;}
  unpack_.uncertain=true;quarantined_=true;
  dispatch_.unpack_bind(GL_PIXEL_UNPACK_BUFFER,buffer);
  if(!unpack_post_without_switch(api)){status.cleanup_failure=true;return status;}
  unpack_.buffer_changed=false;unpack_.uncertain=false;quarantined_=false;
 }
 unpack_.restored=true;status.complete=true;
 status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
}

BnwOwnedGlRetirementNode::TargetRowsChannel::Owner
BnwOwnedGlRetirementNode::TargetRowsChannel::make(
 std::shared_ptr<const BnwRendererContextPin> pin,GlApi api,GlRenderApi render){
 return BnwOwnedGlRetirementNode::make_target_rows_restore_owned(std::move(pin),api,render);
}
BnwOwnedGlRetirementNode::TargetRowsChannel::Storage&
BnwOwnedGlRetirementNode::TargetRowsChannel::storage(BnwOwnedGlRetirementNode& node)noexcept{
 return node.target_rows_storage_without_callbacks();
}
BnwOwnedGlRetirementNode::TargetRowsChannel::RestoreStatus
BnwOwnedGlRetirementNode::TargetRowsChannel::restore(BnwOwnedGlRetirementNode& node)noexcept{
 return node.restore_target_rows_if_current_without_switch();
}
BnwOwnedGlRetirementNode::TargetRowsChannel::DrainStatus
BnwOwnedGlRetirementNode::TargetRowsChannel::drain()noexcept{
 // Captured VALUE status only. All-kind pending/incomplete/failure conservatively
 // stops a subsequent Ui row mutation; no queue-empty/Target/Source claim.
 const auto status=BnwOwnedGlRetirementNode::drain_current_creator_owned();
 return status;
}
BnwOwnedGlRetirementNode::Owner BnwOwnedGlRetirementNode::make_target_rows_restore_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,GlApi api,GlRenderApi render){
 // SAME Pin and dispatch values are owned BEFORE any allocation/new_handler.
 if(!pin)throw std::runtime_error("Target rows restore requires genuine Context Pin");
 pin->first_without_callbacks();
 DeleteDispatch dispatch{};dispatch.error=api.GetError;dispatch.reset=api.GetGraphicsResetStatus;
 dispatch.target_depth=render.DepthRangeIndexed;dispatch.target_viewport=render.ViewportIndexedf;
 dispatch.target_clip=render.ClipControl;
 if(!dispatch.error||!dispatch.reset||!dispatch.target_depth||
    !dispatch.target_viewport||!dispatch.target_clip)
  throw std::runtime_error("Loaded indexed target rows restoration dispatch unavailable");
 Owner node(new BnwOwnedGlRetirementNode(pin,Kind::target_rows_restore,dispatch));
 pin->first_without_callbacks();return node;
}
BnwOwnedGlRetirementNode::TargetRowsStorage&
BnwOwnedGlRetirementNode::target_rows_storage_without_callbacks()noexcept{
 if(kind_!=Kind::target_rows_restore||queue_custody_||quarantined_||
    target_rows_restore_active_)std::terminate();
 return target_rows_;
}
void BnwOwnedGlRetirementNode::quarantine_target_rows_without_callbacks(Failure reason)noexcept{
 target_rows_.uncertain=true;target_rows_.restored=false;quarantine_without_callbacks(reason);
}
const GlApi* BnwOwnedGlRetirementNode::target_rows_physical_without_switch()noexcept{
 if(failure_!=Failure::none)return nullptr;
 const auto* api=BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_);
 // CleanupAccess itself observes WGL. Reentered restore can poison this node
 // even when those observations ultimately return the SAME physical API.
 if(failure_!=Failure::none)return nullptr;
 return api;
}
const GlApi* BnwOwnedGlRetirementNode::clean_target_rows_dispatch_without_switch()noexcept{
 if(failure_!=Failure::none)return nullptr;
 const auto* api=target_rows_physical_without_switch();
 if(!api)return nullptr; // Wrong-current/foreign is pending without effects.
 if(api->GetError!=dispatch_.error||api->GetGraphicsResetStatus!=dispatch_.reset){
  quarantine_target_rows_without_callbacks(Failure::dispatch_mismatch);return nullptr;
 }
 const auto reset=dispatch_.reset();
 if(failure_!=Failure::none)return nullptr;
 if(target_rows_physical_without_switch()!=api){
  quarantine_target_rows_without_callbacks(Failure::pre_observation_lost);return nullptr;
 }
 if(reset!=GL_NO_ERROR){quarantine_target_rows_without_callbacks(Failure::preexisting_reset);return nullptr;}
 const auto error=dispatch_.error();
 if(failure_!=Failure::none)return nullptr;
 if(target_rows_physical_without_switch()!=api){
  quarantine_target_rows_without_callbacks(Failure::pre_observation_lost);return nullptr;
 }
 if(error!=GL_NO_ERROR){quarantine_target_rows_without_callbacks(Failure::preexisting_error);return nullptr;}
 return api;
}
bool BnwOwnedGlRetirementNode::target_rows_post_without_switch(const GlApi* api)noexcept{
 if(failure_!=Failure::none)return false;
 if(target_rows_physical_without_switch()!=api){
  quarantine_target_rows_without_callbacks(Failure::post_observation_lost);return false;
 }
 const auto reset=dispatch_.reset();
 if(failure_!=Failure::none)return false;
 if(target_rows_physical_without_switch()!=api){
  quarantine_target_rows_without_callbacks(Failure::post_observation_lost);return false;
 }
 if(reset!=GL_NO_ERROR){quarantine_target_rows_without_callbacks(Failure::post_reset);return false;}
 const auto error=dispatch_.error();
 if(failure_!=Failure::none)return false;
 if(target_rows_physical_without_switch()!=api){
  quarantine_target_rows_without_callbacks(Failure::post_observation_lost);return false;
 }
 if(error!=GL_NO_ERROR){quarantine_target_rows_without_callbacks(Failure::post_error);return false;}
 return true;
}
BnwOwnedGlRetirementNode::RestoreStatus
BnwOwnedGlRetirementNode::restore_target_rows_if_current_without_switch()noexcept{
 RestoreStatus status;status.cleanup_failure=sticky_cleanup_failure_without_callbacks();
 if(kind_!=Kind::target_rows_restore||creator_!=std::this_thread::get_id())return status;
 if(target_rows_restore_active_){
  quarantine_target_rows_without_callbacks(Failure::target_rows_reentry);
  status.cleanup_failure=true;return status;
 }
 if(quarantined_||target_rows_.uncertain)return status;
 target_rows_restore_active_=true;
 struct ActiveReset final {bool& active;~ActiveReset(){active=false;}} active{target_rows_restore_active_};
 if((target_rows_.depth_changed&&!target_rows_.depth_captured)||
    (target_rows_.viewport_changed&&!target_rows_.viewport_captured)||
    (target_rows_.clip_changed&&!target_rows_.clip_captured)){
  quarantine_target_rows_without_callbacks(Failure::target_rows_capture_invalid);
  status.cleanup_failure=true;return status;
 }
 // Reverse producer order. Index 0 setters leave other viewport slots intact.
 target_rows_.restored=false;
 if(target_rows_.depth_changed){
  const auto saved=target_rows_.depth;const auto* api=clean_target_rows_dispatch_without_switch();
  if(!api){status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;}
  target_rows_.uncertain=true;quarantined_=true;
  dispatch_.target_depth(0,saved[0],saved[1]);
  if(!target_rows_post_without_switch(api)){status.cleanup_failure=true;return status;}
  target_rows_.depth_changed=false;target_rows_.uncertain=false;quarantined_=false;
 }
 if(target_rows_.viewport_changed){
  const auto saved=target_rows_.viewport;const auto* api=clean_target_rows_dispatch_without_switch();
  if(!api){status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;}
  target_rows_.uncertain=true;quarantined_=true;
  dispatch_.target_viewport(0,saved[0],saved[1],saved[2],saved[3]);
  if(!target_rows_post_without_switch(api)){status.cleanup_failure=true;return status;}
  target_rows_.viewport_changed=false;target_rows_.uncertain=false;quarantined_=false;
 }
 if(target_rows_.clip_changed){
  const auto origin=static_cast<GLenum>(target_rows_.clip_origin);
  const auto mode=static_cast<GLenum>(target_rows_.clip_depth_mode);
  const auto* api=clean_target_rows_dispatch_without_switch();
  if(!api){status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;}
  target_rows_.uncertain=true;quarantined_=true;dispatch_.target_clip(origin,mode);
  if(!target_rows_post_without_switch(api)){status.cleanup_failure=true;return status;}
  target_rows_.clip_changed=false;target_rows_.uncertain=false;quarantined_=false;
 }
 target_rows_.restored=true;status.complete=true;
 status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
}

BnwOwnedGlRetirementNode::FontPipelineChannel::Owner
BnwOwnedGlRetirementNode::FontPipelineChannel::make(
 std::shared_ptr<const BnwRendererContextPin> pin,GlApi api,GlRenderApi render){
 return BnwOwnedGlRetirementNode::make_font_pipeline_restore_owned(std::move(pin),api,render);
}
BnwOwnedGlRetirementNode::FontPipelineChannel::Storage&
BnwOwnedGlRetirementNode::FontPipelineChannel::storage(BnwOwnedGlRetirementNode& node)noexcept{
 return node.font_pipeline_storage_without_callbacks();
}
BnwOwnedGlRetirementNode::FontPipelineChannel::RestoreStatus
BnwOwnedGlRetirementNode::FontPipelineChannel::restore(BnwOwnedGlRetirementNode& node)noexcept{
 return node.restore_font_pipeline_if_current_without_switch();
}
BnwOwnedGlRetirementNode::FontPipelineChannel::DrainStatus
BnwOwnedGlRetirementNode::FontPipelineChannel::drain()noexcept{
 return BnwOwnedGlRetirementNode::drain_current_creator_owned();
}
BnwOwnedGlRetirementNode::Owner BnwOwnedGlRetirementNode::make_font_pipeline_restore_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,GlApi api,GlRenderApi render){
 if(!pin)throw std::runtime_error("Font pipeline restore requires genuine Context Pin");
 pin->first_without_callbacks();
 // ALL arguments/dispatch and SAME Pin captured by VALUE before allocator effects.
 DeleteDispatch dispatch{};dispatch.error=api.GetError;dispatch.reset=api.GetGraphicsResetStatus;
 dispatch.pipeline_enablei=render.Enablei;dispatch.pipeline_disablei=render.Disablei;
 dispatch.pipeline_blend_factors=render.BlendFuncSeparatei;
 dispatch.pipeline_blend_equations=render.BlendEquationSeparatei;dispatch.pipeline_color_mask=render.ColorMaski;
 dispatch.pipeline_enable=render.Enable;dispatch.pipeline_disable=render.Disable;
 dispatch.pipeline_depth_mask=render.DepthMask;dispatch.pipeline_depth_func=render.DepthFunc;
 dispatch.pipeline_polygon_mode=render.PolygonMode;dispatch.pipeline_program=render.UseProgram;
 dispatch.pipeline_vao=render.BindVertexArray;dispatch.pipeline_sampler=render.BindSampler;
 dispatch.pipeline_active_texture=render.ActiveTexture;dispatch.pipeline_texture=api.BindTexture;
 if(!dispatch.error||!dispatch.reset||!dispatch.pipeline_enablei||!dispatch.pipeline_disablei||
    !dispatch.pipeline_blend_factors||!dispatch.pipeline_blend_equations||!dispatch.pipeline_color_mask||
    !dispatch.pipeline_enable||!dispatch.pipeline_disable||!dispatch.pipeline_depth_mask||
    !dispatch.pipeline_depth_func||!dispatch.pipeline_polygon_mode||!dispatch.pipeline_program||
    !dispatch.pipeline_vao||!dispatch.pipeline_sampler||!dispatch.pipeline_active_texture||
    !dispatch.pipeline_texture||!render.GetIntegeri_v||!render.GetBooleanv||
    !render.GetBooleani_v||!render.IsEnabledi||!render.IsEnabled||!api.GetIntegerv)
  throw std::runtime_error("Loaded font pipeline capture/restoration dispatch unavailable");
 Owner node(new BnwOwnedGlRetirementNode(pin,Kind::font_pipeline_restore,dispatch));
 pin->first_without_callbacks();return node;
}
BnwOwnedGlRetirementNode::FontPipelineStorage&
BnwOwnedGlRetirementNode::font_pipeline_storage_without_callbacks()noexcept{
 if(kind_!=Kind::font_pipeline_restore||queue_custody_||quarantined_||
    font_pipeline_restore_active_)std::terminate();
 return font_pipeline_;
}
void BnwOwnedGlRetirementNode::quarantine_font_pipeline_without_callbacks(Failure reason)noexcept{
 font_pipeline_.uncertain=true;font_pipeline_.restored=false;quarantine_without_callbacks(reason);
}
const GlApi* BnwOwnedGlRetirementNode::font_pipeline_physical_without_switch()noexcept{
 if(failure_!=Failure::none)return nullptr;
 const auto* api=BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_);
 // CleanupAccess itself observes WGL. Reentered restore can poison this node
 // even when those observations ultimately return the SAME physical API.
 if(failure_!=Failure::none)return nullptr;
 return api;
}
const GlApi* BnwOwnedGlRetirementNode::clean_font_pipeline_dispatch_without_switch()noexcept{
 if(failure_!=Failure::none)return nullptr;
 const auto* api=font_pipeline_physical_without_switch();
 if(!api)return nullptr; // Wrong-current/foreign is pending without effects.
 if(api->GetError!=dispatch_.error||api->GetGraphicsResetStatus!=dispatch_.reset||
    api->BindTexture!=dispatch_.pipeline_texture){
  quarantine_font_pipeline_without_callbacks(Failure::dispatch_mismatch);return nullptr;
 }
 const auto reset=dispatch_.reset();
 if(failure_!=Failure::none)return nullptr;
 if(font_pipeline_physical_without_switch()!=api){
  quarantine_font_pipeline_without_callbacks(Failure::pre_observation_lost);return nullptr;
 }
 if(reset!=GL_NO_ERROR){quarantine_font_pipeline_without_callbacks(Failure::preexisting_reset);return nullptr;}
 const auto error=dispatch_.error();
 if(failure_!=Failure::none)return nullptr;
 if(font_pipeline_physical_without_switch()!=api){
  quarantine_font_pipeline_without_callbacks(Failure::pre_observation_lost);return nullptr;
 }
 if(error!=GL_NO_ERROR){quarantine_font_pipeline_without_callbacks(Failure::preexisting_error);return nullptr;}
 return api;
}
bool BnwOwnedGlRetirementNode::font_pipeline_post_without_switch(const GlApi* api)noexcept{
 if(failure_!=Failure::none)return false;
 if(font_pipeline_physical_without_switch()!=api){
  quarantine_font_pipeline_without_callbacks(Failure::post_observation_lost);return false;
 }
 const auto reset=dispatch_.reset();
 if(failure_!=Failure::none)return false;
 if(font_pipeline_physical_without_switch()!=api){
  quarantine_font_pipeline_without_callbacks(Failure::post_observation_lost);return false;
 }
 if(reset!=GL_NO_ERROR){quarantine_font_pipeline_without_callbacks(Failure::post_reset);return false;}
 const auto error=dispatch_.error();
 if(failure_!=Failure::none)return false;
 if(font_pipeline_physical_without_switch()!=api){
  quarantine_font_pipeline_without_callbacks(Failure::post_observation_lost);return false;
 }
 if(error!=GL_NO_ERROR){quarantine_font_pipeline_without_callbacks(Failure::post_error);return false;}
 return true;
}
bool BnwOwnedGlRetirementNode::font_pipeline_has_changes_without_callbacks()const noexcept{
 const auto& s=font_pipeline_;
 return s.blend_factors_changed||s.blend_equations_changed||s.color_mask_changed||s.blend_enabled_changed||s.depth_enabled_changed||s.depth_mask_changed||s.depth_func_changed||s.cull_enabled_changed||s.stencil_enabled_changed||s.polygon_mode_changed||s.program_changed||s.vao_changed||s.active_texture_changed||
  s.textures_changed[0]||s.textures_changed[1]||s.samplers_changed[0]||s.samplers_changed[1];
}
BnwOwnedGlRetirementNode::RestoreStatus
BnwOwnedGlRetirementNode::restore_font_pipeline_if_current_without_switch()noexcept{
 RestoreStatus status;status.cleanup_failure=sticky_cleanup_failure_without_callbacks();
 if(kind_!=Kind::font_pipeline_restore||creator_!=std::this_thread::get_id())return status;
 if(font_pipeline_restore_active_){
  quarantine_font_pipeline_without_callbacks(Failure::font_pipeline_reentry);
  status.cleanup_failure=true;return status;
 }
 if(quarantined_||font_pipeline_.uncertain)return status;
 font_pipeline_restore_active_=true;
 struct ActiveReset final {bool& active;~ActiveReset(){active=false;}} active{font_pipeline_restore_active_};
 auto& s=font_pipeline_;
 // Validate every armed field BEFORE any native restoration effect.
 if((s.blend_factors_changed&&!s.blend_factors_captured)||
    (s.blend_equations_changed&&!s.blend_equations_captured)||
    (s.color_mask_changed&&!s.color_mask_captured)||
    (s.blend_enabled_changed&&!s.blend_enabled_captured)||
    (s.depth_enabled_changed&&!s.depth_enabled_captured)||
    (s.depth_mask_changed&&!s.depth_mask_captured)||
    (s.depth_func_changed&&!s.depth_func_captured)||
    (s.cull_enabled_changed&&!s.cull_enabled_captured)||
    (s.stencil_enabled_changed&&!s.stencil_enabled_captured)||
    (s.polygon_mode_changed&&!s.polygon_mode_captured)||
    (s.program_changed&&!s.program_captured)||
    (s.vao_changed&&!s.vao_captured)||
    (s.active_texture_changed&&!s.active_texture_captured)||
    (s.textures_changed[0]&&!s.textures_captured[0])||(s.textures_changed[1]&&!s.textures_captured[1])||
    (s.samplers_changed[0]&&!s.samplers_captured[0])||(s.samplers_changed[1]&&!s.samplers_captured[1])||
    ((s.textures_changed[0]||s.textures_changed[1])&&!s.active_texture_captured)||
    (s.polygon_mode_changed&&s.polygon_mode[0]!=s.polygon_mode[1])){
  quarantine_font_pipeline_without_callbacks(Failure::font_pipeline_capture_invalid);
  status.cleanup_failure=true;return status;
 }
 s.restored=false;
 // Internal typed lexical effects only: no caller callback, allocation or Source.
 const auto effect=[&](bool& changed,const auto& invoke)noexcept{
  if(!changed)return true;
  const auto* api=clean_font_pipeline_dispatch_without_switch();
  if(!api)return false;
  s.uncertain=true;quarantined_=true; // Arm BEFORE native effect.
  invoke();
  if(!font_pipeline_post_without_switch(api))return false;
  changed=false;s.uncertain=false;quarantined_=false;return true;
 };
 // Reverse agreed producer order: bindings, polygon/enables/depth, blend0.
 for(unsigned u=2;u!=0;){
  const unsigned unit=--u;const auto saved=static_cast<GLuint>(s.samplers[unit]);
  if(!effect(s.samplers_changed[unit],[&,unit,saved]()noexcept{dispatch_.pipeline_sampler(unit,saved);})){ 
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 for(unsigned u=2;u!=0;){
  const unsigned unit=--u;
  if(!s.textures_changed[unit])continue;
  const auto saved=static_cast<GLuint>(s.textures[unit]);
  const auto selector=static_cast<GLenum>(GL_TEXTURE0+unit);
  // Cursor custody is armed BEFORE the first switch and kept even if there are
  // no remaining texture flags. No BindTextureUnit(unit,0) all-target clear.
  s.active_texture_changed=true;
  bool cursor_effect=true;
  if(!effect(cursor_effect,[&,selector]()noexcept{dispatch_.pipeline_active_texture(selector);})||
     !effect(s.textures_changed[unit],[&,saved]()noexcept{dispatch_.pipeline_texture(GL_TEXTURE_2D,saved);})){ 
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=static_cast<GLenum>(s.active_texture);
  if(!effect(s.active_texture_changed,[&,saved]()noexcept{dispatch_.pipeline_active_texture(saved);})){ 
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=static_cast<GLuint>(s.vao);
  if(!effect(s.vao_changed,[&,saved]()noexcept{dispatch_.pipeline_vao(saved);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=static_cast<GLuint>(s.program);
  if(!effect(s.program_changed,[&,saved]()noexcept{dispatch_.pipeline_program(saved);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.polygon_mode;
  if(!effect(s.polygon_mode_changed,[&,saved]()noexcept{dispatch_.pipeline_polygon_mode(GL_FRONT_AND_BACK,static_cast<GLenum>(saved[0]));})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.stencil_enabled;
  if(!effect(s.stencil_enabled_changed,[&,saved]()noexcept{if(saved)dispatch_.pipeline_enable(GL_STENCIL_TEST);else dispatch_.pipeline_disable(GL_STENCIL_TEST);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.cull_enabled;
  if(!effect(s.cull_enabled_changed,[&,saved]()noexcept{if(saved)dispatch_.pipeline_enable(GL_CULL_FACE);else dispatch_.pipeline_disable(GL_CULL_FACE);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.depth_enabled;
  if(!effect(s.depth_enabled_changed,[&,saved]()noexcept{if(saved)dispatch_.pipeline_enable(GL_DEPTH_TEST);else dispatch_.pipeline_disable(GL_DEPTH_TEST);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.depth_mask;
  if(!effect(s.depth_mask_changed,[&,saved]()noexcept{dispatch_.pipeline_depth_mask(saved);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=static_cast<GLenum>(s.depth_func);
  if(!effect(s.depth_func_changed,[&,saved]()noexcept{dispatch_.pipeline_depth_func(saved);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.blend_enabled;
  if(!effect(s.blend_enabled_changed,[&,saved]()noexcept{if(saved)dispatch_.pipeline_enablei(GL_BLEND,0);else dispatch_.pipeline_disablei(GL_BLEND,0);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.color_mask;
  if(!effect(s.color_mask_changed,[&,saved]()noexcept{dispatch_.pipeline_color_mask(0,saved[0],saved[1],saved[2],saved[3]);})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.blend_equations;
  if(!effect(s.blend_equations_changed,[&,saved]()noexcept{dispatch_.pipeline_blend_equations(0,static_cast<GLenum>(saved[0]),static_cast<GLenum>(saved[1]));})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 {
  const auto saved=s.blend_factors;
  if(!effect(s.blend_factors_changed,[&,saved]()noexcept{dispatch_.pipeline_blend_factors(0,static_cast<GLenum>(saved[0]),static_cast<GLenum>(saved[1]),static_cast<GLenum>(saved[2]),static_cast<GLenum>(saved[3]));})){
   status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
  }
 }
 // Explicit completion requires SAME creator/current, also for zero changes.
 // Unchanged nodes on a foreign/current mismatch remain pending with Pin custody.
 const auto* final_api=clean_font_pipeline_dispatch_without_switch();
 if(!final_api||!font_pipeline_post_without_switch(final_api)){
  status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
 }
 s.restored=true;status.complete=true;
 status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
}

void BnwOwnedGlRetirementNode::enqueue_owned(BnwOwnedGlRetirementNode* node)noexcept{
 // Ownership has already left the unique owner. Queue has no allocating wrapper
 // and no static destructor that could release a final Pin on another thread.
 node->queue_custody_=true;
 auto* head=pending_.load(std::memory_order_relaxed);
 do{node->next_=head;}while(!pending_.compare_exchange_weak(head,node,
  std::memory_order_release,std::memory_order_relaxed));
}
bool BnwOwnedGlRetirementNode::empty_without_callbacks()const noexcept{
 if(kind_==Kind::font_pipeline_restore)return font_pipeline_.restored&&!font_pipeline_has_changes_without_callbacks()&&
  !font_pipeline_.uncertain&&!font_pipeline_restore_active_;
 if(kind_==Kind::pixel_unpack_restore)return unpack_.restored&&!unpack_.changed&&!unpack_.buffer_changed&&!unpack_.uncertain;
 if(kind_==Kind::target_rows_restore)return target_rows_.restored&&!target_rows_.depth_changed&&
  !target_rows_.viewport_changed&&!target_rows_.clip_changed&&!target_rows_.uncertain&&!target_rows_restore_active_;
 return !pair_.textures[0]&&!pair_.textures[1]&&!shader_.stages[0]&&!shader_.stages[1]&&
  !shader_.program&&!shader_.vao&&!shader_.vertices&&!shader_.indices&&!shader_.sampler;
}
bool BnwOwnedGlRetirementNode::sticky_cleanup_failure_without_callbacks()noexcept{
 return sticky_cleanup_failure_.load(std::memory_order_acquire);
}
void BnwOwnedGlRetirementNode::quarantine_without_callbacks(Failure failure)noexcept{
 failure_=failure;quarantined_=true;
 // Conservative process-wide sticky failure. No consumed native error can be
 // hidden by a later void Window hook, or by a different creator's drain.
 sticky_cleanup_failure_.store(true,std::memory_order_release);
}
const GlApi* BnwOwnedGlRetirementNode::clean_current_dispatch_without_switch()noexcept{
 const auto* api=BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_);
 if(!api)return nullptr;
 // Exact loaded texture dispatch only; shader dispatch is captured from the
 // genuine producer's already-loaded GlRenderApi, never resolved during drain.
 if(api->GetError!=dispatch_.error||api->GetGraphicsResetStatus!=dispatch_.reset||
    (kind_==Kind::texture_pair&&api->DeleteTextures!=dispatch_.textures)){
  quarantine_without_callbacks(Failure::dispatch_mismatch);return nullptr;
 }
 const auto reset=dispatch_.reset();
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_without_callbacks(Failure::pre_observation_lost);return nullptr;
 }
 if(reset!=GL_NO_ERROR){quarantine_without_callbacks(Failure::preexisting_reset);return nullptr;}
 const auto error=dispatch_.error();
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_without_callbacks(Failure::pre_observation_lost);return nullptr;
 }
 if(error!=GL_NO_ERROR){quarantine_without_callbacks(Failure::preexisting_error);return nullptr;}
 return api;
}
bool BnwOwnedGlRetirementNode::delete_one_without_switch(GLuint& name,unsigned operation)noexcept{
 if(!name)return true;
 const auto* api=clean_current_dispatch_without_switch();
 if(!api)return false;
 const GLuint issued=name;
 // Quarantine is armed BEFORE an effect. The name stays intact until a verified
 // POST. A failed POST never retries a potentially reused GL name namespace.
 deletion_in_flight_=true;quarantined_=true;
 switch(operation){
 case 0:dispatch_.textures(1,&issued);break;
 case 1:dispatch_.stage(issued);break;
 case 2:dispatch_.program(issued);break;
 case 3:dispatch_.vao(1,&issued);break;
 case 4:dispatch_.buffers(1,&issued);break;
 case 5:dispatch_.sampler(1,&issued);break;
 default:std::terminate();
 }
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_without_callbacks(Failure::post_observation_lost);return false;
 }
 const auto reset=dispatch_.reset();
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_without_callbacks(Failure::post_observation_lost);return false;
 }
 if(reset!=GL_NO_ERROR){quarantine_without_callbacks(Failure::post_reset);return false;}
 const auto error=dispatch_.error();
 if(BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_)!=api){
  quarantine_without_callbacks(Failure::post_observation_lost);return false;
 }
 if(error!=GL_NO_ERROR){quarantine_without_callbacks(Failure::post_error);return false;}
 name=0;deletion_in_flight_=false;quarantined_=false;return true;
}
bool BnwOwnedGlRetirementNode::release_resources_without_switch()noexcept{
 if(creator_!=std::this_thread::get_id()||quarantined_||deletion_in_flight_||target_rows_restore_active_||
    font_pipeline_restore_active_)return false;
 if(kind_==Kind::font_pipeline_restore)return restore_font_pipeline_if_current_without_switch().complete;
 if(kind_==Kind::pixel_unpack_restore)return restore_pixel_unpack_if_current_without_switch().complete;
 if(kind_==Kind::target_rows_restore)return restore_target_rows_if_current_without_switch().complete;
 if(empty_without_callbacks())return true;
 // Current mismatch is pending, with no effect and no permanent quarantine.
 if(!BnwRendererContextPin::CleanupAccess::api_if_current_without_switch(*pin_))return false;
 if(kind_==Kind::texture_pair){
  for(auto& texture:pair_.textures)if(!delete_one_without_switch(texture,0))return false;
 }else{
  if(!delete_one_without_switch(shader_.sampler,5)||
     !delete_one_without_switch(shader_.indices,4)||
     !delete_one_without_switch(shader_.vertices,4)||
     !delete_one_without_switch(shader_.vao,3)||
     !delete_one_without_switch(shader_.program,2))return false;
  for(auto& stage:shader_.stages)if(!delete_one_without_switch(stage,1))return false;
 }
 return empty_without_callbacks();
}
BnwOwnedGlRetirementNode::DrainStatus BnwOwnedGlRetirementNode::drain_current_creator_owned()noexcept{
 DrainStatus status;
 if(draining_||drain_gate_.test_and_set(std::memory_order_acquire)){
  status.incomplete=true;status.cleanup_failure=sticky_cleanup_failure_without_callbacks();return status;
 }
 draining_=true;
 // Detach once: a pending/quarantined node is not retried in this drain.
 // The process-wide gate prevents a concurrent creator drain from hiding a
 // quarantined node in a detached list while this caller checks sticky status.
 // Only a node's creator performs effects or drops its final SAME Pin.
 auto* node=pending_.exchange(nullptr,std::memory_order_acquire);
 while(node){
  auto* next=node->next_;node->next_=nullptr;
  if(node->creator_==std::this_thread::get_id()&&node->release_resources_without_switch()){
   node->queue_custody_=false;delete node;
  }else{
   if(node->creator_==std::this_thread::get_id())status.pending_creator_nodes=true;
   enqueue_owned(node);
  }
  node=next;
 }
 draining_=false;
 status.cleanup_failure=sticky_cleanup_failure_without_callbacks();
 drain_gate_.clear(std::memory_order_release);
 return status;
}
BnwOwnedGlRetirementNode::ResourceImageChannel::Owner
BnwOwnedGlRetirementNode::ResourceImageChannel::CreationAccess::make_image_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,const GlApi& api){
 return BnwOwnedGlRetirementNode::texture_pair_owned(std::move(pin),api);
}
GLuint&BnwOwnedGlRetirementNode::ResourceImageChannel::CreationAccess::
 image_name_storage_without_callbacks(BnwOwnedGlRetirementNode& node)noexcept{
 if(node.kind_!=Kind::texture_pair||node.queue_custody_||node.pair_.textures[1])std::terminate();
 return node.pair_.textures[0];
}
BnwOwnedGlRetirementNode::ResourceImageChannel::Owner
BnwOwnedGlRetirementNode::ResourceImageChannel::CreationAccess::make_unpack_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,const GlApi& api){
 return BnwOwnedGlRetirementNode::make_pixel_unpack_restore_owned(std::move(pin),api);
}
BnwOwnedGlRetirementNode::ResourceImageChannel::UnpackStorage&
BnwOwnedGlRetirementNode::ResourceImageChannel::CreationAccess::
 unpack_storage_without_callbacks(BnwOwnedGlRetirementNode& node)noexcept{
 return node.pixel_unpack_storage_without_callbacks();
}
BnwOwnedGlRetirementNode::ResourceImageChannel::RestoreStatus
BnwOwnedGlRetirementNode::ResourceImageChannel::CreationAccess::
 restore_unpack_without_switch(BnwOwnedGlRetirementNode& node)noexcept{
 return node.restore_pixel_unpack_if_current_without_switch();
}
void BnwOwnedGlRetirementNode::ResourceImageChannel::CreationAccess::
 quarantine_image_without_callbacks(BnwOwnedGlRetirementNode& node)noexcept{
 if(node.kind_!=Kind::texture_pair||node.queue_custody_)std::terminate();
 node.quarantine_without_callbacks(Failure::post_error);
}
void BnwOwnedGlRetirementNode::ResourceImageChannel::ReadAccess::
 require_physical_without_callbacks(const BnwOwnedGlRetirementNode& node){
 if(node.creator_!=std::this_thread::get_id()||node.kind_!=Kind::texture_pair||
    !node.pin_||node.quarantined_||node.queue_custody_||node.deletion_in_flight_||
    node.failure_!=Failure::none||node.pair_.textures[1])
  throw std::runtime_error("Resource image original Node retired/foreign/uncertain");
 node.pin_->first_without_callbacks();
}
void BnwOwnedGlRetirementNode::ResourceImageChannel::ReadAccess::
 require_same_pin_without_callbacks(const BnwOwnedGlRetirementNode& node,
 std::shared_ptr<const BnwRendererContextPin> offered){
 require_physical_without_callbacks(node);
 if(!offered||offered.get()!=node.pin_.get()||offered.owner_before(node.pin_)||node.pin_.owner_before(offered))
  throw std::runtime_error("Resource image Node offered Pin shared control differs");
 require_physical_without_callbacks(node);
}
GLuint BnwOwnedGlRetirementNode::ResourceImageChannel::ReadAccess::
 image_name_without_callbacks(const BnwOwnedGlRetirementNode& node){
 require_physical_without_callbacks(node);
 const auto name=node.pair_.textures[0];
 if(!name)throw std::runtime_error("Resource image has no generated texture name");
 require_physical_without_callbacks(node);return name;
}

BnwOwnedGlRetirementNode::VertexDeclarationChannel::Owner
BnwOwnedGlRetirementNode::VertexDeclarationChannel::CreationAccess::make_owned(
 std::shared_ptr<const BnwRendererContextPin> pin,const GlApi& api,const GlRenderApi& render){
 if(!pin)throw std::runtime_error("Declaration retirement requires original Pin");
 pin->first_without_callbacks();
 // Only the VAO deletion slot is present; every other owned name remains zero.
 const DeleteDispatch dispatch{api.GetError,api.GetGraphicsResetStatus,nullptr,nullptr,nullptr,render.DeleteVertexArrays};
 if(!dispatch.error||!dispatch.reset||!dispatch.vao)
  throw std::runtime_error("Declaration actual deletion dispatch absent");
 Owner node(new BnwOwnedGlRetirementNode(pin,Kind::shader_resources,dispatch));
 pin->first_without_callbacks();return node;
}
GLuint&BnwOwnedGlRetirementNode::VertexDeclarationChannel::CreationAccess::name_storage_without_callbacks(
 BnwOwnedGlRetirementNode& n)noexcept{
 if(n.kind_!=Kind::shader_resources||n.queue_custody_||n.shader_.stages[0]||n.shader_.stages[1]||
    n.shader_.program||n.shader_.vertices||n.shader_.indices||n.shader_.sampler)std::terminate();
 return n.shader_.vao;
}
void BnwOwnedGlRetirementNode::VertexDeclarationChannel::CreationAccess::quarantine_without_callbacks(
 BnwOwnedGlRetirementNode& n)noexcept{
 if(n.kind_!=Kind::shader_resources||n.queue_custody_)std::terminate();
 n.quarantine_without_callbacks(Failure::post_error);
}
void BnwOwnedGlRetirementNode::VertexDeclarationChannel::ReadAccess::require_same_pin_without_callbacks(
 const BnwOwnedGlRetirementNode& n,std::shared_ptr<const BnwRendererContextPin> pin){
 if(n.creator_!=std::this_thread::get_id()||n.kind_!=Kind::shader_resources||!n.pin_||n.queue_custody_||
    n.quarantined_||n.deletion_in_flight_||n.failure_!=Failure::none||!pin||
    pin.get()!=n.pin_.get()||pin.owner_before(n.pin_)||n.pin_.owner_before(pin)||
    n.shader_.stages[0]||n.shader_.stages[1]||n.shader_.program||n.shader_.vertices||n.shader_.indices||n.shader_.sampler)
  throw std::runtime_error("Declaration original VAO/Pin retired or foreign");
 pin->first_without_callbacks();
}
GLuint BnwOwnedGlRetirementNode::VertexDeclarationChannel::ReadAccess::name_without_callbacks(
 const BnwOwnedGlRetirementNode& n){
 require_same_pin_without_callbacks(n,n.pin_);const auto name=n.shader_.vao;
 if(!name)throw std::runtime_error("Declaration has no actual device VAO");
 require_same_pin_without_callbacks(n,n.pin_);return name;
}

}
