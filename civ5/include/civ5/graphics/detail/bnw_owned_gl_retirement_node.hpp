#pragma once
#include "civ5/graphics/detail/bnw_renderer_context_pin.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <utility>
namespace civ5::ui::detail {class BnwOriginalVertexDeclarationDeviceResultLoan;}
namespace civ5::graphics::detail {
class BnwNormalResourceGpuImageLoan;
// Closed physical custody only. This node cannot issue Source or Paint authority.
class BnwOwnedGlRetirementNode final {
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class ::civ5::graphics::GlBnwFontShader;
 friend class ::civ5::graphics::BnwOwnedCameraRendererWindowLease;
 enum class Kind {texture_pair,shader_resources,pixel_unpack_restore,target_rows_restore,font_pipeline_restore};
 enum class Failure {none,dispatch_mismatch,pre_observation_lost,preexisting_reset,
  preexisting_error,post_observation_lost,post_reset,post_error,unpack_capture_invalid,
  target_rows_capture_invalid,target_rows_reentry,font_pipeline_capture_invalid,font_pipeline_reentry};
 struct DrainStatus final {
  bool incomplete=false,cleanup_failure=false,pending_creator_nodes=false;
 };
 struct TexturePairStorage final {std::array<GLuint,2> textures{};};
 struct ShaderResourcesStorage final {
  std::array<GLuint,2> stages{};
  GLuint program=0,vao=0,vertices=0,indices=0,sampler=0;
 };
 struct PixelUnpackStorage final {
  std::array<GLint,12> saved{};
  GLint buffer=0;
  std::size_t captured=0,changed=0;
  bool buffer_captured=false,buffer_changed=false,restored=false,uncertain=false;
 };
 struct RestoreStatus final {bool complete=false,cleanup_failure=false;};
 struct TargetRowsStorage final {
  std::array<GLfloat,4> viewport{};std::array<GLdouble,2> depth{};
  GLint clip_origin=0,clip_depth_mode=0;
  bool viewport_captured=false,depth_captured=false,clip_captured=false;
  bool viewport_changed=false,depth_changed=false,clip_changed=false;
  bool restored=false,uncertain=false;
 };
 struct FontPipelineStorage final {
  // Captured actual GL state only. No native policy or caller defaults.
  std::array<GLint,4> blend_factors{}; // srcRGB,dstRGB,srcAlpha,dstAlpha
  std::array<GLint,2> blend_equations{}; // RGB,alpha
  std::array<GLboolean,4> color_mask{};
  GLboolean blend_enabled=GL_FALSE,depth_enabled=GL_FALSE,depth_mask=GL_FALSE;
  GLboolean cull_enabled=GL_FALSE,stencil_enabled=GL_FALSE;
  GLint depth_func=0,program=0,vao=0,active_texture=0;
  std::array<GLint,2> polygon_mode{},textures{},samplers{};
  bool blend_factors_captured=false,blend_equations_captured=false,color_mask_captured=false;
  bool blend_enabled_captured=false,depth_enabled_captured=false,depth_mask_captured=false;
  bool depth_func_captured=false,cull_enabled_captured=false,stencil_enabled_captured=false;
  bool polygon_mode_captured=false,program_captured=false,vao_captured=false,active_texture_captured=false;
  std::array<bool,2> textures_captured{},samplers_captured{};
  bool blend_factors_changed=false,blend_equations_changed=false,color_mask_changed=false;
  bool blend_enabled_changed=false,depth_enabled_changed=false,depth_mask_changed=false;
  bool depth_func_changed=false,cull_enabled_changed=false,stencil_enabled_changed=false;
  bool polygon_mode_changed=false,program_changed=false,vao_changed=false,active_texture_changed=false;
  std::array<bool,2> textures_changed{},samplers_changed{};
  bool restored=false,uncertain=false;
 };
 inline static constexpr std::array<GLenum,12> pixel_unpack_fields_{
  GL_UNPACK_ALIGNMENT,GL_UNPACK_ROW_LENGTH,GL_UNPACK_IMAGE_HEIGHT,
  GL_UNPACK_SKIP_PIXELS,GL_UNPACK_SKIP_ROWS,GL_UNPACK_SKIP_IMAGES,
  GL_UNPACK_SWAP_BYTES,GL_UNPACK_LSB_FIRST,GL_UNPACK_COMPRESSED_BLOCK_WIDTH,
  GL_UNPACK_COMPRESSED_BLOCK_HEIGHT,GL_UNPACK_COMPRESSED_BLOCK_DEPTH,GL_UNPACK_COMPRESSED_BLOCK_SIZE};
 struct DeleteDispatch final {
  PFNGLGETERRORPROC error=nullptr;
  PFNGLGETGRAPHICSRESETSTATUSPROC reset=nullptr;
  PFNGLDELETETEXTURESPROC textures=nullptr;
  PFNGLDELETESHADERPROC stage=nullptr;
  PFNGLDELETEPROGRAMPROC program=nullptr;
  PFNGLDELETEVERTEXARRAYSPROC vao=nullptr;
  PFNGLDELETEBUFFERSPROC buffers=nullptr;
  PFNGLDELETESAMPLERSPROC sampler=nullptr;
  PFNGLPIXELSTOREIPROC unpack_store=nullptr;
  PFNGLBINDBUFFERPROC unpack_bind=nullptr;
  PFNGLDEPTHRANGEINDEXEDPROC target_depth=nullptr;
  PFNGLVIEWPORTINDEXEDFPROC target_viewport=nullptr;
  PFNGLCLIPCONTROLPROC target_clip=nullptr;
  PFNGLENABLEIPROC pipeline_enablei=nullptr;
  PFNGLDISABLEIPROC pipeline_disablei=nullptr;
  PFNGLBLENDFUNCSEPARATEIPROC pipeline_blend_factors=nullptr;
  PFNGLBLENDEQUATIONSEPARATEIPROC pipeline_blend_equations=nullptr;
  PFNGLCOLORMASKIPROC pipeline_color_mask=nullptr;
  PFNGLENABLEPROC pipeline_enable=nullptr;
  PFNGLDISABLEPROC pipeline_disable=nullptr;
  PFNGLDEPTHMASKPROC pipeline_depth_mask=nullptr;
  PFNGLDEPTHFUNCPROC pipeline_depth_func=nullptr;
  PFNGLPOLYGONMODEPROC pipeline_polygon_mode=nullptr;
  PFNGLUSEPROGRAMPROC pipeline_program=nullptr;
  PFNGLBINDVERTEXARRAYPROC pipeline_vao=nullptr;
  PFNGLBINDSAMPLERPROC pipeline_sampler=nullptr;
  PFNGLACTIVETEXTUREPROC pipeline_active_texture=nullptr;
  PFNGLBINDTEXTUREPROC pipeline_texture=nullptr;
 };
 struct RetireDeleter final {
  void operator()(BnwOwnedGlRetirementNode*)const noexcept;
 };
 using Owner=std::unique_ptr<BnwOwnedGlRetirementNode,RetireDeleter>;
 // New channel entry points are accessible ONLY to real Atlas nested members.
 // Existing Shader friendship may name this type, but cannot call its private
 // members. This is no claim about hostile rewrites of trusted outer friends.
 class PixelUnpackChannel final {
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  PixelUnpackChannel()=delete;
  static Owner pixel_unpack_restore_owned(std::shared_ptr<const BnwRendererContextPin>,const GlApi&);
  static PixelUnpackStorage& storage(BnwOwnedGlRetirementNode&)noexcept;
  static RestoreStatus restore(BnwOwnedGlRetirementNode&)noexcept;
 };

