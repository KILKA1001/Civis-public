#pragma once
#include "civ5/graphics/gl_context.hpp"
#include "civ5/ui/bnw_system_font_cache_owner.hpp"
#include "civ5/ui/detail/bnw_label_paint_text_loan.hpp"
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;}
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui::detail {class BnwLabelSystemFontBorrower;class BnwLabelPaintTextLoan;class BnwLabelPaintPreparedFlushLoan;class BnwLabelPaintPositionedData;class BnwNormalItemOccurrence;}
namespace civ5::graphics {
namespace detail {class BnwRendererContextPin;class BnwLabelSystemFontAtlasMetadata;class BnwOwnedFontPaintEffectFence;}
struct GlBnwSystemFontAtlasFrame {
 GLuint green_texture=0,shadow_texture=0;
 std::int32_t width=0,height=0;
 std::uint64_t revision=0;
 bool pair_storage_available=false,shader_draw_available=false,native_label_initialized=false;
};
struct GlBnwSystemFontAtlasSources {
 std::shared_ptr<GlContext>context;
 std::shared_ptr<const ui::BnwSystemFontCacheOwner>cache;
 std::uintptr_t window=0;
 GlBnwSystemFontAtlasFrame pair;
};
struct GlBnwSystemFontAtlasIdentitySources {
 const GlContext*context=nullptr;
 std::shared_ptr<const ui::BnwSystemFontCacheOwner>cache;
 std::uintptr_t window=0;
 GlBnwSystemFontAtlasFrame pair;
};
class GlBnwSystemFontAtlas {
 public:
 static std::shared_ptr<GlBnwSystemFontAtlas>create(std::shared_ptr<GlContext>,std::shared_ptr<ui::BnwSystemFontCacheOwner>);
 ~GlBnwSystemFontAtlas();
 GlBnwSystemFontAtlasFrame current()const;
 // Identity/lifetime only; no GL/current/reset/source callbacks. Mutable pair
 // and uploaded journal can only be inspected on the creating thread.
 std::shared_ptr<const ui::BnwSystemFontCacheOwner>source_cache_owner()const noexcept;
 GlBnwSystemFontAtlasSources sources_no_current()const;
 bool has_uploaded_quote_no_current(const ui::BnwSystemFontSlotQuote&)const;
 // Pair is published only after BOTH real uploads and final SAME client check.
 ui::BnwSystemFontSlotQuote upload(std::uint32_t bmp_codepoint,const ui::BnwSystemFontSlotQuote&);
 assets::Bytes read_uploaded(const ui::BnwSystemFontSlotQuote&,bool shadow)const;
 void bind_uploaded(const ui::BnwSystemFontSlotQuote&,GLuint green_unit,GLuint shadow_unit)const;
 private:
 // Controlled alias names the existing closed Guard; no issuer is added.
 using PaintGuard=ui::detail::BnwLabelPaintTextLoan::Guard;
 friend class detail::BnwOwnedFontPaintEffectFence;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class detail::BnwLabelSystemFontAtlasMetadata;
 friend class GlBnwFontShader;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class ::civ5::ui::detail::BnwNormalItemOccurrence;
 class HistoricalPlanesIssuerKey final {
  friend class ::civ5::ui::BnwWorldViewControlTree;
  HistoricalPlanesIssuerKey()=default;
  HistoricalPlanesIssuerKey(const HistoricalPlanesIssuerKey&)=delete;
 };
 // Closed physical binding DATA only. No sampler/material/target/draw issuer.
 class AcceptedFontPlanes final {
  friend class GlBnwSystemFontAtlas;
  friend class GlBnwFontShader;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class ::civ5::ui::detail::BnwNormalItemOccurrence;
  struct SelectedStages final {std::array<GLuint,2> textures;std::uint32_t native_format;};
  // This immutable physical capsule retains the ORIGINAL Pair allocation.
  // It grants no old ACTIVE DATA/Paint/Early, Root, Material or draw authority.
  class HistoricalGuard final {
   friend class GlBnwSystemFontAtlas;
   friend class ::civ5::ui::BnwWorldViewControlTree;
   friend class ::civ5::ui::detail::BnwNormalItemOccurrence;
   friend class GlBnwFontShader;
   struct Impl;
   const std::shared_ptr<const Impl> impl_;
   class ConstructionKey final {
    friend class GlBnwSystemFontAtlas;
    ConstructionKey()=default;
    ConstructionKey(const ConstructionKey&)=delete;
   };
   explicit HistoricalGuard(std::shared_ptr<const Impl>,const ConstructionKey&);
   void require_physical_current_without_callbacks()const;
   void require_actual_atlas_without_callbacks(const GlBnwSystemFontAtlas&)const;
   void require_actual_positioned_data_without_callbacks(
       const ::civ5::ui::detail::BnwLabelPaintPositionedData::Guard&)const;
   void require_same_accepted_planes_without_callbacks(const AcceptedFontPlanes&)const;
   SelectedStages selected_stages_without_callbacks(unsigned pass)const;
  public:
   HistoricalGuard(const HistoricalGuard&)=default;
   HistoricalGuard&operator=(const HistoricalGuard&)=delete;
   ~HistoricalGuard();
  };
  struct Storage;
  const std::shared_ptr<const Storage> storage_;
  explicit AcceptedFontPlanes(std::shared_ptr<const Storage>);
  void require_current_without_callbacks()const;
  void require_actual_atlas_without_callbacks(const GlBnwSystemFontAtlas&)const;
  GlBnwSystemFontAtlasFrame accepted_pair_without_callbacks()const;
  SelectedStages selected_stages_without_callbacks(unsigned pass)const;
 public:
  ~AcceptedFontPlanes();
  AcceptedFontPlanes(const AcceptedFontPlanes&)=delete;
  AcceptedFontPlanes&operator=(const AcceptedFontPlanes&)=delete;
 };
 static std::unique_ptr<AcceptedFontPlanes>capture_accepted_font_planes_owned(
     const ::civ5::ui::detail::BnwLabelPaintPositionedData&);
 static AcceptedFontPlanes::HistoricalGuard capture_historical_font_planes_owned(
     const HistoricalPlanesIssuerKey&,const runtime::BnwWorldViewSourceOperationLoan&,
     const ::civ5::ui::detail::BnwLabelPaintPositionedData&,
     const std::shared_ptr<GlBnwSystemFontAtlas>& actual_atlas);
 static std::shared_ptr<GlBnwSystemFontAtlas>create_from_owned_renderer_context(
     std::shared_ptr<const detail::BnwRendererContextPin>,std::shared_ptr<ui::BnwSystemFontCacheOwner>);
 GlBnwSystemFontAtlasIdentitySources sources_identity_no_current()const;
 // Exact same counted token, including its original shared control.
 void require_same_owned_context_pin_no_current(
  const std::shared_ptr<const detail::BnwRendererContextPin>&)const;
 // Midphase only: genuine caller first performs its actual 59104F prefix.
 std::unique_ptr<ui::detail::BnwLabelPaintPreparedFlushLoan>prepare_and_flush_native_owned(
     const runtime::BnwWorldViewSourceOperationLoan&,
     const ui::detail::BnwLabelSystemFontBorrower&,const PaintGuard&);
 friend class ui::detail::BnwLabelPaintPreparedFlushLoan;
 friend class ui::detail::BnwLabelPaintTextLoan;
 std::array<std::uint32_t,9>glyph_bits_native_owned(
     const runtime::BnwWorldViewSourceOperationLoan&,
     const ui::detail::BnwLabelSystemFontBorrower&,const PaintGuard&,std::uint32_t);
 struct OwnedGlyphState;
 struct OwnedFlushState;
 struct Pair;
 static std::shared_ptr<Pair>allocate(GlContext&,int,int,std::shared_ptr<GlContext>,
                                    std::shared_ptr<const detail::BnwRendererContextPin>);
 // Live producer PRE/POST only; a drain snapshot is not queue-wide completion.
 // Logical Pin admission is separate from Node-only physical cleanup.
 static void require_owned_pair_retirement_snapshot_complete(
     std::shared_ptr<const detail::BnwRendererContextPin>);
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit GlBnwSystemFontAtlas(std::unique_ptr<Impl>);
};
}
