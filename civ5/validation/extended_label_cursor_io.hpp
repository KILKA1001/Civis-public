#pragma once
#include "extended_label_cursor_source_pins.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
namespace cursor_test {
using Bytes=std::vector<unsigned char>;
inline void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
inline Bytes read(const std::filesystem::path &p){std::ifstream in(p,std::ios::binary);need(bool(in),"Read retained source/reference");return Bytes(std::istreambuf_iterator<char>(in),{});}
inline std::string sha(const Bytes &b){
    BCRYPT_ALG_HANDLE a=nullptr;BCRYPT_HASH_HANDLE h=nullptr;
    need(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"SHA provider");
    DWORD size=0,received=0;
    const auto property=BCryptGetProperty(a,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&received,0);
    if(property<0){BCryptCloseAlgorithmProvider(a,0);throw std::runtime_error("SHA property");}
    Bytes storage(size),digest(32);
    const auto created=BCryptCreateHash(a,&h,storage.data(),size,nullptr,0,0);
    if(created<0){BCryptCloseAlgorithmProvider(a,0);throw std::runtime_error("SHA create");}
    const auto hashed=BCryptHashData(h,const_cast<PUCHAR>(b.data()),static_cast<ULONG>(b.size()),0);
    const auto finished=hashed<0?hashed:BCryptFinishHash(h,digest.data(),32,0);
    BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(a,0);need(finished>=0,"SHA finish");
    std::string out;for(auto x:digest){out+="0123456789abcdef"[x>>4];out+="0123456789abcdef"[x&15];}return out;
}
inline void write(const std::filesystem::path &p,const Bytes &b){std::ofstream out(p,std::ios::binary);need(bool(out),"Create NEW artifact");out.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));out.flush();need(bool(out),"Flush NEW artifact");out.close();need(!out.fail(),"Close NEW artifact");}
inline void put(Bytes &b,std::uint64_t value,unsigned count=4){for(unsigned n=0;n<count;++n)b.push_back(static_cast<unsigned char>(value>>(8*n)));}
inline std::uint64_t get(const Bytes &b,std::size_t &at,unsigned count=4){need(count<=8&&at+count<=b.size(),"Truncated cursor reference");std::uint64_t out=0;for(unsigned n=0;n<count;++n)out|=std::uint64_t(b[at++])<<(8*n);return out;}
} // namespace cursor_test
