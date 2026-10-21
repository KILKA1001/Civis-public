#pragma once
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/bnw_client_text_policy.hpp"
#include "civ5/ui/glyphgen_style.hpp"
#include "civ5/ui/civis_resource_bank_font_color_snapshot.hpp"
namespace civ5::ui {
class SelectedBnwPreviewHost;
class BnwTechAwardContextOwner;class BnwEngineUiTurnOwner;
class BnwWorldViewFontSourceQuote {
 private:
 friend class BnwWorldViewFontSourceScope;
 friend class BnwWorldViewTitleFontIssuer;
 friend class BnwTechAwardContextOwner;
 struct Data;std::shared_ptr<const Data>data_;
 explicit BnwWorldViewFontSourceQuote(std::shared_ptr<const Data>d):data_(std::move(d)){}
};
// Own C++ source scope only: move the actual camera UI table here and use
// resources() for the rest of that run. No copy/alias of EARLY resources.
// The camera must retire BEFORE teardown or a locale replacement. Retention
// by a font owner keeps payload lifetime safe but does not authorize renewal.
class BnwWorldViewFontSourceScope final {
 public:
 static std::shared_ptr<BnwWorldViewFontSourceScope> create(
     assets::ResourceMountTable&&, std::string selected_ui_language);
 // Genuine own-bank source tuple; no EARLY/completed proof conversion.
 static std::shared_ptr<BnwWorldViewFontSourceScope> create_from_civis_resources(
     const std::shared_ptr<assets::CivisResourceBankOwner>&,
     const std::shared_ptr<const CivisResourceBankUiSnapshot>&,
     const std::shared_ptr<const CivisResourceBankFontColorSnapshot>&,
     std::string selected_ui_language);
 ~BnwWorldViewFontSourceScope();
 BnwWorldViewFontSourceScope(const BnwWorldViewFontSourceScope&)=delete;
 BnwWorldViewFontSourceScope&operator=(const BnwWorldViewFontSourceScope&)=delete;
 std::shared_ptr<BnwWorldViewFontSourceScope> retain_owner()const; // Pure canonical metadata before callbacks.
 void retire() noexcept;
 void require_current()const;
 // Required own-bank Host join BEFORE resources or any UiFrame publication.
 void require_same_civis_resource_host(const SelectedBnwPreviewHost&)const;
 // Append to the actual existing frame augmentation AFTER title text is
 // extracted by the SAME selected camera locale/localize_info path. This
 // quote attests own camera selection, never native Lua/Context locale.
 UiFrameSnapshot augment_snapshot(UiFrameSnapshot)const;
 const assets::ResourceMountTable&resources()const;
 const BnwLabelLocaleSelection&locale()const;
 private:
 friend class BnwWorldViewTitleFontIssuer;
 friend class BnwTechAwardContextOwner;
 friend class BnwEngineUiTurnOwner;
 friend class BnwPlotYieldRefreshOwnedUiContext;
 // Data-only selected language source from the actual endpoint provider.
 // This does not renew a retired scene or issue Label/render readiness.
 static std::shared_ptr<BnwWorldViewFontSourceScope> from_actual_tech_context_snapshot(const UiFrameSnapshot&);
 void require_owned_ui_finality()const;
 void require_selected_locale_without_callbacks(const BnwLabelLocaleSelection&)const;
 std::shared_ptr<BnwWorldViewFontSourceScope> retain_owned_ui_owner(const std::shared_ptr<BnwWorldViewFontSourceScope>&)const;
 struct Impl; std::shared_ptr<Impl>impl_;
 explicit BnwWorldViewFontSourceScope(std::shared_ptr<Impl>);
};
// Descriptor, not a native current Label metric. The adjusted ascent and
// unadjusted line sum are parsed from the SAME selected GG source. No native
// width, GetSizeVal, ForceIME/ForceNonIME, Context or initialized bit is issued.
class BnwWorldViewTitleFontDescriptor {
 public:
 const std::string&font_key()const noexcept{return font_key_;}
 const std::string&font_sha256()const noexcept{return font_sha256_;}
 std::int32_t adjusted_ascent()const noexcept{return adjusted_ascent_;}
 std::int32_t source_line_height()const noexcept{return source_line_height_;}
 std::size_t candidate_index()const noexcept{return candidate_index_;}
 private:
 friend class BnwWorldViewTitleFontIssuer;
 std::string font_key_,font_sha256_;
 std::int32_t adjusted_ascent_=0,source_line_height_=0;
 std::size_t candidate_index_=0;
};
struct BnwWorldViewTitleFontFrame {
 BnwClientFrameStamp publication;
 runtime::BnwStockLuaClientExtent client;
 BnwLabelLocaleSelection locale;
 BnwLanguageFontRow language_row;
 WorldViewPaint title;
 // Source scope has not invoked stock Lua/Label initialization/metrics.
 static constexpr bool native_label_ready=false,native_context_ready=false,
     native_force_flags_known=false,native_get_size_val=false;
};
class BnwWorldViewTitleFontIssuer final {
 public:
 // No callback/bool supplies authority. Host is genuine private EARLY adoption;
 // frame is the actual immutable publication of this exact host's FrameOwner.
 static std::shared_ptr<const BnwWorldViewTitleFontIssuer> create_from_adopted_host(
     std::shared_ptr<const SelectedBnwPreviewHost>,const BnwClientFrameOwner&,
     std::shared_ptr<const BnwClientFrame>,std::shared_ptr<BnwWorldViewFontSourceScope>);
 static std::shared_ptr<const BnwWorldViewTitleFontIssuer> create_from_civis_resources(
     std::shared_ptr<const SelectedBnwPreviewHost>,const BnwClientFrameOwner&,
     std::shared_ptr<const BnwClientFrame>,std::shared_ptr<BnwWorldViewFontSourceScope>);
 ~BnwWorldViewTitleFontIssuer();
 BnwWorldViewTitleFontIssuer(const BnwWorldViewTitleFontIssuer&)=delete;
 BnwWorldViewTitleFontIssuer&operator=(const BnwWorldViewTitleFontIssuer&)=delete;
 const BnwWorldViewTitleFontFrame&current()const;
 const BnwWorldViewTitleFontDescriptor&descriptor(std::size_t index)const;
 void require_descriptor(const BnwWorldViewTitleFontDescriptor&)const;
 const assets::ResourceMountTable&resources()const;
 const content::RulesDatabase&database()const;
 std::uintptr_t actual_hwnd()const;
 private:
 struct Impl; std::shared_ptr<Impl>impl_;
 static std::shared_ptr<const BnwWorldViewTitleFontIssuer> create_from_source_host(
     std::shared_ptr<const SelectedBnwPreviewHost>,const BnwClientFrameOwner&,
     std::shared_ptr<const BnwClientFrame>,std::shared_ptr<BnwWorldViewFontSourceScope>,
     std::shared_ptr<const assets::CivisResourceBankOwner>);
 explicit BnwWorldViewTitleFontIssuer(std::shared_ptr<Impl>);
};
}
