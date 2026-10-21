#include "ui_screen_gl.hpp"
#include "civ5/graphics/detail/bnw_label_default_font_target_loan.hpp"
#include "civ5/graphics/detail/bnw_owned_gl_retirement_node.hpp"
#include "civ5/graphics/detail/bnw_normal_resource_gpu_image_loan.hpp"
#include "civ5/ui/detail/bnw_original_nonfont_device_initializer.hpp"
#include "civ5/ui/detail/bnw_original_vertex_declaration_device_result_loan.hpp"
#include "civ5/ui/detail/bnw_original_vertex_declaration_setup_loan.hpp"
#include "civ5/graphics/detail/bnw_original_vertex_declaration_gl_schema.hpp"
#include <vector>
#include <utility>
#include "civ5/ui/detail/bnw_original_generated_resource_image_loan.hpp"
#include "civ5/graphics/detail/bnw_renderer_context_pin.hpp"
#include <array>
#include <thread>
#include "civ5/ui/detail/bnw_label_actual_paint_call_frame.hpp"
#include "civ5/ui/detail/bnw_normal_label_paint_invocation.hpp"
#include "civ5/ui/detail/bnw_normal_label_material_loan.hpp"
#include "civ5/ui/detail/bnw_normal_root_batch_history.hpp"
#include "civ5/ui/detail/bnw_normal_root_batch_execution.hpp"
#include "civ5/ui/detail/bnw_normal_nonfont_pipeline.hpp"
#include "civ5/ui/detail/bnw_normal_root_context_entry.hpp"
#include "civ5/graphics/detail/bnw_normal_label_draw_record_binding.hpp"
#include "civ5/ui/detail/bnw_windowed_renderer_matrix_data_loan.hpp"
#include "civ5/ui/detail/bnw_standalone_positioned_label_run_loan.hpp"
#include "civ5/ui/detail/bnw_positioned_label_run_loan.hpp"
#include "civ5/ui/detail/bnw_normal_caption_fp32_rect_loan.hpp"
#include "civ5/ui/detail/bnw_label_paint_text_loan.hpp"
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
#include "civ5/ui/detail/bnw_label_system_font_borrower.hpp"
#include "civ5/runtime/bnw_worldview_native_context_issuer.hpp"
#include "civ5/ui/detail/notification_cpu_geometry.hpp"
#include "civ5/graphics/gl_water_world.hpp"
#include "civ5/ui/bnw_worldview_control_effects.hpp"
#include "civ5/ui/bnw_notification_terrain_scene_loan.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/graphics/bnw_unit_visual_owner.hpp"
#include "civ5/terrain/terrain_world.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include <exception>
#include <cstdio>
#include <cerrno>
#include <map>
#include <set>
#include <chrono>
#include "civ5/ui/ui_texture_asset.hpp"
#include "civ5/ui/ui_quad_clip.hpp"
#include "civ5/ui/anchored_text_measurement.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include "civ5/assets/ui_texture_binding.hpp"
#include "civ5/graphics/detail/notification_raw_ui_texture_source.hpp"
#include "civ5/ui/detail/notification_layout_texture_keeper.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include <stdexcept>
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <bit>
#include <limits>
#include <cfenv>
#ifdef _WIN32
#include <windows.h>
#include "civ5/ui/bnw_tech_award_window_pointer_loan.hpp"
#endif

#include "civ5/graphics/detail/bnw_font_texture_observation.hpp"
#include "civ5/ui/detail/bnw_font_texture_request.hpp"
#include "civ5/runtime/detail/bnw_font_resource_loan.hpp"
#include "bnw_font_texture_observation_storage.inc"

namespace civ5::ui {
namespace {
// Invoked ONLY from an active catch, before cleanup or sticky poison writes.
// This observes the exception already propagating; it issues no Source query.
void note_active_worldview_exception(const char* boundary) noexcept {
 const int saved_errno=errno;
#ifdef _WIN32
 const DWORD saved_last_error=GetLastError();
#endif
 try{throw;}
 catch(const std::exception& error){
  std::fprintf(stderr,"[CIVIS_WORLDVIEW_EXCEPTION] boundary=%s exception=%.2048s\n",boundary,error.what());
 }
 catch(...){
  std::fprintf(stderr,"[CIVIS_WORLDVIEW_EXCEPTION] boundary=%s exception=non_std_exception\n",boundary);
 }
 std::fflush(stderr);
#ifdef _WIN32
 SetLastError(saved_last_error);
#endif
 errno=saved_errno;
}
}
#include "bnw_font_texture_cache_storage.inc"
#include "bnw_label_system_font_atlas_storage.inc"
}
#include "bnw_label_system_font_atlas_metadata_core.inc"
namespace civ5::ui {

// Physical storage owners only; no ID registration, sampler or Draw grant.
// Reverse destruction releases GPU, CPU, then the SAME Init occurrence.
struct UiScreenGlRenderer::OriginalResource0Storage final {
 const std::shared_ptr<const detail::BnwOriginalNonFontDeviceInitializer> initializer;
 const std::shared_ptr<const detail::BnwOriginalGeneratedResourceImageLoan> cpu;
 const std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan> gpu;
 OriginalResource0Storage(
  std::shared_ptr<const detail::BnwOriginalNonFontDeviceInitializer> init,
  std::shared_ptr<const detail::BnwOriginalGeneratedResourceImageLoan> image,
  std::shared_ptr<const graphics::detail::BnwNormalResourceGpuImageLoan> storage)
  :initializer(std::move(init)),cpu(std::move(image)),gpu(std::move(storage)){}
};

struct UiScreenGlRenderer::WorldViewPaintScope {
 const std::shared_ptr<WorldViewPaintLifetime> life;
 const BnwWorldViewRendererProjectionLoan& projection;
 const runtime::BnwWorldViewSourceOperationLoan& operation;
 const BnwOwnedCameraGuiScreen::Loan& gui;
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window;
 const std::uint64_t serial;
 const std::array<std::shared_ptr<BnwWorldViewControlTree>,5> trees;
 std::array<std::uint64_t,5> revisions{};
 const BnwWorldViewControlRenderPublication* publication=nullptr;
 const ComposedUiFrame* frame=nullptr;
 const LabelPaintItemScope* label_item=nullptr;
 // SAME original factory objects live inside actual Gui/window lender and
 // WorldView scope. Future genuine manager drain must run before it closes.
 std::vector<std::shared_ptr<const BnwWorldViewControlRenderPublication>> original_publications;
 // ONE original Root/Prefix/Matrix execution for the enclosing traversal.
 // Reverse destruction retires CPU envelopes before the original Prefix,
 // all inside the original actual Gui/window lender and WorldView scope.
 std::shared_ptr<const detail::BnwNormalRootPaintCallFrame>
  capture_actual_normal_root_frame_owned(UiScreenGlRenderer&)const;
 std::shared_ptr<const detail::BnwNormalRootPaintCallFrame>
  capture_actual_normal_root_frame_owned(UiScreenGlRenderer&,
   const detail::BnwNormalRootContextEntry&)const;
 detail::BnwNormalNonFontOwnReturn prepare_current_normal_button_own118_owned(
  UiScreenGlRenderer&,const detail::BnwNormalNonFontPaintInvocation&,
  std::shared_ptr<const detail::BnwNormalRootPaintCallFrame>)const;
 struct NormalLabelCpuCapture;
 struct NormalCompletedRootCapture;
 // Declared before ACTIVE execution/CPU: all originals retire before archives.
 mutable std::vector<std::shared_ptr<NormalCompletedRootCapture>> normal_completed_roots;
 mutable std::shared_ptr<BnwWorldViewControlTree> normal_root_tree;
 // Context originals retire after Execution and before original Publication.
 mutable std::shared_ptr<const detail::BnwNormalRootPaintCallFrame> normal_context_root_frame;
 struct NormalCommonOriginalInput final {
  const BnwWorldViewControlTree::Control control;
  const ComposedUiFrame frame;
  const std::shared_ptr<const detail::BnwNormalLabelPaintEmission> normal_label_emission;
  const std::uint64_t original_control;
 };
 mutable std::shared_ptr<const BnwWorldViewControlTree::NormalCommonChildConsumers> normal_common_consumers;
 struct NormalCommonButtonCapture;
 mutable std::shared_ptr<NormalCommonButtonCapture> normal_common_button;
 // CURRENT typed caption routing only; the CPU envelope is retained separately
 // by the genuine Root-selected item after its authentic Material is returned.
 mutable std::shared_ptr<NormalButtonCaptionCapture> normal_button_caption;
 UiGlPaintReport enter_current_normal_common_button_owned(UiScreenGlRenderer&,
  std::shared_ptr<NormalCommonButtonCapture>,std::shared_ptr<const ComposedUiFrame>,
  std::int32_t,std::int32_t)const;
 std::shared_ptr<const BnwWorldViewControlTree::NormalCommonChildConsumers>
  capture_normal_common_child_consumers_owned(UiScreenGlRenderer&,
   std::shared_ptr<const std::vector<NormalCommonOriginalInput>>)const;
 UiGlPaintReport begin_current_normal_context_root_owned(UiScreenGlRenderer&,
  const detail::BnwNormalRootContextEntry&)const;
 mutable std::unique_ptr<detail::BnwNormalRootBatchExecution> normal_root_execution;
 mutable std::vector<std::shared_ptr<const NormalLabelCpuCapture>> normal_label_cpu_captures;
 // Provider only: Root's earned whole traversal chooses the invocation cut.
 std::shared_ptr<const BnwWorldViewControlTree::NormalQueueConsumer>
  capture_normal_queue_consumer_owned(UiScreenGlRenderer&)const;
 void drain_normal_queue_after_root_return_owned(UiScreenGlRenderer&)const;
 struct NormalVisibleOriginalPublication;
 struct NormalVisibleChildCapture;
 mutable std::shared_ptr<const std::vector<std::shared_ptr<const NormalVisibleOriginalPublication>>> normal_visible_inputs;
 mutable std::shared_ptr<const BnwWorldViewControlTree::NormalVisibleLabelConsumer> normal_visible_consumer;
 mutable std::shared_ptr<LabelPaintCapture> normal_bootstrap_label;
 mutable std::shared_ptr<NormalVisibleChildCapture> normal_visible_child;
 mutable std::vector<UiGlPaintReport> normal_visible_reports;
 mutable std::optional<std::array<std::uint64_t,4>> normal_queue_observations;
 mutable std::optional<detail::BnwNormalRootBootstrapSelection> normal_bootstrap_selection;
 std::shared_ptr<const BnwWorldViewControlTree::NormalVisibleLabelConsumer>
  capture_normal_visible_cpu_consumer_owned(UiScreenGlRenderer&,
   std::shared_ptr<const std::vector<std::shared_ptr<const NormalVisibleOriginalPublication>>>)const;
 std::unique_ptr<detail::BnwNormalLabelMaterialLoan> prepare_current_visible_label_cpu_owned(
  UiScreenGlRenderer&,std::size_t,std::shared_ptr<LabelPaintCapture>,
  const detail::BnwNormalRootBatchHistory::Guard&,
  std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>,
  const detail::BnwNormalVisibleLabelCall&)const;
 bool painting=false;
 bool notification_terrain_active=false;
 // Observation only: at most one genuine entered Label shape per operation.
 mutable bool label_shape_noted=false;
 // Values exist only for this paint operation, captured under the genuine
 // resource lender before immutable paint revisions and before any GL write.
 struct VisualCache {
  std::weak_ptr<const BnwWorldViewControlTree> tree;
  std::uint64_t revision=0,serial=0;
  std::map<std::uint64_t,UiFloatColor> images;
  std::map<std::uint64_t,BnwWorldViewControlTree::ButtonVisualOwned> buttons;
  std::map<std::uint64_t,BnwWorldViewControlTree::StackLayoutOwned> stacks;
 };
 const std::array<VisualCache,5>* visual_cache=nullptr;
 const VisualCache& visual_for_tree_owned(const std::shared_ptr<BnwWorldViewControlTree>& tree,
  std::uint64_t revision)const {
  pure();
  if(!visual_cache)throw std::runtime_error("WorldView resource-phase visual cache absent");
  for(std::size_t n=0;n<trees.size();++n)if(trees[n].get()==tree.get()&&
    !trees[n].owner_before(tree)&&!tree.owner_before(trees[n])){
   const auto& cached=(*visual_cache)[n];
   if(cached.serial!=serial||cached.revision!=revision||revision!=revisions[n]||
      cached.tree.expired()||cached.tree.owner_before(tree)||tree.owner_before(cached.tree))
    throw std::runtime_error("WorldView resource-phase visual tree/revision changed");
   pure();return cached;
  }
  throw std::runtime_error("WorldView resource-phase visual tree substituted");
 }
 WorldViewPaintScope(const std::shared_ptr<WorldViewPaintLifetime>& l,
  const BnwWorldViewRendererProjectionLoan& p,const runtime::BnwWorldViewSourceOperationLoan& op,
  const BnwOwnedCameraGuiScreen::Loan& g,const graphics::BnwOwnedCameraRendererWindowLease::Loan& w,
  const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& t)
  :life(l),projection(p),operation(op),gui(g),window(w),serial(op.operation_serial()),trees(t){
  origin();
  for(std::size_t n=0;n<trees.size();++n){
   if(!trees[n])throw std::runtime_error("WorldView actual native tree absent");
   revisions[n]=trees[n]->render_revision_owned();origin();
  }
  pure();
 }
 void origin()const {
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||!life->active)
   throw std::runtime_error("WorldView renderer callback retired or reentered");
  projection.require_current_without_callbacks();
  if(operation.operation_serial()!=serial)throw std::runtime_error("WorldView render operation replaced");
 }
 void pure()const {
  origin();
  for(std::size_t n=0;n<trees.size();++n)trees[n]->require_render_revision_without_callbacks(revisions[n],operation,serial);
  origin();
 }
 void full()const {
  pure();operation.require_operation(serial);pure();
  gui.require_current();pure();window.require_current();pure();
 }
};

#include "bnw_label_actual_paint_call_frame_core.inc"
UiScreenGlRenderer::WorldViewPaintScope* UiScreenGlRenderer::worldview_scope_without_callbacks()const {
 const auto life=worldview_lifetime_;
 if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||life->renderer!=this){
  life->failed=true;throw std::runtime_error("WorldView renderer source lifetime failed or foreign creator");
 }
 return const_cast<WorldViewPaintScope*>(static_cast<const WorldViewPaintScope*>(life->active_scope));
}

// CPU consumer custody only. No target/material/submit or draw authority.
struct UiScreenGlRenderer::LabelPaintCapture final {
 // CPU geometry only, captured during authentic ACTIVE item entry. It is
 // physically reusable data; a selected Root queued-entry permission is still
 // independently required before any historical target/stage/Draw operation.
 // Closed representation: integer legacy OR original FP32 loan/words.
 // Historical use compares only immutable metadata, never retired CURRENT.
 struct DestinationGeometry final {
  const std::optional<WorldViewRect> integer;
  const std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> caption;
  const std::optional<std::array<std::uint32_t,4>> words;
 private:
  friend class UiScreenGlRenderer;
  explicit DestinationGeometry(WorldViewRect actual):integer(actual),caption(),words(){}
  DestinationGeometry(std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> actual,
   std::array<std::uint32_t,4> raw):integer(),caption(std::move(actual)),words(raw){
   if(!caption)throw std::runtime_error("Caption geometry lacks original closed rect");
  }
 public:
  DestinationGeometry(const DestinationGeometry&)=default;
  void require_same(const DestinationGeometry& actual)const{
   if(bool(integer)!=bool(actual.integer)||bool(caption)!=bool(actual.caption)||words!=actual.words||
      caption.get()!=actual.caption.get()||caption.owner_before(actual.caption)||actual.caption.owner_before(caption))
    throw std::runtime_error("Default target destination representation/physical rect changed");
   if(integer&&(integer->x!=actual.integer->x||integer->y!=actual.integer->y||
      integer->width!=actual.integer->width||integer->height!=actual.integer->height))
    throw std::runtime_error("Default target legacy rectangle changed");
  }
  void require_item(const WorldViewPaint& actual)const{
   const auto& owner=actual.normal_caption_fp32_rect;
   if(caption){
    if(integer||!words||owner.get()!=caption.get()||owner.owner_before(caption)||caption.owner_before(owner))
     throw std::runtime_error("Caption destination is not SAME immutable indexed rect");
   }else if(!integer||words||owner||integer->x!=actual.destination.x||integer->y!=actual.destination.y||
      integer->width!=actual.destination.width||integer->height!=actual.destination.height)
    throw std::runtime_error("Legacy target cannot consume closed caption geometry");
  }
 };
 struct HistoricalTargetGeometry final {
  const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
  const std::shared_ptr<const ComposedUiFrame> frame_owner;
  const detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard scope_health;
  const detail::BnwLabelPaintPositionedData::Guard positioned_data;
  const std::size_t paint_index;
  const std::uint64_t control;
  const DestinationGeometry destination;
  const WorldViewRect clip;
  const std::array<GLint,4> viewport;
 private:
  friend class UiScreenGlRenderer;
  HistoricalTargetGeometry(std::shared_ptr<const BnwWorldViewControlRenderPublication> original,
   std::shared_ptr<const ComposedUiFrame> owned,
   detail::BnwLabelActualPaintCallFrame::RootScopeHealthGuard health,
   detail::BnwLabelPaintPositionedData::Guard data,std::size_t index,std::uint64_t actual_control,
   DestinationGeometry actual_destination,WorldViewRect actual_clip,std::array<GLint,4> actual_viewport)
   :publication(std::move(original)),frame_owner(std::move(owned)),scope_health(std::move(health)),
    positioned_data(std::move(data)),paint_index(index),control(actual_control),
    destination(actual_destination),clip(actual_clip),viewport(actual_viewport){}
 public:
  HistoricalTargetGeometry(const HistoricalTargetGeometry&)=default;
  HistoricalTargetGeometry&operator=(const HistoricalTargetGeometry&)=delete;
  ~HistoricalTargetGeometry()=default;
 };
 std::shared_ptr<const detail::BnwLabelActualPaintCallFrame> frame;
 std::shared_ptr<const detail::BnwPositionedLabelRunLoan> run;
 std::shared_ptr<const detail::BnwStandalonePositionedLabelRunLoan> standalone_run;
 std::unique_ptr<detail::BnwLabelPaintTextLoan> paint;
 std::unique_ptr<detail::BnwLabelPaintPositionedData> data;
 std::optional<graphics::detail::BnwLabelSystemFontAtlasMetadata::Guard> metadata;
 std::shared_ptr<graphics::GlBnwSystemFontAtlas> atlas;
 std::shared_ptr<const graphics::detail::BnwRendererContextPin> pin;
 // Destroy this snapshot before the original DATA/Paint/Run/Frame facades.
 std::optional<HistoricalTargetGeometry> historical_target_geometry;
 // Physical metadata captured under genuine CURRENT before any lease retirement.
 // Historical consumers use these words/owner plus independent Root/item/health;
 // they never call the retained rect's old CURRENT/source/raw getters.
 struct CaptionFp32RectSnapshot final {
  const std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> owner;
  const std::array<std::uint32_t,4> words;
 private:
  friend class UiScreenGlRenderer;
  CaptionFp32RectSnapshot(std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> original,
   std::array<std::uint32_t,4> actual_words):owner(std::move(original)),words(actual_words){}
 public:
  CaptionFp32RectSnapshot(const CaptionFp32RectSnapshot&)=default;
  CaptionFp32RectSnapshot&operator=(const CaptionFp32RectSnapshot&)=delete;
 };
 std::optional<CaptionFp32RectSnapshot> caption_fp32_rect;
 // Canonical association custody; physical references do not renew DATA.
 // Reverse member destruction still retires DATA before Paint/Run/Frame.
};
std::unique_ptr<UiScreenGlRenderer::LabelPaintCapture>
UiScreenGlRenderer::capture_label_actual_positioned_owned(bool normal_only){
 // Callback-free capture of every borrowed outer identity BEFORE Frame FULL.
 const auto life=worldview_lifetime_;
 const auto*const renderer=this;
 if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
    !life->active||life->renderer!=renderer||!life->active_scope)
  throw std::runtime_error("Positioned capture requires genuine entered renderer");
 const auto*const scope=static_cast<const WorldViewPaintScope*>(life->active_scope);
 const auto origin=[&]{
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
     !life->active||life->renderer!=renderer||life->active_scope!=scope)
   throw std::runtime_error("Positioned capture renderer/lexical scope retired");
 };
 origin();scope->pure();origin();
 if(!scope->painting||!scope->frame||!scope->publication)
  throw std::runtime_error("Positioned capture lacks actual painting publication");
 const auto*const composed=scope->frame;
 const auto*const publication=scope->publication;
 const auto*const operation=&scope->operation;
 const auto serial=scope->serial;
 const auto tree=publication->tree_;
 const auto control=publication->control_;
 origin();
 const auto id=publication->actual_checked_control_id_for_paint_without_callbacks();
 origin();
 const auto entered_first=[&]{
  origin();
  if(!scope->painting||scope->frame!=composed||scope->publication!=publication||
     &scope->operation!=operation||scope->serial!=serial)
   throw std::runtime_error("Positioned capture exact entered frame changed");
  scope->pure();origin();
 };
 entered_first();
 auto frame=capture_label_actual_paint_call_frame_owned();
 entered_first();
 if(!frame)throw std::runtime_error("Actual paint Frame issuer returned no facade");
 const auto frame_guard=frame->capture_guard_without_callbacks();
 std::optional<std::size_t> actual_index;
 const auto frame_first=[&]{entered_first();frame_guard.require_current_without_callbacks();
  frame_guard.require_actual_control_without_callbacks(*tree,id);
  if(actual_index)frame_guard.require_actual_paint_item_without_callbacks(*publication,*actual_index);
  entered_first();};
 frame_first();
 std::optional<detail::BnwPositionedLabelRunLoan::Guard> run_guard;
 std::optional<detail::BnwStandalonePositionedLabelRunLoan::Guard> standalone_guard;
 std::optional<detail::BnwLabelPaintTextLoan::Guard> paint_guard;
 std::optional<detail::BnwLabelPaintPositionedData::Guard> data_guard;
 std::optional<graphics::detail::BnwLabelSystemFontAtlasMetadata::Guard> metadata_guard;
 std::shared_ptr<graphics::GlBnwSystemFontAtlas> captured_atlas;
 std::shared_ptr<const graphics::detail::BnwRendererContextPin> captured_pin;
 std::unique_ptr<LabelPaintCapture> result;
 const auto first=[&]{
  frame_first();
  if(run_guard){run_guard->require_current_without_callbacks();frame_first();}
  if(standalone_guard){standalone_guard->require_current_without_callbacks();frame_first();}
  if(paint_guard){paint_guard->require_current_without_callbacks();frame_first();}
  if(metadata_guard){metadata_guard->require_current_without_callbacks();frame_first();}
  if(data_guard){data_guard->require_current_without_callbacks();frame_first();}
  if(captured_pin){captured_pin->first_without_callbacks();
   captured_atlas->require_same_owned_context_pin_no_current(captured_pin);frame_first();}
 };
 {
  // The getter is defined after complete ItemScope. Its number is paired with
  // canonical Run provenance and revalidated by Frame9/Root Normal10.
  auto normal=capture_normal_label_run_for_actual_item_owned(frame);frame_first();
  actual_index=normal.actual_index;frame_first();
  auto standalone_run=std::move(normal.run);
  // Only genuine Root Normal10 discrimination can bypass production submission.
  // This branch precedes the distinct embedded/diagnostic lane.
  if(!standalone_run&&normal_only){frame_guard.require_source_owned(*operation);frame_first();return nullptr;}
  std::shared_ptr<const detail::BnwPositionedLabelRunLoan> run;
  std::shared_ptr<const detail::BnwLabelSystemFontBorrower> system;
  if(standalone_run){
   standalone_guard.emplace(standalone_run->capture_guard_for_paint_frame_guard_owned(
    *operation,tree,id,frame_guard));first();
   standalone_guard->require_source_owned(*operation);first();
   system=standalone_guard->actual_system_font_without_callbacks();first();
   if(!system)throw std::runtime_error("Standalone positioned capture requires genuine positive170");
   standalone_guard->require_system_font_borrower_without_callbacks(*system);first();
  }else{
   // Existing embedded lane remains distinct; no standalone-to-embedded cast.
   frame_first();
   if(!control)throw std::runtime_error("Embedded positioned capture requires genuine indexed Control");
   frame_first();
   run=tree->positioned_label_run_for_paint_owned(tree,*control,*operation,*frame);
   frame_first();
   // Only Root's genuine constructor-NULL178 path may return null here.
   if(!run){frame_guard.require_source_owned(*operation);frame_first();return nullptr;}
   run_guard.emplace(run->capture_guard_for_paint_owned(*operation,tree,id,frame_guard));
   first();run_guard->require_source_owned(*operation);first();
   system=run_guard->actual_system_font_without_callbacks();first();
   if(!system)throw std::runtime_error("Positioned capture requires genuine positive170");
   run_guard->require_system_font_borrower_without_callbacks(*system);first();
  }
  const auto metadata=system->atlas_metadata_without_callbacks();first();
  if(!metadata)throw std::runtime_error("Positioned capture canonical metadata absent");
  metadata_guard.emplace(metadata->capture_guard_without_callbacks());first();
  metadata_guard->require_source_owned(*operation);first();
  const auto atlas=metadata_guard->retain_atlas_for_physical_cleanup_without_callbacks();first();
  if(!atlas)throw std::runtime_error("Positioned capture physical canonical Atlas absent");
  metadata_guard->require_actual_atlas_without_callbacks(*atlas);first();
  captured_atlas=atlas;captured_pin=metadata_guard->impl_->context_pin.lock();
  if(!captured_pin)throw std::runtime_error("Positioned capture canonical counted Pin absent");
  first();
  std::unique_ptr<detail::BnwLabelPaintTextLoan> paint;
  if(standalone_run){
   first();paint=detail::BnwLabelPaintTextLoan::begin_standalone_owned(
    *operation,tree,id,standalone_run,*publication,*actual_index,*frame);
  }else{
   paint=detail::BnwLabelPaintTextLoan::begin_owned(*operation,tree,id,*run,*frame);
  }
  first();
  if(!paint)throw std::runtime_error("Actual Paint issuer returned no lexical facade");
  paint_guard.emplace(paint->capture_guard_without_callbacks());first();
  paint_guard->require_system_font_borrower_without_callbacks(*system);first();
  paint_guard->require_actual_renderer_without_callbacks(*renderer);first();
  paint_guard->require_source_owned(*operation);first();
  // Observe immutable real Run scalars BEFORE the restricted DATA producer.
  // This is neither a guessed first Label nor an admission/readiness gate.
  first();
  if(!scope->label_shape_noted){
   const auto record=paint_guard->actual_positioned_record_without_callbacks();first();
   const auto selectors=paint_guard->actual_positioned_style_passes_without_callbacks();first();
   const auto c4_count=paint_guard->actual_c4_bytes_without_callbacks().size();first();
   // Scalars are independent of the borrowed facades before stderr effects.
   const auto saved_id=id,saved_serial=serial;
   const auto saved_count=record.count,saved_discriminator=record.source_discriminator;
   const auto saved_truncate=record.truncate;
   const auto saved_style=record.style;
   const int saved_errno=errno;
#ifdef _WIN32
   const DWORD saved_last_error=GetLastError();
#endif
   first();scope->label_shape_noted=true;first();
   std::fprintf(stderr,"[CIVIS_LABEL_ENTERED_SHAPE] operation=%llu control=%llu records=%u discriminator=%08x truncate=%d style=%u selectors=%08x,%08x,%08x c4_bytes=%llu\n",
    static_cast<unsigned long long>(saved_serial),static_cast<unsigned long long>(saved_id),
    static_cast<unsigned>(saved_count),static_cast<unsigned>(saved_discriminator),
    static_cast<int>(saved_truncate),static_cast<unsigned>(saved_style),
    static_cast<unsigned>(selectors[0]),static_cast<unsigned>(selectors[1]),static_cast<unsigned>(selectors[2]),
    static_cast<unsigned long long>(c4_count));
   first();std::fflush(stderr);first();
#ifdef _WIN32
   SetLastError(saved_last_error);
#endif
   errno=saved_errno;first();
  }
  // Root Source10 MUST be composed with GetGlyph handoff's corrected Atlas route.
  auto data=paint->positioned_data_owned(*operation,atlas);first();
  if(!data)throw std::runtime_error("Root positioned producer returned no DATA lease");
  data_guard.emplace(data->capture_guard_without_callbacks());first();
  data_guard->require_system_font_borrower_without_callbacks(*system);first();
  data_guard->require_actual_renderer_without_callbacks(*renderer);first();
  data_guard->require_source_owned(*operation);first();
  // Preallocate the only custody envelope, then POST before noexcept adoption.
  result.reset(new LabelPaintCapture);first();
  result->frame=std::move(frame);result->run=std::move(run);
  result->standalone_run=std::move(standalone_run);
  result->paint=std::move(paint);result->data=std::move(data);
  result->metadata.emplace(*metadata_guard);result->atlas=captured_atlas;result->pin=captured_pin;first();
 }
 // System/metadata/Atlas snapshots and moved facade temporaries are gone.
 // Retained independent guards cannot renew the lexical Frame/Paint/DATA.
 first();data_guard->require_source_owned(*operation);first();
 return result;
}


// No DATA/Paint/Early/Atlas/source Guard is retained by this physical cache.
struct UiScreenGlRenderer::LabelOriginalProgramCache final {
 using Shader=graphics::GlBnwFontShader;
 struct Entry final {
  std::shared_ptr<const assets::CivisResourceBankOwner> bank;
  std::shared_ptr<const graphics::detail::BnwRendererContextPin> pin;
  std::shared_ptr<Shader::OriginalProgramLibrary> library;
 };
 const std::thread::id creator=std::this_thread::get_id();
 std::vector<Entry> entries;
 bool active=false,failed=false;
};
struct UiScreenGlRenderer::LabelNativeProgramCapture final {
 std::shared_ptr<LabelPaintCapture> label;
 std::unique_ptr<graphics::GlBnwFontShader::OriginalProgramUse> programs;
 // Reverse destruction: current code use retires before DATA/Paint/Frame.
};
// CPU custody only. No facade duplication, historical consumer permission,
// manager selection, GPU staging or completed queue entry is issued here.
struct UiScreenGlRenderer::WorldViewPaintScope::NormalLabelCpuCapture final {
 const std::shared_ptr<LabelPaintCapture> original_label;
 const std::shared_ptr<LabelNativeProgramCapture> original_programs;
 const std::shared_ptr<graphics::GlBnwFontShader::PositionedVertices> original_vertices;
 const std::shared_ptr<graphics::detail::BnwNormalLabelDrawRecordBinding> original_binding;
 const detail::BnwNormalRootBatchHistory::Guard root;
 const detail::BnwNormalItemOccurrence::Guard occurrence;
 NormalLabelCpuCapture(std::shared_ptr<LabelPaintCapture> label,
  std::shared_ptr<LabelNativeProgramCapture> programs,
  std::shared_ptr<graphics::GlBnwFontShader::PositionedVertices> vertices,
  std::shared_ptr<graphics::detail::BnwNormalLabelDrawRecordBinding> binding,
  detail::BnwNormalRootBatchHistory::Guard actual_root,
  detail::BnwNormalItemOccurrence::Guard actual_occurrence)
  :original_label(std::move(label)),original_programs(std::move(programs)),
   original_vertices(std::move(vertices)),original_binding(std::move(binding)),
   root(std::move(actual_root)),occurrence(std::move(actual_occurrence)){}
};
struct UiScreenGlRenderer::WorldViewPaintScope::NormalVisibleOriginalPublication final {
 const std::shared_ptr<BnwWorldViewControlTree> tree;
 const std::uint64_t control;
 const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
 const std::shared_ptr<const detail::BnwNormalLabelPaintEmission> emission;
 NormalVisibleOriginalPublication(std::shared_ptr<BnwWorldViewControlTree> t,std::uint64_t id,
  std::shared_ptr<const BnwWorldViewControlRenderPublication> p,
  std::shared_ptr<const detail::BnwNormalLabelPaintEmission> e)
  :tree(std::move(t)),control(id),publication(std::move(p)),emission(std::move(e)){}
};
struct UiScreenGlRenderer::WorldViewPaintScope::NormalVisibleChildCapture final {
 const std::shared_ptr<const NormalVisibleOriginalPublication> original;
 const detail::BnwNormalRootBatchHistory::Guard root;
 const std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> matrix;
 const detail::BnwNormalVisibleLabelCall call;
 std::unique_ptr<detail::BnwNormalLabelMaterialLoan> material;
 NormalVisibleChildCapture(std::shared_ptr<const NormalVisibleOriginalPublication> p,
  detail::BnwNormalRootBatchHistory::Guard r,
  std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> m,
  detail::BnwNormalVisibleLabelCall c)
  :original(std::move(p)),root(std::move(r)),matrix(std::move(m)),call(std::move(c)){}
};
struct UiScreenGlRenderer::WorldViewPaintScope::NormalCompletedRootCapture final {
 const std::shared_ptr<BnwWorldViewControlTree> tree;
 const std::array<std::uint64_t,4> observations;
 std::unique_ptr<detail::BnwNormalRootBatchExecution> execution;
 NormalCompletedRootCapture(std::shared_ptr<BnwWorldViewControlTree> t,std::array<std::uint64_t,4> o)
  :tree(std::move(t)),observations(o){}
};

// Ui CPU return custody. The actual Root CURRENT callback alone installs this
// private capture after SAME Control lookup and lazy original child factory.
// It cannot mint Root/Invocation/OwnReturn or world/frame readiness.
struct UiScreenGlRenderer::WorldViewPaintScope::NormalCommonButtonCapture final {
 const detail::BnwNormalCommonChildCall common;
 const detail::BnwNormalRootBatchHistory::Guard root;
 const std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> matrix;
 const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
 std::shared_ptr<const detail::BnwNormalRootPaintCallFrame> actual_frame;
 std::optional<detail::BnwNormalNonFontOwnReturn> own_return;
 NormalCommonButtonCapture(detail::BnwNormalCommonChildCall c,detail::BnwNormalRootBatchHistory::Guard r,
  std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> m,
  std::shared_ptr<const BnwWorldViewControlRenderPublication> p)
  :common(std::move(c)),root(std::move(r)),matrix(std::move(m)),publication(std::move(p)){}
};

// UNFROZEN pending-caption custody only: no emission, item, Material or tail issuer.
// The future genuine Root caption callback owns this once before child paint.
struct UiScreenGlRenderer::NormalButtonCaptionCapture final {
 const detail::BnwNormalCommonChildCall common;
 const detail::BnwNormalButtonCaptionCall caption;
 const detail::BnwNormalRootBatchHistory::Guard root;
 const detail::BnwNormalRootContextEntry entry;
 const std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> matrix;
 const std::shared_ptr<const BnwWorldViewControlRenderPublication> parent_publication;
 const std::shared_ptr<const ComposedUiFrame> parent_frame;
 const std::shared_ptr<const detail::BnwNormalRootPaintCallFrame> parent_actual_frame;
 const std::shared_ptr<const detail::BnwPositionedLabelRunLoan> original_run;
 const std::shared_ptr<BnwWorldViewControlTree> original_tree;
 std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> fp32_rect;
 std::optional<std::array<std::uint32_t,4>> fp32_rect_words;
 std::optional<detail::BnwNormalButtonCaptionAppendProjection> projection;
 std::shared_ptr<const ComposedUiFrame> append_origin;
 std::optional<std::size_t> append_index;
 std::shared_ptr<const detail::BnwNormalButtonCaptionPaintEmission> emission;
 std::shared_ptr<LabelPaintCapture> positioned;
 std::shared_ptr<const detail::BnwLabelActualPaintCallFrame> entered_frame;
 std::shared_ptr<const detail::BnwPositionedLabelRunLoan> entered_run;
 std::shared_ptr<const WorldViewPaintScope::NormalLabelCpuCapture> original_cpu;
 std::optional<detail::BnwNormalItemOccurrence::Guard> item;
 std::shared_ptr<const BnwWorldViewControlRenderPublication> child_publication;
 std::shared_ptr<const ComposedUiFrame> child_frame;
 std::unique_ptr<detail::BnwNormalLabelMaterialLoan> material;
 // This mutable closed capture has no public issuer or completion setter.
 // child_frame/material are filled only by the future genuine entered hook.
 NormalButtonCaptionCapture(detail::BnwNormalCommonChildCall c,detail::BnwNormalButtonCaptionCall k,
  detail::BnwNormalRootBatchHistory::Guard r,detail::BnwNormalRootContextEntry e,
  std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> m,
  std::shared_ptr<const BnwWorldViewControlRenderPublication> p,
  std::shared_ptr<const ComposedUiFrame> f,std::shared_ptr<const detail::BnwNormalRootPaintCallFrame> a,
  std::shared_ptr<const detail::BnwPositionedLabelRunLoan> run,std::shared_ptr<BnwWorldViewControlTree> tree)
  :common(std::move(c)),caption(std::move(k)),root(std::move(r)),entry(std::move(e)),matrix(std::move(m)),
   parent_publication(std::move(p)),parent_frame(std::move(f)),parent_actual_frame(std::move(a)),original_run(std::move(run)),original_tree(std::move(tree)){}
};
std::shared_ptr<UiScreenGlRenderer::NormalButtonCaptionCapture>
UiScreenGlRenderer::capture_current_normal_button_caption_owned(
 const detail::BnwNormalCommonChildCall& supplied_common,const detail::BnwNormalButtonCaptionCall& supplied_caption,
 std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> supplied_matrix){
 const auto saved_life=worldview_lifetime_;const auto*const renderer=this;
 const auto common=supplied_common;const auto caption=supplied_caption;const auto matrix=std::move(supplied_matrix);
 const auto origin=[&]{
  if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
     !saved_life->active||saved_life->renderer!=renderer||!saved_life->active_scope)
   throw std::runtime_error("Caption pending Ui scope retired before borrow");
 };
 origin();const auto*const scope=static_cast<const WorldViewPaintScope*>(saved_life->active_scope);
 const auto root=caption.captured_root_without_callbacks();origin();
 const auto entry=caption.captured_root_context_entry_without_callbacks();origin();
 const auto control=caption.actual_control_without_callbacks();origin();
 const auto ordinal=caption.actual_ordered_ordinal_without_callbacks();origin();
 const auto run=caption.positioned_run_for_actual_caption_without_callbacks();origin();
 // Run is SAME strong physical completed storage; do not call its old cold first.
 if(!matrix||!run||!scope->normal_root_execution||!scope->normal_context_root_frame||
    !scope->normal_root_tree||!scope->publication||!scope->frame||!scope->painting||
    scope->label_item||scope->normal_common_button||scope->normal_visible_child)
  throw std::runtime_error("Caption needs original parent Context after own118/Common RETURN");
 const auto*const execution=scope->normal_root_execution.get();const auto*const operation=&scope->operation;
 const auto parent=scope->publication->original_owner_without_callbacks();origin();
 const auto frame=parent->actual_adjusted_frame_owner_for_paint_without_callbacks(*this,*scope->frame);origin();
 const auto parent_actual=scope->normal_context_root_frame;origin();
 const auto original_tree=scope->normal_root_tree;origin();
 const auto first=[&]{
  origin();if(saved_life->active_scope!=scope)throw std::runtime_error("Caption original Ui scope changed");
  caption.require_current_without_callbacks();common.require_current_without_callbacks();origin();scope->pure();origin();
  if(scope->normal_root_execution.get()!=execution||scope->publication!=parent.get()||
     scope->frame!=frame.get()||!scope->painting||scope->label_item||scope->normal_common_button||scope->normal_visible_child||
     common.actual_kind_without_callbacks()!=detail::BnwNormalCommonChildCall::Kind::button||
     common.actual_phase_without_callbacks()!=detail::BnwNormalCommonChildCall::Phase::button_caption178||
     common.actual_ordered_ordinal_without_callbacks()!=ordinal||!common.matches_actual_control_without_callbacks(control))
   throw std::runtime_error("Caption original selected parent/current phase changed");
  if(scope->normal_root_tree.get()!=original_tree.get()||scope->normal_root_tree.owner_before(original_tree)||
     original_tree.owner_before(scope->normal_root_tree)||scope->normal_context_root_frame.get()!=parent_actual.get()||
     scope->normal_context_root_frame.owner_before(parent_actual)||parent_actual.owner_before(scope->normal_context_root_frame))
   throw std::runtime_error("Caption SAME original Tree/parent physical Frame changed");
  caption.require_same_common_call_without_callbacks(common);origin();
  root.require_physical_current_without_callbacks();root.require_same_root_without_callbacks(common.captured_root_without_callbacks());
  root.require_same_root_without_callbacks(execution->captured_root_without_callbacks());
  root.require_actual_renderer_without_callbacks(*renderer);root.require_actual_matrix_without_callbacks(*matrix);origin();
  const auto actual_run=caption.positioned_run_for_actual_caption_without_callbacks();origin();
  if(actual_run.get()!=run.get()||actual_run.owner_before(run)||run.owner_before(actual_run))
   throw std::runtime_error("Caption SAME completed physical Run changed");
  parent->require_adjusted_frame_owner_without_callbacks(frame);origin();
 };
 const auto full=[&]{first();caption.require_source_owned(*operation);first();
  common.require_source_owned(*operation);first();root.require_source_owned(*operation);first();scope->full();first();};
 first();full();
 // Own immutable copies and original parent physical controls before allocation.
 auto result=std::make_shared<NormalButtonCaptionCapture>(common,caption,root,entry,matrix,parent,frame,parent_actual,run,original_tree);
 first();full();return result;
}

// Actual append/bind/Publication producer body. The only data frontier here is
// exact raw native geometry -> destination/clip mapping: supplied rectangles
// are private operands, not permission; no callsite is issued until it closes.
std::shared_ptr<const BnwWorldViewControlRenderPublication>
UiScreenGlRenderer::append_current_normal_button_caption_publication_owned(
 std::shared_ptr<NormalButtonCaptionCapture> supplied,
 std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> supplied_rect,WorldViewRect actual_clip){
 const auto saved_life=worldview_lifetime_;auto*const renderer=this;const auto child=std::move(supplied);
 const auto origin=[&]{if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
  !saved_life->active||saved_life->renderer!=renderer||!saved_life->active_scope)
  throw std::runtime_error("Caption append original Ui scope retired");};
 origin();const auto*const scope=static_cast<const WorldViewPaintScope*>(saved_life->active_scope);
 if(!child||!child->matrix||!child->parent_frame||!child->original_tree)
  throw std::runtime_error("Caption append lacks original owned physical inputs");
 const auto rect=std::move(supplied_rect);const auto rect_words=child->fp32_rect_words;
  if(!rect||!rect_words||!child->original_run)
   throw std::runtime_error("Caption append lacks actual original FP32 rect snapshot");
  const auto initialized_run=child->original_run;
  const auto tree=child->original_tree;const auto*const execution=scope->normal_root_execution.get();
 const auto*const operation=&scope->operation;
 const auto first=[&]{
  origin();if(saved_life->active_scope!=scope)throw std::runtime_error("Caption append lexical Ui scope changed");
  child->caption.require_current_without_callbacks();child->common.require_current_without_callbacks();
  child->caption.require_same_common_call_without_callbacks(child->common);origin();scope->pure();origin();
  rect->require_current_without_callbacks();rect->require_same_caption_without_callbacks(child->caption);
  rect->require_same_initialized_run_without_callbacks(initialized_run);
  if(rect->raw_words_without_callbacks()!=*rect_words||child->fp32_rect.get()!=rect.get()||
     child->fp32_rect.owner_before(rect)||rect.owner_before(child->fp32_rect)||child->fp32_rect_words!=rect_words)
   throw std::runtime_error("Caption append SAME original FP32 rect changed");origin();
  if(!execution||scope->normal_root_execution.get()!=execution||scope->normal_root_tree.get()!=tree.get()||
     scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree)||
     scope->normal_context_root_frame.get()!=child->parent_actual_frame.get()||
     scope->normal_context_root_frame.owner_before(child->parent_actual_frame)||child->parent_actual_frame.owner_before(scope->normal_context_root_frame)||
     scope->frame!=child->parent_frame.get()||scope->publication!=child->parent_publication.get()||!scope->painting||
     scope->label_item||scope->normal_button_caption||scope->normal_common_button||scope->normal_visible_child)
   throw std::runtime_error("Caption append SAME original parent/current physical associations changed");
  child->root.require_physical_current_without_callbacks();
  child->root.require_same_root_without_callbacks(execution->captured_root_without_callbacks());
  child->root.require_actual_renderer_without_callbacks(*renderer);child->root.require_actual_matrix_without_callbacks(*child->matrix);origin();
 };
 const auto full=[&]{first();child->caption.require_source_owned(*operation);first();
  child->common.require_source_owned(*operation);first();rect->require_source_owned(*operation);first();
  child->root.require_source_owned(*operation);first();scope->full();first();};
 first();if(child->append_origin||child->append_index||child->emission||child->child_publication)
  throw std::runtime_error("Caption actual append/bind cannot replay");
 if(!child->projection||!child->projection->font_owner)
  throw std::runtime_error("Caption append requires genuine already-cached original projection");
 // Own SAME cached projection VALUE/strong font before any Source cut.
 const auto projection=*child->projection;first();
 // Copy only unentered original Ui carrier bookkeeping. Authority is issued
 // from the postappend immutable owner below, never from this value copy.
 auto emitted=*child->parent_frame;first();
 try{
  full();
  if(!projection.font_owner||projection.numeric_controls!=child->root.numeric_controls_without_callbacks()||
     !child->projection||child->projection->font_owner.get()!=projection.font_owner.get()||
     child->projection->font_owner.owner_before(projection.font_owner)||projection.font_owner.owner_before(child->projection->font_owner)||
     child->projection->numeric_controls!=projection.numeric_controls)
   throw std::runtime_error("Caption SAME cached font/numeric projection changed");
  first();
  WorldViewPaint paint;paint.kind=WorldViewPaintKind::text;
  paint.text=projection.text;first();paint.style=projection.style_name;first();
  UiFontStylePaint styled;styled.font=projection.font_name;first();styled.style=projection.style_name;first();
  for(std::size_t p=0;p<styled.colors.size();++p)styled.colors[p]=projection.packed_colors_148_14c_150[p];
  paint.rgba=projection.packed_colors_148_14c_150[0];paint.font_style.emplace(std::move(styled));
  paint.normal_caption_fp32_rect=rect;paint.clip.emplace(actual_clip);first();
  const auto index=emitted.paints.size();
  if(index==emitted.paints.max_size())throw std::runtime_error("Caption actual append carrier exhausted");
  full();emitted.paints.push_back(std::move(paint));first();full();
  if(emitted.paints.size()!=index+1)throw std::runtime_error("Caption actual append count changed");
  full();const auto owner=std::make_shared<const ComposedUiFrame>(std::move(emitted));first();full();
  child->append_origin=owner;child->append_index.emplace(index);first();
  const detail::BnwNormalButtonCaptionCall::EmissionIssuerKey key(owner,index);first();
  full();const auto emission=child->caption.bind_emitted_index_owned(key,*operation);first();full();
  if(!emission)throw std::runtime_error("Caption actual append omitted genuine closed emission");
  child->emission=emission;first();
  const auto control=child->caption.actual_control_without_callbacks();first();
  const auto entry=child->caption.captured_root_context_entry_without_callbacks();first();
  std::vector<std::size_t> indices;full();indices.push_back(index);first();full();
  const BnwWorldViewControlRenderPublication::OriginalOwnerIssuerKey publication_key;
  full();const auto publication=BnwWorldViewControlRenderPublication::capture_context_child_original_owned(
   publication_key,*renderer,std::weak_ptr<const void>(saved_life),tree,control,entry,
   scope->gui,scope->window,*owner,std::move(indices),emission);first();full();
  emission->require_publication_owned(*publication,*operation);first();full();
  const auto canonical=publication->original_owner_before_paint_without_callbacks();first();
  if(canonical.get()!=publication.get()||canonical.owner_before(publication)||publication.owner_before(canonical))
   throw std::runtime_error("Caption actual append Publication original control changed");
  return publication;
 }catch(...){saved_life->failed=true;throw;}
}

// UNFROZEN: exact existing Publication.paint routing and physical restoration.
// The genuine Root caption callback may call this ONLY after typed caption
// projection/append/emission/Publication suppliers have been issued. There is
// no generic callback argument or path for creating a Material from a report.
std::unique_ptr<detail::BnwNormalLabelMaterialLoan>
UiScreenGlRenderer::paint_current_normal_button_caption_owned(
 std::shared_ptr<NormalButtonCaptionCapture> supplied,
 std::shared_ptr<const BnwWorldViewControlRenderPublication> supplied_publication){
 const auto saved_life=worldview_lifetime_;auto*const renderer=this;
 const auto child=std::move(supplied);const auto publication=std::move(supplied_publication);
 const auto origin=[&]{if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||
  saved_life->failed||!saved_life->active||saved_life->renderer!=renderer||!saved_life->active_scope)
  throw std::runtime_error("Caption child paint original Ui lifetime retired");};
 origin();auto*const scope=const_cast<WorldViewPaintScope*>(static_cast<const WorldViewPaintScope*>(saved_life->active_scope));
 if(!child||!publication)throw std::runtime_error("Caption child paint needs genuine closed original custody");
 const auto tree=child->original_tree;const auto*const execution=scope->normal_root_execution.get();
 const auto*const operation=&scope->operation;
 const auto first=[&]{
  origin();if(saved_life->active_scope!=scope)throw std::runtime_error("Caption child original lexical scope changed");
  child->caption.require_current_without_callbacks();child->common.require_current_without_callbacks();
  child->caption.require_same_common_call_without_callbacks(child->common);origin();scope->pure();origin();
  if(!tree||!execution||scope->normal_root_execution.get()!=execution||scope->normal_root_tree.get()!=tree.get()||
     scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree)||
     scope->normal_context_root_frame.get()!=child->parent_actual_frame.get()||
     scope->normal_context_root_frame.owner_before(child->parent_actual_frame)||child->parent_actual_frame.owner_before(scope->normal_context_root_frame)||
     child->common.actual_kind_without_callbacks()!=detail::BnwNormalCommonChildCall::Kind::button||
     child->common.actual_phase_without_callbacks()!=detail::BnwNormalCommonChildCall::Phase::button_caption178)
   throw std::runtime_error("Caption child SAME Root/physical parent changed");
  child->root.require_physical_current_without_callbacks();
  child->root.require_same_root_without_callbacks(execution->captured_root_without_callbacks());
  child->root.require_actual_renderer_without_callbacks(*renderer);child->root.require_actual_matrix_without_callbacks(*child->matrix);origin();
 };
 const auto full=[&]{first();child->caption.require_source_owned(*operation);first();
  child->common.require_source_owned(*operation);first();child->root.require_source_owned(*operation);first();scope->full();first();};
 first();
 if(child->child_publication||child->child_frame||child->material||scope->normal_button_caption||scope->normal_common_button||
    scope->normal_visible_child||scope->label_item||scope->publication!=child->parent_publication.get()||
    scope->frame!=child->parent_frame.get()||!scope->painting)
  throw std::runtime_error("Caption original child replay or parent tuple mismatch");
 const auto canonical=publication->original_owner_before_paint_without_callbacks();first();
 if(canonical.get()!=publication.get()||canonical.owner_before(publication)||publication.owner_before(canonical)||
    publication.get()==child->parent_publication.get())
  throw std::runtime_error("Caption child is not distinct SAME canonical original Publication");
 if(!child->emission||!child->append_origin||!child->append_index||!child->projection)
  throw std::runtime_error("Caption paint lacks authentic original append/emission/projection");
 first();child->emission->require_publication_owned(*publication,*operation);first();full();
 struct RestoreCaptionParent final {
  const std::shared_ptr<WorldViewPaintLifetime> life;WorldViewPaintScope* scope;
  const std::shared_ptr<NormalButtonCaptionCapture> capture;
  ~RestoreCaptionParent()noexcept{if(life->active_scope==scope){
   scope->painting=true;scope->frame=capture->parent_frame.get();scope->publication=capture->parent_publication.get();
   scope->normal_button_caption.reset();}}
 } restore{saved_life,scope,child};
 child->child_publication=publication;
 scope->painting=false;scope->frame=nullptr;scope->publication=publication.get();scope->normal_button_caption=child;first();
 try{
  const auto child_cpu_report=publication->paint(*renderer);first();
  (void)child_cpu_report; // Never a whole Root/frame or queue Draw observation.
  if(scope->painting||scope->frame||scope->label_item||scope->normal_button_caption.get()!=child.get()||
     scope->normal_button_caption.owner_before(child)||child.owner_before(scope->normal_button_caption)||!child->material)
   throw std::runtime_error("Caption actual paint omitted SAME genuine unsealed Material");
  // No old ACTIVE child Frame/Run/Item FIRST and no public Material FIRST
  // before Root's whole Stage returns and seals the original queue records.
  return std::move(child->material);
 }catch(...){saved_life->failed=true;throw;}
}

std::shared_ptr<const BnwWorldViewControlTree::NormalCommonChildConsumers>
UiScreenGlRenderer::WorldViewPaintScope::capture_normal_common_child_consumers_owned(
 UiScreenGlRenderer& supplied,std::shared_ptr<const std::vector<NormalCommonOriginalInput>> supplied_inputs)const{
 const auto saved_life=life;const auto*const scope=this;auto*const renderer=&supplied;
 const auto inputs=std::move(supplied_inputs);
 const auto origin=[&]{
  if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
     !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
   throw std::runtime_error("Common callback original Ui scope retired");
 };
 origin();scope->pure();origin();
 if(!inputs)throw std::runtime_error("Common callback original input custody absent");
 const auto*const operation=&scope->operation;
 // Immutable original compositor inputs and own Ui life precede Source/alloc.
 origin();scope->full();origin();
 BnwWorldViewControlTree::NormalCommonLabelConsumer label=
  [saved_life,scope,renderer,operation,inputs](
   const runtime::BnwWorldViewSourceOperationLoan& op,
   const detail::BnwNormalRootBatchHistory::Guard& supplied_root,
   std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> supplied_matrix,
   const detail::BnwNormalCommonChildCall& supplied_common,
   const detail::BnwNormalVisibleLabelCall& supplied_visible)
   ->std::unique_ptr<detail::BnwNormalLabelMaterialLoan>{
   const auto common=supplied_common;const auto visible=supplied_visible;
   const auto root=supplied_root;const auto matrix=std::move(supplied_matrix);
   const auto origin=[&]{
    if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
       !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
     throw std::runtime_error("Common selected Label original Ui scope retired");
   };
   origin();common.require_current_without_callbacks();origin();scope->pure();origin();
   if(&op!=operation||&scope->operation!=operation||!matrix||!scope->normal_root_execution||
      !scope->normal_root_tree||!scope->normal_context_root_frame||!scope->painting||!scope->frame||
      !scope->publication||scope->label_item||scope->normal_visible_child||scope->normal_bootstrap_label)
    throw std::runtime_error("Common Label lacks SAME entered parent Context/Root");
   const auto tree=scope->normal_root_tree;const auto*const execution=scope->normal_root_execution.get();
   const auto entry=common.captured_root_context_entry_without_callbacks();
   const auto control=common.actual_control_without_callbacks();
   const auto common_root=common.captured_root_without_callbacks();
   const auto ordinal=common.actual_ordered_ordinal_without_callbacks();
   const auto depth=common.actual_common_depth_without_callbacks();
   const auto parent_publication=scope->publication->original_owner_without_callbacks();origin();
   const auto parent_frame=parent_publication->actual_adjusted_frame_owner_for_paint_without_callbacks(*renderer,*scope->frame);
   const auto parent_actual_frame=scope->normal_context_root_frame;origin();
   const auto first=[&]{
    origin();common.require_current_without_callbacks();origin();scope->pure();origin();
    root.require_physical_current_without_callbacks();root.require_same_root_without_callbacks(common_root);
    root.require_same_root_without_callbacks(execution->captured_root_without_callbacks());
    root.require_actual_renderer_without_callbacks(*renderer);root.require_actual_matrix_without_callbacks(*matrix);
    visible.require_current_without_callbacks();visible.require_same_root_without_callbacks(root);
    if(scope->normal_root_execution.get()!=execution||scope->normal_root_tree.get()!=tree.get()||
       scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree)||
       common.actual_kind_without_callbacks()!=detail::BnwNormalCommonChildCall::Kind::standalone_label||
       common.actual_phase_without_callbacks()!=detail::BnwNormalCommonChildCall::Phase::child18||
       common.actual_ordered_ordinal_without_callbacks()!=ordinal||common.actual_common_depth_without_callbacks()!=depth)
     throw std::runtime_error("Common selected Label original Root/current call changed");
    origin();
   };
   const auto full=[&]{first();common.require_source_owned(*operation);first();
    visible.require_source_owned(*operation);first();root.require_source_owned(*operation);first();};
   try{
    first();const auto actual_id=visible.actual_control_without_callbacks();first();
    const NormalCommonOriginalInput* selected=nullptr;
    for(const auto& input:*inputs){first();if(input.original_control==actual_id){
     if(selected)throw std::runtime_error("Common Label original compositor input duplicated");selected=&input;}}
    if(!selected||selected->normal_label_emission||selected->frame.paints.size()!=1)
     throw std::runtime_error("Common Label lacks one original unissued compositor append");
    // The selected storage is in immutable owned inputs, not a borrowed loop.
    // Copy the same original frame BEFORE the first callbackful dispatch.
    auto emitted=selected->frame;first();
    full();const auto dispatch=tree->dispatch_normal_gui_label_paint_owned(tree,control,*operation,entry);first();full();
    if(dispatch.outcome!=BnwWorldViewControlTree::NormalGuiLabelDispatchOwned::Outcome::standalone_label||!dispatch.invocation)
     throw std::runtime_error("Common selected Label did not earn actual standalone invocation");
    const detail::BnwNormalLabelPaintInvocation::EmissionIssuerKey emission_key(emitted,0);
    full();const auto emission=dispatch.invocation->bind_emitted_index_owned(emission_key,*operation);first();full();
    if(!emission)throw std::runtime_error("Common Label original append omitted genuine emission");
    std::vector<std::size_t> indices{0};first();
    const auto count=scope->original_publications.size();
    if(count==scope->original_publications.max_size())throw std::runtime_error("Common child Publication custody exhausted");
    full();const_cast<WorldViewPaintScope*>(scope)->original_publications.reserve(count+1);first();full();
    const BnwWorldViewControlRenderPublication::OriginalOwnerIssuerKey publication_key;
    const auto publication=BnwWorldViewControlRenderPublication::capture_context_child_original_owned(
     publication_key,*renderer,std::weak_ptr<const void>(saved_life),tree,control,entry,
     scope->gui,scope->window,std::move(emitted),std::move(indices),emission);
    first();full();
    const auto canonical=publication->original_owner_before_paint_without_callbacks();first();
    if(canonical.get()!=publication.get()||canonical.owner_before(publication)||publication.owner_before(canonical)||
       publication->actual_checked_control_id_for_paint_without_callbacks()!=actual_id||scope->original_publications.size()!=count)
     throw std::runtime_error("Common Label SAME Control/canonical child Publication changed");
    visible.require_actual_publication_without_callbacks(*publication);first();
    const_cast<WorldViewPaintScope*>(scope)->original_publications.push_back(publication);first();
    full();auto original=std::make_shared<const NormalVisibleOriginalPublication>(tree,actual_id,publication,emission);first();full();
    std::vector<std::shared_ptr<const NormalVisibleOriginalPublication>> owned{original};first();
    full();const auto child_inputs=std::make_shared<const std::vector<std::shared_ptr<const NormalVisibleOriginalPublication>>>(std::move(owned));first();full();
    const auto previous_inputs=scope->normal_visible_inputs;const auto previous_consumer=scope->normal_visible_consumer;
    const auto previous_frame=scope->frame;const auto previous_pub=scope->publication;const auto previous_painting=scope->painting;
    if(previous_frame!=parent_frame.get()||previous_pub!=parent_publication.get()||!previous_painting)
     throw std::runtime_error("Common child paint changed SAME original parent tuple");
    // ORIGINAL parent physical owners are captured before clearing raw scope.
    // Root health delegates to canonical Entry, never parent ACTIVE Frame.
    auto* entered=const_cast<WorldViewPaintScope*>(scope);
    struct RestoreParent final {
     const std::shared_ptr<WorldViewPaintLifetime> life;WorldViewPaintScope* scope;
     const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
     const std::shared_ptr<const ComposedUiFrame> frame;
     const std::shared_ptr<const detail::BnwNormalRootPaintCallFrame> actual;
     const std::shared_ptr<const std::vector<std::shared_ptr<const NormalVisibleOriginalPublication>>> inputs;
     const std::shared_ptr<const BnwWorldViewControlTree::NormalVisibleLabelConsumer> consumer;
     const bool painting;
     ~RestoreParent()noexcept{if(life->active_scope==scope){scope->frame=frame.get();scope->publication=publication.get();
      scope->painting=painting;scope->normal_visible_inputs=inputs;scope->normal_visible_consumer=consumer;}}
    } restore{saved_life,entered,parent_publication,parent_frame,parent_actual_frame,previous_inputs,previous_consumer,previous_painting};
    entered->frame=nullptr;entered->publication=parent_publication.get();entered->painting=false;
    entered->normal_visible_inputs=child_inputs;first();
    auto consumer=scope->capture_normal_visible_cpu_consumer_owned(*renderer,child_inputs);first();full();
    entered->normal_visible_consumer=consumer;first();
    auto material=(*consumer)(*operation,root,matrix,visible);first();
    if(!material)throw std::runtime_error("Common Label omitted genuine unique unsealed Material");
    // No public Material FIRST/scalars before owner whole Stage RETURN.
    return material;
   }catch(...){saved_life->failed=true;throw;}
  };
 BnwWorldViewControlTree::NormalCommonButtonOwnConsumer button=
  [saved_life,scope,renderer,operation,inputs](
   const runtime::BnwWorldViewSourceOperationLoan& op,const detail::BnwNormalRootBatchHistory::Guard& supplied_root,
   std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> supplied_matrix,
   const detail::BnwNormalCommonChildCall& supplied_common)->detail::BnwNormalNonFontOwnReturn{
   const auto common=supplied_common;const auto root=supplied_root;const auto matrix=std::move(supplied_matrix);
   const auto origin=[&]{
    if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
       !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
     throw std::runtime_error("Common Button original Ui scope retired");
   };
   origin();common.require_current_without_callbacks();origin();scope->pure();origin();
   if(&op!=operation||&scope->operation!=operation||!inputs||!matrix||!scope->normal_root_execution||
      !scope->normal_root_tree||!scope->normal_context_root_frame||!scope->painting||!scope->frame||
      !scope->publication||scope->label_item||scope->normal_common_button||scope->normal_visible_child)
    throw std::runtime_error("Common Button lacks SAME genuine entered parent Root");
   const auto tree=scope->normal_root_tree;const auto*const execution=scope->normal_root_execution.get();
   const auto entry=common.captured_root_context_entry_without_callbacks();
   const auto control=common.actual_control_without_callbacks();
   const auto ordinal=common.actual_ordered_ordinal_without_callbacks();
   const auto depth=common.actual_common_depth_without_callbacks();
   common.require_current_without_callbacks();
   if(common.actual_kind_without_callbacks()!=detail::BnwNormalCommonChildCall::Kind::button||
      common.actual_phase_without_callbacks()!=detail::BnwNormalCommonChildCall::Phase::button_own118)
    throw std::runtime_error("Common nonnull body callback is not actual Button own118");
   // Exact frozen Common4 classifier: unknown/unwritten resource throws;
   // authentic NULL is routed by Root before this callback and must never
   // enter Ui paint, Frame, CurrentReceiver, Invocation or own118 effects.
   if(!common.actual_button_texture_is_nonnull_without_callbacks())
    throw std::runtime_error("Authentic NULL Button cannot enter nonnull Ui own118 callback");
   common.require_current_without_callbacks();origin();
   const auto parent_publication=scope->publication->original_owner_without_callbacks();origin();
   const auto parent_frame=parent_publication->actual_adjusted_frame_owner_for_paint_without_callbacks(*renderer,*scope->frame);origin();
   const auto parent_actual=scope->normal_context_root_frame;
   const auto first=[&]{
    origin();common.require_current_without_callbacks();origin();scope->pure();origin();
    if(&scope->operation!=operation||scope->normal_root_execution.get()!=execution||
       scope->normal_root_tree.get()!=tree.get()||scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree)||
       common.actual_kind_without_callbacks()!=detail::BnwNormalCommonChildCall::Kind::button||
       common.actual_phase_without_callbacks()!=detail::BnwNormalCommonChildCall::Phase::button_own118||
       common.actual_ordered_ordinal_without_callbacks()!=ordinal||common.actual_common_depth_without_callbacks()!=depth)
     throw std::runtime_error("Common Button selected Root/current phase changed");
    root.require_physical_current_without_callbacks();root.require_same_root_without_callbacks(common.captured_root_without_callbacks());
    root.require_same_root_without_callbacks(execution->captured_root_without_callbacks());
    root.require_actual_renderer_without_callbacks(*renderer);root.require_actual_matrix_without_callbacks(*matrix);origin();
   };
   const auto full=[&]{first();common.require_source_owned(*operation);first();
    root.require_source_owned(*operation);first();scope->full();first();};
   try{
    first();
    // No NodePaint lookup, synthetic indexed span or per-child frame copy.
    // The actual selected CommonCall supplies canonical Control/Entry; Root25
    // child body publication lends the SAME original parent adjusted object.
    if(!common.actual_button_texture_is_nonnull_without_callbacks())
     throw std::runtime_error("Common Button became authentic NULL before child body birth");
    first();
    const auto count=scope->original_publications.size();
    if(count==scope->original_publications.max_size())throw std::runtime_error("Common Button original Publication custody exhausted");
    full();const_cast<WorldViewPaintScope*>(scope)->original_publications.reserve(count+1);first();full();
    const BnwWorldViewControlRenderPublication::OriginalOwnerIssuerKey key;
    const auto publication=BnwWorldViewControlRenderPublication::capture_context_child_body_original_owned(
     key,*renderer,std::weak_ptr<const void>(saved_life),tree,common,control,entry,
     scope->gui,scope->window,parent_frame);first();full();
    const auto canonical=publication->original_owner_before_paint_without_callbacks();first();
    if(canonical.get()!=publication.get()||canonical.owner_before(publication)||publication.owner_before(canonical)||
       scope->original_publications.size()!=count)
     throw std::runtime_error("Common Button canonical original Control/Publication changed");
    if(!common.actual_button_texture_is_nonnull_without_callbacks())
     throw std::runtime_error("Common Button nonnull state changed at child Publication POST");
    first();
    const_cast<WorldViewPaintScope*>(scope)->original_publications.push_back(publication);first();
    full();auto child=std::make_shared<NormalCommonButtonCapture>(common,root,matrix,publication);first();full();
    const auto previous_button=scope->normal_common_button;
    const auto previous_frame=scope->frame;const auto previous_pub=scope->publication;const auto previous_painting=scope->painting;
    if(previous_button||previous_frame!=parent_frame.get()||previous_pub!=parent_publication.get()||!previous_painting)
     throw std::runtime_error("Common Button parent tuple changed before actual child paint");
    auto* entered=const_cast<WorldViewPaintScope*>(scope);
    struct RestoreButtonParent final {
     const std::shared_ptr<WorldViewPaintLifetime> life;WorldViewPaintScope* scope;
     const std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
     const std::shared_ptr<const ComposedUiFrame> frame;
     const std::shared_ptr<const detail::BnwNormalRootPaintCallFrame> actual;
     const std::shared_ptr<NormalCommonButtonCapture> previous;
     const bool painting;
     ~RestoreButtonParent()noexcept{if(life->active_scope==scope){scope->frame=frame.get();scope->publication=publication.get();
      scope->painting=painting;scope->normal_common_button=previous;}}
    } restore{saved_life,entered,parent_publication,parent_frame,parent_actual,previous_button,previous_painting};
    entered->frame=nullptr;entered->painting=false;entered->publication=publication.get();entered->normal_common_button=child;first();
    // Ui dispatcher intercepts this genuine paint BEFORE generic GL setup.
    // Canonical Invocation is issued inside it only AFTER actual GenericFrame.
    const auto child_cpu_report=publication->paint(*renderer);first();
    (void)child_cpu_report; // CPU body report is not Root/Draw/frame completion.
    if(scope->painting||scope->frame||scope->label_item||scope->normal_common_button.get()!=child.get()||!child->own_return)
     throw std::runtime_error("Common Button actual paint did not return SAME closed OwnReturn");
    const auto result=*child->own_return;
    result.require_physical_current_without_callbacks();first();result.require_source_owned(*operation);first();full();
    // SAME closed Root/Item/Material return only. Do not revive or query old
    // ACTIVE Invocation/Record/Slot/childFrame after Publication.paint RETURN.
    // Parent restoration runs before owner callback receives this return.
    return result;
   }catch(...){saved_life->failed=true;throw;}
  };
  // Exact additive Root8 third consumer. NULL texture does not suppress a
  // genuine positive178 callback after the unconditional Common return.
  BnwWorldViewControlTree::NormalCommonCaptionConsumer caption_consumer=
   [saved_life,scope,renderer,operation](
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const detail::BnwNormalRootBatchHistory::Guard& supplied_root,
    std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> supplied_matrix,
    const detail::BnwNormalCommonChildCall& supplied_common,
    const detail::BnwNormalButtonCaptionCall& supplied_caption)
    ->std::unique_ptr<detail::BnwNormalLabelMaterialLoan>{
    // Immutable controls are owned before Source or allocator callbacks.
    const auto common=supplied_common;const auto caption=supplied_caption;
    const auto root=supplied_root;const auto matrix=std::move(supplied_matrix);
    const auto origin=[&]{
     if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
        !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
      throw std::runtime_error("Common caption callback original Ui scope retired");
    };
    origin();common.require_current_without_callbacks();caption.require_current_without_callbacks();
    caption.require_same_common_call_without_callbacks(common);origin();scope->pure();origin();
    if(&op!=operation||&scope->operation!=operation||!matrix||!scope->normal_root_execution||
       !scope->normal_root_tree||!scope->normal_context_root_frame||!scope->publication||!scope->frame||
       !scope->painting||scope->label_item||scope->normal_button_caption||scope->normal_common_button||
       scope->normal_visible_child||scope->normal_bootstrap_label)
     throw std::runtime_error("Common caption lacks SAME restored original parent Context");
    const auto tree=scope->normal_root_tree;const auto actual_parent=scope->normal_context_root_frame;
    const auto*const execution=scope->normal_root_execution.get();
    const auto parent=scope->publication->original_owner_without_callbacks();origin();
    const auto adjusted=parent->actual_adjusted_frame_owner_for_paint_without_callbacks(*renderer,*scope->frame);origin();
    const auto ordinal=caption.actual_ordered_ordinal_without_callbacks();origin();
    std::shared_ptr<NormalButtonCaptionCapture> child;
    std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> rect;
    std::optional<std::array<std::uint32_t,4>> rect_words;
    const auto first=[&]{
     origin();common.require_current_without_callbacks();caption.require_current_without_callbacks();
     caption.require_same_common_call_without_callbacks(common);origin();scope->pure();origin();
     if(&scope->operation!=operation||scope->normal_root_execution.get()!=execution||
        scope->normal_root_tree.get()!=tree.get()||scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree)||
        scope->normal_context_root_frame.get()!=actual_parent.get()||scope->normal_context_root_frame.owner_before(actual_parent)||
        actual_parent.owner_before(scope->normal_context_root_frame)||scope->publication!=parent.get()||
        scope->frame!=adjusted.get()||!scope->painting||scope->label_item||scope->normal_button_caption||
        scope->normal_common_button||scope->normal_visible_child||scope->normal_bootstrap_label||
        common.actual_kind_without_callbacks()!=detail::BnwNormalCommonChildCall::Kind::button||
        common.actual_phase_without_callbacks()!=detail::BnwNormalCommonChildCall::Phase::button_caption178||
        common.actual_ordered_ordinal_without_callbacks()!=ordinal||caption.actual_ordered_ordinal_without_callbacks()!=ordinal)
      throw std::runtime_error("Common caption SAME original parent/Tree/current phase changed");
     root.require_physical_current_without_callbacks();root.require_same_root_without_callbacks(common.captured_root_without_callbacks());
     root.require_same_root_without_callbacks(caption.captured_root_without_callbacks());
     root.require_same_root_without_callbacks(execution->captured_root_without_callbacks());
     root.require_actual_renderer_without_callbacks(*renderer);root.require_actual_matrix_without_callbacks(*matrix);
     parent->require_adjusted_frame_owner_without_callbacks(adjusted);origin();
     if(rect){
      if(!child||!rect_words)throw std::runtime_error("Caption FP32 rect lost original snapshot custody");
      rect->require_current_without_callbacks();rect->require_same_caption_without_callbacks(caption);
      rect->require_same_initialized_run_without_callbacks(child->original_run);
      if(rect->raw_words_without_callbacks()!=*rect_words)
       throw std::runtime_error("Caption FP32 original four words changed after callback");
      if(child->fp32_rect&&(child->fp32_rect.get()!=rect.get()||child->fp32_rect.owner_before(rect)||
         rect.owner_before(child->fp32_rect)||!child->fp32_rect_words||*child->fp32_rect_words!=*rect_words))
       throw std::runtime_error("Caption FP32 capture adopted another physical rect");
      origin();
     }
    };
    const auto full=[&]{first();common.require_source_owned(*operation);first();
     caption.require_source_owned(*operation);first();root.require_source_owned(*operation);first();
     if(rect){rect->require_source_owned(*operation);first();}scope->full();first();};
    try{
     first();full();
     // Reuse the actual existing closed capture; this owns original completed
     // Run and parent physical owners and issues neither child Pub nor Frame.
     child=renderer->capture_current_normal_button_caption_owned(common,caption,matrix);first();full();
     if(!child||child->original_tree.get()!=tree.get()||child->original_tree.owner_before(tree)||tree.owner_before(child->original_tree)||
        child->parent_publication.get()!=parent.get()||child->parent_publication.owner_before(parent)||parent.owner_before(child->parent_publication)||
        child->parent_frame.get()!=adjusted.get()||child->parent_frame.owner_before(adjusted)||adjusted.owner_before(child->parent_frame)||
        child->parent_actual_frame.get()!=actual_parent.get()||child->parent_actual_frame.owner_before(actual_parent)||actual_parent.owner_before(child->parent_actual_frame))
      throw std::runtime_error("Common caption capture changed SAME original parent owners");
     child->root.require_same_root_without_callbacks(root);first();
     child->caption.require_same_common_call_without_callbacks(common);first();
     if(child->projection||child->fp32_rect||child->fp32_rect_words)
      throw std::runtime_error("Caption original projection/FP32 rect capture cannot replay");
     // Root9 requires genuine once-cached caption raw arguments. Establish
     // them through the actual Root3->3a projection before the rect issuer.
     full();const auto projection=caption.projection_for_append_owned(*operation);first();full();
     if(!projection.font_owner||projection.numeric_controls!=root.numeric_controls_without_callbacks())
      throw std::runtime_error("Caption actual cached projection lacks SAME original font/numeric Root");
     first();full();child->projection.emplace(projection);first();full();
     if(!child->projection||child->projection->font_owner.get()!=projection.font_owner.get()||
        child->projection->font_owner.owner_before(projection.font_owner)||projection.font_owner.owner_before(child->projection->font_owner)||
        child->projection->numeric_controls!=projection.numeric_controls)
      throw std::runtime_error("Caption cached projection adoption changed during Source");
     first();full();const auto actual_rect=tree->capture_normal_button_caption_fp32_rect_owned(*operation,caption);first();
     if(!actual_rect)throw std::runtime_error("Caption genuine Root9 FP32 issuer omitted original loan");
     actual_rect->require_current_without_callbacks();actual_rect->require_same_caption_without_callbacks(caption);
     actual_rect->require_same_initialized_run_without_callbacks(child->original_run);first();
     // Own exact words while CURRENT before the next callback/allocator cut.
     const auto words=actual_rect->raw_words_without_callbacks();first();
     rect_words.emplace(words);rect=actual_rect;first();full();
     child->fp32_rect=rect;child->fp32_rect_words.emplace(words);first();full();
     // Root9 geometry is real; current inheritedclip is the first unavailable
     // producer. Root shared paint/emission/data carrier also awaits freeze.
     // No arbitrary integer rect, viewport clip, child Pub/Frame or fake return.
     throw std::runtime_error("Common caption requires genuine current inheritedclip before FP32 append");
    }catch(...){saved_life->failed=true;throw;}
   };
 origin();scope->full();origin();
 auto result=std::make_shared<const BnwWorldViewControlTree::NormalCommonChildConsumers>(
  std::move(label),std::move(button),std::move(caption_consumer));
 origin();scope->full();origin();return result;
}


UiGlPaintReport UiScreenGlRenderer::WorldViewPaintScope::enter_current_normal_common_button_owned(
 UiScreenGlRenderer& supplied,std::shared_ptr<NormalCommonButtonCapture> supplied_child,
 std::shared_ptr<const ComposedUiFrame> supplied_adjusted,std::int32_t width,std::int32_t height)const{
 const auto saved_life=life;const auto*const scope=this;auto*const renderer=&supplied;
 const auto child=std::move(supplied_child);const auto adjusted=std::move(supplied_adjusted);
 const auto origin=[&]{
  if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
     !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
   throw std::runtime_error("Common Button entered original Ui scope retired");
 };
 origin();if(!child||!child->publication||!child->matrix||!adjusted)
  throw std::runtime_error("Common Button entered CPU original custody absent");
 const auto common=child->common;const auto root=child->root;const auto matrix=child->matrix;
 const auto publication=child->publication;
 const auto first=[&]{
  origin();common.require_current_without_callbacks();origin();scope->pure();origin();
  if(scope->normal_common_button.get()!=child.get()||scope->normal_common_button.owner_before(child)||
     child.owner_before(scope->normal_common_button)||scope->publication!=publication.get()||
     !scope->painting||scope->frame!=adjusted.get()||scope->label_item||
     !scope->normal_root_execution||!scope->normal_root_tree||width<=0||height<=0||
     common.actual_kind_without_callbacks()!=detail::BnwNormalCommonChildCall::Kind::button||
     common.actual_phase_without_callbacks()!=detail::BnwNormalCommonChildCall::Phase::button_own118)
   throw std::runtime_error("Common Button entered SAME publication/frame/current phase changed");
  root.require_physical_current_without_callbacks();root.require_same_root_without_callbacks(common.captured_root_without_callbacks());
  root.require_same_root_without_callbacks(scope->normal_root_execution->captured_root_without_callbacks());
  root.require_actual_renderer_without_callbacks(*renderer);root.require_actual_matrix_without_callbacks(*matrix);
  publication->require_adjusted_frame_owner_without_callbacks(adjusted);origin();
 };
 const auto*const operation=&scope->operation;
 const auto full=[&]{first();common.require_source_owned(*operation);first();
  root.require_source_owned(*operation);first();scope->full();first();};
 first();
 if(child->actual_frame||child->own_return)
  throw std::runtime_error("Common Button actual CPU entry cannot replay Frame or own118");
 const auto parent_entry=common.captured_root_context_entry_without_callbacks();first();
 const auto enclosing=publication->actual_enclosing_root_context_entry_without_callbacks();first();
 if(!enclosing||publication->actual_root_context_entry_for_paint_without_callbacks())
  throw std::runtime_error("Common Button child substituted primary root Context receiver");
 // Generic Frame factory authenticates child Control plus enclosing Entry
 // original Scope/SAME Tree and SAME adjusted owner. No Label role or item0.
 full();const auto actual=scope->capture_actual_normal_root_frame_owned(*renderer);first();full();
 const auto guard=actual->capture_guard_without_callbacks();first();
 guard.require_actual_renderer_without_callbacks(*renderer);
 guard.require_actual_publication_without_callbacks(*publication);first();
 if(child->actual_frame||child->own_return)
  throw std::runtime_error("Common Button actual CPU birth changed during callbacks");
 child->actual_frame=actual;first();
 (void)parent_entry;
 // OWNER-CHOSEN ABI PENDING actual factory freeze. Its body must issue
 // Source28 canonical fields and internally attach SAME impl ONCE before
 // own118 effects; this Ui body cannot create that capability or backing.
 const auto tree=scope->normal_root_tree;first();
 full();const auto invocation=tree->capture_normal_common_button_invocation_owned(*operation,common,*publication,*actual);first();full();
 invocation.require_current_without_callbacks();invocation.require_same_root_without_callbacks(root);
 invocation.require_actual_renderer_without_callbacks(*renderer);
 invocation.require_actual_publication_without_callbacks(*publication);
 invocation.require_actual_frame_without_callbacks(guard);first();
 if(child->own_return)throw std::runtime_error("Common Button own118 closed return replay");
 // ONE actual Ui3 own118 call. Constructor/resource-null/zeroINDEX outcomes
 // remain original Producer decisions, never Ui defaults or report flags.
 auto result=scope->prepare_current_normal_button_own118_owned(*renderer,invocation,actual);first();
 result.require_physical_current_without_callbacks();first();
 result.require_same_invocation_control_without_callbacks(invocation);first();
 result.require_source_owned(*operation);first();full();
 if(child->own_return)throw std::runtime_error("Common Button own118 return changed during Source");
 child->own_return.emplace(result);first();
 // Actual child CPU body report after genuine closed own118 return. No GPU
 // Draw has been submitted here, and this is not a wholeRoot/frame receipt.
 UiGlPaintReport report;
 report.readonly_text.frame={adjusted->frame_owner,adjusted->frame_sequence,adjusted->snapshot_frame,width,height};
 first();return report;

}

UiGlPaintReport UiScreenGlRenderer::WorldViewPaintScope::begin_current_normal_context_root_owned(
 UiScreenGlRenderer& supplied,const detail::BnwNormalRootContextEntry& supplied_entry)const{
 const auto saved_life=life;const auto*const scope=this;auto*const renderer=&supplied;
 const auto entry=supplied_entry;
 const auto origin=[&]{
  if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
     !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
   throw std::runtime_error("Entered Context original Ui scope retired");
 };
 origin();scope->pure();origin();
 if(!scope->painting||!scope->frame||!scope->publication||scope->label_item||
    scope->normal_root_execution||scope->normal_root_tree||scope->normal_context_root_frame)
  throw std::runtime_error("Entered Context must begin ONE genuine original Root without indexed item");
 const auto publication=scope->publication->original_owner_without_callbacks();origin();
 const auto tree=publication->tree_;const auto*const operation=&scope->operation;
 const auto serial=scope->serial;const auto*const composed=scope->frame;
 const auto actual=scope->capture_actual_normal_root_frame_owned(*renderer,entry);origin();
 const auto frame=actual->capture_guard_without_callbacks();
 const auto first=[&]{
  origin();scope->pure();origin();frame.require_current_without_callbacks();
  if(!publication||!tree||scope->publication!=publication.get()||!scope->painting||
     scope->frame!=composed||scope->label_item||&scope->operation!=operation||scope->serial!=serial)
   throw std::runtime_error("Entered Context original Publication/Frame/Source tuple changed");
  frame.require_actual_context_entry_without_callbacks(*publication,entry);
  frame.require_actual_renderer_without_callbacks(*renderer);origin();
 };
 const auto full=[&]{first();frame.require_source_owned(*operation);first();scope->full();first();};
 first();
 bool same_tree=false;for(const auto& original:scope->trees){first();
  if(original.get()==tree.get()&&!original.owner_before(tree)&&!tree.owner_before(original))same_tree=true;}
 if(!same_tree)throw std::runtime_error("Entered Context original Tree is not actual Native cohort");
 for(const auto& old:scope->normal_completed_roots){first();
  if(old&&old->tree.get()==tree.get())throw std::runtime_error("Completed original Context Tree cannot renew Root");}
 full();auto execution=tree->begin_normal_root_batch_execution_owned(*operation,*publication,*actual);first();full();
 if(!execution||scope->normal_root_execution||scope->normal_root_tree||scope->normal_context_root_frame)
  throw std::runtime_error("Entered Context execution birth changed original custody");
 const auto root=execution->captured_root_without_callbacks();first();
 frame.require_actual_publication_without_callbacks(*publication);first();
 root.require_actual_renderer_without_callbacks(*renderer);first();
 scope->normal_context_root_frame=actual;scope->normal_root_tree=tree;
 scope->normal_root_execution=std::move(execution);first();full();
 const auto consumers=scope->normal_common_consumers;first();
 if(!consumers)throw std::runtime_error("Entered Context lacks owned CURRENT Common consumers");
 // UNFROZEN: declaration/getters are exact Source1. Source2 must earn every
 // child return, actual Common tail/cursor, whole Root registration and seal.
 tree->stage_normal_root_common_children_owned(*operation,*scope->normal_root_execution,consumers);
 first();full();
 // The mixed selected-record drain/completion supplier is still Root-owned.
 // Do not send an own118 Material to the accepted font-only queue consumer.
 throw std::runtime_error("Context Common draft requires genuine mixed manager drain/completion supplier");
}

std::unique_ptr<UiScreenGlRenderer::LabelNativeProgramCapture>
UiScreenGlRenderer::capture_label_native_programs_owned(std::shared_ptr<LabelPaintCapture> label){
 // The independent renderer-owned gate exists before this method can allocate.
 // Gate retention never renews renderer/Source authority; origin repeats BOTH.
 if(!label||!label->data)throw std::runtime_error("Font code preparation requires retained genuine DATA");
 const auto initial_data=label->data->capture_guard_without_callbacks();
 initial_data.require_current_without_callbacks();
 const auto life=worldview_lifetime_;const auto gate=label_program_preparation_;
 const auto*const renderer=this;
 if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
    !life->active||life->renderer!=renderer||!life->active_scope||!gate)
  throw std::runtime_error("Font code preparation lacks genuine entered renderer");
 unsigned idle=0;
 if(!gate->compare_exchange_strong(idle,1)){
  gate->store(2);throw std::runtime_error("Font code preparation recursive or sticky failed");
 }
 struct Preparation final {
  const std::shared_ptr<std::atomic<unsigned>> gate;
  ~Preparation(){unsigned active=1;(void)gate->compare_exchange_strong(active,0);}
 } preparation{gate}; // Never clears poisoned2, including caught reentry.
 const auto origin=[&]{if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
  !life->active||life->renderer!=renderer||!life->active_scope||gate->load()!=1)
  throw std::runtime_error("Font code preparation renderer retired or recursively poisoned");
  initial_data.require_current_without_callbacks();};
 origin();const auto*const renderer_context=&context_;const auto loaded_render=gl_;auto cache=label_original_program_cache_;
 if(!cache){
  // Split allocation/adoption; never reread this after an unchecked allocator.
  origin();auto provisional=std::unique_ptr<LabelOriginalProgramCache>(new LabelOriginalProgramCache);origin();
  cache=std::shared_ptr<LabelOriginalProgramCache>(std::move(provisional));origin();
  label_original_program_cache_=cache;origin();
 }
 if(cache->creator!=std::this_thread::get_id()||cache->active||cache->failed){
  cache->failed=true;throw std::runtime_error("Font code cache retired/recursive");}
 cache->active=true;
 struct Active final {LabelOriginalProgramCache& value;~Active(){value.active=false;}};
 std::unique_ptr<LabelNativeProgramCapture> result;
 try{
  {
   Active active{*cache};origin();
   origin(); // SAME owning Label capture is transferred, never recaptured.
   // LAST lexical facade/envelope reads before any subsequent Source effect.
   const auto data=label->data->capture_guard_without_callbacks();
   const auto paint=data.captured_paint_guard_without_callbacks();const auto early=data.captured_early_flush_guard_without_callbacks();
   if(!label->metadata||!label->atlas||!label->pin)throw std::runtime_error("Font code canonical capture association absent");
   const auto metadata=*label->metadata;const auto atlas=label->atlas;const auto pin=label->pin;
   const auto*const scope=static_cast<const WorldViewPaintScope*>(life->active_scope);
   const auto*const operation=&scope->operation;const auto serial=scope->serial;
   const auto first=[&]{origin();data.require_current_without_callbacks();
    if(life->active_scope!=scope||&scope->operation!=operation||scope->serial!=serial||cache->failed||!cache->active)
     throw std::runtime_error("Font code exact entered operation/cache changed");
    scope->pure();data.require_current_without_callbacks();data.require_actual_renderer_without_callbacks(*renderer);
    paint.require_current_without_callbacks();data.require_same_paint_guard_without_callbacks(paint);
    early.require_current_without_callbacks();early.require_same_paint_guard_without_callbacks(paint);
    metadata.require_current_without_callbacks();metadata.require_actual_atlas_without_callbacks(*atlas);
    pin->first_without_callbacks();
    if(&pin->context_reference_without_callbacks()!=renderer_context||metadata.impl_->renderer!=renderer)
     throw std::runtime_error("Font code actual Ui dispatch Context/metadata owner differs");
    atlas->require_same_owned_context_pin_no_current(pin);
    (void)early.accepted_pair_without_callbacks(*atlas);data.require_current_without_callbacks();origin();};
   const auto full=[&]{first();data.require_source_owned(*operation);first();metadata.require_source_owned(*operation);first();};
   // DATA/Paint/Early/metadata/canonical actual Atlas/SAME Pin association is
   // sealed before the operation-selected bank supplier can execute callbacks.
   first();const auto bank=operation->selected_provider_startup_bank_owned();first();
   if(!bank)throw std::runtime_error("Font code canonical selected bank absent");
   std::shared_ptr<graphics::GlBnwFontShader::OriginalProgramLibrary> library;
   for(const auto& entry:cache->entries){first();
    if(entry.bank.get()==bank.get()&&!entry.bank.owner_before(bank)&&!bank.owner_before(entry.bank)&&
       entry.pin.get()==pin.get()&&!entry.pin.owner_before(pin)&&!pin.owner_before(entry.pin)){
     library=entry.library;break;}
   }
   first();
   if(!library){
    full();library=graphics::GlBnwFontShader::create_original_program_library_owned(
     *operation,data,bank,atlas,pin,loaded_render);first();full();
    // Reserve while the accepted local library owns every GL name. Allocator
    // callback failures cannot publish a partial entry or erase old entries.
    first();if(cache->entries.size()==cache->entries.max_size())throw std::runtime_error("Font physical code cache exceeds capacity");
    cache->entries.reserve(cache->entries.size()+1);first();full();
    first();cache->entries.push_back({bank,pin,library});first();
   }
   // Cache hit executes no exe read/hash or GL compile/create. Fresh DATA is
   // still admitted through its own current code use and canonical early Pair.
   first();auto use=graphics::GlBnwFontShader::capture_original_program_use_owned(
    *operation,data,library,bank,atlas,pin);first();full();use->require_current_without_callbacks();first();
   first();result=std::unique_ptr<LabelNativeProgramCapture>(new LabelNativeProgramCapture);first();
   result->label=std::move(label);result->programs=std::move(use);first();full();
  } // No raw facade rereads; temporary source/operation snapshots are gone.
  origin();result->programs->require_current_without_callbacks();origin();
  return result;
 }catch(...){cache->failed=true;throw;}
}
// Only the actual Ui paint loop constructs this lexical item stamp.
struct UiScreenGlRenderer::LabelPaintItemScope final {
 const std::shared_ptr<WorldViewPaintLifetime> life;
 WorldViewPaintScope*const scope;
 const ComposedUiFrame*const frame;
 const BnwWorldViewControlRenderPublication*const publication;
 const runtime::BnwWorldViewSourceOperationLoan*const operation;
 const std::uint64_t serial;
 const WorldViewPaint*const item;
 const std::size_t index;
 const LabelPaintCapture::DestinationGeometry destination;
 const WorldViewRect clip;
 const std::array<GLint,4> viewport;
 bool alive=true;
 LabelPaintItemScope(WorldViewPaintScope& entered,std::size_t actual_index,
  const WorldViewPaint& actual_item,const WorldViewRect& actual_clip,GLint width,GLint height)
  :life(entered.life),scope(&entered),frame(entered.frame),publication(entered.publication),
   operation(&entered.operation),serial(entered.serial),item(&actual_item),index(actual_index),
   destination(legacy_rectangle(actual_item)),clip(actual_clip),viewport{0,0,width,height}{
  entered.pure();
  if(actual_item.normal_caption_fp32_rect)
   throw std::runtime_error("Legacy Ui stamp rejects closed caption geometry");
  install(entered);
 }
 LabelPaintItemScope(WorldViewPaintScope& entered,std::size_t actual_index,
  const WorldViewPaint& actual_item,const WorldViewRect& actual_clip,GLint width,GLint height,
  LabelPaintCapture::DestinationGeometry actual_destination)
  :life(entered.life),scope(&entered),frame(entered.frame),publication(entered.publication),
   operation(&entered.operation),serial(entered.serial),item(&actual_item),index(actual_index),
   destination(actual_destination),clip(actual_clip),viewport{0,0,width,height}{
  entered.pure();
  if(!destination.caption||!destination.words)
   throw std::runtime_error("Caption Ui stamp requires original closed geometry");
  destination.require_item(actual_item);install(entered);
 }
private:
 static WorldViewRect legacy_rectangle(const WorldViewPaint& actual){
  if(actual.normal_caption_fp32_rect)
   throw std::runtime_error("Legacy stamp rejects caption before integer geometry capture");
  return actual.destination;
 }
 void install(WorldViewPaintScope& entered){
  if(!entered.painting||!frame||!publication||entered.label_item||
     index>=frame->paints.size()||&frame->paints[index]!=item)
   throw std::runtime_error("Default target requires actual entered Ui loop item");
  entered.label_item=this;
 }
public:
 ~LabelPaintItemScope(){
  alive=false;
  if(life->creator==std::this_thread::get_id()&&life->active&&life->active_scope==scope&&
     scope->label_item==this)scope->label_item=nullptr;
 }
 LabelPaintItemScope(const LabelPaintItemScope&)=delete;
 LabelPaintItemScope&operator=(const LabelPaintItemScope&)=delete;
};
// Pure actual item comparison only; no index/role/Frame issuer is added.
void detail::BnwLabelActualPaintCallFrame::Guard::require_actual_paint_item_without_callbacks(
 const BnwWorldViewControlRenderPublication& actual,std::size_t paint_index)const{
 const auto saved=impl_;const auto*const publication=&actual;const auto index=paint_index;
 saved->first();
 label_paint_frame_need(saved->publication==publication,
  "Actual Label paint item belongs to another publication");
 const auto*const scope=saved->scope;const auto*const frame=saved->frame;
 // FIRST independently establishes exact active life/scope before these reads.
 label_paint_frame_need(scope&&frame&&scope->painting&&scope->frame==frame&&
  scope->publication==publication&&&scope->operation==saved->operation&&scope->serial==saved->serial,
  "Actual Label paint item frame/operation changed");
 const auto*const stamp=scope->label_item;
 label_paint_frame_need(stamp,"Actual Label paint item lacks genuine entered Ui stamp");
 const auto life=saved->renderer_life.lock();
 label_paint_frame_need(life&&stamp->life.get()==life.get()&&
  !stamp->life.owner_before(life)&&!life.owner_before(stamp->life)&&
  stamp->alive&&stamp->scope==scope&&stamp->frame==frame&&stamp->publication==publication&&
  stamp->operation==saved->operation&&stamp->serial==saved->serial&&stamp->index==index,
  "Actual Label paint item stamp/index identity changed");
 label_paint_frame_need(index<frame->paints.size()&&stamp->item==&frame->paints[index],
  "Actual Label paint item is not SAME current indexed frame entry");
 saved->first();
 label_paint_frame_need(scope->label_item==stamp&&stamp->alive,
  "Actual Label paint item stamp replaced at pure POST");
 saved->first();
}
// SAME indexed Ui entry association; this capability grants no Label role.
void detail::BnwNormalRootPaintCallFrame::Guard::require_actual_paint_item_without_callbacks(
 const BnwWorldViewControlRenderPublication& actual,std::size_t paint_index)const{
 const auto saved=impl_;const auto*const publication=&actual;const auto index=paint_index;
 saved->first();
 label_paint_frame_need(saved->control_id&&!saved->context_entry,
  "Generic Context Frame cannot grant an indexed paint item");
 label_paint_frame_need(saved->publication==publication,
  "Actual generic Root paint item belongs to another publication");
 const auto*const scope=saved->scope;const auto*const frame=saved->frame;
 // FIRST independently establishes exact active life/scope before these reads.
 label_paint_frame_need(scope&&frame&&scope->painting&&scope->frame==frame&&
  scope->publication==publication&&&scope->operation==saved->operation&&scope->serial==saved->serial,
  "Actual generic Root paint item frame/operation changed");
 const auto*const stamp=scope->label_item;
 label_paint_frame_need(stamp,"Actual generic Root paint item lacks genuine entered indexed Ui stamp");
 const auto life=saved->renderer_life.lock();
 label_paint_frame_need(life&&stamp->life.get()==life.get()&&
  !stamp->life.owner_before(life)&&!life.owner_before(stamp->life)&&
  stamp->alive&&stamp->scope==scope&&stamp->frame==frame&&stamp->publication==publication&&
  stamp->operation==saved->operation&&stamp->serial==saved->serial&&stamp->index==index,
  "Actual generic Root paint item stamp/index identity changed");
 label_paint_frame_need(index<frame->paints.size()&&stamp->item==&frame->paints[index],
  "Actual generic Root paint item is not SAME current indexed frame entry");
 saved->first();
 label_paint_frame_need(scope->label_item==stamp&&stamp->alive,
  "Actual generic Root paint item stamp replaced at pure POST");
 saved->first();
}
detail::BnwNormalNonFontOwnReturn
UiScreenGlRenderer::WorldViewPaintScope::prepare_current_normal_button_own118_owned(
 UiScreenGlRenderer& supplied,const detail::BnwNormalNonFontPaintInvocation& supplied_invocation,
 std::shared_ptr<const detail::BnwNormalRootPaintCallFrame> supplied_frame)const{
 // Synchronous lexical Scope remains physical here; independent saved life
 // precedes EVERY later raw Scope read. Copy original closed controls before
 // any Source/native callback, not a captured semantic ButtonVisual.
 const auto saved_life=life;const auto*const saved_scope=this;
 const auto*const renderer=&supplied;const auto invocation=supplied_invocation;
 const auto actual_frame=std::move(supplied_frame);
 const auto scope_first=[&]{
  label_paint_frame_need(saved_life&&saved_life->creator==std::this_thread::get_id()&&
   saved_life->alive&&!saved_life->failed&&saved_life->active&&
   saved_life->renderer==renderer&&saved_life->active_scope==saved_scope,
   "Button own118 Ui scope retired or changed");
  saved_scope->pure();
 };
 scope_first();label_paint_frame_need(actual_frame!=nullptr,
  "Button own118 lacks ORIGINAL generic actual Frame");
 const auto frame_guard=actual_frame->capture_guard_without_callbacks();
 frame_guard.require_current_without_callbacks();scope_first();
 const auto*const publication=saved_scope->publication;
 label_paint_frame_need(publication&&saved_scope->painting,
  "Button own118 is outside ORIGINAL Publication paint");
 frame_guard.require_actual_publication_without_callbacks(*publication);
 const auto publication_owner=publication->original_owner_without_callbacks();scope_first();
 const auto*const operation=&saved_scope->operation;const auto serial=saved_scope->serial;
 const auto root=invocation.captured_root_without_callbacks();
 const auto control=invocation.actual_control_without_callbacks();
 const auto ordinal=invocation.actual_ordered_ordinal_without_callbacks();
 const auto flags=invocation.flags_word40_without_callbacks();
 const auto first=[&]{
  scope_first();frame_guard.require_current_without_callbacks();
  label_paint_frame_need(publication_owner&&publication_owner.get()==publication&&
   saved_scope->painting&&saved_scope->publication==publication&&
   &saved_scope->operation==operation&&saved_scope->serial==serial,
   "Button own118 original Publication/Source tuple changed");
  invocation.require_current_without_callbacks();
  invocation.require_actual_renderer_without_callbacks(*renderer);
  invocation.require_actual_publication_without_callbacks(*publication);
  invocation.require_actual_frame_without_callbacks(frame_guard);
  invocation.require_same_root_without_callbacks(root);
  label_paint_frame_need(invocation.actual_control_without_callbacks()==control&&
   invocation.actual_ordered_ordinal_without_callbacks()==ordinal&&
   invocation.flags_word40_without_callbacks()==flags,
   "Button own118 closed native receiver identity changed");
  frame_guard.require_actual_publication_without_callbacks(*publication);
  label_paint_frame_need(publication->actual_checked_control_id_for_paint_without_callbacks()==control,
   "Button own118 does not belong to SAME original checked Control");
  scope_first();
 };
 const auto full=[&]{
  first();invocation.require_source_owned(*operation);first();
  frame_guard.require_source_owned(*operation);first();
  operation->require_operation(serial);first();saved_scope->full();first();
 };
 first();
 // Root's v18 dispatcher must earn suppression and skip own118. This helper
 // cannot issue a fake zero-index/missing-resource return for an uncalled v8.
 label_paint_frame_need((flags&std::uint32_t{0x2002})==0&&
  (flags&std::uint32_t{0x10000000})==0,
  "Button own118 consumer was called on suppressed or skipped native v8");
 full();
 auto result=detail::BnwNormalNonFontButtonProducer::prepare_button_own118_owned(*operation,invocation);
 first();result.require_physical_current_without_callbacks();first();
 result.require_same_invocation_control_without_callbacks(invocation);first();
 result.require_source_owned(*operation);first();full();first();
 // A real closed result only. Root still performs unconditional56F630 and
 // conditional nonNULL178 AFTER this return, including genuine resource-null.
 return result;
}

UiScreenGlRenderer::NormalLabelRunForActualItem
UiScreenGlRenderer::capture_normal_label_run_for_actual_item_owned(
 std::shared_ptr<const detail::BnwLabelActualPaintCallFrame> actual_frame){
 // All outer/facade identities are snapped before Root's callbackful getter.
 const auto life=worldview_lifetime_;const auto*const renderer=this;
 if(!actual_frame||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
    !life->active||life->renderer!=renderer||!life->active_scope)
  throw std::runtime_error("Normal Label getter requires actual entered Ui/Frame custody");
 const auto frame_guard=actual_frame->capture_guard_without_callbacks();
 frame_guard.require_current_without_callbacks();
 frame_guard.require_actual_renderer_without_callbacks(*renderer);
 const auto*const scope=static_cast<const WorldViewPaintScope*>(life->active_scope);
 const auto origin=[&]{
  frame_guard.require_current_without_callbacks();
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||!life->active||
     life->renderer!=renderer||life->active_scope!=scope)
   throw std::runtime_error("Normal Label getter Ui lexical scope retired");
 };
 origin();scope->pure();origin();
 const auto*const stamp=scope->label_item;
 if(!scope->painting||!scope->frame||!scope->publication||!stamp||!stamp->alive||
    stamp->scope!=scope||stamp->frame!=scope->frame||stamp->publication!=scope->publication||
    stamp->operation!=&scope->operation||stamp->serial!=scope->serial)
  throw std::runtime_error("Normal Label getter lacks actual current item stamp");
 const auto*const publication=scope->publication;
 const auto*const operation=&scope->operation;
 const auto serial=scope->serial;const auto index=stamp->index;
 const auto first=[&]{
  origin();
  if(!scope->painting||scope->publication!=publication||&scope->operation!=operation||
     scope->serial!=serial||scope->label_item!=stamp||!stamp->alive||stamp->index!=index)
   throw std::runtime_error("Normal Label getter entered item changed");
  frame_guard.require_actual_paint_item_without_callbacks(*publication,index);origin();
 };
 first();
 auto run=publication->normal_label_run_for_actual_item_owned(index,*operation,*actual_frame);
 first();
 // Null is only Root's proven other-control/no-Label result. Missing genuine
 // Label init/positive170/emission is an exception in the sealed Root getter.
 // The verified index is VALUE; no raw index is used as a Label-role issuer.
 return NormalLabelRunForActualItem{std::move(run),index};
}
struct UiScreenGlRenderer::LabelDefaultFramebufferTarget final {
 struct HistoricalAssociation final {
  const LabelPaintCapture::HistoricalTargetGeometry geometry;
  const detail::BnwNormalRootBatchHistory::Guard root;
  const detail::BnwNormalItemOccurrence::Guard occurrence;
  const detail::BnwNormalQueueDrainPermission::Guard permission;
  const graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity record;
  const graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt descriptor;
  HistoricalAssociation(LabelPaintCapture::HistoricalTargetGeometry actual_geometry,
   detail::BnwNormalRootBatchHistory::Guard actual_root,
   detail::BnwNormalItemOccurrence::Guard actual_occurrence,
   detail::BnwNormalQueueDrainPermission::Guard actual_permission,
   graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity actual_record,
   graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt actual_descriptor)
   :geometry(std::move(actual_geometry)),root(std::move(actual_root)),occurrence(std::move(actual_occurrence)),
    permission(std::move(actual_permission)),record(std::move(actual_record)),descriptor(std::move(actual_descriptor)){}
  void first(const UiScreenGlRenderer& renderer)const{
   geometry.scope_health.require_physical_current_without_callbacks();
   geometry.scope_health.require_actual_renderer_without_callbacks(renderer);
   geometry.scope_health.require_actual_publication_without_callbacks(*geometry.publication);
   geometry.scope_health.require_actual_frame_owner_without_callbacks(geometry.frame_owner);
   root.require_physical_current_without_callbacks();root.require_actual_renderer_without_callbacks(renderer);
   occurrence.require_physical_current_without_callbacks();occurrence.require_same_root_without_callbacks(root);
   occurrence.require_actual_renderer_without_callbacks(renderer);
   occurrence.require_actual_publication_without_callbacks(*geometry.publication);
   occurrence.require_actual_control_without_callbacks(*geometry.publication->tree_,geometry.control);
   occurrence.require_actual_positioned_data_without_callbacks(geometry.positioned_data);
   permission.require_physical_current_without_callbacks();permission.require_same_root_without_callbacks(root);
   permission.require_actual_renderer_without_callbacks(renderer);
   permission.require_queued_record_without_callbacks(occurrence,record,descriptor);
   const auto dimensions=root.dimensions_without_callbacks();
   if(geometry.viewport[0]!=0||geometry.viewport[1]!=0||geometry.viewport[2]<=0||geometry.viewport[3]<=0||
      dimensions!=std::array<std::int32_t,2>{geometry.viewport[2],geometry.viewport[3]}||
      geometry.paint_index>=geometry.frame_owner->paints.size())
    throw std::runtime_error("Historical target geometry/root viewport association changed");
   // Root Item physical health above authenticates original caption metadata;
   // this additional equality reads immutable carrier/owned snapshot only.
   geometry.destination.require_item(geometry.frame_owner->paints[geometry.paint_index]);
   geometry.scope_health.require_physical_current_without_callbacks();
   root.require_physical_current_without_callbacks();occurrence.require_physical_current_without_callbacks();
   permission.require_queued_record_without_callbacks(occurrence,record,descriptor);
  }
 };
 struct ItemSnapshot final {
  const ComposedUiFrame* frame;
  const BnwWorldViewControlRenderPublication* publication;
  const runtime::BnwWorldViewSourceOperationLoan* operation;
  std::uint64_t serial;
  const WorldViewPaint* item;
  std::size_t index;
  LabelPaintCapture::DestinationGeometry destination;
  WorldViewRect clip;
  std::array<GLint,4> viewport;
 };
 static std::unique_ptr<LabelDefaultFramebufferTarget> capture_historical_owned(
  UiScreenGlRenderer&,const LabelPaintCapture&,
  const detail::BnwNormalRootBatchHistory::Guard&,const detail::BnwNormalItemOccurrence::Guard&,
  const detail::BnwNormalQueueDrainPermission::Guard&,
  const graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity&,
  const graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt&);
 struct Impl final {
  const std::thread::id creator=std::this_thread::get_id();
  std::atomic<bool> alive{true},failed{false};
  bool captured=false;
  const std::shared_ptr<const HistoricalAssociation> historical;
  const std::weak_ptr<WorldViewPaintLifetime> life;
  const UiScreenGlRenderer*const renderer;
  const WorldViewPaintScope*const scope;
  const LabelPaintItemScope*const item_stamp;
  const ComposedUiFrame*const frame;
  const BnwWorldViewControlRenderPublication*const publication;
  const runtime::BnwWorldViewSourceOperationLoan*const operation;
  const std::uint64_t serial;
  const WorldViewPaint*const item;
  const std::size_t index;
  const std::optional<detail::BnwLabelActualPaintCallFrame::Guard> frame_guard;
  const std::optional<detail::BnwLabelPaintTextLoan::Guard> paint_guard;
  const graphics::detail::BnwLabelSystemFontAtlasMetadata::Guard metadata_guard;
  const std::shared_ptr<const graphics::detail::BnwRendererContextPin> pin;
  const graphics::GlContextNativeIdentity native;
  const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
  const LabelPaintCapture::DestinationGeometry destination;
  const WorldViewRect clip;
  const std::array<GLint,4> viewport;
  const graphics::GlApi*loaded=nullptr;
  GLint draw_buffer=GL_NONE;
  Impl(std::weak_ptr<WorldViewPaintLifetime> actual_life,const UiScreenGlRenderer* actual_renderer,
   const WorldViewPaintScope* actual_scope,const LabelPaintItemScope* actual_item,ItemSnapshot actual,
   detail::BnwLabelActualPaintCallFrame::Guard f,detail::BnwLabelPaintTextLoan::Guard p,
   graphics::detail::BnwLabelSystemFontAtlasMetadata::Guard m,
   std::shared_ptr<const graphics::detail::BnwRendererContextPin> actual_pin,
   graphics::GlContextNativeIdentity actual_native,
   std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> actual_window)
   :historical(),life(std::move(actual_life)),renderer(actual_renderer),scope(actual_scope),item_stamp(actual_item),
    frame(actual.frame),publication(actual.publication),operation(actual.operation),
    serial(actual.serial),item(actual.item),index(actual.index),frame_guard(std::move(f)),paint_guard(std::move(p)),
    metadata_guard(std::move(m)),pin(std::move(actual_pin)),native(actual_native),
    window(std::move(actual_window)),destination(actual.destination),clip(actual.clip),
    viewport(actual.viewport){}
  Impl(std::weak_ptr<WorldViewPaintLifetime> actual_life,const UiScreenGlRenderer* actual_renderer,
   const WorldViewPaintScope* actual_scope,const runtime::BnwWorldViewSourceOperationLoan* actual_operation,
   std::uint64_t actual_serial,std::shared_ptr<const HistoricalAssociation> association,
   graphics::detail::BnwLabelSystemFontAtlasMetadata::Guard metadata,
   std::shared_ptr<const graphics::detail::BnwRendererContextPin> actual_pin,
   graphics::GlContextNativeIdentity actual_native,
   std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> actual_window)
   :historical(std::move(association)),life(std::move(actual_life)),renderer(actual_renderer),scope(actual_scope),
    item_stamp(nullptr),frame(historical->geometry.frame_owner.get()),publication(historical->geometry.publication.get()),
    operation(actual_operation),serial(actual_serial),item(&frame->paints[historical->geometry.paint_index]),
    index(historical->geometry.paint_index),frame_guard(),paint_guard(),metadata_guard(std::move(metadata)),
    pin(std::move(actual_pin)),native(actual_native),window(std::move(actual_window)),
    destination(historical->geometry.destination),clip(historical->geometry.clip),viewport(historical->geometry.viewport){}
  void first()const{
   if(!alive||failed||creator!=std::this_thread::get_id())
    throw std::runtime_error("Default target lexical facade retired/failed/foreign");
   const auto actual=life.lock();
   if(!actual||actual->creator!=creator||!actual->alive||actual->failed||!actual->active||
      actual->renderer!=renderer||actual->active_scope!=scope)
    throw std::runtime_error("Default target actual Ui scope retired");
   if(historical){
    historical->first(*renderer);
    if(&scope->operation!=operation||scope->serial!=serial)
     throw std::runtime_error("Historical target actual Source tuple changed");
   }else{
   if(!frame_guard||!paint_guard)throw std::runtime_error("Active target lacks original Frame/Paint guards");
   frame_guard->require_current_without_callbacks();paint_guard->require_current_without_callbacks();
   // Exact active stamp test BEFORE every borrowed item-stamp dereference.
   if(!scope->painting||scope->frame!=frame||scope->publication!=publication||
      &scope->operation!=operation||scope->serial!=serial||scope->label_item!=item_stamp||
      !item_stamp||!item_stamp->alive||item_stamp->frame!=frame||
      item_stamp->publication!=publication||item_stamp->operation!=operation||
      item_stamp->serial!=serial||item_stamp->index!=index||item_stamp->item!=item||
      index>=frame->paints.size()||&frame->paints[index]!=item||item_stamp->viewport!=viewport)
    throw std::runtime_error("Default target is not SAME actual entered Ui item");
   const auto same_rect=[](const WorldViewRect& a,const WorldViewRect& b){
    return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;
   };
   destination.require_same(item_stamp->destination);destination.require_item(*item);
   if(!same_rect(item_stamp->clip,clip))
    throw std::runtime_error("Default target actual immutable item rectangle changed");
   }
   metadata_guard.require_current_without_callbacks();
   pin->first_without_callbacks();
   if(!native.owner_thread||pin->identity_.window!=native.window||pin->identity_.dc!=native.dc||
      pin->identity_.context!=native.context)
    throw std::runtime_error("Default target SAME native Window/Context identity changed");
   if(window.expired())throw std::runtime_error("Default target original Window retired");
   if(historical)historical->first(*renderer);
   else{frame_guard->require_current_without_callbacks();paint_guard->require_current_without_callbacks();}
  }
  void full(){
   try{first();
    if(historical){
     historical->geometry.scope_health.require_source_owned(*operation);first();
     historical->root.require_source_owned(*operation);first();
     historical->occurrence.require_source_owned(*operation);first();
     historical->permission.require_source_owned(*operation);first();
    }else{
     frame_guard->require_source_owned(*operation);first();paint_guard->require_source_owned(*operation);first();
    }
    metadata_guard.require_source_owned(*operation);first();}
   catch(...){failed=true;throw;}
  }
  const graphics::GlApi* current_api(){
   try{first();const auto*actual=graphics::detail::BnwRendererContextPin::NormalAccess::
     api_current_without_switch_owned(pin);first();
    if(!actual||(loaded&&actual!=loaded)||!actual->GetIntegerv||!actual->GetError||
       !actual->GetGraphicsResetStatus)throw std::runtime_error("Default target loaded query dispatch changed");
    return actual;}
   catch(...){failed=true;throw;}
  }
  // Terminal native observations: PURE logical guards + SAME normal CURRENT.
  // No external Source observer may consume a query error before it is sampled.
  void status_without_source_callbacks(){
   try{
    first();const auto*a=current_api();const auto reset=a->GetGraphicsResetStatus();
    (void)current_api();first();
    if(reset!=GL_NO_ERROR){failed=true;throw std::runtime_error("Default target GL reset uncertainty");}
    first();a=current_api();const auto error=a->GetError();(void)current_api();first();
    if(error!=GL_NO_ERROR){failed=true;throw std::runtime_error("Default target GL error uncertainty");}
   }catch(...){failed=true;throw;}
  }
  void query_without_source_callbacks(GLenum field,GLint* actual){
   try{first();status_without_source_callbacks();const auto*a=current_api();
    a->GetIntegerv(field,actual);(void)current_api();
    status_without_source_callbacks();first();}
   catch(...){failed=true;throw;}
  }
  void verify_default_owned(bool initial){
   try{
    // LAST external Source FULL precedes the whole fresh target tuple.
    // All remaining query/status/current POST and acceptance are callback-free
    // with respect to Source; no historical tuple is called current.
    full();first();GLint framebuffer=-1;std::array<GLint,4> actual_viewport{};GLint buffer=GL_NONE;
    query_without_source_callbacks(GL_DRAW_FRAMEBUFFER_BINDING,&framebuffer);first();
    if(framebuffer!=0)throw std::runtime_error("Ui default target no longer actual FBO0");
    query_without_source_callbacks(GL_VIEWPORT,actual_viewport.data());first();
    if(actual_viewport!=viewport)throw std::runtime_error("Ui default target viewport changed");
    query_without_source_callbacks(GL_DRAW_BUFFER0,&buffer);first();
    if(buffer==GL_NONE||(!initial&&buffer!=draw_buffer))
     throw std::runtime_error("Ui default target actual draw buffer changed/unavailable");
    if(initial){draw_buffer=buffer;captured=true;}
    first();
   }catch(...){failed=true;throw;}
  }
 };
 const std::shared_ptr<Impl> impl;
 explicit LabelDefaultFramebufferTarget(std::shared_ptr<Impl> value):impl(std::move(value)){}
 ~LabelDefaultFramebufferTarget(){impl->alive=false;}
 class Guard final {
  friend class UiScreenGlRenderer;
  friend struct LabelDefaultFramebufferTarget;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl> value):impl_(std::move(value)){}
  void require_current_without_callbacks()const{
   impl_->first();if(!impl_->captured)throw std::runtime_error("Default target snapshot incomplete");
  }
  // Captured baseline DATA only. A fresh physical tuple still requires the
  // existing require_current_owned(); neither method enters an adapter phase.
  struct BaselineGeometry final {std::array<GLint,4> viewport;GLint draw_buffer;};
  BaselineGeometry baseline_geometry_without_callbacks()const{
   const auto actual=impl_;
   try{
    actual->first();
    if(!actual->captured||actual->viewport[0]!=0||actual->viewport[1]!=0||
       actual->viewport[2]<=0||actual->viewport[3]<=0||actual->draw_buffer==GL_NONE)
     throw std::runtime_error("Default target captured full baseline geometry absent");
    const BaselineGeometry value{actual->viewport,actual->draw_buffer};actual->first();return value;
   }catch(...){actual->failed=true;throw;}
  }
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
   const auto actual=impl_;const auto*const input=&operation;
   try{
    actual->first();
    if(!actual->captured||actual->operation!=input)
     throw std::runtime_error("Default target belongs to another actual Source operation");
    actual->full();actual->first();
   }catch(...){actual->failed=true;throw;}
  }
  // Independent SAME Pin/normal-current/reset/error gate. It does not read
  // integer viewport or perform Source FULL. Future physical row capture/
  // restore must retain this Guard and implement its own closed phase.
  void require_normal_current_without_source_callbacks()const{
   const auto actual=impl_;
   try{
    actual->first();if(!actual->captured)
     throw std::runtime_error("Default target normal gate lacks completed baseline");
    (void)actual->current_api();actual->status_without_source_callbacks();
    (void)actual->current_api();actual->first();
   }catch(...){actual->failed=true;throw;}
  }
  void require_current_owned()const{
   // Guard/facade may be destroyed by FULL: capture storage before effects.
   const auto actual=impl_;
   actual->first();if(!actual->captured)throw std::runtime_error("Default target snapshot incomplete");
   actual->verify_default_owned(false);actual->first();
  }
 };
 Guard capture_guard_without_callbacks()const{impl->first();return Guard(impl);}
};
// A single physical row transaction across Ui instances. Failure remains sticky;
// pending queues do not establish a safe order for overlapping target snapshots.
namespace { constinit std::atomic<unsigned> label_physical_rows_gate{0}; }
struct UiScreenGlRenderer::LabelDefaultRowsAdapter final {
 using Channel=graphics::detail::BnwOwnedGlRetirementNode::TargetRowsChannel;
 struct State final {
  const std::shared_ptr<LabelDefaultFramebufferTarget::Impl> target;
  const LabelDefaultFramebufferTarget::Guard target_guard;
  const graphics::GlApi api;
  const graphics::GlRenderApi render;
  const LabelDefaultFramebufferTarget::Guard::BaselineGeometry geometry;
  Channel::Owner node;
  std::array<GLfloat,4> baseline_viewport{};
  std::array<GLdouble,2> baseline_depth{};
  GLint baseline_clip_origin=0,baseline_clip_depth=0;
  std::atomic<bool> alive{true},finished{false},method_active{false};
  bool entered=false;
  State(LabelDefaultFramebufferTarget::Guard guard,graphics::GlApi actual_api,
   graphics::GlRenderApi actual_render,
   LabelDefaultFramebufferTarget::Guard::BaselineGeometry actual_geometry)
   :target(guard.impl_),target_guard(std::move(guard)),api(actual_api),render(actual_render),
    geometry(actual_geometry){}
  void poison()noexcept{label_physical_rows_gate.store(2);target->failed=true;}
  ~State()noexcept{
   // The node's deleter only enqueues SAME-Pin physical custody. No Source,
   // facade, normal-current query or context switch is permitted in retirement.
   if(!finished.load())poison();
  }
  void first()const{
   if(!alive.load()||finished.load()||label_physical_rows_gate.load()!=1)
    throw std::runtime_error("Physical rows lexical transaction retired/failed");
   target_guard.require_current_without_callbacks();
  }
  void full(){
   // Global row exclusion is independently FIRST at EVERY Source boundary.
   // A nested different-Ui attempt can poison the global gate without changing
   // this retained target's logical lifetime; do not hide it in Target::full().
   first();
   if(target->historical){
    target->historical->geometry.scope_health.require_source_owned(*target->operation);first();
    target->historical->root.require_source_owned(*target->operation);first();
    target->historical->occurrence.require_source_owned(*target->operation);first();
    target->historical->permission.require_source_owned(*target->operation);first();
   }else{
    target->frame_guard->require_source_owned(*target->operation);first();
    target->paint_guard->require_source_owned(*target->operation);first();
   }
   target->metadata_guard.require_source_owned(*target->operation);first();
  }
  using FontTargetGuard=graphics::detail::BnwLabelDefaultFontTargetLoan::Guard;
  void guarded_first(const FontTargetGuard* guard){
   first();if(guard)guard->require_current_without_callbacks();first();
  }
  void current(const FontTargetGuard* guard=nullptr){
   guarded_first(guard);(void)target->current_api();guarded_first(guard);
  }
  void effect_fence(const FontTargetGuard* guard=nullptr){
   guarded_first(guard);current(guard);const auto reset=api.GetGraphicsResetStatus();
   if(reset!=GL_NO_ERROR&&node)Channel::storage(*node).uncertain=true;
   current(guard);guarded_first(guard);
   if(reset!=GL_NO_ERROR){if(node)Channel::storage(*node).uncertain=true;
    poison();throw std::runtime_error("Physical rows reset uncertainty");}
   current(guard);const auto error=api.GetError();
   if(error!=GL_NO_ERROR&&node)Channel::storage(*node).uncertain=true;
   current(guard);guarded_first(guard);
   if(error!=GL_NO_ERROR){if(node)Channel::storage(*node).uncertain=true;
    poison();throw std::runtime_error("Physical rows GL error uncertainty");}
   current(guard);guarded_first(guard);
  }
  void query_int(GLenum field,GLint* value,const FontTargetGuard* guard=nullptr){
   effect_fence(guard);api.GetIntegerv(field,value);effect_fence(guard);
  }
  void query_viewport(GLfloat* value,const FontTargetGuard* guard=nullptr){
   effect_fence(guard);render.GetFloati_v(GL_VIEWPORT,0,value);effect_fence(guard);
  }
  void query_depth(GLdouble* value,const FontTargetGuard* guard=nullptr){
   effect_fence(guard);render.GetDoublei_v(GL_DEPTH_RANGE,0,value);effect_fence(guard);
  }
  void client_extent(const FontTargetGuard* guard=nullptr){
   effect_fence(guard);RECT rect{};
   const BOOL ok=GetClientRect(reinterpret_cast<HWND>(target->native.window),&rect);
   effect_fence(guard);
   const auto width=std::int64_t(rect.right)-std::int64_t(rect.left);
   const auto height=std::int64_t(rect.bottom)-std::int64_t(rect.top);
   if(!ok||width!=geometry.viewport[2]||height!=geometry.viewport[3])
    throw std::runtime_error("Physical FBO0 drawable extent does not equal full viewport");
   guarded_first(guard);
  }
  void verify_tuple(bool active,const FontTargetGuard* guard=nullptr){
   // Caller performs LAST FULL first. No Source callbacks after this fresh tuple.
   guarded_first(guard);GLint framebuffer=-1,buffer=GL_NONE,origin=0,mode=0;
   std::array<GLfloat,4> viewport{};std::array<GLdouble,2> depth{};
   query_int(GL_DRAW_FRAMEBUFFER_BINDING,&framebuffer,guard);
   query_int(GL_DRAW_BUFFER0,&buffer,guard);
   query_viewport(viewport.data(),guard);query_depth(depth.data(),guard);
   query_int(GL_CLIP_ORIGIN,&origin,guard);query_int(GL_CLIP_DEPTH_MODE,&mode,guard);
   client_extent(guard);guarded_first(guard);
   const std::array<GLfloat,4> expected=active?
    std::array<GLfloat,4>{.5f,-.5f,baseline_viewport[2],baseline_viewport[3]}:baseline_viewport;
   if(framebuffer!=0||buffer!=geometry.draw_buffer||viewport!=expected||
      depth!=baseline_depth||origin!=(active?GL_LOWER_LEFT:baseline_clip_origin)||
      mode!=(active?GL_ZERO_TO_ONE:baseline_clip_depth))
    throw std::runtime_error("Physical rows current target tuple differs from sealed phase");
   guarded_first(guard);
  }
  void capture_baseline(){
   first();auto& saved=Channel::storage(*node);
   query_viewport(saved.viewport.data());saved.viewport_captured=true;
   query_depth(saved.depth.data());saved.depth_captured=true;
   query_int(GL_CLIP_ORIGIN,&saved.clip_origin);
   query_int(GL_CLIP_DEPTH_MODE,&saved.clip_depth_mode);saved.clip_captured=true;
   first();
   const auto w=GLfloat(geometry.viewport[2]),h=GLfloat(geometry.viewport[3]);
   if(double(w)!=double(geometry.viewport[2])||double(h)!=double(geometry.viewport[3])||
      saved.viewport!=std::array<GLfloat,4>{0,0,w,h}||
      (saved.clip_origin!=GL_LOWER_LEFT&&saved.clip_origin!=GL_UPPER_LEFT)||
      (saved.clip_depth_mode!=GL_NEGATIVE_ONE_TO_ONE&&saved.clip_depth_mode!=GL_ZERO_TO_ONE)||
      !std::isfinite(saved.depth[0])||!std::isfinite(saved.depth[1])||
      saved.depth[0]<0||saved.depth[0]>1||saved.depth[1]<0||saved.depth[1]>1)
    throw std::runtime_error("Physical rows baseline cannot be represented exactly");
   baseline_viewport=saved.viewport;baseline_depth=saved.depth;
   baseline_clip_origin=saved.clip_origin;baseline_clip_depth=saved.clip_depth_mode;
   verify_tuple(false);
  }
  void enter(){
   first();auto& saved=Channel::storage(*node);
   effect_fence();saved.clip_changed=true;
   render.ClipControl(GL_LOWER_LEFT,GL_ZERO_TO_ONE);effect_fence();
   effect_fence();saved.viewport_changed=true;
   render.ViewportIndexedf(0,.5f,-.5f,baseline_viewport[2],baseline_viewport[3]);effect_fence();
   // Indexed depth0 remains unchanged. This supplies observed DATA only and
   // cannot attest the original renderer's native viewport depth policy.
   verify_tuple(true);first();entered=true;
  }
  void require_active(){first();if(!entered)
    throw std::runtime_error("Physical rows phase not entered");full();verify_tuple(true);first();}
  void finish(){
   require_active();const auto status=Channel::restore(*node);first();
   if(!status.complete||status.cleanup_failure)
    throw std::runtime_error("Physical rows restoration incomplete/uncertain");
   node.reset();const auto drained=Channel::drain();first();
   if(drained.incomplete||drained.cleanup_failure||drained.pending_creator_nodes)
    throw std::runtime_error("Physical rows retirement drain not closed");
   // Independent baseline Source and fresh exact float/depth/clip/FBO tuple.
   full();verify_tuple(false);first();
   unsigned expected=1;
   if(!label_physical_rows_gate.compare_exchange_strong(expected,0))
    throw std::runtime_error("Physical rows gate poisoned before final commit");
   finished=true;entered=false;
  }
  // Restored phase only. The entered Rows FIRST is deliberately separate:
  // a finished transaction never becomes an active row adapter again.
  void restored_first()const{
   if(!alive.load()||!finished.load()||entered||node||label_physical_rows_gate.load()!=0)
    throw std::runtime_error("Physical rows completed restoration retired/changed");
   target_guard.require_current_without_callbacks();
   if(!target->historical)
    throw std::runtime_error("Restoration witness requires genuine selected historical target");
  }
  void restored_full(){
   restored_first();
   target->historical->geometry.scope_health.require_source_owned(*target->operation);restored_first();
   target->historical->root.require_source_owned(*target->operation);restored_first();
   target->historical->occurrence.require_source_owned(*target->operation);restored_first();
   target->historical->permission.require_source_owned(*target->operation);restored_first();
   target->metadata_guard.require_source_owned(*target->operation);restored_first();
  }
  void restored_effect_fence(){
   restored_first();(void)target->current_api();restored_first();
   const auto reset=api.GetGraphicsResetStatus();
   (void)target->current_api();restored_first();
   if(reset!=GL_NO_ERROR){poison();throw std::runtime_error("Restored rows GL reset uncertainty");}
   const auto error=api.GetError();
   (void)target->current_api();restored_first();
   if(error!=GL_NO_ERROR){poison();throw std::runtime_error("Restored rows GL error uncertainty");}
  }
  void restored_query_int(GLenum field,GLint* value){
   restored_effect_fence();api.GetIntegerv(field,value);restored_effect_fence();
  }
  void verify_restored_tuple_without_source_callbacks(){
   // LAST external FULL must precede this whole fresh tuple. Every native
   // query/status/current POST uses the closed restored phase only.
   restored_first();GLint framebuffer=-1,buffer=GL_NONE,origin=0,mode=0;
   std::array<GLfloat,4> viewport{};std::array<GLdouble,2> depth{};
   restored_query_int(GL_DRAW_FRAMEBUFFER_BINDING,&framebuffer);
   restored_query_int(GL_DRAW_BUFFER0,&buffer);
   restored_effect_fence();render.GetFloati_v(GL_VIEWPORT,0,viewport.data());restored_effect_fence();
   restored_effect_fence();render.GetDoublei_v(GL_DEPTH_RANGE,0,depth.data());restored_effect_fence();
   restored_query_int(GL_CLIP_ORIGIN,&origin);restored_query_int(GL_CLIP_DEPTH_MODE,&mode);
   restored_effect_fence();RECT extent{};
   const BOOL ok=GetClientRect(reinterpret_cast<HWND>(target->native.window),&extent);
   restored_effect_fence();
   const auto width=std::int64_t(extent.right)-std::int64_t(extent.left);
   const auto height=std::int64_t(extent.bottom)-std::int64_t(extent.top);
   if(!ok||width!=geometry.viewport[2]||height!=geometry.viewport[3]||framebuffer!=0||
      buffer!=geometry.draw_buffer||viewport!=baseline_viewport||depth!=baseline_depth||
      origin!=baseline_clip_origin||mode!=baseline_clip_depth)
    throw std::runtime_error("Physical rows fresh restored baseline tuple differs");
   restored_first();
  }
 };
 const std::shared_ptr<State> state;
 explicit LabelDefaultRowsAdapter(std::shared_ptr<State> actual):state(std::move(actual)){}
 ~LabelDefaultRowsAdapter()noexcept{
  state->alive=false;if(!state->finished.load())state->poison();
  // In-flight members retain State so native callbacks cannot delete its node.
 }
 struct Method final {
  const std::shared_ptr<State> saved;
  explicit Method(std::shared_ptr<State> actual):saved(std::move(actual)){
   try{saved->first();bool expected=false;
    if(!saved->method_active.compare_exchange_strong(expected,true))
     throw std::runtime_error("Physical rows recursive scope observation");
   }catch(...){saved->poison();throw;}
  }
  ~Method()noexcept{saved->method_active=false;}
 };
 struct RestoredMethod final {
  const std::shared_ptr<State> saved;
  explicit RestoredMethod(std::shared_ptr<State> actual):saved(std::move(actual)){
   try{saved->restored_first();bool expected=false;
    if(!saved->method_active.compare_exchange_strong(expected,true))
     throw std::runtime_error("Restored rows recursive scope observation");
   }catch(...){saved->poison();throw;}
  }
  ~RestoredMethod()noexcept{saved->method_active=false;}
 };
 graphics::detail::BnwLabelDefaultFontTargetLoan::RowsRestorationWitness
  finish_and_capture_restoration_owned(graphics::detail::BnwLabelDefaultFontTargetLoan::Guard);
 struct ObservedDepth final {std::array<GLdouble,2> saved,current;};
 void require_active_owned(){const auto actual=state;Method method(actual);
  try{actual->require_active();}catch(...){actual->poison();throw;}}
 ObservedDepth preserved_depth_owned(){const auto actual=state;Method method(actual);
  try{actual->require_active();const ObservedDepth value{actual->baseline_depth,actual->baseline_depth};
   actual->first();return value;}catch(...){actual->poison();throw;}}
 void finish_owned(){const auto actual=state;Method method(actual);
  try{actual->finish();}catch(...){actual->poison();throw;}}
};
std::unique_ptr<UiScreenGlRenderer::LabelDefaultRowsAdapter>
UiScreenGlRenderer::begin_label_default_rows_adapter_owned(const LabelDefaultFramebufferTarget& input){
 // Snapshot all actual Ui/target/dispatch relations before callbacks or allocation.
 const auto guard=input.capture_guard_without_callbacks();const auto target=guard.impl_;
 target->first();if(target->renderer!=this||!target->loaded)
  throw std::runtime_error("Physical rows requires SAME actual Ui target");
 const auto geometry=guard.baseline_geometry_without_callbacks();
 const auto render=gl_;const auto api=*target->loaded;target->first();
 if(!render.GetFloati_v||!render.GetDoublei_v||!render.ClipControl||
    !render.ViewportIndexedf||!render.DepthRangeIndexed)
  throw std::runtime_error("Physical rows loaded indexed dispatch incomplete");
 unsigned expected=0;
 if(!label_physical_rows_gate.compare_exchange_strong(expected,1)){
  label_physical_rows_gate.store(2);target->failed=true;
  throw std::runtime_error("Physical rows overlapping creator mutation refused");}
 std::shared_ptr<LabelDefaultRowsAdapter::State> state;
 std::unique_ptr<LabelDefaultRowsAdapter> result;
 try{
  target->first();const auto drain=LabelDefaultRowsAdapter::Channel::drain();target->first();
  if(label_physical_rows_gate.load()!=1||drain.incomplete||drain.cleanup_failure||drain.pending_creator_nodes)
   throw std::runtime_error("Physical rows prior pending/failed restoration refused");
  state=std::make_shared<LabelDefaultRowsAdapter::State>(guard,api,render,geometry);state->first();
  state->node=LabelDefaultRowsAdapter::Channel::make(target->pin,api,render);state->first();
  // Facade allocation BEFORE native changes; do not allocate after first mutation.
  result=std::unique_ptr<LabelDefaultRowsAdapter>(new LabelDefaultRowsAdapter(state));state->first();
  state->full();state->capture_baseline();state->enter();state->first();
  return result;
 }catch(...){label_physical_rows_gate.store(2);target->failed=true;
  if(state)state->alive=false;throw;}
}
// Defined in the real Ui translation unit after the complete Rows State.
// All Root-private guard operations remain Ui-nested member operations.
struct UiScreenGlRenderer::LabelDefaultFontTargetBridgeState final {
 using Loan=graphics::detail::BnwLabelDefaultFontTargetLoan;
 const std::shared_ptr<LabelDefaultRowsAdapter::State> rows;
 const detail::BnwLabelPaintPositionedData::Guard data;
 const std::optional<detail::BnwLabelPaintTextLoan::Guard> paint;
 const std::optional<detail::BnwLabelPaintPreparedFlushLoan::Guard> early;
 LabelDefaultFontTargetBridgeState(std::shared_ptr<LabelDefaultRowsAdapter::State> actual_rows,
  detail::BnwLabelPaintPositionedData::Guard actual_data,
  detail::BnwLabelPaintTextLoan::Guard actual_paint,
  detail::BnwLabelPaintPreparedFlushLoan::Guard actual_early)
  :rows(std::move(actual_rows)),data(std::move(actual_data)),paint(std::move(actual_paint)),
   early(std::move(actual_early)){}
 LabelDefaultFontTargetBridgeState(std::shared_ptr<LabelDefaultRowsAdapter::State> actual_rows,
  detail::BnwLabelPaintPositionedData::Guard actual_data)
  :rows(std::move(actual_rows)),data(std::move(actual_data)),paint(),early(){}
 static std::unique_ptr<Loan> capture_historical_owned(const LabelDefaultRowsAdapter&);
 void poison()const noexcept{rows->poison();}
 void restored_identity_first()const{
  rows->restored_first();
  rows->target->historical->occurrence.require_actual_positioned_data_without_callbacks(data);
  rows->restored_first();
 }
 void first()const{
  rows->first();if(!rows->entered)
   throw std::runtime_error("Font target bridge requires actual entered row phase");
  if(rows->target->historical){
   rows->target->historical->occurrence.require_actual_positioned_data_without_callbacks(data);
   rows->first();return;
  }
  if(!paint||!early||!rows->target->paint_guard)
   throw std::runtime_error("Active font bridge lacks original Paint/Early guards");
  data.require_current_without_callbacks();
  data.require_same_paint_guard_without_callbacks(*rows->target->paint_guard);
  data.require_actual_renderer_without_callbacks(*rows->target->renderer);
  paint->require_current_without_callbacks();
  data.require_same_paint_guard_without_callbacks(*paint);
  early->require_current_without_callbacks();early->require_same_paint_guard_without_callbacks(*paint);
  rows->first();data.require_current_without_callbacks();
 }
 void full(const Loan::Guard& guard)const{
  // Target-owned FULL only. Shader independently owns DATA/program Source
  // admission; do not hide this target fence inside grouped DATA/Early FULL.
  const auto both=[&]{first();guard.require_current_without_callbacks();first();};
  both();
  if(rows->target->historical){
   rows->target->historical->geometry.scope_health.require_source_owned(*rows->target->operation);both();
   rows->target->historical->root.require_source_owned(*rows->target->operation);both();
   rows->target->historical->occurrence.require_source_owned(*rows->target->operation);both();
   rows->target->historical->permission.require_source_owned(*rows->target->operation);both();
  }else{
   rows->target->frame_guard->require_source_owned(*rows->target->operation);both();
   rows->target->paint_guard->require_source_owned(*rows->target->operation);both();
  }
  rows->target->metadata_guard.require_source_owned(*rows->target->operation);both();
 }
 void same_data(detail::BnwLabelPaintPositionedData::Guard input)const{
  first();
  if(rows->target->historical){
   rows->target->historical->occurrence.require_actual_positioned_data_without_callbacks(input);
   if(input.impl_.get()!=data.impl_.get()||input.impl_.owner_before(data.impl_)||data.impl_.owner_before(input.impl_))
    throw std::runtime_error("Historical font target bridge has another original DATA control");
   first();return;
  }
  input.require_current_without_callbacks();
  if(input.impl_.get()!=data.impl_.get()||input.impl_.owner_before(data.impl_)||
     data.impl_.owner_before(input.impl_))
   throw std::runtime_error("Font target bridge belongs to another actual positioned DATA");
  input.require_same_paint_guard_without_callbacks(*rows->target->paint_guard);
  input.require_current_without_callbacks();first();
 }
 Loan::CapturedTargetData captured_data()const{
  first();const Loan::CapturedTargetData value{{rows->geometry.viewport[2],rows->geometry.viewport[3]},
   rows->baseline_depth};first();return value;
 }
};
} // namespace civ5::ui
#include "bnw_original_resource0_gpu_image_upload_core.inc"
#include "bnw_original_vertex_declaration_device_create_core.inc"
namespace civ5::graphics::detail {
struct BnwLabelDefaultFontTargetLoan::Impl final {
 const std::shared_ptr<::civ5::ui::UiScreenGlRenderer::LabelDefaultFontTargetBridgeState> saved;
 std::atomic<bool> alive{true};
 explicit Impl(std::shared_ptr<::civ5::ui::UiScreenGlRenderer::LabelDefaultFontTargetBridgeState> actual)
  :saved(std::move(actual)){}
 void first()const{
  if(!alive.load())throw std::runtime_error("Font target bridge lexical facade retired");
  saved->first();
  if(!alive.load())throw std::runtime_error("Font target bridge retired during pure guard check");
 }
 void poison()const noexcept{saved->poison();}
};
struct BnwLabelDefaultFontTargetLoan::RowsRestorationWitness::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<BnwLabelDefaultFontTargetLoan::Impl> original_font_target;
 const std::shared_ptr<::civ5::ui::UiScreenGlRenderer::LabelDefaultRowsAdapter::State> rows;
 bool sealed=false;
 Impl(std::shared_ptr<BnwLabelDefaultFontTargetLoan::Impl> original,
  std::shared_ptr<::civ5::ui::UiScreenGlRenderer::LabelDefaultRowsAdapter::State> actual_rows)
  :original_font_target(std::move(original)),rows(std::move(actual_rows)){}
 void first()const{
  if(creator!=std::this_thread::get_id()||!sealed||!original_font_target||!rows||
     !original_font_target->saved||original_font_target->saved->rows.get()!=rows.get()||
     original_font_target->saved->rows.owner_before(rows)||rows.owner_before(original_font_target->saved->rows))
   throw std::runtime_error("Restoration witness original FontTarget/Rows control differs");
  // Retained Loan storage is IDENTITY only. Its retired alive bit and old
  // entered/current/source methods never establish restored health here.
  original_font_target->saved->restored_identity_first();
 }
 void poison()const noexcept{if(rows)rows->poison();}
};
BnwLabelDefaultFontTargetLoan::RowsRestorationWitness::RowsRestorationWitness(
 const IssuerKey&,std::shared_ptr<const Impl> actual)noexcept:impl_(std::move(actual)){}
BnwLabelDefaultFontTargetLoan::RowsRestorationWitness::~RowsRestorationWitness()=default;
void BnwLabelDefaultFontTargetLoan::RowsRestorationWitness::require_current_without_callbacks()const{
 const auto saved=impl_;try{saved->first();}catch(...){if(saved)saved->poison();throw;}
}
void BnwLabelDefaultFontTargetLoan::RowsRestorationWitness::require_same_original_font_target_without_callbacks(
 BnwLabelDefaultFontTargetLoan::Guard original)const{
 const auto saved=impl_;const auto identity=original.impl_;
 try{
  saved->first();
  if(identity.get()!=saved->original_font_target.get()||identity.owner_before(saved->original_font_target)||
     saved->original_font_target.owner_before(identity))
   throw std::runtime_error("Restoration witness belongs to another original FontTarget control");
  saved->first();
 }catch(...){if(saved)saved->poison();throw;}
}
void BnwLabelDefaultFontTargetLoan::RowsRestorationWitness::require_source_owned(
 const ::civ5::runtime::BnwWorldViewSourceOperationLoan& input)const{
 const auto saved=impl_;const auto*const operation=&input;
 try{
  saved->first();
  if(operation!=saved->rows->target->operation)
   throw std::runtime_error("Restoration witness another actual Source operation");
  ::civ5::ui::UiScreenGlRenderer::LabelDefaultRowsAdapter::RestoredMethod method(saved->rows);
  saved->first();saved->rows->restored_full();saved->first();
  saved->rows->verify_restored_tuple_without_source_callbacks();saved->first();
 }catch(...){if(saved)saved->poison();throw;}
}
BnwLabelDefaultFontTargetLoan::BnwLabelDefaultFontTargetLoan(
 const IssuerKey&,std::shared_ptr<Impl> actual)noexcept:impl_(std::move(actual)){}
BnwLabelDefaultFontTargetLoan::~BnwLabelDefaultFontTargetLoan()noexcept{impl_->alive=false;}
BnwLabelDefaultFontTargetLoan::Guard::Guard(std::shared_ptr<Impl> actual)noexcept:impl_(std::move(actual)){}
BnwLabelDefaultFontTargetLoan::Guard BnwLabelDefaultFontTargetLoan::capture_guard_without_callbacks()const{
 const auto actual=impl_;
 try{actual->first();return Guard(actual);}
 catch(...){actual->poison();throw;}
}
void BnwLabelDefaultFontTargetLoan::Guard::require_current_without_callbacks()const{
 const auto actual=impl_;try{actual->first();}catch(...){actual->poison();throw;}
}
void BnwLabelDefaultFontTargetLoan::Guard::require_source_owned(
 const ::civ5::runtime::BnwWorldViewSourceOperationLoan& input)const{
 const auto actual=impl_;const auto guard=*this;const auto*const operation=&input;
 try{
  actual->first();if(actual->saved->rows->target->operation!=operation)
   throw std::runtime_error("Font target bridge another actual Source operation");
  ::civ5::ui::UiScreenGlRenderer::LabelDefaultRowsAdapter::Method method(actual->saved->rows);
  actual->first();actual->saved->full(guard);actual->first();
 }catch(...){actual->poison();throw;}
}
void BnwLabelDefaultFontTargetLoan::Guard::require_active_owned()const{
 // Capture Impl and Guard VALUE before any Source or native observation.
 const auto actual=impl_;const auto guard=*this;
 try{
  actual->first();
  ::civ5::ui::UiScreenGlRenderer::LabelDefaultRowsAdapter::Method method(actual->saved->rows);
  actual->first();actual->saved->full(guard);actual->first();
  actual->saved->rows->verify_tuple(true,&guard);actual->first();
 }catch(...){actual->poison();throw;}
}
BnwLabelDefaultFontTargetLoan::CapturedTargetData
BnwLabelDefaultFontTargetLoan::Guard::captured_target_data_without_callbacks()const{
 const auto actual=impl_;
 try{actual->first();const auto value=actual->saved->captured_data();actual->first();return value;}
 catch(...){actual->poison();throw;}
}
void BnwLabelDefaultFontTargetLoan::Guard::require_same_positioned_data_without_callbacks(
 ::civ5::ui::detail::BnwLabelPaintPositionedData::Guard input)const{
 const auto actual=impl_;
 try{actual->first();actual->saved->same_data(std::move(input));actual->first();}
 catch(...){actual->poison();throw;}
}
} // namespace civ5::graphics::detail
namespace civ5::ui {
graphics::detail::BnwLabelDefaultFontTargetLoan::RowsRestorationWitness
UiScreenGlRenderer::LabelDefaultRowsAdapter::finish_and_capture_restoration_owned(
 graphics::detail::BnwLabelDefaultFontTargetLoan::Guard original_guard){
 using Loan=graphics::detail::BnwLabelDefaultFontTargetLoan;using Witness=Loan::RowsRestorationWitness;
 // LAST facade reads: retain genuine opaque Loan IDENTITY and actual Rows
 // before all Source/allocator/native work. Caller closes target_loan first.
 const auto actual=state;const auto original=original_guard.impl_;
 Method method(actual);
 try{
  const auto original_identity=[&]{
   actual->first();
   if(!original||!original->saved||original->saved->rows.get()!=actual.get()||
      original->saved->rows.owner_before(actual)||actual.owner_before(original->saved->rows)||
      !actual->target->historical)
    throw std::runtime_error("Rows restoration issuer lacks SAME original historical FontTarget identity");
   actual->first();
  };
  original_identity();actual->full();original_identity();
  // Allocate the unsealed OWN control before actual finish's final native
  // observations. No unsealed Witness or key is returned to any consumer.
  auto owned=std::make_shared<Witness::Impl>(original,actual);
  original_identity();actual->full();original_identity();
  actual->finish(); //UNCHANGED real restore/drain/FULL/fresh tuple/gate commit
  actual->restored_first();actual->restored_full();actual->restored_first();
  actual->verify_restored_tuple_without_source_callbacks();actual->restored_first();
  owned->sealed=true;owned->first();
  const Loan::IssuerKey key;const Witness result(key,owned);
  result.require_same_original_font_target_without_callbacks(original_guard);
  result.require_current_without_callbacks();return result;
 }catch(...){actual->poison();throw;}
}

std::unique_ptr<graphics::detail::BnwLabelDefaultFontTargetLoan>
UiScreenGlRenderer::capture_label_default_font_target_owned(
 const LabelDefaultRowsAdapter& input,const detail::BnwLabelPaintPositionedData& positioned){
 using Loan=graphics::detail::BnwLabelDefaultFontTargetLoan;
 // LAST facade reads; no raw this/input/positioned access after allocation.
 const auto rows=input.state;
 std::shared_ptr<Loan::Impl> actual;
 std::unique_ptr<Loan> result;
 try{
  rows->first();if(rows->target->renderer!=this)
   throw std::runtime_error("Font target issuer is another Ui");
  if(rows->target->historical||!rows->target->paint_guard)
   throw std::runtime_error("Active font target issuer requires original active Paint association");
  const auto data=positioned.capture_guard_without_callbacks();
  const auto paint=data.captured_paint_guard_without_callbacks();
  const auto early=data.captured_early_flush_guard_without_callbacks();
  LabelDefaultRowsAdapter::Method method(rows);
  const auto pure=[&]{rows->first();data.require_current_without_callbacks();
   data.require_same_paint_guard_without_callbacks(*rows->target->paint_guard);
   data.require_actual_renderer_without_callbacks(*rows->target->renderer);rows->first();};
  pure();auto saved=std::make_shared<LabelDefaultFontTargetBridgeState>(rows,data,paint,early);pure();
  saved->first();actual=std::make_shared<Loan::Impl>(std::move(saved));pure();actual->first();
  result=std::unique_ptr<Loan>(new Loan(Loan::IssuerKey{},actual));pure();actual->first();
  const auto guard=result->capture_guard_without_callbacks();
  // No allocation or facade reads after LAST FULL and terminal fresh tuple.
  actual->saved->full(guard);actual->first();rows->verify_tuple(true,&guard);actual->first();
  return result;
 }catch(...){rows->poison();if(actual)actual->alive=false;throw;}
}
std::unique_ptr<UiScreenGlRenderer::LabelDefaultFramebufferTarget>
UiScreenGlRenderer::capture_label_default_framebuffer_target_owned(const LabelPaintCapture& capture){
 std::shared_ptr<LabelDefaultFramebufferTarget::Impl> state;
 std::unique_ptr<LabelDefaultFramebufferTarget> result;
 {
 // Every facade/outer value read is captured before any allocation/native FULL.
 const auto life=worldview_lifetime_;const auto*const renderer=this;const auto*const context=&context_;
 if(!life->alive||life->failed||!life->active||life->creator!=std::this_thread::get_id()||
    life->renderer!=renderer||!life->active_scope)
  throw std::runtime_error("Default target issuer lacks actual Ui scope");
 const auto*const scope=static_cast<const WorldViewPaintScope*>(life->active_scope);
 const auto*const item=scope->label_item;
 if(!item||!item->alive||!capture.frame||bool(capture.run)==bool(capture.standalone_run)||!capture.paint||!capture.data)
  throw std::runtime_error("Default target issuer lacks real item/CPU capture custody");
 const auto frame_guard=capture.frame->capture_guard_without_callbacks();
 const auto paint_guard=capture.paint->capture_guard_without_callbacks();
 const auto embedded_run=capture.run;const auto standalone_run=capture.standalone_run;
 frame_guard.require_current_without_callbacks();paint_guard.require_current_without_callbacks();
 const LabelDefaultFramebufferTarget::ItemSnapshot snapshot{item->frame,item->publication,
  item->operation,item->serial,item->item,item->index,item->destination,item->clip,item->viewport};
 const auto*const operation=snapshot.operation;
 const auto*const publication=item->publication;
 const auto tree=publication->tree_;
 const auto id=publication->actual_checked_control_id_for_paint_without_callbacks();
 frame_guard.require_actual_control_without_callbacks(*tree,id);
 frame_guard.require_actual_paint_item_without_callbacks(*publication,snapshot.index);
 std::optional<detail::BnwPositionedLabelRunLoan::Guard> run_guard;
 std::optional<detail::BnwStandalonePositionedLabelRunLoan::Guard> standalone_guard;
 std::shared_ptr<const detail::BnwLabelSystemFontBorrower> system;
 if(standalone_run){
  // Official standalone capture may run Source callbacks. ALL envelope/item
  // and Ui native identities were captured above, before this call.
  standalone_guard.emplace(standalone_run->capture_guard_for_paint_frame_guard_owned(
   *operation,tree,id,frame_guard));
  frame_guard.require_current_without_callbacks();paint_guard.require_current_without_callbacks();
  frame_guard.require_actual_paint_item_without_callbacks(*publication,snapshot.index);
  system=standalone_guard->actual_system_font_without_callbacks();
  standalone_guard->require_current_without_callbacks();
 }else{
  run_guard.emplace(embedded_run->capture_guard_for_paint_owned(*operation,tree,id,frame_guard));
  frame_guard.require_current_without_callbacks();paint_guard.require_current_without_callbacks();
  system=run_guard->actual_system_font_without_callbacks();run_guard->require_current_without_callbacks();
 }
 frame_guard.require_current_without_callbacks();paint_guard.require_current_without_callbacks();
 if(!system)throw std::runtime_error("Default target needs actual positive170 borrower");
 paint_guard.require_system_font_borrower_without_callbacks(*system);
 const auto metadata=system->atlas_metadata_without_callbacks();
 if(!metadata)throw std::runtime_error("Default target canonical metadata absent");
 const auto metadata_guard=metadata->capture_guard_without_callbacks();
 const auto metadata_data=metadata_guard.impl_;
 const auto pin=metadata_data->context_pin.lock();
 if(!pin)throw std::runtime_error("Default target SAME counted Context Pin absent");
 pin->first_without_callbacks();
 if(&pin->context_reference_without_callbacks()!=context||metadata_data->renderer!=renderer)
  throw std::runtime_error("Default target canonical Ui Context/renderer differs");
 const auto native=metadata_data->native;
 const auto window=metadata_data->window;
 const auto first=[&]{frame_guard.require_current_without_callbacks();
  frame_guard.require_actual_paint_item_without_callbacks(*publication,snapshot.index);
  paint_guard.require_current_without_callbacks();
  if(run_guard)run_guard->require_current_without_callbacks();
  if(standalone_guard)standalone_guard->require_current_without_callbacks();
  metadata_guard.require_current_without_callbacks();pin->first_without_callbacks();};first();
 std::unique_ptr<LabelDefaultFramebufferTarget::Impl> provisional(
  new LabelDefaultFramebufferTarget::Impl(life,renderer,scope,item,snapshot,frame_guard,paint_guard,
   metadata_guard,pin,native,window));first();provisional->full();first();
 state=std::shared_ptr<LabelDefaultFramebufferTarget::Impl>(std::move(provisional));
 first();state->full();first();state->loaded=state->current_api();state->verify_default_owned(true);
 result=std::unique_ptr<LabelDefaultFramebufferTarget>(new LabelDefaultFramebufferTarget(state));
 first();state->full();first();state->verify_default_owned(false);first();
 }
 // All local facade/system/metadata/tree/pin snapshots are destroyed before POST.
 state->full();state->verify_default_owned(false);state->first();
 return result;
}

// Closed consumer only: every authority input is already issued by its genuine
// owner. There is no Root/Item/drain producer or manager dispatch in this body.
std::unique_ptr<UiScreenGlRenderer::LabelDefaultFramebufferTarget>
UiScreenGlRenderer::LabelDefaultFramebufferTarget::capture_historical_owned(
 UiScreenGlRenderer& renderer,const LabelPaintCapture& original,
 const detail::BnwNormalRootBatchHistory::Guard& actual_root,
 const detail::BnwNormalItemOccurrence::Guard& actual_item,
 const detail::BnwNormalQueueDrainPermission::Guard& actual_permission,
 const graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity& actual_record,
 const graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt& actual_descriptor){
 if(!original.historical_target_geometry||!original.metadata||!original.pin)
  throw std::runtime_error("Historical target lacks producer-captured geometry/metadata/Pin");
 // Snapshot original custody and already-issued VALUE controls before effects.
 const auto geometry=*original.historical_target_geometry;
 if(bool(geometry.destination.caption)!=bool(original.caption_fp32_rect)||
    (original.caption_fp32_rect&&(geometry.destination.caption.get()!=original.caption_fp32_rect->owner.get()||
      geometry.destination.caption.owner_before(original.caption_fp32_rect->owner)||
      original.caption_fp32_rect->owner.owner_before(geometry.destination.caption)||
      !geometry.destination.words||*geometry.destination.words!=original.caption_fp32_rect->words)))
  throw std::runtime_error("Historical target lost original caption rect snapshot");
 const auto metadata=*original.metadata;const auto pin=original.pin;
 const auto root=actual_root;const auto occurrence=actual_item;
 const auto permission=actual_permission;const auto record=actual_record;const auto descriptor=actual_descriptor;
 const auto life=renderer.worldview_lifetime_;const auto*const actual_renderer=&renderer;
 geometry.scope_health.require_physical_current_without_callbacks();
 geometry.scope_health.require_actual_renderer_without_callbacks(*actual_renderer);
 const auto*const scope=renderer.worldview_scope_without_callbacks();
 if(!scope)throw std::runtime_error("Historical target actual root scope is absent");
 const auto*const operation=&scope->operation;const auto serial=scope->serial;
 metadata.require_current_without_callbacks();
 const auto native=metadata.impl_->native;const auto window=metadata.impl_->window;
 const auto physical=[&]{
  geometry.scope_health.require_physical_current_without_callbacks();
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||!life->active||
     life->renderer!=actual_renderer||life->active_scope!=scope)
   throw std::runtime_error("Historical target actual renderer/root scope changed");
  root.require_physical_current_without_callbacks();root.require_actual_renderer_without_callbacks(*actual_renderer);
  occurrence.require_physical_current_without_callbacks();occurrence.require_same_root_without_callbacks(root);
  occurrence.require_actual_publication_without_callbacks(*geometry.publication);
  occurrence.require_actual_positioned_data_without_callbacks(geometry.positioned_data);
  permission.require_physical_current_without_callbacks();permission.require_same_root_without_callbacks(root);
  permission.require_queued_record_without_callbacks(occurrence,record,descriptor);
  metadata.require_current_without_callbacks();pin->first_without_callbacks();
  if(metadata.impl_->renderer!=actual_renderer||&scope->operation!=operation||scope->serial!=serial)
   throw std::runtime_error("Historical target actual metadata/Source identity changed");
 };
 physical();
 auto association=std::make_shared<const HistoricalAssociation>(geometry,root,occurrence,permission,record,descriptor);
 physical();association->first(*actual_renderer);
 const auto full=[&]{physical();geometry.scope_health.require_source_owned(*operation);physical();
  root.require_source_owned(*operation);physical();occurrence.require_source_owned(*operation);physical();
  permission.require_source_owned(*operation);physical();metadata.require_source_owned(*operation);physical();};
 full();association->first(*actual_renderer);
 auto state=std::make_shared<Impl>(life,actual_renderer,scope,operation,serial,association,metadata,pin,native,window);
 physical();state->first();state->full();state->first();
 auto result=std::unique_ptr<LabelDefaultFramebufferTarget>(new LabelDefaultFramebufferTarget(state));
 physical();state->first();state->loaded=state->current_api();state->first();
 state->verify_default_owned(true);state->first();state->full();state->verify_default_owned(false);state->first();
 return result;
}

std::unique_ptr<graphics::detail::BnwLabelDefaultFontTargetLoan>
UiScreenGlRenderer::LabelDefaultFontTargetBridgeState::capture_historical_owned(
 const LabelDefaultRowsAdapter& input){
 using Loan=graphics::detail::BnwLabelDefaultFontTargetLoan;
 const auto rows=input.state;rows->first();
 if(!rows->target->historical)
  throw std::runtime_error("Historical font target requires sealed selected-entry target association");
 const auto data=rows->target->historical->geometry.positioned_data;
 const auto pure=[&]{rows->first();
  rows->target->historical->occurrence.require_actual_positioned_data_without_callbacks(data);
  rows->first();};
 LabelDefaultRowsAdapter::Method method(rows);
 std::shared_ptr<Loan::Impl> actual;
 try{
  pure();auto saved=std::make_shared<LabelDefaultFontTargetBridgeState>(rows,data);pure();saved->first();
  actual=std::make_shared<Loan::Impl>(std::move(saved));pure();actual->first();
  auto result=std::unique_ptr<Loan>(new Loan(Loan::IssuerKey{},actual));pure();actual->first();
  const auto guard=result->capture_guard_without_callbacks();
  actual->saved->full(guard);actual->first();rows->verify_tuple(true,&guard);actual->first();
  return result;
 }catch(...){rows->poison();if(actual)actual->alive=false;throw;}
}

// Executable consumer provider. The genuine Root owner alone chooses when its
// completed traversal may enter this drain; this provider never issues that cut.
std::shared_ptr<const BnwWorldViewControlTree::NormalQueueConsumer>
UiScreenGlRenderer::WorldViewPaintScope::capture_normal_queue_consumer_owned(
 UiScreenGlRenderer& actual_renderer)const{
 const auto saved_life=actual_renderer.worldview_lifetime_;
 const auto*const scope=this;auto*const renderer=&actual_renderer;
 const auto origin=[&]{
  if(saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
     !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
   throw std::runtime_error("Normal manager consumer original Ui scope retired");
 };
 origin();scope->pure();origin();
 const auto tree=scope->normal_root_tree;
 const auto*const execution=scope->normal_root_execution.get();
 if(!tree||!execution)throw std::runtime_error("Normal manager consumer original execution absent");
 const auto root=execution->captured_root_without_callbacks();origin();
 const auto*const operation=&scope->operation;
 const auto first=[&]{origin();scope->pure();origin();
  if(scope->normal_root_execution.get()!=execution||scope->normal_root_tree.get()!=tree.get()||
     scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree))
   throw std::runtime_error("Normal manager consumer original Root replaced");
  root.require_physical_current_without_callbacks();root.require_actual_renderer_without_callbacks(*renderer);
  origin();};
 const auto full=[&]{first();scope->full();first();root.require_source_owned(*operation);first();};
 full();
 auto result=std::make_shared<const BnwWorldViewControlTree::NormalQueueConsumer>(
  [saved_life,scope,renderer,tree,root,operation,execution](
   const runtime::BnwWorldViewSourceOperationLoan& actual_operation,
   const detail::BnwNormalRootBatchHistory::Guard& actual_root,
   const detail::BnwNormalItemOccurrence::Guard& actual_item,
   const detail::BnwNormalQueueDrainPermission::Guard& actual_permission,
   const detail::BnwNormalLabelMaterialLoan::ShaderAccess& material,
   graphics::detail::BnwNormalLabelDrawRecordBinding::Guard actual_binding,
   graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity actual_record,
   graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt actual_descriptor)
   ->detail::BnwNormalQueueRecordCompletion{
   // Copy genuine VALUE controls before Source/native/allocator callbacks.
   const auto selected_root=actual_root;const auto item=actual_item;
   const auto permission=actual_permission;const auto binding=actual_binding;
   const auto record=actual_record;const auto descriptor=actual_descriptor;
   const auto origin=[&]{
    if(saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
       !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
     throw std::runtime_error("Normal selected consumer original Ui scope retired");
   };
   const auto first=[&]{origin();scope->pure();origin();
    if(&actual_operation!=operation||&scope->operation!=operation||
       scope->normal_root_execution.get()!=execution||scope->normal_root_tree.get()!=tree.get()||
       scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree))
     throw std::runtime_error("Normal selected consumer actual Source/Root substituted");
    selected_root.require_physical_current_without_callbacks();
    selected_root.require_same_root_without_callbacks(root);
    selected_root.require_actual_renderer_without_callbacks(*renderer);
    item.require_physical_current_without_callbacks();item.require_same_root_without_callbacks(selected_root);
    item.require_actual_renderer_without_callbacks(*renderer);
    permission.require_physical_current_without_callbacks();permission.require_same_root_without_callbacks(selected_root);
    permission.require_actual_renderer_without_callbacks(*renderer);
    permission.require_queued_record_without_callbacks(item,record,descriptor);origin();};
   const auto full=[&]{first();scope->full();first();
    selected_root.require_source_owned(*operation);first();
    item.require_source_owned(*operation);first();permission.require_source_owned(*operation);first();};
   try{
    first();
    // Select by the earned immutable Item control. Custody-vector position is
    // never a native outer command/inner draw-entry association or selector.
    std::shared_ptr<const NormalLabelCpuCapture> original;
    for(const auto& candidate:scope->normal_label_cpu_captures){
     first();if(!candidate)throw std::runtime_error("Normal original CPU custody absent");
     if(candidate->occurrence.impl_.get()==item.impl_.get()&&
        !candidate->occurrence.impl_.owner_before(item.impl_)&&
        !item.impl_.owner_before(candidate->occurrence.impl_)){
      if(original)throw std::runtime_error("Normal original Item custody duplicated");
      original=candidate;
     }
    }
    first();
    if(!original||!original->original_label||!original->original_programs||
       !original->original_programs->programs||!original->original_vertices||!original->original_binding||
       original->original_programs->label.get()!=original->original_label.get()||
       original->original_programs->label.owner_before(original->original_label)||
       original->original_label.owner_before(original->original_programs->label))
     throw std::runtime_error("Normal selected Item lost SAME original CPU facades");
    original->root.require_same_root_without_callbacks(selected_root);
    original->occurrence.require_same_item_without_callbacks(item);first();
    if(!original->original_label->historical_target_geometry)
     throw std::runtime_error("Normal selected Item immutable target geometry absent");
    item.require_actual_positioned_data_without_callbacks(
     original->original_label->historical_target_geometry->positioned_data);first();full();
    // All original CPU facades survive through transport and restoration.
    // Synchronous Material reference belongs to this exact Root dispatch only.
    auto target=LabelDefaultFramebufferTarget::capture_historical_owned(
     *renderer,*original->original_label,selected_root,item,permission,record,descriptor);first();
    if(!target)throw std::runtime_error("Normal selected historical Target absent");
    auto rows=renderer->begin_label_default_rows_adapter_owned(*target);first();
    if(!rows)throw std::runtime_error("Normal selected authentic Rows absent");
    auto target_loan=LabelDefaultFontTargetBridgeState::capture_historical_owned(*rows);first();
    if(!target_loan)throw std::runtime_error("Normal selected FontTarget loan absent");
    const auto original_target_guard=target_loan->capture_guard_without_callbacks();first();
    auto staging=graphics::GlBnwFontShader::stage_queued_record_owned(
     *operation,*renderer,selected_root,item,permission,binding,record,descriptor);first();
    if(!staging)throw std::runtime_error("Normal selected physical staging lease absent");
    const auto transport=graphics::GlBnwFontShader::submit_queued_record_owned(
     *operation,*renderer,*staging,original_target_guard,material);first();
    // Retired Loan is IDENTITY only. Its actual Rows/Target facades remain
    // alive until Shader accepts the closed successful restored-phase witness.
    target_loan.reset();
    const auto restored=rows->finish_and_capture_restoration_owned(original_target_guard);first();
    auto completion=graphics::GlBnwFontShader::complete_queued_record_after_rows_owned(
     *operation,*renderer,*staging,transport,restored);first();
    return completion; // Root advances the real cursor only after RETURN.
   }catch(...){
    saved_life->failed=true;throw; // Native failure has no healthy completion.
   }
  });
 first();full();return result;
}

void UiScreenGlRenderer::WorldViewPaintScope::drain_normal_queue_after_root_return_owned(
 UiScreenGlRenderer& actual_renderer)const{
 const auto saved_life=actual_renderer.worldview_lifetime_;
 const auto*const scope=this;auto*const renderer=&actual_renderer;
 const auto origin=[&]{
  if(saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
     !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
   throw std::runtime_error("Normal manager drain original Ui scope retired");
 };
 origin();scope->pure();origin();
 const auto tree=scope->normal_root_tree;const auto*const execution=scope->normal_root_execution.get();
 if(!tree||!execution)throw std::runtime_error("Normal manager drain original execution absent");
 const auto*const operation=&scope->operation;
 const auto consumer=scope->capture_normal_queue_consumer_owned(*renderer);origin();scope->pure();origin();
 if(scope->normal_root_execution.get()!=execution||scope->normal_root_tree.get()!=tree.get()||
    scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree))
  throw std::runtime_error("Normal manager drain original execution substituted");
 try{
  // Actual Root25 owns complete-root/registration/current-entry permission and
  // the once-only manager drain boundary. Ui supplies no completion flag/key.
  tree->drain_normal_root_batch_owned(*operation,*execution,consumer);
  origin();scope->pure();origin();
 }catch(...){saved_life->failed=true;throw;}
}

// Genuine embedded low-level producer, consuming the already-issued original
// caption Label Frame and Run. This never calls a Frame or Run issuer again.
void UiScreenGlRenderer::prepare_current_normal_button_caption_cpu_item_owned(
 std::shared_ptr<NormalButtonCaptionCapture> supplied){
 const auto saved_life=worldview_lifetime_;auto*const renderer=this;const auto child=std::move(supplied);
 const auto origin=[&]{if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
  !saved_life->active||saved_life->renderer!=renderer||!saved_life->active_scope)
  throw std::runtime_error("Caption embedded CPU original Ui scope retired");};
 origin();auto*const scope=const_cast<WorldViewPaintScope*>(static_cast<const WorldViewPaintScope*>(saved_life->active_scope));
 if(!child||!child->entered_frame||!child->entered_run||!child->child_publication||!child->child_frame||
    !child->emission||!child->append_index||!child->matrix||!child->original_tree||!child->fp32_rect||!child->fp32_rect_words)
  throw std::runtime_error("Caption embedded CPU lacks original issued Frame/Run/append custody");
 const auto actual_frame=child->entered_frame;const auto run=child->entered_run;
 const auto publication=child->child_publication;const auto frame_owner=child->child_frame;
 const auto tree=child->original_tree;const auto matrix=child->matrix;
 const auto rect=child->fp32_rect;const auto rect_words=*child->fp32_rect_words;
 const auto caption=child->caption;const auto common=child->common;const auto root=child->root;
 const auto index=*child->append_index;const auto*const operation=&scope->operation;
 const auto*const execution=scope->normal_root_execution.get();
 const auto frame=actual_frame->capture_guard_without_callbacks();origin();
 const auto id=publication->actual_checked_control_id_for_paint_without_callbacks();origin();
 const auto*const stamp=scope->label_item;
 // Independent original owners precede all transient producer guards/views.
 std::shared_ptr<LabelPaintCapture> retained;
 std::unique_ptr<graphics::GlBnwFontShader::PositionedVertices> vertices;
 std::unique_ptr<LabelNativeProgramCapture> programs;
 std::unique_ptr<graphics::detail::BnwNormalLabelDrawRecordBinding> binding;
 std::optional<detail::BnwPositionedLabelRunLoan::Guard> run_guard;
 std::optional<detail::BnwLabelPaintTextLoan::Guard> paint_guard;
 std::optional<detail::BnwLabelPaintPositionedData::Guard> data_guard;
 std::optional<graphics::detail::BnwLabelSystemFontAtlasMetadata::Guard> metadata;
 std::optional<graphics::detail::BnwNormalLabelDrawRecordBinding::Guard> binding_guard;
 std::optional<detail::BnwNormalItemOccurrence::Guard> item;
 std::shared_ptr<const detail::BnwLabelSystemFontBorrower> system;
 std::shared_ptr<graphics::GlBnwSystemFontAtlas> atlas;
 std::shared_ptr<const graphics::detail::BnwRendererContextPin> pin;
 const auto first=[&]{
  origin();if(saved_life->active_scope!=scope)throw std::runtime_error("Caption embedded CPU lexical scope changed");
  caption.require_current_without_callbacks();common.require_current_without_callbacks();
  caption.require_same_common_call_without_callbacks(common);origin();scope->pure();origin();
  rect->require_current_without_callbacks();rect->require_same_caption_without_callbacks(caption);
  rect->require_same_initialized_run_without_callbacks(run);
  if(rect->raw_words_without_callbacks()!=rect_words||child->fp32_rect.get()!=rect.get()||
     child->fp32_rect.owner_before(rect)||rect.owner_before(child->fp32_rect)||
     !child->fp32_rect_words||*child->fp32_rect_words!=rect_words)
   throw std::runtime_error("Caption embedded CPU SAME FP32 rect snapshot changed");origin();
  if(!execution||scope->normal_root_execution.get()!=execution||scope->normal_root_tree.get()!=tree.get()||
     scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree)||
     scope->normal_button_caption.get()!=child.get()||scope->normal_button_caption.owner_before(child)||child.owner_before(scope->normal_button_caption)||
     scope->normal_context_root_frame.get()!=child->parent_actual_frame.get()||
     scope->normal_context_root_frame.owner_before(child->parent_actual_frame)||child->parent_actual_frame.owner_before(scope->normal_context_root_frame)||
     scope->publication!=publication.get()||scope->frame!=frame_owner.get()||!scope->painting||
     !stamp||scope->label_item!=stamp||!stamp->alive||stamp->scope!=scope||stamp->publication!=publication.get()||
     stamp->frame!=frame_owner.get()||stamp->index!=index||stamp->operation!=operation||stamp->serial!=scope->serial)
   throw std::runtime_error("Caption embedded CPU SAME original item/Root/parent changed");
  root.require_physical_current_without_callbacks();root.require_same_root_without_callbacks(execution->captured_root_without_callbacks());
  root.require_actual_renderer_without_callbacks(*renderer);root.require_actual_matrix_without_callbacks(*matrix);
  frame.require_current_without_callbacks();frame.require_actual_control_without_callbacks(*tree,id);
  frame.require_actual_paint_item_without_callbacks(*publication,index);publication->require_adjusted_frame_owner_without_callbacks(frame_owner);origin();
  if(run_guard){run_guard->require_current_without_callbacks();rect->require_same_entered_run_without_callbacks(*run_guard);origin();}
  const auto require_rect=[&](const std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>& actual){
   if(actual.get()!=rect.get()||actual.owner_before(rect)||rect.owner_before(actual))
    throw std::runtime_error("Caption Paint/DATA canonical rect control changed");
  };
  const auto indexed_rect=publication->caption_fp32_rect_for_actual_item_without_callbacks(index,frame);
  require_rect(indexed_rect);stamp->destination.require_item(*stamp->item);
  if(!stamp->destination.caption||!stamp->destination.words||*stamp->destination.words!=rect_words)
   throw std::runtime_error("Caption entered stamp lacks SAME captured FP32 words");
  if(paint_guard){paint_guard->require_current_without_callbacks();
   require_rect(paint_guard->actual_caption_fp32_rect_without_callbacks());origin();}
  if(data_guard){data_guard->require_current_without_callbacks();data_guard->require_actual_renderer_without_callbacks(*renderer);
   require_rect(data_guard->actual_caption_fp32_rect_without_callbacks());origin();}
  if(metadata){metadata->require_current_without_callbacks();if(atlas)metadata->require_actual_atlas_without_callbacks(*atlas);origin();}
  if(pin){pin->first_without_callbacks();atlas->require_same_owned_context_pin_no_current(pin);origin();}
  if(binding_guard){binding_guard->require_current_without_callbacks();origin();}
  if(item){item->require_physical_current_without_callbacks();item->require_same_root_without_callbacks(root);origin();}
 };
 const auto full=[&]{first();caption.require_source_owned(*operation);first();common.require_source_owned(*operation);first();
  root.require_source_owned(*operation);first();frame.require_source_owned(*operation);first();
  rect->require_source_owned(*operation);first();
  if(run_guard){run_guard->require_source_owned(*operation);first();}
  if(paint_guard){paint_guard->require_source_owned(*operation);first();}
  if(data_guard){data_guard->require_source_owned(*operation);first();}
  if(metadata){metadata->require_source_owned(*operation);first();}
  if(binding_guard){binding_guard->require_source_owned(*operation);first();}
  if(item){item->require_source_owned(*operation);first();}scope->full();first();};
 first();if(child->positioned||child->original_cpu||child->item)
  throw std::runtime_error("Caption embedded CPU/Binding/Item cannot replay");
 const auto geometry_destination=stamp->destination;const auto geometry_clip=stamp->clip;
 const auto geometry_viewport=stamp->viewport;first();
 try{
  // Private Ui can use the accepted embedded issuer under THIS actual Frame.
  // It is distinct from standaloneRun and never queries the old cold Run.
  run_guard.emplace(run->capture_guard_for_paint_owned(*operation,tree,id,frame));first();
  rect->require_same_entered_run_without_callbacks(*run_guard);first();full();
  system=run_guard->actual_system_font_without_callbacks();first();
  if(!system)throw std::runtime_error("Caption embedded CPU requires genuine positive170");
  run_guard->require_system_font_borrower_without_callbacks(*system);first();
  const auto metadata_owner=system->atlas_metadata_without_callbacks();first();
  if(!metadata_owner)throw std::runtime_error("Caption actual system font metadata absent");
  metadata.emplace(metadata_owner->capture_guard_without_callbacks());first();full();
  atlas=metadata->retain_atlas_for_physical_cleanup_without_callbacks();first();
  if(!atlas)throw std::runtime_error("Caption canonical physical Atlas absent");
  metadata->require_actual_atlas_without_callbacks(*atlas);first();pin=metadata->impl_->context_pin.lock();
  if(!pin)throw std::runtime_error("Caption canonical counted Pin absent");first();full();
  // Adopt the original Frame/Run before issuing Paint and DATA facades.
  full();retained=std::make_shared<LabelPaintCapture>();first();full();
  retained->frame=actual_frame;retained->run=run;retained->atlas=atlas;retained->pin=pin;retained->metadata.emplace(*metadata);first();
  const LabelPaintCapture::CaptionFp32RectSnapshot raw_rect(rect,rect_words);first();
  retained->caption_fp32_rect.emplace(raw_rect);first();
  // Root10 authenticates the actual caption indexed carrier and entered Run.
  retained->paint=detail::BnwLabelPaintTextLoan::begin_caption_owned(
   *operation,tree,id,run,publication,index,*actual_frame);first();
  if(!retained->paint)throw std::runtime_error("Caption actual embedded Paint issuer absent");
  paint_guard.emplace(retained->paint->capture_guard_without_callbacks());first();
  paint_guard->require_system_font_borrower_without_callbacks(*system);first();paint_guard->require_actual_renderer_without_callbacks(*renderer);first();full();
  retained->data=retained->paint->positioned_data_owned(*operation,atlas);first();
  if(!retained->data)throw std::runtime_error("Caption actual positioned DATA issuer absent");
  data_guard.emplace(retained->data->capture_guard_without_callbacks());first();
  data_guard->require_system_font_borrower_without_callbacks(*system);first();full();
  const auto data=*data_guard;const auto*const data_facade=retained->data.get();const auto*const paint_facade=retained->paint.get();
  const auto health=frame.capture_root_scope_health_owned(*operation);first();full();
  health.require_actual_renderer_without_callbacks(*renderer);health.require_actual_publication_without_callbacks(*publication);
  health.require_actual_frame_owner_without_callbacks(frame_owner);first();
  const LabelPaintCapture::HistoricalTargetGeometry geometry(publication,frame_owner,health,data,index,id,
   geometry_destination,geometry_clip,geometry_viewport);first();retained->historical_target_geometry.emplace(geometry);first();
  full();vertices=graphics::GlBnwFontShader::positioned_vertices_owned(*operation,*data_facade);first();
  if(!vertices)throw std::runtime_error("Caption genuine positioned vertices absent");vertices->require_current_without_callbacks();first();
  auto planes=graphics::GlBnwSystemFontAtlas::capture_accepted_font_planes_owned(*data_facade);first();
  if(!planes)throw std::runtime_error("Caption accepted Font planes absent");
  full();programs=capture_label_native_programs_owned(retained);first();full();
  if(!programs||!programs->label||programs->label.get()!=retained.get()||!programs->programs)
   throw std::runtime_error("Caption genuine original Font program/label association absent");
  const auto*const use=programs->programs.get();use->require_current_without_callbacks();first();full();
  binding=graphics::GlBnwFontShader::prepare_normal_label_draw_records_owned(*operation,*use,*vertices,std::move(planes));first();
  if(!binding)throw std::runtime_error("Caption genuine immutable CPU draw records absent");
  binding_guard.emplace(binding->capture_guard_without_callbacks());first();
  binding_guard->require_same_positioned_data_without_callbacks(*data_facade);first();
  data_facade->attach_normal_draw_records_owned(*operation,*binding_guard);first();full();
  // Exact Caption4 -> mandatory4a overload after SAME DATA Binding attachment.
  item.emplace(tree->capture_normal_item_occurrence_owned(*operation,root,*publication,index,
   *actual_frame,*paint_facade,*data_facade,caption));first();full();
  const auto captured_binding=item->captured_normal_draw_records_without_callbacks();first();
  captured_binding.require_same_positioned_data_without_callbacks(*data_facade);first();
  tree->attach_normal_item_historical_font_planes_owned(*operation,*item,*data_facade,atlas);first();full();
  const auto item_atlas=item->actual_font_atlas_owned_without_callbacks();first();
  if(item_atlas.get()!=atlas.get()||item_atlas.owner_before(atlas)||atlas.owner_before(item_atlas))
   throw std::runtime_error("Caption Item SAME historical Atlas owner changed");
  const auto count=scope->normal_label_cpu_captures.size();
  if(count==scope->normal_label_cpu_captures.max_size())throw std::runtime_error("Caption CPU history custody exhausted");
  full();scope->normal_label_cpu_captures.reserve(count+1);first();full();
  auto owned_programs=std::shared_ptr<LabelNativeProgramCapture>(std::move(programs));first();
  auto owned_vertices=std::shared_ptr<graphics::GlBnwFontShader::PositionedVertices>(std::move(vertices));first();
  auto owned_binding=std::shared_ptr<graphics::detail::BnwNormalLabelDrawRecordBinding>(std::move(binding));first();
  full();auto cpu=std::make_shared<const WorldViewPaintScope::NormalLabelCpuCapture>(
   retained,owned_programs,owned_vertices,owned_binding,root,*item);first();full();
  if(scope->normal_label_cpu_captures.size()!=count||child->positioned||child->original_cpu||child->item)
   throw std::runtime_error("Caption CPU history original adoption changed during callbacks");
  scope->normal_label_cpu_captures.push_back(cpu);first();
  child->positioned=retained;child->original_cpu=cpu;child->item.emplace(*item);first();full();
  // Actual frozen Material6 consumes THESE already-issued original controls.
  // Its genuine per-pass preparation stops inside Root before Label540B54;
  // it issues no public Material, child return, whole Root or GPU permission.
  full();auto prepared_material=tree->normal_label_material_for_actual_item_owned(
   tree,*operation,*publication,index,*actual_frame,run,
   *paint_facade,*data_facade,*matrix,caption);first();full();
  // Material6 is deliberately unable to return until its separate Label tail
  // is issued. A later genuine supplier needs explicit consumer integration;
  // an unexpected return cannot silently become a caption callback result.
  throw std::runtime_error("Caption Material6 returned before genuine Label540B54 tail/child-return integration");
 }catch(...){saved_life->failed=true;throw;}
}

std::unique_ptr<detail::BnwNormalLabelMaterialLoan>
UiScreenGlRenderer::WorldViewPaintScope::prepare_current_visible_label_cpu_owned(
 UiScreenGlRenderer& actual_renderer,std::size_t actual_index,
 std::shared_ptr<LabelPaintCapture> original_label,
 const detail::BnwNormalRootBatchHistory::Guard& supplied_root,
 std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> supplied_matrix,
 const detail::BnwNormalVisibleLabelCall& supplied_call)const{
 const auto life=actual_renderer.worldview_lifetime_;const auto gate=actual_renderer.label_submission_;
 const auto*const renderer=&actual_renderer;
 // Root27 lends the genuine current call and SAME original envelope.
 const auto selected_call=supplied_call;const auto selected_root=supplied_root;
 const auto original_matrix=std::move(supplied_matrix);
 if(!gate||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
    !life->active||life->renderer!=renderer||!life->active_scope)
  throw std::runtime_error("Normal submit requires actual entered Ui");
 // The genuine thin paint entry owns the SAME gate throughout bootstrap.
 const auto*const scope=this;
 const auto origin=[&]{
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||!life->active||
     life->renderer!=renderer||life->active_scope!=scope||gate->load()!=1)
   throw std::runtime_error("Normal Label actual Ui/source scope retired or reentered");
 };
 origin();scope->pure();origin();
 const auto*const stamp=scope->label_item;
 if(!scope->painting||!scope->publication||!scope->frame||!stamp||!stamp->alive||
    stamp->scope!=scope||stamp->index!=actual_index||stamp->publication!=scope->publication||
    stamp->frame!=scope->frame||stamp->operation!=&scope->operation||stamp->serial!=scope->serial)
  throw std::runtime_error("Normal submit lacks SAME actual current indexed paint item");
 const auto*const publication=scope->publication;
 const auto*const operation=&scope->operation;const auto serial=scope->serial;
 const auto tree=publication->tree_;
 const auto id=publication->actual_checked_control_id_for_paint_without_callbacks();origin();
 const auto entered=[&]{
  origin();selected_root.require_physical_current_without_callbacks();
  selected_root.require_actual_renderer_without_callbacks(*renderer);
  selected_call.require_current_without_callbacks();
  selected_call.require_same_root_without_callbacks(selected_root);
  selected_call.require_actual_publication_without_callbacks(*publication);
  if(selected_call.actual_control_without_callbacks()!=id)
   throw std::runtime_error("Visible Label token differs from actual selected control");
  // Native ordered ordinal remains Root-owned; it is NOT this paint index.
  (void)selected_call.actual_ordered_ordinal_without_callbacks();
  if(!scope->painting||scope->publication!=publication||scope->label_item!=stamp||!stamp->alive||
     stamp->index!=actual_index||&scope->operation!=operation||scope->serial!=serial)
   throw std::runtime_error("Normal Label exact actual item/operation replaced");
  scope->pure();origin();
 };
 entered();
 // Declared BEFORE all later consumer guards/views and target facades, so its
 // transferred Frame/Run/Paint/DATA envelope outlives them on EVERY exit.
 std::shared_ptr<LabelPaintCapture> retained_label;
 std::unique_ptr<graphics::GlBnwFontShader::PositionedVertices> vertices;
 std::unique_ptr<LabelNativeProgramCapture> programs;
 std::unique_ptr<graphics::detail::BnwNormalLabelDrawRecordBinding> binding;
 std::unique_ptr<detail::BnwNormalLabelMaterialLoan> material;
 try{
  // Root's current-index normal Run is the role discriminator. No kind/XML/
  // style inference and no embedded capture is attempted for nonselected items.
  auto label=std::move(original_label);entered();
  if(!label)throw std::runtime_error("Genuine visible Label token has no actual normal Run");
  if(!label->frame||!label->standalone_run||label->run||!label->paint||!label->data||
     !label->metadata||!label->atlas||!label->pin)
   throw std::runtime_error("Normal submit requires SAME positive170 standalone envelope");
  // All owning facades and independent guards are captured before new callbacks.
  const auto actual_frame=label->frame;const auto actual_run=label->standalone_run;
  const auto*const paint_facade=label->paint.get();const auto*const data_facade=label->data.get();
  const auto frame=actual_frame->capture_guard_without_callbacks();
  const auto data=data_facade->capture_guard_without_callbacks();
  const auto paint=data.captured_paint_guard_without_callbacks();
  const auto early=data.captured_early_flush_guard_without_callbacks();
  const auto metadata=*label->metadata;const auto atlas=label->atlas;const auto pin=label->pin;
  std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> matrix;
  std::optional<graphics::detail::BnwNormalLabelDrawRecordBinding::Guard> binding_guard;
  std::optional<detail::BnwNormalRootBatchHistory::Guard> root_guard;
  std::optional<detail::BnwNormalItemOccurrence::Guard> item_guard;
  const detail::BnwNormalRootBatchExecution* execution=nullptr;
  const auto first=[&]{
   entered();frame.require_current_without_callbacks();
   frame.require_actual_paint_item_without_callbacks(*publication,actual_index);
   frame.require_actual_control_without_callbacks(*tree,id);
   data.require_current_without_callbacks();data.require_actual_renderer_without_callbacks(*renderer);
   paint.require_current_without_callbacks();data.require_same_paint_guard_without_callbacks(paint);
   early.require_current_without_callbacks();early.require_same_paint_guard_without_callbacks(paint);
   metadata.require_current_without_callbacks();metadata.require_actual_atlas_without_callbacks(*atlas);
   pin->first_without_callbacks();atlas->require_same_owned_context_pin_no_current(pin);
   (void)early.accepted_pair_without_callbacks(*atlas);
   // SAME original Matrix is physical custody; later labels never renew its
   // retired ACTIVE first Frame association. RootGuard owns immutable health.
   if(matrix)selected_root.require_actual_matrix_without_callbacks(*matrix);
   if(binding_guard)binding_guard->require_current_without_callbacks();
   if(root_guard){
    if(!execution||scope->normal_root_execution.get()!=execution||
       scope->normal_root_tree.get()!=tree.get()||scope->normal_root_tree.owner_before(tree)||
       tree.owner_before(scope->normal_root_tree))
     throw std::runtime_error("Normal CPU producer original enclosing Root execution replaced");
    const auto actual_root=execution->captured_root_without_callbacks();
    root_guard->require_same_root_without_callbacks(actual_root);
    root_guard->require_physical_current_without_callbacks();
   }
   if(item_guard){item_guard->require_physical_current_without_callbacks();
    item_guard->require_same_root_without_callbacks(*root_guard);
    item_guard->require_actual_positioned_data_without_callbacks(data);}
   // Root26 child-return Material stays unsealed until whole Stage RETURN.
   entered();
  };
  const auto full=[&]{first();frame.require_source_owned(*operation);first();
   data.require_source_owned(*operation);first();metadata.require_source_owned(*operation);first();
   selected_call.require_source_owned(*operation);first();selected_root.require_source_owned(*operation);first();
   if(binding_guard){binding_guard->require_source_owned(*operation);first();}
   if(root_guard){execution->require_source_owned(*operation);first();}
   if(item_guard){item_guard->require_source_owned(*operation);first();}
   // No public child-return Material FIRST/FULL before whole Stage RETURN.
};
  // Shared physical envelope ownership is adopted under DATA FIRST before any
  // later material/vertices view exists. A throwing program-preparation callee
  // cannot destroy these SAME lexical facades before the caller views unwind.
  first();retained_label=std::move(label);first();
  // Preserve genuine immutable item geometry BEFORE later program/vertex
  // callbacks. Snapshotting neither queries GL nor manufactures queue authority.
  const auto geometry_destination=stamp->destination;const auto geometry_clip=stamp->clip;
  const auto geometry_viewport=stamp->viewport;first();
  const auto geometry_health=frame.capture_root_scope_health_owned(*operation);first();
  const auto geometry_publication=publication->original_owner_without_callbacks();first();
  const auto geometry_frame_owner=publication->actual_adjusted_frame_owner_for_paint_without_callbacks(
   *renderer,*scope->frame);first();
  geometry_health.require_actual_renderer_without_callbacks(*renderer);first();
  geometry_health.require_actual_publication_without_callbacks(*geometry_publication);first();
  geometry_health.require_actual_frame_owner_without_callbacks(geometry_frame_owner);first();
  if(geometry_publication.get()!=publication||geometry_frame_owner.get()!=scope->frame)
   throw std::runtime_error("Historical geometry lost SAME original publication/adjusted frame");
  const LabelPaintCapture::HistoricalTargetGeometry geometry_snapshot(geometry_publication,
   geometry_frame_owner,geometry_health,data,actual_index,id,geometry_destination,geometry_clip,geometry_viewport);
  first();retained_label->historical_target_geometry.emplace(geometry_snapshot);first();
  // Root27 callback lends ORIGINAL execution Root and Matrix. This body
  // never begins another Prefix/Matrix nor changes their revision/epoch.
  execution=scope->normal_root_execution.get();
  if(!execution||scope->normal_root_tree.get()!=tree.get()||
     scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree))
   throw std::runtime_error("Visible Label lacks SAME original enclosing execution");
  root_guard.emplace(selected_root);first();
  const auto execution_root=execution->captured_root_without_callbacks();first();
  selected_root.require_same_root_without_callbacks(execution_root);first();
  matrix=original_matrix;first();
  const auto execution_matrix=execution->actual_matrix_owned_without_callbacks();first();
  if(!matrix||matrix.get()!=execution_matrix.get()||matrix.owner_before(execution_matrix)||
     execution_matrix.owner_before(matrix))
   throw std::runtime_error("Visible Label supplied Matrix is not SAME original Root owner");
  full();
  vertices=graphics::GlBnwFontShader::positioned_vertices_owned(*operation,*data_facade);first();
  if(!vertices)throw std::runtime_error("Normal submit genuine vertex facade absent");
  vertices->require_current_without_callbacks();first();
  auto planes=graphics::GlBnwSystemFontAtlas::capture_accepted_font_planes_owned(*data_facade);first();
  if(!planes)throw std::runtime_error("Normal submit genuine accepted planes absent");
  // Physical library cache reads/hashes/compiles only on canonical Bank/Pin miss.
  // Transfer SAME Label envelope so Frame/Run/Paint/DATA are produced once.
  programs=actual_renderer.capture_label_native_programs_owned(retained_label);first();
  if(!programs||!programs->label||!programs->programs)
   throw std::runtime_error("Normal submit current original-program use absent");
  const auto*const actual_label=programs->label.get();
  const auto*const use=programs->programs.get();first();
  use->require_current_without_callbacks();first();full();
  // Actual immutable CPU streams/records precede Material. Transport upload
  // belongs to the genuine selected manager drain, after Root return.
  // The ONE original planes facade moves into Binding; no extra GL resource.
  binding=graphics::GlBnwFontShader::prepare_normal_label_draw_records_owned(
   *operation,*use,*vertices,std::move(planes));first();
  if(!binding)throw std::runtime_error("Normal submit genuine prepared CPU draw records absent");
  binding_guard.emplace(binding->capture_guard_without_callbacks());first();
  binding_guard->require_same_positioned_data_without_callbacks(*data_facade);first();
  data_facade->attach_normal_draw_records_owned(*operation,*binding_guard);first();full();
  // The genuine Root Item captures this SAME original DATA only AFTER its
  // authentic CPU Binding attachment. It earns no GPU/Atlas/drain authority.
  item_guard.emplace(tree->capture_normal_item_occurrence_owned(*operation,*root_guard,
   *publication,actual_index,*actual_frame,*paint_facade,*data_facade));first();full();
  const auto captured_binding=item_guard->captured_normal_draw_records_without_callbacks();first();
  captured_binding.require_same_positioned_data_without_callbacks(*data_facade);first();
  // Root bridges the sole Atlas supplier while this ORIGINAL DATA is active.
  // Install exactly once after CPU Item capture and before Common/custody.
  // Ui never mints a HistoricalPlanesIssuerKey or recaptures AcceptedPlanes.
  full();tree->attach_normal_item_historical_font_planes_owned(
   *operation,*item_guard,*data_facade,atlas);first();full();
  const auto original_item_atlas=item_guard->actual_font_atlas_owned_without_callbacks();first();
  if(original_item_atlas.get()!=atlas.get()||original_item_atlas.owner_before(atlas)||
     atlas.owner_before(original_item_atlas))
   throw std::runtime_error("Normal CPU Item lost SAME original historical Atlas owner");
  first();
  // Reserve before ownership adoption; every allocation is bracketed by the
  // actual ACTIVE and Root/Item/source checks. No later item can drop these
  // original facades before genuine manager consumption and Rows restoration.
  const auto count=scope->normal_label_cpu_captures.size();
  if(count==scope->normal_label_cpu_captures.max_size())
   throw std::runtime_error("Normal CPU original custody space exhausted");
  full();scope->normal_label_cpu_captures.reserve(count+1);first();full();
  auto retained_programs=std::shared_ptr<LabelNativeProgramCapture>(std::move(programs));first();
  auto retained_vertices=std::shared_ptr<graphics::GlBnwFontShader::PositionedVertices>(std::move(vertices));first();
  auto retained_binding=std::shared_ptr<graphics::detail::BnwNormalLabelDrawRecordBinding>(std::move(binding));first();
  full();auto original_cpu=std::make_shared<const WorldViewPaintScope::NormalLabelCpuCapture>(
   retained_label,retained_programs,retained_vertices,retained_binding,*root_guard,*item_guard);first();full();
  if(scope->normal_label_cpu_captures.size()!=count)
   throw std::runtime_error("Normal CPU original custody changed during producer callbacks");
  scope->normal_label_cpu_captures.push_back(std::move(original_cpu));first();full();
  // Root snapshots the SAME supplied facades before its own callbacks, performs
  // real root18/common/Label computation and seals completed selector1 append.
  // Exact Root26/27 Fixed9 returns genuine current-child Material to Root.
  first();material=tree->normal_label_material_for_actual_item_owned(
   tree,*operation,*publication,actual_index,*actual_frame,actual_run,
   *paint_facade,*data_facade,*matrix);first();
  if(!material)throw std::runtime_error("Normal submit genuine completed Material absent");
  // Root26/27 earns current-child return and authentic token association.
  // Public Material FIRST is unavailable until whole Stage RETURN.
  // RootExecution then owns SAME unique Material through wholeRoot/drain.
  return material;
 }catch(...){
  gate->store(2);life->failed=true;
  // Destructors only retire/enqueue SAME-Pin physical custody. They never query
  // Source, throw, switch current, pretend restoration or clear native dirty.
  material.reset();throw;
 }
}

std::shared_ptr<const BnwWorldViewControlTree::NormalVisibleLabelConsumer>
UiScreenGlRenderer::WorldViewPaintScope::capture_normal_visible_cpu_consumer_owned(
 UiScreenGlRenderer& actual_renderer,
 std::shared_ptr<const std::vector<std::shared_ptr<const NormalVisibleOriginalPublication>>> actual_inputs)const{
 const auto saved_life=actual_renderer.worldview_lifetime_;
 const auto*const scope=this;auto*const renderer=&actual_renderer;const auto inputs=std::move(actual_inputs);
 const auto origin=[&]{
  if(saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
     !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
   throw std::runtime_error("Visible CPU original Ui scope retired");
 };
 origin();scope->pure();origin();
 if(!inputs)throw std::runtime_error("Visible CPU original input custody absent");
 const auto*const operation=&scope->operation;
 origin();scope->full();origin();
 auto result=std::make_shared<const BnwWorldViewControlTree::NormalVisibleLabelConsumer>(
  [saved_life,scope,renderer,operation,inputs](
   const runtime::BnwWorldViewSourceOperationLoan& actual_operation,
   const detail::BnwNormalRootBatchHistory::Guard& actual_root,
   std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan> actual_matrix,
   const detail::BnwNormalVisibleLabelCall& actual_call)
   ->std::unique_ptr<detail::BnwNormalLabelMaterialLoan>{
   const auto root=actual_root;const auto matrix=std::move(actual_matrix);const auto call=actual_call;
   const auto origin=[&]{
    if(saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
       !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=scope)
     throw std::runtime_error("Visible selected CPU original Ui scope retired");
   };
   origin();scope->pure();origin();
   const auto tree=scope->normal_root_tree;const auto*const execution=scope->normal_root_execution.get();
   if(!tree||!execution||!matrix)throw std::runtime_error("Visible selected CPU original execution absent");
   const auto first=[&]{origin();scope->pure();origin();
    if(&actual_operation!=operation||&scope->operation!=operation||
       scope->normal_root_execution.get()!=execution||scope->normal_root_tree.get()!=tree.get()||
       scope->normal_root_tree.owner_before(tree)||tree.owner_before(scope->normal_root_tree)||
       scope->normal_visible_inputs.get()!=inputs.get()||scope->normal_visible_inputs.owner_before(inputs)||
       inputs.owner_before(scope->normal_visible_inputs))
     throw std::runtime_error("Visible selected CPU original Source/input/execution replaced");
    root.require_physical_current_without_callbacks();root.require_actual_renderer_without_callbacks(*renderer);
    const auto original_root=execution->captured_root_without_callbacks();
    root.require_same_root_without_callbacks(original_root);root.require_actual_matrix_without_callbacks(*matrix);
    call.require_current_without_callbacks();call.require_same_root_without_callbacks(root);origin();};
   const auto full=[&]{first();root.require_source_owned(*operation);first();
    call.require_source_owned(*operation);first();};
   try{
    first();const auto control=call.actual_control_without_callbacks();first();
    (void)call.actual_ordered_ordinal_without_callbacks();first();
    std::shared_ptr<const NormalVisibleOriginalPublication> original;
    for(const auto& input:*inputs){
     first();if(!input)throw std::runtime_error("Visible original input absent");
     if(input->control==control&&input->tree.get()==tree.get()&&
        !input->tree.owner_before(tree)&&!tree.owner_before(input->tree)){
      if(original)throw std::runtime_error("Visible original input duplicated");original=input;
     }
    }
    first();if(!original||!original->publication||!original->emission)
     throw std::runtime_error("Visible selected Label original Publication/emission absent");
    const auto publication=original->publication;const auto emission=original->emission;
    call.require_actual_publication_without_callbacks(*publication);first();
    if(publication->normal_label_emission_.get()!=emission.get()||
       publication->normal_label_emission_.owner_before(emission)||
       emission.owner_before(publication->normal_label_emission_))
     throw std::runtime_error("Visible selected Label changed SAME sealed emission");
    if(scope->normal_visible_child)
     throw std::runtime_error("Visible CPU child consumer reentered");
    full();auto child=std::make_shared<NormalVisibleChildCapture>(original,root,matrix,call);first();full();
    if(scope->normal_visible_child)
     throw std::runtime_error("Visible CPU child context changed during allocation");
    scope->normal_visible_child=child;first();
    struct EndChild final {
     const std::shared_ptr<WorldViewPaintLifetime> life;const WorldViewPaintScope* scope;
     const std::shared_ptr<NormalVisibleChildCapture> child;
     ~EndChild()noexcept{
      if(life->active_scope==scope&&scope->normal_visible_child.get()==child.get())
       scope->normal_visible_child.reset();
     }
    } end_child{saved_life,scope,child};
    const auto child_first=[&]{first();
     if(scope->normal_visible_child.get()!=child.get()||scope->normal_visible_child.owner_before(child)||
        child.owner_before(scope->normal_visible_child))
      throw std::runtime_error("Visible CPU child context retired or replaced");};
    child_first();
    if(scope->normal_bootstrap_label){
     // Root27 FIRST callback consumes SAME original envelope captured by the
     // already-entered helper. No nested paint, no second Frame/Run/DATA mint.
     const auto label=scope->normal_bootstrap_label;const auto*const stamp=scope->label_item;
     if(!scope->normal_bootstrap_selection)throw std::runtime_error("Bootstrap original native selection absent");
     const auto selection=*scope->normal_bootstrap_selection;child_first();
     selection.require_actual_publication_without_callbacks(*publication);child_first();
     tree->require_normal_root_bootstrap_selection_call_without_callbacks(selection,root,call);child_first();
     if(!scope->painting||!scope->frame||scope->publication!=publication.get()||!stamp||
        !stamp->alive||stamp->scope!=scope||stamp->publication!=publication.get()||stamp->frame!=scope->frame)
      throw std::runtime_error("Visible bootstrap lost SAME entered original item");
     const auto index=stamp->index;child_first();
     child->material=scope->prepare_current_visible_label_cpu_owned(
      *renderer,index,label,root,matrix,call);child_first();
    }else{
     // Root27 continuation enters only AFTER first Publication.paint RETURN.
     // Each later canonical Publication has never issued its adjusted frame.
     if(scope->painting||scope->frame||scope->label_item)
      throw std::runtime_error("Visible continuation attempted nested original paint");
     const auto*const previous=scope->publication;
     const_cast<WorldViewPaintScope*>(scope)->publication=publication.get();child_first();
     struct EndPublication final {
      const std::shared_ptr<WorldViewPaintLifetime> life;const WorldViewPaintScope* scope;
      const BnwWorldViewControlRenderPublication* previous;
      ~EndPublication()noexcept{if(life->active_scope==scope)const_cast<WorldViewPaintScope*>(scope)->publication=previous;}
     } end_publication{saved_life,scope,previous};
     child_first();auto report=publication->paint(*renderer);child_first();
     const auto count=scope->normal_visible_reports.size();
     if(count==scope->normal_visible_reports.max_size())
      throw std::runtime_error("Visible original report journal capacity exhausted");
     full();scope->normal_visible_reports.reserve(count+1);child_first();full();
     if(scope->normal_visible_reports.size()!=count)
      throw std::runtime_error("Visible original report journal changed during Source");
     scope->normal_visible_reports.push_back(std::move(report));child_first();
    }
    child_first();if(!child->material)
     throw std::runtime_error("Visible CPU child omitted genuine unique Material return");
    // Public Material FIRST/scalars are unavailable before whole Stage RETURN.
    // Root alone validates/adopts the authentic child-return control and tail.
    auto material=std::move(child->material);child_first();return material;
   }catch(...){saved_life->failed=true;throw;}
  });
 origin();scope->pure();origin();scope->full();origin();return result;
}

bool UiScreenGlRenderer::submit_current_normal_label_owned(std::size_t actual_index){
 const auto life=worldview_lifetime_;const auto gate=label_submission_;const auto*const renderer=this;
 if(!gate||life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
    !life->active||life->renderer!=renderer||!life->active_scope)
  throw std::runtime_error("Normal visible entry requires genuine entered Ui");
 unsigned idle=0;if(!gate->compare_exchange_strong(idle,1)){
  gate->store(2);life->failed=true;throw std::runtime_error("Normal visible entry recursive or failed");
 }
 struct Submission final {
  const std::shared_ptr<std::atomic<unsigned>> gate;
  ~Submission()noexcept{unsigned active=1;(void)gate->compare_exchange_strong(active,0);}
 } submission{gate};
 const auto*const scope=static_cast<const WorldViewPaintScope*>(life->active_scope);
 const auto origin=[&]{
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||!life->active||
     life->renderer!=renderer||life->active_scope!=scope||gate->load()!=1)
   throw std::runtime_error("Normal visible original Ui scope retired");
 };
 try{
  origin();scope->pure();origin();const auto*const stamp=scope->label_item;
  const auto*const publication=scope->publication;const auto*const operation=&scope->operation;
  if(!scope->painting||!scope->frame||!publication||!stamp||!stamp->alive||
     stamp->scope!=scope||stamp->index!=actual_index||stamp->publication!=publication||
     stamp->frame!=scope->frame||stamp->operation!=operation||stamp->serial!=scope->serial)
   throw std::runtime_error("Normal visible entry has no SAME original current paint item");
  auto captured=capture_label_actual_positioned_owned(true);origin();scope->pure();origin();
  if(!captured){
   if(scope->normal_visible_child)
    throw std::runtime_error("CURRENT visible Label token has no genuine normal Run");
   return false;
  }
  if(!captured->frame||!captured->standalone_run||!captured->paint||!captured->data)
   throw std::runtime_error("Normal visible entry original envelope incomplete");
  const auto actual_frame=captured->frame;const auto frame=actual_frame->capture_guard_without_callbacks();
  const auto data=captured->data->capture_guard_without_callbacks();const auto tree=publication->tree_;
  const auto first=[&]{origin();scope->pure();origin();
   if(scope->publication!=publication||scope->label_item!=stamp||!stamp->alive||stamp->index!=actual_index)
    throw std::runtime_error("Normal visible original item replaced");
   frame.require_current_without_callbacks();frame.require_actual_paint_item_without_callbacks(*publication,actual_index);
   data.require_current_without_callbacks();data.require_actual_renderer_without_callbacks(*renderer);origin();};
  const auto full=[&]{first();frame.require_source_owned(*operation);first();data.require_source_owned(*operation);first();};
  first();auto label=std::shared_ptr<LabelPaintCapture>(std::move(captured));first();full();
  const auto child=scope->normal_visible_child;first();
  if(child){
   // The authentic continuation consumer already entered this original
   // Publication.paint once; it lends the closed CURRENT call synchronously.
   child->call.require_current_without_callbacks();child->call.require_same_root_without_callbacks(child->root);
   child->call.require_actual_publication_without_callbacks(*publication);first();
   if(child->original->publication.get()!=publication||child->material)
    throw std::runtime_error("Normal visible child Publication/unique return differs");
   auto material=scope->prepare_current_visible_label_cpu_owned(
    *this,actual_index,label,child->root,child->matrix,child->call);first();
   if(scope->normal_visible_child.get()!=child.get()||child->material||!material)
    throw std::runtime_error("Normal visible child return changed during actual CPU preparation");
   child->material=std::move(material);first();return true;
  }
  // Only Root27 bootstrap may consume an entry without a current child box.
  // Capture this SAME original Frame/Run/Paint/DATA before creating Root.
  if(scope->normal_root_execution||scope->normal_root_tree)
   throw std::runtime_error("Normal Label cannot begin another Root/epoch outside genuine owner traversal");
  if(!scope->normal_visible_consumer||!scope->normal_visible_inputs)
   throw std::runtime_error("Normal bootstrap has no OWN original-publication consumer");
  if(!scope->normal_bootstrap_selection)throw std::runtime_error("Normal entry lacks genuine native prepaint selection");
  const auto selection=*scope->normal_bootstrap_selection;first();
  selection.require_actual_publication_without_callbacks(*publication);first();
  selection.require_source_owned(*operation);first();
  full();auto execution=tree->begin_normal_root_batch_execution_owned(
   *operation,*publication,actual_index,*actual_frame);first();
  if(!execution)throw std::runtime_error("Normal bootstrap original Root execution absent");
  execution->require_source_owned(*operation);first();
  scope->normal_root_tree=tree;scope->normal_root_execution=std::move(execution);first();
  if(scope->normal_bootstrap_label)
   throw std::runtime_error("Normal bootstrap envelope already lent");
  scope->normal_bootstrap_label=label;first();
  struct EndBootstrap final {
   const std::shared_ptr<WorldViewPaintLifetime> life;const WorldViewPaintScope* scope;
   const std::shared_ptr<LabelPaintCapture> label;
   ~EndBootstrap()noexcept{
    if(life->active_scope==scope&&scope->normal_bootstrap_label.get()==label.get())
     scope->normal_bootstrap_label.reset();
   }
  } end_bootstrap{life,scope,label};
  const auto consumer=scope->normal_visible_consumer;first();full();
  tree->bootstrap_normal_root_visible_label_owned(
   *operation,*scope->normal_root_execution,*actual_frame,consumer);first();full();
  // Root27 has accepted the authentic first child and advanced its real cursor.
  // Root continuation/registration/drain belongs after THIS original paint RETURN.
  return true;
 }catch(...){gate->store(2);life->failed=true;throw;}
}
#ifdef _WIN32
struct UiScreenGlRenderer::PointerReceipt {
 const std::shared_ptr<const BnwTechAwardWindowPointerLoan> native;
 const LiveUiFrame ui;
 const std::shared_ptr<const BnwClientFrame> frame;
 const std::shared_ptr<const BnwTechAwardAnimationWitness> witness;
 const std::uint64_t clock_sequence;
 PointerReceipt(std::shared_ptr<const BnwTechAwardWindowPointerLoan> n,const LiveUiFrame& u,
  std::shared_ptr<const BnwClientFrame> f,std::shared_ptr<const BnwTechAwardAnimationWitness> w,std::uint64_t sequence)
  :native(std::move(n)),ui(u),frame(std::move(f)),witness(std::move(w)),clock_sequence(sequence){}
 PointerReceipt(const PointerReceipt&)=delete;
};
#endif
namespace {

graphics::GlTexture white_texture(const graphics::GlContext &context) {
    const assets::Rgba8Level pixel{1, 1, {255, 255, 255, 255}};
    return graphics::GlTexture::rgba8(context, std::span<const assets::Rgba8Level>(&pixel, 1));
}

std::array<float, 4> color(std::uint32_t rgba) {
    return {((rgba >> 24) & 255) / 255.f, ((rgba >> 16) & 255) / 255.f,
            ((rgba >> 8) & 255) / 255.f, (rgba & 255) / 255.f};
}

bool complete_style_layers(const UiGlyphStyleBinding &binding, const UiFontStylePaint &paint) {
    for (const auto &layer:binding.layers) {
        if (!layer.glyphs.geometry || !layer.glyphs.texture || layer.color_index>=paint.colors.size())
            return false;
        const bool floating=paint.float_colors && layer.color_index<paint.float_colors->size();
        if (!paint.colors[layer.color_index] && !floating) return false;
    }
    return true;
}

bool looks_like_dds(std::string_view key) {
    return key.size() >= 4 && (key.substr(key.size() - 4) == ".dds" ||
                               key.substr(key.size() - 4) == ".DDS");
}

} // namespace

UiScreenGlRenderer::UiScreenGlRenderer(const graphics::GlContext &context,
    const assets::ResourceMountTable &resources, UiGlyphBinding font16, UiGlyphBinding font20,
    UiGlyphBinding icons,
    std::function<std::optional<std::uint32_t>(std::string_view)> icon_resolver,
    UiGlyphBinding font24, UiGlyphStyleResolver font_styles)
    : context_(context), resources_(resources), gl_(context), font16_(font16), font20_(font20),
      font24_(font24),
      icons_(icons), icon_resolver_(std::move(icon_resolver)), font_styles_(std::move(font_styles)), white_(white_texture(context)) {
    worldview_lifetime_->renderer=this;
    label_module_=detail::BnwLabelProcessModuleOwner::create_fresh_owned(*this);
    font_texture_cache_=std::make_shared<FontTextureCacheStorage>(worldview_lifetime_,this);
    system_font_cache_=std::make_shared<SystemFontCacheStorage>(worldview_lifetime_,this);
    if (!font16_.geometry || !font16_.texture || !font20_.geometry || !font20_.texture)
        throw std::invalid_argument("UI painter needs active GlyphGen font bindings");
    if (bool(icons_.geometry) != bool(icons_.texture) ||
        bool(icons_.geometry) != bool(icon_resolver_))
        throw std::invalid_argument("UI painter needs complete FontIcons bindings");
    if (bool(font24_.geometry) != bool(font24_.texture))
        throw std::invalid_argument("UI painter needs a complete TwCenMT24 binding");
    const char *vertex = "#version 460 core\n"
        "uniform vec4 rect; uniform vec2 viewport; out vec2 uv;"
        "void main(){vec2 p=vec2((gl_VertexID&1)!=0?1.:0.,(gl_VertexID>=2)?1.:0.);"
        "vec2 xy=rect.xy+p*rect.zw;"
        "gl_Position=vec4(xy.x/viewport.x*2.-1.,1.-xy.y/viewport.y*2.,0.,1.);uv=p;}";
    const char *fragment = "#version 460 core\n"
        "layout(binding=0) uniform sampler2D source; in vec2 uv; out vec4 color;"
        "uniform vec4 uvRect; uniform vec4 tint; uniform int alphaOnly;"
        "void main(){vec2 t=uvRect.xy+uv*uvRect.zw; vec4 pixel=texture(source,t);"
        "color=alphaOnly!=0?vec4(tint.rgb,pixel.a*tint.a):pixel*tint;}";
    const auto vs = gl_.CreateShader(GL_VERTEX_SHADER), ps = gl_.CreateShader(GL_FRAGMENT_SHADER);
    gl_.ShaderSource(vs, 1, &vertex, nullptr);
    gl_.ShaderSource(ps, 1, &fragment, nullptr);
    gl_.CompileShader(vs);
    gl_.CompileShader(ps);
    program_ = gl_.CreateProgram();
    gl_.AttachShader(program_, vs);
    gl_.AttachShader(program_, ps);
    gl_.LinkProgram(program_);
    gl_.DeleteShader(vs);
    gl_.DeleteShader(ps);
    GLint linked = 0;
    gl_.GetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) {
        gl_.DeleteProgram(program_);
        program_ = 0;
        throw std::runtime_error("UI screen painter shader did not link");
    }
    rect_ = gl_.GetUniformLocation(program_, "rect");
    viewport_ = gl_.GetUniformLocation(program_, "viewport");
    uv_ = gl_.GetUniformLocation(program_, "uvRect");
    tint_ = gl_.GetUniformLocation(program_, "tint");
    alpha_only_ = gl_.GetUniformLocation(program_, "alphaOnly");
    if (rect_ < 0 || viewport_ < 0 || uv_ < 0 || tint_ < 0 || alpha_only_ < 0)
        throw std::runtime_error("UI screen painter shader uniforms are missing");
    gl_.CreateVertexArrays(1, &vao_);
}

UiScreenGlRenderer::~UiScreenGlRenderer() {
    worldview_lifetime_->alive=false;
    if(original_resource0_storage_)original_resource0_storage_->initializer->retire_owned();
    original_resource0_storage_.reset();
    if(label_module_)label_module_->retire_owned();
    if(font_texture_cache_)font_texture_cache_->retire_owned();
    if(system_font_cache_)system_font_cache_->retire_owned();
    label_program_preparation_->store(2);
    // Cache-only program Nodes enter the closed queue BEFORE this drain.
    // Separately retained physical Uses remain subject to deferred SAME Pin custody.
    label_original_program_cache_.reset();
    graphics::BnwOwnedCameraRendererWindowLease::drain_retired_creator_storage();
    tech_award_lifetime_->alive=false;
    if (masked_buffer_) gl_.DeleteBuffers(1, &masked_buffer_);
    if (masked_index_buffer_) gl_.DeleteBuffers(1, &masked_index_buffer_);
    if (masked_vao_) gl_.DeleteVertexArrays(1, &masked_vao_);
    gl_.DeleteSamplers(static_cast<GLsizei>(masked_samplers_.size()),masked_samplers_.data());
    if (masked_program_) gl_.DeleteProgram(masked_program_);
    if (vao_) gl_.DeleteVertexArrays(1, &vao_);
    if (program_) gl_.DeleteProgram(program_);
}

const graphics::GlTexture *UiScreenGlRenderer::texture(std::string_view key,
                                                        UiGlPaintReport &report) {
    if (key.empty()) return nullptr;
    const auto* fence=worldview_scope_without_callbacks();
    if(fence)fence->full();
    const auto name = std::string(key);
    if (const auto it = textures_.find(name); it != textures_.end()) return &it->second;
    try {
        // Authored FTXTInterface metadata, rather than a caller-added prefix,
        // determines whether the image is a paired block atlas. COLOR/ordinary
        // DDS retain their existing upload path and authored mip chain.
        const auto loaded = load_ui_texture_asset(resources_, key);
        if(fence)fence->full();
        if (loaded.expanded) {
            return &textures_.emplace(name, graphics::GlTexture::rgba8(
                context_, std::span<const assets::Rgba8Level>(&*loaded.expanded, 1),
                graphics::DdsSamplingColor::linear)).first->second;
        }
        return &textures_.emplace(name, graphics::GlTexture(
            context_, loaded.dds, graphics::DdsSamplingColor::srgb)).first->second;
    } catch (const std::exception &error) {
        if(fence)fence->full();
        report.unresolved.push_back("Active DDS cannot be rendered: " + name + ": " + error.what());
        return nullptr;
    }
}

const TopPanelGrid9Style *UiScreenGlRenderer::style(std::string_view name,
                                                    UiGlPaintReport &report) {
    const auto* fence=worldview_scope_without_callbacks();
    if(fence)fence->full();
    const auto key = std::string(name);
    if (const auto it = styles_.find(key); it != styles_.end())
        return it->second ? &*it->second : nullptr;
    try {
        auto parsed = read_top_panel_grid9_style(resources_, key);
        if(fence)fence->full();
        return &*styles_.emplace(key, std::move(parsed)).first->second;
    } catch (const std::exception &) {
        if(fence)fence->full();
        // Button styles in Styles.xml refer to a Grid9 style through the
        // authored GridData child (for example BaseButton).
        try {
            const auto selected = resources_.resolve(assets::AssetKey("Assets/UI/Styles.xml"));
            if(fence)fence->full();
            const auto *asset = std::get_if<assets::AssetHandle>(&selected.asset);
            if (!asset) throw std::runtime_error("Styles.xml is unavailable");
            const auto bytes = asset->read_bytes(64ULL * 1024 * 1024);
            if(fence)fence->full();
            const auto document = content::read_xml_metadata(
                std::string(bytes.begin(), bytes.end()));
            const auto *node = document.child(key);
            const auto *grid_data = node ? node->child("GridData") : nullptr;
            const auto target = grid_data ? grid_data->attribute("Style") : std::string{};
            if (target.empty() || target == key)
                throw std::runtime_error("Style has no GridData reference");
            auto parsed = read_top_panel_grid9_style(resources_, target);
            if(fence)fence->full();
            return &*styles_.emplace(key, std::move(parsed)).first->second;
        } catch (const std::exception &) {
            if(fence)fence->full();
            styles_.emplace(key, std::nullopt);
            report.unresolved.push_back("Grid9 style is unavailable: " + key);
            return nullptr;
        }
    }
}

void UiScreenGlRenderer::quad(const graphics::GlTexture &texture,
                              const WorldViewRect &dest, std::array<float, 4> uv,
                              std::uint32_t rgba, bool alpha_only,
                              const WorldViewRect &clip, std::optional<UiFloatColor> float_tint) {
    const auto* fence=worldview_scope_without_callbacks();
    if(fence)fence->pure();
    const auto visible = clip_ui_quad(dest, uv, clip);
    if (!visible) return;
    uv = visible->uv;
    const auto &r = visible->destination;
    const std::array<float, 4> rectangle{float(r.x), float(r.y),
                                        float(r.width), float(r.height)};
    const auto tint = float_tint.value_or(color(rgba));
    for(const auto channel:tint)if(!std::isfinite(channel)||channel<0||channel>1)
        throw std::runtime_error("UI float tint is nonfinite or outside normalized channels");
    texture.bind(0);
    gl_.ProgramUniform4fv(program_, rect_, 1, rectangle.data());
    gl_.ProgramUniform4fv(program_, uv_, 1, uv.data());
    gl_.ProgramUniform4fv(program_, tint_, 1, tint.data());
    gl_.ProgramUniform1i(program_, alpha_only_, alpha_only ? 1 : 0);
    gl_.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// PRIVATE primitive consumer. Root must supply the exact sealed publication
// paint and four borrowed raw index/block textures from its genuine source
// owner. This does not load/expand resources or manufacture UV/time authority.
void UiScreenGlRenderer::create_masked_pipeline_owned() {
    const auto life = worldview_lifetime_;
    const auto* scope = worldview_scope_without_callbacks();
    if (!scope) throw std::runtime_error("Masked pipeline needs actual WorldView scope");
    scope->pure();
    if (masked_program_) return;
    const char* vertex =
#include "../../shaders/notification_scrollanim_masked.vert.inc"
    ;
    const char* fragment =
#include "../../shaders/notification_scrollanim_indexed.frag.inc"
    ;
    GLuint vs = 0, fs = 0, program = 0, vao = 0, buffer = 0, index_buffer = 0;
    std::array<GLuint,4> samplers{};
    const auto cleanup = [&]() noexcept {
        if (vs) gl_.DeleteShader(vs);
        if (fs) gl_.DeleteShader(fs);
        if (buffer) gl_.DeleteBuffers(1, &buffer);
        if (index_buffer) gl_.DeleteBuffers(1, &index_buffer);
        if (vao) gl_.DeleteVertexArrays(1, &vao);
        gl_.DeleteSamplers(static_cast<GLsizei>(samplers.size()), samplers.data());
        if (program) gl_.DeleteProgram(program);
    };
    try {
        vs = gl_.CreateShader(GL_VERTEX_SHADER);
        fs = gl_.CreateShader(GL_FRAGMENT_SHADER);
        if (!vs || !fs) throw std::runtime_error("Masked shader allocation failed");
        gl_.ShaderSource(vs, 1, &vertex, nullptr);
        gl_.ShaderSource(fs, 1, &fragment, nullptr);
        gl_.CompileShader(vs); gl_.CompileShader(fs);
        GLint compiled_vs = 0, compiled_fs = 0;
        gl_.GetShaderiv(vs, GL_COMPILE_STATUS, &compiled_vs);
        gl_.GetShaderiv(fs, GL_COMPILE_STATUS, &compiled_fs);
        if (!compiled_vs || !compiled_fs) throw std::runtime_error("Masked shader compile refused");
        program = gl_.CreateProgram();
        if (!program) throw std::runtime_error("Masked program allocation failed");
        gl_.AttachShader(program, vs); gl_.AttachShader(program, fs);
        gl_.LinkProgram(program);
        GLint linked = 0; gl_.GetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) throw std::runtime_error("Masked indexed program did not link");
        const GLint viewport = gl_.GetUniformLocation(program, "viewport");
        const std::array<const char*,8> names{"BlockSize", "IndexScale", "IndexTextureSize",
            "BlockTextureSize", "MaskBlockSize", "MaskIndexScale", "MaskIndexTextureSize", "MaskBlockTextureSize"};
        std::array<GLint,8> registers{};
        if (viewport < 0) throw std::runtime_error("Masked viewport uniform missing");
        for (std::size_t n=0; n<registers.size(); ++n) {
            registers[n] = gl_.GetUniformLocation(program, names[n]);
            if (registers[n] < 0) throw std::runtime_error("Masked native register uniform missing");
        }
        static_assert(std::is_standard_layout_v<WorldViewMaskedVertex>);
        static_assert(sizeof(WorldViewMaskedVertex)==10*sizeof(float));
        gl_.CreateBuffers(1, &buffer); gl_.CreateBuffers(1, &index_buffer);
        gl_.CreateVertexArrays(1, &vao);
        gl_.CreateSamplers(static_cast<GLsizei>(samplers.size()), samplers.data());
        if (!buffer || !index_buffer || !vao || std::any_of(samplers.begin(),samplers.end(),[](GLuint id){return !id;}))
            throw std::runtime_error("Masked vertex/sampler allocation failed");
        gl_.VertexArrayVertexBuffer(vao, 0, buffer, 0, sizeof(WorldViewMaskedVertex));
        // Actual native Quad corner order: (1,1),(0,1),(1,0),(0,0).
        // The sealed publisher supplies those independent already composed
        // positions/BaseUV/MaskUV/COLOR0; this leaf never derives them.
        static_assert(sizeof(GLushort)==2);
        static constexpr std::array<GLushort,6> indices{2,1,0,3,1,2};
        gl_.NamedBufferData(index_buffer,sizeof(indices),indices.data(),GL_STATIC_DRAW);
        gl_.VertexArrayElementBuffer(vao,index_buffer);
        const std::array<GLuint,4> offsets{
            offsetof(WorldViewMaskedVertex,pixel_position), offsetof(WorldViewMaskedVertex,base_uv),
            offsetof(WorldViewMaskedVertex,mask_uv), offsetof(WorldViewMaskedVertex,color0)};
        const std::array<GLint,4> sizes{2,2,2,4};
        for (GLuint n=0; n<4; ++n) {
            gl_.EnableVertexArrayAttrib(vao, n);
            gl_.VertexArrayAttribFormat(vao, n, sizes[n], GL_FLOAT, GL_FALSE, offsets[n]);
            gl_.VertexArrayAttribBinding(vao, n, 0);
            gl_.SamplerParameteri(samplers[n], GL_TEXTURE_WRAP_S, GL_REPEAT);
            gl_.SamplerParameteri(samplers[n], GL_TEXTURE_WRAP_T, GL_REPEAT);
            gl_.SamplerParameteri(samplers[n], GL_TEXTURE_WRAP_R, GL_REPEAT);
            gl_.SamplerParameteri(samplers[n], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            gl_.SamplerParameteri(samplers[n], GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        }
        context_.check("Create masked indexed vertex pipeline");
        // No client/source callback occurred since entry; pure scope FIRST
        // tests independent renderer life before committing names to this.
        scope->pure();
        gl_.DeleteShader(vs); vs=0; gl_.DeleteShader(fs); fs=0;
        masked_program_=program; program=0; masked_vao_=vao; vao=0;
        masked_buffer_=buffer; buffer=0; masked_index_buffer_=index_buffer; index_buffer=0;
        masked_samplers_=samplers; samplers.fill(0);
        masked_viewport_=viewport; masked_registers_=registers;
    } catch (...) { cleanup(); life->failed=true; throw; }
}

// Genuine resource factory only. UV/clip/phase/tick remain actual publisher
// obligations. These private values never construct a public paint authority.
struct UiScreenGlRenderer::MaskedTexture {
    const graphics::detail::NotificationRawUiTextureSource source;
    graphics::GlTexture atlas;
    std::optional<graphics::GlTexture> index;
    MaskedTexture(graphics::detail::NotificationRawUiTextureSource s,
        graphics::GlTexture a,std::optional<graphics::GlTexture> i)
      :source(std::move(s)),atlas(std::move(a)),index(std::move(i)){}
};
struct UiScreenGlRenderer::LayoutResourceEntry {
    const std::shared_ptr<const detail::NotificationLayoutTextureKeeper> keeper;
    const std::weak_ptr<BnwClientSceneFrame> scene;
    const std::weak_ptr<const SelectedBnwPreviewHost> host;
    const std::weak_ptr<const assets::CivisResourceBankOwner> bank;
    // SAME canonical retained DDS, never a second key resolution or Image alias.
    mutable std::optional<graphics::GlTexture> button_texture;
    LayoutResourceEntry(std::shared_ptr<const detail::NotificationLayoutTextureKeeper> k,
        std::weak_ptr<BnwClientSceneFrame> s,std::weak_ptr<const SelectedBnwPreviewHost> h,
        std::weak_ptr<const assets::CivisResourceBankOwner> b)
      :keeper(std::move(k)),scene(std::move(s)),host(std::move(h)),bank(std::move(b)){}
};
void UiScreenGlRenderer::with_notification_layout_resources_owned(
    const BnwWorldViewRendererProjectionLoan& projection,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
    const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& actual_trees,
    const std::function<void()>& target){
    using Scope=detail::NotificationLayoutResourceScope;
    using Keeper=detail::NotificationLayoutTextureKeeper;
    const auto life=worldview_lifetime_;
    const auto first=[&]{
        if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
           life->renderer!=this||!life->active||life->active_scope)
            throw std::runtime_error("Layout resources need actual pre-paint entered renderer/source scope");
    };
    first();
    if(layout_resource_active_->exchange(true)){
        life->failed=true;throw std::runtime_error("Layout resource initializer scope reentered");
    }
    struct EndActive{std::shared_ptr<std::atomic<bool>> p;~EndActive(){p->store(false);}}active{layout_resource_active_};
    const auto serial=operation.operation_serial();first();
    // Preserve the actual five original NativeIssuer identities before callbacks;
    // they are required inclusions, not the complete physical resource cohort.
    std::array<std::weak_ptr<const BnwWorldViewControlTree>,5> original_trees;
    for(std::size_t n=0;n<original_trees.size();++n){
        if(!actual_trees[n]){life->failed=true;throw std::runtime_error("Actual layout NativeIssuer slot absent");}
        original_trees[n]=actual_trees[n];
    }
    std::vector<std::weak_ptr<const BnwWorldViewControlTree>> trees;
    std::weak_ptr<BnwClientSceneFrame> scene_origin;
    std::weak_ptr<const SelectedBnwPreviewHost> host_origin;
    std::weak_ptr<const assets::CivisResourceBankOwner> bank_origin;
    bool origin_captured=false;
    const auto source_origin=[&]{
        first();projection.require_current_without_callbacks();first();
        if(operation.operation_serial()!=serial)throw std::runtime_error("Layout resource actual Source serial changed");
        {
            const auto scene=projection.scene_.lock();
            if(!scene||scene->gui_thread_!=std::this_thread::get_id()||scene->gui_self_.expired()||
               scene.owner_before(scene->gui_self_)||scene->gui_self_.owner_before(scene)||
               scene->gui_self_.use_count()<=static_cast<long>(scene->gui_publication_pins_.load())+1)
                throw std::runtime_error("Layout resource actual Scene outside owner retired");
            const auto& selected=scene->selected_host_;
            if(!selected)throw std::runtime_error("Layout resource actual selected Host absent");
            const auto host=selected->retain_civis_resource_host_owned();
            if(host.get()!=selected.get()||host.owner_before(selected)||selected.owner_before(host))
                throw std::runtime_error("Layout resource actual Host canonical control changed");
            const auto& bank=host->civis_resource_bank_;
            if(!bank)throw std::runtime_error("Layout resource actual Bank absent");
            const auto canonical=bank->retain_owner();
            if(canonical.get()!=bank.get()||canonical.owner_before(bank)||bank.owner_before(canonical))
                throw std::runtime_error("Layout resource Bank canonical control changed");
            host->require_same_civis_resources(*bank);
            if(&bank->resources()!=&resources_)throw std::runtime_error("Layout resource actual table is not SAME Bank");
            const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
            if(origin_captured&&(scene_origin.expired()||host_origin.expired()||bank_origin.expired()||
               !same(scene_origin,scene)||!same(host_origin,host)||!same(bank_origin,bank)))
                throw std::runtime_error("Layout resource Scene/Host/Bank origin changed");
            // Only weak metadata survives this checker block. The real native
            // GUI/Source lender supplies every outside owner, not these pins.
            scene_origin=scene;host_origin=host;bank_origin=bank;origin_captured=true;
        }
        first();projection.require_current_without_callbacks();first();
        if(origin_captured&&(scene_origin.expired()||host_origin.expired()||bank_origin.expired()))
            throw std::runtime_error("Layout resource weak origin retired after checker release");
    };
    const auto full=[&]{
        source_origin();operation.require_operation(serial);first();source_origin();
        gui.require_current();first();source_origin();window.require_current();first();source_origin();
    };
    std::shared_ptr<Scope> scope;
    std::vector<std::unique_ptr<BnwWorldViewControlTree::LayoutResourceBinding>> bindings;
    std::unique_ptr<std::function<void()>> owned;
    // Returned real facades survive binding/Scope cleanup, including failures.
    std::vector<std::shared_ptr<BnwWorldViewControlTree>> resource_cohort;
    try{
        full();
        resource_cohort=operation.selected_client_retained_context_trees_owned();first();full();
        if(resource_cohort.empty())throw std::runtime_error("Actual constructed layout resource cohort absent");
        trees.reserve(resource_cohort.size());
        for(const auto& tree:resource_cohort){
            first();source_origin();
            if(!tree)throw std::runtime_error("Actual constructed layout resource Tree absent");
            trees.emplace_back(tree);
        }
        for(const auto& weak:original_trees){
            first();source_origin();
            const auto original=weak.lock();
            if(!original)throw std::runtime_error("Original layout NativeIssuer Tree retired");
            bool included=false;
            for(const auto& tree:resource_cohort)
                if(tree.get()==original.get()&&!tree.owner_before(original)&&!original.owner_before(tree))included=true;
            if(!included)throw std::runtime_error("Actual layout cohort omits/substitutes an original NativeIssuer Tree");
        }
        first();source_origin();
        // Only weak actual returned membership is read by this pure checker.
        // Never enumerate/create Contexts or call the accessor from the checker.
        const auto pure=[&](const auto& actual_operation,const auto& actual_tree){
            source_origin();
            if(&actual_operation!=&operation||actual_operation.operation_serial()!=serial)
                throw std::runtime_error("Layout raw keeper used by another actual Source operation");
            bool member=false;
            for(const auto& weak:trees){
                const auto tree=weak.lock();
                if(!tree)throw std::runtime_error("Layout actual NativeIssuer Tree retired");
                if(tree.get()==actual_tree){
                    tree->require_layout_resource_operation_without_callbacks(operation);member=true;
                }
            }
            if(!member)throw std::runtime_error("Layout raw keeper Tree is not actual NativeIssuer member");
            // All temporary Tree checker locks die before any full callback.
            source_origin();
        };
        const auto entry=[&](const std::shared_ptr<const Keeper>& keeper){
            source_origin();Keeper::require_canonical(keeper);
            const auto found=layout_resource_entries_.find(keeper->key_);
            if(found==layout_resource_entries_.end())throw std::runtime_error("Layout retained raw entry absent");
            const auto& selected=found->second;
            if(selected->keeper.get()!=keeper.get()||selected->keeper.owner_before(keeper)||
               keeper.owner_before(selected->keeper)||!keeper->entry_revision_||
               selected->scene.owner_before(scene_origin)||scene_origin.owner_before(selected->scene)||
               selected->host.owner_before(host_origin)||host_origin.owner_before(selected->host)||
               selected->bank.owner_before(bank_origin)||bank_origin.owner_before(selected->bank))
                throw std::runtime_error("Layout raw keeper entry/revision/actual owner lineage changed");
            source_origin();
        };
        const auto load=[&](const std::string& key)->std::shared_ptr<const Keeper>{
            full();
            if(const auto old=layout_resource_entries_.find(key);old!=layout_resource_entries_.end()){
                auto actual=old->second->keeper;entry(actual);full();return actual;
            }
            auto source=graphics::detail::NotificationRawUiTextureSource::load(resources_,key,full);
            first();source_origin();
            // 4B7BD0+8E8970+8E69C0:114 is HEIGHT;118 is WIDTH.
            std::uint64_t height=source.binding_.source.height(),width=source.binding_.source.width();
            if(source.index_supplier_){
                height=std::uint64_t(source.binding_.index->height())*source.block_size_;
                width=std::uint64_t(source.binding_.index->width())*source.block_size_;
            }
            if(!height||!width||height>INT32_MAX||width>INT32_MAX||layout_resource_revision_==UINT64_MAX)
                throw std::runtime_error("Layout native114/118 resource dimensions/history outside selected domain");
            full(); // LAST observer/source callback before real cache insertion.
            const auto revision=layout_resource_revision_+1;
            auto keeper=std::shared_ptr<const Keeper>(new Keeper(std::move(source),key,
                {static_cast<std::uint32_t>(height),static_cast<std::uint32_t>(width)},revision));
            keeper->canonical_=keeper;
            auto selected=std::make_shared<const LayoutResourceEntry>(keeper,scene_origin,host_origin,bank_origin);
            const auto added=layout_resource_entries_.emplace(key,std::move(selected));
            if(!added.second)throw std::runtime_error("Layout raw source cache changed during same-thread capture");
            layout_resource_revision_=revision;entry(keeper);return keeper;
        };
        scope=std::shared_ptr<Scope>(new Scope(pure,full,load,entry));
        for(const auto& tree:resource_cohort){
            first();source_origin();
            bindings.push_back(tree->enter_layout_resource_scope_owned(scope,operation,tree));
        }
        full();owned=std::make_unique<std::function<void()>>(target);first();full();
        if(!*owned)throw std::invalid_argument("Layout resource initializer target absent");
        (*owned)();first();full();owned.reset();first();full();
        for(const auto& binding:bindings)binding->require_current_without_callbacks();
        bindings.clear();first();source_origin();
        scope->retire_without_callbacks();scope.reset();first();source_origin();
        // Binding destructors are noexcept and poison their retained Core on
        // a failed destructor bookend. Refuse that poison before returning.
        for(const auto& tree:resource_cohort){first();tree->require_layout_resource_operation_without_callbacks(operation);}
        resource_cohort.clear();first();source_origin();
    }catch(...){
        note_active_worldview_exception("layout_resources");
        // Target destruction remains inside real lender lifetime even on
        // unwind. FIRST precedes every raw renderer/Scene/source read.
        owned.reset();
        try{first();source_origin();}catch(...){}
        bindings.clear();if(scope)scope->retire_without_callbacks();scope.reset();
        resource_cohort.clear();
        try{first();source_origin();}catch(...){}
        life->failed=true;throw;
    }
}

void UiScreenGlRenderer::require_masked_resources_origin_without_callbacks()const {
    const auto life=worldview_lifetime_;
    if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||life->renderer!=this||!life->active)
        throw std::runtime_error("Masked resources renderer origin retired");
    const auto* scope=worldview_scope_without_callbacks();
    if(!scope)throw std::runtime_error("Masked resources outside real WorldView scope");
    scope->pure();
    {
        const auto scene=scope->projection.scene_.lock();
        if(!scene||scene->gui_thread_!=std::this_thread::get_id()||
           scene->gui_self_.expired()||scene.owner_before(scene->gui_self_)||scene->gui_self_.owner_before(scene)||
           scene->gui_self_.use_count()<=static_cast<long>(scene->gui_publication_pins_.load())+1)
            throw std::runtime_error("Masked resource actual Scene outside owner retired");
        const auto& selected=scene->selected_host_;
        if(!selected)throw std::runtime_error("Masked resource actual selected Host absent");
        const auto canonical=selected->retain_civis_resource_host_owned();
        if(canonical.get()!=selected.get()||canonical.owner_before(selected)||selected.owner_before(canonical))
            throw std::runtime_error("Masked actual selected Host factory control differs");
        const auto& bank=canonical->civis_resource_bank_;
        if(!bank)throw std::runtime_error("Masked real selected Host has no own Bank");
        const auto bank_owner=bank->retain_owner();
        if(bank_owner.get()!=bank.get()||bank_owner.owner_before(bank)||bank.owner_before(bank_owner))
            throw std::runtime_error("Masked actual Bank factory control differs");
        canonical->require_same_civis_resources(*bank);
        if(&bank->resources()!=&resources_)throw std::runtime_error("Masked renderer resources table is not SAME selected Bank");
    }
    // All temporary Scene/Host/Bank locks dropped before final pure fence and
    // before caller may enter any source/window/observer callback boundary.
    scope->pure();
}
const UiScreenGlRenderer::MaskedTexture& UiScreenGlRenderer::masked_texture_owned(const std::string& key) {
    const auto life=worldview_lifetime_;const auto* scope=worldview_scope_without_callbacks();
    const auto first=[&]{
        if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||!life->active||life->active_scope!=scope)
            throw std::runtime_error("Masked resource callback retired renderer/source scope");
    };
    first();if(!scope)throw std::runtime_error("Masked resources scope absent");
    const auto boundary=[&]{first();scope->pure();require_masked_resources_origin_without_callbacks();
        scope->full();first();require_masked_resources_origin_without_callbacks();scope->pure();};
    try {
        boundary();
        if(const auto it=masked_textures_.find(key);it!=masked_textures_.end())return *it->second;
        auto source=graphics::detail::NotificationRawUiTextureSource::load(resources_,key,boundary);
        first();scope->pure();require_masked_resources_origin_without_callbacks();
        graphics::GlTexture atlas(context_,source.binding_.source,graphics::DdsSamplingColor::linear);
        first();scope->pure();
        std::optional<graphics::GlTexture> index;
        if(source.index_supplier_){index.emplace(source.index_supplier_->upload(context_));first();scope->pure();}
        auto ready=std::make_shared<const MaskedTexture>(std::move(source),std::move(atlas),std::move(index));
        boundary(); // LAST callback BEFORE writing renderer-owned cache.
        const auto inserted=masked_textures_.emplace(key,std::move(ready));
        first();scope->pure();return *inserted.first->second;
    }catch(...){life->failed=true;throw;}
}
UiScreenGlRenderer::MaskedResources UiScreenGlRenderer::masked_resources_owned(
    const BnwWorldViewControlRenderPublication& publication,const WorldViewPaint& paint) {
    const auto life=worldview_lifetime_;const auto* scope=worldview_scope_without_callbacks();
    const auto first=[&]{
        if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||!life->active||life->active_scope!=scope)
            throw std::runtime_error("Masked publication resource owner retired");
    };
    first();try {
        if(!scope||!scope->painting||scope->publication!=&publication||!scope->frame||!paint.masked)
            throw std::runtime_error("Masked resource factory lacks SAME actual publication/paint scope");
        if(masked_resource_active_->exchange(true)){
            life->failed=true;throw std::runtime_error("Masked resource factory reentered");
        }
        struct Exit {std::shared_ptr<std::atomic<bool>> active;~Exit(){active->store(false);}} exit{masked_resource_active_};
        scope->pure();bool member=false;
        for(const auto& actual:scope->frame->paints)if(&actual==&paint){member=true;break;}
        if(!member)throw std::runtime_error("Masked resource paint not canonical composed stream member");
        bool actual_tree=false;
        for(const auto& tree:scope->trees)if(tree.get()==publication.tree_.get()&&
            !tree.owner_before(publication.tree_)&&!publication.tree_.owner_before(tree)){actual_tree=true;break;}
        if(!actual_tree)throw std::runtime_error("Masked publication Tree not actual Native slot");
        const auto actual_control=publication.control_;first();scope->pure();
        if(!actual_control)throw std::runtime_error("Masked indexed publisher has no genuine Control");
        first();scope->pure();
        const auto quote=publication.tree_->inspect_control(*actual_control);first();scope->pure();
        if(!quote.alive||quote.authored.name!="ScrollAnim"||quote.effect_revision!=publication.effect_revision_)
            throw std::runtime_error("Masked resource publisher control not current authored ScrollAnim");
        const auto base=quote.texture.value_or(quote.authored.attribute("Texture"));
        const auto mask=quote.authored.attribute("MaskTexture");
        if(base.empty()||mask.empty()||paint.source_key!=base)
            throw std::runtime_error("Masked actual authored Base/Mask resource absent or substituted");
        const auto& b=masked_texture_owned(base);first();scope->pure();
        const auto& m=masked_texture_owned(mask);first();scope->pure();
        MaskedResources out;
        const std::array<const MaskedTexture*,2> arms{&b,&m};
        for(std::size_t arm=0;arm<2;++arm){const auto& s=*arms[arm];const auto& raw=s.source;
            const auto n=arm*2,c=arm*4;
            // Direct arm never samples IndexTexture; bind the SAME actual atlas
            // there as explicit OWN unused-unit custody, not a fabricated index.
            out.textures[n]=s.index?&*s.index:&s.atlas;out.textures[n+1]=&s.atlas;
            out.keys[n]=s.index?raw.index_requested_:raw.requested_;out.keys[n+1]=raw.requested_;
            out.registers[c][0]=static_cast<float>(raw.block_size_);
            out.registers[c+3][0]=static_cast<float>(raw.binding_.source.width());
            out.registers[c+3][1]=static_cast<float>(raw.binding_.source.height());
            if(raw.index_supplier_){
                out.registers[c+1][0]=raw.index_supplier_->index_scale_x();
                out.registers[c+2][0]=static_cast<float>(raw.binding_.index->width());
                out.registers[c+2][1]=static_cast<float>(raw.binding_.index->height());
                const auto x=std::uint64_t(raw.binding_.index->width())*raw.block_size_;
                const auto y=std::uint64_t(raw.binding_.index->height())*raw.block_size_;
                if(x>UINT32_MAX||y>UINT32_MAX)throw std::runtime_error("Masked logical authored dimensions overflow");
                out.logical_sizes[arm]={static_cast<std::uint32_t>(x),static_cast<std::uint32_t>(y)};
            }else out.logical_sizes[arm]={raw.binding_.source.width(),raw.binding_.source.height()};
        }
        // Only used native lanes above are issued. Other lanes are explicit OWN
        // zero, shader-unused; no assertion that native ring allocation zeroed.
        require_masked_resources_origin_without_callbacks();scope->full();first();
        require_masked_resources_origin_without_callbacks();scope->pure();return out;
    }catch(...){life->failed=true;throw;}
}

void UiScreenGlRenderer::masked_quad_owned(
    const BnwWorldViewControlRenderPublication& publication, const WorldViewPaint& paint,
    const std::array<const graphics::GlTexture*,4>& actual_textures,
    const std::array<std::string_view,4>& actual_source_keys) {
    const auto life = worldview_lifetime_;
    const auto active = masked_draw_active_;
    const auto* scope = worldview_scope_without_callbacks();
    const auto first = [&] {
        if (life->creator!=std::this_thread::get_id() || !life->alive || life->failed ||
            !life->active || life->active_scope!=scope) {
            life->failed=true;
            throw std::runtime_error("Masked renderer retired or changed current scope");
        }
    };
    first();
    if (!scope || !scope->painting || scope->publication!=&publication || !scope->frame) {
        life->failed=true;
        throw std::runtime_error("Masked primitive lacks exact current publication/frame");
    }
    if (active->exchange(true)) {
        life->failed=true; throw std::runtime_error("Masked primitive reentered");
    }
    struct End { std::shared_ptr<std::atomic<bool>> active; ~End(){active->store(false);} } end{active};
    try {
        scope->pure();
        bool member=false;
        for (const auto& candidate:scope->frame->paints) if (&candidate==&paint) {member=true;break;}
        if (!member || !paint.masked) throw std::runtime_error("Masked data not in this exact composed stream");
        // Copy immutable data before callbacks. Independent vertices include
        // clipping and BOTH UV channels; no autoquad/default UV is constructed.
        const auto data=*paint.masked;
        const auto textures=actual_textures;
        for (const auto& vertex:data.vertices) {
            for (const auto value:vertex.pixel_position) if (!std::isfinite(value))
                throw std::runtime_error("Masked pixel vertex nonfinite");
            for (const auto value:vertex.base_uv) if (!std::isfinite(value))
                throw std::runtime_error("Masked Base UV nonfinite");
            for (const auto value:vertex.mask_uv) if (!std::isfinite(value))
                throw std::runtime_error("Masked Mask UV nonfinite");
            for (const auto value:vertex.color0) if (!std::isfinite(value)||value<0||value>1)
                throw std::runtime_error("Masked COLOR0 outside normalized channels");
        }
        for (const auto& reg:data.native_registers) for (const auto value:reg)
            if (!std::isfinite(value)) throw std::runtime_error("Masked native register nonfinite");
        for (std::size_t n=0; n<4; ++n) {
            if (data.resource_keys[n].empty() || actual_source_keys[n]!=data.resource_keys[n] || !textures[n])
                throw std::runtime_error("Masked selected resource tuple absent or differs");
            const auto& lod=data.sampler_lod[n];
            if (!std::isfinite(lod[0]) || !std::isfinite(lod[1]) || !std::isfinite(lod[2]) || lod[0]>lod[1])
                throw std::runtime_error("Masked native sampler LOD state unavailable");
        }
        // Borrowed texture tuple must outlive this enclosing lender, including
        // this full callback boundary. No new strong resource/self pin masks
        // retirement; Root's actual source factory supplies that custody.
        scope->full(); first();
        create_masked_pipeline_owned(); first(); scope->pure();
        const auto& api=context_.api(); first(); scope->pure();
        const auto activate=reinterpret_cast<PFNGLACTIVETEXTUREPROC>(context_.proc_address("glActiveTexture"));
        first(); scope->pure();
        const auto get_indexed=reinterpret_cast<PFNGLGETINTEGERI_VPROC>(context_.proc_address("glGetIntegeri_v"));
        first(); scope->pure();
        if (!activate || !get_indexed) throw std::runtime_error("Masked GL state query functions unavailable");
        const auto width=scope->projection.width_, height=scope->projection.height_;
        if (width<=0 || height<=0) throw std::runtime_error("Masked actual framebuffer absent");
        GLint physical_viewport[4]{}; api.GetIntegerv(GL_VIEWPORT,physical_viewport);
        if (physical_viewport[0]!=0 || physical_viewport[1]!=0 || physical_viewport[2]!=width || physical_viewport[3]!=height)
            throw std::runtime_error("Masked GL viewport differs from actual current frame");
        std::array<GLuint,4> ids{};
        for (std::size_t n=0; n<4; ++n) {
            if (!textures[n]->belongs_to(context_) || textures[n]->face_count()!=1 || textures[n]->depth()!=1)
                throw std::runtime_error("Masked borrowed texture not same context/2D");
            const auto& levels=textures[n]->levels();
            if (levels.empty()) throw std::runtime_error("Masked texture actual mips absent");
            for (std::size_t mip=0; mip<levels.size(); ++mip) {
                const auto& level=levels[mip];
                if (mip>31 || level.face!=0 || level.mip!=mip || !level.width || !level.height ||
                    level.width!=std::max(1u,levels[0].width>>mip) ||
                    level.height!=std::max(1u,levels[0].height>>mip))
                    throw std::runtime_error("Masked actual mip chain not contiguous");
            }
            ids[n]=textures[n]->id();
            GLint base_level=-1,max_level=-1;
            api.GetTextureParameteriv(ids[n],GL_TEXTURE_BASE_LEVEL,&base_level);
            api.GetTextureParameteriv(ids[n],GL_TEXTURE_MAX_LEVEL,&max_level);
            if (base_level!=0 || max_level!=static_cast<GLint>(levels.size()-1))
                throw std::runtime_error("Masked physical texture mip range differs from retained bytes");
        }
        // DSA upload does not mutate the outside VAO/EBO/array-buffer. Still
        // save/restore/check those states, plus only the touched 2D targets.
        struct Restore {
            graphics::GlRenderApi gl;
            graphics::GlApi api;
            PFNGLACTIVETEXTUREPROC activate;
            GLint program=0,vao=0,array_buffer=0,element_buffer=0,active_texture=0;
            std::array<GLint,4> textures{},samplers{};
            bool saved=false;
            Restore(const graphics::GlRenderApi& g,const graphics::GlApi& a,
                    PFNGLACTIVETEXTUREPROC at,PFNGLGETINTEGERI_VPROC indexed):gl(g),api(a),activate(at) {
                api.GetIntegerv(GL_CURRENT_PROGRAM,&program);api.GetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
                api.GetIntegerv(GL_ARRAY_BUFFER_BINDING,&array_buffer);api.GetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&element_buffer);
                api.GetIntegerv(GL_ACTIVE_TEXTURE,&active_texture);
                for (GLuint n=0;n<4;++n) {
                    activate(GL_TEXTURE0+n);api.GetIntegerv(GL_TEXTURE_BINDING_2D,&textures[n]);
                    indexed(GL_SAMPLER_BINDING,n,&samplers[n]);
                }
                activate(static_cast<GLenum>(active_texture));saved=true;
            }
            void restore() noexcept {
                if (!saved) return;
                for (GLuint n=0;n<4;++n) {
                    activate(GL_TEXTURE0+n);api.BindTexture(GL_TEXTURE_2D,static_cast<GLuint>(textures[n]));
                    gl.BindSampler(n,static_cast<GLuint>(samplers[n]));
                }
                activate(static_cast<GLenum>(active_texture));
                gl.UseProgram(static_cast<GLuint>(program));gl.BindVertexArray(static_cast<GLuint>(vao));
                api.BindBuffer(GL_ARRAY_BUFFER,static_cast<GLuint>(array_buffer));saved=false;
                // Original EBO belongs to saved VAO and is never mutated.
            }
            ~Restore(){restore();}
        } restore(gl_,api,activate,get_indexed);
        gl_.NamedBufferData(masked_buffer_,sizeof(data.vertices),data.vertices.data(),GL_STREAM_DRAW);
        gl_.UseProgram(masked_program_);gl_.BindVertexArray(masked_vao_);
        const std::array<float,2> viewport{float(width),float(height)};
        gl_.ProgramUniform2fv(masked_program_,masked_viewport_,1,viewport.data());
        for (std::size_t n=0;n<8;++n)
            gl_.ProgramUniform4fv(masked_program_,masked_registers_[n],1,data.native_registers[n].data());
        for (GLuint n=0;n<4;++n) {
            const auto& lod=data.sampler_lod[n];
            gl_.SamplerParameterf(masked_samplers_[n],GL_TEXTURE_MIN_LOD,lod[0]);
            gl_.SamplerParameterf(masked_samplers_[n],GL_TEXTURE_MAX_LOD,lod[1]);
            gl_.SamplerParameterf(masked_samplers_[n],GL_TEXTURE_LOD_BIAS,lod[2]);
            activate(GL_TEXTURE0+n);api.BindTexture(GL_TEXTURE_2D,ids[n]);gl_.BindSampler(n,masked_samplers_[n]);
        }
        first();scope->pure();
        // Query the actual current own VAO/EBO join before indexed dispatch.
        // Binding this private VAO never changes the saved caller VAO's EBO.
        GLint current_indices=0;api.GetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&current_indices);
        if (static_cast<GLuint>(current_indices)!=masked_index_buffer_)
            throw std::runtime_error("Masked indexed quad own EBO substituted");
        gl_.DrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,nullptr);
        first();scope->pure();
        restore.restore();
        GLint actual_ebo=-1;api.GetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&actual_ebo);
        if (actual_ebo!=restore.element_buffer) throw std::runtime_error("Masked draw altered outside VAO EBO");
        context_.check("Masked indexed quad and scoped GL state restoration");
        first();scope->pure();
        // State restored BEFORE any source/client callback. FIRST local life
        // POST follows callbacks; no borrowed renderer/texture used afterward.
        scope->full();first();
    } catch (...) { life->failed=true; throw; }
}

void UiScreenGlRenderer::grid(const WorldViewPaint &paint, std::string_view name,
                              UiGlPaintReport &report, const WorldViewRect &clip) {
    const auto *grid = style(name, report);
    if (!grid) {
        quad(white_, paint.destination, {0, 0, 1, 1}, 0x0d1522d0, false, clip);
        ++report.rectangles;
        return;
    }
    const auto *image = texture(grid->texture, report);
    if (!image) {
        quad(white_, paint.destination, {0, 0, 1, 1}, 0x0d1522d0, false, clip);
        ++report.rectangles;
        return;
    }
    const auto image_width = float(image->levels().front().width);
    const auto image_height = float(image->levels().front().height);
    const auto left = grid->source[0].width, right = grid->source[2].width;
    const auto top = grid->source[0].height, bottom = grid->source[6].height;
    const auto &r = paint.destination;
    if (left < 0 || right < 0 || top < 0 || bottom < 0 ||
        r.width < left + right || r.height < top + bottom) {
        report.unresolved.push_back("Grid9 destination is smaller than authored corners: " +
                                    std::string(name));
        quad(white_, r, {0, 0, 1, 1}, 0x0d1522d0, false, clip);
        ++report.rectangles;
        return;
    }
    const std::array<std::int32_t, 4> xs{r.x, r.x + left, r.x + r.width - right, r.x + r.width};
    const std::array<std::int32_t, 4> ys{r.y, r.y + top, r.y + r.height - bottom, r.y + r.height};
    for (std::size_t i = 0; i < 9; ++i) {
        const auto &s = grid->source[i];
        if (s.x < 0 || s.y < 0 || s.width < 0 || s.height < 0 ||
            s.x + s.width > image_width || s.y + s.height > image_height) {
            report.unresolved.push_back("Grid9 source crop exceeds selected DDS: " +
                                        std::string(name));
            return;
        }
        const auto col = i % 3, row = i / 3;
        quad(*image, {xs[col], ys[row], xs[col + 1] - xs[col], ys[row + 1] - ys[row]},
             {s.x / image_width, s.y / image_height,
              s.width / image_width, s.height / image_height}, paint.rgba, false, clip, paint.tint);
    }
    ++report.rectangles;
}

void UiScreenGlRenderer::text(const WorldViewPaint &input, UiGlPaintReport &report,
                              const WorldViewRect &inherited_clip) {
    // Pin command values before resolver callbacks. Actual caller retains the
    // style set/geometry/texture owner through paint, as in the existing path.
    const auto* fence=worldview_scope_without_callbacks();
    if(fence)fence->full();
    auto selected_paint=input;
    std::optional<UnitNameOwnFontSelection> selected_name;
    const UiGlyphStyleBinding *selected_style=nullptr;
    if(selected_paint.unit_name_font_selection) {
        if (!selected_paint.font_style || !font_styles_ ||
            selected_paint.font_style->font!="TwCenMT24" ||
            selected_paint.font_style->style!="SoftShadow" ||
            selected_paint.style!="TwCenMT24:center" ||
            selected_paint.destination.width!=selected_paint.unit_name_font_selection->button_width) {
            report.unresolved.push_back("Current UnitName font-selection command/style/width differs");return;
        }
        std::array<const UiGlyphStyleBinding*,3> bindings{};
        std::array<const GlyphGenGeometry*,3> geometries{};
        try {
            for(std::size_t n=0;n<bindings.size();++n) {
                bindings[n]=resolve_worldview_font_style(unit_name_source_fonts[n],selected_paint.font_style->style);
                geometries[n]=bindings[n]?bindings[n]->layout_geometry:nullptr;
            }
            selected_name=select_unit_name_own_glyphgen_font(*selected_paint.unit_name_font_selection,
                selected_paint.text,selected_paint.font_style->style,geometries);
        } catch(const UnitNameOwnFontSelectionError &error) {
            report.unit_name_font_refusals.push_back(error.refusal);
            report.unresolved.push_back("Own UnitName font selection refused: "+std::string(error.what()));return;
        } catch(const std::exception &error) {
            report.unresolved.push_back("Own UnitName font selection refused: "+std::string(error.what()));return;
        }
        selected_style=bindings[selected_name->selected_index];
        selected_paint.font_style->font=std::string(unit_name_source_fonts[selected_name->selected_index]);
        selected_paint.style=selected_paint.font_style->font+":center";
    }
    const auto &paint=selected_paint;
    const auto *styled = selected_style ? selected_style : paint.font_style && font_styles_ ?
        resolve_worldview_font_style(paint.font_style->font,paint.font_style->style) : nullptr;
    if(paint.font_style&&(!styled||!styled->layout_geometry||styled->layers.empty())) {
        report.unresolved.push_back("Active authored font style is unavailable: "+paint.font_style->font+" "+paint.font_style->style);
        return;
    }
    if (styled && !complete_style_layers(*styled,*paint.font_style)) {
        report.unresolved.push_back("Styled font layer/color binding is incomplete");
        return;
    }
    if (selected_name) report.unit_name_own_font_selections.push_back(*selected_name);
    const auto font = styled ? UiGlyphBinding{styled->layout_geometry,styled->layers.front().glyphs.texture} :
        paint.style.find("16") != std::string::npos ? font16_ :
        (paint.style.find("24") != std::string::npos && font24_.geometry ? font24_ : font20_);
    const bool centered = paint.style.find(":center") != std::string::npos;
    const auto &bounds = paint.destination;
    const auto visible = clip_ui_quad(bounds, {0,0,1,1}, inherited_clip);
    if (!visible) return;
    const auto &clip = visible->destination;
    const auto render_line = [&](std::string_view value, std::int32_t y) {
        const auto baseline = y + std::min(font.geometry->line_height,
                                            std::max(1, bounds.height));
        const bool markup = value.find("[ICON_") != std::string_view::npos ||
                            value.find("[COLOR") != std::string_view::npos ||
                            value.find("[/COLOR") != std::string_view::npos ||
                            value.find("[ENDCOLOR]") != std::string_view::npos;
        if(styled) {
            if(markup) {
                report.unresolved.push_back("Styled CityBanner markup is unresolved");return;
            }
            GlyphGenLine line;
            try {line=selected_name?selected_name->draw_line:project_glyphgen_line(*styled->layout_geometry,value);}
            catch(const std::exception &error) {
                report.unresolved.push_back("Selected styled font cannot encode UI text: "+std::string(value)+": "+error.what());return;
            }
            const auto origin_x=bounds.x+(centered?std::max(0,(bounds.width-line.advance_width)/2):0);
            // Source declarations assign foreground color0; effects have ink
            // beneath opaque Base and are composited behind that foreground.
            for(auto layer=styled->layers.rbegin();layer!=styled->layers.rend();++layer) {
                if(!layer->glyphs.geometry||!layer->glyphs.texture||layer->color_index>=paint.font_style->colors.size()) {
                    report.unresolved.push_back("Styled font layer binding is incomplete");continue;
                }
                const auto tint=paint.font_style->colors[layer->color_index];
                const bool floating=paint.font_style->float_colors &&
                    layer->color_index<paint.font_style->float_colors->size();
                if(!tint&&layer->color_index!=0&&!floating) {
                    report.unresolved.push_back("Styled font effect color has no current source owner");continue;
                }
                for(const auto &glyph:line.glyphs) {
                    const auto &metrics=styled->layout_geometry->glyphs;
                    const auto base=std::find_if(metrics.begin(),metrics.end(),[&](const auto &g){return g.code_point==glyph.code_point;});
                    const auto &layer_metrics=layer->glyphs.geometry->glyphs;
                    const auto effect=std::find_if(layer_metrics.begin(),layer_metrics.end(),[&](const auto &g){return g.code_point==glyph.code_point;});
                    if(effect==layer_metrics.end()) {
                        report.unresolved.push_back("Authored font effect has no runtime locale glyph: "+std::to_string(glyph.code_point));continue;
                    }
                    const WorldViewRect dest{origin_x+glyph.left_from_line_origin+effect->origin_x-base->origin_x,
                        baseline-effect->origin_y,effect->width,effect->height};
                    quad(*layer->glyphs.texture,dest,
                        {float(effect->atlas_x)/layer->glyphs.geometry->atlas_width,float(effect->atlas_y)/layer->glyphs.geometry->atlas_height,
                         float(effect->width)/layer->glyphs.geometry->atlas_width,float(effect->height)/layer->glyphs.geometry->atlas_height},
                        tint.value_or(paint.rgba),true,clip,
                        floating?std::optional<UiFloatColor>{(*paint.font_style->float_colors)[layer->color_index]}:
                            layer->color_index==0?paint.tint:std::nullopt);
                    ++report.glyphs;
                }
            }
            return;
        }
        if (markup) {
            if (!icons_.geometry) {
                report.unresolved.push_back("Active FontIcons binding is unavailable for UI text: " +
                                            std::string(value));
                return;
            }
            GlyphGenInlineLine line;
            try {
                line = project_glyphgen_markup(*font.geometry, *icons_.geometry,
                                               value, fence ? worldview_icon_resolver() : icon_resolver_);
                if(fence)fence->full();
            } catch (const std::exception &error) {
                report.unresolved.push_back("Active GlyphGen markup cannot encode UI text: " +
                                            std::string(value) + ": " + error.what());
                return;
            }
            const auto origin_x = bounds.x + (centered ?
                std::max(0, (bounds.width - line.advance_width) / 2) : 0);
            for (const auto &glyph : line.glyphs) {
                const auto binding = glyph.icon ? icons_ : font;
                const WorldViewRect dest{origin_x + glyph.left_from_line_origin,
                                         baseline - glyph.top_from_baseline,
                                         glyph.width, glyph.height};
                const auto multiply = [](std::uint32_t base, std::uint8_t channel,
                                         unsigned shift) {
                    return (((base >> shift) & 255u) * channel + 127u) / 255u;
                };
                const std::uint32_t tint =
                    (multiply(paint.rgba, glyph.color[0], 24) << 24) |
                    (multiply(paint.rgba, glyph.color[1], 16) << 16) |
                    (multiply(paint.rgba, glyph.color[2], 8) << 8) |
                    multiply(paint.rgba, glyph.color[3], 0);
                quad(*binding.texture, dest,
                     {float(glyph.atlas_x) / binding.geometry->atlas_width,
                      float(glyph.atlas_y) / binding.geometry->atlas_height,
                      float(glyph.width) / binding.geometry->atlas_width,
                      float(glyph.height) / binding.geometry->atlas_height},
                     tint, !glyph.icon, clip);
                if (glyph.icon) ++report.icons;
                else ++report.glyphs;
            }
            return;
        }
        GlyphGenLine line;
        try {
            line = project_glyphgen_line(*font.geometry, value);
        } catch (const std::exception &) {
            report.unresolved.push_back("Active GlyphGen font cannot encode UI text: " +
                                        std::string(value));
            line = project_glyphgen_line(*font.geometry, "N/A");
        }
        const auto origin_x = bounds.x + (centered ?
            std::max(0, (bounds.width - line.advance_width) / 2) : 0);
        for (const auto &glyph : line.glyphs) {
            const WorldViewRect dest{origin_x + glyph.left_from_line_origin,
                                     baseline - glyph.top_from_baseline,
                                     glyph.width, glyph.height};
            quad(*font.texture, dest,
                 {float(glyph.atlas_x) / font.geometry->atlas_width,
                  float(glyph.atlas_y) / font.geometry->atlas_height,
                  float(glyph.width) / font.geometry->atlas_width,
                  float(glyph.height) / font.geometry->atlas_height}, paint.rgba, true, clip);
            ++report.glyphs;
        }
    };
    std::size_t start = 0;
    std::int32_t y = bounds.y;
    while (start <= paint.text.size() && y < bounds.y + bounds.height) {
        const auto next = paint.text.find('\n', start);
        const auto length = next == std::string::npos ? paint.text.size() - start : next - start;
        render_line(std::string_view(paint.text).substr(start, length), y);
        if (next == std::string::npos) break;
        start = next + 1;
        y += font.geometry->line_height;
    }
}

std::optional<WorldViewRect> UiScreenGlRenderer::anchored_text(
    const WorldViewPaint &paint, UiGlPaintReport &report, const WorldViewRect &clip) {
    const auto* fence=worldview_scope_without_callbacks();
    if(fence)fence->full();
    const auto *styled = paint.font_style && font_styles_ ?
        resolve_worldview_font_style(paint.font_style->font,paint.font_style->style) : nullptr;
    if (!styled || !styled->layout_geometry || styled->layers.empty() ||
        !paint.font_style->colors[0] || !paint.font_style->colors[1] ||
        !complete_style_layers(*styled,*paint.font_style)) {
        report.unresolved.push_back("Anchored TextButton lacks its selected font/style/colors");
        return std::nullopt;
    }
    struct Glyph { bool icon; std::uint32_t code; WorldViewRect atlas;
        std::int32_t left, top; std::uint32_t rgba; };
    std::vector<Glyph> glyphs;
    std::int32_t advance=0;
    const auto &geometry=*styled->layout_geometry;
    const bool markup=paint.text.find('[')!=std::string::npos;
    try {
        if (markup) {
            if (!icons_.geometry || !icons_.texture || *paint.font_style->colors[0]!=0xffffc8ffu) {
                report.unresolved.push_back("Anchored markup requires source Beige foreground and current FontIcons");
                return std::nullopt;
            }
            const auto line=project_glyphgen_markup(geometry,*icons_.geometry,paint.text,fence?worldview_icon_resolver():icon_resolver_);
            if(fence)fence->full();
            advance=line.advance_width;
            for (const auto &g:line.glyphs) {
                const auto rgba=(std::uint32_t(g.color[0])<<24)|(std::uint32_t(g.color[1])<<16)|
                    (std::uint32_t(g.color[2])<<8)|g.color[3];
                glyphs.push_back({g.icon,g.code_point,{g.atlas_x,g.atlas_y,g.width,g.height},
                    g.left_from_line_origin,g.top_from_baseline,rgba});
            }
        } else {
            const auto line=project_glyphgen_line(geometry,paint.text);
            advance=line.advance_width;
            for (const auto &g:line.glyphs) glyphs.push_back({false,g.code_point,
                {g.atlas_x,g.atlas_y,g.width,g.height},g.left_from_line_origin,g.top_from_baseline,
                *paint.font_style->colors[0]});
        }
    } catch (const std::exception &error) {
        report.unresolved.push_back("Anchored TextButton cannot project selected text: "+std::string(error.what()));
        return std::nullopt;
    }
    struct Quad { const graphics::GlTexture *texture; const GlyphGenGeometry *geometry;
        WorldViewRect relative,atlas; std::uint32_t rgba; std::optional<UiFloatColor> tint; bool icon; };
    const auto checked=[](std::int64_t value) {
        if(value<std::numeric_limits<std::int32_t>::min() || value>std::numeric_limits<std::int32_t>::max())
            throw std::overflow_error("Anchored glyph coordinate exceeds int32");
        return static_cast<std::int32_t>(value);
    };
    std::vector<Quad> quads;
    std::vector<WorldViewRect> relative;
    bool authored_complete=true;
    for (auto layer=styled->layers.rbegin();layer!=styled->layers.rend();++layer) {
        if (!layer->glyphs.geometry || !layer->glyphs.texture || layer->color_index>=paint.font_style->colors.size()) {
            report.unresolved.push_back("Anchored font layer binding is incomplete");
            authored_complete=false;
            continue;
        }
        for (const auto &g:glyphs) {
            if (g.icon) continue;
            const auto base=std::find_if(geometry.glyphs.begin(),geometry.glyphs.end(),
                [&](const auto &m){return m.code_point==g.code;});
            const auto &metrics=layer->glyphs.geometry->glyphs;
            const auto effect=std::find_if(metrics.begin(),metrics.end(),
                [&](const auto &m){return m.code_point==g.code;});
            if (base==geometry.glyphs.end() || effect==metrics.end()) {
                report.unresolved.push_back("Anchored authored effect lacks locale glyph: "+std::to_string(g.code));
                authored_complete=false;
                continue;
            }
            const auto left=std::int64_t(g.left)+effect->origin_x-base->origin_x;
            if (left<std::numeric_limits<std::int32_t>::min() || left>std::numeric_limits<std::int32_t>::max())
                throw std::overflow_error("Anchored effect origin exceeds UI coordinates");
            const WorldViewRect rect{static_cast<std::int32_t>(left),checked(-std::int64_t(effect->origin_y)),effect->width,effect->height};
            const auto tint=paint.font_style->float_colors &&
                layer->color_index<paint.font_style->float_colors->size() ?
                std::optional<UiFloatColor>{(*paint.font_style->float_colors)[layer->color_index]} : std::nullopt;
            quads.push_back({layer->glyphs.texture,layer->glyphs.geometry,rect,
                {effect->atlas_x,effect->atlas_y,effect->width,effect->height},
                layer->color_index==0?g.rgba:paint.font_style->colors[layer->color_index].value_or(0xffffffffu),tint,false});
            relative.push_back(rect);
        }
    }
    bool has_icons=false;
    for (const auto &g:glyphs) if (g.icon) {
        const WorldViewRect rect{g.left,checked(-std::int64_t(g.top)),g.atlas.width,g.atlas.height};
        quads.push_back({icons_.texture,icons_.geometry,rect,g.atlas,g.rgba,std::nullopt,true});
        relative.push_back(rect);has_icons=true;
    }
    if (has_icons) report.unresolved.push_back("Anchored FontIcons foreground rendered; native style-effect route remains unverified");
    AnchoredTextMeasurement measured;
    try { measured=measure_right_center_text_quads(paint.destination,advance,geometry.line_height,
        geometry.line_height,relative,clip); }
    catch (const std::exception &error) {
        report.unresolved.push_back("Anchored TextButton geometry rejected: "+std::string(error.what()));
        return std::nullopt;
    }
    for (const auto &q:quads) {
        if(q.relative.width<=0 || q.relative.height<=0) continue;
        const WorldViewRect dest{measured.origin_x+q.relative.x,measured.baseline_y+q.relative.y,
            q.relative.width,q.relative.height};
        if (!clip_ui_quad(dest,{0,0,1,1},clip)) continue;
        quad(*q.texture,dest,
            {float(q.atlas.x)/q.geometry->atlas_width,float(q.atlas.y)/q.geometry->atlas_height,
             float(q.atlas.width)/q.geometry->atlas_width,float(q.atlas.height)/q.geometry->atlas_height},
            q.rgba,!q.icon,clip,q.tint);
        if(q.icon) ++report.icons; else ++report.glyphs;
    }
    if(!authored_complete) return std::nullopt;
    return measured.clipped_glyph_bounds;
}

void UiScreenGlRenderer::paint_readonly_tooltip(const TopPanelTooltipPlan &plan,
    const TopPanelGrid9Style &selected,UiGlPaintReport &report,std::int32_t width,std::int32_t height) {
    if(worldview_lifetime_->active){worldview_lifetime_->failed=true;throw std::runtime_error("WorldView callback attempted foreign tooltip paint");}
    if(width<=0 || height<=0 || report.readonly_text.frame.framebuffer_width!=width ||
        report.readonly_text.frame.framebuffer_height!=height)
        throw std::invalid_argument("Readonly tooltip requires the preceding paint's positive framebuffer extent");
    const WorldViewRect clip{0,0,width,height};
    const auto *background=texture(selected.texture,report);
    if(!background || !font16_.geometry || !font16_.texture || !icons_.geometry || !icons_.texture)
        throw std::runtime_error("Readonly tooltip lacks retained TTManager font/icon/texture owners");
    const auto rectangle=[](const auto &r) {
        constexpr auto limit=std::uint32_t(std::numeric_limits<std::int32_t>::max());
        if(r.x>limit || r.y>limit || r.width>limit || r.height>limit)
            throw std::overflow_error("Readonly tooltip bounds exceed UI coordinates");
        return WorldViewRect{static_cast<std::int32_t>(r.x),static_cast<std::int32_t>(r.y),
            static_cast<std::int32_t>(r.width),static_cast<std::int32_t>(r.height)};
    };
    const auto texture_width=background->levels().front().width,texture_height=background->levels().front().height;
    const auto tw=float(texture_width),th=float(texture_height);
    for(std::size_t i=0;i<plan.grid.destination.size();++i) {
        const auto &source=selected.source[i];
        const auto source_x=std::int64_t(source.x)+selected.state_offset.x;
        const auto source_y=std::int64_t(source.y)+selected.state_offset.y;
        if(source_x<0 || source_y<0 || source.width<0 || source.height<0 ||
            source_x+source.width>texture_width || source_y+source.height>texture_height)
            throw std::runtime_error("Readonly tooltip Grid9 crop exceeds its actual texture");
        quad(*background,rectangle(plan.grid.destination[i]),
            {float(source_x)/tw,float(source_y)/th,
             source.width/tw,source.height/th},0xffffffffu,false,clip);
        ++report.textures;
    }
    for(std::size_t i=0;i<plan.lines.size();++i) for(const auto &g:plan.lines[i].glyphs) {
        if(g.width<=0 || g.height<=0) continue;
        const auto binding=g.icon?icons_:font16_;
        const auto x=std::int64_t(plan.text_origin.x)+g.left_from_line_origin;
        const auto y=std::int64_t(plan.text_origin.y)+std::int64_t(i)*plan.line_step-g.top_from_baseline;
        if(x<0 || y<0 || x+g.width>width || y+g.height>height)
            throw std::runtime_error("Readonly tooltip projected glyph exceeds current framebuffer");
        const auto rgba=(std::uint32_t(g.color[0])<<24)|(std::uint32_t(g.color[1])<<16)|
            (std::uint32_t(g.color[2])<<8)|g.color[3];
        quad(*binding.texture,{static_cast<std::int32_t>(x),static_cast<std::int32_t>(y),g.width,g.height},
            {float(g.atlas_x)/binding.geometry->atlas_width,float(g.atlas_y)/binding.geometry->atlas_height,
             float(g.width)/binding.geometry->atlas_width,float(g.height)/binding.geometry->atlas_height},
            rgba,!g.icon,clip);
        if(g.icon) ++report.icons;else ++report.glyphs;
    }
    context_.check("Paint current readonly UnitPanel tooltip");
}

UiGlPaintReport UiScreenGlRenderer::paint(const ComposedUiFrame &frame,
                                          std::int32_t width, std::int32_t height) {
    // Reject the entire closed carrier before Source, scope mutation, report
    // allocation, generic GL setup or any legacy integer geometry consumer.
    // Actual caption publication paint uses its separate preGL embedded route.
    for(const auto& item:frame.paints)
        if(item.normal_caption_fp32_rect)
            throw std::runtime_error("Closed caption frame cannot enter public legacy painter");
    auto* fence=const_cast<WorldViewPaintScope*>(worldview_scope_without_callbacks());
    if(worldview_lifetime_->active&&!fence){
        worldview_lifetime_->failed=true;throw std::runtime_error("WorldView paint before actual native scope issuer");
    }
    if(fence&&(fence->painting||fence->frame!=&frame)){
        fence->life->failed=true;throw std::runtime_error("WorldView paint has no exact owned frame loan or reentered");
    }
    if(fence)fence->painting=true;
    struct EndPaint{WorldViewPaintScope* scope;~EndPaint(){if(scope)scope->painting=false;}} end_paint{fence};
    if(fence)fence->full();
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("UI screen painter viewport must be positive");
    UiGlPaintReport report;
    report.unresolved = frame.unresolved;
    report.readonly_text.frame={frame.frame_owner,frame.frame_sequence,frame.snapshot_frame,width,height};
    gl_.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_.Viewport(0, 0, width, height);
    gl_.Disable(GL_DEPTH_TEST);
    gl_.Disable(GL_CULL_FACE);
    gl_.Disable(GL_FRAMEBUFFER_SRGB);
    gl_.Enable(GL_BLEND);
    gl_.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    gl_.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gl_.UseProgram(program_);
    gl_.BindVertexArray(vao_);
    gl_.BindSampler(0, 0);
    const std::array<float, 2> viewport{float(width), float(height)};
    gl_.ProgramUniform2fv(program_, viewport_, 1, viewport.data());
    for (std::size_t paint_index=0;paint_index<frame.paints.size();++paint_index) {
        if(fence)fence->pure();
        const auto &item=frame.paints[paint_index];
        if(item.normal_caption_fp32_rect)
            throw std::runtime_error("Closed caption requires authentic preGL embedded route");
        if(item.unit_name_font_selection && item.unit_name_font_selection->snapshot_frame!=frame.snapshot_frame) {
            report.unresolved.push_back("UnitName font-selection command belongs to another composed frame");continue;
        }
        const bool anchored=item.kind==WorldViewPaintKind::text && item.destination.width==0 &&
            item.destination.height>0 && item.style.ends_with(":right-center");
        if (!anchored && (item.destination.width <= 0 || item.destination.height <= 0)) continue;
        const WorldViewRect viewport_clip{0,0,width,height};
        const auto visible_clip = clip_ui_quad(item.clip.value_or(viewport_clip),
                                               {0,0,1,1}, viewport_clip);
        if (!visible_clip) continue;
        const auto &clip = visible_clip->destination;
        std::optional<LabelPaintItemScope> entered_label_item;
        if(fence && fence->publication) entered_label_item.emplace(*fence,paint_index,item,clip,width,height);
        // Normal10 actual emitted role is checked before diagnostic text fallback.
        // Failure of a selected genuine Label never falls back to GlyphGen text.
        if(fence && fence->publication && submit_current_normal_label_owned(paint_index)) {
            fence->pure();continue;
        }
        if (anchored) {
            const auto bounds=anchored_text(item,report,clip);
            if(bounds) for(const auto &request:frame.readonly_tooltips) {
                if(request.source_paint_index!=paint_index || !request.font_blocker.empty() ||
                    request.tooltip.empty() || request.text.text!=item.text) continue;
                report.readonly_text.regions.push_back({paint_index,*bounds,
                    request.context_id,request.control_id,request.tooltip});
            }
        }
        else if (item.kind == WorldViewPaintKind::text) text(item, report, clip);
        else if (item.kind == WorldViewPaintKind::texture ||
                 looks_like_dds(item.source_key)) {
            const graphics::GlTexture* image=nullptr;
            std::shared_ptr<const LayoutResourceEntry> button_entry;
            std::optional<BnwWorldViewControlTree::ButtonVisualOwned> button_visual;
            if(fence&&fence->publication){
                fence->pure();const auto* publication=fence->publication;
                const auto held_tree=publication->tree_;const auto held_control=publication->control_;
                if(!held_control)throw std::runtime_error("Texture indexed publisher has no genuine Control");
                fence->pure();
                const auto quote=held_tree->inspect_control(*held_control);fence->pure();
                if(quote.authored.name=="Button"){
                    if(quote.effect_revision!=publication->effect_revision_)
                        throw std::runtime_error("Button current control differs from immutable GPU publication");
                    const auto& cached=fence->visual_for_tree_owned(held_tree,quote.effect_revision);
                    const auto found=cached.buttons.find(quote.id);
                    if(found==cached.buttons.end())throw std::runtime_error("Button GPU resource-phase visual absent");
                    button_visual=found->second;fence->pure();
                    const auto keeper=button_visual->texture_keeper;
                    if(!button_visual->earned_constructor_null_178||!button_visual->texture_extent_15c||!keeper)
                        throw std::runtime_error("Button draw lacks actual canonical resource/NULL178 outcome");
                    detail::NotificationLayoutTextureKeeper::require_canonical(keeper);fence->pure();
                    const auto entry_it=layout_resource_entries_.find(keeper->key_);
                    if(entry_it==layout_resource_entries_.end())throw std::runtime_error("Button retained raw entry unavailable");
                    button_entry=entry_it->second;
                    const auto require_entry=[&]{
                        fence->pure();require_masked_resources_origin_without_callbacks();fence->pure();
                        const auto current=layout_resource_entries_.find(keeper->key_);
                        if(current==layout_resource_entries_.end()||current->second.get()!=button_entry.get()||
                           current->second.owner_before(button_entry)||button_entry.owner_before(current->second)||
                           button_entry->keeper.get()!=keeper.get()||button_entry->keeper.owner_before(keeper)||
                           keeper.owner_before(button_entry->keeper)||!keeper->entry_revision_||item.source_key!=keeper->key_)
                            throw std::runtime_error("Button actual raw entry/control/revision changed");
                        {const auto scene=fence->projection.scene_.lock();
                         const auto prior_scene=button_entry->scene.lock();
                         if(!scene||!prior_scene||scene.get()!=prior_scene.get()||scene.owner_before(prior_scene)||prior_scene.owner_before(scene))
                             throw std::runtime_error("Button raw entry has another Scene");
                         const auto host=scene->selected_host_;const auto prior_host=button_entry->host.lock();
                         if(!host||!prior_host||host.get()!=prior_host.get()||host.owner_before(prior_host)||prior_host.owner_before(host))
                             throw std::runtime_error("Button raw entry has another selected Host");
                         const auto bank=host->civis_resource_bank_;const auto prior_bank=button_entry->bank.lock();
                         if(!bank||!prior_bank||bank.get()!=prior_bank.get()||bank.owner_before(prior_bank)||prior_bank.owner_before(bank))
                             throw std::runtime_error("Button raw entry has another selected Bank");}
                        detail::NotificationLayoutTextureKeeper::require_canonical(keeper);fence->pure();
                    };
                    require_entry();
                    const auto& raw=keeper->source_;
                    if(raw.index_supplier_||raw.binding_.index||raw.block_size_!=0)
                        throw std::runtime_error("Button paired-index shader profile is not implemented");
                    if(*button_visual->texture_extent_15c!=keeper->native_114_118_||
                       keeper->native_114_118_[0]!=raw.binding_.source.height()||
                       keeper->native_114_118_[1]!=raw.binding_.source.width())
                        throw std::runtime_error("Button native114/118 differs from SAME retained DDS");
                    const auto uv0=button_visual->uv_170_174,uvsize=button_visual->uv_168_16c;
                    for(std::size_t axis=0;axis<2;++axis)
                        if(!std::isfinite(uv0[axis])||!std::isfinite(uvsize[axis])||uv0[axis]<0||uvsize[axis]<0||uv0[axis]+uvsize[axis]>1)
                            throw std::runtime_error("Button actual UV outside retained DDS");
                    if(!button_entry->button_texture){
                        fence->full();require_entry();
                        graphics::GlTexture uploaded(context_,raw.binding_.source,graphics::DdsSamplingColor::srgb);
                        fence->pure();fence->full();require_entry();
                        if(!uploaded.belongs_to(context_))throw std::runtime_error("Button DDS upload has another actual GL context");
                        button_entry->button_texture.emplace(std::move(uploaded));fence->pure();require_entry();
                    }
                    require_entry();
                    if(!button_entry->button_texture->belongs_to(context_))throw std::runtime_error("Button retained GPU texture context changed");
                    image=&*button_entry->button_texture;
                }
            }
            if(!image)image=texture(item.source_key,report);
            if (!image) continue;
            const auto iw = float(image->levels().front().width);
            const auto ih = float(image->levels().front().height);
            const auto &source = item.source;
            const std::array<float, 4> uv = source.width > 0 && source.height > 0
                ? std::array<float, 4>{source.x / iw, source.y / ih,
                                        source.width / iw, source.height / ih}
                : std::array<float, 4>{0, 0, 1, 1};
            if(item.style=="CityBanner"&&!item.tint)
                report.unresolved.push_back("Current CityBanner tint has no current appearance owner");
            if(button_visual){
                if(!item.tint)throw std::runtime_error("Button canonical float tint absent");
                const auto start=button_visual->uv_170_174,span=button_visual->uv_168_16c;
                fence->full();quad(*image,item.destination,{start[0],start[1],span[0],span[1]},item.rgba,false,clip,item.tint);
                fence->pure();context_.check("Paint actual canonical ordinary Button");fence->full();
            }else quad(*image, item.destination, uv, item.rgba, false, clip, item.tint);
            ++report.textures;
        } else {
            const auto style_name = !item.style.empty() ? item.style : item.source_key;
            if (style_name.empty()) {
                quad(white_, item.destination, {0, 0, 1, 1}, fence&&item.tint?item.rgba:0x0d1522d0, false, clip,fence?item.tint:std::nullopt);
                ++report.rectangles;
            } else grid(item, style_name, report, clip);
        }
    }
    if(fence)fence->full();
    context_.check("Paint BNW UI screens");
    if(fence)fence->full();
    return report;
}

} // namespace civ5::ui

namespace civ5::ui {
// The float path preserves Owner geometry until the one actual framebuffer
// conversion; the old integer WorldView painter remains unchanged.
void UiScreenGlRenderer::tech_award_quad(const graphics::GlTexture& texture,
    std::array<float,4> r, std::array<float,4> uv, UiFloatColor tint, bool alpha_only,
    std::int32_t width, std::int32_t height) {
    for(const auto f:r) if(!std::isfinite(f)) throw std::runtime_error("TechAward nonfinite rect");
    for(const auto f:uv) if(!std::isfinite(f)) throw std::runtime_error("TechAward nonfinite UV");
    for(const auto f:tint) if(!std::isfinite(f)||f<0||f>1) throw std::runtime_error("TechAward color outside normalized channels");
    if(r[2]<=0||r[3]<=0)return;
    const float left=std::max(0.0f,r[0]),top=std::max(0.0f,r[1]);
    const float right=std::min(static_cast<float>(width),r[0]+r[2]);
    const float bottom=std::min(static_cast<float>(height),r[1]+r[3]);
    if(right<=left||bottom<=top)return;
    uv={uv[0]+(left-r[0])/r[2]*uv[2],uv[1]+(top-r[1])/r[3]*uv[3],
        (right-left)/r[2]*uv[2],(bottom-top)/r[3]*uv[3]};
    r={left,top,right-left,bottom-top};
    texture.bind(0);
    gl_.ProgramUniform4fv(program_,rect_,1,r.data());
    gl_.ProgramUniform4fv(program_,uv_,1,uv.data());
    gl_.ProgramUniform4fv(program_,tint_,1,tint.data());
    gl_.ProgramUniform1i(program_,alpha_only_,alpha_only?1:0);
    gl_.DrawArrays(GL_TRIANGLE_STRIP,0,4);
}

UiGlPaintReport UiScreenGlRenderer::paint_tech_award(
    const LiveUiFrame& input,const BnwClientFrame& actual_frame,
    std::int32_t width,std::int32_t height,std::optional<ClientPoint> /*pointer*/,
    const std::function<bool()>& source_current) {
    if(worldview_lifetime_->active){worldview_lifetime_->failed=true;throw std::runtime_error("WorldView callback attempted foreign TechAward paint");}
    const auto lifetime=tech_award_lifetime_;
    if(std::this_thread::get_id()!=lifetime->creator||!lifetime->alive||lifetime->failed)
        throw std::runtime_error("TechAward painter is retired or foreign thread");
    if(lifetime->active){lifetime->failed=true;throw std::runtime_error("TechAward painter reentry");}
    lifetime->active=true;
    struct PaintScope {
        std::shared_ptr<TechAwardPaintLifetime> state;
        ~PaintScope(){state->active=false;}
    } scope{lifetime};
    // Retain actual payload storage before any resolver/currentness callback.
    const auto held=input;
    const auto* issued=held.snapshot.find<BnwTechAwardContextSnapshot>();
    UiGlPaintReport report;
    if(!issued)return report;
    const auto snapshot=*issued;
    const auto stamp=actual_frame.stamp;
    const auto animation=snapshot.animation;
    const auto sample=tech_award_updated_sample_.lock();
    const auto witness=tech_award_updated_context_.lock();
    try {
    if(!animation||(!witness||animation.get()!=witness.get()||animation.owner_before(witness)||witness.owner_before(animation))||!sample||!sample->consumed_)
        throw std::runtime_error("TechAward actual UI update/animation publication is unjoined");
    const auto phases=tech_award_alphas_;
    const auto animation_pure=[&] {
        sample->require_current();animation->require_current_without_callbacks();
        if(!lifetime->alive||lifetime->failed||!lifetime->active)
            throw std::runtime_error("TechAward renderer retired after animation source callback");
    };
    const auto current=[&] {
        if(!lifetime->alive||lifetime->failed||!lifetime->active||!source_current||!source_current()||
           !lifetime->alive||lifetime->failed||!lifetime->active)
            throw std::runtime_error("TechAward actual Scene/UI source retired during paint");
        animation->require_publication(stamp,snapshot.occurrence,snapshot.generation,snapshot.context.client_layout);
        animation_pure();
    };
    current();
    const auto* published=held.snapshot.find<BnwClientFrameStamp>();
    if(!published||published->identity!=stamp.identity||published->selection!=stamp.selection||
       published->viewport!=stamp.viewport||snapshot.publication.identity!=stamp.identity||
       snapshot.publication.selection!=stamp.selection||snapshot.publication.viewport!=stamp.viewport)
        throw std::runtime_error("TechAward queued payload belongs to another actual frame");
    const auto& observed=snapshot.context;
    if(!observed.visible||!observed.queued)return report;
    if(!observed.auto_size_complete||!observed.client_layout||!snapshot.occurrence||!snapshot.generation)
        throw std::runtime_error("TechAward genuine queued layout is incomplete");
    const auto layout=observed.client_layout;
    if(layout->viewport!=stamp.viewport||!stamp.viewport.drawable()||
       width!=stamp.viewport.framebuffer_width||height!=stamp.viewport.framebuffer_height)
        throw std::runtime_error("TechAward layout differs from actual framebuffer viewport");
    const float sx=static_cast<float>(width)/static_cast<float>(stamp.viewport.client_width);
    const float sy=static_cast<float>(height)/static_cast<float>(stamp.viewport.client_height);
    const auto rect=[&](std::array<float,4> r) {return std::array<float,4>{r[0]*sx,r[1]*sy,r[2]*sx,r[3]*sy};};
    const auto same_control=[](const auto& a,const auto& b) {
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
    };
    const auto selected=[&](std::string_view key) {
        current();auto found=resources_.resolve(assets::AssetKey(std::string(key)));current();
        if(std::holds_alternative<std::monostate>(found.asset)) {
            found=resources_.resolve(assets::AssetKey(assets::resource_basename(std::string(key))));current();
        }
        const auto* handle=std::get_if<assets::AssetHandle>(&found.asset);
        if(!handle)throw std::runtime_error("TechAward selected source unavailable or ambiguous: "+std::string(key));
        return found;
    };
    const auto pure_current=[&] {
        if(std::this_thread::get_id()!=lifetime->creator||!lifetime->alive||
           lifetime->failed||!lifetime->active)
            throw std::runtime_error("TechAward painter retired after source bookend");
        animation_pure();
    };
    const auto same_candidate=[](const assets::AssetCandidate& a,const assets::AssetCandidate& b) {
        return a.original_name==b.original_name&&a.normalized_name==b.normalized_name&&
            a.payload_size==b.payload_size&&a.provenance.kind==b.provenance.kind&&
            a.provenance.source_id==b.provenance.source_id&&
            a.provenance.source_relative_path==b.provenance.source_relative_path&&
            a.provenance.archive_relative_path==b.provenance.archive_relative_path;
    };
    struct RetainedSource {
        assets::AssetHandle handle;
        UiTextureAssetSource expected;
    };
    std::vector<RetainedSource> retained_sources;
    const auto retain_source=[&](const UiTextureAssetSource& issued_source) {
        // Snapshot all descriptor strings before resolver/currentness callbacks.
        const auto expected=issued_source;
        const auto mounted=selected(expected.candidate.original_name);
        const auto handle=std::get<assets::AssetHandle>(mounted.asset);
        if(mounted.mount_id!=expected.mount_id||!same_candidate(handle.candidate(),expected.candidate))
            throw std::runtime_error("TechAward selected source candidate/mount changed");
        const auto bytes=handle.read_bytes(256ull*1024*1024);
        current();
        if(runtime::identify_bytes(bytes).sha256!=expected.sha256)
            throw std::runtime_error("TechAward retained source SHA differs from selected payload");
        for(const auto& retained:retained_sources)
            if(retained.expected.mount_id==expected.mount_id&&same_candidate(retained.expected.candidate,expected.candidate)) {
                if(retained.expected.sha256!=expected.sha256)
                    throw std::runtime_error("TechAward reused source has conflicting payload identity");
                return;
            }
        if(retained_sources.size()>=512)
            throw std::runtime_error("TechAward retained source profile exceeds bound");
        retained_sources.push_back({handle,expected});
    };
    // Genuine selected ColorAtlas, retained through all parsing and draw calls.
    // This Civis palette does not certify the Original context ColorSet stack.
    const auto atlas_mounted=selected("Assets/UI/ColorAtlas.xml");
    const auto atlas_handle=std::get<assets::AssetHandle>(atlas_mounted.asset);
    current();const auto atlas_bytes=atlas_handle.read_bytes(8ull*1024*1024);current();
    retain_source({atlas_handle.candidate(),atlas_mounted.mount_id,runtime::identify_bytes(atlas_bytes).sha256});
    const auto atlas=content::read_xml_metadata(std::string(atlas_bytes.begin(),atlas_bytes.end()));
    if(atlas.name!="ColorAtlas")throw std::runtime_error("TechAward selected ColorAtlas root differs");
    const auto literal=[](std::string_view value)->std::optional<UiFloatColor> {
        UiFloatColor rgba{};
        for(std::size_t i=0;i<4;++i) {
            const auto split=value.find_first_of(",.");const auto token=value.substr(0,split);
            std::uint32_t n=0;const auto p=std::from_chars(token.data(),token.data()+token.size(),n);
            if(p.ec!=std::errc{}||p.ptr!=token.data()+token.size()||n>255)return {};
            rgba[i]=static_cast<float>(n)/255.0f;
            if(i==3){if(split!=value.npos)return {};}
            else {if(split==value.npos)return {};value.remove_prefix(split+1);}
        }
        return rgba;
    };
    const auto color_value=[&](std::string_view declaration)->UiFloatColor {
        if(const auto v=literal(declaration))return *v;
        const auto comma=declaration.find(',');
        const auto name=declaration.substr(0,comma);
        const content::XmlNode* found=nullptr;
        for(const auto& n:atlas.children)if(n.name=="Color"&&n.attribute("Name")==name) {
            if(found)throw std::runtime_error("TechAward ambiguous selected Color");found=&n;
        }
        if(!found)throw std::runtime_error("TechAward selected Color is absent: "+std::string(name));
        auto v=literal(found->attribute("Color"));
        if(!v)throw std::runtime_error("TechAward selected Color literal is unsupported");
        if(comma!=declaration.npos) {
            const auto alpha=declaration.substr(comma+1);std::uint32_t n=0;
            const auto p=std::from_chars(alpha.data(),alpha.data()+alpha.size(),n);
            if(p.ec!=std::errc{}||p.ptr!=alpha.data()+alpha.size()||n>255)
                throw std::runtime_error("TechAward selected named Color alpha is invalid");
            (*v)[3]=static_cast<float>(n)/255.0f;
        }
        return *v;
    };
    const auto colors=[&](const BnwTechAwardLayoutNode& node) {
        std::array<UiFloatColor,3> value{{{1,1,1,1},{1,1,1,1},{1,1,1,1}}};
        // Actual default ColorSet initializes all three colors white.
        if(!node.color_set.empty()) {
            const content::XmlNode* found=nullptr;
            for(const auto& n:atlas.children)if(n.name=="ColorSet"&&n.attribute("Name")==node.color_set) {
                if(found)throw std::runtime_error("TechAward ambiguous selected ColorSet");found=&n;
            }
            if(!found)throw std::runtime_error("TechAward selected ColorSet is absent: "+node.color_set);
            for(std::size_t i=0;i<3;++i) {
                const auto v=found->attribute("Color"+std::to_string(i));
                if(!v.empty())value[i]=color_value(v);
            }
        }
        for(std::size_t i=0;i<3;++i)if(!node.color_groups[i].empty())value[i]=color_value(node.color_groups[i]);
        if(!node.color.empty())value[0]=color_value(node.color);
        return value;
    };
    const auto texture=[&](const std::shared_ptr<const assets::UiTextureBinding>& binding)->const graphics::GlTexture& {
        if(!binding||!binding->width()||!binding->height())
            throw std::runtime_error("TechAward selected texture payload is absent");
        const auto held_binding=binding;
        const UiTextureAssetSource source{held_binding->source_identity,held_binding->source_mount_id,
            runtime::identify_bytes(held_binding->source.original_bytes()).sha256};
        const auto index=held_binding->index_identity;
        const auto index_mount=held_binding->index_mount_id;
        const auto index_sha=held_binding->index?
            runtime::identify_bytes(held_binding->index->original_bytes()).sha256:std::string{};
        if(index.has_value()!=held_binding->index.has_value())
            throw std::runtime_error("TechAward texture index storage is incomplete");
        retain_source(source);
        if(index)retain_source({*index,index_mount,index_sha});
        for(const auto& cached:tech_award_textures_)if(same_control(cached.source,held_binding))return cached.texture;
        if(tech_award_textures_.size()>=128)throw std::runtime_error("TechAward selected texture cache exceeds bounded profile");
        auto image=binding->expanded?graphics::GlTexture::rgba8(context_,
            std::span<const assets::Rgba8Level>(&*binding->expanded,1),graphics::DdsSamplingColor::linear):
            graphics::GlTexture(context_,binding->source,graphics::DdsSamplingColor::srgb);
        tech_award_textures_.push_back({binding,std::move(image)});
        return tech_award_textures_.back().texture;
    };
    const auto font=[&](const BnwTechAwardMeasuredLabel& label)->const TechAwardFont& {
        current();
        const auto metadata=read_glyphgen_style_metadata(resources_,label.font_key,label.font_style);current();
        if(metadata.font_source.sha256!=label.font_sha256)
            throw std::runtime_error("TechAward measured GG SHA differs from selected font");
        retain_source(metadata.font_source);
        const auto retain_layers=[&](const GlyphGenStyleAsset& asset) {
            std::vector<UiTextureAssetSource> sources;
            sources.reserve(asset.layers.size()*2);
            for(const auto& layer:asset.layers) {
                sources.push_back(layer.source);sources.push_back(layer.index_source);
            }
            for(const auto& source:sources)retain_source(source);
        };
        for(const auto& cached:tech_award_fonts_)
            if(cached.key==label.font_key&&cached.style==label.font_style&&cached.sha256==label.font_sha256) {
                retain_layers(cached.source);return cached;
            }
        if(tech_award_fonts_.size()>=32)throw std::runtime_error("TechAward font cache exceeds bounded selected profile");
        auto source=load_glyphgen_style(resources_,label.font_key,label.font_style);current();
        const auto after=read_glyphgen_style_metadata(resources_,label.font_key,label.font_style);current();
        if(after.font_source.sha256!=label.font_sha256)
            throw std::runtime_error("TechAward selected GG changed during layer preparation");
        const auto same_font=[](const GlyphGenFont& a,const GlyphGenFont& b) {
            if(a.root_attributes!=b.root_attributes||a.texture_atlas_attributes!=b.texture_atlas_attributes||
               a.glyph_container_attributes!=b.glyph_container_attributes||a.declared_glyph_count!=b.declared_glyph_count||
               a.textures.size()!=b.textures.size()||a.styles.size()!=b.styles.size()||a.glyphs.size()!=b.glyphs.size())return false;
            for(std::size_t n=0;n<a.textures.size();++n)if(a.textures[n].attributes!=b.textures[n].attributes)return false;
            for(std::size_t n=0;n<a.styles.size();++n)if(a.styles[n].name!=b.styles[n].name||a.styles[n].layers!=b.styles[n].layers||a.styles[n].attributes!=b.styles[n].attributes)return false;
            for(std::size_t n=0;n<a.glyphs.size();++n)if(a.glyphs[n].code_point!=b.glyphs[n].code_point||a.glyphs[n].attributes!=b.glyphs[n].attributes)return false;
            return true;
        };
        if(!same_font(source.font,metadata.font)||!same_font(source.font,after.font))
            throw std::runtime_error("TechAward loaded glyph/style data differs from measured GG");
        retain_source(after.font_source);retain_layers(source);
        TechAwardFont prepared{label.font_key,label.font_style,label.font_sha256,std::move(source),{}};
        prepared.textures.reserve(prepared.source.layers.size());
        for(const auto& layer:prepared.source.layers)
            prepared.textures.push_back(graphics::GlTexture::alpha8(context_,layer.atlas.width,layer.atlas.height,layer.atlas.alpha));
        tech_award_fonts_.push_back(std::move(prepared));return tech_award_fonts_.back();
    };
    struct IconPaint {
        std::shared_ptr<const GlyphGenRgbaAtlas> atlas;
        std::string font_sha,packed_sha,index_sha;
        graphics::GlTexture texture;
    };
    std::vector<IconPaint> icon_paints;
    const auto icon_texture=[&](const BnwTechAwardMarkupIcon& issued_icon)->const graphics::GlTexture& {
        // Retain immutable actual Owner11 data BEFORE any resolver callback.
        const auto icon=issued_icon;
        if(!icon.font||!icon.geometry||!icon.rgba_atlas||icon.font->textures.size()!=1||
           !icon.rgba_atlas->width||!icon.rgba_atlas->height||
           icon.rgba_atlas->width!=icon.geometry->atlas_width||
           icon.rgba_atlas->height!=icon.geometry->atlas_height)
            throw std::runtime_error("TechAward actual icon payload/geometry is incomplete");
        const auto pixels=std::uint64_t{icon.rgba_atlas->width}*icon.rgba_atlas->height;
        if(pixels>256ull*1024*1024/4||icon.rgba_atlas->rgba.size()!=pixels*4)
            throw std::runtime_error("TechAward actual icon atlas exceeds bounded RGBA profile");
        const auto font_mounted=selected(icon.font_key+".ggxml");
        const auto font_handle=std::get<assets::AssetHandle>(font_mounted.asset);
        retain_source({font_handle.candidate(),font_mounted.mount_id,icon.font_sha256});
        const auto image=selected(icon.font->textures.front().source());
        const auto packed_handle=std::get<assets::AssetHandle>(image.asset);
        const auto packed_name=packed_handle.candidate().original_name;
        if(packed_name.size()<4||assets::normalize_asset_name(packed_name).substr(packed_name.size()-4)!=".dds")
            throw std::runtime_error("TechAward actual icon packed source is not DDS");
        const auto index_name=packed_name.substr(0,packed_name.size()-4)+"-index.dds";
        const auto index=selected(index_name);
        const auto index_handle=std::get<assets::AssetHandle>(index.asset);
        const auto& a=packed_handle.candidate().provenance;
        const auto& b=index_handle.candidate().provenance;
        if(image.mount_id!=index.mount_id||a.kind!=b.kind||a.source_id!=b.source_id||
           a.archive_relative_path!=b.archive_relative_path||
           assets::normalize_asset_name(index_handle.candidate().original_name)!=assets::normalize_asset_name(index_name))
            throw std::runtime_error("TechAward actual icon atlas/index has mixed selected sources");
        retain_source({packed_handle.candidate(),image.mount_id,icon.packed_sha256});
        retain_source({index_handle.candidate(),index.mount_id,icon.index_sha256});
        for(const auto& cached:icon_paints)if(same_control(cached.atlas,icon.rgba_atlas)) {
            if(cached.font_sha!=icon.font_sha256||cached.packed_sha!=icon.packed_sha256||cached.index_sha!=icon.index_sha256)
                throw std::runtime_error("TechAward reused icon atlas has foreign source hashes");
            return cached.texture;
        }
        if(icon_paints.size()>=32)throw std::runtime_error("TechAward actual icon profile exceeds bound");
        // Reconstruct the selected pair once for this operation; authenticate
        // retained RGBA pixels rather than trusting arbitrary atlas bytes.
        const auto font_bytes=font_handle.read_bytes(64ull*1024*1024);current();
        const auto packed_bytes=packed_handle.read_bytes(64ull*1024*1024);current();
        const auto index_bytes=index_handle.read_bytes(64ull*1024*1024);current();
        if(runtime::identify_bytes(font_bytes).sha256!=icon.font_sha256||
           runtime::identify_bytes(packed_bytes).sha256!=icon.packed_sha256||
           runtime::identify_bytes(index_bytes).sha256!=icon.index_sha256)
            throw std::runtime_error("TechAward selected icon source changed during paint preparation");
        const auto actual_font=GlyphGenFont::from_xml(content::read_xml_metadata(std::string(font_bytes.begin(),font_bytes.end())));
        const auto actual=reconstruct_glyphgen_rgba(actual_font,assets::DdsData::parse(index_bytes),assets::DdsData::parse(packed_bytes));
        if(actual.width!=icon.rgba_atlas->width||actual.height!=icon.rgba_atlas->height||actual.rgba!=icon.rgba_atlas->rgba)
            throw std::runtime_error("TechAward retained icon RGBA differs from selected packed/index bytes");
        const assets::Rgba8Level level{actual.width,actual.height,actual.rgba};
        auto texture=graphics::GlTexture::rgba8(context_,std::span<const assets::Rgba8Level>(&level,1),
                                               graphics::DdsSamplingColor::linear);
        icon_paints.push_back({icon.rgba_atlas,icon.font_sha256,icon.packed_sha256,icon.index_sha256,std::move(texture)});
        return icon_paints.back().texture;
    };
    gl_.BindFramebuffer(GL_FRAMEBUFFER,0);gl_.Viewport(0,0,width,height);
    gl_.Disable(GL_DEPTH_TEST);gl_.Disable(GL_CULL_FACE);gl_.Disable(GL_FRAMEBUFFER_SRGB);
    gl_.Enable(GL_BLEND);gl_.BlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_ADD);
    gl_.BlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    gl_.UseProgram(program_);gl_.BindVertexArray(vao_);gl_.BindSampler(0,0);
    const std::array<float,2> viewport{static_cast<float>(width),static_cast<float>(height)};
    gl_.ProgramUniform2fv(program_,viewport_,1,viewport.data());
    for(const auto& node:layout->nodes) {
        if(node.hidden)continue;
        if(!animation->mouse_visible(node.path,*layout))continue;
        float inherited_alpha=1;
        // Layout is actual private publisher storage, in authored preorder.
        // Each local alpha is multiplied once; parents are not precomposited.
        for(const auto& ancestor:layout->nodes)if((node.path==ancestor.path||node.path.starts_with(ancestor.path+"/"))&&
            ancestor.kind=="AlphaAnim") {
            const auto phase=std::find_if(phases.begin(),phases.end(),[&](const auto& p){return p.first==ancestor.path;});
            if(phase==phases.end())throw std::runtime_error("TechAward issued authored alpha path absent");
            volatile float product=inherited_alpha*phase->second;inherited_alpha=product;
        }
        current();auto tint=colors(node);
        for(auto& layer:tint){volatile float alpha=layer[3]*inherited_alpha;layer[3]=alpha;}
        if(node.kind=="Grid"||node.kind=="GridData") {
            const auto slices=std::find_if(layout->grids.begin(),layout->grids.end(),[&](const auto& g){return g.path==node.path;});
            if(slices==layout->grids.end())throw std::runtime_error("TechAward genuine nine-slice payload is absent");
            const auto& image=texture(slices->selected_texture);
            for(std::size_t i=0;i<9;++i)tech_award_quad(image,rect(slices->rects[i]),slices->uvs[i],tint[0],false,width,height);
            report.textures+=9;
        } else if((node.kind=="Image"||node.kind=="AlphaAnim")&&!node.texture.empty()) {
            const auto& image=texture(node.selected_texture);
            const float x=static_cast<float>(node.texture_offset[0]),y=static_cast<float>(node.texture_offset[1]);
            const float iw=static_cast<float>(node.selected_texture->width()),ih=static_cast<float>(node.selected_texture->height());
            if(x<0||y<0||x+node.rect[2]>iw||y+node.rect[3]>ih)
                throw std::runtime_error("TechAward selected Image crop exceeds actual payload");
            tech_award_quad(image,rect(node.rect),{x/iw,y/ih,node.rect[2]/iw,node.rect[3]/ih},tint[0],false,width,height);++report.textures;
        } else if(node.kind=="Label") {
            if(!node.measured_label||*node.measured_label>=observed.measured_labels.size())
                throw std::runtime_error("TechAward actual measured Label is absent");
            const auto& label=observed.measured_labels[*node.measured_label];
            if(label.path!=node.path)throw std::runtime_error("TechAward measured Label path differs");
            const auto& selected_font=font(label);const auto base=glyphgen_geometry(selected_font.source.font);
            const auto styled_color=[inherited_alpha](const UiFloatColor& fallback,
                                       const std::optional<std::array<std::uint8_t,4>>& authored) {
                if(!authored)return fallback;
                return UiFloatColor{static_cast<float>((*authored)[0])/255.0f,static_cast<float>((*authored)[1])/255.0f,
                                    static_cast<float>((*authored)[2])/255.0f,
                    static_cast<float>((*authored)[3])/255.0f*inherited_alpha};
            };
            for(std::size_t n=selected_font.source.layers.size();n--!=0;) {
                const auto& layer=selected_font.source.layers[n];
                if(layer.color_index>=tint.size())throw std::runtime_error("TechAward selected font layer ColorSet index unsupported");
                for(const auto& line:label.layout.lines) {
                    const auto draw_text=[&](const GlyphGenProjectedGlyph& glyph,const UiFloatColor& color) {
                        const auto b=std::find_if(base.glyphs.begin(),base.glyphs.end(),[&](const auto& g){return g.code_point==glyph.code_point;});
                        const auto e=std::find_if(layer.geometry.glyphs.begin(),layer.geometry.glyphs.end(),[&](const auto& g){return g.code_point==glyph.code_point;});
                        if(b==base.glyphs.end()||e==layer.geometry.glyphs.end())
                            throw std::runtime_error("TechAward selected font effect glyph is absent");
                        const std::array<float,4> r{node.rect[0]+static_cast<float>(glyph.left_from_line_origin+e->origin_x-b->origin_x),
                            node.rect[1]+static_cast<float>(base.ascent-base.ascent_adjustment+line.baseline_offset-e->origin_y),
                            static_cast<float>(e->width),static_cast<float>(e->height)};
                        tech_award_quad(selected_font.textures[n],rect(r),
                            {static_cast<float>(e->atlas_x)/static_cast<float>(layer.geometry.atlas_width),
                             static_cast<float>(e->atlas_y)/static_cast<float>(layer.geometry.atlas_height),
                             static_cast<float>(e->width)/static_cast<float>(layer.geometry.atlas_width),
                             static_cast<float>(e->height)/static_cast<float>(layer.geometry.atlas_height)},
                            color,true,width,height);++report.glyphs;
                    };
                    if(line.styled_glyphs.empty()) {
                        for(const auto& glyph:line.glyphs.glyphs)draw_text(glyph,tint[layer.color_index]);
                    } else for(const auto& styled:line.styled_glyphs) {
                        if(!styled.icon) {
                            draw_text(styled.glyph,styled_color(tint[layer.color_index],styled.color_groups[layer.color_index]));
                        } else if(n==0) {
                            const auto& icon=*styled.icon;
                            const auto& glyph=styled.glyph;
                            if(glyph.code_point!=icon.code_point||!icon.geometry)
                                throw std::runtime_error("TechAward styled icon identity is foreign");
                            const auto& image=icon_texture(icon);
                            const auto actual_glyph=std::find_if(icon.geometry->glyphs.begin(),icon.geometry->glyphs.end(),
                                [&](const auto& g){return g.code_point==glyph.code_point;});
                            if(actual_glyph==icon.geometry->glyphs.end()||glyph.atlas_x!=actual_glyph->atlas_x||
                               glyph.atlas_y!=actual_glyph->atlas_y||glyph.width!=actual_glyph->width||
                               glyph.height!=actual_glyph->height||glyph.top_from_baseline!=actual_glyph->origin_y)
                                throw std::runtime_error("TechAward styled icon crop differs from actual selected geometry");
                            const std::array<float,4> r{node.rect[0]+static_cast<float>(glyph.left_from_line_origin),
                                node.rect[1]+static_cast<float>(base.ascent-icon.geometry->ascent_adjustment+line.baseline_offset-glyph.top_from_baseline),
                                static_cast<float>(glyph.width),static_cast<float>(glyph.height)};
                            tech_award_quad(image,rect(r),
                                {static_cast<float>(glyph.atlas_x)/static_cast<float>(icon.geometry->atlas_width),
                                 static_cast<float>(glyph.atlas_y)/static_cast<float>(icon.geometry->atlas_height),
                                 static_cast<float>(glyph.width)/static_cast<float>(icon.geometry->atlas_width),
                                 static_cast<float>(glyph.height)/static_cast<float>(icon.geometry->atlas_height)},
                                styled_color(tint[0],styled.color_groups[0]),false,width,height);++report.glyphs;
                        }
                    }
                }
            }
        }
        current();
    }
    current();const auto after_atlas=atlas_handle.read_bytes(8ull*1024*1024);current();
    if(after_atlas!=atlas_bytes)throw std::runtime_error("TechAward selected ColorAtlas changed during paint");
    // Replay actual resolution before the last callback; retain concrete file
    // handles so final payload reads invoke no AssetSource/currentness target.
    for(const auto& source:retained_sources) {
        const auto mounted=selected(source.expected.candidate.original_name);
        const auto& handle=std::get<assets::AssetHandle>(mounted.asset);
        if(mounted.mount_id!=source.expected.mount_id||!same_candidate(handle.candidate(),source.expected.candidate))
            throw std::runtime_error("TechAward selected source changed after paint");
    }
    context_.check("Paint actual queued TechAward snapshot");current();
    // LAST actual source callback has returned. AssetHandle::read_bytes is
    // concrete bounded file IO; no resolver or supplied target follows this.
    for(const auto& source:retained_sources) {
        pure_current();
        const auto bytes=source.handle.read_bytes(256ull*1024*1024);
        if(runtime::identify_bytes(bytes).sha256!=source.expected.sha256)
            throw std::runtime_error("TechAward selected payload changed after last source callback");
        pure_current();
    }
    pure_current();
#ifdef _WIN32
    {const auto painted_scene=lifetime->scene.lock();
     if(!painted_scene||painted_scene->frame().get()!=&actual_frame)
        throw std::runtime_error("TechAward painted Scene publication retired");
     tech_award_painted_frame_=painted_scene->frame();}
    pure_current();tech_award_painted_context_=animation;tech_award_painted_ui_=held;
    tech_award_presented_context_.reset();tech_award_presented_frame_.reset();
#endif
    return report;
    }catch(...){lifetime->failed=true;if(animation)animation->quarantine();throw;}
}
} // namespace civ5::ui

namespace civ5::ui {
namespace {
void alpha_need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class A,class B>bool alpha_control(const A& a,const B& b){
 return a&&b&&a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
// Original x87 unsigned integer * actual f32 operand 0x3A83126F, then
// FSTP f32. The 56-bit exact product is rounded once to nearest/even;
// this avoids an intermediate f32 integer conversion for long stalls.
float alpha_native_seconds(std::uint32_t milliseconds){
 alpha_need(std::fegetround()==FE_TONEAREST,"TechAward animation floating rounding mode differs");
 if(!milliseconds)return 0;
 const std::uint64_t product=std::uint64_t{milliseconds}*0x83126full;
 const unsigned bits=64u-static_cast<unsigned>(std::countl_zero(product));
 const unsigned shift=bits>24u?bits-24u:0u;
 std::uint64_t significand=product;
 if(shift){significand=product>>shift;const auto lost=product&((std::uint64_t{1}<<shift)-1);
  const auto half=std::uint64_t{1}<<(shift-1);
  if(lost>half||(lost==half&&(significand&1)))++significand;}
 return std::ldexp(static_cast<float>(significand),static_cast<int>(shift)-33);
}
}
struct UiScreenGlRenderer::AnimationSources {
 std::shared_ptr<TechAwardPaintLifetime> clock;
 std::shared_ptr<BnwClientSceneFrame> scene;
 std::shared_ptr<LiveUiHost> ui;
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
 std::unique_ptr<graphics::BnwOwnedCameraRendererWindowLease::RendererPin> window_pin;
 bool counted=false;
 AnimationSources(const AnimationSources&)=delete;
 AnimationSources& operator=(const AnimationSources&)=delete;
 explicit AnimationSources(const std::shared_ptr<TechAwardPaintLifetime>& supplied):clock(supplied){
  alpha_need(clock&&clock->creator==std::this_thread::get_id()&&clock->alive&&!clock->failed&&clock->active,
             "TechAward update sources outside active renderer");
  scene=clock->scene.lock();ui=clock->ui.lock();window=clock->window.lock();
  alpha_need(scene&&ui&&window,"TechAward update actual renderer sources expired");
  alpha_need(scene->gui_thread_==std::this_thread::get_id(),"TechAward Scene foreign thread");
  const auto count=scene->gui_publication_pins_.load();
  alpha_need(count<SIZE_MAX,"TechAward Scene pin overflow");
  scene->gui_publication_pins_.fetch_add(1);counted=true;
  try{window_pin.reset(new graphics::BnwOwnedCameraRendererWindowLease::RendererPin(window));pure();}
  catch(...){scene->gui_publication_pins_.fetch_sub(1);counted=false;throw;}
 }
 ~AnimationSources(){if(counted)scene->gui_publication_pins_.fetch_sub(1);}
 void pure()const {
  alpha_need(clock->creator==std::this_thread::get_id()&&clock->alive&&!clock->failed&&clock->active,
             "TechAward actual renderer lifetime retired/reentered");
  alpha_need(bool(window_pin),"TechAward actual counted Window lender absent");
  window_pin->require_current_without_native_callbacks();
  // Short locks are gone before callbacks and do not count as outside owners.
  const auto actual_scene=scene->gui_self_.lock();const auto actual_ui=ui->selected_owner_lifetime_.lock();
  alpha_need(alpha_control(actual_scene,scene)&&alpha_control(actual_ui,ui)&&
   scene.use_count()>static_cast<long>(scene->gui_publication_pins_.load()+1)&&ui.use_count()>2,
   "TechAward canonical Scene/UI control or outside owner lost");
  const auto bound_scene=clock->scene.lock();const auto bound_ui=clock->ui.lock();const auto bound_window=clock->window.lock();
  alpha_need(alpha_control(bound_scene,scene)&&alpha_control(bound_ui,ui)&&alpha_control(bound_window,window),
             "TechAward renderer source binding replaced");
 }
 void full(const graphics::GlContext& context,const LiveUiFrame* frame=nullptr,const BnwClientFrame* publication=nullptr)const {
  pure();scene->verify_owned();pure();
  window->with_actual_window_loan([&](const auto& loan){
   pure();loan.require_current();pure();
   {const auto canonical=loan.retain_owner();
    alpha_need(alpha_control(canonical,window),"TechAward renderer canonical Window changed");}
   // The uncounted returned comparison lock ends before any native callback
   // or outside-owner check, and before the next actual Loan getter.
   pure();const auto identity=context.native_identity();
   const auto hwnd=loan.window();pure();const auto dc=loan.dc();pure();const auto wgl=loan.wgl_context();pure();
   alpha_need(identity.owner_thread&&identity.window==hwnd&&identity.dc==dc&&identity.context==wgl,
              "TechAward renderer Window/GL binding changed");pure();
  });pure();
  if(frame){alpha_need(publication&&scene->frame().get()==publication&&scene->accepts(*ui,*frame),
      "TechAward UI update actual Scene/publication changed");pure();}
  scene->verify_owned();pure();
 }
};
#ifdef _WIN32
std::shared_ptr<const UiScreenGlRenderer::PresentationTarget> UiScreenGlRenderer::tech_award_presentation_target(){
 const auto lifetime=tech_award_lifetime_;
 alpha_need(lifetime->creator==std::this_thread::get_id()&&lifetime->alive&&!lifetime->failed,
            "TechAward presentation target requested from retired renderer");
 return std::shared_ptr<const PresentationTarget>(new PresentationTarget(*this));
}
void UiScreenGlRenderer::PresentationTarget::acknowledge(const graphics::GlContext& context)const {
 const auto clock=clock_.lock();
 alpha_need(clock&&clock->creator==std::this_thread::get_id()&&clock->alive&&!clock->failed,
            "TechAward actual-present target retired/foreign thread");
 if(clock->active){clock->failed=true;throw std::runtime_error("TechAward actual-present target reentry");}
 // Renderer destruction poisons this independent Clock before native cleanup.
 // No callback occurs between the check and borrowed renderer entry.
 renderer_->acknowledge_tech_award_presented(context);
}
void UiScreenGlRenderer::acknowledge_tech_award_presented(const graphics::GlContext& actual_context){
 const auto lifetime=tech_award_lifetime_;
 alpha_need(lifetime->creator==std::this_thread::get_id()&&lifetime->alive&&!lifetime->failed&&
    &actual_context==&context_,"TechAward actual-present renderer/context changed");
 if(lifetime->active){lifetime->failed=true;throw std::runtime_error("TechAward actual-present renderer reentry");}
 if(!tech_award_painted_ui_)return; // This real frame did not draw a TechAward.
 const auto held=*tech_award_painted_ui_;const auto frame=tech_award_painted_frame_.lock();
 const auto witness=tech_award_painted_context_.lock();
 const auto* issued=held.snapshot.find<BnwTechAwardContextSnapshot>();
 if(!issued)return;
 lifetime->active=true;struct End{std::shared_ptr<TechAwardPaintLifetime> p;~End(){p->active=false;}} end{lifetime};
 try {
  AnimationSources sources(lifetime);sources.pure();
  alpha_need(frame&&witness&&alpha_control(issued->animation,witness)&&issued->context.client_layout,
             "TechAward presented actual paint/source expired");
  const auto pure=[&]{sources.pure();witness->require_current_without_callbacks();sources.pure();};
  pure();sources.full(context_,&held,frame.get());pure();
  witness->require_publication(frame->stamp,issued->occurrence,issued->generation,issued->context.client_layout);pure();
  sources.full(context_,&held,frame.get());pure();
  tech_award_presented_context_=witness;tech_award_presented_frame_=frame;
 }catch(...){lifetime->failed=true;if(witness)witness->quarantine();throw;}
}
void UiScreenGlRenderer::capture_tech_award_pointer(
 const std::shared_ptr<const BnwTechAwardWindowPointerLoan>& native,const LiveUiFrame& input,
 const std::shared_ptr<const BnwClientFrame>& frame) {
 const auto lifetime=tech_award_lifetime_;
 alpha_need(lifetime->creator==std::this_thread::get_id()&&lifetime->alive&&!lifetime->failed,
            "TechAward pointer renderer retired/foreign thread");
 if(lifetime->active){lifetime->failed=true;throw std::runtime_error("TechAward native pointer renderer reentry");}
 lifetime->active=true;struct End{std::shared_ptr<TechAwardPaintLifetime> p;~End(){p->active=false;}} end{lifetime};
 const auto held=input;const auto* issued=held.snapshot.find<BnwTechAwardContextSnapshot>();
 const auto witness=issued?issued->animation:nullptr;
 try {
  AnimationSources sources(lifetime);sources.pure();
  alpha_need(native&&alpha_control(native,native->self_.lock())&&!native->captured_&&witness&&frame&&
    issued->context.client_layout&&issued->context.visible&&issued->context.queued,
    "TechAward native pointer lacks actual displayed publication");
  native->require_entered();witness->require_current_without_callbacks();sources.pure();
  const auto canonical=witness->layout_.lock();
  alpha_need(alpha_control(canonical,issued->context.client_layout)&&held.owner==sources.ui.get()&&
    held.sequence==sources.ui->sequence_&&sources.scene->frame()==frame&&
    frame->stamp.identity==issued->publication.identity&&frame->stamp.selection==issued->publication.selection&&
    frame->stamp.viewport==issued->publication.viewport&&frame->stamp.viewport==native->viewport_&&
    reinterpret_cast<std::uintptr_t>(native->state_->window)==context_.native_identity().window,
    "TechAward pointer displayed Scene/UI/native viewport changed");
  alpha_need(alpha_control(tech_award_painted_context_.lock(),witness)&&
    alpha_control(tech_award_painted_frame_.lock(),frame)&&
    alpha_control(tech_award_presented_context_.lock(),witness)&&alpha_control(tech_award_presented_frame_.lock(),frame),
    "TechAward pointer was not bound to actual prior paint/present");
  alpha_need(sources.ui->issued_plot_yield_snapshot_&&
    held.snapshot.same_payload_storage(*sources.ui->issued_plot_yield_snapshot_),"TechAward pointer did not capture actual issued snapshot storage");
  if(const auto previous=tech_award_pointer_previous_)
   alpha_need(previous->state_==native->state_&&previous->sequence_<native->sequence_,"TechAward pointer source order/replacement");
  auto receipt=std::make_shared<PointerReceipt>(native,held,frame,witness,lifetime->sequence);
  native->require_entered();witness->require_current_without_callbacks();sources.pure();
  tech_award_pointer_queue_.push_back(std::move(receipt));native->captured_=true;tech_award_pointer_previous_=native;
 }catch(...){lifetime->failed=true;if(witness)witness->quarantine();throw;}
}
void UiScreenGlRenderer::consume_tech_award_pointer(const std::function<bool()>& source_current) {
 if(tech_award_pointer_queue_.empty())return;
 const auto lifetime=tech_award_lifetime_;
 alpha_need(lifetime->creator==std::this_thread::get_id()&&lifetime->alive&&!lifetime->failed,
            "TechAward pointer consumer retired/foreign thread");
 if(lifetime->active){lifetime->failed=true;throw std::runtime_error("TechAward pointer consumer reentry");}
 lifetime->active=true;struct End{std::shared_ptr<TechAwardPaintLifetime> p;~End(){p->active=false;}} end{lifetime};
 auto requests=std::move(tech_award_pointer_queue_);tech_award_pointer_queue_.clear();
 std::optional<std::function<bool()>> target;std::optional<AnimationSources> lender;
 try {
  lender.emplace(lifetime);const auto& sources=*lender;sources.pure();
  const auto pure=[&]{sources.pure();for(const auto& request:requests){
   alpha_need(request&&request->clock_sequence==lifetime->sequence&&request->frame==sources.scene->frame(),
      "TechAward pointer receipt is not same pre-command renderer phase");
   request->native->require_completed();request->witness->require_current_without_callbacks();sources.pure();
  }};
  const auto full=[&]{pure();for(const auto& request:requests){
   sources.full(context_,&request->ui,request->frame.get());pure();
   const auto* issued=request->ui.snapshot.find<BnwTechAwardContextSnapshot>();
   alpha_need(issued&&issued->animation==request->witness&&issued->context.client_layout,"TechAward pointer sealed snapshot replaced");
   request->witness->require_publication(request->frame->stamp,issued->occurrence,issued->generation,issued->context.client_layout);pure();
  }};
  full();target.emplace(source_current);pure();alpha_need(bool(*target),"TechAward pointer has no actual source validator");
  alpha_need((*target)(),"TechAward pointer source failed before transitions");pure();full();
  for(const auto& request:requests){
   const auto* issued=request->ui.snapshot.find<BnwTechAwardContextSnapshot>();
   request->witness->pointer_transition(request->native->point_,*issued->context.client_layout);pure();full();
  }
  alpha_need((*target)(),"TechAward pointer source failed after transitions");pure();full();
  target.reset();pure();full();pure();
 }catch(...){lifetime->failed=true;for(const auto& request:requests)if(request&&request->witness)request->witness->quarantine();
  target.reset();lender.reset();throw;}
}
#endif
std::shared_ptr<const BnwTechAwardAnimationSample> UiScreenGlRenderer::begin_tech_award_ui_update(
 const std::shared_ptr<BnwClientSceneFrame>& scene,const std::shared_ptr<LiveUiHost>& ui,
 const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& window){
 const auto lifetime=tech_award_lifetime_;
 alpha_need(lifetime->creator==std::this_thread::get_id()&&lifetime->alive&&!lifetime->failed,
            "TechAward update clock retired/foreign thread");
 if(lifetime->active){lifetime->failed=true;throw std::runtime_error("TechAward renderer update clock reentry");}
 lifetime->active=true;struct End{std::shared_ptr<TechAwardPaintLifetime> p;~End(){p->active=false;}} end{lifetime};
 try {
#ifdef _WIN32
  // Actual loop-start sample, before any UI/source/native input callback.
  std::uint32_t previous=lifetime->tick_initialized?lifetime->previous_tick:GetTickCount();
  const std::uint32_t tick=GetTickCount();if(tick<previous)previous=0;
  const auto seconds=alpha_native_seconds(tick-previous);
#else
  throw std::runtime_error("TechAward native update clock requires Windows GetTickCount");
#endif
  alpha_need(scene&&ui&&window,"TechAward renderer update requires genuine Scene/UI/Window");
  if(lifetime->tick_initialized){const auto old_scene=lifetime->scene.lock();const auto old_ui=lifetime->ui.lock();const auto old_window=lifetime->window.lock();
   alpha_need(alpha_control(old_scene,scene)&&alpha_control(old_ui,ui)&&alpha_control(old_window,window),
              "TechAward update clock cannot change source session");}
  else {lifetime->scene=scene;lifetime->ui=ui;lifetime->window=window;}
  AnimationSources sources(lifetime);sources.full(context_);sources.pure();
  alpha_need(lifetime->sequence<std::numeric_limits<std::uint64_t>::max(),"TechAward update sequence overflow");
#ifdef _WIN32
  const auto sample=std::shared_ptr<BnwTechAwardAnimationSample>(new BnwTechAwardAnimationSample(lifetime,lifetime->sequence+1,tick,seconds));
  sample->self_=sample;sources.pure();
  lifetime->previous_tick=tick;lifetime->tick_initialized=true;++lifetime->sequence;lifetime->sample=sample;
  sample->require_current();sources.pure();return sample;
#endif
 }catch(...){lifetime->failed=true;throw;}
}
std::pair<std::unique_ptr<BnwScrollAnimationLoopTick>,std::shared_ptr<const BnwTechAwardAnimationSample>> UiScreenGlRenderer::begin_scroll_animation_loop_tick_owned(
 const std::shared_ptr<BnwClientSceneFrame>& scene,const std::shared_ptr<LiveUiHost>& ui,
 const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& window){
 const auto life=tech_award_lifetime_;
 alpha_need(life->creator==std::this_thread::get_id()&&life->alive&&!life->failed&&!life->active,
     "Scroll loop tick requires actual idle renderer creator");
 // Called only from the actual Camera loop after its message batch. Preserve
 // the existing earned GetTickCount sample and previous-tick arithmetic.
 const auto sample=begin_tech_award_ui_update(scene,ui,window);
 // FIRST saved life after the callbackful source admission; never this.
 alpha_need(life->alive&&!life->failed,"Scroll renderer retired while sampling actual loop");
 return {std::unique_ptr<BnwScrollAnimationLoopTick>(new BnwScrollAnimationLoopTick(sample)),sample};
}
void UiScreenGlRenderer::advance_worldview_scroll_update_owned(const std::shared_ptr<BnwScrollAnimationLoopTick::State>& state,
 runtime::BnwSelectedStockLuaSourceOwner& source_owner,const runtime::BnwWorldViewSourceOperationLoan& operation,
 const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& trees){
 const auto life=tech_award_lifetime_;
 alpha_need(life->creator==std::this_thread::get_id()&&life->alive&&!life->failed,
     "Scroll active renderer updater retired/foreign");
 state->origin();const auto* actual_context=&context_;
 try {
  state->current();alpha_need(!life->active&&!state->entered.load(),"Scroll updater reentered");
  if(state->consumed){
   operation.require_current();state->current();
   alpha_need(life->alive&&!life->failed,"Scroll renderer retired after repeated-paint check");
   return; // No new tick/loan/phase mutation, never a completion receipt.
  }
  life->active=true;state->entered=true;
  struct End{std::shared_ptr<TechAwardPaintLifetime> l;std::shared_ptr<BnwScrollAnimationLoopTick::State>s;
      ~End(){s->entered=false;l->active=false;}} end{life,state};
  const BnwScrollAnimationUpdateLoan update(state);
  AnimationSources sources(life);sources.pure();sources.full(*actual_context);sources.pure();
  update.require_current_without_callbacks();operation.require_current();update.require_current_without_callbacks();
  // The platform tick is only a delta lender. Policy's closed child issuer
  // authenticates this SAME retained SourceOwner/Bootstrap/local client.
  const auto client_first=[&]{
   if(life->creator!=std::this_thread::get_id()){
    state->failed=true;life->failed=true;throw std::runtime_error("SP client loop child foreign renderer creator");
   }
   alpha_need(life->alive&&!life->failed&&life->active,
       "SP client loop child renderer retired or failed before borrowed access");
   state->origin();alpha_need(state->entered.load()&&!state->consumed,
       "SP client loop child left/consumed its actual update scope");
  };
  client_first();sources.pure();
  source_owner.run_sp_client_update_owned(operation,update);
  // FIRST after the callbackful child, before touching borrowed Context/Source.
  client_first();sources.pure();update.require_current_without_callbacks();client_first();
  operation.require_current();client_first();sources.pure();
  sources.full(*actual_context);client_first();sources.pure();
  update.require_current_without_callbacks();client_first();
  sources.pure();runtime::BnwWorldViewNativeContextIssuer::advance_scroll_updates_owned(operation,trees,update);
  update.require_current_without_callbacks();sources.pure();sources.full(*actual_context);sources.pure();
  state->consumed=true;
 }catch(...){state->failed=true;life->failed=true;throw;}
}
void UiScreenGlRenderer::update_tech_award_animation(const LiveUiFrame& input,const BnwClientFrame& frame,
 const std::shared_ptr<const BnwTechAwardAnimationSample>& supplied,const std::function<bool()>& source_current){
 const auto lifetime=tech_award_lifetime_;
 alpha_need(lifetime->creator==std::this_thread::get_id()&&lifetime->alive&&!lifetime->failed,
            "TechAward animation updater retired/foreign thread");
 if(lifetime->active){lifetime->failed=true;throw std::runtime_error("TechAward animation updater reentry");}
 lifetime->active=true;struct End{std::shared_ptr<TechAwardPaintLifetime> p;~End(){p->active=false;}} end{lifetime};
 const auto sample=supplied;const auto held=input;const auto* issued=held.snapshot.find<BnwTechAwardContextSnapshot>();
 const auto witness=issued?issued->animation:nullptr;
 std::optional<std::function<bool()>> target;std::optional<AnimationSources> lender;
 try {
  lender.emplace(lifetime);const auto& sources=*lender;sources.pure();
  alpha_need(sample&&alpha_control(sample,sample->self_.lock())&&!sample->consumed_,"TechAward repeated/foreign UI update sample");
  sample->require_current();sample->consumed_=true;
  const auto pure=[&]{sources.pure();sample->require_current();if(witness)witness->require_current_without_callbacks();};
  const auto full=[&]{pure();sources.full(context_,&held,&frame);pure();
   if(witness){alpha_need(issued->context.client_layout!=nullptr,"TechAward animation actual layout missing");
    witness->require_publication(frame.stamp,issued->occurrence,issued->generation,issued->context.client_layout);pure();}};
  full();target.emplace(source_current);pure();alpha_need(bool(*target),"TechAward UI update has no actual source target");
  alpha_need((*target)(),"TechAward UI update source failed before phase arithmetic");pure();full();
  auto next=witness?witness->advance(*sample):std::vector<std::pair<std::string,float>>{};
  pure();full();alpha_need((*target)(),"TechAward UI update source failed after phase arithmetic");pure();full();
  // Real copied target destruction is still inside the same active renderer
  // and accounted Scene/Context lender scope; its effects are checked afresh.
  target.reset();pure();full();pure();
#ifdef _WIN32
  // A fresh real UI update precedes this frame's paint. Closing the popup
  // retires its previous presentation without treating ordinary Hide as exit.
  tech_award_painted_ui_.reset();tech_award_painted_context_.reset();tech_award_painted_frame_.reset();
  tech_award_presented_context_.reset();tech_award_presented_frame_.reset();
#endif
  tech_award_alphas_.swap(next);tech_award_updated_sample_=sample;tech_award_updated_context_=witness;
 }catch(...){lifetime->failed=true;if(witness)witness->quarantine();target.reset();lender.reset();throw;}
}
}

namespace civ5::ui {

BnwWorldViewRendererRunLoan::State::State(const UiScreenGlRenderer* r,
 const graphics::GlWaterWorldRenderer* w,const terrain::CameraControllerState* c,
 const std::shared_ptr<RendererLifetime>& life,
 const std::shared_ptr<const terrain::TerrainWorld>& actual_terrain,
 const std::shared_ptr<BnwClientSceneFrame>& actual_scene)
 :renderer(r),water(w),controller(c),renderer_life(life),terrain_owner(actual_terrain),scene_owner(actual_scene),
  map_guid(actual_scene?actual_scene->receipt().map_guid:std::array<std::uint8_t,16>{}),
  map_width(actual_scene?actual_scene->receipt().width:0),map_height(actual_scene?actual_scene->receipt().height:0){
 if(!actual_terrain||!actual_scene||!map_width||!map_height||
    actual_terrain->source().layout.source_grid.width!=static_cast<std::int64_t>(map_width)||
    actual_terrain->source().layout.source_grid.height!=static_cast<std::int64_t>(map_height))
  throw std::runtime_error("WorldView Camera terrain has another actual Scene map extent");
}
void BnwWorldViewRendererRunLoan::State::require_current(const UiScreenGlRenderer* receiver)const{
 // Foreign revocation touches atomic poison before creator-only mutable/raw data.
 if(creator!=std::this_thread::get_id()){const_cast<State*>(this)->failed=true;throw std::runtime_error("WorldView Camera run foreign creator");}
 if(!alive||failed)throw std::runtime_error("WorldView Camera run retired");
 const auto life=renderer_life.lock();
 if(!life||life->creator!=creator||!life->alive||life->failed||
    life->renderer!=renderer||receiver!=renderer||!water||!controller)
  throw std::runtime_error("WorldView Camera run has another or retired actual renderer");
}
std::shared_ptr<BnwWorldViewRendererRunLoan::State>
 BnwWorldViewRendererRunLoan::Binding::current_for(const UiScreenGlRenderer* renderer)const{
 const auto state=state_.lock();
 if(!state)throw std::runtime_error("WorldView Camera run binding retired");
 state->require_current(renderer);return state;
}
BnwWorldViewRendererRunLoan::BnwWorldViewRendererRunLoan(UiScreenGlRenderer& renderer,
 const graphics::GlWaterWorldRenderer& water,const terrain::CameraControllerState& controller,
 const std::shared_ptr<const terrain::TerrainWorld>& terrain,
 const std::shared_ptr<BnwClientSceneFrame>& scene)
 :state_(std::make_shared<State>(&renderer,&water,&controller,renderer.worldview_lifetime_,terrain,scene)){
 state_->require_current(&renderer);
}
BnwWorldViewRendererRunLoan::~BnwWorldViewRendererRunLoan(){
 if(state_->creator!=std::this_thread::get_id())std::terminate();
 state_->alive=false;
}
BnwWorldViewRendererRunLoan::Binding BnwWorldViewRendererRunLoan::bind_owned()const{
 state_->require_current(state_->renderer);return Binding(state_);
}
terrain::CameraFrameState BnwWorldViewRendererProjectionLoan::capture_current_camera(
 const std::shared_ptr<BnwWorldViewRendererRunLoan::State>& run,const UiScreenGlRenderer* renderer){
 run->require_current(renderer);
 const auto captured=run->water->camera_state();
 run->require_current(renderer);return captured;
}
BnwWorldViewRendererProjectionLoan::BnwWorldViewRendererProjectionLoan(
 UiScreenGlRenderer& renderer,const BnwWorldViewRendererRunLoan::Binding& binding,
 const std::shared_ptr<BnwClientSceneFrame>& scene,std::int32_t width,std::int32_t height)
 :run_(binding.current_for(&renderer)),renderer_(&renderer),lifetime_(run_->renderer_life.lock()),
  scene_(scene),frame_(renderer.capture_worldview_projection_frame(scene)),
  camera_(capture_current_camera(run_,&renderer)),width_(width),height_(height),unwind_(std::uncaught_exceptions()){
 try {require_current_without_callbacks();}
 catch(...){lifetime_->failed=true;throw;}
}
BnwWorldViewRendererProjectionLoan::~BnwWorldViewRendererProjectionLoan(){
 if(lifetime_->creator!=std::this_thread::get_id())std::terminate();
 if(std::uncaught_exceptions()>unwind_)lifetime_->failed=true;
}
void BnwWorldViewRendererProjectionLoan::require_current_without_callbacks()const {
 // This retained token is checked before touching the borrowed renderer.
 if(lifetime_->creator!=std::this_thread::get_id()||!lifetime_->alive||
    lifetime_->failed||lifetime_->renderer!=renderer_){
  lifetime_->failed=true;throw std::runtime_error("WorldView projection renderer retired or foreign creator");
 }
 run_->require_current(renderer_);
 try {renderer_->require_worldview_projection_current(*this);}
 catch(...){lifetime_->failed=true;throw;}
}
void UiScreenGlRenderer::require_worldview_projection_current(
 const BnwWorldViewRendererProjectionLoan& projection)const {
 const auto& p=projection;
 if(p.renderer_!=this||p.lifetime_.get()!=worldview_lifetime_.get()||
    p.lifetime_.owner_before(worldview_lifetime_)||worldview_lifetime_.owner_before(p.lifetime_)||
    !p.lifetime_->alive||p.lifetime_->failed)
  throw std::runtime_error("WorldView projection has another canonical renderer");
 const auto scene=p.scene_.lock();
 if(!scene||scene->gui_thread_!=std::this_thread::get_id()||
    scene->gui_self_.expired()||scene.owner_before(scene->gui_self_)||scene->gui_self_.owner_before(scene)||
    scene->gui_self_.use_count()<=static_cast<long>(scene->gui_publication_pins_.load())+1)
  throw std::runtime_error("WorldView actual Scene outside owner retired");
 scene->require_publication_finality(p.frame_);
 {
  const auto bound_scene=p.run_->scene_owner.lock();
  const auto terrain=p.run_->terrain_owner.lock();
  if(!bound_scene||bound_scene.get()!=scene.get()||
     bound_scene.owner_before(scene)||scene.owner_before(bound_scene)||
     !terrain||p.run_->terrain_owner.use_count()<=1||
     !p.frame_||p.frame_->stamp.identity.map_guid!=p.run_->map_guid||
     p.frame_->world.map_header.guid!=p.run_->map_guid||
     p.frame_->world.map_header.grid.width!=p.run_->map_width||
     p.frame_->world.map_header.grid.height!=p.run_->map_height||
     terrain->source().layout.source_grid.width!=static_cast<std::int64_t>(p.run_->map_width)||
     terrain->source().layout.source_grid.height!=static_cast<std::int64_t>(p.run_->map_height)||
     p.frame_->world.map_header.wrap_x!=(terrain->source().map_mode==1)||
     p.frame_->world.map_header.wrap_y)
   throw std::runtime_error("WorldView actual immutable terrain/Scene map origin changed");
 }

 if(!p.frame_||p.width_<=0||p.height_<=0||
    p.frame_->stamp.viewport.framebuffer_width!=p.width_||
    p.frame_->stamp.viewport.framebuffer_height!=p.height_||!p.frame_->stamp.viewport.drawable()||
    p.frame_->stamp.viewport.client_width!=p.width_||p.frame_->stamp.viewport.client_height!=p.height_)
  throw std::runtime_error("WorldView actual projection framebuffer/frame differs");
 p.run_->require_current(this);
 const auto& a=p.run_->controller->camera;const auto& b=p.frame_->camera.camera;
 if(a.world!=b.world||a.field_of_view!=b.field_of_view||a.aspect_ratio!=b.aspect_ratio||
    a.near_plane!=b.near_plane||a.far_plane!=b.far_plane)
  throw std::runtime_error("WorldView actual controller differs from latched Scene camera");
 p.run_->require_current(this);
 const auto& live=p.run_->water->camera_state();const auto& saved=p.camera_;
 if(!saved.view_count||saved.view_count>saved.views.size()||live.view_count!=saved.view_count||
    live.projection!=saved.projection)
  throw std::runtime_error("WorldView actual published camera projection changed");
 for(std::size_t n=0;n<saved.view_count;++n)
  if(live.views[n]!=saved.views[n]||live.inverse_views[n]!=saved.inverse_views[n]||
     live.relative_offsets[n]!=saved.relative_offsets[n])
   throw std::runtime_error("WorldView actual published wrap view changed");
 p.run_->require_current(this);
 if(!p.run_->water->color_texture().belongs_to(context_))
  throw std::runtime_error("WorldView production camera renderer has another GL context");
 p.run_->require_current(this);
}
std::optional<std::array<double,2>> BnwWorldViewRendererProjectionLoan::project(
 const std::array<double,3>& point)const {
 require_current_without_callbacks();
 for(const auto value:point)if(!std::isfinite(value))
  throw std::runtime_error("WorldView genuine WorldAnchor point is nonfinite");
 for(std::size_t view=0;view<camera_.view_count;++view){
  const auto matrix=terrain::matrix_multiply(camera_.views[view],camera_.projection,
    terrain::X87Precision::double_precision,terrain::MatrixPath::sse);
  for(const auto value:matrix)if(!std::isfinite(value))
   throw std::runtime_error("WorldView published projection matrix is nonfinite");
  const double v[4]{point[0],point[1],point[2],1.0};double clip[4]{};
  for(std::size_t column=0;column<4;++column)
   for(std::size_t row=0;row<4;++row)clip[column]+=v[row]*matrix[row*4+column];
  bool finite=true;for(const auto value:clip)finite=finite&&std::isfinite(value);
  if(!finite)continue;
  const double w=clip[3];
  if(!std::isfinite(w)||w<=0||clip[0]<-w||clip[0]>w||clip[1]<-w||clip[1]>w||
     clip[2]<0||clip[2]>w)continue;
  const double x=(clip[0]/w+1.0)*width_*0.5;
  const double y=(1.0-clip[1]/w)*height_*0.5;
  if(std::isfinite(x)&&std::isfinite(y)){
   require_current_without_callbacks();return std::array<double,2>{x,y};
  }
 }
 require_current_without_callbacks();return std::nullopt;
}
struct UiScreenGlRenderer::WorldViewGridProvider final : BnwWorldViewGridToWorldLoan {
 const BnwWorldViewRendererProjectionLoan& projection_;
 const std::shared_ptr<BnwWorldViewRendererRunLoan::RendererLifetime> life_;
 const int unwind_;
 mutable bool querying_=false;
 explicit WorldViewGridProvider(const BnwWorldViewRendererProjectionLoan&);
 ~WorldViewGridProvider() override;
 void require_for_source(const runtime::BnwWorldViewSourceOperationLoan&)const override;
 std::shared_ptr<detail::BnwLabelProcessModuleOwner> label_module_for_source_owned(
   const runtime::BnwWorldViewSourceOperationLoan&)const override;
 std::unique_ptr<graphics::detail::BnwFontTextureObservation> query_font_texture_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const runtime::detail::BnwFontResourceLoan&,
        const detail::BnwFontTextureRequest& )const override;
 std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> query_system_font_atlas_owned(
   const runtime::BnwWorldViewSourceOperationLoan&,const BnwOwnedCameraGuiScreen::Loan&,
   const graphics::BnwOwnedCameraRendererWindowLease::Loan&,const detail::BnwLabelSystemFontRequest::Guard&)const override;
 std::array<double,3> world_xyz(const runtime::BnwWorldViewSourceOperationLoan&,
   std::int32_t,std::int32_t)const override;
 void with_notification_initial_lua_resources_owned(const runtime::BnwWorldViewSourceOperationLoan&,
   const BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
   const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&,const std::function<void()>&)const override;
 void prepare_clear_hex_highlights_owned(const runtime::BnwWorldViewSourceOperationLoan&,
   const BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&)const override;
 std::shared_ptr<graphics::BnwClearHexHighlightOwner> clear_hex_highlight_owner_owned(
   const runtime::BnwWorldViewSourceOperationLoan&)const override;
 void prepare_notification_cpu_geometry_owned(const runtime::BnwWorldViewSourceOperationLoan&,
   const BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
   const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&)const override;
};
} // namespace civ5::ui
#include "bnw_clear_hex_highlight_owner_core.inc"
namespace civ5::ui {
#include "bnw_font_texture_cache_owner_core.inc"
#include "bnw_label_system_font_atlas_receiver_core.inc"

UiScreenGlRenderer::WorldViewGridProvider::WorldViewGridProvider(const BnwWorldViewRendererProjectionLoan& p)
 :projection_(p),life_(p.lifetime_),unwind_(std::uncaught_exceptions()){
 projection_.require_current_without_callbacks();
}
UiScreenGlRenderer::WorldViewGridProvider::~WorldViewGridProvider(){
 if(life_->creator!=std::this_thread::get_id())std::terminate();
 if(std::uncaught_exceptions()>unwind_)life_->failed=true;
}
void UiScreenGlRenderer::WorldViewGridProvider::require_for_source(
 const runtime::BnwWorldViewSourceOperationLoan& source)const{
 if(life_->creator!=std::this_thread::get_id()||!life_->alive||life_->failed){
  life_->failed=true;throw std::runtime_error("WorldView GridToWorld provider retired or foreign creator");
 }
 try {
  projection_.require_current_without_callbacks();
  require_scene_for_source(source,projection_.scene_);
  projection_.require_current_without_callbacks();
 }catch(...){life_->failed=true;throw;}
}
std::shared_ptr<detail::BnwLabelProcessModuleOwner>
UiScreenGlRenderer::WorldViewGridProvider::label_module_for_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& source)const{
 require_for_source(source);
 const auto* const renderer=projection_.renderer_;
 if(!renderer||life_->renderer!=renderer)
  throw std::runtime_error("Label module Grid lender actual renderer changed");
 const auto result=renderer->label_module_;
 if(!result)throw std::runtime_error("Label module actual renderer constructor supplier missing");
 result->require_renderer_origin_owned(*renderer);require_for_source(source);
 if(renderer->label_module_.get()!=result.get()||renderer->label_module_.owner_before(result)||
    result.owner_before(renderer->label_module_))
  throw std::runtime_error("Label module renderer member replaced during lending");
 result->require_renderer_origin_owned(*renderer);return result;
}
std::array<double,3> UiScreenGlRenderer::WorldViewGridProvider::world_xyz(
 const runtime::BnwWorldViewSourceOperationLoan& source,std::int32_t x,std::int32_t y)const{
 require_for_source(source);
 if(querying_){life_->failed=true;throw std::runtime_error("WorldView GridToWorld query reentered");}
 querying_=true;struct End{bool& active;~End(){active=false;}} end{querying_};
 try {
  const auto serial=source.operation_serial();require_for_source(source);
  source.require_operation(serial);require_for_source(source);
  std::array<double,3> result;
  {
   // The temporary immutable terrain anchor is confined to a callback-free
   // native center/nearest-u16 read. It never hides retirement across providers.
   const auto actual=projection_.run_->terrain_owner.lock();
   if(!actual||projection_.run_->terrain_owner.use_count()<=1)
    throw std::runtime_error("WorldView GridToWorld actual terrain outside owner retired");
   const auto& layout=actual->source().layout;
   const auto& height=actual->owner();
   if(x<0||y<0||x>=layout.source_grid.width||y>=layout.source_grid.height)
    throw std::runtime_error("WorldView GridToWorld source grid coordinate out of bounds");
   const auto center=graphics::bnw_unit_world_center(layout,height.parameters,x,y);
   const auto& p=height.parameters;
   if(p.width<=0||p.height<=0||!std::isfinite(p.scale_x)||!std::isfinite(p.scale_y)||
      p.scale_x<=0||p.scale_y<=0||!std::isfinite(p.center_x)||!std::isfinite(p.center_y)||
      !std::isfinite(p.scale_z)||height.height.element_bytes()!=2)
    throw std::runtime_error("WorldView GridToWorld real height owner invalid");
   const float fx=(center.x+p.center_x)/p.scale_x,fy=(center.y+p.center_y)/p.scale_y;
   if(!std::isfinite(fx)||!std::isfinite(fy)||fx<-2147483648.0F||fx>=2147483648.0F||
      fy<-2147483648.0F||fy>=2147483648.0F)
    throw std::runtime_error("WorldView GridToWorld real height coordinate unrepresentable");
   auto ix=static_cast<std::int32_t>(fx);const auto iy=static_cast<std::int32_t>(fy);
   const bool wrap=actual->source().map_mode==1;
   if(wrap)ix=terrain::wrap_add(terrain::wrap_mul(p.width,100),ix)%p.width;
   if(ix<0||iy<0||ix>=height.height.domain().width()||iy>=height.height.domain().height())
    throw std::runtime_error("WorldView GridToWorld actual nearest height source absent");
   const auto z=graphics::bnw_unit_world_height(height,center.x,center.y,wrap);
   result={center.x,center.y,z};
   for(const auto value:result)if(!std::isfinite(value))
    throw std::runtime_error("WorldView GridToWorld real terrain point nonfinite");
  }
  require_for_source(source);source.require_operation(serial);require_for_source(source);
  return result;
 }catch(...){life_->failed=true;throw;}
}
void UiScreenGlRenderer::with_notification_terrain_scene_owned(
 BnwNotificationPublicationLender& lender,
 const std::function<void(BnwNotificationTerrainSceneLoan&)>& target){
 // Private uncalled Root hook, never a caller matrix/ready issuer.
 const auto life=worldview_lifetime_;
 if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
    life->renderer!=this||!life->active||!life->active_scope){
  life->failed=true;
  throw std::runtime_error("Notification terrain needs the actual active WorldView paint scope");
 }
 if(lender.creator_!=std::this_thread::get_id()){
  life->failed=true;
  throw std::runtime_error("Notification terrain lender foreign creator");
 }
 const auto storage=lender.storage_;
 try{BnwNotificationPublicationLender::pure(storage);}
 catch(...){
  life->failed=true;
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(storage);
  throw;
 }
 auto* const scope=static_cast<WorldViewPaintScope*>(const_cast<void*>(life->active_scope));
 if(scope->life.get()!=life.get()||scope->life.owner_before(life)||life.owner_before(scope->life)||
    scope->notification_terrain_active){
  life->failed=true;
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(storage);
  throw std::runtime_error("Notification terrain scope substituted or reentered");
 }
 scope->notification_terrain_active=true;
 struct End {
  std::shared_ptr<WorldViewPaintLifetime> life;
  WorldViewPaintScope* scope;
  ~End() noexcept {
   // Never touch a borrowed scope after callback retirement of its renderer.
   if(life->creator==std::this_thread::get_id()&&life->alive&&!life->failed&&
      life->active&&life->active_scope==scope)
    scope->notification_terrain_active=false;
   else life->failed=true;
  }
 } end{life,scope};
 const auto pure=[life,scope,storage]{
  // Retained independent life FIRST, before raw scope/Projection/producer reads.
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
     !life->active||life->active_scope!=scope){
   life->failed=true;
   throw std::runtime_error("Notification terrain renderer scope retired during admission");
  }
  BnwNotificationPublicationLender::pure(storage);
  if(!scope->notification_terrain_active||scope->life.get()!=life.get()||
     scope->life.owner_before(life)||life.owner_before(scope->life))
   throw std::runtime_error("Notification terrain active scope control changed");
  scope->pure();
 };
 const auto full=[&]{
  pure();
  BnwNotificationPublicationLender::full(storage);pure();
  scope->full();pure();
 };
 try{
  pure();
  // No raw Lender/this access follows callbackful constructor admission.
  BnwNotificationTerrainSceneLoan loan(scope->projection,lender);
  pure();
  std::unique_ptr<std::function<void(BnwNotificationTerrainSceneLoan&)>> owned;
  try{
   full();pure();
   owned=std::make_unique<std::function<void(BnwNotificationTerrainSceneLoan&)>>(target);
   pure();full();pure();
   if(!*owned)throw std::invalid_argument("Notification terrain target absent");
   // The wrapper contains only lexical references. User target COPY/DTOR are
   // guarded here; the same loan guards wrapper COPY/invoke/DTOR.
   loan.invoke_owned([&](BnwNotificationTerrainSceneLoan& actual){
    pure();full();pure();
    (*owned)(actual);pure();full();pure();
   });
   pure();full();pure();
   owned.reset();pure();full();pure();
  }catch(...){
   life->failed=true;
   BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(storage);
   // Destroy the user target while its actual stack Loan still exists.
   owned.reset();throw;
  }
 }catch(...){
  life->failed=true;
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(storage);
  throw;
 }
}
}

namespace civ5::ui {
void UiScreenGlRenderer::WorldViewGridProvider::with_notification_initial_lua_resources_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
 const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& actual_trees,
 const std::function<void()>& target)const {
 const auto life=life_;
 const auto& projection=projection_;
 const auto first=[&]{
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
     !life->active||life->active_scope)
   throw std::runtime_error("Initial Lua resources lost actual pre-paint renderer lifetime");
 };
 first();projection.require_current_without_callbacks();first();
 auto* const renderer=const_cast<UiScreenGlRenderer*>(projection.renderer_);
 if(life->renderer!=renderer){life->failed=true;throw std::runtime_error("Initial Lua resources renderer origin changed");}
 try {
  require_for_source(operation);first();
  // Do not mark querying_: legitimate Lua GridToWorld calls remain distinct
  // actual queries. ResourceBridge owns its independent sticky reentry fence.
  renderer->with_notification_layout_resources_owned(projection,operation,gui,window,actual_trees,target);
  first();require_for_source(operation);first();
 }catch(...){life->failed=true;throw;}
}

void UiScreenGlRenderer::WorldViewGridProvider::prepare_notification_cpu_geometry_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
 const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& issuer_trees)const {
 const auto diagnostic_life=life_; // Pin before any Source/observer; never raw this after logging.
 // Called by the actual issuer AFTER canonical construction and BEFORE Lua.
 // Issuer order is InGame,WorldView,UnitFlagManager,ActionInfoPanel,Notification.
 require_for_source(operation);
 if(querying_){life_->failed=true;throw std::runtime_error("Notification CPU producer reentered");}
 querying_=true;struct EndQuery{bool& v;~EndQuery(){v=false;}} end_query{querying_};
 try {
  if(!life_->active||life_->active_scope)throw std::runtime_error("Notification CPU producer needs exclusive actual renderer scope");
  const std::array<std::shared_ptr<BnwWorldViewControlTree>,5> trees={
   issuer_trees[1],issuer_trees[2],issuer_trees[3],issuer_trees[0],issuer_trees[4]};
  // Real NativeIssuer entry BEFORE Lua and BEFORE immutable paint revisions.
  // No Context-init/5761E0 completion is inferred from this parser/4C path.
  const auto resource_life=life_;
  auto* const resource_renderer=const_cast<UiScreenGlRenderer*>(projection_.renderer_);
  const auto resource_first=[&]{
   if(resource_life->creator!=std::this_thread::get_id()||!resource_life->alive||
      resource_life->failed||!resource_life->active||resource_life->active_scope||
      resource_life->renderer!=resource_renderer)
    throw std::runtime_error("Notification pre-Lua resource caller retired or lost actual renderer scope");
  };
  const auto resource_full=[&]{
   resource_first();projection_.require_current_without_callbacks();resource_first();
   operation.require_current();resource_first();
   gui.require_current();resource_first();window.require_current();resource_first();
   projection_.require_current_without_callbacks();resource_first();
  };
  std::array<WorldViewPaintScope::VisualCache,5> initial_layout_cache;
  resource_full();
  resource_renderer->with_notification_layout_resources_owned(
   projection_,operation,gui,window,trees,[&]{
    resource_full();
    std::map<std::string,content::XmlNode,std::less<>> actual_styles;
    for(const auto key:{"Assets/UI/Styles.xml","Assets/DLC/Expansion/UI/Styles_Expansion1.xml","Assets/DLC/Expansion2/UI/Styles_Expansion2.xml"}){
     resource_full();
     const auto mounted=resource_renderer->resources_.resolve(assets::AssetKey(key));
     resource_first();resource_full();
     const auto* handle=std::get_if<assets::AssetHandle>(&mounted.asset);
     if(!handle)throw std::runtime_error("Notification pre-Lua selected styles unavailable or ambiguous");
     const auto bytes=handle->read_bytes(16ull*1024*1024);resource_first();resource_full();
     const auto authored=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
     for(const auto& value:authored.children){
      if(value.name.empty())throw std::runtime_error("Notification pre-Lua selected style name empty");
      actual_styles.insert_or_assign(value.name,value);
     }
    }
    resource_full();
    // Parse actual retained scalar chains without autoloading Notification
    // ahead of its real parent. Constructor state is not an initializer return.
    trees[3]->prepare_selected_client_registry_baselines_owned(operation,actual_styles);
    resource_first();resource_full();
    // Ui reorder slot3 is the actual NativeIssuer slot0 InGame root.
    // The private entry owns first preload/5761, or validates its authentic
    // completed return; no renderer flag, epoch repair or repeated4C prefix.
    trees[3]->ensure_selected_root_context_initialized_owned(operation);
    resource_first();resource_full();
    const auto serial=operation.operation_serial();resource_first();resource_full();
    for(std::size_t index=0;index<trees.size();++index){
     const auto tree=trees[index];auto& cached=initial_layout_cache[index];
     cached.tree=tree;cached.serial=serial;cached.revision=tree->render_revision_owned();
     resource_first();resource_full();
     const auto same=[&]{
      resource_first();tree->require_render_revision_without_callbacks(cached.revision,operation,serial);
      resource_first();resource_full();
     };
     const auto controls=tree->render_controls_owned();same();
     for(const auto& control:controls){
      const auto quote=tree->inspect_control(control);same();
      if(quote.authored.name!="Stack")continue;
      auto value=tree->stack_layout_owned(control,operation);same();
      if(!value.actual_size_calculation&&!value.earned_empty_initializer)
       throw std::runtime_error("Notification Stack capture lacks actual initializer return");
      if(!cached.stacks.emplace(quote.id,std::move(value)).second)
       throw std::runtime_error("Notification actual Stack result duplicated");
     }
     same();
    }
   });
  resource_first();resource_full();
  // Actual resource bindings/target DTOR have completed. Validate all captured
  // epochs before immutable paint birth and consume only these closed copies.
  for(std::size_t index=0;index<trees.size();++index){
   resource_first();trees[index]->require_render_revision_without_callbacks(
    initial_layout_cache[index].revision,operation,initial_layout_cache[index].serial);
   resource_first();resource_full();
  }
  WorldViewPaintScope scope(life_,projection_,operation,gui,window,trees);
  scope.visual_cache=&initial_layout_cache;
  for(std::size_t index=0;index<trees.size();++index)
   if(scope.revisions[index]!=initial_layout_cache[index].revision)
    throw std::runtime_error("Notification Stack resource capture epoch changed before paint");
  life_->active_scope=&scope;
  struct EndScope{std::shared_ptr<WorldViewPaintLifetime> p;~EndScope(){p->active_scope=nullptr;}} end_scope{life_};
  scope.pure();
  auto* renderer=const_cast<UiScreenGlRenderer*>(projection_.renderer_);
  // FIRST independent renderer/source guard precedes every borrowed reuse.
  // Accepted ResourceFactory1 proves SAME canonical Scene/Host/Bank table;
  // its temporary locks end before any callbackful full validation.
  const auto pure=[&]{scope.pure();require_for_source(operation);scope.pure();
   renderer->require_masked_resources_origin_without_callbacks();scope.pure();};
  const auto full=[&]{pure();scope.full();pure();};
  full();
  gui.owner_.with_worldview_frame_storage_owned(gui,[&](const auto& scene,const auto& frame,
    const auto& live_ui,const auto& layout){
   pure();full();
   {
    const auto same_scene=projection_.scene_.lock();
    if(!same_scene||same_scene.get()!=&scene||projection_.frame_.get()!=&frame||
       scene.frame().get()!=projection_.frame_.get()||scene.frame().owner_before(projection_.frame_)||
       projection_.frame_.owner_before(scene.frame())||layout.frame_owner!=live_ui.owner||
       layout.frame_sequence!=live_ui.sequence||layout.snapshot_frame!=live_ui.snapshot.frame()||
       !frame.stamp.viewport.drawable()||!live_ui.world_view)
     throw std::runtime_error("Notification CPU geometry has another actual GUI Scene/frame");
   }
   const auto frame_revision=gui.frame_revision();pure();full();
   pure();
   const auto& tree=trees[4];
   if(tree->context()!="NotificationPanel")throw std::runtime_error("Notification CPU canonical context substituted");pure();
   const auto actual_source=tree->actual_source();pure();operation.require_same_source(*actual_source);pure();
   const auto root=tree->render_context_owned();pure();
   const auto action_root=trees[2]->render_context_owned();pure();
   const content::XmlNode* descriptor=nullptr;
   for(const auto& child:action_root.children)if(child.name=="LuaContext"&&child.attribute("ID")=="NotificationPanel"){
    if(descriptor||child.attribute("FileName")!="Assets/UI/InGame/WorldView/NotificationPanel")
     throw std::runtime_error("Notification CPU actual direct context descriptor invalid");descriptor=&child;
   }
   if(!descriptor)throw std::runtime_error("Notification CPU requires coherent retained direct LuaContext metadata");
   for(const auto key:{"Style","Size","Offset","Anchor","AnchorSide","Hidden","BranchAlpha","Font","FontStyle","ColorSet","Parent"})
    if(!descriptor->attribute(key).empty())throw std::runtime_error("Notification CPU direct descriptor geometry/profile changed");
   // Same selected resource table as actual renderer. Styles are source values;
   // styles may supply attributes, never fabricate canonical child controls.
   std::map<std::string,content::XmlNode,std::less<>> styles;
   for(const auto key:{"Assets/UI/Styles.xml","Assets/DLC/Expansion/UI/Styles_Expansion1.xml","Assets/DLC/Expansion2/UI/Styles_Expansion2.xml"}){
    full();const auto mounted=renderer->resources_.resolve(assets::AssetKey(key));pure();full();
    const auto* handle=std::get_if<assets::AssetHandle>(&mounted.asset);
    if(!handle)throw std::runtime_error("Notification CPU selected styles unavailable or ambiguous");
    const auto bytes=handle->read_bytes(16ull*1024*1024);pure();full();
    const auto authored=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
    for(const auto& value:authored.children){if(value.name.empty())throw std::runtime_error("Notification CPU style name empty");styles.insert_or_assign(value.name,value);}
   }
   // Baselines and actual root5761 completed in the genuine resource phase.
   // Do not replay parser/preload/4C here or call a resource getter in paint.
   pure();full();
   detail::NotificationCpuGeometry root_geometry;
   root_geometry.rect={0,0,static_cast<float>(frame.stamp.viewport.client_width),static_cast<float>(frame.stamp.viewport.client_height)};
   root_geometry.font=action_root.attribute("Font");root_geometry.font_style=action_root.attribute("FontStyle");root_geometry.color_set=action_root.attribute("ColorSet");
   for(const auto& field:std::array<std::pair<const char*,std::string*>,3>{{{"Font",&root_geometry.font},{"FontStyle",&root_geometry.font_style},{"ColorSet",&root_geometry.color_set}}}){
    const auto value=root.attribute(field.first);if(!value.empty())*field.second=value;
   }
   const auto controls=tree->render_controls_owned();pure();
   std::vector<BnwWorldViewControlTree::ControlQuote> quotes;quotes.reserve(controls.size());
   for(const auto& control:controls){auto quote=tree->inspect_control(control);pure();quotes.push_back(std::move(quote));}
   std::map<std::uint64_t,detail::NotificationCpuGeometry> parents;
   std::map<std::uint64_t,const BnwWorldViewControlTree::StackLayoutOwned*> stack_parents;
   const auto& native_cache=scope.visual_for_tree_owned(tree,scope.revisions[4]);pure();
   std::vector<detail::NotificationCpuGeometry> computed;computed.reserve(quotes.size());
   for(const auto& q:quotes){
    pure();if(q.external_parent_id||!q.alive)throw std::runtime_error("Notification CPU foreign or removed canonical leaf");
    detail::NotificationCpuLeafInput input;input.effective=q.authored;input.parent=root_geometry;
    if(q.parent){const auto found=parents.find(q.parent);if(found==parents.end())throw std::runtime_error("Notification CPU attachment order lost parent");input.parent=found->second;}
    // Follow actual named style chain before authored leaf attributes, matching
    // native OR-only AnchorSide assignment for the supported two-token profile.
    content::XmlNode effective=q.authored;effective.attributes.clear();std::set<std::string> seen;
    std::vector<const content::XmlNode*> chain;const auto* selected=&q.authored;
    while(!selected->attribute("Style").empty()){
     const auto name=selected->attribute("Style");if(!seen.insert(name).second)throw std::runtime_error("Notification CPU actual style cycle");
     const auto found=styles.find(name);if(found==styles.end())throw std::runtime_error("Notification CPU selected style absent");selected=&found->second;chain.push_back(selected);
    }
    const auto merge=[&](const content::XmlNode& value){
     input.prior_authored_side_flags|=detail::notification_authored_side_flags(value.attribute("AnchorSide"));
     for(const auto& attr:value.attributes){auto at=std::find_if(effective.attributes.begin(),effective.attributes.end(),[&](const auto& old){return old.first==attr.first;});
      if(at==effective.attributes.end())effective.attributes.push_back(attr);else at->second=attr.second;}
    };
    for(auto it=chain.rbegin();it!=chain.rend();++it)merge(**it);merge(q.authored);input.effective=std::move(effective);
    input.actual_offset=q.offset;input.actual_size_y=q.size_y_override;input.hidden=q.hidden;
    const BnwWorldViewControlTree::StackLayoutOwned* own_stack=nullptr;
    if(q.authored.name=="Stack"){
     const auto found=native_cache.stacks.find(q.id);
     if(found==native_cache.stacks.end())throw std::runtime_error("Notification current Stack result absent");
     own_stack=&found->second;
    }
    const BnwWorldViewControlTree::StackLayoutOwned* parent_stack=nullptr;
    if(q.parent){const auto found=stack_parents.find(q.parent);if(found!=stack_parents.end())parent_stack=found->second;}
    const BnwWorldViewControlTree::StackLayoutOwned::Child* child_result=nullptr;
    if(parent_stack){
     for(const auto& child:parent_stack->children)
      if(child.tree.get()==tree.get()&&!child.tree.owner_before(tree)&&!tree.owner_before(child.tree)&&child.id==q.id){
       if(child_result||!child.control)throw std::runtime_error("Notification Stack returned child ambiguous");
       child_result=&child;
      }
     if(!child_result)throw std::runtime_error("Notification Stack derived child result missing");
    }
    const bool native_route=own_stack||child_result;
    detail::NotificationCpuGeometry own;
    if(native_route){
     own=input.parent;own.hidden=input.parent.hidden||q.hidden;
     for(const auto& field:std::array<std::pair<const char*,std::string*>,3>{{{"Font",&own.font},{"FontStyle",&own.font_style},{"ColorSet",&own.color_set}}}){
      const auto value=input.effective.attribute(field.first);if(!value.empty())*field.second=value;
     }
     const auto offset=own_stack?own_stack->native_offset:child_result->native_offset;
     const auto size=own_stack?own_stack->native_size:child_result->native_size;
     if(own_stack&&child_result&&own_stack->native_offset!=child_result->native_offset)
      throw std::runtime_error("Notification inner Stack returned offset changed");
     std::array<float,2> origin{input.parent.rect[0],input.parent.rect[1]};
     if(child_result)for(std::size_t axis=0;axis<2;++axis)
      origin[axis]+=parent_stack->anchor_origin[axis]-parent_stack->native_offset[axis]+child_result->derived_offset[axis];
     own.offset=offset;own.intrinsic_size=size;
     own.rect={origin[0]+offset[0],origin[1]+offset[1],size[0],size[1]};
     for(const auto value:own.rect)if(!std::isfinite(value))throw std::runtime_error("Notification native Stack rect nonfinite");
     own.measurement_source="actual-owned-Stack-initializer";
    }else own=detail::notification_cpu_leaf(input);
    if(!native_route&&q.authored.name=="Label"&&input.effective.attribute("Size").empty()){
     const auto text=q.text.value_or(input.effective.attribute("String"));
     full();const auto* style=renderer->resolve_worldview_font_style(own.font,own.font_style);pure();full();
     const GlyphGenGeometry* geometry=style?style->layout_geometry:nullptr;
     // Exact style unavailable is a failure; there is no N/A/generic-font width.
     if(!geometry)throw std::runtime_error("Notification CPU exact active GG/style geometry unavailable");
     std::int32_t width=0;
     if(text.find('[')!=std::string::npos){
      if(!renderer->icons_.geometry)throw std::runtime_error("Notification CPU actual FontIcons absent");
      const auto line=project_glyphgen_markup(*geometry,*renderer->icons_.geometry,text,renderer->worldview_icon_resolver());pure();full();width=line.advance_width;
     }else {width=project_glyphgen_line(*geometry,text).advance_width;pure();}
     input.returned_intrinsic=std::array<float,2>{static_cast<float>(width),static_cast<float>(geometry->line_height)};
     own=detail::notification_cpu_leaf(input);own.measurement_source="actual-renderer-GlyphGen:"+own.font+":"+own.font_style;
    }else if(!native_route&&(q.authored.name=="Image"||q.authored.name=="AlphaAnim"||q.authored.name=="ScrollAnim")&&input.effective.attribute("Size").empty()){
     const auto key=q.texture.value_or(input.effective.attribute("Texture"));
     if(!key.empty()){
      // Raw all-mip source selector calls full after EACH actual resolve/read.
      // No expanded atlas or GL allocation participates in this CPU measure.
      full();const auto raw=graphics::detail::NotificationRawUiTextureSource::load(renderer->resources_,key,full);pure();full();
      std::uint64_t width=raw.binding_.source.width(),height=raw.binding_.source.height();
      if(raw.index_supplier_){
       width=std::uint64_t(raw.binding_.index->width())*raw.block_size_;
       height=std::uint64_t(raw.binding_.index->height())*raw.block_size_;
      }
      if(!width||!height||width>UINT32_MAX||height>UINT32_MAX)throw std::runtime_error("Notification CPU actual logical dimensions invalid");
      input.returned_intrinsic=std::array<float,2>{static_cast<float>(width),static_cast<float>(height)};
      own=detail::notification_cpu_leaf(input);own.measurement_source="actual-selected-raw-UI-resource:"+raw.binding_.source_identity.original_name;
     }
    }
    own.id=q.id;own.parent=q.parent;own.source_serial=scope.serial;own.effect_revision=scope.revisions[4];own.gui_frame_revision=frame_revision;
    if(own_stack&&!stack_parents.emplace(q.id,own_stack).second)
     throw std::runtime_error("Notification current Stack parent duplicated");
    if(!parents.emplace(q.id,own).second)throw std::runtime_error("Notification CPU canonical leaf duplicated");computed.push_back(std::move(own));
   }
   full();tree->publish_notification_cpu_geometry_owned(operation,std::move(computed));pure();full();
  });pure();full();
 }catch(...){note_active_worldview_exception("notification_cpu_geometry");diagnostic_life->failed=true;throw;}
}

const UiGlyphStyleBinding* UiScreenGlRenderer::resolve_worldview_font_style(
 std::string_view font,std::string_view style){
 const auto* fence=worldview_scope_without_callbacks();
 if(!fence)return font_styles_?font_styles_(font,style):nullptr;
 fence->full();
 std::optional<UiGlyphStyleResolver> target;
 try {
  target.emplace(font_styles_);fence->full();
  const auto* result=*target?(*target)(font,style):nullptr;fence->full();
  target.reset();fence->full();return result;
 }catch(...){target.reset();fence->life->failed=true;throw;}
}
std::function<std::optional<std::uint32_t>(std::string_view)>
 UiScreenGlRenderer::worldview_icon_resolver(){
 const auto* fence=worldview_scope_without_callbacks();
 if(!fence)return icon_resolver_;
 fence->full();
 struct Target {
  const WorldViewPaintScope* scope;
  std::optional<std::function<std::optional<std::uint32_t>(std::string_view)>> callback;
  explicit Target(const WorldViewPaintScope* s):scope(s){}
  ~Target(){
   try{scope->full();}catch(...){scope->life->failed=true;}
   callback.reset();
   try{scope->full();}catch(...){scope->life->failed=true;}
  }
 };
 const auto target=std::make_shared<Target>(fence);
 target->callback.emplace(icon_resolver_);fence->full();
 return [target](std::string_view token)->std::optional<std::uint32_t>{
  target->scope->full();
  try {
   const auto value=*target->callback?(*target->callback)(token):std::optional<std::uint32_t>{};
   target->scope->full();return value;
  }catch(...){target->scope->life->failed=true;throw;}
 };
}
}

namespace civ5::ui {
UiGlPaintReport UiScreenGlRenderer::paint_worldview_frame_owned(
 const ComposedUiFrame& frame,std::int32_t width,std::int32_t height){
 auto* scope=const_cast<WorldViewPaintScope*>(worldview_scope_without_callbacks());
 if(!scope||scope->frame||scope->painting)throw std::runtime_error("WorldView actual renderer frame entry absent or reentered");
 scope->full();scope->frame=&frame;
 struct End{WorldViewPaintScope& s;~End(){s.frame=nullptr;}} end{*scope};
 const auto report=paint(frame,width,height);scope->full();return report;
}
UiGlPaintReport UiScreenGlRenderer::paint_worldview_control_publication(
 const BnwWorldViewControlRenderPublication& actual,const ComposedUiFrame& frame,
 std::int32_t width,std::int32_t height){
 const auto* scope=worldview_scope_without_callbacks();
 if(!scope||scope->publication!=&actual){
  worldview_lifetime_->failed=true;throw std::runtime_error("WorldView paint publication is not this actual renderer issuer");
 }
 // Canonical strong Publication precedes private receiver reads/Source.
 const auto original=actual.original_owner_without_callbacks();scope->pure();
 const auto entry=original->actual_root_context_entry_for_paint_without_callbacks();scope->pure();
 if(entry){
  auto* entered=const_cast<WorldViewPaintScope*>(scope);
  if(entered->frame||entered->painting||entered->label_item||!frame.paints.empty()||width<=0||height<=0)
   throw std::runtime_error("Context renderer entry cannot substitute indexed paint or reenter");
  const auto adjusted=original->actual_adjusted_frame_owner_for_paint_without_callbacks(*this,frame);
  scope->pure();if(adjusted.get()!=&frame)throw std::runtime_error("Context entered Frame differs from SAME adjusted owner");
  const auto saved_life=scope->life;
  entered->frame=adjusted.get();entered->painting=true;
  struct EndContextPaint final {
   const std::shared_ptr<WorldViewPaintLifetime> life;WorldViewPaintScope* scope;
   ~EndContextPaint(){if(life->active_scope==scope){scope->painting=false;scope->frame=nullptr;}}
  } end{saved_life,entered};
  // Direct actual Context receiver entry. Generic quad GL setup is bypassed.
  // Draft throws after authentic CPU begin until Root's continuation exists.
  return entered->begin_current_normal_context_root_owned(*this,*entry);
 }
 if(scope->normal_button_caption){
  // Closed installed CURRENT caption child: intercept BEFORE generic paint,
  // texture quad setup, GlyphGen or standalone Label dispatch.
  const auto child=scope->normal_button_caption;const auto saved_life=scope->life;
  const auto first=[&]{
   if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
      !saved_life->active||saved_life->renderer!=this||saved_life->active_scope!=scope)
    throw std::runtime_error("Caption publication entry original Ui scope retired");
   child->caption.require_current_without_callbacks();child->common.require_current_without_callbacks();
   child->caption.require_same_common_call_without_callbacks(child->common);scope->pure();
   if(scope->normal_button_caption.get()!=child.get()||scope->normal_button_caption.owner_before(child)||
      child.owner_before(scope->normal_button_caption)||child->child_publication.get()!=original.get()||
      child->child_publication.owner_before(original)||original.owner_before(child->child_publication)||
      scope->normal_root_tree.get()!=child->original_tree.get()||scope->normal_root_tree.owner_before(child->original_tree)||
      child->original_tree.owner_before(scope->normal_root_tree)||
      scope->normal_context_root_frame.get()!=child->parent_actual_frame.get()||
      scope->normal_context_root_frame.owner_before(child->parent_actual_frame)||child->parent_actual_frame.owner_before(scope->normal_context_root_frame))
    throw std::runtime_error("Caption entry is not SAME installed child/parent physical Frame");
  };
  first();if(scope->frame||scope->painting||scope->label_item||width<=0||height<=0)
   throw std::runtime_error("Caption original child attempted nested paint");
  const auto adjusted=original->actual_adjusted_frame_owner_for_paint_without_callbacks(*this,frame);first();
  if(adjusted.get()!=&frame||child->child_frame||child->material)
   throw std::runtime_error("Caption child SAME adjusted Frame replay");
  // Own SAME adjusted frame before FULL or any entered capture allocation.
  child->child_frame=adjusted;first();
  auto* entered=const_cast<WorldViewPaintScope*>(scope);entered->frame=adjusted.get();entered->painting=true;
  struct EndCaptionPaint final {
   const std::shared_ptr<WorldViewPaintLifetime> life;WorldViewPaintScope* scope;
   ~EndCaptionPaint()noexcept{if(life->active_scope==scope){scope->painting=false;scope->frame=nullptr;}}
  } end{saved_life,entered};
  child->caption.require_source_owned(scope->operation);first();child->common.require_source_owned(scope->operation);first();
  if(!child->emission||!child->append_index||!child->append_origin||!child->projection||
     adjusted.get()!=child->append_origin.get()||adjusted.owner_before(child->append_origin)||
     child->append_origin.owner_before(adjusted)||*child->append_index>=adjusted->paints.size()||
     child->entered_frame||child->entered_run)
   throw std::runtime_error("Caption entry lacks SAME immutable append/item or attempted Frame replay");
  child->emission->require_publication_owned(*original,scope->operation);first();
  const auto index=*child->append_index;const auto& item=adjusted->paints[index];
  if(!item.clip)throw std::runtime_error("Caption actual inherited clip supplier absent");
  const auto clip=*item.clip;first();
  if(!child->fp32_rect||!child->fp32_rect_words)
   throw std::runtime_error("Caption preGL entry lacks original closed FP32 snapshot");
  const auto rect=child->fp32_rect;const auto words=*child->fp32_rect_words;
  rect->require_current_without_callbacks();rect->require_same_caption_without_callbacks(child->caption);
  rect->require_same_initialized_run_without_callbacks(child->original_run);first();
  if(rect->raw_words_without_callbacks()!=words)
   throw std::runtime_error("Caption preGL original FP32 words changed");
  const LabelPaintCapture::DestinationGeometry destination(rect,words);first();
  LabelPaintItemScope entered_item(*entered,index,item,clip,width,height,destination);first();
  const auto actual_frame=capture_label_actual_paint_call_frame_owned();first();
  if(!actual_frame)throw std::runtime_error("Caption original Label Frame issuer omitted actual facade");
  const auto frame_guard=actual_frame->capture_guard_without_callbacks();first();
  frame_guard.require_actual_paint_item_without_callbacks(*original,index);first();
  const auto indexed_rect=original->caption_fp32_rect_for_actual_item_without_callbacks(index,frame_guard);first();
  if(indexed_rect.get()!=rect.get()||indexed_rect.owner_before(rect)||rect.owner_before(indexed_rect))
   throw std::runtime_error("Caption Frame belongs to another canonical FP32 carrier");
  frame_guard.require_source_owned(scope->operation);first();
  const auto run=original->caption_run_for_actual_item_owned(index,scope->operation,*actual_frame);first();
  if(!run||run.get()!=child->original_run.get()||run.owner_before(child->original_run)||child->original_run.owner_before(run))
   throw std::runtime_error("Caption entered Run is not SAME original physical completed178");
  child->entered_frame=actual_frame;child->entered_run=run;first();
  // Consume THESE already-issued original controls with accepted embedded
  // Paint/DATA, H16 CPU Binding, Caption4->4a Item and once Atlas attachment.
  prepare_current_normal_button_caption_cpu_item_owned(child);first();
  if(!child->positioned||!child->original_cpu||!child->item)
   throw std::runtime_error("Caption actual CPU producer omitted retained original Item");
  throw std::runtime_error("Caption entered consumer awaits genuine Label540B54 tail and closed Common callback return");
 }
 if(scope->normal_common_button){
  const auto child=scope->normal_common_button;
  const auto saved_life=scope->life;
  const auto first=[&]{
   if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
      !saved_life->active||saved_life->renderer!=this||saved_life->active_scope!=scope)
    throw std::runtime_error("Common Button publication entry original Ui scope retired");
   child->common.require_current_without_callbacks();scope->pure();
   if(scope->normal_common_button.get()!=child.get()||scope->normal_common_button.owner_before(child)||
      child.owner_before(scope->normal_common_button)||child->publication.get()!=original.get()||
      child->publication.owner_before(original)||original.owner_before(child->publication))
    throw std::runtime_error("Common Button publication entry is not SAME installed CURRENT child");
  };
  first();
  if(scope->frame||scope->painting||scope->label_item||width<=0||height<=0)
   throw std::runtime_error("Common Button original child attempted nested paint");
  const auto adjusted=original->actual_adjusted_frame_owner_for_paint_without_callbacks(*this,frame);first();
  if(adjusted.get()!=&frame)throw std::runtime_error("Common Button original adjusted Frame differs");
  auto* entered=const_cast<WorldViewPaintScope*>(scope);
  entered->frame=adjusted.get();entered->painting=true;
  struct EndButtonPaint final {
   const std::shared_ptr<WorldViewPaintLifetime> life;WorldViewPaintScope* scope;
   ~EndButtonPaint(){if(life->active_scope==scope){scope->painting=false;scope->frame=nullptr;}}
  } end{saved_life,entered};
  // Exact actual entered receiver, before generic GL setup/texture quad.
  // It returns only after closed own118 CPU return has been stored in child.
  return entered->enter_current_normal_common_button_owned(*this,child,adjusted,width,height);
 }
 return paint_worldview_frame_owned(frame,width,height);
}
}

namespace civ5::ui {
namespace {
double world_scalar(std::string_view text){
 double value=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
 if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!std::isfinite(value)||std::abs(value)>32768)
  throw std::runtime_error("WorldView selected layout scalar unavailable");
 return value;
}
std::array<std::string_view,2> world_tokens(std::string_view text){
 const auto delimiter=text.find_first_of(",.");
 if(delimiter==text.npos||text.find_first_of(",.",delimiter+1)!=text.npos)
  throw std::runtime_error("WorldView selected layout pair unavailable");
 return {text.substr(0,delimiter),text.substr(delimiter+1)};
}
std::array<double,2> world_pair(std::string_view text){
 const auto tokens=world_tokens(text);return {world_scalar(tokens[0]),world_scalar(tokens[1])};
}
char world_letter(std::string_view token){
 if(token.size()!=1)throw std::runtime_error("WorldView selected anchor unavailable");
 const char c=token[0];return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;
}
double world_anchor(double origin,double parent,double size,char anchor,char side,double offset){
 if(side!='I'&&side!='O')throw std::runtime_error("WorldView selected anchor side unavailable");
 // Same coordinate convention as the existing real TechAward layout producer.
 if(anchor=='C')return origin+(parent-size)*0.5+offset;
 if(anchor=='L'||anchor=='T')return origin+(side=='O'?-size:0)+offset;
 if(anchor=='R'||anchor=='B')return origin+parent+(side=='O'?offset:-size-offset);
 throw std::runtime_error("WorldView selected anchor unavailable");
}
std::uint32_t world_color(std::string_view text){
 std::uint32_t result=0;
 for(std::size_t n=0;n<4;++n){
  const auto delimiter=text.find_first_of(",.");const auto token=text.substr(0,delimiter);
  unsigned value=0;const auto parsed=std::from_chars(token.data(),token.data()+token.size(),value);
  if(parsed.ec!=std::errc{}||parsed.ptr!=token.data()+token.size()||value>255||
     (n<3&&delimiter==text.npos)||(n==3&&delimiter!=text.npos))
   throw std::runtime_error("WorldView selected color unavailable");
  result=(result<<8)|value;
  if(delimiter!=text.npos)text.remove_prefix(delimiter+1);
 }
 return result;
}
WorldViewRect world_rect(const std::array<double,4>& rect){
 std::array<std::int32_t,4> value{};
 for(std::size_t n=0;n<value.size();++n){
  if(!std::isfinite(rect[n])||rect[n]<-1073741824.0||rect[n]>1073741824.0)
   throw std::runtime_error("WorldView projected layout overflows real paint coordinates");
  value[n]=static_cast<std::int32_t>(std::lround(rect[n]));
 }
 return {value[0],value[1],value[2],value[3]};
}
}
void UiScreenGlRenderer::shutdown_owned_worldview(
 runtime::BnwSelectedStockLuaSourceOwner& source_owner,const BnwOwnedCameraGuiScreen& actual_gui,
 const BnwWorldViewRendererProjectionLoan& projection){
 const auto life=worldview_lifetime_;
 projection.require_current_without_callbacks();
 if(projection.renderer_!=this||projection.lifetime_.get()!=life.get()||
    projection.lifetime_.owner_before(life)||life.owner_before(projection.lifetime_)){
  life->failed=true;throw std::runtime_error("WorldView shutdown projection has another canonical renderer");
 }
 if(life->active){life->failed=true;throw std::runtime_error("WorldView renderer shutdown reentered");}
 life->active=true;
 struct End{std::shared_ptr<WorldViewPaintLifetime> p;~End(){p->active_scope=nullptr;p->active=false;}} end{life};
 try {
  WorldViewGridProvider grid(projection);
  // The private source method consumes its own closed genuine WM_CLOSE
  // publication, opens SAME GUI/window/4-tree Native scope, invokes actual
  // SetShutdown closures, and completes full source/loan POST before return.
  source_owner.shutdown_worldview_renderer_context_owned(actual_gui,grid);
  // FIRST local control check precedes any borrowed renderer access. A client
  // callback may destroy this renderer; no strong self pin hides retirement.
  if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed)
   throw std::runtime_error("WorldView renderer retired during shutdown callbacks");
  projection.require_current_without_callbacks();
 }catch(...){life->failed=true;throw;}
}

UiGlPaintReport UiScreenGlRenderer::paint_owned_worldview(
 runtime::BnwSelectedStockLuaSourceOwner& source_owner,const BnwOwnedCameraGuiScreen& actual_gui,
 const BnwWorldViewRendererProjectionLoan& projection,const BnwScrollAnimationLoopTick* scroll_tick){
 const auto life=worldview_lifetime_;
 std::shared_ptr<BnwScrollAnimationLoopTick::State> scroll_state;
 if(scroll_tick){scroll_tick->state_->origin();scroll_state=scroll_tick->state_;scroll_state->current();}
 projection.require_current_without_callbacks();
 if(life->active){life->failed=true;throw std::runtime_error("WorldView actual renderer operation reentered");}
 life->active=true;
 struct End{std::shared_ptr<WorldViewPaintLifetime> p;~End(){p->active_scope=nullptr;p->active=false;}} end{life};
 UiGlPaintReport result;
 try {
 WorldViewGridProvider grid(projection);
 source_owner.with_worldview_renderer_context_owned(actual_gui,grid,[&](const auto& operation,const auto& gui,
   const auto& window,const auto& actual_trees){
  projection.require_current_without_callbacks();
  if(scroll_state)advance_worldview_scroll_update_owned(scroll_state,source_owner,operation,actual_trees);
  projection.require_current_without_callbacks();
  // NativeIssuer has returned. Scroll mutation above precedes this capture.
  // Getters requiring layout_resource_bound execute ONLY inside this genuine
  // lender callback, while active_scope is still NULL. No paint scope is faked.
  const auto resource_first=[&]{
   if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||
      life->renderer!=this||!life->active||life->active_scope)
    throw std::runtime_error("WorldView visual capture renderer retired or reentered");
  };
  resource_first();
  const auto captured_trees=actual_trees;
  // Snapshot the physical cache before Source callbacks can retire this Ui.
  auto operation_resource0=original_resource0_storage_;resource_first();
  const auto resource_serial=operation.operation_serial();resource_first();
  std::array<WorldViewPaintScope::VisualCache,5> visual_cache;
  const auto resource_pure=[&]{
   resource_first();projection.require_current_without_callbacks();resource_first();
   if(operation.operation_serial()!=resource_serial)
    throw std::runtime_error("WorldView visual capture Source serial changed");
   resource_first();
  };
  const auto resource_full=[&]{
   resource_pure();operation.require_operation(resource_serial);resource_pure();
   gui.require_current();resource_pure();window.require_current();resource_pure();
  };
  resource_full();
  // All five actual slots must already exist: the genuine Tree helper then
  // returns their SAME canonical GuiPort and cannot take its capture fallback.
  for(const auto& tree:captured_trees){
   resource_pure();if(!tree)throw std::runtime_error("Resource0 requires all five actual Context slots");
  }
  resource_full();
  const auto canonical_gui=BnwWorldViewControlTree::capture_context_gui_port_owned(operation,captured_trees);
  resource_first();resource_full();
  if(!operation_resource0){
   const auto prepared=detail::BnwOriginalNonFontDeviceInitializer::prepare_cpu_resource0_owned(
    *this,projection,operation,canonical_gui,gui,window);
   resource_first();resource_full();
   const auto initializer=prepared.initializer_;const auto cpu=prepared.image_;
   const auto gpu=initializer->upload_resource0_storage_owned(*this,operation,cpu);
   resource_first();resource_full();
   require_original_resource0_storage_current_owned(initializer,operation,cpu,gpu);
   resource_first();resource_full();
   // All operands own their physical controls before this allocation.
   auto adopted=std::make_shared<const OriginalResource0Storage>(initializer,cpu,gpu);
   resource_first();resource_full();
   if(original_resource0_storage_)
    throw std::runtime_error("Resource0 canonical storage was already initialized");
   original_resource0_storage_=adopted;operation_resource0=std::move(adopted);
  }
  resource_first();
  if(!operation_resource0->initializer||!operation_resource0->cpu||!operation_resource0->gpu)
   throw std::runtime_error("Resource0 canonical physical cache incomplete");
  require_original_resource0_storage_current_owned(operation_resource0->initializer,operation,
   operation_resource0->cpu,operation_resource0->gpu);
  resource_first();resource_full();
  // operation_resource0 stays strong through this entire selected operation.
  // Current failure poisons the existing renderer catch; it never recreates Init.
  with_notification_layout_resources_owned(projection,operation,gui,window,captured_trees,[&]{
   resource_full();
   for(std::size_t n=0;n<captured_trees.size();++n){
    resource_pure();const auto tree=captured_trees[n];
    if(!tree)throw std::runtime_error("WorldView visual capture actual tree absent");
    auto& cached=visual_cache[n];cached.tree=tree;cached.serial=resource_serial;
    cached.revision=tree->render_revision_owned();resource_pure();
    const auto same_tree=[&]{
     resource_pure();tree->require_render_revision_without_callbacks(cached.revision,operation,resource_serial);
     resource_pure();
     if(cached.tree.expired()||cached.tree.owner_before(tree)||tree.owner_before(cached.tree))
      throw std::runtime_error("WorldView visual capture canonical tree changed");
    };
    same_tree();const auto controls=tree->render_controls_owned();same_tree();
    for(const auto& control:controls){
     same_tree();const auto held_control=control;
     const auto quote=tree->inspect_control(held_control);same_tree();
     if(quote.effect_revision!=cached.revision)
      throw std::runtime_error("WorldView visual capture actual control revision changed");
     if(quote.authored.name=="Image"){
      const auto rgba=tree->image_color_rgba_owned(held_control,operation);same_tree();
      if(!cached.images.emplace(quote.id,rgba).second)
       throw std::runtime_error("WorldView visual capture Image control duplicated");
     }else if(quote.authored.name=="Button"){
      auto visual=tree->button_visual_owned(held_control,operation);same_tree();
      if(!cached.buttons.emplace(quote.id,std::move(visual)).second)
       throw std::runtime_error("WorldView visual capture Button control duplicated");
     }else if(quote.authored.name=="Stack"){
      // Actual completed initializer/member result under the genuine resource
      // borrower. Paint consumes this value; it never calls this getter later.
      auto layout=tree->stack_layout_owned(held_control,operation);same_tree();
      if(!layout.actual_size_calculation&&!layout.earned_empty_initializer)
       throw std::runtime_error("WorldView Stack result lacks actual returned initializer branch");
      const auto finite=[](const auto& values){
       for(const auto value:values)if(!std::isfinite(value))
        throw std::runtime_error("WorldView Stack returned coordinate nonfinite");
      };
      finite(layout.native_size);finite(layout.native_extent);
      finite(layout.native_offset);finite(layout.anchor_origin);finite(layout.native_bounds);
      for(const auto& child:layout.children){
       same_tree();
       if(!child.tree||(child.id==0?bool(child.control):!child.control))
        throw std::runtime_error("WorldView Stack returned member/Context identity invalid");
       finite(child.derived_offset);finite(child.native_offset);finite(child.native_size);
      }
      same_tree();
      if(!cached.stacks.emplace(quote.id,std::move(layout)).second)
       throw std::runtime_error("WorldView visual capture Stack control duplicated");
     }
     same_tree();
    }
    same_tree();resource_full();same_tree();
   }
   resource_full();
  });
  // Resource binding/target DTOR is complete. Revalidate all five revisions,
  // including earlier trees across later capture callbacks, before paint birth.
  resource_first();resource_full();
  for(std::size_t n=0;n<captured_trees.size();++n){
   resource_pure();captured_trees[n]->require_render_revision_without_callbacks(
    visual_cache[n].revision,operation,resource_serial);resource_pure();
  }
  WorldViewPaintScope scope(life,projection,operation,gui,window,captured_trees);
  scope.visual_cache=&visual_cache;
  for(std::size_t n=0;n<scope.trees.size();++n)
   if(scope.revisions[n]!=visual_cache[n].revision)
    throw std::runtime_error("WorldView immutable paint revision differs from resource capture");
  life->active_scope=&scope;
  struct EndScope{std::shared_ptr<WorldViewPaintLifetime> p;~EndScope(){p->active_scope=nullptr;}} end_scope{life};
  scope.full();
  gui.owner_.with_worldview_frame_storage_owned(gui,[&](const auto& scene,const auto& frame,
    const auto& live_ui,const auto& layout){
   scope.full();
   {
    const auto same_scene=projection.scene_.lock();
    if(!same_scene||same_scene.get()!=&scene||projection.frame_.get()!=&frame||
       scene.frame().get()!=projection.frame_.get()||
       scene.frame().owner_before(projection.frame_)||projection.frame_.owner_before(scene.frame())||
       layout.frame_owner!=live_ui.owner||layout.frame_sequence!=live_ui.sequence||
       layout.snapshot_frame!=live_ui.snapshot.frame()||!live_ui.world_view)
     throw std::runtime_error("WorldView paint has another actual GUI Scene/frame/stream");
   }
   const auto gui_width=gui.width();scope.pure();
   const auto gui_height=gui.height();scope.pure();
   if(gui_width!=projection.width_||gui_height!=projection.height_)
    throw std::runtime_error("WorldView actual GUI projection framebuffer differs");
   scope.full();
   const auto carrier=[&]{
    auto f=layout;f.paints.clear();f.hits.clear();f.readonly_tooltips.clear();f.unresolved.clear();return f;
   };
   const auto merge=[&](UiGlPaintReport r,std::size_t old_offset,bool original){
    result.rectangles+=r.rectangles;result.textures+=r.textures;result.glyphs+=r.glyphs;result.icons+=r.icons;
    result.unresolved.insert(result.unresolved.end(),r.unresolved.begin(),r.unresolved.end());
    result.readonly_text.frame=r.readonly_text.frame;
    if(original)for(auto& region:r.readonly_text.regions){
     region.source_paint_index+=old_offset;result.readonly_text.regions.push_back(std::move(region));
    }
    result.unit_name_own_font_selections.insert(result.unit_name_own_font_selections.end(),
      r.unit_name_own_font_selections.begin(),r.unit_name_own_font_selections.end());
    result.unit_name_font_refusals.insert(result.unit_name_font_refusals.end(),
      r.unit_name_font_refusals.begin(),r.unit_name_font_refusals.end());
   };
   // Load the actual named palette under this same fresh source operation.
   scope.full();const auto mounted=resources_.resolve(assets::AssetKey("Assets/UI/ColorAtlas.xml"));scope.full();
   const auto* handle=std::get_if<assets::AssetHandle>(&mounted.asset);
   if(!handle)throw std::runtime_error("WorldView selected color palette unavailable");
   const auto palette_bytes=handle->read_bytes(8ull*1024*1024);scope.full();
   const auto palette=content::read_xml_metadata(std::string(palette_bytes.begin(),palette_bytes.end()));
   if(palette.name!="ColorAtlas")throw std::runtime_error("WorldView selected palette root differs");
   std::map<std::string,std::uint32_t,std::less<>> named_colors;
   std::map<std::string,std::array<std::optional<std::uint32_t>,3>,std::less<>> color_sets;
   for(const auto& n:palette.children)if(n.name=="Color"){
    if(!named_colors.emplace(n.attribute("Name"),world_color(n.attribute("Color"))).second)
     throw std::runtime_error("WorldView selected named color ambiguous");
   }
   const auto resolve_color=[&](std::string_view token)->std::uint32_t {
    const auto named=named_colors.find(token);if(named!=named_colors.end())return named->second;
    const auto delimiter=token.find_first_of(",.");
    if(delimiter!=token.npos&&token.find_first_of(",.",delimiter+1)==token.npos){
     const auto base=named_colors.find(token.substr(0,delimiter));
     if(base==named_colors.end())throw std::runtime_error("WorldView selected alpha color base unavailable");
     const auto alpha=world_scalar(token.substr(delimiter+1));
     if(alpha<0||alpha>255||std::floor(alpha)!=alpha)throw std::runtime_error("WorldView named alpha unavailable");
     return (base->second&0xffffff00)|static_cast<std::uint32_t>(alpha);
    }
    return world_color(token);
   };
   for(const auto& n:palette.children)if(n.name=="ColorSet"){
    std::array<std::optional<std::uint32_t>,3> c{};
    for(std::size_t k=0;k<c.size();++k){const auto v=n.attribute("Color"+std::to_string(k));if(!v.empty())c[k]=resolve_color(v);}
    if(!color_sets.emplace(n.attribute("Name"),c).second)throw std::runtime_error("WorldView selected ColorSet ambiguous");
   }
   struct LayoutNode {
    std::array<double,4> rect{};bool hidden=false,positioned=true,unsupported=false;
    std::string font,font_style,color_set;double authored_alpha=1.0;
    // Immutable resource-phase value for THIS canonical Stack only.
    const BnwWorldViewControlTree::StackLayoutOwned* stack=nullptr;
   };
   std::map<const BnwWorldViewControlTree*,LayoutNode> stack_context_roots;
   using NodePaint=WorldViewPaintScope::NormalCommonOriginalInput;
   std::array<std::vector<NodePaint>,5> commands;
   std::size_t projected_anchors=0;
   const auto animation_sample=std::chrono::steady_clock::now();
   std::array<std::map<std::uint64_t,LayoutNode>,5> layouts;
   // Value inherited from the genuine ActionInfoPanel root's actual direct
   // LuaContext descriptor. Effects metadata supplier retains only direct
   // LuaContext declarations; this is no fabricated canonical Controls node.
   std::optional<LayoutNode> notification_context_parent;
   for(const auto tree_index:std::array<std::size_t,5>{3,0,1,2,4}){
    const auto& tree=actual_trees[tree_index];
    if(tree_index!=3){const auto source=tree->actual_source();scope.pure();operation.require_same_source(*source);scope.pure();}
    const auto context=tree->context();scope.pure();
    const auto root=tree->render_context_owned();scope.pure();
    const auto controls=tree->render_controls_owned();scope.pure();
    // All quotes are copied before layout or any actual GL write.
    std::vector<BnwWorldViewControlTree::ControlQuote> quotes;quotes.reserve(controls.size());
    // Canonical Image float4 values, aligned with the copied actual controls.
    // This does not reparse Color or issue state for AlphaAnim image paints.
    std::vector<std::optional<UiFloatColor>> image_color_quotes;image_color_quotes.reserve(controls.size());
    std::vector<std::optional<BnwWorldViewControlTree::ButtonVisualOwned>> button_visual_quotes;
    button_visual_quotes.reserve(controls.size());
    std::map<std::uint64_t,std::string> grid_data;
    for(const auto& control:controls){
     auto q=tree->inspect_control(control);scope.pure();
     if(q.effect_revision!=scope.revisions[tree_index])throw std::runtime_error("WorldView actual effect changed during layout");
     std::optional<UiFloatColor> image_color;
     if(q.authored.name=="Image"){
      const auto& cached=scope.visual_for_tree_owned(tree,q.effect_revision);
      const auto found=cached.images.find(q.id);
      if(found==cached.images.end())throw std::runtime_error("WorldView Image resource-phase color absent");
      image_color=found->second;scope.pure();
      for(const auto channel:*image_color)if(!std::isfinite(channel)||channel<0||channel>1)
       throw std::runtime_error("WorldView actual canonical Image color invalid");
     }
     image_color_quotes.push_back(image_color);
     std::optional<BnwWorldViewControlTree::ButtonVisualOwned> button_visual;
     if(q.authored.name=="Button"){
      const auto& cached=scope.visual_for_tree_owned(tree,q.effect_revision);
      const auto found=cached.buttons.find(q.id);
      if(found==cached.buttons.end())throw std::runtime_error("WorldView Button resource-phase visual absent");
      button_visual=found->second;scope.pure();
      if(!button_visual->earned_constructor_null_178)
       throw std::runtime_error("Button visual Label178 outcome is not earned constructor NULL");
      for(const auto channel:button_visual->rgba_12c_138)if(!std::isfinite(channel)||channel<0||channel>1)
       throw std::runtime_error("Button actual embedded RGBA invalid");
      for(const auto size:button_visual->destination_size_13c_140)if(!std::isfinite(size)||size<0)
       throw std::runtime_error("Button actual embedded destination size invalid");
      for(const auto uv:button_visual->uv_168_16c)if(!std::isfinite(uv))
       throw std::runtime_error("Button actual embedded UV span invalid");
      for(const auto uv:button_visual->uv_170_174)if(!std::isfinite(uv))
       throw std::runtime_error("Button actual embedded UV origin invalid");
     }
     button_visual_quotes.push_back(std::move(button_visual));
     if(q.authored.name=="GridData"&&!q.authored.attribute("Style").empty())
      if(!grid_data.emplace(q.parent,q.authored.attribute("Style")).second)
       throw std::runtime_error("WorldView actual grid style metadata ambiguous");
     quotes.push_back(std::move(q));
    }
    auto& parents=layouts[tree_index];
    std::set<std::uint64_t> root_ancestors;
    if(tree_index==3){
     const auto target=std::find_if(quotes.begin(),quotes.end(),[](const auto& q){return q.authored.attribute("ID")=="SelectedUnitContainer";});
     if(target==quotes.end())throw std::runtime_error("WorldView actual InGame SelectedUnitContainer layout absent");
     auto id=target->id;
     while(id){
      if(!root_ancestors.insert(id).second)throw std::runtime_error("WorldView actual InGame parent layout cycle");
      const auto q=std::find_if(quotes.begin(),quotes.end(),[&](const auto& q){return q.id==id;});
      if(q==quotes.end())throw std::runtime_error("WorldView actual InGame ancestor absent");id=q->parent;
     }
    }
    LayoutNode root_layout;
    root_layout.rect={0,0,static_cast<double>(frame.stamp.viewport.client_width),
       static_cast<double>(frame.stamp.viewport.client_height)};
    root_layout.font=root.attribute("Font");root_layout.font_style=root.attribute("FontStyle");
    root_layout.color_set=root.attribute("ColorSet");
    if(tree_index==2){
     if(context!="ActionInfoPanel")throw std::runtime_error("Notification actual ActionInfoPanel context differs");
     const content::XmlNode* declaration=nullptr;
     for(const auto& child:root.children)if(child.name=="LuaContext"&&
        child.attribute("ID")=="NotificationPanel"){
      if(declaration)throw std::runtime_error("Notification actual child declaration ambiguous");
      declaration=&child;
     }
     if(!declaration||declaration->attribute("FileName")!="Assets/UI/InGame/WorldView/NotificationPanel")
      throw std::runtime_error("Notification actual direct child source declaration absent");
     // The selected Original descriptor has no spatial/visibility/style
     // override. Refuse another profile rather than invent child geometry.
     for(const auto* field:std::array<const char*,10>{"Size","Offset","Anchor","AnchorSide",
       "Hidden","BranchAlpha","Font","FontStyle","ColorSet","Parent"})
      if(!declaration->attribute(field).empty())
       throw std::runtime_error("Notification child context descriptor profile differs");
     notification_context_parent=root_layout;scope.pure();
    }
    if(tree_index==4){
     if(context!="NotificationPanel"||!notification_context_parent)
      throw std::runtime_error("Notification canonical fifth source lacks actual parent layout");
     root_layout=*notification_context_parent;
     for(const auto& field:std::array<std::pair<const char*,std::string*>,3>{
       {{"Font",&root_layout.font},{"FontStyle",&root_layout.font_style},{"ColorSet",&root_layout.color_set}}}){
      const auto value=root.attribute(field.first);if(!value.empty())*field.second=value;
     }
     scope.pure();
    }
    if(const auto found=stack_context_roots.find(tree.get());found!=stack_context_roots.end()){
     const auto fonts=root_layout;root_layout=found->second;
     root_layout.font=fonts.font;root_layout.font_style=fonts.font_style;root_layout.color_set=fonts.color_set;
    }
    for(std::size_t n=0;n<quotes.size();++n){
     const auto& q=quotes[n];const auto& xml=q.authored;
     if(tree_index==3&&!root_ancestors.count(q.id))continue;
     LayoutNode parent=root_layout;
     if(q.external_parent_id){
      if(q.parent)throw std::runtime_error("WorldView actual control has two parents");
      const auto external=q.external_parent_tree.lock();
      if(!external||external.get()!=actual_trees[3].get()||
         external.owner_before(actual_trees[3])||actual_trees[3].owner_before(external))
       throw std::runtime_error("WorldView actual external layout has another InGame tree");
      const auto found=layouts[3].find(q.external_parent_id);
      if(found==layouts[3].end())throw std::runtime_error("WorldView actual external parent layout absent");
      parent=found->second;
     }
     if(q.parent){const auto found=parents.find(q.parent);
      if(found==parents.end())throw std::runtime_error("WorldView real attachment preorder lost parent");
      parent=found->second;
     }
     LayoutNode node=parent;node.hidden=parent.hidden||q.hidden;node.stack=nullptr;
     const BnwWorldViewControlTree::StackLayoutOwned* actual_stack=nullptr;
     if(xml.name=="Stack"){
      const auto& cached=scope.visual_for_tree_owned(tree,q.effect_revision);
      const auto found=cached.stacks.find(q.id);
      if(found==cached.stacks.end())throw std::runtime_error("WorldView actual Stack resource result absent");
      actual_stack=&found->second;scope.pure();
     }
     const BnwWorldViewControlTree::StackLayoutOwned::Child* stack_child=nullptr;
     if(parent.stack){
      // Preserve qualified attachment identity and genuine helper ordering;
      // a missing row is never a derived zero-position fallback.
      for(const auto& child:parent.stack->children){
       if(child.tree.get()==tree.get()&&!child.tree.owner_before(tree)&&!tree.owner_before(child.tree)&&child.id==q.id){
        if(stack_child||!child.control)throw std::runtime_error("WorldView Stack child result ambiguous or not a Control");
        stack_child=&child;
       }
      }
      if(!stack_child)throw std::runtime_error("WorldView Stack derived child placement absent");
      scope.pure();
     }
     for(const auto& field:std::array<std::pair<const char*,std::string*>,3>{
       {{"Font",&node.font},{"FontStyle",&node.font_style},{"ColorSet",&node.color_set}}}){
      const auto v=xml.attribute(field.first);if(!v.empty())*field.second=v;
     }
     if(!xml.attribute("BranchAlpha").empty())node.authored_alpha*=world_scalar(xml.attribute("BranchAlpha"));
     if(!std::isfinite(node.authored_alpha)||node.authored_alpha<0)throw std::runtime_error("WorldView authored inherited alpha invalid");
     if(xml.name=="ShowOnMouseOver"||xml.name=="GridData"||xml.name=="ScrollPanelData"||
        xml.name=="StackData"){node.unsupported=true;}
     if(xml.name=="WorldAnchor"){
      node.rect={0,0,0,0};node.positioned=false;
      if(!node.hidden&&q.world_position){
       const auto screen=projection.project(*q.world_position);scope.pure();
       if(screen){
        node.rect={(*screen)[0]*frame.stamp.viewport.client_width/projection.width_,
          (*screen)[1]*frame.stamp.viewport.client_height/projection.height_,0,0};
        node.positioned=true;++projected_anchors;
       }
      }else if(!node.hidden&&!q.world_position)
       result.unresolved.push_back(context+" WorldAnchor "+std::to_string(q.id)+" has no genuine stored world position");
     }else if(node.positioned&&!node.unsupported&&(actual_stack||stack_child)){
      // Selected OWN retained-field convention from the frozen coordinate
      // contract. Native28/2C already contains authored anchoring; never run
      // world_anchor() on these returned values or add margins again.
      std::array<double,2> translation{parent.rect[0],parent.rect[1]};
      std::array<float,2> offset=actual_stack?actual_stack->native_offset:stack_child->native_offset;
      std::array<float,2> size=actual_stack?actual_stack->native_size:stack_child->native_size;
      if(stack_child){
       if(actual_stack&&actual_stack->native_offset!=stack_child->native_offset)
        throw std::runtime_error("WorldView inner Stack current offset disagrees with parent result");
       for(std::size_t axis=0;axis<2;++axis){
        // Parent global base already includes its28/2C. Add only90/94 minus
        // that offset, then derived displacement, then this child's28/2C.
        translation[axis]+=static_cast<double>(parent.stack->anchor_origin[axis])-
         static_cast<double>(parent.stack->native_offset[axis])+stack_child->derived_offset[axis];
       }
      }
      for(std::size_t axis=0;axis<2;++axis){
       if(!std::isfinite(translation[axis])||!std::isfinite(offset[axis])||!std::isfinite(size[axis]))
        throw std::runtime_error("WorldView current Stack primitive coordinate nonfinite");
      }
      node.rect={translation[0]+offset[0],translation[1]+offset[1],size[0],size[1]};
      node.stack=actual_stack;
      scope.pure();
     }else if(node.positioned&&!node.unsupported){
      std::array<double,2> size{parent.rect[2],parent.rect[3]};
      const auto declared_size=xml.attribute("Size");
      if(!declared_size.empty()){
       const auto tokens=world_tokens(declared_size);
       for(std::size_t k=0;k<2;++k)size[k]=tokens[k]=="Full"?parent.rect[k+2]:world_scalar(tokens[k]);
      }
      std::array<double,2> offset{};
      if(q.offset)offset=*q.offset;
      else if(!xml.attribute("Offset").empty())offset=world_pair(xml.attribute("Offset"));
      auto anchors=std::array<std::string_view,2>{"L","T"};
      const auto declared_anchor=xml.attribute("Anchor");
      if(!declared_anchor.empty())anchors=world_tokens(declared_anchor);
      auto sides=std::array<std::string_view,2>{"I","I"};
      const auto declared_side=xml.attribute("AnchorSide");
      if(!declared_side.empty())sides=world_tokens(declared_side);
      // Autosized labels use actual retained GlyphGen/style geometry. An absent
      // exact font refuses this label; no generic font width is substituted.
      if(xml.name=="Label"&&declared_size.empty()&&!node.hidden){
       const auto text=q.text.value_or(xml.attribute("String"));
       const auto* style=resolve_worldview_font_style(node.font,node.font_style);scope.pure();
       const GlyphGenGeometry* geometry=style?style->layout_geometry:nullptr;
       if(!geometry){
        if(node.font=="TwCenMT16")geometry=font16_.geometry;
        else if(node.font=="TwCenMT20")geometry=font20_.geometry;
        else if(node.font=="TwCenMT24")geometry=font24_.geometry;
       }
       if(!geometry){node.unsupported=true;result.unresolved.push_back(context+" exact authored label font unavailable: "+node.font);}
       else {
        std::int32_t measured=0;
        if(text.find('[')!=std::string::npos){
         if(!icons_.geometry||!icons_.texture)throw std::runtime_error("WorldView actual label markup icon owner unavailable");
         const auto line=project_glyphgen_markup(*geometry,*icons_.geometry,text,worldview_icon_resolver());
         scope.full();measured=line.advance_width;
        }else measured=project_glyphgen_line(*geometry,text).advance_width;
        size={static_cast<double>(measured),static_cast<double>(geometry->line_height)};
       }
      }
      node.rect={world_anchor(parent.rect[0],parent.rect[2],size[0],world_letter(anchors[0]),world_letter(sides[0]),offset[0]),
        world_anchor(parent.rect[1],parent.rect[3],size[1],world_letter(anchors[1]),world_letter(sides[1]),offset[1]),size[0],size[1]};
     }
     if(node.stack&&node.positioned&&!node.unsupported){
      for(const auto& child:node.stack->children)if(child.id==0){
       if(child.control)throw std::runtime_error("WorldView attached Context invented a Control");
       const auto member=std::find_if(actual_trees.begin(),actual_trees.end(),[&](const auto& actual){
        return actual.get()==child.tree.get()&&!actual.owner_before(child.tree)&&!child.tree.owner_before(actual);
       });
       if(member==actual_trees.end())throw std::runtime_error("WorldView attached Stack Context has no actual paint-tree scope");
       LayoutNode placed=node;placed.stack=nullptr;
       for(std::size_t axis=0;axis<2;++axis){
        placed.rect[axis]=node.rect[axis]-node.stack->native_offset[axis]+node.stack->anchor_origin[axis]+
         child.derived_offset[axis]+child.native_offset[axis];
        placed.rect[axis+2]=child.native_size[axis];
        if(!std::isfinite(placed.rect[axis])||!std::isfinite(placed.rect[axis+2]))
         throw std::runtime_error("WorldView attached Stack Context rect nonfinite");
       }
       if(!stack_context_roots.emplace(child.tree.get(),std::move(placed)).second)
        throw std::runtime_error("WorldView attached Stack Context placement duplicated");
       scope.pure();
      }
     }
     if(!parents.emplace(q.id,node).second)throw std::runtime_error("WorldView canonical control repeats in render preorder");
     if(tree_index==3||node.hidden||!node.positioned||node.unsupported)continue;
     bool paused=xml.name=="AlphaAnim"&&(xml.attribute("Stopped")=="1"||xml.attribute("Stopped")=="true");
     double sampled_alpha=1.0;
     if(xml.name=="AlphaAnim"){
      const auto start_text=xml.attribute("AlphaStart"),end_text=xml.attribute("AlphaEnd");
      if(start_text.empty()||end_text.empty())throw std::runtime_error("WorldView actual animation endpoints absent");
      const double start=world_scalar(start_text),finish=world_scalar(end_text);
      sampled_alpha=start;
      if(q.animation&&q.animation->play_at&&q.animation->play_sequence>q.animation->reset_sequence){
       const auto speed_text=xml.attribute("Speed");if(speed_text.empty())throw std::runtime_error("WorldView animation speed absent");
       const double speed=world_scalar(speed_text);
       const double elapsed=std::chrono::duration<double>(animation_sample-*q.animation->play_at).count();
       if(speed<=0||elapsed<0)throw std::runtime_error("WorldView actual animation clock/speed invalid");
       const auto cycle=xml.attribute("Cycle");
       const double phase=elapsed*speed;double progress=0;
       if(cycle=="Bounce"){const double loop=std::fmod(phase,2.0);progress=loop<=1.0?loop:2.0-loop;}
       else if(cycle=="Once")progress=std::clamp(phase,0.0,1.0);
       else if(cycle=="Loop")progress=std::fmod(phase,1.0);
       else throw std::runtime_error("WorldView actual animation cycle unsupported");
       sampled_alpha=start+(finish-start)*progress;paused=true;
      }else if(q.animation&&q.animation->reset_at)paused=true;
      // This is an own renderer sample from genuine commands/time. Native
      // Original interpolation/stop timing equivalence is not asserted.
      if(!std::isfinite(sampled_alpha)||sampled_alpha<0)throw std::runtime_error("WorldView actual own animation sample invalid");
      node.authored_alpha*=sampled_alpha;parents[q.id].authored_alpha=node.authored_alpha;
     }
     const bool image=xml.name=="Image"||(paused&&(!xml.attribute("Texture").empty()||q.texture));
     const bool grid=xml.name=="Grid"||xml.name=="GridButton";
     const bool text=xml.name=="Label";
     if((xml.name=="AlphaAnim"&&!paused)||xml.name=="ScrollAnim"){
      parents[q.id].unsupported=true;
      result.unresolved.push_back(context+" "+xml.name+" lacks an actual current animation/bar sample");continue;
     }
     const bool bar=xml.name=="Bar";
     const bool button=xml.name=="Button";
     if(!image&&!grid&&!text&&!bar&&!button)continue;
     // Canonical normal dispatch is supplied by the real Tree/Control, before
     // text construction. XML/paint kind never issues standalone Label role.
     scope.pure();
     // Label invocation/emission is issued lazily by the real CURRENT Common call.
     scope.pure();
     WorldViewPaint paint;
     paint.destination=world_rect({node.rect[0]*projection.width_/frame.stamp.viewport.client_width,
       node.rect[1]*projection.height_/frame.stamp.viewport.client_height,
       node.rect[2]*projection.width_/frame.stamp.viewport.client_width,
       node.rect[3]*projection.height_/frame.stamp.viewport.client_height});
     if(button){
      if(n>=button_visual_quotes.size()||!button_visual_quotes[n])
       throw std::runtime_error("Actual Button lacks canonical embedded visual quote");
      const auto& visual=*button_visual_quotes[n];
      if(!visual.earned_constructor_null_178)
       throw std::runtime_error("Actual Button Label178 producer is unsupported");
      // Getter separately proves constructor Texture NULL; it is not missing data.
      if(!visual.texture_extent_15c)continue;
      if(!visual.texture_keeper)throw std::runtime_error("Actual Button raw texture keeper absent");
      detail::NotificationLayoutTextureKeeper::require_canonical(visual.texture_keeper);scope.pure();
      paint.tint=visual.rgba_12c_138;
      paint.destination.width=world_rect({0,0,visual.destination_size_13c_140[0]*projection.width_/frame.stamp.viewport.client_width,
        visual.destination_size_13c_140[1]*projection.height_/frame.stamp.viewport.client_height}).width;
      paint.destination.height=world_rect({0,0,visual.destination_size_13c_140[0]*projection.width_/frame.stamp.viewport.client_width,
        visual.destination_size_13c_140[1]*projection.height_/frame.stamp.viewport.client_height}).height;
     }else if(xml.name=="Image"){
      if(n>=image_color_quotes.size()||!image_color_quotes[n])
       throw std::runtime_error("WorldView actual Image lacks canonical color field quote");
      // RGBA already includes the native Color alpha suffix. Lua SetColor
      // updates this same producer field; generic q.color never replaces it.
      paint.tint=*image_color_quotes[n];
     }else paint.rgba=xml.attribute("Color").empty()?0xffffffff:resolve_color(xml.attribute("Color"));
     if(q.color&&!bar&&xml.name!="Image"&&!button){
      UiFloatColor tint{};for(std::size_t k=0;k<tint.size();++k){
       if(!std::isfinite((*q.color)[k])||(*q.color)[k]<0||(*q.color)[k]>1)
        throw std::runtime_error("WorldView actual normalized color invalid");
       tint[k]=static_cast<float>((*q.color)[k]);
      }paint.tint=tint;
     }
     if(node.authored_alpha!=1.0){
      if(!paint.tint)paint.tint=color(paint.rgba);
      (*paint.tint)[3]*=static_cast<float>(node.authored_alpha);
     }
     if(button){
      const auto& visual=*button_visual_quotes[n];
      paint.kind=WorldViewPaintKind::texture;paint.source_key=visual.texture_keeper->key_;
      if(paint.source_key.empty())throw std::runtime_error("Button canonical raw resource key empty");
      // UVs come from embedded5C7B50 fields at the actual draw, not XML cropping.
     }else if(image){
      paint.kind=WorldViewPaintKind::texture;paint.source_key=q.texture.value_or(xml.attribute("Texture"));
      if(paint.source_key.empty())throw std::runtime_error("WorldView actual Image texture is absent");
      std::array<double,2> crop{};if(q.texture_offset)crop=*q.texture_offset;else if(!xml.attribute("TextureOffset").empty())crop=world_pair(xml.attribute("TextureOffset"));
      paint.source=world_rect({crop[0],crop[1],node.rect[2],node.rect[3]});
     }else if(bar){
      if(!q.percent)throw std::runtime_error("WorldView actual Bar fill percentage absent");
      if(!std::isfinite(*q.percent)||*q.percent<0||*q.percent>1)throw std::runtime_error("WorldView actual Bar fill invalid");
      auto f=carrier();paint.kind=WorldViewPaintKind::rectangle;
      const auto bg=xml.attribute("BGColor"),fg=xml.attribute("FGColor");
      if(bg.empty()||fg.empty())throw std::runtime_error("WorldView actual Bar foreground/background source absent");
      paint.tint=color(resolve_color(bg));(*paint.tint)[3]*=static_cast<float>(node.authored_alpha);f.paints.push_back(paint);
      paint.tint=q.fg_color?std::optional<UiFloatColor>{UiFloatColor{
        static_cast<float>((*q.fg_color)[0]),static_cast<float>((*q.fg_color)[1]),
        static_cast<float>((*q.fg_color)[2]),static_cast<float>((*q.fg_color)[3])}}:
        std::optional<UiFloatColor>{color(resolve_color(fg))};
      (*paint.tint)[3]*=static_cast<float>(node.authored_alpha);
      if(xml.attribute("Direction")=="Up"){
       const auto h=static_cast<std::int32_t>(std::lround(paint.destination.height**q.percent));
       paint.destination.y+=paint.destination.height-h;paint.destination.height=h;
      }else if(xml.attribute("Direction")=="Right"){
       paint.destination.width=static_cast<std::int32_t>(std::lround(paint.destination.width**q.percent));
      }else throw std::runtime_error("WorldView selected Bar fill direction unsupported");
      f.paints.push_back(paint);commands[tree_index].push_back({controls[n],std::move(f),{},q.id});continue;
     }else if(grid){
      paint.kind=WorldViewPaintKind::rectangle;paint.style=xml.attribute("Style");
      if(paint.style.empty()&&grid_data.count(q.id))paint.style=grid_data.at(q.id);
      if(paint.style.empty())throw std::runtime_error("WorldView actual Grid style is absent");
     }else {
      paint.kind=WorldViewPaintKind::text;paint.text=q.text.value_or(xml.attribute("String"));
      if(paint.text.empty())continue;
      paint.style=node.font;
      UiFontStylePaint styled;styled.font=node.font;styled.style=node.font_style;
      if(!node.color_set.empty()){
       const auto found=color_sets.find(node.color_set);
       if(found==color_sets.end())throw std::runtime_error("WorldView actual inherited ColorSet absent");
       styled.colors=found->second;
      }
      for(std::size_t k=0;k<styled.colors.size();++k){
       const auto color=xml.attribute("Color"+std::to_string(k));if(!color.empty())styled.colors[k]=resolve_color(color);
      }
      if(!styled.colors[0])throw std::runtime_error("WorldView actual Label foreground absent");
      paint.rgba=*styled.colors[0];
      if(node.authored_alpha!=1.0){
       if(!styled.colors[1])throw std::runtime_error("WorldView actual styled background layer color absent");
       std::array<UiFloatColor,2> c{color(*styled.colors[0]),color(*styled.colors[1])};
       for(auto& v:c)v[3]*=static_cast<float>(node.authored_alpha);styled.float_colors=c;
      }
      if(!node.font_style.empty())paint.font_style=styled;
     }
     auto f=carrier();f.paints.push_back(std::move(paint));
     std::shared_ptr<const detail::BnwNormalLabelPaintEmission> normal_label_emission;
     commands[tree_index].push_back({controls[n],std::move(f),std::move(normal_label_emission),q.id});
     scope.pure();
    }
    scope.pure();
   }
   // These ranges are emitted by actual world_view.cpp append sites and remapped
   // by the same actual compositor. They supply ordering, never source authority.
   const auto& source_spans=live_ui.world_view->owned_paint_spans;
   const auto& spans=layout.worldview_owned_paint_spans;
   if(source_spans.size()!=2||spans.size()!=source_spans.size())
    throw std::runtime_error("WorldView actual compositor source span profile absent");
   if(spans[0].begin<source_spans[0].begin)throw std::runtime_error("WorldView actual append offset underflows");
   const auto world_offset=spans[0].begin-source_spans[0].begin;
   if(world_offset>layout.paints.size()||live_ui.world_view->paints.size()>layout.paints.size()-world_offset)
    throw std::runtime_error("WorldView actual appended world stream exceeds GUI layout");
   for(std::size_t n=0;n<spans.size();++n){
    if(spans[n].context_id!=source_spans[n].context_id||
       source_spans[n].begin>live_ui.world_view->paints.size()||source_spans[n].end>live_ui.world_view->paints.size()||
       spans[n].begin!=world_offset+source_spans[n].begin||spans[n].end!=world_offset+source_spans[n].end||
       spans[n].begin>spans[n].end||spans[n].end>layout.paints.size()||
       (n&&spans[n].begin<spans[n-1].end))
     throw std::runtime_error("WorldView actual source span/order changed");
   }
   if(spans[0].context_id!="ActionInfoPanel"||spans[1].context_id!="UnitFlagManager")
    throw std::runtime_error("WorldView actual selected source spans differ");
   if(spans[1].begin!=spans[1].end&&!projected_anchors)
    throw std::runtime_error("WorldView has semantic UnitFlag paints without any genuine projected factory instance");
   result.unresolved.insert(result.unresolved.end(),layout.unresolved.begin(),layout.unresolved.end());
   const auto paint_original=[&](std::size_t begin,std::size_t end){
    if(begin==end)return;
    auto f=carrier();f.paints.assign(layout.paints.begin()+begin,layout.paints.begin()+end);
    for(const auto& t:layout.readonly_tooltips)if(t.source_paint_index&&*t.source_paint_index>=begin&&*t.source_paint_index<end){
     auto local=t;local.source_paint_index=*t.source_paint_index-begin;f.readonly_tooltips.push_back(std::move(local));
    }
    scope.full();merge(paint_worldview_frame_owned(f,projection.width_,projection.height_),begin,true);scope.full();
   };
   const auto paint_tree=[&](std::size_t index){
    using Inputs=std::vector<WorldViewPaintScope::NormalCommonOriginalInput>;
    using ContextBody=std::function<UiGlPaintReport(const detail::BnwNormalRootContextEntry&)>;
    const auto context_tree=actual_trees[index];const auto saved_life=life;
    auto*const renderer=this;auto*const entered_scope=&scope;
    const auto first=[&]{
     if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
        !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=entered_scope)
      throw std::runtime_error("Outer Common original Ui scope retired");
     entered_scope->pure();
    };
    first();scope.full();first();
    // Unissued compositor append storage: no eager Control Publication, Label
    // dispatch, emission binding, or masked receiver read before child18.
    const auto inputs=std::make_shared<const Inputs>(std::move(commands[index]));first();scope.full();first();
    const auto context_frame=std::make_shared<const ComposedUiFrame>(carrier());first();scope.full();first();
    const auto body=std::make_shared<const ContextBody>(
     [saved_life,renderer,entered_scope,context_tree,context_frame,inputs](const detail::BnwNormalRootContextEntry& supplied)->UiGlPaintReport{
      const auto entry=supplied;
      const auto first=[&]{
       if(!saved_life||saved_life->creator!=std::this_thread::get_id()||!saved_life->alive||saved_life->failed||
          !saved_life->active||saved_life->renderer!=renderer||saved_life->active_scope!=entered_scope)
        throw std::runtime_error("Outer Common callback original Ui scope retired");
       entered_scope->pure();
      };
      first();entered_scope->full();first();
      if(!context_frame||!context_frame->paints.empty()||entered_scope->frame||entered_scope->painting||
         entered_scope->publication||entered_scope->normal_root_execution||entered_scope->normal_root_tree)
       throw std::runtime_error("Outer Common callback is not genuine original Context entry");
      auto consumers=entered_scope->capture_normal_common_child_consumers_owned(*renderer,inputs);first();entered_scope->full();first();
      const auto count=entered_scope->original_publications.size();
      if(count==entered_scope->original_publications.max_size())throw std::runtime_error("Context original custody exhausted");
      entered_scope->original_publications.reserve(count+1);first();entered_scope->full();first();
      const BnwWorldViewControlRenderPublication::OriginalOwnerIssuerKey key;
      const auto publication=BnwWorldViewControlRenderPublication::capture_context_original_owned(
       key,*renderer,std::weak_ptr<const void>(saved_life),context_tree,entry,
       entered_scope->gui,entered_scope->window,*context_frame);
      first();entered_scope->full();first();
      const auto canonical=publication->original_owner_without_callbacks();first();
      if(canonical.get()!=publication.get()||canonical.owner_before(publication)||publication.owner_before(canonical)||
         entered_scope->original_publications.size()!=count)
       throw std::runtime_error("Context original Publication custody changed");
      entered_scope->original_publications.push_back(publication);entered_scope->normal_common_consumers=std::move(consumers);first();
      entered_scope->publication=publication.get();
      struct EndPublication final {
       const std::shared_ptr<WorldViewPaintLifetime> life;WorldViewPaintScope* scope;
       ~EndPublication(){if(life->active_scope==scope)scope->publication=nullptr;}
      } end{saved_life,entered_scope};
      // Whole root/children/drain remain in SAME owned Root16 callback extent.
      // Draft fails at genuine unissued edges; no empty successful report.
      return publication->paint(*renderer);
     });
    first();scope.full();first();
    auto report=context_tree->with_normal_root_context_entry_owned(context_tree,operation,body);
    first();scope.full();first();
    (void)report;
    throw std::runtime_error("Common Context draft has no earned mixed drain/retirement return");
   };
   std::size_t cursor=0;
   // WorldView root currently declares child contexts; canonical own controls,
   // if a future selected root contains any, precede its owned child spans.
   paint_tree(0);
   for(const auto& span:spans){
    paint_original(cursor,span.begin);
    paint_tree(span.context_id=="ActionInfoPanel"?2:1);
    // Genuine nested LuaContext belongs after the actual ActionInfoPanel
    // controls, inside the same source span and actual entered render scope.
    if(span.context_id=="ActionInfoPanel"){scope.full();paint_tree(4);scope.full();}
    cursor=span.end;
   }
   paint_original(cursor,layout.paints.size());scope.full();
  });
  scope.full();
 });
 projection.require_current_without_callbacks();
 return result;
 }catch(...){note_active_worldview_exception("paint_owned_worldview");life->failed=true;throw;}
}
}

namespace civ5::ui {
std::shared_ptr<const BnwClientFrame> UiScreenGlRenderer::capture_worldview_projection_frame(
 const std::shared_ptr<BnwClientSceneFrame>& scene)const {
 const auto life=worldview_lifetime_;
 if(life->creator!=std::this_thread::get_id()||!life->alive||life->failed||life->renderer!=this||
    !scene||scene->gui_thread_!=std::this_thread::get_id())
  throw std::runtime_error("WorldView actual camera capture retired or foreign creator");
 scene->require_publication_finality(scene->frame_);
 return scene->frame_;
}
}
