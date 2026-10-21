#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {std::error_code error;std::filesystem::remove(path,error);}
};
}

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        const std::filesystem::path original(argv[1]);
        const auto source_sha=civ5::runtime::identify_file(original).sha256;
        const auto imported=civ5::saves::read_loaded_save(original);
        for(const std::uint32_t id:{41u,42u,43u,44u}) {
            auto document=civ5::saves::make_civis_import_document(
                imported,source_sha);
            document.sections.push_back({id,0,{1}});
            const auto nonce=std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count());
            Cleanup output{std::filesystem::current_path()/
                ("bnw-partial-unit-overlay-"+std::to_string(id)+"-"+nonce+
                 ".CivisSave")};
            civ5::saves::write_civis_save(output.path,document);
            bool rejected=false;
            try {
                (void)civ5::saves::read_loaded_save(output.path);
            } catch(const std::exception &error) {
                rejected=std::string(error.what()).find(
                    "Unsupported Civis import section")!=std::string::npos;
            }
            check(rejected,
                  "Incomplete BNW effect section reached the common loader");
        }
        check(civ5::runtime::identify_file(original).sha256==source_sha,
              "Partial unit overlay test changed the source Civ5Save");
        std::cout<<"PASS incomplete BNW sections 41-44 reject at common import\n";
    } catch(const std::exception &error) {
        std::cerr<<"FAIL BNW partial unit overlay load guard: "
                 <<error.what()<<'\n';
        return 1;
    }
}
