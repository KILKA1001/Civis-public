#include "civ5/ui/civis_resource_bank_font_color_snapshot.hpp"
#include <algorithm>
#include <charconv>
#include <array>
#include <exception>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>
namespace civ5::ui {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void add_budget(std::uint64_t& total,std::uint64_t value,std::uint64_t maximum){
 need(value<=maximum&&total<=maximum-value,"Own bank font/color source budget exceeded");
 total+=value;
}
bool named_alpha_style(std::string_view name){
 return name=="Base"||name=="Soft"||name=="Shadow"||name=="Stroke"||name=="SoftShadow";
}
}
struct CivisResourceBankFontColorSnapshot::Control {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},retired{false},failed{false};
 bool busy=false;
 void pure()const{
  if(creator!=std::this_thread::get_id()){
   const_cast<Control*>(this)->failed=true;
   throw std::runtime_error("Own bank font/color snapshot foreign creator");
  }
  need(alive&&!retired&&!failed,"Own bank font/color snapshot retired or poisoned");
 }
};
CivisResourceBankFontColorSnapshot::CivisResourceBankFontColorSnapshot(
 std::shared_ptr<Control> control,std::shared_ptr<const assets::CivisResourceBankOwner> bank,
 std::shared_ptr<const CivisResourceBankUiSnapshot> ui,
 std::vector<CivisResourceBankAuthoredFont> fonts,
 std::vector<CivisResourceBankAuthoredStyle> styles,std::optional<content::XmlNode> colors,
 std::vector<CivisResourceBankFontSource> sources)
 :control_(std::move(control)),bank_(std::move(bank)),ui_(std::move(ui)),
  capability_(bank_->capability()),fonts_(std::move(fonts)),styles_(std::move(styles)),
  colors_(std::move(colors)),sources_(std::move(sources)){require_current();}
