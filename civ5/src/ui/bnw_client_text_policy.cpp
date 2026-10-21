#include "civ5/ui/bnw_client_text_policy.hpp"
#include <cmath>
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
namespace civ5::ui {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
bool same(const runtime::BnwStockLuaClientExtent&a,const runtime::BnwStockLuaClientExtent&b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
}
BnwLanguageFontRow read_bnw_language_font_row(const content::RulesDatabase&db,std::string_view language){
 need(!language.empty()&&language.find('\0')==language.npos,"Explicit language unavailable");BnwLanguageFontRow result;std::size_t count=0;
 // Exact selected attached Localization DB. SQL defaults and selected DLC/mod
 // updates have already been applied by saved_rules::apply_saved_localization.
 const auto report=db.query_rows({"SELECT Type,UseExtendedFont,FontName,FontWidthScale,FontHeightScale FROM Localization.Languages WHERE Type = ? LIMIT 1",{std::string(language)}},[&](const content::RulesRowView&row){
  ++count;for(const auto key:{"Type","UseExtendedFont","FontName","FontWidthScale","FontHeightScale"})need(row.has_column(key),"Languages font query schema incomplete");
  result.language=row.text("Type").value_or("");result.use_extended_font=row.integer("UseExtendedFont")!=0;result.font_name=row.text("FontName").value_or("");result.width_scale=static_cast<float>(row.real("FontWidthScale"));result.height_scale=static_cast<float>(row.real("FontHeightScale"));
 },1);
 need(report.all_succeeded()&&count==1&&result.language==language,"Selected Languages font row missing/query failed");
 need(!result.font_name.empty()&&std::isfinite(result.width_scale)&&std::isfinite(result.height_scale)&&result.width_scale>0&&result.height_scale>0,"Selected Languages font data unsupported");return result;
}
std::shared_ptr<BnwClientTextPolicyOwner>BnwClientTextPolicyOwner::create(const content::RulesDatabase&db,
 runtime::BnwStockLuaClientExtentProvider client,BnwLabelLocaleSelectionProvider locale,
 BnwClientTextPolicy policy,std::shared_ptr<const BnwCivilopediaLabelMetricsOwner>metrics){
 need(static_cast<bool>(client.current)&&static_cast<bool>(locale)&&static_cast<bool>(metrics),"Explicit text policy client/locale/metrics producer absent");
 const auto c=client.current();const auto l=locale();need(c&&l&&!c->surface_owner.empty()&&c->revision!=0&&c->width>0&&c->height>0&&!l->owner_token.empty()&&l->revision!=0,"Text policy requires current owned client/selected locale");
 need(same(*c,metrics->snapshot().client),"Text policy metric owner belongs to another client epoch");
 auto owner=std::shared_ptr<BnwClientTextPolicyOwner>(new BnwClientTextPolicyOwner(db,std::move(client),std::move(locale),std::move(metrics)));auto&f=owner->frame_;f.client=*c;f.locale=*l;f.policy=policy;f.source_row=read_bnw_language_font_row(db,l->language);
 switch(policy){case BnwClientTextPolicy::selected_language:f.backend=f.source_row.use_extended_font?BnwClientTextBackend::native_system_font:BnwClientTextBackend::asset_glyphgen;f.follows_selected_language_font_branch=true;break;case BnwClientTextPolicy::asset_glyphgen:f.backend=BnwClientTextBackend::asset_glyphgen;break;case BnwClientTextPolicy::native_system_font:f.backend=BnwClientTextBackend::native_system_font;break;default:throw std::runtime_error("Unknown explicit client text policy");}
 f.authored_ascii_measurement_available=owner->metrics_->snapshot().missing_font_keys.empty()&&!owner->metrics_->snapshot().staged_metrics.empty();
 if(f.backend==BnwClientTextBackend::native_system_font)f.pending_producer="Native system-font raster/measurement owner for selected Languages.FontName/FontWidthScale/FontHeightScale (source0x50E500/0x68C8D0)";
 else if(!f.authored_ascii_measurement_available)f.pending_producer="Actual selected GlyphGen descriptors/glyph measurement owner";
 else f.pending_producer="Label.initialize ordered child/anchor/bounds callbacks and explicit localized text; GlyphGen quote is not initialized Label";
 owner->current();return owner;
}
std::shared_ptr<BnwClientTextPolicyOwner>BnwClientTextPolicyOwner::create_from_worldview_title(
 std::shared_ptr<const BnwWorldViewTitleFontIssuer>issuer){
 need(static_cast<bool>(issuer),"Actual WorldView title font issuer absent");
 const auto observed=issuer->current();const auto&db=issuer->database();
 auto owner=std::shared_ptr<BnwClientTextPolicyOwner>(new BnwClientTextPolicyOwner(db,{},{},{}));
 owner->worldview_=std::move(issuer);owner->worldview_facade_=owner;auto&f=owner->frame_;
 f.client=observed.client;f.locale=observed.locale;f.source_row=observed.language_row;
 f.policy=BnwClientTextPolicy::selected_language;
 f.backend=f.source_row.use_extended_font?BnwClientTextBackend::native_system_font:BnwClientTextBackend::asset_glyphgen;
 f.follows_selected_language_font_branch=true;
 f.pending_producer="WorldView own-C++ candidate descriptor; actual Label Context/Force/GetSize/whole CLASSIN/initialized producer unavailable";
 owner->current();return owner;
}
const BnwClientTextPolicyFrame&BnwClientTextPolicyOwner::current()const{
 if(worldview_){
  // Pin before source callbacks; a callback may release the last facade.
  const auto issuer=worldview_;const auto expected=frame_;const auto weak=worldview_facade_;
  const auto now=issuer->current();need(same(now.client,expected.client)&&now.locale==expected.locale&&now.language_row==expected.source_row,"WorldView policy source epoch changed");
  const auto alive=weak.lock();need(static_cast<bool>(alive),"WorldView policy facade retired during query");return alive->frame_;
 }

 const auto c=client_.current();const auto l=locale_();need(c&&l&&same(*c,frame_.client)&&*l==frame_.locale,"Stale client/selected locale text policy epoch");need(read_bnw_language_font_row(database_,l->language)==frame_.source_row,"Selected Languages font row changed within retained epoch");const auto after=client_.current();const auto next=locale_();need(after&&next&&same(*after,frame_.client)&&*next==frame_.locale,"Text policy owner changed during row query");return frame_;
}
const BnwNativePlainLabelMetric&BnwClientTextPolicyOwner::declared_metric(std::size_t index)const{
 need(!worldview_,"Civilopedia metric seal cannot be replaced by a WorldView descriptor");current();const auto*m=metrics_->metric(frame_.client,metrics_->snapshot().shell_revision,index);need(m,"Declared owned Label metric absent/localized");current();return *m;
}
BnwClientTextBackend BnwClientTextPolicyOwner::backend_for_label(std::size_t index)const{
 if(worldview_){const auto issuer=worldview_;const auto expected=frame_;const auto weak=worldview_facade_;issuer->descriptor(index);need(!weak.expired(),"WorldView policy facade retired");return expected.backend;}

 current();const auto*m=metrics_->metric(frame_.client,metrics_->snapshot().shell_revision,index);need(m,"Selected authored Label backend flags absent/localized");
 // 0x710270: ForceIME overrides global language choice; ForceNonIME disables
 // the global extended-font branch, but cannot override ForceIME.
 return m->force_ime||(frame_.backend==BnwClientTextBackend::native_system_font&&!m->force_non_ime)?BnwClientTextBackend::native_system_font:BnwClientTextBackend::asset_glyphgen;
}
const BnwNativePlainLabelMetric&BnwClientTextPolicyOwner::authored_ascii_metric(std::size_t index)const{
 need(!worldview_,"WorldView descriptor does not authorize Civilopedia ASCII metric");current();need(frame_.authored_ascii_measurement_available&&backend_for_label(index)==BnwClientTextBackend::asset_glyphgen,"Selected text backend lacks actual authored ASCII producer");const auto*m=metrics_->metric(frame_.client,metrics_->snapshot().shell_revision,index);need(m,"Selected authored ASCII Label metric absent/localized");current();return *m;
}
const BnwWorldViewTitleFontDescriptor&BnwClientTextPolicyOwner::declared_worldview_descriptor(std::size_t index)const{
 const auto issuer=worldview_;const auto weak=worldview_facade_;need(static_cast<bool>(issuer),"WorldView source policy absent");const auto&d=issuer->descriptor(index);need(!weak.expired(),"WorldView policy retired during descriptor query");return d;
}
const assets::ResourceMountTable&BnwClientTextPolicyOwner::worldview_resources()const{
 const auto issuer=worldview_;const auto weak=worldview_facade_;need(static_cast<bool>(issuer),"WorldView source policy absent");const auto&t=issuer->resources();need(!weak.expired(),"WorldView policy retired during resources query");return t;
}
std::uintptr_t BnwClientTextPolicyOwner::worldview_hwnd()const{
 const auto issuer=worldview_;const auto weak=worldview_facade_;need(static_cast<bool>(issuer),"WorldView source policy absent");const auto h=issuer->actual_hwnd();need(!weak.expired(),"WorldView policy retired during HWND query");return h;
}
}
