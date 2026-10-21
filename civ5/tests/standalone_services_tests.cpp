#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
template<class F> void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}
    check(caught,"Unsupported standalone service data unexpectedly accepted");}
struct Temp {std::filesystem::path path;~Temp(){std::error_code e;std::filesystem::remove(path,e);}};
Temp temporary(const std::string& label){return {std::filesystem::current_path()/
    ("standalone-services-"+label+"-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave")};}
const saves::CivisSaveSection& service_section(const saves::CivisSaveDocument& d){
    const auto id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::standalone_services);
    const auto it=std::find_if(d.sections.begin(),d.sections.end(),[id](const auto& s){return s.id==id;});
    check(it!=d.sections.end(),"Actual checkpoint did not serialize standalone service configuration");return *it;}
void corrupt_service_schema(const std::filesystem::path& path){
    // Container framing: 24-byte header followed by interleaved 16-byte section
    // header and payload. Alter only the service schema in this disposable copy.
    std::fstream file(path,std::ios::in|std::ios::out|std::ios::binary);check(bool(file),"Cannot open disposable corruption fixture");
    auto integer=[&](unsigned width){std::uint64_t v=0;for(unsigned i=0;i<width;++i){char c;file.read(&c,1);
        check(bool(file),"Corruption fixture is truncated");v|=std::uint64_t(static_cast<unsigned char>(c))<<(8*i);}return v;};
    file.seekg(16);const auto count=integer(8);
    for(std::uint64_t i=0;i<count;++i){const auto id=integer(4);(void)integer(4);const auto length=integer(8);
        if(id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::standalone_services)){
            const auto offset=file.tellg();file.seekp(offset);const char bad[4]={2,0,0,0};file.write(bad,4);file.flush();
            check(bool(file),"Cannot corrupt disposable service schema");return;}
        file.seekg(static_cast<std::streamoff>(length),std::ios::cur);}
    throw std::runtime_error("Disposable checkpoint has no service section");}
}
int main(int argc,char** argv){try{
    check(argc==3,"Supply actual original BNW save and installed game root");
    const std::filesystem::path original=argv[1],game_root=argv[2];const auto original_pin=runtime::identify_file(original);
    const saves::StandaloneServicesConfig supported;
    const auto wire=saves::make_standalone_services_section(supported);
    check(wire.bytes==std::vector<std::uint8_t>({1,0,0,0,1,0,0,0,1,0,0,0}),"Standalone wire encoding changed");
    check(saves::read_standalone_services_section(wire)==supported,"Standalone codec failed round trip");
    for(unsigned slot=0;slot<3;++slot){auto bad=wire;bad.bytes.at(slot*4)=2;rejects([&]{(void)saves::read_standalone_services_section(bad);});}
    for(unsigned slot=1;slot<3;++slot){auto bad=wire;bad.bytes.at(slot*4)=0;rejects([&]{(void)saves::read_standalone_services_section(bad);});}
    auto flags=wire;flags.flags=1;rejects([&]{(void)saves::read_standalone_services_section(flags);});
    auto short_wire=wire;short_wire.bytes.pop_back();rejects([&]{(void)saves::read_standalone_services_section(short_wire);});
    auto trailing=wire;trailing.bytes.push_back(0);rejects([&]{(void)saves::read_standalone_services_section(trailing);});

    auto bootstrap=ui::bootstrap_selected_bnw_preview(original,game_root,0);
    check(!bootstrap.preview.blocker&&bootstrap.preview.host,"Actual original BNW import unavailable");
    auto& host=*bootstrap.preview.host;check(host.session().state().standalone_services==supported,"Original import did not choose explicit standalone profile");
    check(!host.session().readiness().can_continue(),"Service config falsely enabled full gameplay");
    const auto exported=host.export_import_clock_checkpoint();check(!exported.full_persistence,"Partial checkpoint falsely claims full persistence");
    check(saves::read_standalone_services_section(service_section(exported.document))==supported,"Actual export lost live profile");
    auto first=temporary("first");saves::write_civis_save(first.path,exported.document);
    const auto loaded=saves::read_loaded_save(first.path);check(loaded.standalone_services&&*loaded.standalone_services==supported,"Normal container loader lost config");
    auto restarted=ui::bootstrap_selected_bnw_preview(first.path,game_root,0);
    check(!restarted.preview.blocker&&restarted.preview.host,"Actual standalone checkpoint normal restore failed");
    check(restarted.preview.host->session().state().standalone_services==supported,"Live restore did not preserve config");
    check(!restarted.preview.host->session().readiness().can_continue(),"Checkpoint falsely enables full continuation");
    auto second=temporary("second");const auto reexport=saves::make_civis_import_document(loaded,original_pin.sha256);
    saves::write_civis_save(second.path,reexport);check(saves::read_loaded_save(second.path).standalone_services==loaded.standalone_services,"Second normal load lost profile");
    const auto first_pin=runtime::identify_file(first.path);rejects([&]{saves::write_civis_save(first.path,exported.document);});
    check(runtime::identify_file(first.path).sha256==first_pin.sha256,"Exclusive checkpoint write modified existing save");
    auto duplicate=exported.document;duplicate.sections.push_back(wire);auto bad_output=temporary("bad");
    rejects([&]{saves::write_civis_save(bad_output.path,duplicate);});check(!std::filesystem::exists(bad_output.path),"Duplicate contract published output");
    auto unknown=exported.document;for(auto& s:unknown.sections)if(s.id==wire.id)s.bytes.at(0)=2;
    rejects([&]{saves::write_civis_save(bad_output.path,unknown);});check(!std::filesystem::exists(bad_output.path),"Unknown schema published output");
    auto corrupt=temporary("corrupt");saves::write_civis_save(corrupt.path,exported.document);corrupt_service_schema(corrupt.path);
    rejects([&]{(void)saves::read_loaded_save(corrupt.path);});
    auto legacy=exported.document;std::erase_if(legacy.sections,[&](const auto& s){return s.id==wire.id;});
    auto missing=temporary("missing");saves::write_civis_save(missing.path,legacy);
    const auto legacy_loaded=saves::read_loaded_save(missing.path);check(!legacy_loaded.standalone_services,"Legacy absence was guessed as supported");
    const auto retained=saves::make_civis_import_document(legacy_loaded,original_pin.sha256);
    check(std::none_of(retained.sections.begin(),retained.sections.end(),[&](const auto& s){return s.id==wire.id;}),"Legacy read-only reexport inferred config");
    const auto rejected=ui::bootstrap_selected_bnw_preview(missing.path,game_root,0);
    check(rejected.preview.blocker.has_value()&&!rejected.preview.host,"Missing live config accepted for overlay restore");
    check(runtime::identify_file(original).sha256==original_pin.sha256,"Standalone tests changed original Civ5Save");
    std::cout<<"PASS explicit standalone services original import / partial checkpoint / negative cases\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL standalone services: "<<e.what()<<'\n';return 1;}}
