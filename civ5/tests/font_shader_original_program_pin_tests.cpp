#include "font_shader_dx9_oracle.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;
void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void rejects(F&&f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Altered original shader transport accepted");}
int main(int argc,char**argv){
 check(argc==2,"Original read-only game root required");const std::filesystem::path root(argv[1]);
 const auto pinned=validation::read_original_font_shader_programs(root/"CivilizationV.exe");
 validation::validate_original_font_shader_programs(pinned);
 check(pinned.vs.size()==123&&pinned.ps.size()==127,"Original retained row4 size differs");
 auto bad=pinned;bad.vs[27]^=1;rejects([&]{validation::validate_original_font_shader_programs(bad);});
 bad=pinned;bad.ps[32]^=0x100;rejects([&]{validation::validate_original_font_shader_programs(bad);});
 bad=pinned;bad.vs.pop_back();rejects([&]{validation::validate_original_font_shader_programs(bad);});
 bad=pinned;std::swap(bad.vs,bad.ps);rejects([&]{validation::validate_original_font_shader_programs(bad);});
 bad=pinned;bad.exe_sha256=std::string(64,'0');rejects([&]{validation::validate_original_font_shader_programs(bad);});
 bad=pinned;bad.ps_sha256=bad.vs_sha256;rejects([&]{validation::validate_original_font_shader_programs(bad);});
 rejects([&]{validation::read_original_font_shader_programs(root/"no-such-font-shader-source.exe");});
 validation::validate_original_font_shader_programs(pinned);
 std::cout<<"PASS original executable memory SHA/PE row4 extraction/token pins and instruction/length/source substitution refusals; GPU/device/original-game execution=false\n";
 std::cout<<"exe="<<pinned.exe_sha256<<" vs="<<pinned.vs_sha256<<" ps="<<pinned.ps_sha256<<'\n';
}
