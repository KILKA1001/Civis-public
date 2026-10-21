// Own C++ file-only CRC gate; never executes the original file.
#include "civ5/graphics/file_only_fov_input.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
namespace fov=civ5::graphics::file_only_fov;
void need(bool v,const char *m){if(!v)throw std::runtime_error(m);}
template<class F>void rejected(F &&f) {
    bool refused=false;
    try{f();}catch(const fov::InputError&e){refused=e.refusal==fov::Refusal::wrong_pinned_file;}
    need(refused,"Unpinned file produced a quote");
}
int main(int argc,char **argv) {try{
    need(argc==2,"Argument must be pinned original CivilizationV.exe FILE (never execute it)");
    std::ifstream input(std::filesystem::path(argv[1]),std::ios::binary);
    need(static_cast<bool>(input),"Original file unavailable");
    const std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(input)),{});
    const auto quote=fov::calculate(file);
    // Independent rawbytes/zlib result recorded by source-facts.json.
    need(quote.file_crc_bits()==0x42700000u&&quote.file_ieee_float()==60.f,
        "Own C++ file-only port differs from independent source reference");
    auto wrong=file;wrong[fov::detail::ordered_ranges.front().file_offset]^=1;
    rejected([&]{fov::calculate(wrong);});
    rejected([&]{fov::calculate(std::span<const std::uint8_t>(file).first(file.size()-1));});
    rejected([&]{fov::calculate({});});
    need(fov::calculate(file).file_crc_bits()==quote.file_crc_bits(),"Immutable file replay differs");
    static_assert(!fov::Quote::loaded_memory_observed&&!fov::Quote::native_current_fov_issued&&
        !fov::Quote::native_lens_issuer_ready&&!fov::Quote::changed_extent_drawable);
    std::cout<<"PASS own C++ file-only CRC input; current lens/resize authority=false\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
