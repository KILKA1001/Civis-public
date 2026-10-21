#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#include <thread>
#include <stdexcept>
#include <random>

using namespace civ5;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool stopped=false;try{f();}catch(const std::exception&){stopped=true;}need(stopped,"Expected actual refusal");}
std::int64_t value(content::RulesDatabase& db){
    std::int64_t result=0;std::size_t count=0;
    db.query({"SELECT Value FROM SimpleValues WHERE Name='169-value'",{}},[&](auto row){++count;result=std::get<std::int64_t>(row[0]);});
    need(count==1,"Actual saved SQL row absent/duplicated");return result;
}
}
int main(int argc,char** argv){
    need(argc==4,"Requires actual SAVE, installation and shared stock assets");
    const auto install=std::filesystem::path(argv[2]),assets=std::filesystem::path(argv[3]);
    auto imported=ui::bootstrap_selected_bnw_preview(argv[1],install,0,true);
    need(static_cast<bool>(imported.preview),"Actual baseline selected source unavailable");
    auto& baseline=*imported.preview.host;
    // Prepared image is explicit test content, not permission to synthesize an
    // absent original image in production. Actual loader validates this ID6.
    auto db=content::RulesDatabase::new_saved_values();
    db->apply_sql("INSERT INTO SimpleValues VALUES('169-value',5000000001);");
    const auto first=db->saved_image();
    std::filesystem::path directory;std::random_device random;
    for(unsigned attempt=0;attempt<64&&directory.empty();++attempt){
        const auto candidate=std::filesystem::temp_directory_path()/
            ("civis-selected-save-data-169-"+std::to_string(random())+"-"+std::to_string(random()));
        if(std::filesystem::create_directory(candidate))directory=candidate;
    }
    need(!directory.empty(),"Exclusive private test directory unavailable");
    const auto first_path=directory/"first.CivisSave";
    const auto& loaded=baseline.session().source().source;
    saves::write_civis_save(first_path,saves::make_civis_import_document(
        loaded,baseline.identity().payload_sha256,std::span<const std::uint8_t>(first)));
    auto selected=ui::bootstrap_selected_bnw_preview(first_path,install,0,true);
    if(!selected.preview)throw std::runtime_error(selected.preview.blocker->reason);
    std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(selected.preview.host));
    need(host->identity().payload_sha256==baseline.identity().payload_sha256&&
         host->identity().rules_sha256==baseline.identity().rules_sha256,
         "ID6 was confused with payload/rules identity");
    auto catalog=content::discover_packages({install/"Assets/DLC"});
    need(catalog.diagnostics.empty(),"Actual selected catalog unavailable");
    runtime::BnwSelectedStockLuaSourceOwner owner(install,catalog,selected.profile,host->identity());
    const auto prefix=owner.start_stock_bootstrap(host,assets);
    if(!prefix.first_prefix_executed)throw std::runtime_error(prefix.blocker);
    const auto snapshot=owner.snapshot_stock_save_data();
    need(snapshot.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::saved_database)&&snapshot.flags==0,
         "Actual attempted owner did not emit the DB domain");
    auto saved=content::RulesDatabase::from_saved_image(snapshot.bytes);
    need(value(*saved)==5000000001,"Production source storage differs from actual ID6");
    runtime::BnwSelectedStockLuaSourceOwner second(install,catalog,selected.profile,host->identity());
    refuses([&]{second.start_stock_bootstrap(host,assets);});

    // Exercise the EXISTING actual Lua CAPI/provider backend, independently of
    // production context activation. No unrestricted production VM API is added.
    runtime::LuaScriptHost api({},{},{},{},runtime::lua_saved_data_services(saved));
    api.execute("169",R"lua(
      a=Modding.OpenSaveData(); b=Modding.OpenSaveData()
      assert(a~=b and a.GetValue('169-value')==5000000001)
      assert(a.SetValue('169-value',5000000002) and b.GetValue('169-value')==5000000002)
      assert(a.SetValue('169-null',nil) and b.GetValue('169-null')==nil)
      assert(select('#',b.GetValue('missing'))==1)
      -- Ordinary SQLite statement failure has legitimate false/nil results.
      a.Query('CREATE TRIGGER reject169 BEFORE INSERT ON SimpleValues BEGIN SELECT RAISE(ABORT,\'reject\'); END')()
      assert(a.SetValue('rejected',1)==false and b.GetValue('rejected')==nil)
      a.Query('DROP TRIGGER reject169')()
      assert(a.SetValue('after-error',3) and b.GetValue('after-error')==3)
      a.Query('SAVEPOINT held169')()
    )lua","=actual-save-data-capi");
    refuses([&]{saved->require_saved_checkpoint_boundary();});
    refuses([&]{saved->saved_image();});
    api.execute("169","a.Query('RELEASE held169')()","=actual-savepoint-return");
    saved->require_saved_checkpoint_boundary();
    // Busy implicit read transaction is also refused even with autocommit true.
    auto retained=saved->reopen_saved();
    auto cursor=retained->script_query("SELECT Name FROM SimpleValues",{});
    need(cursor&&cursor->step(),"Real busy SQL cursor absent");
    refuses([&]{saved->require_saved_checkpoint_boundary();});
    cursor->reset();saved->require_saved_checkpoint_boundary();
    const auto second_path=directory/"second.CivisSave";
    const auto second_image=saved->saved_image();
    saves::write_civis_save(second_path,saves::make_civis_import_document(
        host->session().source().source,host->identity().payload_sha256,
        std::span<const std::uint8_t>(second_image)));
    const auto restored=saves::read_loaded_save(second_path);
    need(restored.saved_database_override.has_value(),"Actual reader dropped ID6");
    auto restored_db=content::RulesDatabase::from_saved_image(*restored.saved_database_override);
    need(value(*restored_db)==5000000002,"Actual file roundtrip lost Lua writes");
    auto restored_host=ui::bootstrap_selected_bnw_preview(second_path,install,0,true);
    need(static_cast<bool>(restored_host.preview),"Selected production consumer rejected validated ID6");
    need(!restored_host.preview.host->session().readiness().can_continue(),"DB domain granted full game readiness");
    std::shared_ptr<ui::SelectedBnwPreviewHost> failed_host(std::move(restored_host.preview.host));
    runtime::BnwSelectedStockLuaSourceOwner failed_owner(install,catalog,restored_host.profile,failed_host->identity());
    need(failed_owner.start_stock_bootstrap(failed_host,assets).first_prefix_executed,"Actual failure fixture prefix failed");
    need(failed_owner.attach_stock_client_extent({[]()->std::optional<runtime::BnwStockLuaClientExtent>{
        throw std::runtime_error("169 real provider failure");
    }}),"Actual failure provider was not retained");
    refuses([&]{failed_owner.advance_stock_bootstrap();});
    need(failed_owner.stock_operation_evidence().state==runtime::BnwStockLuaOperationState::uncertain,
         "Actual callback failure was not retained");
    refuses([&]{failed_owner.snapshot_stock_save_data();});
    refuses([&]{saves::write_civis_save(second_path,saves::make_civis_import_document(
        loaded,baseline.identity().payload_sha256,std::span<const std::uint8_t>(first)));});
    need(value(*content::RulesDatabase::from_saved_image(
        *saves::read_loaded_save(second_path).saved_database_override))==5000000002,
        "Exclusive physical writer overwrote the prior save");

    bool foreign_refused=false;
    std::thread foreign([&]{try{(void)owner.snapshot_stock_save_data();}catch(const std::exception&){foreign_refused=true;}});
    foreign.join();need(foreign_refused,"Foreign source owner thread exported SQL");
    refuses([&]{owner.snapshot_stock_save_data();});
    auto malformed=first;malformed[0]=0;
    refuses([&]{content::RulesDatabase::from_saved_image(malformed);});
    auto wrong=host->identity();wrong.rules_sha256[0]=wrong.rules_sha256[0]=='a'?'b':'a';
    runtime::BnwSelectedStockLuaSourceOwner unrelated(install,catalog,selected.profile,wrong);
    refuses([&]{unrelated.start_stock_bootstrap(host,assets);});
    need(!host->session().readiness().can_continue(),"Saved data snapshot opened gameplay");
    std::cout<<"PASS: actual selected ID6 restore/source binding; guarded attempted owner DB snapshot; actual Lua CAPI shared connections, false/nil, savepoint/busy cursor refusal and physical reload; no world/VM export grant\n";
}
