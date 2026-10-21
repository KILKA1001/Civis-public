#include "civ5/ui/bnw_whole_raw_label_input.hpp"
#include <stdexcept>
namespace civ5::ui {
BnwWholeRawLabelInput prepare_bnw_whole_raw_label_input(std::string_view borrowed){
    BnwWholeRawLabelInput out;
    // Pin the complete source before any later owner callback; this helper
    // itself has no external callback, borrowed font, GDI or resource state.
    if(borrowed.size()>4096)throw std::runtime_error("Whole raw Label exceeds strict4109004096byte scope");
    out.raw_utf8=borrowed;
    if(out.raw_utf8.find('\0')!=std::string::npos)throw std::runtime_error("Whole raw Label has embedded NUL");
    out.utf16.reserve(out.raw_utf8.size());
    for(std::size_t at=0;at<out.raw_utf8.size();){
        const auto first=static_cast<unsigned char>(out.raw_utf8[at]);
        unsigned trailing=0;std::uint32_t scalar=0,minimum=0;
        if(first<0x80){scalar=first;}
        else if(first>=0xc2&&first<=0xdf){trailing=1;scalar=first&0x1fu;minimum=0x80;}
        else if(first>=0xe0&&first<=0xef){trailing=2;scalar=first&0x0fu;minimum=0x800;}
        else if(first>=0xf0&&first<=0xf4){trailing=3;scalar=first&0x07u;minimum=0x10000;}
        else throw std::runtime_error("Whole raw Label invalid UTF8 lead");
        if(at+trailing>=out.raw_utf8.size())throw std::runtime_error("Whole raw Label incomplete UTF8");
        for(unsigned n=1;n<=trailing;++n){
            const auto next=static_cast<unsigned char>(out.raw_utf8[at+n]);
            if((next&0xc0u)!=0x80u)throw std::runtime_error("Whole raw Label invalid UTF8 continuation");
            scalar=(scalar<<6)|(next&0x3fu);
        }
        if(scalar<minimum||scalar>0x10ffffu||(scalar>=0xd800u&&scalar<=0xdfffu))
            throw std::runtime_error("Whole raw Label invalid UTF8 scalar");
        at+=trailing+1;
        if(scalar<=0xffffu)out.utf16.push_back(static_cast<std::uint16_t>(scalar));
        else{
            scalar-=0x10000u;
            out.utf16.push_back(static_cast<std::uint16_t>((scalar>>10)+0xd800u));
            out.utf16.push_back(static_cast<std::uint16_t>((scalar&0x3ffu)+0xdc00u));
        }
    }
    out.source_bytes_consumed=static_cast<std::uint32_t>(out.raw_utf8.size());
    out.whole_raw_preserved=true;return out;
}
} // namespace civ5::ui