 public:
 // Visible type names only; ONLY real Ui members may invoke this channel.
 // Resource image path: split creation/restore access from physical reads.
 // No blanket Node, Font, Atlas or cleanup friendship is added.
 class ResourceImageChannel final {
  ResourceImageChannel()=delete;
 public:
  using Owner=BnwOwnedGlRetirementNode::Owner;
  using UnpackStorage=BnwOwnedGlRetirementNode::PixelUnpackStorage;
  using RestoreStatus=BnwOwnedGlRetirementNode::RestoreStatus;
  class CreationAccess final {
   friend class ::civ5::ui::UiScreenGlRenderer;
   CreationAccess()=delete;
   static Owner make_image_owned(std::shared_ptr<const BnwRendererContextPin>,const GlApi&);
   static GLuint& image_name_storage_without_callbacks(BnwOwnedGlRetirementNode&)noexcept;
   static Owner make_unpack_owned(std::shared_ptr<const BnwRendererContextPin>,const GlApi&);
   static UnpackStorage& unpack_storage_without_callbacks(BnwOwnedGlRetirementNode&)noexcept;
   static RestoreStatus restore_unpack_without_switch(BnwOwnedGlRetirementNode&)noexcept;
   static void quarantine_image_without_callbacks(BnwOwnedGlRetirementNode&)noexcept;
  };
  class ReadAccess final {
   friend class ::civ5::ui::UiScreenGlRenderer;
   friend class BnwNormalResourceGpuImageLoan;
   ReadAccess()=delete;
   static void require_physical_without_callbacks(const BnwOwnedGlRetirementNode&);
   static void require_same_pin_without_callbacks(const BnwOwnedGlRetirementNode&,
     std::shared_ptr<const BnwRendererContextPin>);
   static GLuint image_name_without_callbacks(const BnwOwnedGlRetirementNode&);
  };
 };
 //Narrow existing-resource channel; no new kind, cleanup dispatch or grant.
 class ClearHighlightChannel final {
  friend class ::civ5::ui::UiScreenGlRenderer;
  ClearHighlightChannel()=delete;
 public:
  using Owner=BnwOwnedGlRetirementNode::Owner;
 private:
  static Owner make_owned(std::shared_ptr<const BnwRendererContextPin> pin,const GlApi& api,const GlRenderApi& gl){
   return BnwOwnedGlRetirementNode::shader_resources_owned(std::move(pin),api,gl);
  }
  static ShaderResourcesStorage&storage_without_callbacks(BnwOwnedGlRetirementNode& node)noexcept{
   return node.shader_resources_storage_without_callbacks();
  }
 };
 class VertexDeclarationChannel final {
  VertexDeclarationChannel()=delete;
 public:
  using Owner=BnwOwnedGlRetirementNode::Owner;
  class CreationAccess final {
   friend class ::civ5::ui::UiScreenGlRenderer;
   CreationAccess()=delete;
   static Owner make_owned(std::shared_ptr<const BnwRendererContextPin>,const GlApi&,const GlRenderApi&);
   static GLuint& name_storage_without_callbacks(BnwOwnedGlRetirementNode&)noexcept;
   static void quarantine_without_callbacks(BnwOwnedGlRetirementNode&)noexcept;
  };
  class ReadAccess final {
   friend class ::civ5::ui::UiScreenGlRenderer;
   friend class ::civ5::ui::detail::BnwOriginalVertexDeclarationDeviceResultLoan;
   ReadAccess()=delete;
   static void require_same_pin_without_callbacks(const BnwOwnedGlRetirementNode&,
     std::shared_ptr<const BnwRendererContextPin>);
   static GLuint name_without_callbacks(const BnwOwnedGlRetirementNode&);
  };
 };
 class TargetRowsChannel final {
  friend class ::civ5::ui::UiScreenGlRenderer;
  TargetRowsChannel()=delete;
 public:
  using Owner=BnwOwnedGlRetirementNode::Owner;
  using Storage=BnwOwnedGlRetirementNode::TargetRowsStorage;
  using RestoreStatus=BnwOwnedGlRetirementNode::RestoreStatus;
  using DrainStatus=BnwOwnedGlRetirementNode::DrainStatus;
 private:
  static Owner make(std::shared_ptr<const BnwRendererContextPin>,GlApi,GlRenderApi);
  static Storage& storage(BnwOwnedGlRetirementNode&)noexcept;
  static RestoreStatus restore(BnwOwnedGlRetirementNode&)noexcept;
  static DrainStatus drain()noexcept;
 };
 // Visible type names only; ONLY real Shader members invoke this channel.
 class FontPipelineChannel final {
  friend class ::civ5::graphics::GlBnwFontShader;
  FontPipelineChannel()=delete;
 public:
  using Owner=BnwOwnedGlRetirementNode::Owner;
  using Storage=BnwOwnedGlRetirementNode::FontPipelineStorage;
  using RestoreStatus=BnwOwnedGlRetirementNode::RestoreStatus;
  using DrainStatus=BnwOwnedGlRetirementNode::DrainStatus;
 private:
  static Owner make(std::shared_ptr<const BnwRendererContextPin>,GlApi,GlRenderApi);
  static Storage& storage(BnwOwnedGlRetirementNode&)noexcept;
  static RestoreStatus restore(BnwOwnedGlRetirementNode&)noexcept;
  static DrainStatus drain()noexcept;
 };
 private:
 const std::shared_ptr<const BnwRendererContextPin> pin_;
 const std::thread::id creator_;
 const Kind kind_;
 const DeleteDispatch dispatch_;
 TexturePairStorage pair_;
 ShaderResourcesStorage shader_;
 PixelUnpackStorage unpack_;
 TargetRowsStorage target_rows_;
 bool target_rows_restore_active_=false;
 FontPipelineStorage font_pipeline_;
 bool font_pipeline_restore_active_=false;
 BnwOwnedGlRetirementNode* next_=nullptr;
 bool quarantined_=false;
 bool queue_custody_=false;
 bool deletion_in_flight_=false;
 Failure failure_=Failure::none;
 inline static std::atomic<BnwOwnedGlRetirementNode*> pending_{nullptr};
 inline static std::atomic_flag drain_gate_=ATOMIC_FLAG_INIT;
 inline static std::atomic<bool> sticky_cleanup_failure_{false};
 inline static thread_local bool draining_=false;
 BnwOwnedGlRetirementNode(std::shared_ptr<const BnwRendererContextPin>,Kind,DeleteDispatch)noexcept;
 ~BnwOwnedGlRetirementNode()noexcept;
 BnwOwnedGlRetirementNode(const BnwOwnedGlRetirementNode&)=delete;
 static Owner texture_pair_owned(std::shared_ptr<const BnwRendererContextPin>,const GlApi&);
 static Owner shader_resources_owned(std::shared_ptr<const BnwRendererContextPin>,const GlApi&,const GlRenderApi&);
 // Producers pass these preallocated actual fields directly to GL creation.
 // No fresh allocation or facade access is needed to receive generated names.
 TexturePairStorage& texture_pair_storage_without_callbacks()noexcept;
 ShaderResourcesStorage& shader_resources_storage_without_callbacks()noexcept;
 static Owner make_pixel_unpack_restore_owned(std::shared_ptr<const BnwRendererContextPin>,const GlApi&);
 PixelUnpackStorage& pixel_unpack_storage_without_callbacks()noexcept;
 RestoreStatus restore_pixel_unpack_if_current_without_switch()noexcept;
 const GlApi* clean_unpack_dispatch_without_switch()noexcept;
 bool unpack_post_without_switch(const GlApi*)noexcept;
 void quarantine_unpack_without_callbacks(Failure)noexcept;
 static Owner make_target_rows_restore_owned(std::shared_ptr<const BnwRendererContextPin>,GlApi,GlRenderApi);
 TargetRowsStorage& target_rows_storage_without_callbacks()noexcept;
 RestoreStatus restore_target_rows_if_current_without_switch()noexcept;
 const GlApi* target_rows_physical_without_switch()noexcept;
 const GlApi* clean_target_rows_dispatch_without_switch()noexcept;
 bool target_rows_post_without_switch(const GlApi*)noexcept;
 void quarantine_target_rows_without_callbacks(Failure)noexcept;
 static Owner make_font_pipeline_restore_owned(std::shared_ptr<const BnwRendererContextPin>,GlApi,GlRenderApi);
 FontPipelineStorage& font_pipeline_storage_without_callbacks()noexcept;
 RestoreStatus restore_font_pipeline_if_current_without_switch()noexcept;
 const GlApi* font_pipeline_physical_without_switch()noexcept;
 const GlApi* clean_font_pipeline_dispatch_without_switch()noexcept;
 bool font_pipeline_post_without_switch(const GlApi*)noexcept;
 void quarantine_font_pipeline_without_callbacks(Failure)noexcept;
 bool font_pipeline_has_changes_without_callbacks()const noexcept;
 static void enqueue_owned(BnwOwnedGlRetirementNode*)noexcept;
 static DrainStatus drain_current_creator_owned()noexcept;
 static bool sticky_cleanup_failure_without_callbacks()noexcept;
 void quarantine_without_callbacks(Failure)noexcept;
 const GlApi* clean_current_dispatch_without_switch()noexcept;
 bool delete_one_without_switch(GLuint&,unsigned)noexcept;
 bool release_resources_without_switch()noexcept;
 bool empty_without_callbacks()const noexcept;
};
static_assert(std::atomic<BnwOwnedGlRetirementNode*>::is_always_lock_free,
 "Owned GL retirement requires allocation-free pointer enqueue");
}
