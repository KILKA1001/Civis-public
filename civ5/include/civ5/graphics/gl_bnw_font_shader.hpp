#pragma once
#include "civ5/graphics/gl_bnw_system_font_atlas.hpp"
#include "civ5/graphics/detail/bnw_label_default_font_target_loan.hpp"
#include "civ5/graphics/detail/bnw_normal_label_draw_record_binding.hpp"
#include "civ5/ui/detail/bnw_normal_root_batch_history.hpp"
#include "civ5/ui/detail/bnw_normal_label_material_loan.hpp"
#include "civ5/ui/native_label_color_pack.hpp"
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
#include <array>
#include <functional>
#include <string_view>
#include <vector>
#include <span>
#include <optional>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui::detail {class BnwLabelPaintPositionedData;}
namespace civ5::assets {class CivisResourceBankOwner;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::graphics {
struct GlRenderApi;
namespace detail {class BnwRendererContextPin;}
// Diagnostic translation of original family0x2B/selector4 only. The caller
// supplies prepared vertices. This is not a Label/layout/color-pack producer.
inline constexpr std::string_view bnw_font_shader_original_exe_sha256 =
 "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686";
inline constexpr std::string_view bnw_font_shader_original_vs_sha256 =
 "66deeaff51d75620df139c344c86388336ed98091605a33bad3b54e3c67601c9";
inline constexpr std::string_view bnw_font_shader_original_ps_sha256 =
 "8f36bc8721e801f37c45cfab81703509ecb659b56db487e7e6f1734b1ac8278c";
// These modes declare different OFFSCREEN storage coordinate domains.
// Mode0: GL row0 is logical bottom. Mode1: GL row0 is logical native TOP.
// Mode1 is not an accepted overlay into an existing GL world/window target:
// composition needs an owned UI offscreen/flip route, still pending.
enum class BnwFontPixelCenterControl { unshifted_gl, d3d9_integer_center_diagnostic };
enum class BnwFontTargetRowOrigin { logical_bottom_at_gl_row0, logical_top_at_gl_row0 };
constexpr BnwFontTargetRowOrigin bnw_font_target_row_origin(BnwFontPixelCenterControl mode)noexcept{
 return mode==BnwFontPixelCenterControl::d3d9_integer_center_diagnostic?
  BnwFontTargetRowOrigin::logical_top_at_gl_row0:BnwFontTargetRowOrigin::logical_bottom_at_gl_row0;
}
struct BnwFontShaderVertex {
 float x=0,y=0,u=0,v=0;
 std::array<float,4> diagnostic_rgba{};
};
struct BnwFontShaderQuad {
 ui::BnwSystemFontSlotQuote uploaded_slot;
 // Already positioned source-cache quads, including run cursor/layer offset.
 // Do not apply the shadow offset a second time in this consumer.
 std::array<BnwFontShaderVertex,4> vertices;
};
struct BnwFontShaderLayer {
 unsigned slot=0; // 2=shadow plane, 1/0=green plane; submit decreasing order.
 std::vector<BnwFontShaderQuad> quads;
};
struct BnwFontShaderControls {
 std::array<std::array<float,4>,4> source_matrix_rows{};
 std::array<float,2> translation{},depth_pair{};
 float parent_alpha=1;
 std::array<float,4> clip_xmin_xmax_ymin_ymax{};
 bool allow_color=true;
};
struct BnwFontShaderBatch {
 GlBnwSystemFontAtlasFrame retained_pair;
 BnwFontShaderControls controls;
 std::vector<BnwFontShaderLayer> layers;
 std::function<bool()> current;
 // Explicit fixture declaration: no source-backed packed-color producer yet.
 bool caller_declared_diagnostic_colors=false;
};
// Source format9 numeric/declaration fixture. Supplied colors and geometry do
// not attest current native Label/parser/parent ownership. SAME atlas quotes
// and current fences remain mandatory. All request bytes are pinned before callbacks.
struct BnwFontShaderPackedQuad {
 ui::BnwSystemFontSlotQuote uploaded_slot;
 std::array<ui::NativeLabelPackedVertex,4> vertices;
};
struct BnwFontShaderPackedLayer {
 unsigned slot=0;
 std::vector<BnwFontShaderPackedQuad> quads;
};
struct BnwFontShaderPackedBatch {
 GlBnwSystemFontAtlasFrame retained_pair;
 BnwFontShaderControls controls;
 std::vector<BnwFontShaderPackedLayer> layers;
 std::function<bool()> current;
 bool caller_declared_diagnostic_colors=false;
};
// Private engines share this validation view. Only scalar checks use the
// normalized float view; the actual packed GPU stream stays unchanged bytes.
enum class BnwFontVertexInputControl {diagnostic_float32,source_rgba_ubyte4n_stride24};
struct BnwFontShaderTarget {
 // Bound real non-default FBO with ONE level0 GL_TEXTURE_2D RGBA8 output.
 // Storage row domain is declared by pixel_centers; it is not window/overlay
 // readiness. Decode using the accepted result.actual_target_row_origin.
 GLuint framebuffer=0,color_texture=0;
 std::int32_t width=0,height=0;
 runtime::BnwStockLuaClientExtent retained_client;
 BnwFontPixelCenterControl pixel_centers=BnwFontPixelCenterControl::unshifted_gl;
 std::function<bool()> current;
};
struct BnwFontShaderDrawResult {
 GlContextNativeIdentity actual_context;
 GlBnwSystemFontAtlasFrame accepted_pair;
 GLuint actual_target_framebuffer=0,actual_target_color_texture=0;
 std::int32_t actual_target_width=0,actual_target_height=0;
 std::uint64_t accepted_pair_revision=0;
 std::size_t draw_calls=0,quads=0;
 bool source_shader_available=false;
 bool native_label_initialized=false,native_pixels_compared=false;
 bool diagnostic_colors=true,unused_stage0_differs_from_native=true;
 BnwFontPixelCenterControl pixel_centers=BnwFontPixelCenterControl::unshifted_gl;
 // Actual accepted mode determines offscreen row interpretation, not native
 // Label initialization or permission to composite into an existing world.
 BnwFontTargetRowOrigin actual_target_row_origin=BnwFontTargetRowOrigin::logical_bottom_at_gl_row0;
 BnwFontVertexInputControl actual_vertex_input=BnwFontVertexInputControl::diagnostic_float32;
};
class GlBnwFontShader : public std::enable_shared_from_this<GlBnwFontShader> {
 public:
 static std::shared_ptr<GlBnwFontShader> create(std::shared_ptr<GlContext>,
  std::shared_ptr<GlBnwSystemFontAtlas>);
 ~GlBnwFontShader();
 // Copies request BEFORE callbacks; rejects foreign current WGL context rather
 // than changing it. Retains self through callbacks. Caught same-owner nested
 // draw permanently fails this consumer. Post-effect failure throws; no
 // accepted result is minted.
 BnwFontShaderDrawResult draw(const BnwFontShaderBatch&,const BnwFontShaderTarget&);
 BnwFontShaderDrawResult draw_packed(const BnwFontShaderPackedBatch&,const BnwFontShaderTarget&);
 private:
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class detail::BnwNormalLabelDrawRecordBinding;
 struct DrawRecordState;
 // One exact selected queued-record transport lease; no public issuer or ACK.
 class GpuStagingLease final {
  friend class GlBnwFontShader;
  struct Life;
  struct Impl;
  // Physical custody only; original facade destruction still retires Life.
  std::shared_ptr<Impl> impl_;
  explicit GpuStagingLease(std::unique_ptr<Impl>);
 public:
  ~GpuStagingLease();
  GpuStagingLease(const GpuStagingLease&)=delete;
  GpuStagingLease&operator=(const GpuStagingLease&)=delete;
  GpuStagingLease(GpuStagingLease&&)=delete;
  GpuStagingLease&operator=(GpuStagingLease&&)=delete;
 };
 // Pure location DATA for already linked physical CODE; no values/material.
 struct OriginalProgramUniformLocations final {
  GLint rows,translation,depth,parent_alpha,block_texture,allow_color;
  struct ClipFields final {GLint clip,fragment_bias,native_top_at_row_zero,target_height;};
  std::optional<ClipFields> clip_fields; // Absent for native selector1 PS.
 };
 // Immutable physical code custody, independent of all creating DATA leases.
 // Neither the library nor its cache key grants a native material or draw.
 class OriginalProgramLibrary final {
  friend class GlBnwFontShader;
  friend class ::civ5::ui::UiScreenGlRenderer;
  struct Storage;
  const std::shared_ptr<Storage> storage_;
  explicit OriginalProgramLibrary(std::shared_ptr<Storage>);
  bool same_key_without_callbacks(const std::shared_ptr<const assets::CivisResourceBankOwner>&,
   const std::shared_ptr<const detail::BnwRendererContextPin>&)const noexcept;
 public:
  ~OriginalProgramLibrary();
  OriginalProgramLibrary(const OriginalProgramLibrary&)=delete;
  OriginalProgramLibrary&operator=(const OriginalProgramLibrary&)=delete;
 };
 class OriginalProgramUse final {
  friend class GlBnwFontShader;
  friend class ::civ5::ui::UiScreenGlRenderer;
  struct Storage;
  const std::shared_ptr<Storage> storage_;
  explicit OriginalProgramUse(std::shared_ptr<Storage>);
  OriginalProgramUniformLocations uniform_locations_without_callbacks(unsigned native_selector)const;
  void require_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 public:
  ~OriginalProgramUse();
  OriginalProgramUse(const OriginalProgramUse&)=delete;
  OriginalProgramUse&operator=(const OriginalProgramUse&)=delete;
 };
 static std::shared_ptr<OriginalProgramLibrary> create_original_program_library_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,ui::detail::BnwLabelPaintPositionedData::Guard,
  std::shared_ptr<const assets::CivisResourceBankOwner>,std::shared_ptr<GlBnwSystemFontAtlas>,
  std::shared_ptr<const detail::BnwRendererContextPin>,GlRenderApi);
 static std::unique_ptr<OriginalProgramUse> capture_original_program_use_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,ui::detail::BnwLabelPaintPositionedData::Guard,
  std::shared_ptr<OriginalProgramLibrary>,std::shared_ptr<const assets::CivisResourceBankOwner>,
  std::shared_ptr<GlBnwSystemFontAtlas>,std::shared_ptr<const detail::BnwRendererContextPin>);
 // Exact immutable selected Original program bytes and their qualified GLSL
 // translation only. It does not issue GUI material values, target or draw.
 class OriginalProgramSource final {
  friend class GlBnwFontShader;
  friend class ::civ5::ui::UiScreenGlRenderer;
  struct Storage;
  const std::shared_ptr<Storage> storage_;
  explicit OriginalProgramSource(std::shared_ptr<Storage>);
  class Guard final {
   friend class OriginalProgramSource;
   friend class GlBnwFontShader;
   const std::shared_ptr<Storage> storage_;
   explicit Guard(std::shared_ptr<Storage>);
   void require_current_without_callbacks()const;
   void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
   // Physical validated pair lookup only. This argument does not prove an
   // actual GUI operation chose that selector; material issuer remains separate.
   struct NativeProgramPair final {
    std::span<const std::uint32_t> vertex_tokens,fragment_tokens;
    std::string_view translated_vertex,translated_fragment;
    unsigned native_selector;
   };
   NativeProgramPair native_selector_without_callbacks(unsigned)const;
   std::span<const std::uint32_t> original_vertex_tokens_without_callbacks()const;
   std::span<const std::uint32_t> original_fragment_tokens_without_callbacks()const;
   std::string_view translated_vertex_glsl_without_callbacks()const;
   std::string_view translated_fragment_glsl_without_callbacks()const;
  };
  Guard capture_guard_without_callbacks()const;
 public:
  ~OriginalProgramSource();
  OriginalProgramSource(const OriginalProgramSource&)=delete;
  OriginalProgramSource&operator=(const OriginalProgramSource&)=delete;
 };
 // Parent obtains SAME bank via operation.selected_provider_startup_bank_owned().
 // Arbitrary paths, caller token/GLSL strings and declaration booleans are absent.
 static std::unique_ptr<OriginalProgramSource> original_program_source_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,
  ::civ5::ui::detail::BnwLabelPaintPositionedData::Guard,
  std::shared_ptr<const ::civ5::assets::CivisResourceBankOwner>);
 // Closed CPU vertex DATA only. No GPU/material/target or pass execution issuer.
 class PositionedVertices final {
  friend class GlBnwFontShader;
  friend class ::civ5::ui::UiScreenGlRenderer;
  using RawVertex=std::array<std::uint32_t,6>;
  struct Storage;
  const std::shared_ptr<const Storage> storage_;
  explicit PositionedVertices(std::shared_ptr<const Storage>);
  void require_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
  std::span<const RawVertex> vertices_without_callbacks(unsigned pass)const;
  std::span<const std::uint32_t> indices_without_callbacks(unsigned pass)const;
  std::uint32_t captured_pass0_vertex_count_without_callbacks()const;
 public:
  ~PositionedVertices();
  PositionedVertices(const PositionedVertices&)=delete;
  PositionedVertices&operator=(const PositionedVertices&)=delete;
 };
 static std::unique_ptr<PositionedVertices> positioned_vertices_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,
  const ::civ5::ui::detail::BnwLabelPaintPositionedData&);
 // Genuine original token/translation pair consumed into physical GL names.
 // The selector chooses CODE only; no native GUI invocation or material values
 // are asserted. Root wraps this factory with genuine operation FULL checks.
 static std::shared_ptr<GlBnwFontShader> create_from_original_program_owned(
  std::shared_ptr<const detail::BnwRendererContextPin>,std::shared_ptr<GlBnwSystemFontAtlas>,
  OriginalProgramSource::Guard,unsigned native_selector,GlRenderApi loaded_render);
 // Native sampler registry record9 only; no caller policy/index or draw grant.
 static void initialize_native_font_sampler_policy_owned(std::shared_ptr<GlBnwFontShader>);
 // Physical shader storage only. No preparedRun/Glyph issuer exists here.
 // Public diagnostic draw remains closed for this owned branch.
 static std::shared_ptr<GlBnwFontShader>create_from_owned_renderer_context(
  std::shared_ptr<const detail::BnwRendererContextPin>,std::shared_ptr<GlBnwSystemFontAtlas>);
 // Shader-authored transport VALUE only; Ui restoration and Root completion
 // are separate closed effects. No public ctor/outcome setter/zero entry.
 class QueuedRecordTransportResult final {
  friend class GlBnwFontShader;
  struct Impl;
  const std::shared_ptr<const Impl> impl_;
  explicit QueuedRecordTransportResult(std::shared_ptr<const Impl>)noexcept;
 public:
  QueuedRecordTransportResult(const QueuedRecordTransportResult&)=default;
  QueuedRecordTransportResult&operator=(const QueuedRecordTransportResult&)=delete;
  ~QueuedRecordTransportResult();
 };
 static void require_draw_record_data_without_callbacks(std::shared_ptr<DrawRecordState>,
  const ::civ5::ui::detail::BnwLabelPaintPositionedData&);
 // Physical per-item CPU record producer only. Root separately registers
 // SAME actual occurrence/DATA; GPU drain requires its sealed permission.
 // Actual selected-entry consumer: Root must issue genuine complete/draining
 // permission with retained original Pair health before any production call.
 static std::unique_ptr<GpuStagingLease> stage_queued_record_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const ::civ5::ui::UiScreenGlRenderer&,
  const ::civ5::ui::detail::BnwNormalRootBatchHistory::Guard&,
  const ::civ5::ui::detail::BnwNormalItemOccurrence::Guard&,
  const ::civ5::ui::detail::BnwNormalQueueDrainPermission::Guard&,
  detail::BnwNormalLabelDrawRecordBinding::Guard,
  const detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity&,
  const detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt&);
 template<class PhysicalCuts,class SourceCuts>
 static void upload_prepared_record_transport_unpublished_owned(
  GpuStagingLease::Impl&,const PhysicalCuts&,const SourceCuts&);
 // Exactly the currently selected nonempty queued pass, not all item passes.
 // Genuine current manager Material and Rows restoration are closed inputs.
 static QueuedRecordTransportResult submit_queued_record_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const ::civ5::ui::UiScreenGlRenderer&,
  GpuStagingLease&,detail::BnwLabelDefaultFontTargetLoan::Guard,
  const ::civ5::ui::detail::BnwNormalLabelMaterialLoan::ShaderAccess&);
 static ::civ5::ui::detail::BnwNormalQueueRecordCompletion complete_queued_record_after_rows_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const ::civ5::ui::UiScreenGlRenderer&,
  GpuStagingLease&,const QueuedRecordTransportResult&,
  const detail::BnwLabelDefaultFontTargetLoan::RowsRestorationWitness&);
 static std::unique_ptr<detail::BnwNormalLabelDrawRecordBinding> prepare_normal_label_draw_records_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const OriginalProgramUse&,const PositionedVertices&,
  std::unique_ptr<GlBnwSystemFontAtlas::AcceptedFontPlanes>);
 static std::unique_ptr<detail::BnwNormalLabelDrawRecordBinding> normal_label_draw_record_binding_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const OriginalProgramUse&,const PositionedVertices&,
  std::unique_ptr<GlBnwSystemFontAtlas::AcceptedFontPlanes>);
 static void submit_normal_label_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const ::civ5::ui::UiScreenGlRenderer&,
  const ::civ5::ui::BnwWorldViewControlRenderPublication&,std::size_t,
  const ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t,
  detail::BnwNormalLabelDrawRecordBinding::Guard,
  detail::BnwLabelDefaultFontTargetLoan::Guard,
  const ::civ5::ui::detail::BnwWindowedRendererMatrixDataLoan::ShaderAccess&,
  const ::civ5::ui::detail::BnwNormalLabelMaterialLoan::ShaderAccess&);
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit GlBnwFontShader(std::unique_ptr<Impl>);
};
std::string_view bnw_font_shader_vertex_glsl();
std::string_view bnw_font_shader_fragment_glsl();
}
