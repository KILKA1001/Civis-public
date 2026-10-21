#include "civ5/saves/loaded_save.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace civ5;
void check(bool yes,const char *why) {if(!yes) throw std::runtime_error(why);}
void equal_document(const saves::CivisSaveDocument &a,const saves::CivisSaveDocument &b) {
    check(a.version==b.version&&a.flags==b.flags&&a.sections.size()==b.sections.size(),
          "Import roundtrip changed document envelope");
    for(const auto &part:a.sections) {
        const auto other=std::find_if(b.sections.begin(),b.sections.end(),[&](const auto &p){return p.id==part.id;});
        check(other!=b.sections.end()&&other->flags==part.flags&&other->bytes==part.bytes,
              "Import roundtrip lost source provenance or section bytes");
    }
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"Usage: civis_import_prepublication_tests ORIGINAL_SAVE FRESH_OUTPUT_DIRECTORY");
        const std::filesystem::path original=argv[1],out=argv[2];
        check(!std::filesystem::exists(out),"Test output directory must be fresh");
        const auto before=runtime::identify_file(original);
        const auto source=saves::read_loaded_save(original);
        // read_loaded_save validates native chunk framing and its actual EOF.
        // Retail saves end at Z_SYNC_FLUSH, not necessarily Z_STREAM_END.
        // Require all framed input consumed and all reported output retained;
        // this is import provenance, not full game-state codec readiness.
        check(source.container==saves::SaveContainerKind::civ5&&source.source_prefix&&
              !source.source_prefix->empty()&&!source.bytes.empty()&&
              source.header.end_offset<=source.frame.offset&&
              source.frame.offset==source.source_prefix->size()&&
              source.frame.offset<source.frame.end&&source.frame.end==before.bytes&&
              !source.frame.chunks.empty()&&source.frame.chunks.back().bytes>0&&
              source.frame.chunks.back().bytes<65536&&source.frame.compressed_bytes>0&&
              source.inflate.compressed_bytes==source.frame.compressed_bytes&&
              source.inflate.available_bytes==source.bytes.size()&&
              (source.inflate.stream_end||
               (source.frame.sync_flush_suffix&&source.inflate.sync_flush_suffix)),
              "Natural fixture needs a fully consumed original Civ5Save frame and actual prefix");
        const auto document=saves::make_civis_import_document(source,before.sha256);
        std::filesystem::create_directories(out);
        const auto first=out/"first.CivisSave",second=out/"second.CivisSave";
        saves::write_civis_save(first,document);
        const auto loaded=saves::read_loaded_save(first);
        check(loaded.container==saves::SaveContainerKind::civis&&loaded.bytes==source.bytes&&
              loaded.source_prefix==source.source_prefix&&loaded.source_sha256==before.sha256&&
              loaded.standalone_services&&*loaded.standalone_services==saves::StandaloneServicesConfig{},
              "First natural import lost payload/setup/original source/services");
        const auto next=saves::make_civis_import_document(loaded,runtime::identify_file(first).sha256);
        equal_document(document,next);
        saves::write_civis_save(second,next);
        equal_document(document,saves::read_civis_save(second));
        const auto twice=saves::read_loaded_save(second);
        check(twice.bytes==loaded.bytes&&twice.source_prefix==loaded.source_prefix&&
              twice.source_sha256==loaded.source_sha256&&twice.standalone_services==loaded.standalone_services,
              "Second natural import changed retained state");

        unsigned case_number=0;
        const auto refuses=[&](const saves::CivisSaveDocument &bad) {
            const auto path=out/("bad-"+std::to_string(++case_number)+".CivisSave");
            bool refused=false;
            try {saves::write_civis_save(path,bad);} catch(const std::exception &) {refused=true;}
            check(refused&&!std::filesystem::exists(path),"Malformed import was published");
            for(const auto &entry:std::filesystem::directory_iterator(out))
                check(entry.path().filename().string().find(".civis-tmp-")==std::string::npos,
                      "Rejected import leaked a temporary save");
        };
        const auto erase=[&](saves::CivisSaveDocument &doc,saves::CivisSaveSectionId id) {
            std::erase_if(doc.sections,[&](const auto &p){return p.id==static_cast<std::uint32_t>(id);});
        };
        for(const auto id:{saves::CivisSaveSectionId::imported_payload,saves::CivisSaveSectionId::source_metadata}) {
            auto bad=document;erase(bad,id);refuses(bad);
        }
        for(const auto id:{saves::CivisSaveSectionId::source_metadata,saves::CivisSaveSectionId::source_prefix,
                           saves::CivisSaveSectionId::imported_payload}) {
            auto bad=document;
            for(auto &p:bad.sections) if(p.id==static_cast<std::uint32_t>(id)) p.flags=1;
            refuses(bad);
        }
        {auto bad=document;for(auto &p:bad.sections) if(p.id==4) p.bytes.resize(1);refuses(bad);}
        {auto bad=document;for(auto &p:bad.sections) if(p.id==1) p.bytes.clear();refuses(bad);}
        {auto bad=document;bad.sections.push_back({999,0,{1}});refuses(bad);}
        {auto bad=document;bad.sections.push_back(document.sections.front());refuses(bad);}
        for(const auto id:{saves::CivisSaveSectionId::live_simulation_random,
                           saves::CivisSaveSectionId::live_bnw_city_core}) {
            auto bad=document;bad.sections.push_back({static_cast<std::uint32_t>(id),0,{}});refuses(bad);
            bad.sections.back().flags=1;bad.sections.back().bytes={1};refuses(bad);
        }
        for(const auto id:{saves::CivisSaveSectionId::live_bnw_units,
                           saves::CivisSaveSectionId::live_bnw_treasury_balance,
                           saves::CivisSaveSectionId::live_bnw_research_queue,
                           saves::CivisSaveSectionId::live_bnw_team_turn}) {
            auto bad=document;bad.sections.push_back({static_cast<std::uint32_t>(id),0,{1}});refuses(bad);
        }
        {auto bad=document;bad.sections.push_back({20,0,{1}});bad.sections.push_back({24,0,{1}});refuses(bad);}
        const auto first_hash=runtime::identify_file(first).sha256;
        bool existing_refused=false;
        try{saves::write_civis_save(first,document);}catch(const std::exception&){existing_refused=true;}
        check(existing_refused&&runtime::identify_file(first).sha256==first_hash,"Writer replaced an existing checkpoint");
        saves::CivisSaveDocument general;general.flags=7;general.sections.push_back({3,7,{1,2,3}});
        const auto generic=out/"opaque.CivisSave";saves::write_civis_save(generic,general);
        equal_document(general,saves::read_civis_save(generic));
        check(runtime::identify_file(original).sha256==before.sha256,"Natural test changed original save");
        std::cout<<"PASS natural two-cycle import retention and prepublication structural refusals; full-world continuation UNAVAILABLE\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<"FAIL import prepublication: "<<e.what()<<'\n';return 1;}
}
