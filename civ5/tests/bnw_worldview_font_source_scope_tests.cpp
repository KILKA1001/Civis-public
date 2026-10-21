#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5;
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void refused(F&&f,const char*s){bool bad=false;try{f();}catch(const std::exception&){bad=true;}need(bad,s);}
struct Fixture {
 std::filesystem::path root;
 std::shared_ptr<assets::LooseResourceIndex>index;
 unsigned reads=0;std::function<void()>callback;
 explicit Fixture(std::filesystem::path p):root(std::move(p)){
  std::filesystem::create_directories(root);put("English.xml","<GameData><Languages><Row><Type>en_US</Type><TableName>Language_en_US</TableName><FontName>metadata-only-ignored-face</FontName></Row></Languages></GameData>");
  put("Russian.xml","<GameData><Languages><Row><Type>ru_RU</Type><TableName>Language_RU_RU</TableName></Row></Languages></GameData>");
  index=std::make_shared<assets::LooseResourceIndex>(root);index->add_file("English.xml");index->add_file("Russian.xml");
 }
 void put(std::string name,std::string data){std::ofstream out(root/name,std::ios::binary);out<<data;need(static_cast<bool>(out),"Fixture write failed");}
 assets::ResourceMountTable table(){
  return assets::ResourceMountTable([this,source=index](const assets::AssetKey&key){
   ++reads;const auto copy=callback;if(copy)copy();return assets::ResourceMountHit{source->find(assets::AssetKey(assets::resource_basename(key.original_name()))),true};
  });
 }
};
ui::UiFrameSnapshot frame(){return {7,2,3};}
}
int main(int argc,char**argv){try{
 need(argc==2,"Unique bounded fixture-output directory required");const std::filesystem::path root=argv[1];
 {Fixture f(root/"current");auto table=f.table();auto scope=ui::BnwWorldViewFontSourceScope::create(std::move(table),"en_US");
  need(scope->locale().language=="en_US"&&!scope->locale().owner_token.empty(),"Selected metadata binding absent");
  const auto*t=&scope->resources();need(&scope->resources()==t&&f.reads>0,"Source scope replaced table or skipped actual lookup");
  const auto a=scope->augment_snapshot(frame());need(a.find<ui::BnwWorldViewFontSourceQuote>()&&ui::ui_owners_current(a)&&a.frame()==7&&a.active_player_id()==2&&a.turn()==3,"Typed own binding lost frame/base fences");
  refused([&]{scope->augment_snapshot(a);},"Duplicate source quote accepted");
  // Expected failed augmentation poisons this epoch, rather than re-admitting
  // old published quotes after a partial/source error.
  need(!ui::ui_owners_current(a),"Failed augmentation left source quote current");
 }
 // A real inherited fence may query this SAME metadata-backed font scope.
 // It must not collide with the augmentation admission guard; no currentness
 // is fabricated, and the normal published quote fence remains mandatory.
 {Fixture f(root/"inherited-font-query");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");
  std::weak_ptr<ui::BnwWorldViewFontSourceScope> weak=scope;unsigned checks=0;
  auto fences=std::make_shared<ui::UiOwnerFences>();
  fences->checks.push_back([weak,&checks]{{auto current=weak.lock();if(!current)return false;++checks;current->require_current();}return !weak.expired();});
  const auto before=frame().with(std::shared_ptr<const ui::UiOwnerFences>(fences));const auto reads=f.reads;
  const auto after=scope->augment_snapshot(before);
  need(checks>=2&&f.reads>reads&&after.find<ui::BnwWorldViewFontSourceQuote>()&&ui::ui_owners_current(after),
       "Legitimate inherited SAME font query failed augmentation or skipped resource checks");
  scope->retire();need(!ui::ui_owners_current(before)&&!ui::ui_owners_current(after),"Inherited/published font fences lost retirement");
 }
 {Fixture f(root/"nested-augmentation-fence");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");bool nested=false;
  auto fences=std::make_shared<ui::UiOwnerFences>();
  fences->checks.push_back([&]{try{scope->augment_snapshot(frame());}catch(const std::exception&){nested=true;}return true;});
  const auto before=frame().with(std::shared_ptr<const ui::UiOwnerFences>(fences));
  refused([&]{scope->augment_snapshot(before);},"Caught inherited-fence nested augmentation authorized outer quote");
  need(nested,"Inherited fence did not reach nested augmentation");refused([&]{scope->require_current();},"Nested augmentation poison disappeared");
 }
 {Fixture f(root/"nested-augmentation-resource");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");bool nested=false;
  f.callback=[&]{try{scope->augment_snapshot(frame());}catch(const std::exception&){nested=true;}};
  refused([&]{scope->augment_snapshot(frame());},"Caught resource nested augmentation authorized outer quote");
  need(nested,"Resource callback did not reach nested augmentation");f.callback={};refused([&]{scope->require_current();},"Resource nested augmentation poison disappeared");
 }
 {Fixture f(root/"augmentation-resource-query");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");bool nested=false;
  f.callback=[&]{try{scope->require_current();}catch(const std::exception&){nested=true;}};
  refused([&]{scope->augment_snapshot(frame());},"Caught busy resource query authorized augmentation");
  need(nested,"Augmentation metadata callback did not reach recursive source query");f.callback={};refused([&]{scope->require_current();},"Augmentation resource reentry poison disappeared");
 }
 {Fixture f(root/"retire");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"ru_RU");
  auto a=scope->augment_snapshot(frame());scope->retire();scope->retire();refused([&]{scope->require_current();},"Retired scope refreshed");need(!ui::ui_owners_current(a),"Retired locale frame remained current");
 }
 {Fixture f(root/"payload");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");auto a=scope->augment_snapshot(frame());
  // Same file size, changed payload. Restore is not a retirement refresh.
  f.put("English.xml","<GameData><Languages><Row><Type>en_US</Type><TableName>Language_en_US</TableName><FontName>metadata-only-CHANGED-face</FontName></Row></Languages></GameData>");
  refused([&]{scope->resources();},"Changed metadata bytes accepted");need(!ui::ui_owners_current(a),"Changed source binding accepted");
 }
 {Fixture f(root/"source");auto first=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");auto a=first->augment_snapshot(frame());first.reset();need(!ui::ui_owners_current(a),"Snapshot immortalized real source facade");
  auto second=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");need(!ui::ui_owners_current(a)&&second->locale().revision==1,"New scope resurrected old quote");
 }
 {Fixture f(root/"fence-release");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");auto a=scope->augment_snapshot(frame());
  f.callback=[&]{scope.reset();};need(!ui::ui_owners_current(a),"A fence validation lease immortalized the final facade");
 }
 {Fixture f(root/"recursive");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");bool inner=false;
  f.callback=[&]{try{scope->require_current();}catch(const std::exception&){inner=true;}};
  refused([&]{scope->require_current();},"Caught recursive source query authorized outer operation");need(inner,"Recursive source query not reached");f.callback={};refused([&]{scope->require_current();},"Recursive poison disappeared");
 }
 {Fixture f(root/"foreign");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");const auto before=f.reads;bool bad=false;
  std::thread other([&]{try{scope->require_current();}catch(const std::exception&){bad=true;}});other.join();need(bad&&f.reads==before,"Foreign thread reached resource callback");refused([&]{scope->locale();},"Foreign-thread poison disappeared");
 }
 {Fixture f(root/"release");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");auto*raw=scope.get();std::weak_ptr<ui::BnwWorldViewFontSourceScope>weak=scope;
  f.callback=[&]{scope.reset();};refused([&]{raw->resources();},"Borrowed table returned after final facade release");need(weak.expired(),"Validation lease became facade authority");
 }
 {Fixture f(root/"fences");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");bool base_live=true;auto fences=std::make_shared<ui::UiOwnerFences>();fences->checks.push_back([&]{return base_live;});
  auto before=frame().with(std::shared_ptr<const ui::UiOwnerFences>(fences));f.callback=[&]{base_live=false;};refused([&]{scope->augment_snapshot(before);},"Source callback mutation lost original base fences");need(!ui::ui_owners_current(before),"Original fence unexpectedly overwritten");refused([&]{scope->require_current();},"Failed inherited-base augmentation did not poison epoch");
 }
 {Fixture f(root/"retire-during");auto scope=ui::BnwWorldViewFontSourceScope::create(f.table(),"en_US");f.callback=[&]{scope->retire();};refused([&]{scope->augment_snapshot(frame());},"Retirement callback published locale quote");}
 {Fixture f(root/"wrong-language");refused([&]{ui::BnwWorldViewFontSourceScope::create(f.table(),"unknown");},"Unknown UI language admitted");f.put("Russian.xml","<GameData><Languages><Row><Type>en_US</Type><TableName>Language_en_US</TableName></Row></Languages></GameData>");refused([&]{ui::BnwWorldViewFontSourceScope::create(f.table(),"ru_RU");},"Mismatched metadata language admitted");}
 // Query reader is the EXISTING attached-Languages reader, independent of
 // metadata strings and ordinary GlyphGen scale defaults.
 {content::RulesDatabase main,loc;loc.apply_sql("CREATE TABLE Languages(Type TEXT,UseExtendedFont INTEGER,FontName TEXT,FontWidthScale REAL,FontHeightScale REAL); INSERT INTO Languages VALUES('ru_RU',1,'actual-sql-face',0.37,0.81)");main.attach_localization(loc);
  const auto row=ui::read_bnw_language_font_row(main,"ru_RU");need(row.use_extended_font&&row.font_name=="actual-sql-face"&&row.width_scale==0.37f&&row.height_scale==0.81f,"Metadata/defaults replaced effective attached row");
  main.apply_sql("UPDATE Localization.Languages SET FontWidthScale=0 WHERE Type='ru_RU'");refused([&]{ui::read_bnw_language_font_row(main,"ru_RU");},"Collapsed actual scale admitted");
 }
 refused([&]{ui::BnwClientTextPolicyOwner::create_from_worldview_title({});},"Policy minted missing title issuer");
 std::cout<<"PASS source-scope fixtures only: retained moved table, private quotes, base fences, release/reentry/thread/retirement/source and effective SQL refusals; no actual camera/native Label proof\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