CivisResourceBankFontColorSnapshot::~CivisResourceBankFontColorSnapshot(){control_->alive=false;}
std::shared_ptr<const CivisResourceBankFontColorSnapshot>
CivisResourceBankFontColorSnapshot::capture(
 const std::shared_ptr<assets::CivisResourceBankOwner>& supplied_bank,
 const std::shared_ptr<const CivisResourceBankUiSnapshot>& supplied_ui,
 CivisResourceBankFontColorBudget budget){
 need(supplied_bank&&supplied_ui,"Own bank fonts require actual bank and authored UI");
 // Canonical metadata first. Neither call performs client callbacks or lazy IO.
 const auto bank=supplied_bank->retain_owner();
 need(bank&&bank.get()==supplied_bank.get()&&!bank.owner_before(supplied_bank)&&
      !supplied_bank.owner_before(bank),"Own bank font input has another bank control");
 const auto ui=supplied_ui->retain_owner();
 need(ui&&ui.get()==supplied_ui.get()&&!ui.owner_before(supplied_ui)&&
      !supplied_ui.owner_before(ui),"Own bank font input has another authored UI control");
 ui->require_same_bank(*bank);
 auto control=std::make_shared<Control>();control->busy=true;
 struct End{std::shared_ptr<Control> c;int unwind=std::uncaught_exceptions();
  ~End(){if(std::uncaught_exceptions()>unwind)c->failed=true;c->busy=false;}} end{control};
 const auto cap=bank->capability();
 const auto pure=[&]{control->pure();cap.require_current();bank->require_current();
  ui->require_current();ui->require_same_bank(*bank);control->pure();};
 const auto boundary=[&]{pure();bank->require_boundary();pure();};
 need(budget.source_bytes&&budget.alpha_bytes&&budget.retained_dds_bytes&&budget.sources&&budget.styles,
      "Own bank font budget is empty");
 boundary();
 std::vector<CivisResourceBankFontSource> sources;
 std::set<std::string> requested;
 std::set<const assets::Bytes*> resident;
 std::uint64_t source_bytes=0,alpha_bytes=0,retained_dds_bytes=0;
 const auto stage=[&](const std::string& key){
  pure();bank->load(assets::AssetKey(key),assets::CivisResourceParse::bytes);pure();
  const auto loaded=cap.loaded(assets::AssetKey(key));pure();
  need(static_cast<bool>(loaded.bytes),"Own bank font resource has no real bytes");
  if(resident.insert(loaded.bytes.get()).second)
   add_budget(source_bytes,loaded.bytes->size(),budget.source_bytes);
  if(requested.insert(assets::normalize_asset_name(key)).second){
   need(sources.size()<budget.sources,"Own bank font source count exceeded");
   sources.push_back({key,loaded.candidate,loaded.payload_identity});
  }
  pure();return loaded;
 };
 std::optional<content::XmlNode> colors;
 const std::string color_key="Assets/UI/ColorAtlas.xml";
 if(bank->has_indexed(assets::AssetKey(color_key))){
  pure();const auto loaded=stage(color_key);
  // Match existing optional style parsing: missing/malformed palette is unknown.
  try {auto value=content::read_xml_metadata(std::string(loaded.bytes->begin(),loaded.bytes->end()));
   if(value.name=="ColorAtlas")colors=std::move(value);
  }catch(const std::exception&){colors.reset();}
  pure();
 }
 std::vector<CivisResourceBankAuthoredFont> fonts;
 std::vector<CivisResourceBankAuthoredStyle> styles;
 for(const auto font_name:std::array<const char*,5>{"TwCenMT14","TwCenMT16","TwCenMT18","TwCenMT20","TwCenMT24"}){
  const std::string key=std::string("Assets/UI/Fonts/Tw Cent MT/")+font_name+".ggxml";
  const auto loaded=stage(key);
  auto font=GlyphGenFont::from_xml(content::read_xml_metadata(
    std::string(loaded.bytes->begin(),loaded.bytes->end())));pure();
  need(!font.styles.empty()&&font.styles.size()<=64&&font.textures.size()<=64,
       "Own bank authored font declarations exceed selected profile");
  // The complete declarations, including genuine empty/native style0, remain
  // in font(). Only the FIVE named A8 styles use this existing CPU loader.
  for(const auto& declared:font.styles){
   if(!named_alpha_style(declared.name))continue;
   need(styles.size()<budget.styles,"Own bank alpha style count exceeded");
   pure();const auto metadata=read_glyphgen_style_metadata(
     bank->resources(cap),font_name,declared.name);pure();
   const auto pixels=std::uint64_t{font.atlas_width()}*font.atlas_height();
   need(metadata.layers.size()<=3,"Own bank alpha style has too many real layers");
   for(std::size_t n=0;n<metadata.layers.size();++n)
    add_budget(alpha_bytes,pixels,budget.alpha_bytes);
   for(std::size_t n=0;n<metadata.layers.size();++n){
    const auto& layer=metadata.layers[n];
    const auto image=stage(layer.texture_source);
    const auto& original=image.candidate.original_name;
    need(original.size()>=4&&assets::normalize_asset_name(original).ends_with(".dds"),
         "Own bank authored layer is not actual DDS");
    const auto index=stage(original.substr(0,original.size()-4)+"-index.dds");
    // Existing load_style retains the first pair once per style. Reserve its
    // complete file sizes before the decoder: DDS payload cannot exceed them.
    if(n==0){
     add_budget(retained_dds_bytes,image.bytes->size(),budget.retained_dds_bytes);
     add_budget(retained_dds_bytes,index.bytes->size(),budget.retained_dds_bytes);
    }
   }
   pure();auto asset=load_glyphgen_style(bank->resources(cap),font_name,declared.name);pure();
   need(asset.layers.size()==metadata.layers.size(),"Own bank actual style layer order changed");
   for(const auto& layer:asset.layers)
    need(std::uint64_t{layer.atlas.width}*layer.atlas.height==layer.atlas.alpha.size(),
         "Own bank real alpha atlas size differs");
   styles.push_back({font_name,declared.name,std::move(asset)});
  }
  for(const auto required:std::array<const char*,5>{"Base","Soft","Shadow","Stroke","SoftShadow"})
   need(std::any_of(styles.begin(),styles.end(),[&](const auto& s){
     return s.font_key==font_name&&s.style_key==required;}),
     "Own bank selected font misses actual supported authored style");
  fonts.push_back({font_name,std::move(font)});pure();
 }
 boundary();
 auto result=std::shared_ptr<CivisResourceBankFontColorSnapshot>(
  new CivisResourceBankFontColorSnapshot(control,bank,ui,std::move(fonts),std::move(styles),
    std::move(colors),std::move(sources)));
 result->canonical_self_=result;
 boundary();result->require_current();
 return result;
}
std::shared_ptr<const CivisResourceBankFontColorSnapshot>
CivisResourceBankFontColorSnapshot::retain_owner()const{
 require_current();const auto self=canonical_self_.lock();
 need(self&&self.get()==this,"Own bank font/color factory facade retired");return self;
}
void CivisResourceBankFontColorSnapshot::retire()const noexcept{control_->retired=true;}
void CivisResourceBankFontColorSnapshot::require_current()const{
 control_->pure();capability_.require_current();bank_->require_current();
 ui_->require_current();ui_->require_same_bank(*bank_);control_->pure();
}
void CivisResourceBankFontColorSnapshot::require_boundary()const{
 require_current();
 // Callback may release the caller's last facade owner. Retain producers,
 // never the facade, and observe its atomic lifetime BEFORE any facade read.
 const auto control=control_;const auto bank=bank_;const auto ui=ui_;
 const auto cap=bank->capability();
 const auto pure=[&]{control->pure();cap.require_current();bank->require_current();
  ui->require_current();ui->require_same_bank(*bank);control->pure();};
 if(control->busy){control->failed=true;throw std::runtime_error("Own font/color boundary reentered");}
 control->busy=true;
 struct End{std::shared_ptr<Control> c;int unwind=std::uncaught_exceptions();
  ~End(){if(std::uncaught_exceptions()>unwind)c->failed=true;c->busy=false;}} end{control};
 pure();
 try {bank->require_boundary();}
 catch(...){control->pure();throw;}
 pure();
}
void CivisResourceBankFontColorSnapshot::require_same_bank(const assets::CivisResourceBankOwner& bank)const{
 require_current();need(bank_.get()==&bank,"Own font/color snapshot has another bank");
}
void CivisResourceBankFontColorSnapshot::require_same_ui(const CivisResourceBankUiSnapshot& ui)const{
 require_current();need(ui_.get()==&ui,"Own font/color snapshot has another authored UI");
}
const GlyphGenFont& CivisResourceBankFontColorSnapshot::font(std::string_view key)const{
 require_current();for(const auto& f:fonts_)if(f.font_key==key)return f.authored;
 throw std::runtime_error("Own bank font not captured");
}
const GlyphGenStyleAsset& CivisResourceBankFontColorSnapshot::style(
 std::string_view font_key,std::string_view style_key)const{
 require_current();for(const auto& s:styles_)if(s.font_key==font_key&&s.style_key==style_key)return s.asset;
 throw std::runtime_error("Own bank supported named alpha style not captured");
}
const std::vector<CivisResourceBankAuthoredStyle>& CivisResourceBankFontColorSnapshot::styles()const{
 require_current();return styles_;
}
const std::optional<content::XmlNode>& CivisResourceBankFontColorSnapshot::color_atlas()const{
 require_current();return colors_;
}
std::optional<std::uint32_t>CivisResourceBankFontColorSnapshot::color(std::string_view declaration)const{
 require_current();const std::string request(declaration);
 const auto control=control_;const auto bank=bank_;const auto ui=ui_;
 const auto cap=bank->capability();
 const auto pure=[&]{control->pure();cap.require_current();bank->require_current();
  ui->require_current();ui->require_same_bank(*bank);control->pure();};
 if(control->busy){control->failed=true;throw std::runtime_error("Own font/color resolver reentered");}
 control->busy=true;
 struct End{std::shared_ptr<Control> c;~End(){c->busy=false;}} end{control};
 pure();std::optional<std::uint32_t> value;
 try {value=resolve_ui_label_color(bank->resources(cap),request);}
 catch(const std::exception&){pure();return {};}
 catch(...){control->pure();throw;}
 pure();return value;
}
const std::vector<CivisResourceBankFontSource>& CivisResourceBankFontColorSnapshot::source_records()const{
 require_current();return sources_;
}
std::optional<std::uint32_t> CivisResourceBankFontColorSnapshot::strict_named_color(std::string_view token)const{
 require_current();const std::string request(token);
 need(!request.empty()&&request.size()<=1024&&request.find_first_of(",\0",0,2)==std::string::npos,
      "Strict named Color requires one bounded actual name token");
 const auto control=control_;const auto bank=bank_;const auto ui=ui_;
 const auto cap=bank->capability();
 const auto expected=cap.loaded(assets::AssetKey("Assets/UI/ColorAtlas.xml"));
 need(static_cast<bool>(expected.bytes),"Strict named Color palette bytes were not captured");
 const auto pure=[&]{control->pure();cap.require_current();bank->require_current();
  ui->require_current();ui->require_same_bank(*bank);control->pure();};
 if(control->busy){control->failed=true;throw std::runtime_error("Strict named Color resolver reentered");}
 control->busy=true;
 struct End{std::shared_ptr<Control> c;int unwind=std::uncaught_exceptions();
  ~End(){if(std::uncaught_exceptions()>unwind)c->failed=true;c->busy=false;}} end{control};
 pure();
 try{
  // Resolve the real selected immutable namespace: observer errors stay errors.
  const auto resource=bank->resources(cap).resolve(assets::AssetKey("Assets/UI/ColorAtlas.xml"));
  control->pure();pure();
  need(std::holds_alternative<assets::AssetHandle>(resource.asset),"Strict named Color palette missing or ambiguous");
  const auto& handle=std::get<assets::AssetHandle>(resource.asset);
  need(assets::normalize_asset_name(handle.candidate().original_name)==assets::normalize_asset_name(expected.candidate.original_name),
       "Strict named Color selected winner changed");
  const auto bytes=handle.read_bytes(64ULL*1024*1024);control->pure();pure();
  const auto identity=runtime::identify_bytes(bytes);
  need(identity.bytes==expected.payload_identity.bytes&&identity.sha256==expected.payload_identity.sha256,
       "Strict named Color palette differs from captured source");
  const auto palette=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
  need(palette.name=="ColorAtlas","Strict named Color palette root unsupported");
  std::set<std::string> names;std::optional<std::uint32_t> value;
  for(const auto& row:palette.children){
   // ColorSet belongs to an independent palette row kind, not named Color lookup.
   if(row.name=="ColorSet")continue;
   need(row.name=="Color","Strict named Color palette row unsupported");
   const auto name=row.attribute("Name");
   need(!name.empty()&&name.size()<=1024&&names.insert(name).second,"Strict named Color duplicate or invalid name");
   auto literal=row.attribute("Color");std::replace(literal.begin(),literal.end(),'.',',');
   std::array<std::uint32_t,4> channels{};
   for(std::size_t i=0;i<4;++i){
    const auto comma=literal.find(',');const auto part=literal.substr(0,comma);
    const auto parsed=std::from_chars(part.data(),part.data()+part.size(),channels[i]);
    need(parsed.ec==std::errc{}&&parsed.ptr==part.data()+part.size()&&channels[i]<=255&&
         (i==3?comma==std::string::npos:comma!=std::string::npos),"Strict named Color invalid RGBA row");
    if(i!=3)literal.erase(0,comma+1);
   }
   if(name==request)value=(channels[0]<<24)|(channels[1]<<16)|(channels[2]<<8)|channels[3];
  }
  pure();return value; // Absent name only AFTER successful full palette validation.
 }catch(...){control->pure();throw;}
}
}
