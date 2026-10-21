#pragma once
#include "civ5/ui/bnw_civilopedia_label_metrics.hpp"
#include "civ5/content/rules_database.hpp"
namespace civ5::ui {
// A caller binds this getter to its explicitly selected retained locale owner.
// No lookup/format/resolver is replaced here. An absent getter is a dependency.
struct BnwLabelLocaleSelection {
 std::string language,owner_token;
 std::uint64_t revision=0;
 bool operator==(const BnwLabelLocaleSelection&)const=default;
};
using BnwLabelLocaleSelectionProvider=std::function<std::optional<BnwLabelLocaleSelection>()>;
enum class BnwClientTextPolicy {selected_language,asset_glyphgen,native_system_font};
enum class BnwClientTextBackend {asset_glyphgen,native_system_font};
struct BnwLanguageFontRow {
 std::string language,font_name;
 bool use_extended_font=false;
 float width_scale=0,height_scale=0;
 bool operator==(const BnwLanguageFontRow&)const=default;
};
struct BnwClientTextPolicyFrame {
 runtime::BnwStockLuaClientExtent client;
 BnwLabelLocaleSelection locale;
 BnwLanguageFontRow source_row;
 BnwClientTextPolicy policy=BnwClientTextPolicy::selected_language;
 BnwClientTextBackend backend=BnwClientTextBackend::asset_glyphgen;
 // Capability is the actual implemented bounded measurement producer, not
 // font rasterization, initialized Labels or original rendering parity.
 bool authored_ascii_measurement_available=false;
 bool native_system_font_available=false;
 bool native_label_initialization_complete=false;
 bool follows_selected_language_font_branch=false;
 std::string pending_producer;
};
// Data reader reused by genuine retained owners. No policy/lifetime authority.
BnwLanguageFontRow read_bnw_language_font_row(const content::RulesDatabase&,std::string_view);
class BnwWorldViewTitleFontIssuer;
class BnwWorldViewTitleFontDescriptor;
class BnwClientTextPolicyOwner {
 public:
 // Keep database/locale/client providers alive for the retained owner epoch.
 static std::shared_ptr<BnwClientTextPolicyOwner> create(const content::RulesDatabase&,
  runtime::BnwStockLuaClientExtentProvider,BnwLabelLocaleSelectionProvider,
  BnwClientTextPolicy,std::shared_ptr<const BnwCivilopediaLabelMetricsOwner>);
 // Own C++ WorldView candidate policy; never native Label Force flags.
 static std::shared_ptr<BnwClientTextPolicyOwner> create_from_worldview_title(
     std::shared_ptr<const BnwWorldViewTitleFontIssuer>);
 const BnwWorldViewTitleFontDescriptor&declared_worldview_descriptor(std::size_t)const;
 const assets::ResourceMountTable&worldview_resources()const;
 std::uintptr_t worldview_hwnd()const;
 const BnwClientTextPolicyFrame&snapshot()const noexcept{return frame_;}
 const BnwClientTextPolicyFrame&current()const;
 const BnwNativePlainLabelMetric&declared_metric(std::size_t control_index)const;
 BnwClientTextBackend backend_for_label(std::size_t control_index)const;
 const BnwNativePlainLabelMetric&authored_ascii_metric(std::size_t control_index)const;
 private:
 BnwClientTextPolicyOwner(const content::RulesDatabase&d,runtime::BnwStockLuaClientExtentProvider c,
 BnwLabelLocaleSelectionProvider l,std::shared_ptr<const BnwCivilopediaLabelMetricsOwner> m):database_(d),client_(std::move(c)),locale_(std::move(l)),metrics_(std::move(m)){}
 const content::RulesDatabase&database_;
 runtime::BnwStockLuaClientExtentProvider client_;
 BnwLabelLocaleSelectionProvider locale_;
 std::shared_ptr<const BnwCivilopediaLabelMetricsOwner>metrics_;
 std::shared_ptr<const BnwWorldViewTitleFontIssuer>worldview_;
 std::weak_ptr<BnwClientTextPolicyOwner>worldview_facade_;
 BnwClientTextPolicyFrame frame_;
};
}
