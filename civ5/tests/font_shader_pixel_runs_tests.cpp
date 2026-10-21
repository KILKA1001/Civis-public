#include "font_shader_pixel_runs.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace civ5::validation;
void check(bool b){if(!b)throw std::runtime_error("Pixel run regression failed");}
template<class F>void rejects(F f){bool b=false;try{f();}catch(const std::invalid_argument&){b=true;}check(b);}
int main(int argc,char**argv){
 check(argc==2);const std::filesystem::path out(argv[1]);std::filesystem::create_directories(out);
 const std::array<std::uint8_t,16> fp{0,0,0,0,0,0,0,128,1,0,192,127,0xff,0xff,0x7f,0x7f};
 std::vector<std::uint8_t> raw;for(int n=0;n<7;++n)raw.insert(raw.end(),fp.begin(),fp.end());
 raw.insert(raw.end(),16,0x35);raw.insert(raw.end(),fp.begin(),fp.end());
 const auto encoded=encode_font_shader_pixel_runs(raw,3,3,16);
 check(encoded.size()==20+3*20&&encoded[20]==7&&encoded[40]==1&&encoded[60]==1);
 check(std::equal(fp.begin(),fp.end(),encoded.begin()+24));
 const auto write=[&](const char*key,const auto&bytes){std::ofstream f(out/key,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(static_cast<bool>(f));};
 write("float.raw",raw);write("float.pxr",encoded);
 const std::array<std::uint8_t,16> rgba{0,1,254,255,0,1,254,255,3,2,1,0,4,5,6,7};
 const auto rgb=encode_font_shader_pixel_runs(rgba,2,2,4);check(rgb.size()==44&&rgb[20]==2);write("rgba.raw",rgba);write("rgba.pxr",rgb);
 rejects([&]{encode_font_shader_pixel_runs(raw,0,9,16);});
 rejects([&]{encode_font_shader_pixel_runs(raw,3,3,8);});
 rejects([&]{encode_font_shader_pixel_runs(raw,3,4,16);});
 rejects([&]{encode_font_shader_pixel_runs(raw,0xffffffff,0xffffffff,16);});
 std::cout<<"PASS exact float bit patterns/zero sign/NaN bytes, RGBA8, run boundaries and malformed encoder inputs; CPU only\n";
}
